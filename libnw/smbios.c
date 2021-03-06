// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "smbios.h"
#include "utils.h"

static const char* mem_human_sizes[6] =
{ "B", "KB", "MB", "GB", "TB", "PB", };

static const char* LocateString(const char* str, UINT i)
{
	static const char nul[] = "NULL";
	if (0 == i || 0 == *str)
		return nul;
	while (--i)
		str += strlen((char*)str) + 1;
	return str;
}

static const char* toPointString(void* p)
{
	return (char*)p + ((PSMBIOSHEADER)p)->Length;
}

static void ProcBIOSInfo(PNODE tab, void* p)
{
	PBIOSInfo pBIOS = (PBIOSInfo)p;
	const char* str = toPointString(p);
	NWL_NodeAttrSet(tab, "Description", "BIOS Information", 0);
	NWL_NodeAttrSet(tab, "Vendor", LocateString(str, pBIOS->Vendor), 0);
	NWL_NodeAttrSet(tab, "Version", LocateString(str, pBIOS->Version), 0);
	NWL_NodeAttrSetf(tab, "Starting Segment", 0, "%04Xh", pBIOS->StartingAddrSeg);
	NWL_NodeAttrSet(tab, "Release Date", LocateString(str, pBIOS->ReleaseDate), 0);
	NWL_NodeAttrSetf(tab, "Image Size (K)", NAFLG_FMT_NUMERIC, "%u", (pBIOS->ROMSize + 1) * 64);
	if (pBIOS->Header.Length > 0x14)
	{
		if (pBIOS->MajorRelease != 0xff || pBIOS->MinorRelease != 0xff)
			NWL_NodeAttrSetf(tab, "System BIOS Version", 0, "%u.%u", pBIOS->MajorRelease, pBIOS->MinorRelease);
		if (pBIOS->ECFirmwareMajor != 0xff || pBIOS->ECFirmwareMinor != 0xff)
			NWL_NodeAttrSetf(tab, "EC Firmware Version", 0, "%u.%u", pBIOS->ECFirmwareMajor, pBIOS->ECFirmwareMinor);
	}
}

static void ProcSysInfo(PNODE tab, void* p)
{
	PSystemInfo pSystem = (PSystemInfo)p;
	const char* str = toPointString(p);

	NWL_NodeAttrSet(tab, "Description", "System Information", 0);
	NWL_NodeAttrSet(tab, "Manufacturer", LocateString(str, pSystem->Manufacturer), 0);
	NWL_NodeAttrSet(tab, "Product Name", LocateString(str, pSystem->ProductName), 0);
	NWL_NodeAttrSet(tab, "Version", LocateString(str, pSystem->Version), 0);
	NWL_NodeAttrSet(tab, "Serial Number", LocateString(str, pSystem->SN), 0);
	// for v2.1 and later
	if (pSystem->Header.Length > 0x08)
		NWL_NodeAttrSet(tab, "UUID", NWL_GuidToStr(pSystem->UUID), NAFLG_FMT_GUID);

	if (pSystem->Header.Length > 0x19)
	{
		// fileds for spec. 2.4
		NWL_NodeAttrSet(tab, "SKU Number", LocateString(str, pSystem->SKUNumber), 0);
		NWL_NodeAttrSet(tab, "Family", LocateString(str, pSystem->Family), 0);
	}
}

static void ProcBoardInfo(PNODE tab, void* p)
{
	PBoardInfo pBoard = (PBoardInfo)p;
	const char* str = toPointString(p);

	NWL_NodeAttrSet(tab, "Description", "Base Board Information", 0);
	NWL_NodeAttrSet(tab, "Manufacturer", LocateString(str, pBoard->Manufacturer), 0);
	NWL_NodeAttrSet(tab, "Product Name", LocateString(str, pBoard->Product), 0);
	NWL_NodeAttrSet(tab, "Version", LocateString(str, pBoard->Version), 0);
	NWL_NodeAttrSet(tab, "Serial Number", LocateString(str, pBoard->SN), 0);
	NWL_NodeAttrSet(tab, "Asset Tag", LocateString(str, pBoard->AssetTag), 0);
	if (pBoard->Header.Length > 0x08)
	{
		NWL_NodeAttrSet(tab, "Location in Chassis", LocateString(str, pBoard->LocationInChassis), 0);
	}
}

static void ProcSystemEnclosure(PNODE tab, void* p)
{
	PSystemEnclosure pSysEnclosure = (PSystemEnclosure)p;
	const char* str = toPointString(p);
	NWL_NodeAttrSet(tab, "Description", "System Enclosure Information", 0);
	NWL_NodeAttrSet(tab, "Manufacturer", LocateString(str, pSysEnclosure->Manufacturer), 0);
	NWL_NodeAttrSet(tab, "Version", LocateString(str, pSysEnclosure->Version), 0);
	NWL_NodeAttrSet(tab, "Serial Number", LocateString(str, pSysEnclosure->SN), 0);
	NWL_NodeAttrSet(tab, "Asset Tag", LocateString(str, pSysEnclosure->AssetTag), 0);
}

