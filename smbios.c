// SPDX-License-Identifier: Unlicense

// based on https://github.com/KunYi/DumpSMBIOS

#include <windows.h>
#include <sysinfoapi.h>
#include <stdio.h>
#include <stdlib.h>
#include "nwinfo.h"

static const char* mem_human_sizes[6] =
{ "B", "KB", "MB", "GB", "TB", "PB", };

static const char* LocateString(const char* str, UINT i)
{
	static const char nul[] = "NULL";

	if (0 == i || 0 == *str)
		return nul;

	while (--i)
	{
		str += strlen((char*)str) + 1;
	}
	return str;
}

static const char* toPointString(void* p)
{
	return (char*)p + ((PSMBIOSHEADER)p)->Length;
}

static void ProcBIOSInfo(void* p)
{
	PBIOSInfo pBIOS = (PBIOSInfo)p;
	const char* str = toPointString(p);
	printf("BIOS information\n");
	printf("  Vendor: %s\n", LocateString(str, pBIOS->Vendor));
	printf("  Version: %s\n", LocateString(str, pBIOS->Version));
	printf("  Starting Segment: %04Xh\n", pBIOS->StartingAddrSeg);
	printf("  Release Date: %s\n", LocateString(str, pBIOS->ReleaseDate));
	printf("  Image Size: %uK\n", (pBIOS->ROMSize + 1) * 64);
	if (pBIOS->Header.Length > 0x14)
	{
		if (pBIOS->MajorRelease != 0xff || pBIOS->MinorRelease != 0xff)
			printf("  System BIOS version: %u.%u\n", pBIOS->MajorRelease, pBIOS->MinorRelease);
		if (pBIOS->ECFirmwareMajor != 0xff || pBIOS->ECFirmwareMinor != 0xff)
			printf("  EC Firmware version: %u.%u\n", pBIOS->ECFirmwareMajor, pBIOS->ECFirmwareMinor);
	}
}

static void ProcSysInfo(void* p)
{
	PSystemInfo pSystem = (PSystemInfo)p;
	const char* str = toPointString(p);

	printf("System information\n");
	printf("  Manufacturer: %s\n", LocateString(str, pSystem->Manufacturer));
	printf("  Product Name: %s\n", LocateString(str, pSystem->ProductName));
	printf("  Version: %s\n", LocateString(str, pSystem->Version));
	printf("  Serial Number: %s\n", LocateString(str, pSystem->SN));
	// for v2.1 and later
	if (pSystem->Header.Length > 0x08)
	{
		printf("  UUID: %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
			pSystem->UUID[0], pSystem->UUID[1], pSystem->UUID[2], pSystem->UUID[3],
			pSystem->UUID[4], pSystem->UUID[5], pSystem->UUID[6], pSystem->UUID[7],
			pSystem->UUID[8], pSystem->UUID[9], pSystem->UUID[10], pSystem->UUID[11],
			pSystem->UUID[12], pSystem->UUID[13], pSystem->UUID[14], pSystem->UUID[15]);
	}

	if (pSystem->Header.Length > 0x19)
	{
		// fileds for spec. 2.4
		printf("  SKU Number: %s\n", LocateString(str, pSystem->SKUNumber));
		printf("  Family: %s\n", LocateString(str, pSystem->Family));
	}
}

static void ProcBoardInfo(void* p)
{
	PBoardInfo pBoard = (PBoardInfo)p;
	const char* str = toPointString(p);

	printf("Base Board information\n");
	printf("  Manufacturer: %s\n", LocateString(str, pBoard->Manufacturer));
	printf("  Product Name: %s\n", LocateString(str, pBoard->Product));
	printf("  Version: %s\n", LocateString(str, pBoard->Version));
	printf("  Serial Number: %s\n", LocateString(str, pBoard->SN));
	printf("  Asset Tag: %s\n", LocateString(str, pBoard->AssetTag));
	if (pBoard->Header.Length > 0x08)
	{
		printf("  Location in Chassis: %s\n", LocateString(str, pBoard->LocationInChassis));
	}
}

static void ProcSystemEnclosure(void* p)
{
	PSystemEnclosure pSysEnclosure = (PSystemEnclosure)p;
	const char* str = toPointString(p);
	printf("System Enclosure information\n");
	printf("  Manufacturer: %s\n", LocateString(str, pSysEnclosure->Manufacturer));
	printf("  Version: %s\n", LocateString(str, pSysEnclosure->Version));
	printf("  Serial Number: %s\n", LocateString(str, pSysEnclosure->SN));
	printf("  Asset Tag: %s\n", LocateString(str, pSysEnclosure->AssetTag));
}

