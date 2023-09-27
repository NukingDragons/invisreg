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

#ifndef _CUSTOM_ERRNOS_H_
#define _CUSTOM_ERRNOS_H_

__push_errno_defs
#undef __CUSTOM_ERRNO_DEFS
#define __CUSTOM_ERRNO_DEFS __pop_errno_defs __CUSTOM_ERRNO_DEFS	\
	ETOOMANY,														\
	EMULTIOPS,														\
	EMISSINGARGVAL,													\
	ETYPE,															\
	EKEY,															\
	EHIVE,															\
	ENEEDVAL,														\
	EOPENKEY,														\
	ENTUNK,															\
	EREGUNAVAIL,													\
	EBUFSIZE,														\
	EDELETE,														\
	EHANDLE,

__push_errno_strs
#undef __CUSTOM_ERRNO_STRS
#define __CUSTOM_ERRNO_STRS __pop_errno_strs __CUSTOM_ERRNO_STRS	\
	"Too many of the same argument specified",						\
	"Too many operations specified",								\
	"Argument expecting value, none provided",						\
	"Invalid registry value type provided",							\
	"Invalid registry key",											\
	"Invalid hive provided",										\
	"Specfied type expects a value",								\
	"Failed to open the registry key",								\
	"Unknown error from the ntdll API",								\
	"Registry key is unavailable",									\
	"The query buffer is too small",								\
	"Unable to delete the registry key",							\
	"Invalid handle",

#endif