static const CHAR*
pProcessorFamilyToStr(UCHAR Family)
{
	switch (Family)
	{
	case 0x3: return "8086";
	case 0x4: return "80286";
	case 0x5: return "Intel386 processor";
	case 0x6: return "Intel486 processor";
	case 0x7: return "8087";
	case 0x8: return "80287";
	case 0x9: return "80387";
	case 0xA: return "80487";
	case 0xB: return "Pentium processor Family";
	case 0xC: return "Pentium Pro processor";
	case 0xD: return "Pentium II processor";
	case 0xE: return "Pentium processor with MMX technology";
	case 0xF: return "Celeron processor";
	case 0x10: return "Pentium II Xeon processor";
	case 0x11: return "Pentium III processor";
	case 0x12: return "M1 Family";
	case 0x13: return "M2 Family";
	case 0x14: return "Intel Celeron M processor";
	case 0x15: return "Intel Pentium 4 HT processor";

	case 0x18: return "AMD Duron Processor Family";
	case 0x19: return "K5 Family";
	case 0x1A: return "K6 Family";
	case 0x1B: return "K6 - 2";
	case 0x1C: return "K6 - 3";
	case 0x1D: return "AMD Athlon Processor Family";
	case 0x1E: return "AMD29000 Family";
	case 0x1F: return "K6 - 2 +";

	case 0x28: return "Intel Core Duo processor";
	case 0x29: return "Intel Core Duo mobile processor";
	case 0x2A: return "Intel Core Solo mobile processor";
	case 0x2B: return "Intel Atom processor";
	case 0x2C: return "Intel CoreM processor";
	case 0x2D: return "Intel Core m3 processor";
	case 0x2E: return "Intel Core m5 processor";
	case 0x2F: return "Intel Core m7 processor";

	case 0x38: return "AMD Turion II Ultra Dual - Core Mobile M Processor Family";
	case 0x39: return "AMD Turion II Dual - Core Mobile M Processor Family";
	case 0x3A: return "AMD Athlon II Dual - Core M Processor Family";
	case 0x3B: return "AMD Opteron 6100 Series Processor";
	case 0x3C: return "AMD Opteron 4100 Series Processor";
	case 0x3D: return "AMD Opteron 6200 Series Processor";
	case 0x3E: return "AMD Opteron 4200 Series Processor";
	case 0x3F: return "AMD FX Series Proceesor";

	case 0x46: return "AMD C - Series Processor";
	case 0x47: return "AMD E - Series Processor";
	case 0x48: return "AMD S - Series Processor";
	case 0x49: return "AMD G - Series Processor";
	case 0x4A: return "AMD Z - Series Processor";
	case 0x4B: return "AMD R - Series Procerssor";
	case 0x4C: return "AMD Opteron 4300 Series Processor";
	case 0x4D: return "AMD Opteron 6300 Series Processor";
	case 0x4E: return "AMD Opteron 3300 Series Processor";
	case 0x4F: return "AMD FirePro Series Processor";

	case 0x60: return "68040 Family";
	case 0x61: return "68xxx";
	case 0x62: return "68000";
	case 0x63: return "68010";
	case 0x64: return "68020";
	case 0x65: return "68030";
	case 0x66: return "AMD Athlon X4 Quad - Core Processor Family";
	case 0x67: return "AMD Opteron X1000 Series Processor";
	case 0x68: return "AMD Opteron X2000 Series APU";
	case 0x69: return "AMD Opteron A - Series Processor";
	case 0x6A: return "AMD Opteron X3000 Series APU";
	case 0x6B: return "AMD Zen Processor Family";

	case 0x78: return "Crusoe TM5000 Family";
	case 0x79: return "Crusoe TM3000 Family";
	case 0x7A: return "Efficeon TM8000 Family";

	case 0x80: return "Weitek";

	case 0x82: return "Itanium processor";
	case 0x83: return "AMD Athlon 64 Processor Family";
	case 0x84: return "AMD Opteron Processor Family";
	case 0x85: return "AMD Sempron Processor Family";
	case 0x86: return "AMD Turion 64 Mobile Technology";
	case 0x87: return "Dual - Core AMD Opteron Processor Family";
	case 0x88: return "AMD Athlon 64 X2 Dual - Core Processor Family";
	case 0x89: return "AMD Turion 64 X2 Mobile Technology";
	case 0x8A: return "Quad - Core AMD Opteron Processor Family";
	case 0x8B: return "Third - Generation AMD Opteron Processor Family";
	case 0x8C: return "AMD Phenom FX Quard - Core Processor Family";
	case 0x8D: return "AMD Phenom FX X4 Quard - Core Processor Family";
	case 0x8E: return "AMD Phenom FX X2 Quard - Core Processor Family";
	case 0x8F: return "AMD Athlon X2 Dual - Core Processor Family";

	case 0xA0: return "V30 Family";
	case 0xA1: return "Quad - Core Intel Xeon processor 3200 Series";
	case 0xA2: return "Dual - Core Intel Xeon processor 3000 Series";
	case 0xA3: return "Quad - Core Intel Xeon processor 5300 Series";
	case 0xA4: return "Dual - Core Intel Xeon processor 5100 Series";
	case 0xA5: return "Dual - Core Intel Xeon processor 5000 Series";
	case 0xA6: return "Dual - Core Intel Xeon processor LV";
	case 0xA7: return "Dual - Core Intel Xeon processor ULV";
	case 0xA8: return "Dual - Core Intel Xeon processor 7100 Series";
	case 0xA9: return "Quad - Core Intel Xeon processor 5400 Series";
	case 0xAA: return "Quad - Core Intel Xeon processor";
	case 0xAB: return "Dual - Core Intel Xeon processor 5200 Series";
	case 0xAC: return "Dual - Core Intel Xeon processor 7200 Series";
	case 0xAD: return "Quad - Core Intel Xeon processor 7300 Series";
	case 0xAE: return "Quad - Core Intel Xeon processor 7400 Series";
	case 0xAF: return "Multi - Core Intel Xeon processor 7400 Series";
	case 0xB0: return "Pentium III Xeon processor";
	case 0xB1: return "Pentium III Processor with Intel SpeedStep.Technology";
	case 0xB2: return "Pentium 4 Processor";
	case 0xB3: return "Intel Xeon";
	case 0xB4: return "AS400 Family";
	case 0xB5: return "Intel Xeon processor MP";
	case 0xB6: return "AMD Athlon XP Processor Family";
	case 0xB7: return "AMD Athlon MP Processor Family";
	case 0xB8: return "Intel ItaniumR 2 processor";
	case 0xB9: return "Intel Pentium M processor";
	case 0xBA: return "Intel Celeron D processor";
	case 0xBB: return "Intel Pentium D processor";
	case 0xBC: return "Intel Pentium Processor Extreme Edition";
	case 0xBD: return "Intel Core Solo Processor";
	case 0xBF: return "Intel Core 2 Duo Processor";
	case 0xC0: return "Intel Core 2 Solo processor";
	case 0xC1: return "Intel Core 2 Extreme processor";
	case 0xC2: return "Intel Core 2 Quad processor";
	case 0xC3: return "Intel Core 2 Extreme mobile processor";
	case 0xC4: return "Intel Core 2 Duo mobile processor";
	case 0xC5: return "Intel Core 2 Solo mobile processor";
	case 0xC6: return "Intel Core i7 processor";
	case 0xC7: return "Dual - Core Intel Celeron processor";
	case 0xC8: return "IBM390 Family";
	case 0xC9: return "G4";
	case 0xCA: return "G5";
	case 0xCB: return "ESA / 390 G6";
	case 0xCC: return "z / Architectur base";
	case 0xCD: return "Intel Core i5 processor";
	case 0xCE: return "Intel Core i3 processor";
	case 0xCF: return "Intel Core i9 processor";
	case 0xD0: return "Available for assignment";
	case 0xD1: return "Available for assignment";
	case 0xD2: return "VIA C7 - M Processor Family";
	case 0xD3: return "VIA C7 - D Processor Family";
	case 0xD4: return "VIA C7 Processor Family";
	case 0xD5: return "VIA Eden Processor Family";
	case 0xD6: return "Multi - Core Intel Xeon processor";
	case 0xD7: return "Dual - Core Intel Xeon processor 3xxx Series";
	case 0xD8: return "Quad - Core Intel Xeon processor 3xxx Series";
	case 0xD9: return "VIA Nano Processor Family";
	case 0xDA: return "Dual - Core Intel Xeon processor 5xxx Series";
	case 0xDB: return "Quad - Core Intel Xeon processor 5xxx Series";

	case 0xDD: return "Dual - Core Intel Xeon processor 7xxx Series";
	case 0xDE: return "Quad - Core Intel Xeon processor 7xxx Series";
	case 0xDF: return "Multi - Core Intel Xeon processor 7xxx Series";
	case 0xE0: return "Multi - Core Intel Xeon processor 3400 Series";

	case 0xE4: return "AMD Opteron 3000 Series Processor";
	case 0xE5: return "AMD Sempron II Processor";
	case 0xE6: return "Embedded AMD Opteron Quad - Core Processor Family";
	case 0xE7: return "AMD Phenom Triple - Core Processor Family";
	case 0xE8: return "AMD Turion Ultra Dual - Core Mobile Processor Family";
	case 0xE9: return "AMD Turion Dual - Core Mobile Processor Family";
	case 0xEA: return "AMD Athlon Dual - Core Processor Family";
	case 0xEB: return "AMD Sempron SI Processor Family";
	case 0xEC: return "AMD Phenom II Processor Family";
	case 0xED: return "AMD Athlon II Processor Family";
	case 0xEE: return "Six - Core AMD Opteron Processor Family";
	case 0xEF: return "AMD Sempron M Processor Family";
	}
	return "Unknown";
}