static void ProcProcessorInfo(void* p)
{
	PProcessorInfo	pProcessor = (PProcessorInfo)p;
	const char* str = toPointString(p);

	printf("Processor information\n");
	printf("  Socket Designation: %s\n", LocateString(str, pProcessor->SocketDesignation));
	printf("  Processor Manufacturer: %s\n", LocateString(str, pProcessor->Manufacturer));
	printf("  Processor Version: %s\n", LocateString(str, pProcessor->Version));
	if (!pProcessor->Voltage) {
		// unsupported
	}
	else if (pProcessor->Voltage & (1 << 7)) {
		UCHAR volt = pProcessor->Voltage - 0x80;
		printf("  Voltage: %u.%uV\n", volt / 10, volt % 10);
	}
	else {
		printf("  Voltage:%s%s%s\n",
			pProcessor->Voltage & (1 << 0) ? " 5V" : "",
			pProcessor->Voltage & (1 << 1) ? " 3.3V" : "",
			pProcessor->Voltage & (1 << 2) ? " 2.9V" : "");
	}
	if (pProcessor->ExtClock)
		printf("  External Clock: %uMHz\n", pProcessor->ExtClock);
	printf("  Max Speed: %uMHz\n", pProcessor->MaxSpeed);
	printf("  Current Speed: %uMHz\n", pProcessor->CurrentSpeed);
	if (pProcessor->Header.Length > 0x20)
	{
		// 2.3+
		printf("  Serial Number: %s\n", LocateString(str, pProcessor->Serial));
		printf("  Asset Tag: %s\n", LocateString(str, pProcessor->AssetTag));
		if (pProcessor->CoreCount == 0xff && pProcessor->Header.Length > 0x2a)
			printf("  Core Count: %u\n", pProcessor->CoreCount2);
		else
			printf("  Core Count: %u\n", pProcessor->CoreCount);
		if (pProcessor->ThreadCount == 0xff && pProcessor->Header.Length > 0x2a)
			printf("  Thread Count: %u\n", pProcessor->ThreadCount2);
		else
			printf("  Thread Count: %u\n", pProcessor->ThreadCount);
	}
}

static void ProcMemModuleInfo(void* p)
{
	PMemModuleInfo	pMemModule = (PMemModuleInfo)p;
	const char* str = toPointString(p);

	printf("Memory Module information\n");
	printf("  Socket Designation: %s\n", LocateString(str, pMemModule->SocketDesignation));
	printf("  Current Speed: %uns\n", pMemModule->CurrentSpeed);
}

static void ProcCacheInfo(void* p)
{
	PCacheInfo	pCache = (PCacheInfo)p;
	const char* str = toPointString(p);
	UINT64 sz = 0;

	printf("Cache information\n");
	printf("  Socket Designation: %s\n", LocateString(str, pCache->SocketDesignation));
	if (pCache->MaxSize & (1 << 15))
	{
		sz = ((UINT64)pCache->MaxSize - (1 << 15)) * 64 * 1024;
	}
	else
	{
		sz = ((UINT64) pCache->MaxSize) * 1024;
	}
	printf("  Max Cache Size: %s\n", GetHumanSize(sz, mem_human_sizes, 1024));
	if (pCache->InstalledSize & (1 << 15))
	{
		sz = ((UINT64)pCache->InstalledSize - (1 << 15)) * 64 * 1024;
	}
	else
	{
		sz = ((UINT64)pCache->InstalledSize) * 1024;
	}
	printf("  Installed Cache Size: %s\n", GetHumanSize(sz, mem_human_sizes, 1024));
	if (pCache->Speed)
		printf("  Cache Speed: %uns\n", pCache->Speed);
}

static void ProcOEMString(void* p)
{
	PSMBIOSHEADER pHdr = (PSMBIOSHEADER)p;
	const char* str = toPointString(p);
	printf("OEM String: %s\n", LocateString(str, *(((char*)p) + 4)));
}

