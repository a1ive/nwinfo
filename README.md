<br />
<div align="center">
  <img src="icon.ico">
  <h3 align="center">NWinfo</h3>
  <img src="https://img.shields.io/github/license/a1ive/nwinfo">
  <img src="https://img.shields.io/github/downloads/a1ive/nwinfo/total">
  <img src="https://img.shields.io/github/v/release/a1ive/nwinfo">
</div>
<br />


NWinfo is a Win32 console program that collects system and hardware information.

## Features
* Collects ACPI, SMBIOS, CPUID, S.M.A.R.T., PCI, EDID, and USB details.
* Supports JSON, YAML, and LUA Table formats.
* Compatible with Windows XP to Windows 11.

## Usage
```
nwinfo.exe OPTIONS
OPTIONS:
  --format=XXX     Specify output format. [YAML|JSON|LUA]
  --output=FILE    Write to FILE instead of printing to screen.
  --human          Display numbers in human readable format.
  --sys            Print system info.
  --cpu            Print processor details.
  --net[=active]   Print network infomation for each adapter interface.
  --acpi[=XXXX]    Print ACPI tables.
  --smbios[=XX]    Print SMBIOS tables.
  --disk           Print disk S.M.A.R.T and partitions.
  --display        Print display monitors information (EDID).
  --pci[=XX]       List PCI devices.
  --usb            List USB devices.
  --spd            Print SPD infomation of memory modules (Experimental).
  --battery        Print battery infomation.
```

## Credits

* [libcpuid](https://libcpuid.sourceforge.net)
* [SysInv](https://github.com/cavaliercoder/sysinv)
* [DumpSMBIOS](https://github.com/KunYi/DumpSMBIOS)
* [CrystalDiskInfo](https://github.com/hiyohiyo/CrystalDiskInfo)
* [CrystalDiskInfoEmbedded](https://github.com/iTXTech/CrystalDiskInfoEmbedded)
* [HardInfo](https://github.com/lpereira/hardinfo)
* [Memtest86+](https://github.com/memtest86plus/memtest86plus)
* [hwdata](https://github.com/vcrhonek/hwdata)
* [Linux USB](http://www.linux-usb.org)
* [The PCI ID Repository](https://pci-ids.ucw.cz)
* [Ventoy](https://github.com/ventoy/Ventoy)

