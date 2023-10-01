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

#ifndef _REG_H_
#define _REG_H_

#include <stdint.h>
#include <windows.h>
#include <error.h>

#define OPERATION_CREATE	0
#define OPERATION_EDIT   	0
#define OPERATION_DELETE 	1
#define OPERATION_QUERY  	2

#define OPERATION_MASK  3
#define MAKE_VISIBLE	(1<<3)
#define MAKE_KEY		(1<<4)

struct key_data_t
{
	ULONG type;
	wchar_t *name;
	void *value;
	uint32_t size;

	int8_t invis;
};

/*
 * For value keys:
 *  On create/edit: type, value, and size are input variables and are required.
 *                  key_data and num_keys are always ignored
 *  On delete/query: type, value, and size are ignored
 *  On query: key_data and num_keys are outputs
 * For keys (MAKE_KEY):
 *  type, value, and size are all ignored
 */
int reg(int8_t              operation,
		HKEY                hive,
		char               *path,
		ULONG               type,
		void               *value,
		uint32_t            size,
		struct key_data_t **key_data,
		uint64_t           *num_keys);

void free_key_data(struct key_data_t *key_data, uint64_t num_keys);

#endif