static void ProcProcessorInfo(PNODE tab, void* p)
{
	PProcessorInfo	pProcessor = (PProcessorInfo)p;
	const char* str = toPointString(p);
	NWL_NodeAttrSet(tab, "Description", "Processor Information", 0);
	NWL_NodeAttrSet(tab, "Socket Designation", LocateString(str, pProcessor->SocketDesignation), 0);
	NWL_NodeAttrSet(tab, "Processor Family", pProcessorFamilyToStr(pProcessor->Family), 0);
	NWL_NodeAttrSet(tab, "Processor Manufacturer", LocateString(str, pProcessor->Manufacturer), 0);
	NWL_NodeAttrSet(tab, "Processor Version", LocateString(str, pProcessor->Version), 0);
	if (!pProcessor->Voltage)
	{
		// unsupported
	}
	else if (pProcessor->Voltage & (1U << 7))
	{
		UCHAR volt = pProcessor->Voltage - 0x80;
		NWL_NodeAttrSetf(tab, "Voltage", 0, "%u.%u V", volt / 10, volt % 10);
	}
	else
	{
		NWL_NodeAttrSetf(tab, "Voltage", 0, "%s%s%s",
			pProcessor->Voltage & (1U << 0) ? " 5 V" : "",
			pProcessor->Voltage & (1U << 1) ? " 3.3 V" : "",
			pProcessor->Voltage & (1U << 2) ? " 2.9 V" : "");
	}
	if (pProcessor->ExtClock)
		NWL_NodeAttrSetf(tab, "External Clock (MHz)", NAFLG_FMT_NUMERIC, "%u", pProcessor->ExtClock);
	NWL_NodeAttrSetf(tab, "Max Speed (MHz)", NAFLG_FMT_NUMERIC, "%u", pProcessor->MaxSpeed);
	NWL_NodeAttrSetf(tab, "Current Speed (MHz)", NAFLG_FMT_NUMERIC, "%u", pProcessor->CurrentSpeed);
	if (pProcessor->Header.Length > 0x20)
	{
		// 2.3+
		NWL_NodeAttrSet(tab, "Serial Number", LocateString(str, pProcessor->Serial), 0);
		NWL_NodeAttrSet(tab, "Asset Tag", LocateString(str, pProcessor->AssetTag), 0);
		if (pProcessor->CoreCount == 0xff && pProcessor->Header.Length > 0x2a)
			NWL_NodeAttrSetf(tab, "Core Count", NAFLG_FMT_NUMERIC, "%u", pProcessor->CoreCount2);
		else
			NWL_NodeAttrSetf(tab, "Core Count", NAFLG_FMT_NUMERIC, "%u", pProcessor->CoreCount);
		if (pProcessor->ThreadCount == 0xff && pProcessor->Header.Length > 0x2a)
			NWL_NodeAttrSetf(tab, "Thread Count", NAFLG_FMT_NUMERIC, "%u", pProcessor->ThreadCount2);
		else
			NWL_NodeAttrSetf(tab, "Thread Count", NAFLG_FMT_NUMERIC, "%u", pProcessor->ThreadCount);
	}
}