static void ProcMemoryDevice(void* p)
{
	PMemoryDevice pMD = (PMemoryDevice)p;
	const char* str = toPointString(p);
	UINT64 sz = 0;

	printf("Memory Device\n");
	printf("  Total Width: %ubits\n", pMD->TotalWidth);
	printf("  Data Width: %ubits\n", pMD->DataWidth);
	if (pMD->Size & (1 << 15))
		sz = ((UINT64)pMD->Size - (1 << 15)) * 1024;
	else
		sz = ((UINT64)pMD->Size) * 1024 * 1024;
	printf("  Size: %s\n", GetHumanSize(sz, mem_human_sizes, 1024));
	printf("  Device Locator: %s\n", LocateString(str, pMD->DeviceLocator));
	printf("  Bank Locator: %s\n", LocateString(str, pMD->BankLocator));
	if (pMD->Header.Length > 0x15)
	{
		printf("  Speed: %u\n", pMD->Speed);
		printf("  Manufacturer: %s\n", LocateString(str, pMD->Manufacturer));
		printf("  Serial Number: %s\n", LocateString(str, pMD->SN));
		printf("  Asset Tag Number: %s\n", LocateString(str, pMD->AssetTag));
		printf("  Part Number: %s\n", LocateString(str, pMD->PN));
	}
}

static void ProcMemoryArrayMappedAddress(void* p)
{
	PMemoryArrayMappedAddress pMAMA = (PMemoryArrayMappedAddress)p;
	const char* str = toPointString(p);

	printf("Memory Array Mapped Address\n");
	printf("  Starting Address: 0x%08X\n", pMAMA->Starting);
	printf("  Ending Address: 0x%08X\n", pMAMA->Ending);
	printf("  Memory Array Handle: 0x%X\n", pMAMA->Handle);
	printf("  Partition Width: 0x%X\n", pMAMA->PartitionWidth);
}

static void ProcPortableBattery(void* p)
{
	PPortableBattery pPB = (PPortableBattery)p;
	const char* str = toPointString(p);

	printf("Portable Battery\n");
	printf("  Location: %s\n", LocateString(str, pPB->Location));
	printf("  Manufacturer: %s\n", LocateString(str, pPB->Manufacturer));
	printf("  Manufacturer Date: %s\n", LocateString(str, pPB->Date));
	printf("  Serial Number: %s\n", LocateString(str, pPB->SN));
}

static void DumpSMBIOSStruct(void* Addr, UINT Len)
{
	LPBYTE p = (LPBYTE)(Addr);
	const LPBYTE lastAddress = p + Len;
	PSMBIOSHEADER pHeader;

	for (;;) {
		pHeader = (PSMBIOSHEADER)p;
		switch (pHeader->Type)
		{
		case 0:
			ProcBIOSInfo(pHeader);
			break;
		case 1:
			ProcSysInfo(pHeader);
			break;
		case 2:
			ProcBoardInfo(pHeader);
			break;
		case 3:
			ProcSystemEnclosure(pHeader);
			break;
		case 4:
			ProcProcessorInfo(pHeader);
			break;
		case 6:
			ProcMemModuleInfo(pHeader);
			break;
		case 7:
			ProcCacheInfo(pHeader);
			break;
		case 11:
			ProcOEMString(pHeader);
			break;
		case 17:
			ProcMemoryDevice(pHeader);
			break;
		case 19:
			ProcMemoryArrayMappedAddress(pHeader);
			break;
		case 22:
			ProcPortableBattery(pHeader);
			break;
		default:
			break;
		}
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

void nwinfo_smbios(void)
{
    DWORD smBiosDataSize = 0;
    struct RawSMBIOSData* smBiosData = NULL;
    DWORD bytesWritten = 0;

    // Query size of SMBIOS data.
    smBiosDataSize = GetSystemFirmwareTable('RSMB', 0, NULL, 0);
    if (smBiosDataSize == 0)
        return;
    // Allocate memory for SMBIOS data
    smBiosData = (struct RawSMBIOSData*)malloc(smBiosDataSize);
    if (!smBiosData)
        return;
    // Retrieve the SMBIOS table
    GetSystemFirmwareTable('RSMB', 0, smBiosData, smBiosDataSize);
    printf("SMBIOS Version: %u.%u\n", smBiosData->MajorVersion, smBiosData->MinorVersion);
    printf("DMI Version: %u\n", smBiosData->DmiRevision);
	printf("\n");
	DumpSMBIOSStruct(smBiosData->Data, smBiosData->Length);
}
