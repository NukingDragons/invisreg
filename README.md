# invisreg
**Invisible Registry Key Management Utility**

- This can create, edit, delete, and query invisible registry keys. Query only works if you already know the key name beforehand, I have yet to implement a registry wide scanner of invisible keys. See the TODO section.

- This tool was made to be used in penetration testing engagements, I'm not responsible for how you use this.

- The code isn't beautiful yet, and there is a lot to be done. I intend on improving this tool as I use it and come across more interesting use cases for invisible registry keys.

---
> [!NOTE]
> This tool is still beta and requires a lot of refinement and tweaks.
---
 

### Changelog
```
v1.0.0
  - Initial release.
  
v2.0.0
  - Introduced registry query functionality and bug fixes.

v2.1.0
  - Query functionaliy refinement.
```

### Usage

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