// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "smbios.h"
#include "utils.h"

static const char* LocateString(UINT8* hdr, UINT8 offset)
{
	UINT8* ptr = hdr + hdr[1]; // PSMBIOSHEADER->Length
	UINT8* end = NWLC->NwSmbios->Data + NWLC->NwSmbios->Length;
	UINT8 i;

	if (offset == 0)
		goto fail;

	for (i = 1; *ptr != 0 && ptr < end; i++)
	{
		if (i == offset)
		{
			const char* str = (const char*)ptr;
			while (*ptr++ != 0)
				if (ptr >= end)
					goto fail;
			return str;
		}
		else
			while (*ptr++ != 0 && ptr < end);
	}
fail:
	return "NULL";
}

static void ProcBIOSInfo(PNODE tab, void* p)
{
	PBIOSInfo pBIOS = (PBIOSInfo)p;
	NWL_NodeAttrSet(tab, "Description", "BIOS Information", 0);
	if (pBIOS->Header.Length < 0x12) // 2.0
		return;
	NWL_NodeAttrSet(tab, "Vendor", LocateString(p, pBIOS->Vendor), 0);
	NWL_NodeAttrSet(tab, "Version", LocateString(p, pBIOS->Version), 0);
	NWL_NodeAttrSetf(tab, "Starting Segment", 0, "%04Xh", pBIOS->StartingAddrSeg);
	NWL_NodeAttrSet(tab, "Release Date", LocateString(p, pBIOS->ReleaseDate), 0);
	NWL_NodeAttrSetf(tab, "Image Size (K)", NAFLG_FMT_NUMERIC, "%u", (pBIOS->ROMSize + 1) * 64);
	NWL_NodeAttrSetf(tab, "BIOS Characteristics", 0, "0x%016llX", pBIOS->Characteristics);
	if (pBIOS->Header.Length < 0x18) // 2.4
		return;
	if (pBIOS->MajorRelease != 0xff || pBIOS->MinorRelease != 0xff)
		NWL_NodeAttrSetf(tab, "System BIOS Version", 0, "%u.%u", pBIOS->MajorRelease, pBIOS->MinorRelease);
	if (pBIOS->ECFirmwareMajor != 0xff || pBIOS->ECFirmwareMinor != 0xff)
		NWL_NodeAttrSetf(tab, "EC Firmware Version", 0, "%u.%u", pBIOS->ECFirmwareMajor, pBIOS->ECFirmwareMinor);
}

static const CHAR*
pWakeUpTypeToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x00: return "Reserved";
	case 0x01: return "Other";
	case 0x03: return "APM Timer";
	case 0x04: return "Modem Ring";
	case 0x05: return "LAN Remote";
	case 0x06: return "Power Switch";
	case 0x07: return "PCI PME#";
	case 0x08: return "AC Power Restored";
	}
	return "Unknown";
}

static void ProcSysInfo(PNODE tab, void* p)
{
	PSystemInfo pSystem = (PSystemInfo)p;
	NWL_NodeAttrSet(tab, "Description", "System Information", 0);
	if (pSystem->Header.Length < 0x08) // 2.0
		return;
	NWL_NodeAttrSet(tab, "Manufacturer", LocateString(p, pSystem->Manufacturer), 0);
	NWL_NodeAttrSet(tab, "Product Name", LocateString(p, pSystem->ProductName), 0);
	NWL_NodeAttrSet(tab, "Version", LocateString(p, pSystem->Version), 0);
	NWL_NodeAttrSet(tab, "Serial Number", LocateString(p, pSystem->SN), NAFLG_FMT_SENSITIVE);
	if (pSystem->Header.Length < 0x19) // 2.1
		return;
	NWL_NodeAttrSet(tab, "UUID", NWL_GuidToStr(pSystem->UUID), NAFLG_FMT_GUID);
	NWL_NodeAttrSet(tab, "Wake-up Type", pWakeUpTypeToStr(pSystem->WakeUpType), 0);
	if (pSystem->Header.Length < 0x1b) // 2.4
		return;
	NWL_NodeAttrSet(tab, "SKU Number", LocateString(p, pSystem->SKUNumber), 0);
	NWL_NodeAttrSet(tab, "Family", LocateString(p, pSystem->Family), 0);
}

static const CHAR*
pBoardTypeToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x02: return "Other";
	case 0x03: return "Server Blade";
	case 0x04: return "Connectivity Switch";
	case 0x05: return "System Management Module";
	case 0x06: return "Processor Module";
	case 0x07: return "I/O Module";
	case 0x08: return "Memory Module";
	case 0x09: return "Daughter Board";
	case 0x0a: return "Motherboard";
	case 0x0b: return "Processor/Memory Module";
	case 0x0c: return "Processor/IO Module";
	case 0x0d: return "Interconnect Board";
	}
	return "Unknown";
}

static void ProcBoardInfo(PNODE tab, void* p)
{
	PBoardInfo pBoard = (PBoardInfo)p;
	NWL_NodeAttrSet(tab, "Description", "Base Board Information", 0);
	if (pBoard->Header.Length < 0x08)
		return;
	NWL_NodeAttrSet(tab, "Manufacturer", LocateString(p, pBoard->Manufacturer), 0);
	NWL_NodeAttrSet(tab, "Product Name", LocateString(p, pBoard->Product), 0);
	NWL_NodeAttrSet(tab, "Version", LocateString(p, pBoard->Version), 0);
	NWL_NodeAttrSet(tab, "Serial Number", LocateString(p, pBoard->SN), NAFLG_FMT_SENSITIVE);
	if (pBoard->Header.Length < 0x09)
		return;
	NWL_NodeAttrSet(tab, "Asset Tag", LocateString(p, pBoard->AssetTag), 0);
	if (pBoard->Header.Length < 0x0a)
		return;
	NWL_NodeAttrSetf(tab, "Feature Flags", 0, "0x%02X", pBoard->FeatureFlags);
	if (pBoard->Header.Length < 0x0b)
		return;
	NWL_NodeAttrSet(tab, "Location in Chassis", LocateString(p, pBoard->LocationInChassis), 0);
	if (pBoard->Header.Length < 0x0d)
		return;
	NWL_NodeAttrSetf(tab, "Chassis Handle", NAFLG_FMT_NUMERIC, "%u", pBoard->ChassisHandle);
	if (pBoard->Header.Length < 0x0e)
		return;
	NWL_NodeAttrSet(tab, "Board Type", pBoardTypeToStr(pBoard->Type), 0);
}

static const CHAR*
pSystemEnclosureTypeToStr(UCHAR Type)
{
	switch (Type & 0x7f)
	{
	case 0x01: return "Other";
	case 0x03: return "Desktop";
	case 0x04: return "Low Profile Desktop";
	case 0x05: return "Pizza Box";
	case 0x06: return "Mini Tower";
	case 0x07: return "Tower";
	case 0x08: return "Portable";
	case 0x09: return "Laptop";
	case 0x0a: return "Notebook";
	case 0x0b: return "Hand Held";
	case 0x0c: return "Docking Station";
	case 0x0d: return "All in One";
	case 0x0e: return "Sub Notebook";
	case 0x0f: return "Space-saving";
	case 0x10: return "Lunch Box";
	case 0x11: return "Main Server Chassis";
	case 0x12: return "Expansion Chassis";
	case 0x13: return "SubChassis";
	case 0x14: return "Bus Expansion Chassis";
	case 0x15: return "Peripheral Chassis";
	case 0x16: return "RAID Chassis";
	case 0x17: return "Rack Mount Chassis";
	case 0x18: return "Sealed-case PC";
	case 0x19: return "Multi-system chassis";
	case 0x1a: return "Compact PCI";
	case 0x1b: return "Advanced TCA";
	case 0x1c: return "Blade";
	case 0x1d: return "Blade Enclosure";
	case 0x1e: return "Tablet";
	case 0x1f: return "Convertible";
	case 0x20: return "Detachable";
	case 0x21: return "IoT Gateway";
	case 0x22: return "Embedded PC";
	case 0x23: return "Mini PC";
	case 0x24: return "Stick PC";
	}
	return "Unknown";
}

static const CHAR*
pBootUpStateToStr(UCHAR State)
{
	switch (State)
	{
	case 0x01: return "Other";
	case 0x03: return "Safe";
	case 0x04: return "Warning";
	case 0x05: return "Critical";
	case 0x06: return "Non-recoverable";
	}
	return "Unknown";
}

