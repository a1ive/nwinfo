<br />
<div align="center">
  <img src="icon.ico">
  <h3 align="center">NWinfo</h3>
  <img src="https://img.shields.io/github/stars/a1ive/nwinfo?style=flat&label=%E2%98%85&color=grey">
  <img src="https://img.shields.io/github/license/a1ive/nwinfo?logo=unlicense&label=">
  <img src="https://img.shields.io/github/downloads/a1ive/nwinfo/total?label=%E2%87%A9&labelColor=blue&color=blue">
  <img src="https://img.shields.io/github/v/release/a1ive/nwinfo?label=%F0%9F%93%A6&labelColor=cyan&color=cyan">
</div>
<br />

NWinfo is a Win32 program that allows you to obtain system and hardware information.

## Features
* Obtain detailed information about SMBIOS, CPUID, S.M.A.R.T., PCI, EDID, and more.
* Support exporting in JSON, YAML, and LUA table formats.
* Compatible with Windows XP.

## Note
For Windows 11, the "Memory Integrity" and "Microsoft Vulnerable Driver Blocklist" options should be disabled.
```txt
Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\DeviceGuard\Scenarios\HypervisorEnforcedCodeIntegrity]
"Enabled"=dword:0000000

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\DeviceGuard]
"EnableVirtualizationBasedSecurity"=dword:00000000

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\DeviceGuard\Scenarios\SystemGuard]
"Enabled"=dword:00000000

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\CI\Config]
"VulnerableDriverBlocklistEnable"=dword:00000000
```

## GUI Preview

<div align="center">
  <img src="assets/images/demo.png">
</div>

## CLI Usage
```txt
.\nwinfo.exe OPTIONS
OPTIONS:
  --format=FMT     Specify output format.
                   FMT can be 'YAML' (default), 'JSON' and 'LUA'.
  --output=FILE    Write to FILE instead of printing to screen.
  --cp=CODEPAGE    Set the code page of output text.
                   CODEPAGE can be 'ANSI' and 'UTF8'.
  --human          Display numbers in human readable format.
  --debug          Print debug info to stdout.
  --hide-sensitive Hide sensitive data (MAC & S/N).
  --sys            Print system info.
  --cpu            Print CPUID info.
  --net[=FLAG]     Print network info.
                   FLAG can be 'ACTIVE' (print only the active network).
  --acpi[=SGN]     Print ACPI info.
                   SGN specifies the signature of the ACPI table,
                   e.g. 'FACP' (Fixed ACPI Description Table).
  --smbios[=TYPE]  Print SMBIOS info.
                   TYPE specifies the type of the SMBIOS table,
                   e.g. '2' (Base Board Information).
  --disk[=PATH]    Print disk info.
                   PATH specifies the path of the disk,
                   e.g. '\\\\.\\PhysicalDrive0', '\\\\.\\CdRom0'.
  --no-smart       Don't print disk S.M.A.R.T. info.
  --display        Print EDID info.
  --pci[=CLASS]    Print PCI info.
                   CLASS specifies the class code of pci devices,
                   e.g. '0C05' (SMBus).
  --usb            Print USB info.
  --spd            Print SPD info.
  --battery        Print battery info.
  --uefi           Print UEFI info.
  --shares         Print network mapped drives.
  --audio          Print audio devices.
  --public-ip      Print public IP address.
  --product-policy Print ProductPolicy.
  --gpu            Print GPU usage.
  --font           Print installed fonts.
```

## Credits

* [libcpuid](https://libcpuid.sourceforge.net)
* [SysInv](https://github.com/cavaliercoder/sysinv)
* [DumpSMBIOS](https://github.com/KunYi/DumpSMBIOS)
* [CrystalDiskInfo](https://github.com/hiyohiyo/CrystalDiskInfo)
* [CrystalDiskInfoEmbedded](https://github.com/iTXTech/CrystalDiskInfoEmbedded)
* [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)
* [HardInfo](https://github.com/lpereira/hardinfo)
* [Memtest86+](https://github.com/memtest86plus/memtest86plus)
* [hwdata](https://github.com/vcrhonek/hwdata)
* [Linux USB](http://www.linux-usb.org)
* [The PCI ID Repository](https://pci-ids.ucw.cz)
* [RyzenAdj](https://github.com/FlyGoat/RyzenAdj)