static void ProcMemCtrlInfo(PNODE tab, void* p)
{
	PMemCtrlInfo pMemCtrl = (PMemCtrlInfo)p;

	NWL_NodeAttrSet(tab, "Description", "Memory Controller Information", 0);
	NWL_NodeAttrSetf(tab, "Max Memory Module Size (MB)", NAFLG_FMT_NUMERIC, "%llu", 2ULL << pMemCtrl->MaxMemModuleSize);
	NWL_NodeAttrSetf(tab, "Number of Slots", NAFLG_FMT_NUMERIC, "%u", pMemCtrl->NumOfSlots);
}

static void ProcMemModuleInfo(PNODE tab, void* p)
{
	PMemModuleInfo	pMemModule = (PMemModuleInfo)p;
	const char* str = toPointString(p);
	UCHAR sz = 0;
	NWL_NodeAttrSet(tab, "Description", "Memory Module Information", 0);
	NWL_NodeAttrSet(tab, "Socket Designation", LocateString(str, pMemModule->SocketDesignation), 0);
	NWL_NodeAttrSetf(tab, "Current Speed (ns)", NAFLG_FMT_NUMERIC, "%u", pMemModule->CurrentSpeed);
	sz = pMemModule->InstalledSize & 0x7F;
	if (sz > 0x7D)
		sz = 0;
	NWL_NodeAttrSetf(tab, "Installed Size (MB)", NAFLG_FMT_NUMERIC, "%llu", 2ULL << sz);
}

static void ProcCacheInfo(PNODE tab, void* p)
{
	PCacheInfo	pCache = (PCacheInfo)p;
	const char* str = toPointString(p);
	UINT64 sz = 0;

	NWL_NodeAttrSet(tab, "Description", "Cache Information", 0);
	NWL_NodeAttrSet(tab, "Socket Designation", LocateString(str, pCache->SocketDesignation), 0);
	if (pCache->MaxSize == 0xffff && pCache->Header.Length > 0x13)
	{
		if (pCache->MaxSize2 & (1ULL << 31))
			sz = ((UINT64)pCache->MaxSize2 - (1ULL << 31)) * 64 * 1024;
		else
			sz = ((UINT64)pCache->MaxSize2) * 1024;
	}
	else if (pCache->MaxSize & (1ULL << 15))
		sz = ((UINT64)pCache->MaxSize - (1ULL << 15)) * 64 * 1024;
	else
		sz = ((UINT64) pCache->MaxSize) * 1024;
	NWL_NodeAttrSet(tab, "Max Cache Size", NWL_GetHumanSize(sz, mem_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
	if (pCache->InstalledSize == 0xffff && pCache->Header.Length > 0x13)
	{
		if (pCache->InstalledSize2 & (1ULL << 31))
			sz = ((UINT64)pCache->InstalledSize2 - (1ULL << 31)) * 64 * 1024;
		else
			sz = ((UINT64)pCache->InstalledSize2) * 1024;
	}
	else if (pCache->InstalledSize & (1ULL << 15))
	{
		sz = ((UINT64)pCache->InstalledSize - (1ULL << 15)) * 64 * 1024;
	}
	else
	{
		sz = ((UINT64)pCache->InstalledSize) * 1024;
	}
	NWL_NodeAttrSet(tab, "Installed Cache Size", NWL_GetHumanSize(sz, mem_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
	if (pCache->Speed)
		NWL_NodeAttrSetf(tab, "Cache Speed (ns)", NAFLG_FMT_NUMERIC, "%u", pCache->Speed);
}

static const CHAR*
pIntConnectTypeToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x00: return "None";
	case 0x01: return "Centronics";
	case 0x02: return "Mini Centronics";
	case 0x03: return "Proprietary";
	case 0x04: return "DB-25 pin male";
	case 0x05: return "DB-25 pin female";
	case 0x06: return "DB-15 pin male";
	case 0x07: return "DB-15 pin female";
	case 0x08: return "DB-9 pin male";
	case 0x09: return "DB-9 pin female";
	case 0x0a: return "RJ-11";
	case 0x0b: return "RJ-45";
	case 0x0c: return "50 Pin MiniSCSI";
	case 0x0d: return "Mini-DIN";
	case 0x0e: return "Micro-DIN";
	case 0x0f: return "PS/2";
	case 0x10: return "Infrared";
	case 0x11: return "HP-HIL";
	case 0x12: return "Access Bus (USB)";
	case 0x13: return "SSA SCSI";
	case 0x14: return "Circular DIN-8 male";
	case 0x15: return "Circular DIN-8 female";
	case 0x16: return "On Board IDE";
	case 0x17: return "On Board Floppy";
	case 0x18: return "9-pin Dual Inline (pin 10 cut)";
	case 0x19: return "25-pin Dual Inline (pin 26 cut)";
	case 0x1a: return "50-pin Dual Inline";
	case 0x1b: return "68-pin Dual Inline";
	case 0x1c: return "On Board Sound Input from CD-ROM";
	case 0x1d: return "Mini-Centronics Type-14";
	case 0x1e: return "Mini-Centronics Type-26";
	case 0x1f: return "Mini-jack (headphones)";
	case 0x20: return "BNC";
	case 0x21: return "1394";
	case 0x22: return "SAS/SATA Plug Receptacle";
	case 0x23: return "USB Type-C Receptacle";
	case 0xa0: return "PC-98";
	case 0xa1: return "PC-98Hireso";
	case 0xa2: return "PC-H98";
	case 0xa3: return "PC-98Note";
	case 0xa4: return "PC-98Full";
	}
	return "Other";
}

static const CHAR*
pPortTypeToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x00: return "None";
	case 0x01: return "Parallel Port XT/AT Compatible";
	case 0x02: return "Parallel Port PS/2";
	case 0x03: return "Parallel Port ECP";
	case 0x04: return "Parallel Port EPP";
	case 0x05: return "Parallel Port ECP/EPP";
	case 0x06: return "Serial Port XT/AT Compatible";
	case 0x07: return "Serial Port 16450 Compatible";
	case 0x08: return "Serial Port 16550 Compatible";
	case 0x09: return "Serial Port 16550A Compatible";
	case 0x0a: return "SCSI Port";
	case 0x0b: return "MIDI Port";
	case 0x0c: return "Joy Stick Port";
	case 0x0d: return "Keyboard Port";
	case 0x0e: return "Mouse Port";
	case 0x0f: return "SSA SCSI";
	case 0x10: return "USB";
	case 0x11: return "FireWire (IEEE P1394)";
	case 0x12: return "PCMCIA Type I";
	case 0x13: return "PCMCIA Type II";
	case 0x14: return "PCMCIA Type III";
	case 0x15: return "Cardbus";
	case 0x16: return "Access Bus Port";
	case 0x17: return "SCSI II";
	case 0x18: return "SCSI Wide";
	case 0x19: return "PC-98";
	case 0x1a: return "PC-98-Hireso";
	case 0x1b: return "PC-H98";
	case 0x1c: return "Video Port";
	case 0x1d: return "Audio Port";
	case 0x1e: return "Modem Port";
	case 0x1f: return "Network Port";
	case 0x20: return "SATA";
	case 0x21: return "SAS";
	case 0x22: return "MFDP (Multi-Function Display Port)";
	case 0x23: return "Thunderbolt";
	case 0xa0: return "8251 Compatible";
	case 0xa1: return "8251 FIFO Compatible";
	}
	return "Other";
}