static const CHAR*
pSecurityStatusToStr(UCHAR State)
{
	switch (State)
	{
	case 0x01: return "Other";
	case 0x03: return "None";
	case 0x04: return "External interface locked out";
	case 0x05: return "External interface enabled";
	}
	return "Unknown";
}

static void ProcSystemEnclosure(PNODE tab, void* p)
{
	PSystemEnclosure pSysEnclosure = (PSystemEnclosure)p;
	NWL_NodeAttrSet(tab, "Description", "System Enclosure Information", 0);
	if (pSysEnclosure->Header.Length < 0x09) // 2.0
		return;
	NWL_NodeAttrSetf(tab, "Type", 0, "%s%s",
		(pSysEnclosure->Type & 0x80) ? "[LOCK]" : "",
		pSystemEnclosureTypeToStr(pSysEnclosure->Type));
	NWL_NodeAttrSet(tab, "Manufacturer", LocateString(p, pSysEnclosure->Manufacturer), 0);
	NWL_NodeAttrSet(tab, "Version", LocateString(p, pSysEnclosure->Version), 0);
	NWL_NodeAttrSet(tab, "Serial Number", LocateString(p, pSysEnclosure->SN), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(tab, "Asset Tag", LocateString(p, pSysEnclosure->AssetTag), 0);
	if (pSysEnclosure->Header.Length < 0x0d) // 2.1
		return;
	NWL_NodeAttrSet(tab, "Boot-up State", pBootUpStateToStr(pSysEnclosure->BootupState), 0);
	NWL_NodeAttrSet(tab, "Power Supply State", pBootUpStateToStr(pSysEnclosure->PowerSupplyState), 0);
	NWL_NodeAttrSet(tab, "Thermal State", pBootUpStateToStr(pSysEnclosure->ThermalState), 0);
	NWL_NodeAttrSet(tab, "Security Status", pSecurityStatusToStr(pSysEnclosure->SecurityStatus), 0);
	if (pSysEnclosure->Header.Length < 0x15) // 2.3
		return;
	NWL_NodeAttrSetf(tab, "OEM-defined", NAFLG_FMT_NUMERIC, "%u", pSysEnclosure->OEMDefine);
}

static const CHAR*
pProcessorTypeToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x01: return "Other";
	case 0x03: return "Central Processor";
	case 0x04: return "Math Processor";
	case 0x05: return "DSP Processor";
	case 0x06: return "Video Processor";
	}
	return "Unknown";
}

static const CHAR*
pProcessorFamilyToStr(UCHAR Family, LPCSTR lpManufacturer)
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
	case 0x16: return "Intel processor";

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

	case 0x70: return "Hobbit Family";

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
	case 0xBE:
		// reserved.
		if (lpManufacturer[0] == 'A')
			return "AMD K7";
		// fall through
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
	NWL_NodeAttrSet(tab, "Description", "Processor Information", 0);
	if (pProcessor->Header.Length < 0x1a) // 2.0
		return;
	LPCSTR lpManufacturer = LocateString(p, pProcessor->Manufacturer);
	NWL_NodeAttrSet(tab, "Socket Designation", LocateString(p, pProcessor->SocketDesignation), 0);
	NWL_NodeAttrSet(tab, "Type", pProcessorTypeToStr(pProcessor->Type), 0);
	NWL_NodeAttrSet(tab, "Processor Family", pProcessorFamilyToStr(pProcessor->Family, lpManufacturer), 0);
	NWL_NodeAttrSet(tab, "Processor Manufacturer", lpManufacturer, 0);
	NWL_NodeAttrSet(tab, "Processor Version", LocateString(p, pProcessor->Version), 0);
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
	if (pProcessor->Header.Length < 0x20) // 2.1
		return;
	if (pProcessor->Header.Length < 0x23) // 2.3
		return;
	NWL_NodeAttrSet(tab, "Serial Number", LocateString(p, pProcessor->Serial), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(tab, "Asset Tag", LocateString(p, pProcessor->AssetTag), 0);
	NWL_NodeAttrSet(tab, "Part Number", LocateString(p, pProcessor->PartNum), 0);
	if (pProcessor->Header.Length < 0x28) // 2.5
		return;
	if (pProcessor->CoreCount == 0xff && pProcessor->Header.Length > 0x2a)
		NWL_NodeAttrSetf(tab, "Core Count", NAFLG_FMT_NUMERIC, "%u", pProcessor->CoreCount2);
	else
		NWL_NodeAttrSetf(tab, "Core Count", NAFLG_FMT_NUMERIC, "%u", pProcessor->CoreCount);
	if (pProcessor->CoreEnabled == 0xff && pProcessor->Header.Length > 0x2c)
		NWL_NodeAttrSetf(tab, "Core Enabled", NAFLG_FMT_NUMERIC, "%u", pProcessor->CoreEnabled2);
	else
		NWL_NodeAttrSetf(tab, "Core Enabled", NAFLG_FMT_NUMERIC, "%u", pProcessor->CoreEnabled);
	if (pProcessor->ThreadCount == 0xff && pProcessor->Header.Length > 0x2a)
		NWL_NodeAttrSetf(tab, "Thread Count", NAFLG_FMT_NUMERIC, "%u", pProcessor->ThreadCount2);
	else
		NWL_NodeAttrSetf(tab, "Thread Count", NAFLG_FMT_NUMERIC, "%u", pProcessor->ThreadCount);
	NWL_NodeAttrSetf(tab, "Processor Characteristics", 0, "0x%04X", pProcessor->ProcessorChar);
}

static void ProcMemCtrlInfo(PNODE tab, void* p)
{
	PMemCtrlInfo pMemCtrl = (PMemCtrlInfo)p;
	NWL_NodeAttrSet(tab, "Description", "Memory Controller Information", 0);
	if (pMemCtrl->Header.Length < 0x15) // 2.0
		return;
	NWL_NodeAttrSetf(tab, "Max Memory Module Size (MB)", NAFLG_FMT_NUMERIC, "%llu", 2ULL << pMemCtrl->MaxMemModuleSize);
	NWL_NodeAttrSetf(tab, "Number of Slots", NAFLG_FMT_NUMERIC, "%u", pMemCtrl->NumOfSlots);
}

static void ProcMemModuleInfo(PNODE tab, void* p)
{
	PMemModuleInfo	pMemModule = (PMemModuleInfo)p;
	UCHAR sz = 0;
	NWL_NodeAttrSet(tab, "Description", "Memory Module Information", 0);
	if (pMemModule->Header.Length < 0x0c)
		return;
	NWL_NodeAttrSet(tab, "Socket Designation", LocateString(p, pMemModule->SocketDesignation), 0);
	NWL_NodeAttrSetf(tab, "Current Speed (ns)", NAFLG_FMT_NUMERIC, "%u", pMemModule->CurrentSpeed);
	sz = pMemModule->InstalledSize & 0x7F;
	if (sz > 0x7D)
		sz = 0;
	NWL_NodeAttrSetf(tab, "Installed Size (MB)", NAFLG_FMT_NUMERIC, "%llu", 2ULL << sz);
}

static const CHAR*
pCacheConfToOpMode(UINT16 Conf)
{
	UINT16 OpMode = (Conf & 0x300) >> 8;
	switch (OpMode)
	{
	case 0x00: return "Write Through";
	case 0x01: return "Write Back";
	case 0x02: return "Varies with Memory Address";
	}
	return "Unknown";
}

static const CHAR*
pCacheConfToLocation(UINT16 Conf)
{
	UINT16 OpMode = (Conf & 0x60) >> 5;
	switch (OpMode)
	{
	case 0x00: return "Internal";
	case 0x01: return "External";
	case 0x02: return "Reserved";
	}
	return "Unknown";
}

static void
pCacheSetSRAMType(PNODE tab, LPCSTR key, UINT16 value)
{
	char* str = NULL;
	if (value & (1 << 2))
		NWL_NodeAppendMultiSz(&str, "Non-Burst");
	if (value & (1 << 3))
		NWL_NodeAppendMultiSz(&str, "Burst");
	if (value & (1 << 4))
		NWL_NodeAppendMultiSz(&str, "Pipeline Burst");
	if (value & (1 << 5))
		NWL_NodeAppendMultiSz(&str, "Synchronous");
	if (value & (1 << 6))
		NWL_NodeAppendMultiSz(&str, "Asynchronous");
	if (str == NULL)
		NWL_NodeAppendMultiSz(&str, "Unknown");
	NWL_NodeAttrSetMulti(tab, key, str, 0);
	free(str);
}

