<br />
<div align="center">
  <img src="./assets/images/icon.ico">
  <h2 align="center">NWinfo</h2>
  <div align="center">
    <img src="https://img.shields.io/github/stars/a1ive/nwinfo?style=flat&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHhtbDpzcGFjZT0icHJlc2VydmUiIHdpZHRoPSI2NTUuMzU5IiBoZWlnaHQ9IjY1NS4zNTkiIHN0eWxlPSJzaGFwZS1yZW5kZXJpbmc6Z2VvbWV0cmljUHJlY2lzaW9uO3RleHQtcmVuZGVyaW5nOmdlb21ldHJpY1ByZWNpc2lvbjtpbWFnZS1yZW5kZXJpbmc6b3B0aW1pemVRdWFsaXR5O2ZpbGwtcnVsZTpldmVub2RkO2NsaXAtcnVsZTpldmVub2RkIiB2aWV3Qm94PSIwIDAgNi44MjcgNi44MjciPjxwYXRoIHN0eWxlPSJmaWxsOiNmZjhmMDA7ZmlsbC1ydWxlOm5vbnplcm8iIGQ9Im0zLjUxIDEuMjUyLjU0NiAxLjUzNiAxLjYyOC4wNDMuMjkuMDA4LS4yMy4xNzYtMS4yOTMuOTkzLjQ2MyAxLjU2My4wODIuMjc3LS4yMzktLjE2My0xLjM0NC0uOTIzLTEuMzQzLjkyMy0uMjM5LjE2NC4wODItLjI3OC40NjItMS41NjQtMS4yOTItLjk5Mi0uMjMtLjE3Ni4yOS0uMDA4IDEuNjMtLjA0NC41NDQtMS41MzUuMDk3LS4yNzR6Ii8+PHBhdGggc3R5bGU9ImZpbGw6I2U2ODEwMDtmaWxsLXJ1bGU6bm9uemVybyIgZD0ibTMuNTEgMS4yNTIuNTQ2IDEuNTM2IDEuNjI4LjA0My4yOS4wMDgtLjIzLjE3Ni0xLjI5My45OTMuNDYzIDEuNTYzLjA4Mi4yNzctLjIzOS0uMTYzLTEuMzQ0LS45MjNWLjk4eiIvPjxwYXRoIHN0eWxlPSJmaWxsOm5vbmUiIGQ9Ik0wIDBoNi44Mjd2Ni44MjdIMHoiLz48L3N2Zz4=&label=&color=grey">
    <img src="https://img.shields.io/github/license/a1ive/nwinfo?logo=unlicense&label=&color=ad1453">
    <img src="https://img.shields.io/github/downloads/a1ive/nwinfo/total?label=%E2%87%A9&labelColor=blue&color=blue">
    <img src="https://img.shields.io/github/v/release/a1ive/nwinfo?label=%F0%9F%93%A6&labelColor=cyan&color=cyan">
    <img src="https://img.shields.io/github/languages/top/a1ive/nwinfo?logo=c&label=&color=1453ad">
    <img src="https://img.shields.io/github/actions/workflow/status/a1ive/nwinfo/msbuild.yml?logo=github&label=">
  </div>
</div>
<br />

NWinfo is a Win32 program that allows you to obtain system and hardware information.

<div style="page-break-after: always;"></div>

## Features
* Obtain detailed information about SMBIOS, CPUID, S.M.A.R.T., PCI, EDID, and more.
* Support exporting in JSON, YAML, and LUA table formats.
* Compatible with Windows XP.

<div style="page-break-after: always;"></div>

## GUI Preview

<div align="center">
  <img src="./assets/images/demo.png">
</div>

<div style="page-break-after: always;"></div>

