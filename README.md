# invisreg
Invisible Registry Key Management Utility

# Building

Ensure that you have MinGW installed, and then run `make`. This will produce the binary in the current folder.

# Usage

Running the command by itself or with --help/-h results in the following usage prompt. All of the details necessary to use this application exist there as well.
This tool is able to create, edit, and delete invisible registry keys. Further, it's able to locate them and render invisible keys by using the query functionality.

```
Usage: .\invisreg.exe [options]
Options:
        --help,-h               Display this help
        --create,-c             Create an invisible registry key
        --edit,-e               Edit an invisible registry key
        --delete,-d             Delete an invisible registry key
        --query,-q              Query an invisible registry key
        --visible,-V            Make the key visible
        --type,-t               Specify the data type of the registry key
        --key,-k                The key to create as an invisible key
        --value,-v              The data of the specified type to place into the key

Only the following hives are supported:
 HKLM          = HKEY_LOCAL_MACHINE
 HKCU or HCU   = HKEY_CURRENT_USER
 HKCR          = HKEY_CLASSES_ROOT
 HKCC          = HKEY_CURRENT_CONFIG
 HKU           = HKEY_USERS
Only the following types are currently supported:
 REG_NONE      = Value will be ignored, and is the default type
 REG_SZ        = Value is expected to be a string
 REG_EXPAND_SZ = Value is expected to be a string
 REG_DWORD     = Value is expected to be a 32-bit integer
 REG_QWORD     = Value is expected to be a 64-bit integer
 REG_BINARY    = Value is expected to be the name of a file
                 This file is read into this program and placed into the key

Examples:
 invisreg --key HKLM:\SOFTWARE\MICROSOFT\Windows\CurrentVersion\Run\KeyName --type REG_SZ --create --value "calc.exe"
 invisreg --key HKLM:\SOFTWARE\MICROSOFT\Windows\CurrentVersion\Run\KeyName --type REG_DWORD --edit --value 1337
 invisreg --key HKLM:\SOFTWARE\MICROSOFT\Windows\CurrentVersion\Run\KeyName --delete
 invisreg --key HKLM:\SOFTWARE\MICROSOFT\Windows\CurrentVersion\Run\KeyName --query
```

# Technical Explanation

Within the Windows OS, Microsoft has two different sets of API's that can be used to interface with the registry. These API's are intended to be used in different parts of the OS: Userland via the functions located within "kernel32.dll", and within kernel mode/drivers located within "ntdll.dll".

To open a registry key using the intended userland functions, the "RegOpenExA" function can be used like so:

```c
// Open HKLM:\SOFTWARE\MICROSOFT\Windows\CurrentVersion\Run\KeyName
HKEY KeyName;
if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "\\SOFTWARE\\MICROSOFT\\Windows\\CurrentVersion\\Run\\KeyName", 0, KEY_ALL_ACCESS, &KeyName) == ERROR_SUCCESS)
{
	// Do something with the KeyName HKEY
}
else
{
	// Failed to open the key
}
```

Notice how the path to "KeyName" is a normal string. In order for the API to understand where the string ends, it must end in a NULL byte. If the string is in UTF-16LE, which is the internal encoding of the registry, then it must end in 2 NULL bytes. In this case, the function will automatically convert the ASCII string into UTF-16LE before performing the operation.

Now, let's take a look at the internal API's usage. To open a key using the "NtOpenKey" function, the following code muse be used:

```c
// Open HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run\KeyName
HKEY KeyName;

// Set up the string structure
UNICODE_STRING trick_key = { 0 };
// The buffer is the actual string, but this does NOT need the NULL bytes
trick_key.Buffer = L"\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run\\KeyName";
// Instead of the NULL bytes, this Length parameter is used instead
trick_key.Length = wcslen(trick_key.Buffer) * 2;
// Setting this to 0 doesn't hurt anything
trick_key.MaximumLength = 0;

// To open the key, this attributes structure must be populated
OBJECT_ATTRIBUTES attributes = { 0 };
attributes.Length = sizeof(OBJECT_ATTRIBUTES);
attributes.Attributes = OBJ_KERNEL_HANDLE;
attributes.SecurityDescriptor = 0;
attributes.SecurityQualityOfService = 0;
// RootDirectory contains the hive, object name contains the path
attributes.RootDirectory = HKEY_LOCAL_MACHINE;
attributes.ObjectName = &trick_key;
if (NtOpenKey(&KeyName, KEY_ALL_ACCESS, &attributes) == ERROR_SUCCESS)
{
	// Do something with the KeyName HKEY
}
else
{
	// Failed to open the key
}
```

As can be seen with the second example, the string is stored in a different structure that explicitly sets the length, and does not depend on the NULL bytes at the end of the string. This begs the question, what if the NULL bytes are placed into the first 2 bytes of the string, with the length properly set? The result of performing this trick using the internal "NtSetValueKey" function results in the keys being invisible to userland applications, yet still visible to the kernel and drivers.

![Pasted image 20231002135146](https://github.com/NukingDragons/invisreg/assets/9376673/6a915026-b925-4c9b-b079-482d355ce88d)


As far as IOC's go, it appears that any tool that monitors when the registry is affected will still fire. However, if the tool does not use the NT internals API, then it will fail to render the key name as can be seen in process monitor. Note, that the data placed into the registry is still visible, but the name of the key itself is not. Further, deleting this key is impossible using the default userland registry utilities. Instead, the key must be deleted in the same way that it was created, using the NT internals API.

![Pasted image 20231002141525](https://github.com/NukingDragons/invisreg/assets/9376673/ec8a7e3e-c466-46be-ad82-d7b9b5238b93)
