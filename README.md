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

> [!WARNING]
>
> This application uses the `HwRwDrv` / `WinRing0` driver, which is known for security vulnerabilities.
>
> *   It may be flagged as malware by your antivirus.
> *   It can be detected by anti-cheat software, potentially leading to an account ban in online games.
>
> If you have any concerns, feel free to delete the driver file. This may disable some program functionality.

## GUI Preview

<div align="center">
  <img src="./docs/images/demo.png">
</div>

## [CLI Usage](./docs/README.md)

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
