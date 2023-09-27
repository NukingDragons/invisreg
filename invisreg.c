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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <error.h>
#include <invis/reg.h>

// Name of the program if argv[0] fails
#define NAME "invisreg"

struct args_t
{
	// Options
	uint8_t help:1;
	uint8_t create:1;
	uint8_t edit:1;
	uint8_t delete:1;
	uint8_t query:1;
	uint8_t visible:1;

	ULONG type;

	HKEY hive;
	char *path;

	void *value;
	uint32_t value_size;
};

void usage(char *name, FILE *f)
{
	// Set the name for the usage prompt
	char *n = NAME;
	if (name
	&&  strlen(name))
		n = name;

	fprintf(f,
			"Usage: %s [options]\n"
			"Options:\n"
			"\t--help,-h\t\tDisplay this help\n"
			"\t--create,-c\t\tCreate an invisible registry key\n"
			"\t--edit,-e\t\tEdit an invisible registry key\n"
			"\t--delete,-d\t\tDelete an invisible registry key\n"
			"\t--query,-q\t\tQuery an invisible registry key\n"
			"\t--visible,-V\t\tMake the key visible\n"
			"\t--type,-t\t\tSpecify the data type of the registry key\n"
			"\t--key,-k\t\tThe key to create as an invisible key\n"
			"\t--value,-v\t\tThe data of the specified type to place into the key\n"
			"\n"
			"Only the following hives are supported:\n"
			" HKLM        = HKEY_LOCAL_MACHINE\n"
			" HKCU or HCU = HKEY_CURRENT_USER\n"
			" HKCR        = HKEY_CLASSES_ROOT\n"
			" HKCC        = HKEY_CURRENT_CONFIG\n"
			" HKU         = HKEY_USERS\n"
			"Only the following types are currently supported:\n"
			" REG_NONE    = Value will be ignored, and is the default type\n"
			" REG_SZ      = Value is expected to be a string\n"
			" REG_DWORD   = Value is expected to be a 32-bit integer\n"
			" REG_QWORD   = Value is expected to be a 64-bit integer\n"
			" REG_BINARY  = Value is expected to be the name of a file\n"
			"               This file is read into this program and placed into the key\n"
			"\n"
			"Examples:\n"
			" " NAME " --key HKLM:\\SOFTWARE\\MICROSOFT\\Windows\\CurrentVersion\\Run\\KeyName --type REG_SZ --create --value \"calc.exe\"\n"
			" " NAME " --key HKLM:\\SOFTWARE\\MICROSOFT\\Windows\\CurrentVersion\\Run\\KeyName --type REG_DWORD --edit --value 1337\n"
			" " NAME " --key HKLM:\\SOFTWARE\\MICROSOFT\\Windows\\CurrentVersion\\Run\\KeyName --delete\n"
			" " NAME " --key HKLM:\\SOFTWARE\\MICROSOFT\\Windows\\CurrentVersion\\Run\\KeyName --query\n"
			,
			n);
}

struct args_t parse_args(int32_t argc, char **argv, int32_t min_args)
{
	set_errno(ESUCCESS);

	char *value = 0;

	struct args_t args;
	memset(&args, 0, sizeof(struct args_t));

	// Ensure arguments provided and that the minimum are provided
	// argc should be set to at least 1 for the program name
	if (argc < (min_args + 1))
		set_errno(ETOOFEW);