## CLI Usage
```txt
.\nwinfo.exe OPTIONS
OPTIONS:
  --format=FORMAT  Specify output format.
                   FORMAT can be 'YAML' (default), 'JSON' and 'LUA'.
  --output=FILE    Write to FILE instead of printing to screen.
  --cp=CODEPAGE    Set the code page of output text.
                   CODEPAGE can be 'ANSI' and 'UTF8'.
  --human          Display numbers in human readable format.
  --debug          Print debug info to stdout.
  --hide-sensitive Hide sensitive data (MAC & S/N).
  --sys            Print system info.
  --cpu            Print CPUID info.
  --net[=FLAG,...] Print network info.
    GUID           Specify the GUID of the network interface,
                   e.g. '{B16B00B5-CAFE-BEEF-DEAD-001453AD0529}'
    FLAGS:
      ACTIVE       Filter out active network interfaces.
      PHYS         Filter out physical network interfaces.
      ETH          Filter out Ethernet network interfaces.
      WLAN         Filter out IEEE 802.11 wireless addresses.
      IPV4         Filter out IPv4 addresses.
      IPV6         Filter out IPv6 addresses.
  --acpi[=SGN]     Print ACPI info.
                   SGN specifies the signature of the ACPI table,
                   e.g. 'FACP' (Fixed ACPI Description Table).
  --smbios[=TYPE]  Print SMBIOS info.
                   TYPE specifies the type of the SMBIOS table,
                   e.g. '2' (Base Board Information).
  --disk[=FLAG,..] Print disk info.
    PATH           Specify the path of the disk,
                   e.g. '\\.\PhysicalDrive0', '\\.\CdRom0'.
    FLAGS:
      NO-SMART     Don't print disk S.M.A.R.T. info.
      PHYS         Exclude virtual drives.
      CD           Filter out CD-ROM devices.
      HD           Filter out hard drives.
      NVME         Filter out NVMe devices.
      SATA         Filter out SATA devices.
      SCSI         Filter out SCSI devices.
      SAS          Filter out SAS devices.
      USB          Filter out USB devices.
  --smart=FLAG,... Specify S.M.A.R.T. features.
                   Features enabled by default: 'WMI', 'ATA',
                   'NVIDIA', 'MARVELL', 'SAT', 'SUNPLUS',
                   'IODATA', 'LOGITEC', 'PROLIFIC', 'USBJMICRON',
                   'CYPRESS', 'MEMORY', 'JMICRON', 'ASMEDIA',
                   'REALTEK', 'MEGARAID', 'VROC' and 'ASM1352R'.
                   Use 'DEFAULT' to specify the above features.
                   Other features are 'ADVANCED', 'HD204UI',
                   'ADATA', 'NOWAKEUP', 'JMICRON3' and 'RTK9220DP'.
  --display        Print EDID info.
  --pci[=CLASS]    Print PCI info.
                   CLASS specifies the class code of pci devices,
                   e.g. '0C05' (SMBus).
  --usb            Print USB info.
  --spd            Print SPD info.
  --battery        Print battery info.
  --uefi[=FLAG,..] Print UEFI info.
    FLAGS:
      MENU         Print UEFI boot menus.
      VARS         List all UEFI variables.
  --shares         Print network mapped drives and shared folders.
  --audio          Print audio devices.
  --public-ip      Print public IP address.
  --product-policy Print ProductPolicy.
  --gpu            Print GPU usage.
  --font           Print installed fonts.
```

<div style="page-break-after: always;"></div>

## Note
For Win11, if the driver cannot be loaded properly, please modify the following registry keys.
```
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

<div style="page-break-after: always;"></div>

## Credits

* [libcpuid](https://libcpuid.sourceforge.net)
* [SysInv](https://github.com/cavaliercoder/sysinv)
* [DumpSMBIOS](https://github.com/KunYi/DumpSMBIOS)
* [CrystalDiskInfo](https://github.com/hiyohiyo/CrystalDiskInfo)
* [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)
* [HardInfo](https://github.com/lpereira/hardinfo)
* [Memtest86+](https://github.com/memtest86plus/memtest86plus)
* [hwdata](https://github.com/vcrhonek/hwdata)
* [Linux USB](http://www.linux-usb.org)
* [The PCI ID Repository](https://pci-ids.ucw.cz)
* [RyzenAdj](https://github.com/FlyGoat/RyzenAdj)