static const CHAR*
pCacheECTypeToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x01: return "Other";
	case 0x03: return "None";
	case 0x04: return "Parity";
	case 0x05: return "Single-bit ECC";
	case 0x06: return "Multi-bit ECC";
	}
	return "Unknown";
}

static const CHAR*
pCacheTypeToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x01: return "Other";
	case 0x03: return "Instruction";
	case 0x04: return "Data";
	case 0x05: return "Unified";
	}
	return "Unknown";
}

static const CHAR*
pCacheAssocToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x01: return "Other";
	case 0x03: return "Direct Mapped";
	case 0x04: return "2-way Set-Associative";
	case 0x05: return "4-way Set-Associative";
	case 0x06: return "Fully Associative";
	case 0x07: return "8-way Set-Associative";
	case 0x08: return "16-way Set-Associative";
	case 0x09: return "12-way Set-Associative";
	case 0x0a: return "24-way Set-Associative";
	case 0x0b: return "32-way Set-Associative";
	case 0x0c: return "48-way Set-Associative";
	case 0x0d: return "64-way Set-Associative";
	case 0x0e: return "20-way Set-Associative";
	}
	return "Unknown";
}

static void ProcCacheInfo(PNODE tab, void* p)
{
	PCacheInfo	pCache = (PCacheInfo)p;
	UINT64 sz = 0;
	NWL_NodeAttrSet(tab, "Description", "Cache Information", 0);
	if (pCache->Header.Length < 0x0f) // 2.0
		return;
	NWL_NodeAttrSet(tab, "Socket Designation", LocateString(p, pCache->SocketDesignation), 0);
	NWL_NodeAttrSetf(tab, "Cache Configuration", 0, "0x%04X", pCache->Configuration);
	NWL_NodeAttrSet(tab, "Operational Mode", pCacheConfToOpMode(pCache->Configuration), 0);
	NWL_NodeAttrSetBool(tab, "Enabled", pCache->Configuration & 0x80, 0);
	NWL_NodeAttrSet(tab, "Location", pCacheConfToLocation(pCache->Configuration), 0);
	NWL_NodeAttrSetf(tab, "Cache Level", 0, "L%u", 1U + (pCache->Configuration & 0x07));
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
	NWL_NodeAttrSet(tab, "Max Cache Size", NWL_GetHumanSize(sz, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
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
	NWL_NodeAttrSet(tab, "Installed Cache Size", NWL_GetHumanSize(sz, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
	pCacheSetSRAMType(tab, "Supported SRAM Type", pCache->SupportSRAMType);
	pCacheSetSRAMType(tab, "Current SRAM Type", pCache->SupportSRAMType);
	if (pCache->Header.Length < 0x13) // 2.1
		return;
	if (pCache->Speed)
		NWL_NodeAttrSetf(tab, "Cache Speed (ns)", NAFLG_FMT_NUMERIC, "%u", pCache->Speed);
	NWL_NodeAttrSet(tab, "Error Correction Type", pCacheECTypeToStr(pCache->ErrorCorrectionType), 0);
	NWL_NodeAttrSet(tab, "System Cache Type", pCacheTypeToStr(pCache->SystemCacheType), 0);
	NWL_NodeAttrSet(tab, "Associativity", pCacheAssocToStr(pCache->Associativity), 0);
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
	NWL_NodeAttrSet(tab, "Description", "Port Connector Information", 0);
	if (pPort->Header.Length < 0x09)
		return;
	NWL_NodeAttrSet(tab, "Internal Reference Designator", LocateString(p, pPort->IntDesignator), 0);
	NWL_NodeAttrSet(tab, "Internal Connector Type", pIntConnectTypeToStr(pPort->IntConnectorType), 0);
	NWL_NodeAttrSet(tab, "External Reference Designator", LocateString(p, pPort->ExtDesignator), 0);
	NWL_NodeAttrSet(tab, "External Connector Type", pIntConnectTypeToStr(pPort->ExtConnectorType), 0);
	NWL_NodeAttrSet(tab, "Port Type", pPortTypeToStr(pPort->PortType), 0);
}

static const CHAR*
pSysSlotTypeToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x01: return "Other";
	case 0x03: return "ISA";
	case 0x04: return "MCA";
	case 0x05: return "EISA";
	case 0x06: return "PCI";
	case 0x07: return "PC Card (PCMCIA)";
	case 0x08: return "VL-VESA";
	case 0x09: return "Proprietary";
	case 0x0a: return "Processor Card Slot";
	case 0x0b: return "Proprietary Memory Card Slot";
	case 0x0c: return "I/O Riser Card Slot";
	case 0x0d: return "NuBus";
	case 0x0e: return "PCI ¨C 66MHz Capable";
	case 0x0f: return "AGP";
	case 0x10: return "AGP 2X";
	case 0x11: return "AGP 4X";
	case 0x12: return "PCI-X";
	case 0x13: return "AGP 8X";
	case 0x14: return "M.2 Socket 1-DP (Mechanical Key A)";
	case 0x15: return "M.2 Socket 1-SD (Mechanical Key E)";
	case 0x16: return "M.2 Socket 2 (Mechanical Key B)";
	case 0x17: return "M.2 Socket 3 (Mechanical Key M)";
	case 0x18: return "MXM Type I";
	case 0x19: return "MXM Type II";
	case 0x1a: return "MXM Type III (standard connector)";
	case 0x1b: return "MXM Type III (HE connector)";
	case 0x1c: return "MXM Type IV";
	case 0x1d: return "MXM 3.0 Type A";
	case 0x1e: return "MXM 3.0 Type B";
	case 0x1f: return "PCI Express Gen 2 SFF-8639 (U.2)";
	case 0x20: return "PCI Express Gen 3 SFF-8639 (U.2)";
	case 0x21: return "PCI Express Mini 52-pin (CEM spec. 2.0) with bottom-side keep-outs";
	case 0x22: return "PCI Express Mini 52-pin (CEM spec. 2.0) without bottom-side keep-outs";
	case 0x23: return "PCI Express Mini 76-pin (CEM spec. 2.0)";
	case 0x24: return "PCI Express Gen 4 SFF-8639 (U.2)";
	case 0x25: return "PCI Express Gen 5 SFF-8639 (U.2)";
	case 0x26: return "OCP NIC 3.0 Small Form Factor (SFF)";
	case 0x27: return "OCP NIC 3.0 Large Form Factor (LFF)";
	case 0x28: return "OCP NIC Prior to 3.0";

	case 0x30: return "CXL Flexbus 1.0";

	case 0xa0: return "PC-98/C20";
	case 0xa1: return "PC-98/C24";
	case 0xa2: return "PC-98/E";
	case 0xa3: return "PC-98/Local Bus";
	case 0xa4: return "PC-98/Card";
	case 0xa5: return "PCI Express";
	case 0xa6: return "PCI Express x1";
	case 0xa7: return "PCI Express x2";
	case 0xa8: return "PCI Express x4";
	case 0xa9: return "PCI Express x8";
	case 0xaa: return "PCI Express x16";
	case 0xab: return "PCI Express Gen 2";
	case 0xac: return "PCI Express Gen 2 x1";
	case 0xad: return "PCI Express Gen 2 x2";
	case 0xae: return "PCI Express Gen 2 x4";
	case 0xaf: return "PCI Express Gen 2 x8";
	case 0xb0: return "PCI Express Gen 2 x16";
	case 0xb1: return "PCI Express Gen 3";
	case 0xb2: return "PCI Express Gen 3 x1";
	case 0xb3: return "PCI Express Gen 3 x2";
	case 0xb4: return "PCI Express Gen 3 x4";
	case 0xb5: return "PCI Express Gen 3 x8";
	case 0xb6: return "PCI Express Gen 3 x16";

	case 0xb8: return "PCI Express Gen 4";
	case 0xb9: return "PCI Express Gen 4 x1";
	case 0xba: return "PCI Express Gen 4 x2";
	case 0xbb: return "PCI Express Gen 4 x4";
	case 0xbc: return "PCI Express Gen 4 x8";
	case 0xbd: return "PCI Express Gen 4 x16";
	case 0xbe: return "PCI Express Gen 5";
	case 0xbf: return "PCI Express Gen 5 x1";
	case 0xc0: return "PCI Express Gen 5 x2";
	case 0xc1: return "PCI Express Gen 5 x4";
	case 0xc2: return "PCI Express Gen 5 x8";
	case 0xc3: return "PCI Express Gen 5 x16";
	case 0xc4: return "PCI Express Gen 6 and Beyond";
	case 0xc5: return "EDSFF E1.S, E1.L";
	case 0xc6: return "EDSFF E3.S, E3.L";
	}
	return "Unknown";
}

static const CHAR*
pSysSlotBusWidthToStr(UCHAR Width)
{
	switch (Width)
	{
	case 0x01: return "Other";
	case 0x03: return "8 bit";
	case 0x04: return "16 bit";
	case 0x05: return "32 bit";
	case 0x06: return "64 bit";
	case 0x07: return "128 bit";
	case 0x08: return "x1";
	case 0x09: return "x2";
	case 0x0a: return "x4";
	case 0x0b: return "x8";
	case 0x0c: return "x12";
	case 0x0d: return "x16";
	case 0x0e: return "x32";
	}
	return "Unknown";
}

static const CHAR*
pSysSlotUsageToStr(UCHAR Usage)
{
	switch (Usage)
	{
	case 0x01: return "Other";
	case 0x03: return "Available";
	case 0x04: return "In use";
	case 0x05: return "Unavailable";
	}
	return "Unknown";
}

static const CHAR*
pSysSlotLengthToStr(UCHAR Length)
{
	switch (Length)
	{
	case 0x01: return "Other";
	case 0x03: return "Short Length";
	case 0x04: return "Long Length";
	case 0x05: return "2.5 drive form factor";
	case 0x06: return "3.5 drive form factor";
	}
	return "Unknown";
}

static void ProcPSystemSlots(PNODE tab, void* p)
{
	PSystemSlots pSys = (PSystemSlots)p;
	NWL_NodeAttrSet(tab, "Description", "System Slots", 0);
	if (pSys->Header.Length < 0x0c) // 2.0
		return;
	NWL_NodeAttrSet(tab, "Slot Designation", LocateString(p, pSys->SlotDesignation), 0);
	NWL_NodeAttrSet(tab, "Slot Type", pSysSlotTypeToStr(pSys->SlotType), 0);
	NWL_NodeAttrSet(tab, "Slot Data Bus Width", pSysSlotBusWidthToStr(pSys->SlotDataBusWidth), 0);
	NWL_NodeAttrSet(tab, "Current Usage", pSysSlotUsageToStr(pSys->CurrentUsage), 0);
	NWL_NodeAttrSet(tab, "Slot Length", pSysSlotLengthToStr(pSys->SlotLength), 0);
	NWL_NodeAttrSetf(tab, "Slot ID", NAFLG_FMT_NUMERIC, "%u", pSys->SlotID);
	NWL_NodeAttrSetf(tab, "Slot Characteristics 1", 0, "0x%02X", pSys->SlotCharacteristics1);
	if (pSys->Header.Length < 0x0d) // 2.1
		return;
	NWL_NodeAttrSetf(tab, "Slot Characteristics 2", 0, "0x%02X", pSys->SlotCharacteristics2);
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
	NWL_NodeAttrSet(tab, "Description", "On Board Devices Information", 0);
	if (pDev->Header.Length < 0x04)
		return;
	count = (pDev->Header.Length - sizeof(SMBIOSHEADER)) / (sizeof(pDev->DeviceInfo[0]));
	NWL_NodeAttrSetf(tab, "Number of Devices", NAFLG_FMT_NUMERIC, "%u", count);
	ndev = NWL_NodeAppendNew(tab, "On Board Devices", NFLG_TABLE);
	for (i = 0; i < count; i++)
	{
		PNODE pd = NWL_NodeAppendNew(ndev, "Device", NFLG_TABLE_ROW);
		UCHAR type = pDev->DeviceInfo[i].DeviceType & 0x7f;
		UCHAR status = pDev->DeviceInfo[i].DeviceType & 0x90;
		NWL_NodeAttrSet(pd, "Type", pOnBoardDeviceTypeToStr(type), 0);
		NWL_NodeAttrSet(pd, "Status", status ? "Enabled" : "Disabled", 0);
		NWL_NodeAttrSet(pd, "Description", LocateString(p, pDev->DeviceInfo[i].Description), 0);
	}
}

static void ProcOEMString(PNODE tab, void* p)
{
	UCHAR i;
	char* oem = NULL;
	POEMString pString = (POEMString)p;
	NWL_NodeAttrSet(tab, "Description", "OEM String", 0);
	if (pString->Header.Length < 0x05)
		return;
	NWL_NodeAttrSetf(tab, "Number of Strings", NAFLG_FMT_NUMERIC, "%u", pString->Count);
	for (i = 1; i <= pString->Count; i++)
		NWL_NodeAppendMultiSz(&oem, LocateString(p, i));
	NWL_NodeAttrSetMulti(tab, "OEM Strings", oem, 0);
	free(oem);
}

static void ProcSysConfOptions(PNODE tab, void* p)
{
	UCHAR i;
	POEMString pString = (POEMString)p;
	char* conf = NULL;
	NWL_NodeAttrSet(tab, "Description", "System Configuration Options", 0);
	if (pString->Header.Length < 0x05)
		return;
	NWL_NodeAttrSetf(tab, "Number of Strings", NAFLG_FMT_NUMERIC, "%u", pString->Count);
	for (i = 1; i <= pString->Count; i++)
		NWL_NodeAppendMultiSz(&conf, LocateString(p, i));
	NWL_NodeAttrSetMulti(tab, "Configuration Strings", conf, 0);
	free(conf);
}

static void ProcBIOSLangInfo(PNODE tab, void* p)
{
	PBIOSLangInfo pLang = (PBIOSLangInfo)p;
	NWL_NodeAttrSet(tab, "Description", "BIOS Language Information", 0);
	if (pLang->Header.Length < 0x16)
		return;
	NWL_NodeAttrSetf(tab, "Installable Languages", NAFLG_FMT_NUMERIC, "%u", pLang->InstallableLang);
	NWL_NodeAttrSet(tab, "Current Language", LocateString(p, pLang->CurrentLang), 0);
}

static void ProcGroupAssoc(PNODE tab, void* p)
{
	UINT i, count;
	PNODE ndev;
	PGroupAssoc pGA = (PGroupAssoc)p;
	NWL_NodeAttrSet(tab, "Description", "Group Associations", 0);
	if (pGA->Header.Length < 0x05)
		return;
	NWL_NodeAttrSet(tab, "Group Name", LocateString(p, pGA->GroupName), 0);
	count = (pGA->Header.Length - sizeof(pGA->GroupName) - sizeof(SMBIOSHEADER)) / (sizeof(pGA->GAItem[0]));
	NWL_NodeAttrSetf(tab, "Number of Items", NAFLG_FMT_NUMERIC, "%u", count);
	ndev = NWL_NodeAppendNew(tab, "Items", NFLG_TABLE);
	for (i = 0; i < count; i++)
	{
		PNODE p = NWL_NodeAppendNew(ndev, "Item", NFLG_TABLE_ROW);
		NWL_NodeAttrSetf(p, "Type", NAFLG_FMT_NUMERIC, "%u", pGA->GAItem[i].ItemType);
		NWL_NodeAttrSetf(p, "Handle", NAFLG_FMT_NUMERIC, "%u", pGA->GAItem[i].ItemHandle);
	}
}

static void ProcSystemEventLog(PNODE tab, void* p)
{
	PSystemEventLog pSys = (PSystemEventLog)p;
	NWL_NodeAttrSet(tab, "Description", "System Event Log", 0);
	if (pSys->Header.Length < 0x14) // 2.0
		return;
	NWL_NodeAttrSetf(tab, "Log Area Length", NAFLG_FMT_NUMERIC, "%u", pSys->LogAreaLength);
	NWL_NodeAttrSetf(tab, "Log Header Start Offset", NAFLG_FMT_NUMERIC, "%u", pSys->LogHdrStartOffset);
	NWL_NodeAttrSetf(tab, "Log Data Start Offset", NAFLG_FMT_NUMERIC, "%u", pSys->LogDataStartOffset);
	NWL_NodeAttrSetf(tab, "Access Method", 0, "0x%02X", pSys->AccessMethod);
	NWL_NodeAttrSetf(tab, "Log Status", 0, "0x%02X", pSys->LogStatus);
	NWL_NodeAttrSetf(tab, "Log Change Token", 0, "0x%08lX", pSys->LogChangeToken);
	NWL_NodeAttrSetf(tab, "Access Method Address", 0, "0x%08lX", pSys->AccessMethodAddr);
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
	if (pMA->Header.Length < 0x0f) // 2.1
		return;
	NWL_NodeAttrSet(tab, "Location", pMALocationToStr(pMA->Location), 0);
	NWL_NodeAttrSet(tab, "Function", pMAUseToStr(pMA->Use), 0);
	NWL_NodeAttrSet(tab, "Error Correction", pMAEccToStr(pMA->ErrCorrection), 0);
	if (pMA->MaxCapacity == 0x80000000 && pMA->Header.Length > 0x0f)
		sz = pMA->ExtMaxCapacity;
	else
		sz = ((UINT64)pMA->MaxCapacity) * 1024;
	NWL_NodeAttrSet(tab, "Max Capacity", NWL_GetHumanSize(sz, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
	NWL_NodeAttrSetf(tab, "Number of Slots", NAFLG_FMT_NUMERIC, "%u", pMA->NumOfMDs);
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
	case 0x20: return "HBM";
	case 0x21: return "HBM2";
	case 0x22: return "DDR5";
	case 0x23: return "LPDDR5";
	case 0x24: return "HBM3";
	}
	return "Unknown";
}

static const CHAR*
pMDFormFactorToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x01: return "Other";
	case 0x03: return "SIMM";
	case 0x04: return "SIP";
	case 0x05: return "Chip";
	case 0x06: return "DIP";
	case 0x07: return "ZIP";
	case 0x08: return "Proprietary Card";
	case 0x09: return "DIMM";
	case 0x0a: return "TSOP";
	case 0x0b: return "Row of chips";
	case 0x0c: return "RIMM";
	case 0x0d: return "SODIMM";
	case 0x0e: return "SRIMM";
	case 0x0f: return "FB-DIMM";
	case 0x10: return "Die";
	}
	return "Unknown";
}

static void
JedecIdToVendor(PNODE node, PMemoryDevice md)
{
	UINT id;
	DWORD sz = 0;
	CHAR* db = NULL;
	LPCSTR vendor = NULL;

	db = NWL_LoadIdsToMemory(L"jep106.ids", &sz);
	if (!db)
		goto fail;

	if (md->Header.Length >= 0x54) // 3.2
		id = (md->ModuleVID & 0x7F) << 8 | (md->ModuleVID >> 8) & 0x7F;
	else
	{
		vendor = LocateString((void*)md, md->Manufacturer);
		if (!isxdigit(vendor[0]) || !isxdigit(vendor[1])
			|| !isxdigit(vendor[2]) || !isxdigit(vendor[3])
			|| vendor[4] != '\0')
			goto fail;
		id = strtoul(vendor, NULL, 16);
	}
	NWL_GetSpdManufacturer(node, db, sz, (id >> 8) & 0x7F, id & 0x7F);
	free(db);
	return;
fail:
	if (db)
		free(db);
	vendor = LocateString((void*)md, md->Manufacturer);
	NWL_NodeAttrSet(node, "Manufacturer", vendor, 0);
}

static void ProcMemoryDevice(PNODE tab, void* p)
{
	PMemoryDevice pMD = (PMemoryDevice)p;
	UINT64 sz = 0;
	NWL_NodeAttrSet(tab, "Description", "Memory Device", 0);
	if (pMD->Header.Length < 0x15) // 2.1
		return;
	NWL_NodeAttrSet(tab, "Device Locator", LocateString(p, pMD->DeviceLocator), 0);
	NWL_NodeAttrSet(tab, "Bank Locator", LocateString(p, pMD->BankLocator), 0);
	NWL_NodeAttrSet(tab, "Form Factor", pMDFormFactorToStr(pMD->FormFactor), 0);
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
	NWL_NodeAttrSet(tab, "Device Size", NWL_GetHumanSize(sz, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
	NWL_NodeAttrSet(tab, "Device Type", pMDMemoryTypeToStr(pMD->MemoryType), 0);
	if (pMD->Header.Length < 0x1b) // 2.3
		return;
	if (pMD->Speed)
		NWL_NodeAttrSetf(tab, "Speed (MT/s)", NAFLG_FMT_NUMERIC, "%u", pMD->Speed);
	JedecIdToVendor(tab, pMD);
	NWL_NodeAttrSet(tab, "Serial Number", LocateString(p, pMD->SN), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(tab, "Asset Tag Number", LocateString(p, pMD->AssetTag), 0);
	NWL_NodeAttrSet(tab, "Part Number", LocateString(p, pMD->PN), 0);
}

static const CHAR*
pMemErrTypeToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x01: return "Other";
	case 0x03: return "OK";
	case 0x04: return "Bad Read";
	case 0x05: return "Parity Error";
	case 0x06: return "Single-bit Error";
	case 0x07: return "Double-bit Error";
	case 0x08: return "Multi-bit Error";
	case 0x09: return "Nibble Error";
	case 0x0a: return "Checksum Error";
	case 0x0b: return "CRC Error";
	case 0x0c: return "Corrected Single-bit Error";
	case 0x0d: return "Corrected Error";
	case 0x0e: return "Uncorrectable Error";
	}
	return "Unknown";
}

static const CHAR*
pMemErrGranularityToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x01: return "Other";
	case 0x03: return "Device level";
	case 0x04: return "Memory partition level";
	}
	return "Unknown";
}

