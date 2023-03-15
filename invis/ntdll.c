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

#include <invis/ntdll.h>

_NtCreateKey      NtCreateKey;
_NtSetValueKey    NtSetValueKey;
_NtDeleteKey      NtDeleteKey;
_NtDeleteValueKey NtDeleteValueKey;
_NtQueryKey       NtQueryKey;
_NtQueryValueKey  NtQueryValueKey;
_NtClose          NtClose;

void init_ntdll(void)
{
	if (!NtCreateKey
	||  !NtSetValueKey
	||  !NtDeleteKey
	||  !NtDeleteValueKey
	||  !NtQueryKey
	||  !NtQueryValueKey
	||  !NtClose)
	{
		HANDLE ntdll     = LoadLibraryA("ntdll.dll");
		NtCreateKey      = (_NtCreateKey)      GetProcAddress(ntdll, "NtCreateKey");
	        NtSetValueKey    = (_NtSetValueKey)    GetProcAddress(ntdll, "NtSetValueKey");
	        NtDeleteKey      = (_NtDeleteKey)      GetProcAddress(ntdll, "NtDeleteKey");
	        NtDeleteValueKey = (_NtDeleteValueKey) GetProcAddress(ntdll, "NtDeleteValueKey");
		NtQueryKey       = (_NtQueryKey)       GetProcAddress(ntdll, "NtQueryKey");
		NtQueryValueKey  = (_NtQueryValueKey)  GetProcAddress(ntdll, "NtQueryValueKey");
		NtClose          = (_NtClose)          GetProcAddress(ntdll, "NtClose");
	}
}
