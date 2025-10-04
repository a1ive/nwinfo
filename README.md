<br />
<div align="center">
  <img src="./docs/images/icon.ico">
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
* Support exporting in JSON, YAML, and HTML formats.
* Gathers information directly without relying on WMI.
* Compatible with Windows XP.

## GUI Preview

<div align="center">
  <img src="./docs/images/demo.png">
</div>

## CLI Usage

For details, see [CLI (nwinfo)](./docs/README.md#cli-nwinfo) in the documentation.

## Supported Drivers

This project searches for and loads drivers from the same directory in the following order: **PawnIO -> HwRwDrv -> WinRing0**.

| Driver     | Author | License | Notes |
|------------|--------|---------|-------|
| [PawnIO](https://github.com/namazso/PawnIO) | namazso | GPL v2 | Safe to use, but some hardware information may be unavailable. |
| [HwRwDrv](https://hwrwdrv.phpnet.us/?i=1) | Faintsnow | Closed source | May be flagged as a virus by antivirus software and detected by anti-cheat systems. |
| [WinRing0](http://openlibsys.org/) | hiyohiyo | BSD | Listed as a vulnerable driver by Microsoft, detected as a virus, and triggers anti-cheat software. |

**Note:** The program can still run normally even if all drivers are removed, but some hardware information may not be accessible.

## Licenses & Credits

This project is licensed under the [Unlicense](https://unlicense.org/) license.

* [libcpuid](https://libcpuid.sourceforge.net)
* [SysInv](https://github.com/cavaliercoder/sysinv)
* [DumpSMBIOS](https://github.com/KunYi/DumpSMBIOS)
* [CrystalDiskInfo](https://github.com/hiyohiyo/CrystalDiskInfo)
* [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)
* [hwdata](https://github.com/vcrhonek/hwdata)
* [Linux USB](http://www.linux-usb.org)
* [The PCI ID Repository](https://pci-ids.ucw.cz)
* [RyzenAdj](https://github.com/FlyGoat/RyzenAdj)