static const CHAR*
pMemErrOperationToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x01: return "Other";
	case 0x03: return "Read";
	case 0x04: return "Write";
	case 0x05: return "Partial Write";
	}
	return "Unknown";
}

static void ProcMemoryErrInfo(PNODE tab, void* p)
{
	PMemoryErrInfo pMemErrInfo = (PMemoryErrInfo)p;
	NWL_NodeAttrSet(tab, "Description", "32-Bit Memory Error Information", 0);
	if (pMemErrInfo->Header.Length < 0x17) // 2.1
		return;
	NWL_NodeAttrSet(tab, "Error Type", pMemErrTypeToStr(pMemErrInfo->ErrType), 0);
	NWL_NodeAttrSet(tab, "Error Granularity", pMemErrGranularityToStr(pMemErrInfo->ErrGranularity), 0);
	NWL_NodeAttrSet(tab, "Error Operation", pMemErrOperationToStr(pMemErrInfo->ErrOperation), 0);
	NWL_NodeAttrSetf(tab, "Vendor Syndrome", 0, "0x%08lX", pMemErrInfo->VendorSyndrome);
	NWL_NodeAttrSetf(tab, "Memory Array Error Address", 0, "0x%08lX", pMemErrInfo->MemArrayErrAddr);
	NWL_NodeAttrSetf(tab, "Device Error Address", 0, "0x%08lX", pMemErrInfo->DevErrAddr);
	NWL_NodeAttrSetf(tab, "Error Resolution", 0, "0x%08lX", pMemErrInfo->ErrResolution);
}