static void ProcPortConnectInfo(PNODE tab, void* p)
{
	PPortConnectInfo pPort = (PPortConnectInfo)p;
	const char* str = toPointString(p);

	NWL_NodeAttrSet(tab, "Description", "Port Connector Information", 0);
	NWL_NodeAttrSet(tab, "Internal Reference Designator", LocateString(str, pPort->IntDesignator), 0);
	NWL_NodeAttrSet(tab, "Internal Connector Type", pIntConnectTypeToStr(pPort->IntConnectorType), 0);
	NWL_NodeAttrSet(tab, "External Reference Designator", LocateString(str, pPort->ExtDesignator), 0);
	NWL_NodeAttrSet(tab, "External Connector Type", pIntConnectTypeToStr(pPort->ExtConnectorType), 0);
	NWL_NodeAttrSet(tab, "Port Type", pPortTypeToStr(pPort->PortType), 0);
}

static void ProcPSystemSlots(PNODE tab, void* p)
{
	PSystemSlots pSys = (PSystemSlots)p;
	const char* str = toPointString(p);

	NWL_NodeAttrSet(tab, "Description", "System Slots", 0);
	NWL_NodeAttrSet(tab, "Slot Designation", LocateString(str, pSys->SlotDesignation), 0);
}

static const CHAR*
pOnBoardDeviceTypeToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x01: return "Other";
	//case 0x02: return "Unknown";
	case 0x03: return "Video";
	case 0x04: return "SCSI Controller";
	case 0x05: return "Ethernet";
	case 0x06: return "Token Ring";
	case 0x07: return "Sound";
	case 0x08: return "PATA Controller";
	case 0x09: return "SATA Controller";
	case 0x0a: return "SAS Controller";
	}
	return "Unknown";
}

static void ProcOnBoardDevInfo(PNODE tab, void* p)
{
	UINT count, i;
	PNODE ndev;
	POnBoardDevicesInfo pDev = (POnBoardDevicesInfo)p;
	const char* str = toPointString(p);

	count = (pDev->Header.Length - sizeof(SMBIOSHEADER)) / (sizeof(pDev->DeviceInfo[0]));
	NWL_NodeAttrSet(tab, "Description", "On Board Devices Information", 0);
	NWL_NodeAttrSetf(tab, "Number of Devices", NAFLG_FMT_NUMERIC, "%u", count);
	ndev = NWL_NodeAppendNew(tab, "On Board Devices", NFLG_TABLE);
	for (i = 0; i < count; i++)
	{
		PNODE p = NWL_NodeAppendNew(ndev, "Device", NFLG_TABLE_ROW);
		UCHAR type = pDev->DeviceInfo[i].DeviceType & 0x7f;
		UCHAR status = pDev->DeviceInfo[i].DeviceType & 0x90;
		NWL_NodeAttrSet(p, "Type", pOnBoardDeviceTypeToStr(type), 0);
		NWL_NodeAttrSet(p, "Status", status ? "Enabled" : "Disabled", 0);
		NWL_NodeAttrSet(p, "Description", LocateString(str, pDev->DeviceInfo[i].Description), 0);
	}
}

