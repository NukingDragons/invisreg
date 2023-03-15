/*
 *    invisreg - suite of utilities for hiding registry keys
 *    Copyright (C) 2023  Sabrina Andersen (NukingDragons)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <invis/reg.h>
#include <invis/ntdll.h>

int reg(int8_t operation, char *key, ULONG *type, void *value, uint32_t *size)
{
	// Load the internals functions
	init_ntdll();

	int r = 0;

	uint64_t key_len = strlen(key);
	wchar_t *RealKeyPath = 0;
	UNICODE_STRING TrickKey = { 0 };

	HKEY hive = 0;

	// Ensure the arguments are correct
	if (key && strlen(key)
	&& (((operation & OPERATION_MASK) != OPERATION_DELETE && type && value && size)
	||  ((operation & OPERATION_MASK) == OPERATION_DELETE)))
	{
		RealKeyPath = malloc(key_len);
		TrickKey.Buffer = malloc(key_len + 2);

		if (RealKeyPath && TrickKey.Buffer)
		{
			memset(RealKeyPath, 0, key_len);
			memset(TrickKey.Buffer, 0, key_len + 2);

			// Seperate out the parts of the path
			// Split the path and the key name
			char *path = 0;
			char *key_name = 0;
			for (char *a = key; (*a) != 0x00; a++)
			{
				if (*a == ':')
				{
					(*a) = 0x00;
					path = a + 2;
				}
				if (*a == '\\')
					key_name = a;
			}

			// To make the key invis, set the offset to 1 on the TrickKeyBuffer
			int8_t offset = 1;
			if (operation & MAKE_VISIBLE)
				offset = 0;

			// Its important to set the TrickKeyBuffer here before we remove the key
			// TrickKeyBuffer[0] needs to be 0x0000 for this trick to work
			MultiByteToWideChar(CP_OEMCP, 0, path, -1, &TrickKey.Buffer[offset], key_len + 2);
			TrickKey.Length = 2 * (strlen(path) + offset); // Length is the number of bytes - the null byte at the *END* of the trick key path
			TrickKey.MaximumLength = 0;
			(*key_name) = 0x00; // This removes the key
			// Get the RealKeyPath for RegOpenKeyExW below (Doesn't include keyname)
			MultiByteToWideChar(CP_OEMCP, 0, path, -1, RealKeyPath, key_len);

			// Get the hive
			if (!strcmp(key, "HKLM"))
				hive = HKEY_LOCAL_MACHINE;
			else if (!strcmp(key, "HKCU") || !strcmp(key, "HCU"))
				hive = HKEY_CURRENT_USER;
			else if (!strcmp(key, "HKCR"))
				hive = HKEY_CLASSES_ROOT;
			else if (!strcmp(key, "HKCC"))
				hive = HKEY_CURRENT_CONFIG;
			else if (!strcmp(key, "HKU"))
				hive = HKEY_USERS;
			else
				r = 2;
		}
		else
			r = 4;
	}
	else
		r = 1;

	if (!r)
	{
		if (operation & MAKE_KEY)
		{
			HANDLE Key;

			OBJECT_ATTRIBUTES attribs = { 0 };

			// Weird structure for NtCreateKey
			attribs.Length = sizeof(OBJECT_ATTRIBUTES);
			attribs.RootDirectory = hive;
			attribs.Attributes = OBJ_KERNEL_HANDLE;
			attribs.ObjectName = &TrickKey;
			attribs.SecurityDescriptor = 0;
			attribs.SecurityQualityOfService = 0;

			// Open (or create) the key
			NtCreateKey(&Key, KEY_ALL_ACCESS, &attribs, 0, 0, REG_OPTION_NON_VOLATILE, 0);

			// Perform the operation
			switch (operation & OPERATION_MASK)
			{
				case OPERATION_CREATE:
					// This was done above, and is needed for both delete and query anyways
					break;
				case OPERATION_DELETE:
					NtDeleteKey(Key);
					break;
				case OPERATION_QUERY:
					// TODO
					break;
				default:
					r = 3;
					break;
			};

			NtClose(Key);
		}
		// Perform the operation
		else
		{
			HKEY Key;
			RegOpenKeyExW(hive, RealKeyPath, 0, KEY_ALL_ACCESS, &Key);
			switch (operation & OPERATION_MASK)
			{
				case OPERATION_CREATE:
					NtSetValueKey(Key, &TrickKey, 0, *type, value, *size);
					break;
				case OPERATION_DELETE:
					NtDeleteValueKey(Key, &TrickKey);
					break;
				case OPERATION_QUERY:
					// Get the size of the buffer
					NtQueryValueKey(Key, &TrickKey, 1, 0, 0, (PULONG) size);

					// Get the contents of the buffer
					uint8_t *raw = malloc(*size);
					if (raw)
					{
						memset(raw, 0, *size);
						PKEY_VALUE_FULL_INFORMATION info = (PKEY_VALUE_FULL_INFORMATION) raw;
						NtQueryValueKey(Key, &TrickKey, 1, raw, (ULONG) *size, (PULONG) size);

						// Return only the data and the type
						(*type) = info->Type;
						*((void **) value) = malloc(info->DataLength);
						if (*((void **) value))
						{
							memset(*((void **) value), 0, info->DataLength);
							memcpy(*((void **) value), &raw[info->DataOffset], info->DataLength);
						}
						else
							r = 4;
						
						free(raw);
					}
					else
						r = 4;
					break;
				default:
					r = 3;
					break;
			};
			RegCloseKey(Key);
		}

	}

	if (TrickKey.Buffer)
		free(TrickKey.Buffer);

	if (RealKeyPath)
		free(RealKeyPath);

	return r;
}