static void ProcMemoryArrayMappedAddress(PNODE tab, void* p)
{
	PMemoryArrayMappedAddress pMAMA = (PMemoryArrayMappedAddress)p;
	NWL_NodeAttrSet(tab, "Description", "Memory Array Mapped Address", 0);
	if (pMAMA->Header.Length < 0x0f) // 2.1
		return;
	NWL_NodeAttrSetf(tab, "Starting Address", 0, "0x%016llX",
		(pMAMA->Header.Length >= 0x1f && pMAMA->StartAddr == 0xFFFFFFFF) ?
		pMAMA->ExtStartAddr : pMAMA->StartAddr);
	NWL_NodeAttrSetf(tab, "Ending Address", 0, "0x%016llX",
		(pMAMA->Header.Length >= 0x1f && pMAMA->EndAddr == 0xFFFFFFFF) ?
		pMAMA->ExtEndAddr : pMAMA->EndAddr);
	NWL_NodeAttrSetf(tab, "Memory Array Handle", NAFLG_FMT_NUMERIC, "%u", pMAMA->Handle);
	NWL_NodeAttrSetf(tab, "Partition Width", 0, "0x%X", pMAMA->PartitionWidth);
}

static void ProcMemoryDeviceMappedAddress(PNODE tab, void* p)
{
	PMemoryDeviceMappedAddress pMDMA = (PMemoryDeviceMappedAddress)p;
	NWL_NodeAttrSet(tab, "Description", "Memory Device Mapped Address", 0);
	if (pMDMA->Header.Length < 0x13) // 2.1
		return;
	NWL_NodeAttrSetf(tab, "Starting Address", 0, "0x%016llX",
		(pMDMA->Header.Length >= 0x23 && pMDMA->StartAddr == 0xFFFFFFFF) ?
		pMDMA->ExtStartAddr : pMDMA->StartAddr);
	NWL_NodeAttrSetf(tab, "Ending Address", 0, "0x%016llX",
		(pMDMA->Header.Length >= 0x23 && pMDMA->EndAddr == 0xFFFFFFFF) ?
		pMDMA->ExtEndAddr : pMDMA->EndAddr);
	NWL_NodeAttrSetf(tab, "Memory Device Handle", NAFLG_FMT_NUMERIC, "%u", pMDMA->MDHandle);
	NWL_NodeAttrSetf(tab, "Memory Array Mapped Address Handle", NAFLG_FMT_NUMERIC, "%u", pMDMA->MAMAHandle);
}