static void ProcOEMString(PNODE tab, void* p)
{
	UCHAR i;
	PNODE nstr;
	POEMString pString = (POEMString)p;
	const char* str = toPointString(p);
	NWL_NodeAttrSet(tab, "Description", "OEM String", 0);
	NWL_NodeAttrSetf(tab, "Number of Strings", NAFLG_FMT_NUMERIC, "%u", pString->Count);
	nstr = NWL_NodeAppendNew(tab, "OEM Strings", NFLG_TABLE);
	for (i = 1; i <= pString->Count; i++)
	{
		PNODE p = NWL_NodeAppendNew(nstr, "OEM String", NFLG_TABLE_ROW);
		NWL_NodeAttrSet(p, "String", LocateString(str, i), 0);
	}
}

static void ProcSysConfOptions(PNODE tab, void* p)
{
	UCHAR i;
	PNODE nstr;
	POEMString pString = (POEMString)p;
	const char* str = toPointString(p);
	NWL_NodeAttrSet(tab, "Description", "System Configuration Options", 0);
	NWL_NodeAttrSetf(tab, "Number of Strings", NAFLG_FMT_NUMERIC, "%u", pString->Count);
	nstr = NWL_NodeAppendNew(tab, "Configuration Strings", NFLG_TABLE);
	for (i = 1; i <= pString->Count; i++)
	{
		PNODE p = NWL_NodeAppendNew(nstr, "String", NFLG_TABLE_ROW);
		NWL_NodeAttrSet(p, "String", LocateString(str, i), 0);
	}
}

static void ProcBIOSLangInfo(PNODE tab, void* p)
{
	PBIOSLangInfo pLang = (PBIOSLangInfo)p;
	const char* str = toPointString(p);

	NWL_NodeAttrSet(tab, "Description", "BIOS Language Information", 0);
	NWL_NodeAttrSetf(tab, "Installable Languages", NAFLG_FMT_NUMERIC, "%u", pLang->InstallableLang);
	NWL_NodeAttrSet(tab, "Current Language", LocateString(str, pLang->CurrentLang), 0);
}

static void ProcGroupAssoc(PNODE tab, void* p)
{
	PGroupAssoc pGA = (PGroupAssoc)p;
	const char* str = toPointString(p);

	NWL_NodeAttrSet(tab, "Description", "Group Associations", 0);
	NWL_NodeAttrSet(tab, "Group Name", LocateString(str, pGA->GroupName), 0);
	NWL_NodeAttrSetf(tab, "Item Type", NAFLG_FMT_NUMERIC, "%u", pGA->ItemType);
	NWL_NodeAttrSetf(tab, "Item Handle", NAFLG_FMT_NUMERIC, "%u", pGA->ItemHandle);
}

static const CHAR*
pMALocationToStr(UCHAR Location)
{
	switch (Location)
	{
	case 0x01: return "Other";
	//case 0x02: return "Unknown";
	case 0x03: return "System board";
	case 0x04: return "ISA add-on card";
	case 0x05: return "EISA add-on card";
	case 0x06: return "PCI add-on card";
	case 0x07: return "MCA add-on card";
	case 0x08: return "PCMCIA add-on card";
	case 0x09: return "Proprietary add-on card";
	case 0x0a: return "NuBus";
	case 0xa0: return "PC-98/C20 add-on card";
	case 0xa1: return "PC-98/C24 add-on card";
	case 0xa2: return "PC-98/E add-on card";
	case 0xa3: return "PC-98/Local bus add-on card add-on card";
	case 0xa4: return "CXL add-on card";
	}
	return "Unknown";
}

static const CHAR*
pMAUseToStr(UCHAR Use)
{
	switch (Use)
	{
	case 0x01: return "Other";
	//case 0x02: return "Unknown";
	case 0x03: return "System memory";
	case 0x04: return "Video memory";
	case 0x05: return "Flash memory";
	case 0x06: return "Non-volatile RAM";
	case 0x07: return "Cache memory";
	}
	return "Unknown";
}

static const CHAR*
pMAEccToStr(UCHAR ErrCorrection)
{
	switch (ErrCorrection)
	{
	case 0x01: return "Other";
	//case 0x02: return "Unknown";
	case 0x03: return "None";
	case 0x04: return "Parity";
	case 0x05: return "Single-bit ECC";
	case 0x06: return "Multi-bit ECC";
	case 0x07: return "CRC";
	}
	return "Unknown";
}

static void ProcMemoryArray(PNODE tab, void* p)
{
	PMemoryArray pMA = (PMemoryArray)p;
	UINT64 sz = 0;
	NWL_NodeAttrSet(tab, "Description", "Memory Array", 0);
	NWL_NodeAttrSet(tab, "Location", pMALocationToStr(pMA->Location), 0);
	NWL_NodeAttrSet(tab, "Function", pMAUseToStr(pMA->Use), 0);
	NWL_NodeAttrSet(tab, "Error Correction", pMAEccToStr(pMA->ErrCorrection), 0);
	NWL_NodeAttrSetf(tab, "Number of Slots", NAFLG_FMT_NUMERIC, "%u", pMA->NumOfMDs);
	if (pMA->MaxCapacity == 0x80000000 && pMA->Header.Length > 0x0f)
		sz = pMA->ExtMaxCapacity;
	else
		sz = ((UINT64)pMA->MaxCapacity) * 1024;
	NWL_NodeAttrSet(tab, "Max Capacity", NWL_GetHumanSize(sz, mem_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
}

static const CHAR*
pMDMemoryTypeToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x01: return "Other";
	//case 0x02: return "Unknown";
	case 0x03: return "DRAM";
	case 0x04: return "EDRAM";
	case 0x05: return "VRAM";
	case 0x06: return "SRAM";
	case 0x07: return "RAM";
	case 0x08: return "ROM";
	case 0x09: return "FLASH";
	case 0x0a: return "EEPROM";
	case 0x0b: return "FEPROM";
	case 0x0c: return "EPROM";
	case 0x0d: return "CDRAM";
	case 0x0e: return "3DRAM";
	case 0x0f: return "SDRAM";
	case 0x10: return "SGRAM";
	case 0x11: return "RDRAM";
	case 0x12: return "DDR";
	case 0x13: return "DDR2";
	case 0x14: return "DDR2 FB-DIMM";
	case 0x15:
	case 0x16:
	case 0x17: return "Reserved";
	case 0x18: return "DDR3";
	case 0x19: return "FBD2";
	case 0x1a: return "DDR4";
	case 0x1b: return "LPDDR";
	case 0x1c: return "LPDDR2";
	case 0x1d: return "LPDDR3";
	case 0x1e: return "LPDDR4";
	case 0x1f: return "Logical non-volatile device";
	case 0x20: return "HBM (High Bandwidth Memory)";
	case 0x21: return "HBM2 (High Bandwidth Memory Generation 2)";
	case 0x22: return "DDR5";
	case 0x23: return "LPDDR5";
	}
	return "Unknown";
}

