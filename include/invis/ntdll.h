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

#ifndef _NTDLL_H_
#define _NTDLL_H_

#include <windows.h>

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

typedef struct _KEY_VALUE_FULL_INFORMATION {
	ULONG TitleIndex;
	ULONG Type;
	ULONG DataOffset;
	ULONG DataLength;
	ULONG NameLength;
	WCHAR Name[1];
} KEY_VALUE_FULL_INFORMATION, * PKEY_VALUE_FULL_INFORMATION;

#define OBJ_KERNEL_HANDLE 0x00000200

// Internals function declarations
typedef NTSTATUS (*_NtCreateKey)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, ULONG, PUNICODE_STRING, ULONG, PULONG);
typedef NTSTATUS (*_NtSetValueKey)(HANDLE, PUNICODE_STRING, ULONG, ULONG, PVOID, ULONG);
typedef NTSTATUS (*_NtDeleteKey)(HANDLE);
typedef NTSTATUS (*_NtDeleteValueKey)(HANDLE, PUNICODE_STRING);
typedef NTSTATUS (*_NtQueryKey)(HANDLE, ULONG, PVOID, ULONG, PULONG);
typedef NTSTATUS (*_NtQueryValueKey)(HANDLE, PUNICODE_STRING, ULONG, PVOID, ULONG, PULONG);
typedef NTSTATUS (*_NtClose)(HANDLE);

// Internals functions
extern _NtCreateKey      NtCreateKey;
extern _NtSetValueKey    NtSetValueKey;
extern _NtDeleteKey      NtDeleteKey;
extern _NtDeleteValueKey NtDeleteValueKey;
extern _NtQueryKey       NtQueryKey;
extern _NtQueryValueKey  NtQueryValueKey;
extern _NtClose          NtClose;

void init_ntdll(void);

#endif
