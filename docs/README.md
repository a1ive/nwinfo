﻿<br />
<div align="center">
  <img src="./images/icon.ico">
  <h2 align="center">NWinfo</h2>
</div>
<br />

**NWinfo** is a Win32 program that allows you to obtain system and hardware information.

## Features
* Obtain detailed information about SMBIOS, CPUID, S.M.A.R.T., PCI, EDID, and more.
* Support exporting in JSON, YAML, and HTML formats.
* Gathers information directly without relying on WMI.

## Download

The source code is maintained in a git repository at https://github.com/a1ive/nwinfo.  
You can download the latest version of the binary from [GitHub Releases](https://github.com/a1ive/nwinfo/releases).  

<div style="page-break-after: always;"></div>

## CLI (nwinfo)

### Usage

```txt
.\nwinfo.exe OPTIONS
```

### Example Commands

```bat
.\nwinfo.exe --format=json --output=report.json --cp=UTF8 --sys --disk
```

Exports system and disk information to `report.json` in JSON format.

```bat
.\nwinfo.exe --pci=03
```

Prints information about all PCI devices in the class code `03` (Display Controllers).

```bat
.\nwinfo.exe --format=html --output=report.html --net=active,phys,ipv4
```

Exports active physical network interfaces with IPv4 addresses to `report.html` in HTML format.

<div style="page-break-after: always;"></div>

### General Options

- --format=`FORMAT`  
  Specify output format.  
  `FORMAT` can be `YAML` (default), `JSON`, `LUA`, `TREE` and `HTML`.  
- --output=`FILE`  
  Write to `FILE` instead of printing to screen.  
- --cp=`CODEPAGE`  
  Set the code page of output text.  
  `CODEPAGE` can be `ANSI` and `UTF8`.  
- --human  
  Display numbers in human readable format.  
- --bin=`FORMAT`  
  Specify binary format.  
  `FORMAT` can be `NONE` (default), `BASE64` and `HEX`.  
- --debug  
  Print debug info to stdout.  
- --hide-sensitive  
  Hide sensitive data (MAC & S/N).  

### Hardware Details

- --cpu[=`FILE`]  
  Print CPUID info.  
  Driver is required to access sensors (e.g. temperature).  
  Intel, AMD and VIA/Zhaoxin CPUs are supported.  
  `FILE` specifies the file name of the CPUID dump.  
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
  Driver is required to access some ACPI tables.  
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
  `CYPRESS`, `MEMORY`, `JMICRON`, `ASMEDIA`, `REALTEK`, `MEGARAID`, `VROC`, `ASM1352R` and `HIDERAID`.  
  Use `DEFAULT` to specify the above features.  
  Other features are `ADVANCED`, `HD204UI`, `ADATA`, `NOWAKEUP` and `RTK9220DP`.  
- --display  
  Print EDID info.  
- --pci[=`CLASS`]  
  Print PCI info.  
  `CLASS` specifies the class code of pci devices, e.g. `0C05` (SMBus).  
- --usb  
  Print USB info.  
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
   Print GPU utilization and sensors (e.g. temperature).  
   GPU drivers are required to access these info.  
   nVidia (NVAPI), AMD (ADL2) and Intel (IGCL) are supported.  
 - --device[=`TYPE`]  
   Print device tree.  
   `TYPE` specifies the type of the devices, e.g. `ACPI`, `SWD`, `PCI` or `USB`.  

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

### Windows XP
This project is compatible with Windows XP using [YY-Thunks](https://github.com/Chuyu-Team/YY-Thunks), but it may not retrieve some hardware information.

## Supported Drivers

This project searches for and loads drivers in the following order: **HwRwDrv -> WinRing0 -> PawnIO**.

| Driver     | Author | License | Notes |
|------------|--------|---------|-------|
| [PawnIO](https://github.com/namazso/PawnIO) | namazso | GPL v2 | Safe to use, but some hardware information may be unavailable. |
| [HwRwDrv](https://hwrwdrv.phpnet.us/?i=1) | Faintsnow | Closed source | May be flagged as a virus by antivirus software and detected by anti-cheat systems. |
| [WinRing0](http://openlibsys.org/) | hiyohiyo | BSD | Listed as a vulnerable driver by Microsoft, detected as a virus, and triggers anti-cheat software. |

**Note:** The program can still run normally even if all drivers are removed, but some hardware information may not be accessible.

<div style="page-break-after: always;"></div>

## Licenses & Credits

This project is licensed under the [Unlicense](https://unlicense.org/) license.

* [libcpuid](https://libcpuid.sourceforge.net)
* [SysInv](https://github.com/cavaliercoder/sysinv)
* [CrystalDiskInfo](https://github.com/hiyohiyo/CrystalDiskInfo)
* [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)
* [hwdata](https://github.com/vcrhonek/hwdata)
* [Linux USB](http://www.linux-usb.org)
* [The PCI ID Repository](https://pci-ids.ucw.cz)
* [edid-decode](https://git.linuxtv.org/v4l-utils.git/tree/utils/edid-decode)
* [PawnIO](https://pawnio.eu/)
