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

int reg(int8_t              operation,
		HKEY                hive,
		char               *path,
		ULONG               type,
		void               *value,
		uint32_t            size,
		struct key_data_t **key_data,
		uint64_t           *num_keys)
{
	// Load the internals functions
	init_ntdll();

	int r = 0;

	// *2 here for UTF-16LE
	uint64_t path_len = strlen(path) * 2;
	UNICODE_STRING trick_key = { 0 };

	// This points to the backslash prior to the key name
	char *key_name = 0;

	int8_t offset = 0;

	// Ensure the needed arguments are correct
	if (path && path_len)
	{
		// Ok, so for some fucking reason when path_len == 102 (becomes 104 later), the final free() at the end
		// of this function will crash. I have no idea why, but lets just avoid path_len == 102 I guess
		// smh my head
		// This is probably due to some type of heap corruption, but this works for now
		if (path_len == 102)
			path_len += 2;

		// The trick key buffer needs 2 null bytes at the start to become invis
		trick_key.Buffer = malloc(path_len + 2);

		if (trick_key.Buffer)
		{
			memset(trick_key.Buffer, 0, path_len + 2);

			// To make the key invis, set the offset to 1 on the trick_keyBuffer
			offset = 1;
			if (operation & MAKE_VISIBLE)
				offset = 0;

			// Its important to set the trick key buffer here before we remove the key name
			// trick_keyBuffer[0] needs to be 0x0000 for the key to be invisible
			MultiByteToWideChar(CP_OEMCP, 0, path, -1, &trick_key.Buffer[offset], path_len + 2);
			trick_key.Length = 2 * (strlen(path) + offset); // Length is the number of bytes - the null byte at the *END* of the trick key path
			trick_key.MaximumLength = 0;

			// Seperate out the parts of the path
			// Split the path and the key name
			for (char *a = path; (*a) != 0x00; a++)
				if (*a == '\\')
					key_name = a;

			(*key_name) = 0x00; // This removes the key name
		}
		else
			r = -2;
	}
	else
	{
		set_errno(EINVAL);
		r = -1;
	}

	if ((operation & OPERATION_MASK) == OPERATION_QUERY
	&& (!key_data || !num_keys))
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
			if (RegOpenKeyExA(hive, path, 0, KEY_ALL_ACCESS, &key) == ERROR_SUCCESS)
			{
				NTSTATUS status = STATUS_SUCCESS;
				switch (operation & OPERATION_MASK)
				{
					case OPERATION_CREATE:
						status = NtSetValueKey(key, &trick_key, 0, type, value, size);
						break;
					case OPERATION_DELETE:
						status = NtDeleteValueKey(key, &trick_key);
						break;
					case OPERATION_QUERY:
						// Get the size of the buffer
						status = NtQueryValueKey(key, &trick_key, 1, 0, 0, (PULONG) &size);

						// Expect buffer too small here if the key exists
						if (status == STATUS_BUFFER_TOO_SMALL)
						{
							// Get the contents of the buffer
							uint8_t *raw = malloc(size);
							*num_keys = 1;
							*key_data = malloc(sizeof(struct key_data_t));
							if (raw && *key_data)
							{
								memset(raw, 0, size);
								memset(*key_data, 0, sizeof(struct key_data_t));

								PKEY_VALUE_FULL_INFORMATION info = (PKEY_VALUE_FULL_INFORMATION) raw;
								status = NtQueryValueKey(key, &trick_key, 1, raw, (ULONG) size, (PULONG) &size);

								// Return only the data, name, and the type
								(*key_data)[0].type = info->Type;
								(*key_data)[0].size = info->DataLength;
								if ((*key_data)[0].value = malloc(info->DataLength))
								{
									memset((*key_data)[0].value, 0, info->DataLength);
									memcpy((*key_data)[0].value, &raw[info->DataOffset], info->DataLength);
								}
								else
									r = -5;

								if ((*key_data)[0].name = malloc(info->NameLength))
								{
									// This is a trick key, probably
									if (((uint8_t *) info->Name)[0] == 0)
									{
										(*key_data)[0].invis = 1;
										offset = 2;
									}
									else
										offset = 0;

									memset((*key_data)[0].name, 0, info->NameLength);
									memcpy((*key_data)[0].name, ((uint8_t *) info->Name) + offset, info->NameLength - offset);
								}
								else
									r = -6;
							}

							if (raw)
								free(raw);
						}
						// Check if it's a key instead of a key value
						else if (status == STATUS_OBJECT_NAME_NOT_FOUND)
						{
							// To check if this is truly a key, close the old HKEY
							// Then append the key name again, before attempting to open the key again
							RegCloseKey(key);
							*key_name = '\\';
							if (RegOpenKeyExA(hive, path, 0, KEY_ALL_ACCESS, &key) == ERROR_SUCCESS)
							{
								// Fetch the number of subkeys
								status = STATUS_BUFFER_TOO_SMALL;
								while (status == STATUS_BUFFER_TOO_SMALL)
									status = NtEnumerateValueKey(key, (*num_keys)++, 1, 0, 0, (PULONG) &size);

								// This means that we have a key with key values
								if (status == STATUS_NO_MORE_ENTRIES)
								{
									*key_data = malloc(sizeof(struct key_data_t) * *num_keys);
									if (*key_data)
									{
										memset(*key_data, 0, sizeof(struct key_data_t) * *num_keys);
										for (uint64_t i = 0; i < *num_keys; i++)
										{
											status = NtEnumerateValueKey(key, i, 1, 0, 0, (PULONG) &size);

											if (status != STATUS_BUFFER_TOO_SMALL)
												break;

											uint8_t *raw = malloc(size);
											if (raw)
											{
												memset(raw, 0, size);

												PKEY_VALUE_FULL_INFORMATION info = (PKEY_VALUE_FULL_INFORMATION) raw;
												status = NtEnumerateValueKey(key, i, 1, raw, (ULONG) size, (PULONG) &size);

												// Return only the name, and the type
												// +2 in case of string without null bytes
												(*key_data)[i].type = info->Type;
												(*key_data)[i].size = info->DataLength;
												if ((*key_data)[i].value = malloc(info->DataLength + 2))
												{
													memset((*key_data)[i].value, 0, info->DataLength + 2);
													memcpy((*key_data)[i].value, &raw[info->DataOffset], info->DataLength);
												}
												else
													r = -6;

												if ((*key_data)[i].name = malloc(info->NameLength))
												{
													// This is a trick key, probably
													if (((uint8_t *) info->Name)[0] == 0)
													{
														(*key_data)[i].invis = 1;
														offset = 2;
													}
													else
														offset = 0;

													memset((*key_data)[i].name, 0, info->NameLength);
													memcpy((*key_data)[i].name, ((uint8_t *) info->Name) + offset,
																						info->NameLength - offset);
												}
												else
													r = -7;
											}

											if (raw)
												free(raw);
										}
									}
								}
							}
							else
							{
								r = -3;
								set_errno(EOPENKEY);
							}
						}
						break;
					default:
						break;
				};

				r = -4;
				switch (status)
				{
					// Not a valid error, really
					case STATUS_NO_MORE_ENTRIES:
						/* fall through */
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

	return r;
}

void free_key_data(struct key_data_t *key_data, uint64_t num_keys)
{
	if (key_data)
	{
		for (uint64_t i = 0; i < num_keys; i++)
		{
			if (key_data[i].name)
				free(key_data[i].name);

			if (key_data[i].value)
				free(key_data[i].value);
		}

		free(key_data);
	}
}
