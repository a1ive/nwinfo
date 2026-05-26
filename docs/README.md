<br />
<div align="center">
  <img src="./images/icon.ico">
  <h2 align="center">NWinfo</h2>
  <a href="https://deepwiki.com/a1ive/nwinfo"><img src="https://deepwiki.com/badge.svg" alt="Ask DeepWiki"></a>
</div>
<br />

**NWinfo** is a Win32 program for collecting system and hardware information.

## Features

* Retrieves detailed information about SMBIOS, CPUID, S.M.A.R.T., PCI, EDID, and more.
* Supports exporting in JSON, YAML, and HTML formats.
* Gathers information directly without relying on WMI.

## Download

- Source code: https://github.com/a1ive/nwinfo  
- Latest release: [GitHub Releases](https://github.com/a1ive/nwinfo/releases)  
- Nightly build: [Direct link](https://nightly.link/a1ive/nwinfo/workflows/msbuild/master/NWinfo.zip)  
- Libraries: [Direct link](https://nightly.link/a1ive/nwinfo/workflows/msbuild/master/NWinfo%20Library.zip)  
- Github actions: [GitHub Actions](https://github.com/a1ive/nwinfo/actions/workflows/msbuild.yml)  

<div style="page-break-after: always;"></div>

## GUI (gnwinfo)

### OS Support

  Windows 7 and later

### Language Support

- English
- Chinese (Simplified)
- Chinese (Traditional)
- French
- German
- Greek
- Italian
- Japanese
- Korean
- Polish
- Portuguese (Brazil)
- Slovenian
- Spanish
- Swedish
- Turkish

### Command-Line Options

- /debug  
  Print debug information to stdout.  

<div style="page-break-after: always;"></div>

## CLI (nwinfo)

### OS Support

  Windows XP and later

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

- \-\-format=`FORMAT`  
  Specify output format.  
  `FORMAT` can be `YAML` (default), `JSON`, `LUA`, `TREE`, or `HTML`.  
- \-\-output=`FILE`  
  Write to `FILE` instead of printing to the screen.  
- \-\-cp=`CODEPAGE`  
  Set the code page of output text.  
  `CODEPAGE` can be `ANSI` or `UTF8`.  
- \-\-human  
  Display numbers in a human-readable format.  
- \-\-temp-unit=`UNIT`  
  Specify the temperature unit.  
  `UNIT` can be `C` (Celsius, default), `F` (Fahrenheit), or `K` (Kelvin).  
- \-\-bin=`FORMAT`  
  Specify binary format.  
  `FORMAT` can be `NONE` (default), `BASE64`, or `HEX`.  
- \-\-debug  
  Print debug information to stdout.  
- \-\-hide-sensitive  
  Hide sensitive data (MAC & S/N).  
- \-\-driver=`NAME`  
  Specify the driver name.  
  Available drivers are `CPUZ162`, `NwHwIo`, and `PawnIO`.  
  Use `NONE` to disable driver usage.  
  By default, the program searches for and loads drivers in the order shown above. See [Supported Drivers](#supported-drivers) for details.  

### Hardware Details

- \-\-cpu[=`FILE`]  
  Print CPUID info.  
  A driver is required to access sensors, such as temperature sensors.  
  Intel, AMD, and VIA/Zhaoxin CPUs are supported.  
  `FILE` specifies the filename of the CPUID dump.  
- \-\-net[=`FLAG,...`]  
  Print network info.  
  - `GUID`  
    Specify the GUID of the network interface, e.g., `{B16B00B5-CAFE-BEEF-DEAD-001453AD0529}`.  
  - `FLAGS`  
    `ACTIVE` Exclude inactive network interfaces.  
    `PHYS`   Exclude virtual network interfaces.  
    `ETH`    Include Ethernet network interfaces.  
    `WLAN`   Include IEEE 802.11 wireless addresses.  
    `IPV4`   Show IPv4 addresses only.  
    `IPV6`   Show IPv6 addresses only.  
- \-\-board  
  Print mainboard info.  
- \-\-acpi[=`SGN`]  
  Print ACPI info.  
  A driver is required to access some ACPI tables.  
  `SGN` specifies the ACPI table signature, e.g., `FACP` (Fixed ACPI Description Table).  
- \-\-smbios[=`TYPE,...`]  
  Print SMBIOS info.  
  `TYPE` specifies the SMBIOS structure types, e.g., `2` or `2,4,17`.  
- \-\-disk[=`FLAG,..`]  
  Print disk info.  
  - `PATH`  
    Specify the disk path, e.g., `\\.\PhysicalDrive0` or `\\.\CdRom0`.  
  - `FLAGS`  
    `NO-SMART` Do not print disk S.M.A.R.T. info.  
    `NO-VOL`   Do not print volume info.  
    `PHYS`     Exclude virtual drives.  
    `CD`       Include CD-ROM devices.  
    `HD`       Include hard drives.  
    `NVME`     Include NVMe devices.  
    `SATA`     Include SATA devices.  
    `SCSI`     Include SCSI devices.  
    `SAS`      Include SAS devices.  
    `USB`      Include USB devices.  
- \-\-smart=`FLAG,...`  
  Specify S.M.A.R.T. features.  
  Features enabled by default:  
  `WMI`, `ADATA`, `HIDENOSMART`, `ATA`, `SAT`, `SUNPLUS`, `IODATA`, `LOGITEC`,  
  `PROLIFIC`, `USBJMICRON`, `CYPRESS`, `JMICRON`, `ASMEDIA`, `REALTEK`,  
  `MEGARAID`, `VROC`, `HIDERAID`, and `CSMIAUTO`.  
  Use `DEFAULT` to specify the above features.  
  Other features are `ADVANCED`, `HD204UI`, `MEMORY`, `RTK9220DP`,  
  `ASM1352R`, `AMDRC2`, `NOCSMI`, and `CSMIRAID`.  
- \-\-display[=`FILE`]  
  Print EDID info.  
  `FILE` specifies the filename of the EDID dump.  
- \-\-pci[=`CLASS,..`]  
  Print PCI info.  
  `CLASS` specifies PCI device class codes, e.g., `0c05` or `03,0c05`.  
- \-\-spd[=`FILE`]  
  Print DIMM SPD info.  
  A driver is required to access SPD data.  
  :warning: This option may damage the hardware.  
  `FILE` specifies the filename of the SPD dump.  
- \-\-usb  
  Print USB info.  
- \-\-battery  
  Print battery info.  
- \-\-uefi[=`FLAG,..`]  
  Print UEFI info.  
  - `FLAGS`  
    `CERT` Parse UEFI Secure Boot signature databases.  
    `MENU` Print UEFI boot menus.  
    `VARS` List all UEFI variables.  
- \-\-audio  
  Print audio devices.  
- \-\-gpu  
  Print GPU utilization and sensors (e.g. temperature).  
  GPU drivers are required to access this information.  
  NVIDIA (NVAPI), AMD (ADL2), and Intel (IGCL) are supported.  
- \-\-device[=`TYPE`]  
  Print device tree.  
  `TYPE` specifies the device type, e.g., `ACPI`, `SWD`, `PCI`, or `USB`.  
- \-\-drv-store[=`DRIVE_LETTER`]
  Print Windows driver store info.
  `DRIVE_LETTER` specifies the drive letter of an offline target system, e.g., `D`.
- \-\-sensors[=`SRC,..`]  
  Print sensor readings.  
  `SRC` specifies the sensor provider.  
  Available providers are:  
  `LHM`, `HWINFO`, `GPU-Z`,  
  `CPU`, `DIMM`, `GPU`, `SMART`, `DISK`, `NET`, `IMC`, `INTEL`, and `ZEN`.  

### System Information

- \-\-sys  
  Print system info.  
- \-\-shares  
  Print mapped network drives and shared folders.  
- \-\-public-ip  
  Print public IP address.  
- \-\-product-policy[=`NAME`]  
  Print ProductPolicy.  
  `NAME` specifies the product policy name.  
- \-\-font  
  Print installed fonts.  

### PowerShell Script for System Diagnostics

`hw_report.ps1` is a PowerShell script that generates and displays a detailed hardware and system report using `nwinfo`.  

You might need to temporarily allow script execution by running the following command:
```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
```

<div style="page-break-after: always;"></div>

## Supported Hardware

### CPU

| Vendor | CPUID | Temperature | Voltage | Power | Clock |
|--------|-------|-------------|---------|-------|-------|
| Intel         | ✅ | ✅ | ✅ | ✅ | ✅ |
| AMD           | ✅ | ✅ | ✅ | ✅ | ✅ |
| VIA / Zhaoxin | ✅ | ✅ | ❌ | ❌ | ❌ |

### GPU / NPU

| Vendor | API | GPU Usage | VRAM | Temperature | Power | Frequency | Voltage | Fan Speed |
|--------|-----|-----------|------|-------------|-------|-----------|---------|-----------|
| NVIDIA      | NVAPI  | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| AMD         | ADL2   | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Intel       | IGCL   | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Generic NPU | DXCore | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Generic GPU | D3D    | ✅ | ✅ | ✅ | ✅ | ❌ | ❌ | ⚠️ |

Notes:
 * `VRAM` refers to the dedicated video memory only.
 * `Frequency` refers to the GPU core frequency.
 * `Power` refers to the board power draw.

### Memory Module SPD / IMC

- SMBus: Intel PCH, PIIX4 / AMD SB / Hygon
- Memory Module: SDR, DDR, DDR2, DDR3, DDR4, DDR5
- Thermal Sensor: DDR4, DDR5
- IMC: Intel Core 2nd Gen and later / AMD Zen and later

### HDD / SSD S.M.A.R.T.

NWinfo uses [libcdi](https://github.com/a1ive/libcdi) to access S.M.A.R.T. data.

`libcdi` is a dynamic library based on [CrystalDiskInfo](https://crystalmark.info/en/software/crystaldiskinfo/).

Note: NVMe requires Windows 10 or later.

<div style="page-break-after: always;"></div>

## Supported Drivers

The program searches for and loads drivers in the following order: **CPUZ162 -> NwHwIo -> PawnIO**.

| Driver  | Bundled   | CPU Sensor | SPD | IMC |
|---------|-----------|------------|-----|-----|
| PawnIO  | ✅Full    | ✅ | ✅ | ✅ |
| NwHwIo  | ✅Lite    | ✅ | ✅ | ✅ |
| CPUZ162 | ❌        | ✅ | ✅ | ✅ |

**Note:** The program can still run normally without a driver, but some hardware information may not be accessible.

### PawnIO Driver Installation

PawnIO driver requires Windows (x64) 10 1809 or later.

Install the PawnIO driver silently using the following command:

```bat
.\PawnIOSetup.exe -install -silent
```

Uninstall the PawnIO driver silently using the following command:
```bat
.\PawnIOSetup.exe -uninstall -silent
```

Delete the PawnIO driver from the Windows DriverStore:
```bat
REM Find the <OEMXXX.inf> name of the PawnIO driver
pnputil /enum-drivers
REM Delete the driver (replace oemXXX.inf with the name identified in the previous step)
pnputil /delete-driver <oemXXX.inf> /uninstall
```

<div style="page-break-after: always;"></div>

## File List

This section describes all files included in the final release package.

| File Name | Category | Description |
|-----------|----------|-------------|
| `nwinfo.exe` | Executable | Main executable (x64) |
| `nwinfox86.exe` | Executable | Main executable (x86) |
| `gnwinfo.exe` | Executable | GUI executable (x64) |
| `gnwinfox86.exe` | Executable | GUI executable (x86) |
| `libcdi.dll` | Library | S.M.A.R.T. data access library (x64) |
| `libcdix86.dll` | Library | S.M.A.R.T. data access library (x86) |
| `hw_report.ps1` | Script | Example PowerShell script |
| `gnwinfo.ini` | Configuration | Configuration file for the GUI |
| `pci.ids` | Database | PCI database |
| `usb.ids` | Database | USB database |
| `pnp.ids` | Database | PnP (monitor) vendor database |
| `jep106.ids` | Database | JEDEC memory module vendor database |
| `PawnIOSetup.exe` | Driver | PawnIO driver installer (x64) |
| `IntelMCHBAR.bin` | PawnIO Module | Intel MCHBAR module for the PawnIO driver |
| `IntelMSR.bin` | PawnIO Module | Intel MSR module for the PawnIO driver |
| `AMDFamily0F.bin` | PawnIO Module | AMD K8 MSR module for the PawnIO driver |
| `AMDFamily10.bin` | PawnIO Module | AMD K10 MSR module for the PawnIO driver |
| `AMDFamily17.bin` | PawnIO Module | AMD Zen MSR module for the PawnIO driver |
| `RyzenSMU.bin` | PawnIO Module | AMD Ryzen SMU module for the PawnIO driver |
| `SmbusPIIX4.bin` | PawnIO Module | PIIX4 SMBus module for the PawnIO driver |
| `SmbusI801.bin` | PawnIO Module | I801 SMBus module for the PawnIO driver |
| `LpcIO.bin` | PawnIO Module | LPC I/O module for the PawnIO driver |

<div style="page-break-after: always;"></div>

## Licenses & Credits

This project is licensed under the [Unlicense](https://unlicense.org/) license.

* [libcpuid](https://libcpuid.sourceforge.net)
* [CrystalDiskInfo](https://github.com/hiyohiyo/CrystalDiskInfo)
* [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)
* [stb](https://github.com/nothings/stb)
* [optparse](https://github.com/skeeto/optparse)
* [hwdata](https://github.com/vcrhonek/hwdata)
* [PawnIO](https://pawnio.eu/)