	if (!errno
	&&  argv)
	{
		// Scan for arguments
		// Start at 1 so that the name of the program is not a false positive
		for (int32_t i = 1; i < argc; i++)
		{
#undef check_arg
#define check_arg(full, small)		  (!strncmp(full, argv[i], strlen(full)) || !strncmp(small, argv[i], strlen(small)))
			if      (check_arg("--help", "-h"))
				args.help = 1;
			else if (check_arg("--create", "-c"))
			{
				// Only allow a single operation to be specified
				if      (args.create)
					set_errno(ETOOMANY);
				// Only allow a single one of these operations
				else if (args.edit || args.delete || args.query)
					set_errno(EMULTIOPS);

				args.create = 1;
			}
			else if (check_arg("--edit", "-e"))
			{
				// Only allow a single operation to be specified
				if      (args.edit)
					set_errno(ETOOMANY);
				// Only allow a single one of these operations
				else if (args.create || args.delete || args.query)
					set_errno(EMULTIOPS);

				args.edit = 1;
			}
			else if (check_arg("--delete", "-d"))
			{
				// Only allow a single operation to be specified
				if      (args.delete)
					set_errno(ETOOMANY);
				// Only allow a single one of these operations
				else if (args.create || args.edit || args.query)
					set_errno(EMULTIOPS);

				args.delete = 1;
			}
			else if (check_arg("--query", "-q"))
			{
				// Only allow a single operation to be specified
				if      (args.query)
					set_errno(ETOOMANY);
				// Only allow a single one of these operations
				else if (args.create || args.edit || args.delete)
					set_errno(EMULTIOPS);

				args.query = 1;
			}
			else if (check_arg("--visible", "-V"))
			{
				if (args.visible)
					set_errno(ETOOMANY);

				args.visible = 1;
			}
			else if (check_arg("--type", "-t"))
			{
				// Only allow a single one of these flags
				if (args.type)
					set_errno(ETOOMANY);
				else
				{
					// Ensure that the arguments expected value is provided
					if (i <= argc + 1)
					{
						char *type = argv[++i];
						if      (!strcmp(type, "REG_NONE"))
							args.type = REG_NONE;
						else if (!strcmp(type, "REG_SZ"))
							args.type = REG_SZ;
						else if (!strcmp(type, "REG_DWORD"))
							args.type = REG_DWORD;
						else if (!strcmp(type, "REG_QWORD"))
							args.type = REG_QWORD;
						else if (!strcmp(type, "REG_BINARY"))
							args.type = REG_BINARY;
						else
							set_errno(ETYPE);
					}
					else
						set_errno(EMISSINGARGVAL);
				}
			}
			else if (check_arg("--key", "-k"))
			{
				// Only allow a single one of these flags
				if (args.hive || args.path)
					set_errno(ETOOMANY);
				else
				{
					// Ensure that the arguments expected value is provided
					if (i <= argc + 1)
					{
						char *key = argv[++i];

						for (char *a = key; (*a) != 0x00; a++)
						{
							if (*a == ':')
							{
								(*a) = 0x00;
								args.path = a + 2;
							}
						}

						// Ensure the key is valid
						if (args.path)
						{
							if      (!strcmp(key, "HKLM"))
								args.hive = HKEY_LOCAL_MACHINE;
							else if (!strcmp(key, "HKCU") || !strcmp(key, "HCU"))
								args.hive = HKEY_CURRENT_USER;
							else if (!strcmp(key, "HKCR"))
								args.hive = HKEY_CLASSES_ROOT;
							else if (!strcmp(key, "HKCC"))
								args.hive = HKEY_CURRENT_CONFIG;
							else if (!strcmp(key, "HKU"))
								args.hive = HKEY_USERS;
							else
								set_errno(EHIVE);
						}
						else
							set_errno(EKEY);
					}
					else
						set_errno(EMISSINGARGVAL);
				}
			}
			else if (check_arg("--value", "-v"))
			{
				// Only allow a single one of these flags
				if (args.value)
					set_errno(ETOOMANY);
				else
				{
					// Ensure that the arguments expected value is provided
					if (i <= argc + 1)
						value = argv[++i];
					else
						set_errno(EMISSINGARGVAL);
				}
			}
			else
				set_errno(EUNKARG);
#undef check_arg

			if (errno || args.help)
				break;
		}

		// Create/edit need type and value
		if (!errno)
		{
			if ((args.create || args.edit)
			&&   args.type != REG_NONE)
			{
				if (value)
				{
					switch (args.type)
					{
						case REG_SZ:
							// * 2 for UTF-16LE
							args.value_size = strlen(value) * 2;
							args.value = malloc(args.value_size);

							if (args.value)
							{
								memset(args.value, 0, args.value_size);
								MultiByteToWideChar(CP_OEMCP, 0, value, -1,
												   (wchar_t *) args.value,
												               args.value_size);
							}
							break;
						case REG_DWORD:
							args.value_size = sizeof(uint32_t);
							args.value = malloc(args.value_size);

							if (args.value)
							{
								memset(args.value, 0, args.value_size);
								sscanf(value, "%d", (uint32_t *) args.value);
							}
							break;
						case REG_QWORD:
							args.value_size = sizeof(uint64_t);
							args.value = malloc(args.value_size);

							if (args.value)
							{
								memset(args.value, 0, args.value_size);
								sscanf(value, "%ld", (uint64_t *) args.value);
							}
							break;
						case REG_BINARY:
							FILE *f = fopen(value, "r");
							if (f)
							{
								fseek(f, 0, SEEK_END);
								args.value_size = ftell(f);
								rewind(f);
								args.value = malloc(args.value_size);

								if (args.value)
								{
									memset(args.value, 0, args.value_size);
									fread(args.value, 1, args.value_size, f);
								}

								fclose(f);
							}
							break;
						default:
							break;
					};
				}
				else
					set_errno(ENEEDVAL);
			}
		}
	}

	return args;
}

int32_t main(int32_t argc, char **argv)
{
	int32_t r = 0;
	struct args_t args = parse_args(argc, argv, 3);

	if (!errno)
	{
		if (args.help)
			usage(argv[0], stdout);

		uint8_t operation = 0;

		void *value = args.value;

		if (args.create || args.edit)
			operation |= OPERATION_CREATE;

		if (args.delete)
			operation |= OPERATION_DELETE;

		if (args.query)
		{
			operation |= OPERATION_QUERY;

			// When the op is a query, this becomes an output variable
			args.value = 0;
			value = &args.value;
		}

		if (args.visible)
			operation |= MAKE_VISIBLE;

		if (!reg(operation, args.hive, args.path, &args.type, value, &args.value_size))
		{
			if (args.query)
				switch (args.type)
				{
					case REG_SZ:
						printf("REG_SZ\t%ls\n", (wchar_t *) args.value);
						break;
					case REG_DWORD:
						printf("REG_DWORD\t%d\n", *(uint32_t *) args.value);
						break;
					case REG_QWORD:
						printf("REG_QWORD\t%ld\n", *(uint64_t *) args.value);
						break;
					case REG_BINARY:
						printf("REG_BINARY\tTODO\n");
						break;
					case REG_NONE:
						printf("REG_NONE\n");
						break;
					default:
						printf("REG_UNK\n");
						break;
				};

			printf("Completed successfully!\n");
		}
		else
		{
			r = 1;
			fprintf(stderr, "Error: %s\n", errorstr(errno));
		}
	}
	else
	{
		fprintf(stderr, "Error: %s\n", errorstr(errno));

		if (argv)
			usage(argv[0], stderr);
		else
			usage(0, stderr);

		r = 1;
	}

	if (args.value)
		free(args.value);

	return r;
}
