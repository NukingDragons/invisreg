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

#include <stdio.h>
#include <stdlib.h>

#include <invis/reg.h>

void usage(void)
{
	fprintf(stderr, "Usage: invisreg [operation] [path] [type] [value]\n"
			"       operations:\n"
			"         create - Create a new invisible registry key\n"
			"         edit   - Edit an existing invisible registry key\n"
			"         delete - Delete an existing invisible registry key\n"
			"         query  - Query an invisible registry key\n"
			"       path:\n"
			"         Like this - HKLM:\\PATH\\TO\\KEY\n"
			"         supported hives:\n"
			"           HKLM        - HKEY_LOCAL_MACHINE\n"
			"           HKCU or HCU - HKEY_CURRENT_USER\n"
			"           HKCR        - HKEY_CLASSES_ROOT\n"
			"           HKCC        - HKEY_CURRENT_CONFIG\n"
			"           HKU         - HKEY_USERS\n"
			"       type:\n"
			"         REG_SZ        - Value is expected to be a string\n"
			"         REG_DWORD     - Value is expected to be a 32-bit integer\n"
			"         REG_QWORD     - Value is expected to be a 64-bit integer\n"
			"         REG_BINARY    - Value is expected to be the name of a file\n"
			"       value:\n"
			"         ...\n\n"
			"Examples:\n"
			"       invisreg create HKLM:\\SOFTWARE\\MICROSOFT\\Windows\\CurrentVersion\\Run\\KeyName REG_SZ \"calc.exe\"\n"
			"       invisreg edit HKLM:\\SOFTWARE\\MICROSOFT\\Windows\\CurrentVersion\\Run\\KeyName REG_DWORD 1337\n"
			"       invisreg delete HKLM:\\SOFTWARE\\MICROSOFT\\Windows\\CurrentVersion\\Run\\KeyName\n"
			"       invisreg query HKLM:\\SOFTWARE\\MICROSOFT\\Windows\\CurrentVersion\\Run\\KeyName\n"
			"\n");
}

int main(int argc, char **argv)
{
	if (argc > 1
	&& (((!strcmp(argv[1], "create") || !strcmp(argv[1], "edit"))  && argc == 5)
	||  ((!strcmp(argv[1], "delete") || !strcmp(argv[1], "query")) && argc == 3)))
	{
		int8_t operation = 0;
		if (!strcmp(argv[1], "create") || !strcmp(argv[1], "edit"))
			operation = OPERATION_CREATE;
		else if (!strcmp(argv[1], "delete"))
			operation = OPERATION_DELETE;
		else if (!strcmp(argv[1], "query"))
			operation = OPERATION_QUERY;

		ULONG type = REG_NONE;
		void *raw_value = 0;
		uint32_t data_size = 0;
		if (argc == 5)
		{
			FILE *binary_file = 0;
			// Get the type and the corresponding value
			if (!strcmp(argv[3], "REG_SZ"))
			{
				type = REG_SZ;
				data_size = strlen(argv[4]) * 2;
			}
			else if (!strcmp(argv[3], "REG_DWORD"))
			{
				type = REG_DWORD;
				data_size = sizeof(int32_t);
			}
			else if (!strcmp(argv[3], "REG_QWORD"))
			{
				type = REG_QWORD;
				data_size = sizeof(int64_t);
			}
			else if (!strcmp(argv[3], "REG_BINARY"))
			{
				type = REG_BINARY;
				binary_file = fopen(argv[4], "r");

				if (!binary_file)
				{
					fprintf(stderr, "Failed to open file\n");
					return 1;
				}

				// Get the size
				fseek(binary_file, 0, SEEK_END);
				data_size = ftell(binary_file);
				rewind(binary_file);
			}

			// Allocate memory for the registry value
			raw_value = malloc(data_size);
			if (!raw_value)
			{
				fprintf(stderr, "You outta memory foo\n");
				return 1;
			}

			// Render the proper type
			switch (type)
			{
				// Convert to UTF-16LE
				case REG_SZ:
					MultiByteToWideChar(CP_OEMCP, 0, argv[4], -1, (wchar_t *) raw_value, data_size);
					break;
				// scanf for 32-bit integer
				case REG_DWORD:
					sscanf(argv[4], "%d", ((int32_t *) raw_value));
					break;
				// scanf for 64-bit integer
				case REG_QWORD:
					sscanf(argv[4], "%ld", ((int64_t *) raw_value));
					break;
				// Read a file and load it into raw_value
				case REG_BINARY:
					fread(raw_value, 1, data_size, binary_file);
					fclose(binary_file);
					break;
				default:
					fprintf(stderr, "Invalid type\n");
					usage();
					if (raw_value)
						free(raw_value);
					if (binary_file)
						fclose(binary_file);
					return 1;
			};
		}

		int r = 0;
		if (operation == OPERATION_QUERY)
		{
			r = reg(operation, argv[2], &type, &raw_value, &data_size);

			// TODO: Implement more types here
			if (type == REG_SZ)
				printf("\"%ls\"\n", ((wchar_t *) raw_value));
			else if (type == REG_DWORD)
				printf("%d\n", ((uint32_t *) raw_value)[0]);
			else if (type == REG_QWORD)
				printf("%d\n", ((uint64_t *) raw_value)[0]);
			else if (type == REG_BINARY)
			{
				for (uint8_t i = 0; i > data_size; i++)
					putchar(((uint8_t *) raw_value)[i]);
				putchar('\n');
			}
			else if (type == REG_NONE)
				printf("Registry key doesn't exist...\n");
			else
				fprintf(stderr, "I don't know how to read this type (%u)...\n", type);
		}
		else
			r = reg(operation, argv[2], &type, raw_value, &data_size);

		// Error codes from invisreg
		if (r)
		{
			switch (r)
			{
				case 1:
					fprintf(stderr, "Invalid arguments to internal function\n");
					break;
				case 2:
					fprintf(stderr, "Invalid hive\n");
					break;
				case 3:
					fprintf(stderr, "Invalid operation\n");
					break;
				case 4:
					fprintf(stderr, "Out of memory\n");
					break;
			};

			usage();
		}
		else
			printf("Completed successfully\n");

		if (raw_value)
			free(raw_value);
	}
	else
	{
		fprintf(stderr, "Invalid number of arguments\n");
		usage();
	}

	return 0;
}