static const CHAR*
pPointingDevTypeToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x01: return "Other";
	case 0x03: return "Mouse";
	case 0x04: return "Track Ball";
	case 0x05: return "Track Point";
	case 0x06: return "Glide Point";
	case 0x07: return "Touch Pad";
	case 0x08: return "Touch Screen";
	case 0x09: return "Optical Sensor";
	}
	return "Unknown";
}

static const CHAR*
pPointingDevInterfaceToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x01: return "Other";
	case 0x03: return "Serial";
	case 0x04: return "PS/2";
	case 0x05: return "Infrared";
	case 0x06: return "HP-HIL";
	case 0x07: return "Bus mouse";
	case 0x08: return "ADB";
	case 0xa0: return "Bus mouse DB-9";
	case 0xa1: return "Bus mouse micro-DIN";
	case 0xa2: return "USB";
	case 0xa3: return "I2C";
	case 0xa4: return "SPI";
	}
	return "Unknown";
}

static void ProcBuiltinPointing(PNODE tab, void* p)
{
	PBuiltinPointing pBP = (PBuiltinPointing)p;
	NWL_NodeAttrSet(tab, "Description", "Built-in Pointing Device", 0);
	if (pBP->Header.Length < 0x07) // 2.1
		return;
	NWL_NodeAttrSet(tab, "Type", pPointingDevTypeToStr(pBP->Type), 0);
	NWL_NodeAttrSet(tab, "Interface", pPointingDevInterfaceToStr(pBP->Interface), 0);
	NWL_NodeAttrSetf(tab, "Number of Buttons", NAFLG_FMT_NUMERIC, "%u", pBP->NumOfButtons);
}

static void ProcPortableBattery(PNODE tab, void* p)
{
	PPortableBattery pPB = (PPortableBattery)p;
	NWL_NodeAttrSet(tab, "Description", "Portable Battery", 0);
	if (pPB->Header.Length < 0x1a) // 2.1
		return;
	NWL_NodeAttrSet(tab, "Location", LocateString(p, pPB->Location), 0);
	NWL_NodeAttrSet(tab, "Manufacturer", LocateString(p, pPB->Manufacturer), 0);
	NWL_NodeAttrSet(tab, "Manufacturer Date", LocateString(p, pPB->Date), 0);
	NWL_NodeAttrSet(tab, "Serial Number", LocateString(p, pPB->SN), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(tab, "Device Name", LocateString(p, pPB->DeviceName), 0);
}

static const CHAR*
pSysResetCapabilitiesToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x01: return "Operating System";
	case 0x02: return "System Utilities";
	case 0x03: return "Do Not Reboot";
	}
	return "Reserved";
}

static void ProcSysReset(PNODE tab, void* p)
{
	PSysReset pSysReset = (PSysReset)p;
	NWL_NodeAttrSet(tab, "Description", "System Reset", 0);
	if (pSysReset->Header.Length < 0x0d)
		return;
	NWL_NodeAttrSetBool(tab, "Watchdog Timer", pSysReset->Capabilities & (1 << 5), 0);
	NWL_NodeAttrSet(tab, "Boot Option on Limit",
		pSysResetCapabilitiesToStr((pSysReset->Capabilities & 0x18) >> 3), 0);
	NWL_NodeAttrSet(tab, "Boot Option",
		pSysResetCapabilitiesToStr((pSysReset->Capabilities & 0x06) >> 1), 0);
	NWL_NodeAttrSetBool(tab, "System Reset Status", pSysReset->Capabilities & 0x01, 0);
	NWL_NodeAttrSetf(tab, "Reset Count", NAFLG_FMT_NUMERIC, "%u", pSysReset->ResetCount);
	NWL_NodeAttrSetf(tab, "Reset Limit", NAFLG_FMT_NUMERIC, "%u", pSysReset->ResetLimit);
	NWL_NodeAttrSetf(tab, "Timer Interval", NAFLG_FMT_NUMERIC, "%u", pSysReset->TimerInterval);
	NWL_NodeAttrSetf(tab, "Timeout", NAFLG_FMT_NUMERIC, "%u", pSysReset->Timeout);
}

static const CHAR*
pHwSecurityStatusToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x00: return "Disabled";
	case 0x01: return "Enabled";
	case 0x02: return "Not Implemented";
	}
	return "Unknown";
}

static void ProcHwSecurity(PNODE tab, void* p)
{
	PHwSecurity pHwSecurity = (PHwSecurity)p;
	NWL_NodeAttrSet(tab, "Description", "Hardware Security", 0);
	if (pHwSecurity->Header.Length < 0x05)
		return;
	NWL_NodeAttrSet(tab, "Power-on Password",
		pHwSecurityStatusToStr((pHwSecurity->Settings & 0xc0) >> 6), 0);
	NWL_NodeAttrSet(tab, "Keyboard Password",
		pHwSecurityStatusToStr((pHwSecurity->Settings & 0x30) >> 4), 0);
	NWL_NodeAttrSet(tab, "Administrator Password",
		pHwSecurityStatusToStr((pHwSecurity->Settings & 0x0c) >> 2), 0);
	NWL_NodeAttrSet(tab, "Front Panel Reset",
		pHwSecurityStatusToStr(pHwSecurity->Settings & 0x03), 0);
}

static void ProcSysPowerCtrl(PNODE tab, void* p)
{
	PSysPowerCtrl pSysPowerCtrl = (PSysPowerCtrl)p;
	NWL_NodeAttrSet(tab, "Description", "System Power Controls", 0);
	if (pSysPowerCtrl->Header.Length < 0x09)
		return;
	NWL_NodeAttrSetf(tab, "Next Scheduled Power-on", 0,
		"%02X-%02X-%02X:%02X:%02X",
		pSysPowerCtrl->NextPwrOnMonth, pSysPowerCtrl->NextPwrOnDay,
		pSysPowerCtrl->NextPwrOnHour, pSysPowerCtrl->NextPwrOnMinute,
		pSysPowerCtrl->NextPwrOnSecond);
}

static void ProcVoltageProbe(PNODE tab, void* p)
{
	PVoltageProbe pVoltage = (PVoltageProbe)p;
	NWL_NodeAttrSet(tab, "Description", "Voltage Probe", 0);
	if (pVoltage->Header.Length < 0x14)
		return;
	NWL_NodeAttrSet(tab, "Description", LocateString(p, pVoltage->Description), 0);
	NWL_NodeAttrSetf(tab, "Location and Status", 0, "%02Xh", pVoltage->LocationStatus);
	NWL_NodeAttrSetf(tab, "Maximum Value", NAFLG_FMT_NUMERIC, "%u", pVoltage->MaxValue);
	NWL_NodeAttrSetf(tab, "Minimum Value", NAFLG_FMT_NUMERIC, "%u", pVoltage->MinValue);
	NWL_NodeAttrSetf(tab, "Resolution", NAFLG_FMT_NUMERIC, "%u", pVoltage->Resolution);
	NWL_NodeAttrSetf(tab, "Tolerance", NAFLG_FMT_NUMERIC, "%u", pVoltage->Tolerance);
	NWL_NodeAttrSetf(tab, "Accuracy", NAFLG_FMT_NUMERIC, "%u", pVoltage->Accuracy);
	NWL_NodeAttrSetf(tab, "OEM-defined", NAFLG_FMT_NUMERIC, "%lu", pVoltage->OEMDefined);
	NWL_NodeAttrSetf(tab, "Nominal Value", NAFLG_FMT_NUMERIC, "%u", pVoltage->NominalValue);
}

