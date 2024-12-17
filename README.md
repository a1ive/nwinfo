<br />
<div align="center">
  <img src="./assets/images/icon.ico">
  <h2 align="center">NWinfo</h2>
  <div align="center">
    <img src="https://img.shields.io/github/license/a1ive/nwinfo?label=LICENSE&color=ad1453">
    <img src="https://img.shields.io/github/downloads/a1ive/nwinfo/total?label=DOWNLOADS&color=blue">
  </div>
</div>
<br />

**NWinfo** is a Win32 program that allows you to obtain system and hardware information.

## Features
* Obtain detailed information about SMBIOS, CPUID, S.M.A.R.T., PCI, EDID, and more.
* Support exporting in JSON, YAML, and LUA table formats.
* Gathers information directly without relying on WMI.
* Compatible with Windows XP.

<div style="page-break-after: always;"></div>

## GUI (gnwinfo)

### Preview

<div align="center">
  <img src="./assets/images/demo.png">
</div>

<div style="page-break-after: always;"></div>

## CLI (nwinfo)

### Usage

```txt
.\nwinfo.exe OPTIONS
```

### Example Command

```txt
.\nwinfo.exe --format=json --output=report.json --cp=UTF8 --sys --disk --smbios --net
```

This command exports system, disk, SMBIOS, and network information to `report.json` in JSON format.

### General Options

- --format=`FORMAT`  
  Specify output format.  
  `FORMAT` can be `YAML` (default), `JSON` and `LUA`.  
- --output=`FILE`  
  Write to `FILE` instead of printing to screen.  
- --cp=`CODEPAGE`  
  Set the code page of output text.  
  `CODEPAGE` can be `ANSI` and `UTF8`.  
- --human  
  Display numbers in human readable format.  
- --debug  
  Print debug info to stdout.  
- --hide-sensitive  
  Hide sensitive data (MAC & S/N).  

### Hardware Details

- --cpu  
  Print CPUID info.  
- --net[=`FLAG,...`]  
  Print network info.  
  - `GUID`  
    Specify the GUID of the network interface, e.g. `{B16B00B5-CAFE-BEEF-DEAD-001453AD0529}`  
  - `FLAGS`  
    `ACTIVE` Exclude inactive network interfaces.  
    `PHYS`   Exclude virtual network interfaces.  
    `ETH`    Include Ethernet network interfaces.  
    `WLAN`   Include IEEE 802.11 wireless addresses.  
    `IPV4`   Show IPv4 addresses only.  
    `IPV6`   Show IPv6 addresses only.  
- --acpi[=`SGN`]  
  Print ACPI info.  
  `SGN` specifies the signature of the ACPI table, e.g. `FACP` (Fixed ACPI Description Table).  
- --smbios[=`TYPE`]  
  Print SMBIOS info.  
  `TYPE` specifies the type of the SMBIOS table, e.g. `2` (Base Board Information).  
- --disk[=`FLAG,..`]  
  Print disk info.  
  - `PATH`  
    Specify the path of the disk, e.g. `\\.\PhysicalDrive0`, `\\.\CdRom0`.  
  - `FLAGS`  
    `NO-SMART` Don't print disk S.M.A.R.T. info.  
    `PHYS`     Exclude virtual drives.  
    `CD`       Include CD-ROM devices.  
    `HD`       Include hard drives.  
    `NVME`     Include NVMe devices.  
    `SATA`     Include SATA devices.  
    `SCSI`     Include SCSI devices.  
    `SAS`      Include SAS devices.  
    `USB`      Include USB devices.  
- --smart=`FLAG,...`  
  Specify S.M.A.R.T. features.  
  Features enabled by default:
  `WMI`, `ATA`, `NVIDIA`, `MARVELL`, `SAT`, `SUNPLUS`, `IODATA`, `LOGITEC`, `PROLIFIC`, `USBJMICRON`,
  `CYPRESS`, `MEMORY`, `JMICRON`, `ASMEDIA`, `REALTEK`, `MEGARAID`, `VROC` and `ASM1352R`.  
  Use `DEFAULT` to specify the above features.  
  Other features are `ADVANCED`, `HD204UI`, `ADATA`, `NOWAKEUP`, `JMICRON3` and `RTK9220DP`.  
- --display  
  Print EDID info.  
- --pci[=`CLASS`]  
  Print PCI info.  
  `CLASS` specifies the class code of pci devices, e.g. `0C05` (SMBus).  
- --usb  
  Print USB info.  
- --spd  
  Print SPD info.  
- --battery  
  Print battery info.  
- --uefi[=`FLAG,..`]  
  Print UEFI info.  
  - `FLAGS`  
    `MENU` Print UEFI boot menus.  
    `VARS` List all UEFI variables.  
 - --audio  
   Print audio devices.  
 - --gpu  
   Print GPU usage.  

### System Information

- --sys  
  Print system info.  
- --shares  
  Print network mapped drives and shared folders.  
- --public-ip  
  Print public IP address.  
- --product-policy[=`NAME`]  
  Print ProductPolicy.  
  `NAME` specifies the name of the product policy.  
- --font  
  Print installed fonts.  

### PowerShell Script for System Diagnostics

`hw_report.ps1` is a PowerShell script designed to generate and display a detailed hardware and system report using `nwinfo`.

<div style="page-break-after: always;"></div>

## Notes

### Windows 11
If the driver cannot be loaded properly, modify the following registry keys:
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

### Windows 7
For earlier versions of Windows 7, the driver may not work properly and requires a SHA1-signed certificate.

### Windows XP
This project is compatible with Windows XP using [YY-Thunks](https://github.com/Chuyu-Team/YY-Thunks), but it may not retrieve some hardware information.

<div style="page-break-after: always;"></div>

## Licenses & Credits

This project is licensed under the [Unlicense](https://unlicense.org/) license.

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
