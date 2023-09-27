/*
 * invisreg - suite of utilities for hiding registry keys
 * Copyright (C) 2023  Sabrina Andersen (NukingDragons)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <invis/reg.h>
#include <invis/ntdll.h>

int reg(int8_t operation, HKEY hive, char *path, ULONG *type, void *value, uint32_t *size)
{
	// Load the internals functions
	init_ntdll();

	int r = 0;

	// *2 here for UTF-16LE
	uint64_t path_len = strlen(path) * 2;
	wchar_t *real_key_path = 0;
	UNICODE_STRING trick_key = { 0 };

	// Ensure the needed arguments are correct
	if (path && path_len)
	{
		// The trick key buffer needs 2 null bytes at the start to become invis
		real_key_path = malloc(path_len);
		trick_key.Buffer = malloc(path_len + 2);

		if (real_key_path && trick_key.Buffer)
		{
			memset(real_key_path, 0, path_len);
			memset(trick_key.Buffer, 0, path_len + 2);

			// To make the key invis, set the offset to 1 on the trick_keyBuffer
			int8_t offset = 1;
			if (operation & MAKE_VISIBLE)
				offset = 0;

			// Its important to set the trick key buffer here before we remove the key name
			// trick_keyBuffer[0] needs to be 0x0000 for the key to be invisible
			MultiByteToWideChar(CP_OEMCP, 0, path, -1, &trick_key.Buffer[offset], path_len + 2);
			trick_key.Length = 2 * (strlen(path) + offset); // Length is the number of bytes - the null byte at the *END* of the trick key path
			trick_key.MaximumLength = 0;

			// Seperate out the parts of the path
			// Split the path and the key name
			char *key_name = 0;
			for (char *a = path; (*a) != 0x00; a++)
				if (*a == '\\')
					key_name = a;

			(*key_name) = 0x00; // This removes the key name
			// Get the real_key_path for RegOpenKeyExW below (Doesn't include keyname)
			MultiByteToWideChar(CP_OEMCP, 0, path, -1, real_key_path, path_len);
		}
		else
			r = -2;
	}
	else
	{
		set_errno(EINVAL);
		r = -1;
	}

	if (!r)
	{
		// VERY EXPERIMENTAL, USE WITH CAUTION
		if (operation & MAKE_KEY)
		{
			HANDLE key;

			OBJECT_ATTRIBUTES attribs = { 0 };

			// Weird structure for NtCreateKey
			attribs.Length = sizeof(OBJECT_ATTRIBUTES);
			attribs.RootDirectory = hive;
			attribs.Attributes = OBJ_KERNEL_HANDLE;
			attribs.ObjectName = &trick_key;
			attribs.SecurityDescriptor = 0;
			attribs.SecurityQualityOfService = 0;

			// Open (or create) the key
			NtCreateKey(&key, KEY_ALL_ACCESS, &attribs, 0, 0, REG_OPTION_NON_VOLATILE, 0);

			// Perform the operation
			switch (operation & OPERATION_MASK)
			{
				case OPERATION_CREATE:
					// This was done above, and is needed for both delete and query anyways
					break;
				case OPERATION_DELETE:
					NtDeleteKey(key);
					break;
				case OPERATION_QUERY:
					// TODO
					break;
				default:
					r = 3;
					break;
			};

			NtClose(key);
		}
		// Perform the operation
		else
		{
			HKEY key;
			if (RegOpenKeyExW(hive, real_key_path, 0, KEY_ALL_ACCESS, &key) == ERROR_SUCCESS)
			{
				NTSTATUS status = STATUS_SUCCESS;
				switch (operation & OPERATION_MASK)
				{
					case OPERATION_CREATE:
						status = NtSetValueKey(key, &trick_key, 0, *type, value, *size);
						break;
					case OPERATION_DELETE:
						status = NtDeleteValueKey(key, &trick_key);
						break;
					case OPERATION_QUERY:
						// Get the size of the buffer
						status = NtQueryValueKey(key, &trick_key, 1, 0, 0, (PULONG) size);

						// Expect buffer too small here
						if (status == STATUS_SUCCESS
						||  status == STATUS_BUFFER_TOO_SMALL)
						{
							// Get the contents of the buffer
							uint8_t *raw = malloc(*size);
							if (raw)
							{
								memset(raw, 0, *size);
								PKEY_VALUE_FULL_INFORMATION info = (PKEY_VALUE_FULL_INFORMATION) raw;
								status = NtQueryValueKey(key, &trick_key, 1, raw, (ULONG) *size, (PULONG) size);

								// Return only the data and the type
								(*type) = info->Type;
								if (info->DataLength)
								{
									*((void **) value) = malloc(info->DataLength);
									if (*((void **) value))
									{
										memset(*((void **) value), 0, info->DataLength);
										memcpy(*((void **) value), &raw[info->DataOffset], info->DataLength);
									}
									else
										r = -5;
								}
								else
									*((void **) value) = 0;

								free(raw);
							}
						}
						break;
					default:
						break;
				};

				r = -4;
				switch (status)
				{
					case STATUS_SUCCESS:
						r = 0;
						set_errno(ESUCCESS);
						break;
					case STATUS_CANNOT_DELETE:
						set_errno(EDELETE);
						break;
					case STATUS_ACCESS_DENIED:
						set_errno(EACCES);
						break;
					case STATUS_INVALID_HANDLE:
						set_errno(EHANDLE);
						break;
					case STATUS_OBJECT_NAME_NOT_FOUND:
						set_errno(EREGUNAVAIL);
						break;
					case STATUS_BUFFER_OVERFLOW:
						/* fall through */
					case STATUS_BUFFER_TOO_SMALL:
						set_errno(EBUFSIZE);
						break;
					case STATUS_INVALID_PARAMETER:
						set_errno(EINVAL);
						break;
					default:
						set_errno(ENTUNK);
						break;
				};
			}
			else
			{
				r = -3;
				set_errno(EOPENKEY);
			}

			RegCloseKey(key);
		}
	}

	if (trick_key.Buffer)
		free(trick_key.Buffer);

	if (real_key_path)
		free(real_key_path);

	return r;
}