static void ProcCoolingDevice(PNODE tab, void* p)
{
	PCoolingDevice pCoolingDevice = (PCoolingDevice)p;
	NWL_NodeAttrSet(tab, "Description", "Cooling Device", 0);
	if (pCoolingDevice->Header.Length < 0x0c)
		return;
	NWL_NodeAttrSetf(tab, "Temperature Probe Handle", NAFLG_FMT_NUMERIC,
		"%u", pCoolingDevice->TempProbeHandle);
	NWL_NodeAttrSetf(tab, "Device Type and Status", 0,
		"%02Xh", pCoolingDevice->DeviceTypeStatus);
	NWL_NodeAttrSetf(tab, "Cooling Unit Group", NAFLG_FMT_NUMERIC,
		"%u", pCoolingDevice->CoolingUnitGroup);
	NWL_NodeAttrSetf(tab, "OEM-defined", NAFLG_FMT_NUMERIC, "%lu", pCoolingDevice->OEMDefined);
	NWL_NodeAttrSetf(tab, "Nominal Speed", NAFLG_FMT_NUMERIC, "%u", pCoolingDevice->NominalSpeed);
	NWL_NodeAttrSet(tab, "Description", LocateString(p, pCoolingDevice->Description), 0);
}

static void ProcTempProbe(PNODE tab, void* p)
{
	PTempProbe pTemp = (PTempProbe)p;
	NWL_NodeAttrSet(tab, "Description", "Temperature Probe", 0);
	if (pTemp->Header.Length < 0x14)
		return;
	NWL_NodeAttrSet(tab, "Description", LocateString(p, pTemp->Description), 0);
	NWL_NodeAttrSetf(tab, "Location and Status", 0, "%02Xh", pTemp->LocationStatus);
	NWL_NodeAttrSetf(tab, "Maximum Value", NAFLG_FMT_NUMERIC, "%u", pTemp->MaxValue);
	NWL_NodeAttrSetf(tab, "Minimum Value", NAFLG_FMT_NUMERIC, "%u", pTemp->MinValue);
	NWL_NodeAttrSetf(tab, "Resolution", NAFLG_FMT_NUMERIC, "%u", pTemp->Resolution);
	NWL_NodeAttrSetf(tab, "Tolerance", NAFLG_FMT_NUMERIC, "%u", pTemp->Tolerance);
	NWL_NodeAttrSetf(tab, "Accuracy", NAFLG_FMT_NUMERIC, "%u", pTemp->Accuracy);
	NWL_NodeAttrSetf(tab, "OEM-defined", NAFLG_FMT_NUMERIC, "%lu", pTemp->OEMDefined);
	NWL_NodeAttrSetf(tab, "Nominal Value", NAFLG_FMT_NUMERIC, "%u", pTemp->NominalValue);
}

static void ProcElecCurrentProbe(PNODE tab, void* p)
{
	PElecCurrentProbe pCurrent = (PElecCurrentProbe)p;
	NWL_NodeAttrSet(tab, "Description", "Electrical Current Probe", 0);
	if (pCurrent->Header.Length < 0x14)
		return;
	NWL_NodeAttrSet(tab, "Description", LocateString(p, pCurrent->Description), 0);
	NWL_NodeAttrSetf(tab, "Location and Status", 0, "%02Xh", pCurrent->LocationStatus);
	NWL_NodeAttrSetf(tab, "Maximum Value", NAFLG_FMT_NUMERIC, "%u", pCurrent->MaxValue);
	NWL_NodeAttrSetf(tab, "Minimum Value", NAFLG_FMT_NUMERIC, "%u", pCurrent->MinValue);
	NWL_NodeAttrSetf(tab, "Resolution", NAFLG_FMT_NUMERIC, "%u", pCurrent->Resolution);
	NWL_NodeAttrSetf(tab, "Tolerance", NAFLG_FMT_NUMERIC, "%u", pCurrent->Tolerance);
	NWL_NodeAttrSetf(tab, "Accuracy", NAFLG_FMT_NUMERIC, "%u", pCurrent->Accuracy);
	NWL_NodeAttrSetf(tab, "OEM-defined", NAFLG_FMT_NUMERIC, "%lu", pCurrent->OEMDefined);
	NWL_NodeAttrSetf(tab, "Nominal Value", NAFLG_FMT_NUMERIC, "%u", pCurrent->NominalValue);
}

static void ProcOutOfBandRemoteAccess(PNODE tab, void* p)
{
	POutOfBandRemoteAccess pRemoteAccess = (POutOfBandRemoteAccess)p;
	NWL_NodeAttrSet(tab, "Description", "Out-of-Band Remote Access", 0);
	if (pRemoteAccess->Header.Length < 0x06)
		return;
	NWL_NodeAttrSet(tab, "Manufacturer", LocateString(p, pRemoteAccess->Manufacturer), 0);
	NWL_NodeAttrSetBool(tab, "Outbound Connection Enabled", pRemoteAccess->Connections & (1 << 1), 0);
	NWL_NodeAttrSetBool(tab, "Inbound Connection Enabled", pRemoteAccess->Connections & (1 << 0), 0);
}

static void ProcBISEntryPoint(PNODE tab, void* p)
{
	//PBISEntryPoint pBISEntryPoint = (PBISEntryPoint)p;
	(void)p;
	NWL_NodeAttrSet(tab, "Description", "Boot Integrity Services Entry Point", 0);
}

static void ProcSysBootInfo(PNODE tab, void* p)
{
	PSysBootInfo pBootInfo = (PSysBootInfo)p;
	NWL_NodeAttrSet(tab, "Description", "System Boot Information", 0);
	if (pBootInfo->Header.Length < 0x0b)
		return;
	// TODO
}

static void ProcMemoryErrInfo64(PNODE tab, void* p)
{
	PMemoryErrInfo64 pMemErrInfo = (PMemoryErrInfo64)p;
	NWL_NodeAttrSet(tab, "Description", "64-Bit Memory Error Information", 0);
	if (pMemErrInfo->Header.Length < 0x17) // 2.1
		return;
	NWL_NodeAttrSet(tab, "Error Type", pMemErrTypeToStr(pMemErrInfo->ErrType), 0);
	NWL_NodeAttrSet(tab, "Error Granularity", pMemErrGranularityToStr(pMemErrInfo->ErrGranularity), 0);
	NWL_NodeAttrSet(tab, "Error Operation", pMemErrOperationToStr(pMemErrInfo->ErrOperation), 0);
	NWL_NodeAttrSetf(tab, "Vendor Syndrome", 0, "0x%08lX", pMemErrInfo->VendorSyndrome);
	NWL_NodeAttrSetf(tab, "Memory Array Error Address", 0, "0x%016llX", pMemErrInfo->MemArrayErrAddr);
	NWL_NodeAttrSetf(tab, "Device Error Address", 0, "0x%016llX", pMemErrInfo->DevErrAddr);
	NWL_NodeAttrSetf(tab, "Error Resolution", 0, "0x%08lX", pMemErrInfo->ErrResolution);
}

static const CHAR*
pOnBoardDevExtTypeToStr(UCHAR Type)
{
	switch (Type & 0x7f)
	{
	case 0x01: return "Other";
	case 0x03: return "Video";
	case 0x04: return "SCSI Controller";
	case 0x05: return "Ethernet";
	case 0x06: return "Token Ring";
	case 0x07: return "Sound";
	case 0x08: return "PATA Controller";
	case 0x09: return "SATA Controller";
	case 0x0a: return "SAS Controller";
	case 0x0b: return "Wireless LAN";
	case 0x0c: return "Bluetooth";
	case 0x0d: return "WWAN";
	case 0x0e: return "eMMC";
	case 0x0f: return "NVMe Controller";
	case 0x10: return "UFS Controller";
	}
	return "Unknown";
}

static void ProcOnBoardDevicesExtInfo(PNODE tab, void* p)
{
	POnBoardDevicesExtInfo pDevInfo = (POnBoardDevicesExtInfo)p;
	NWL_NodeAttrSet(tab, "Description", "Onboard Devices Extended Information", 0);
	if (pDevInfo->Header.Length < 0x0b)
		return;
	NWL_NodeAttrSet(tab, "Reference Designation", LocateString(p, pDevInfo->RefDesignation), 0);
	NWL_NodeAttrSet(tab, "Device Status", (pDevInfo->DeviceType & 0x80) ? "Enabled" : "Disabled", 0);
	NWL_NodeAttrSet(tab, "Device Type", pOnBoardDevExtTypeToStr(pDevInfo->DeviceType), 0);
	NWL_NodeAttrSetf(tab, "Device Type Instance", NAFLG_FMT_NUMERIC, "%u", pDevInfo->DeviceTypeInstance);
	NWL_NodeAttrSetf(tab, "Segment Group Number", NAFLG_FMT_NUMERIC, "%u", pDevInfo->SegmentGroupNum);
	NWL_NodeAttrSetf(tab, "Bus Number", NAFLG_FMT_NUMERIC, "%u", pDevInfo->BusNum);
	NWL_NodeAttrSetf(tab, "Device Number", NAFLG_FMT_NUMERIC, "%u", (pDevInfo->DevFunNum & 0xf8) >> 3);
	NWL_NodeAttrSetf(tab, "Function Number", NAFLG_FMT_NUMERIC, "%u", pDevInfo->DevFunNum & 0x07);
}