static void ProcMemoryDevice(PNODE tab, void* p)
{
	PMemoryDevice pMD = (PMemoryDevice)p;
	const char* str = toPointString(p);
	UINT64 sz = 0;

	NWL_NodeAttrSet(tab, "Description", "Memory Device", 0);
	NWL_NodeAttrSet(tab, "Device Locator", LocateString(str, pMD->DeviceLocator), 0);
	NWL_NodeAttrSet(tab, "Bank Locator", LocateString(str, pMD->BankLocator), 0);
	if (pMD->TotalWidth)
		NWL_NodeAttrSetf(tab, "Total Width (bits)", NAFLG_FMT_NUMERIC, "%u", pMD->TotalWidth);
	if (pMD->DataWidth)
		NWL_NodeAttrSetf(tab, "Data Width (bits)", NAFLG_FMT_NUMERIC, "%u", pMD->DataWidth);
	if (pMD->Size & (1ULL << 15))
		sz = ((UINT64)pMD->Size - (1ULL << 15)) * 1024;
	else
		sz = ((UINT64)pMD->Size) * 1024 * 1024;
	if (!sz)
		return;
	NWL_NodeAttrSet(tab, "Device Size", NWL_GetHumanSize(sz, mem_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
	NWL_NodeAttrSet(tab, "Device Type", pMDMemoryTypeToStr(pMD->MemoryType), 0);
	if (pMD->Header.Length > 0x15)
	{
		if (pMD->Speed)
			NWL_NodeAttrSetf(tab, "Speed (MT/s)", NAFLG_FMT_NUMERIC, "%u", pMD->Speed);
		NWL_NodeAttrSet(tab, "Manufacturer", LocateString(str, pMD->Manufacturer), 0);
		NWL_NodeAttrSet(tab, "Serial Number", LocateString(str, pMD->SN), 0);
		NWL_NodeAttrSet(tab, "Asset Tag Number", LocateString(str, pMD->AssetTag), 0);
		NWL_NodeAttrSet(tab, "Part Number", LocateString(str, pMD->PN), 0);
	}
}

static void ProcMemoryArrayMappedAddress(PNODE tab, void* p)
{
	PMemoryArrayMappedAddress pMAMA = (PMemoryArrayMappedAddress)p;

	NWL_NodeAttrSet(tab, "Description", "Memory Array Mapped Address", 0);
	NWL_NodeAttrSetf(tab, "Starting Address", 0, "0x%016llX",
		pMAMA->StartAddr == 0xFFFFFFFF ? pMAMA->ExtStartAddr : pMAMA->StartAddr);
	NWL_NodeAttrSetf(tab, "Ending Address", 0, "0x%016llX",
		pMAMA->EndAddr == 0xFFFFFFFF ? pMAMA->ExtEndAddr : pMAMA->EndAddr);
	NWL_NodeAttrSetf(tab, "Memory Array Handle", NAFLG_FMT_NUMERIC, "%u", pMAMA->Handle);
	NWL_NodeAttrSetf(tab, "Partition Width", 0, "0x%X", pMAMA->PartitionWidth);
}

static void ProcMemoryDeviceMappedAddress(PNODE tab, void* p)
{
	PMemoryDeviceMappedAddress pMDMA = (PMemoryDeviceMappedAddress)p;

	NWL_NodeAttrSet(tab, "Description", "Memory Device Mapped Address", 0);
	NWL_NodeAttrSetf(tab, "Starting Address", 0, "0x%016llX",
		pMDMA->StartAddr == 0xFFFFFFFF ? pMDMA->ExtStartAddr : pMDMA->StartAddr);
	NWL_NodeAttrSetf(tab, "Ending Address", 0, "0x%016llX",
		pMDMA->EndAddr == 0xFFFFFFFF ? pMDMA->ExtEndAddr : pMDMA->EndAddr);
	NWL_NodeAttrSetf(tab, "Memory Device Handle", NAFLG_FMT_NUMERIC, "%u", pMDMA->MDHandle);
	NWL_NodeAttrSetf(tab, "Memory Array Mapped Address Handle", NAFLG_FMT_NUMERIC, "%u", pMDMA->MAMAHandle);
}

static void ProcPortableBattery(PNODE tab, void* p)
{
	PPortableBattery pPB = (PPortableBattery)p;
	const char* str = toPointString(p);

	NWL_NodeAttrSet(tab, "Description", "Portable Battery", 0);
	NWL_NodeAttrSet(tab, "Location", LocateString(str, pPB->Location), 0);
	NWL_NodeAttrSet(tab, "Manufacturer", LocateString(str, pPB->Manufacturer), 0);
	NWL_NodeAttrSet(tab, "Manufacturer Date", LocateString(str, pPB->Date), 0);
	NWL_NodeAttrSet(tab, "Serial Number", LocateString(str, pPB->SN), 0);
	NWL_NodeAttrSet(tab, "Device Name", LocateString(str, pPB->DeviceName), 0);
}

static void ProcSysBootInfo(PNODE tab, void* p)
{
	PSysBootInfo pBootInfo = (PSysBootInfo)p;
	const char* str = toPointString(p);

	NWL_NodeAttrSet(tab, "Description", "System Boot Information", 0);
}

static void ProcTPMDevice(PNODE tab, void* p)
{
	PTPMDevice pTPM = (PTPMDevice)p;
	const char* str = toPointString(p);

	NWL_NodeAttrSet(tab, "Description", "TPM Device", 0);
	NWL_NodeAttrSetf(tab, "Vendor", 0, "%c%c%c%c",
		pTPM->Vendor[0], pTPM->Vendor[1],
		pTPM->Vendor[2], pTPM->Vendor[3]);
	NWL_NodeAttrSetf(tab, "Spec Version", 0, "%u%u", pTPM->MajorSpecVer, pTPM->MinorSpecVer);
	NWL_NodeAttrSet(tab, "Description", LocateString(str, pTPM->Description), 0);
}

static void ProcEndTable(PNODE tab, void* p)
{
	NWL_NodeAttrSet(tab, "Description", "End-of-Table", 0);
}

static void DumpSMBIOSStruct(PNODE node, void* Addr, UINT Len, UINT8 Type)
{
	LPBYTE p = (LPBYTE)(Addr);
	const LPBYTE lastAddress = p + Len;
	PSMBIOSHEADER pHeader;
	PNODE tab;

	for (;;) {
		pHeader = (PSMBIOSHEADER)p;
		if (Type != 127 && pHeader->Type != Type)
			goto next_table;
		tab = NWL_NodeAppendNew(node, "Table", NFLG_TABLE_ROW);
		NWL_NodeAttrSetf(tab, "Table Type", NAFLG_FMT_NUMERIC, "%u", pHeader->Type);
		NWL_NodeAttrSetf(tab, "Table Length", NAFLG_FMT_NUMERIC, "%u", pHeader->Length);
		NWL_NodeAttrSetf(tab, "Table Handle", NAFLG_FMT_NUMERIC, "%u", pHeader->Handle);
		switch (pHeader->Type)
		{
		case 0:
			ProcBIOSInfo(tab, pHeader);
			break;
		case 1:
			ProcSysInfo(tab, pHeader);
			break;
		case 2:
			ProcBoardInfo(tab, pHeader);
			break;
		case 3:
			ProcSystemEnclosure(tab, pHeader);
			break;
		case 4:
			ProcProcessorInfo(tab, pHeader);
			break;
		case 5:
			ProcMemCtrlInfo(tab, pHeader);
			break;
		case 6:
			ProcMemModuleInfo(tab, pHeader);
			break;
		case 7:
			ProcCacheInfo(tab, pHeader);
			break;
		case 8:
			ProcPortConnectInfo(tab, pHeader);
			break;
		case 9:
			ProcPSystemSlots(tab, pHeader);
			break;
		case 10:
			ProcOnBoardDevInfo(tab, pHeader);
			break;
		case 11:
			ProcOEMString(tab, pHeader);
			break;
		case 12:
			ProcSysConfOptions(tab, pHeader);
			break;
		case 13:
			ProcBIOSLangInfo(tab, pHeader);
			break;
		case 14:
			ProcGroupAssoc(tab, pHeader);
			break;
		case 16:
			ProcMemoryArray(tab, pHeader);
			break;
		case 17:
			ProcMemoryDevice(tab, pHeader);
			break;
		case 19:
			ProcMemoryArrayMappedAddress(tab, pHeader);
			break;
		case 20:
			ProcMemoryDeviceMappedAddress(tab, pHeader);
			break;
		case 22:
			ProcPortableBattery(tab, pHeader);
			break;
		case 32:
			ProcSysBootInfo(tab, pHeader);
			break;
		case 43:
			ProcTPMDevice(tab, pHeader);
			break;
		case 127:
			ProcEndTable(tab, pHeader);
			break;
		default:
			break;
		}
	next_table:
		if ((pHeader->Type == 127) && (pHeader->Length == 4))
			break; // last avaiable tables
		LPBYTE nt = p + pHeader->Length; // point to struct end
		while (0 != (*nt | *(nt + 1))) nt++; // skip string area
		nt += 2;
		if (nt >= lastAddress)
			break;
		p = nt;
	}
}

PNODE NW_Smbios(VOID)
{
	DWORD smBiosDataSize = 0;
	struct RAW_SMBIOS_DATA* smBiosData = NULL;
	PNODE node = NWL_NodeAlloc("SMBIOS", NFLG_TABLE);
	PNODE info = NWL_NodeAppendNew(node, "DMI", NFLG_TABLE_ROW);
	if (NWLC->DmiInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	// Query size of SMBIOS data.
	smBiosDataSize = NWL_GetSystemFirmwareTable('RSMB', 0, NULL, 0);
	if (smBiosDataSize == 0)
		return node;
	// Allocate memory for SMBIOS data
	smBiosData = (struct RAW_SMBIOS_DATA*)malloc(smBiosDataSize);
	if (!smBiosData)
		return node;
	// Retrieve the SMBIOS table
	NWL_GetSystemFirmwareTable('RSMB', 0, smBiosData, smBiosDataSize);
	NWL_NodeAttrSetf(info, "SMBIOS Version", 0, "%u.%u", smBiosData->MajorVersion, smBiosData->MinorVersion);
	if (smBiosData->DmiRevision)
		NWL_NodeAttrSetf(info, "DMI Version", NAFLG_FMT_NUMERIC, "%u", smBiosData->DmiRevision);
	DumpSMBIOSStruct(node, smBiosData->Data, smBiosData->Length, NWLC->SmbiosType);
	return node;
}