static void ProcTPMDevice(PNODE tab, void* p)
{
	PTPMDevice pTPM = (PTPMDevice)p;
	NWL_NodeAttrSet(tab, "Description", "TPM Device", 0);
	if (pTPM->Header.Length < 0x1f)
		return;
	NWL_NodeAttrSetf(tab, "Vendor", 0, "%c%c%c%c",
		pTPM->Vendor[0], pTPM->Vendor[1],
		pTPM->Vendor[2], pTPM->Vendor[3]);
	NWL_NodeAttrSetf(tab, "Spec Version", 0, "%u%u", pTPM->MajorSpecVer, pTPM->MinorSpecVer);
	NWL_NodeAttrSet(tab, "TPM Description", LocateString(p, pTPM->Description), 0);
}

static const CHAR*
pProcessorAdditionalInfoTypeToStr(UCHAR Type)
{
	switch (Type)
	{
	case 0x01: return "IA32";
	case 0x02: return "x64";
	case 0x03: return "IA64";
	case 0x04: return "AARCH32";
	case 0x05: return "AARCH64";
	}
	return "Unsupported Architecture";
}

static void ProcProcessorAdditionalInfo(PNODE tab, void* p)
{
	PProcessorAdditionalInfo pProcessor = (PProcessorAdditionalInfo)p;
	NWL_NodeAttrSet(tab, "Description", "Processor Additional Information", 0);
	if (pProcessor->Header.Length < 0x06)
		return;
	NWL_NodeAttrSetf(tab, "Referenced Handle", NAFLG_FMT_NUMERIC, "%u", pProcessor->RefHandle);
	if (pProcessor->Header.Length < 0x08)
		return;
	NWL_NodeAttrSetf(tab, "Block Length", NAFLG_FMT_NUMERIC, "%u", pProcessor->BlockLength);
	NWL_NodeAttrSet(tab, "Processor Type", pProcessorAdditionalInfoTypeToStr(pProcessor->ProcessorType), 0);
}

static void ProcFwInventory(PNODE tab, void* p)
{
	PFirmwareInventoryInfo pFirmware = (PFirmwareInventoryInfo)p;
	NWL_NodeAttrSet(tab, "Description", "Firmware Inventory Information", 0);
	if (pFirmware->Header.Length < 0x18)
		return;
	NWL_NodeAttrSet(tab, "Component Name", LocateString(p, pFirmware->ComponentName), 0);
	NWL_NodeAttrSet(tab, "Firmware Version", LocateString(p, pFirmware->Version), 0);
	NWL_NodeAttrSetf(tab, "Version Format", NAFLG_FMT_NUMERIC, "%u", pFirmware->VersionFormat);
	NWL_NodeAttrSet(tab, "Firmware ID", LocateString(p, pFirmware->ID), 0);
	NWL_NodeAttrSetf(tab, "ID Format", NAFLG_FMT_NUMERIC, "%u", pFirmware->IDFormat);
	NWL_NodeAttrSet(tab, "Release Date", LocateString(p, pFirmware->ReleaseDate), 0);
	NWL_NodeAttrSet(tab, "Manufacturer", LocateString(p, pFirmware->Manufacturer), 0);
	NWL_NodeAttrSet(tab, "Lowest Supported Firmware Version",
		LocateString(p, pFirmware->LowestSupportedVersion), 0);
	if (pFirmware->ImageSize != ~0ULL)
		NWL_NodeAttrSet(tab, "Image Size", NWL_GetHumanSize(pFirmware->ImageSize, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
}

static void ProcStringProperty(PNODE tab, void* p)
{
	PStringProperty pString = (PStringProperty)p;
	NWL_NodeAttrSet(tab, "Description", "String Property", 0);
	if (pString->Header.Length < 0x09)
		return;
	NWL_NodeAttrSetf(tab, "String Property ID", NAFLG_FMT_NUMERIC, "%u", pString->ID);
	NWL_NodeAttrSet(tab, "String Property Value",LocateString(p, pString->Value), 0);
	NWL_NodeAttrSetf(tab, "Parent Handle", NAFLG_FMT_NUMERIC, "%u", pString->ParentHandle);
}

static void ProcInactive(PNODE tab, void* p)
{
	NWL_NodeAttrSet(tab, "Description", "Inactive", 0);
}

static void ProcEndTable(PNODE tab, void* p)
{
	NWL_NodeAttrSet(tab, "Description", "End-of-Table", 0);
}

static void DumpSMBIOS(PNODE node, void* Addr, UINT Len, UINT8 Type)
{
	LPBYTE p = (LPBYTE)(Addr);
	const LPBYTE lastAddress = p + Len;
	PSMBIOSHEADER pHeader;
	PNODE tab;

	while (p < lastAddress)
	{
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
		case 15:
			ProcSystemEventLog(tab, pHeader);
			break;
		case 16:
			ProcMemoryArray(tab, pHeader);
			break;
		case 17:
			ProcMemoryDevice(tab, pHeader);
			break;
		case 18:
			ProcMemoryErrInfo(tab, pHeader);
			break;
		case 19:
			ProcMemoryArrayMappedAddress(tab, pHeader);
			break;
		case 20:
			ProcMemoryDeviceMappedAddress(tab, pHeader);
			break;
		case 21:
			ProcBuiltinPointing(tab, pHeader);
			break;
		case 22:
			ProcPortableBattery(tab, pHeader);
			break;
		case 23:
			ProcSysReset(tab, pHeader);
			break;
		case 24:
			ProcHwSecurity(tab, pHeader);
			break;
		case 25:
			ProcSysPowerCtrl(tab, pHeader);
			break;
		case 26:
			ProcVoltageProbe(tab, pHeader);
			break;
		case 27:
			ProcCoolingDevice(tab, pHeader);
			break;
		case 28:
			ProcTempProbe(tab, pHeader);
			break;
		case 29:
			ProcElecCurrentProbe(tab, pHeader);
			break;
		case 30:
			ProcOutOfBandRemoteAccess(tab, pHeader);
			break;
		case 31:
			ProcBISEntryPoint(tab, pHeader);
			break;
		case 32:
			ProcSysBootInfo(tab, pHeader);
			break;
		case 33:
			ProcMemoryErrInfo64(tab, pHeader);
			break;
		case 41:
			ProcOnBoardDevicesExtInfo(tab, pHeader);
			break;
		case 43:
			ProcTPMDevice(tab, pHeader);
			break;
		case 44:
			ProcProcessorAdditionalInfo(tab, pHeader);
			break;
		case 45:
			ProcFwInventory(tab, pHeader);
			break;
		case 46:
			ProcStringProperty(tab, pHeader);
			break;
		case 126:
			ProcInactive(tab, pHeader);
			break;
		case 127:
			ProcEndTable(tab, pHeader);
			break;
		default:
			break;
		}
	next_table:
		if ((pHeader->Type == 127) && (pHeader->Length == 4))
			break;
		p += pHeader->Length;
		while ((*p++ != 0 || *p++ != 0) && p < lastAddress)
			;
	}
}

PNODE NW_Smbios(VOID)
{
	
	PNODE node = NWL_NodeAlloc("SMBIOS", NFLG_TABLE);
	PNODE info = NWL_NodeAppendNew(node, "DMI", NFLG_TABLE_ROW);
	if (NWLC->DmiInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	if (!NWLC->NwSmbios)
		NWLC->NwSmbios = NWL_GetSmbios();
	if (!NWLC->NwSmbios)
		return node;
	NWL_NodeAttrSetf(info, "SMBIOS Version", 0, "%u.%u", NWLC->NwSmbios->MajorVersion, NWLC->NwSmbios->MinorVersion);
	NWL_NodeAttrSetf(info, "DMI Reversion", NAFLG_FMT_NUMERIC, "%u", NWLC->NwSmbios->DmiRevision);
	NWL_NodeAttrSetf(info, "SMBIOS Length", NAFLG_FMT_NUMERIC, "%u", NWLC->NwSmbios->Length);
	DumpSMBIOS(node, NWLC->NwSmbios->Data, NWLC->NwSmbios->Length, NWLC->SmbiosType);
	return node;
}
