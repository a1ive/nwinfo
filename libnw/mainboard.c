// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libnw.h"
#include "utils.h"
#include "smbios.h"
#include "tpm.h"

#define MB_VENDOR_IMPL
#include "mb_vendor.h"
#define CHIPSET_IDS_IMPL
#include "chipset_ids.h"

#include "chip_ids.h"

#include "lpc/lpc.h"

static enum DMI_VENDOR_ID
GetBoardVendor(const char* vendor)
{
	enum DMI_VENDOR_ID id = VENDOR_UNKNOWN;
	if (vendor == NULL)
		goto out;
	for (size_t i = 0; i < ARRAYSIZE(DMI_VENDOR_LIST); i++)
	{
		switch (DMI_VENDOR_LIST[i].match)
		{
		case MATCH_EQUAL:
			if (_stricmp(vendor, DMI_VENDOR_LIST[i].str) == 0)
			{
				id = DMI_VENDOR_LIST[i].id;
				goto out;
			}
			break;
		case MATCH_START:
			if (_strnicmp(vendor, DMI_VENDOR_LIST[i].str, strlen(DMI_VENDOR_LIST[i].str)) == 0)
			{
				id = DMI_VENDOR_LIST[i].id;
				goto out;
			}
			break;
		case MATCH_SEARCH:
			if (strstr(vendor, DMI_VENDOR_LIST[i].str) != NULL)
			{
				id = DMI_VENDOR_LIST[i].id;
				goto out;
			}
			break;
		}
	}
out:
	return id;
}

static LPCSTR
GetPciDeviceName(PNODE dev)
{
	const char* name = NWL_NodeAttrGet(dev, "Device");
	if (name[0] == '-' && name[1] == '\0')
		name = NWL_NodeAttrGet(dev, "Description");
	if (name[0] == '-' && name[1] == '\0')
		name = NWL_NodeAttrGet(dev, "HWID");
	return name;
}

static void
FillPciDevInfo(NWLIB_PCI_DEV_INFO* pci, PNODE dev)
{
	strncpy_s(pci->Name, sizeof(pci->Name), GetPciDeviceName(dev), _TRUNCATE);
	strncpy_s(pci->VendorId, sizeof(pci->VendorId), NWL_NodeAttrGet(dev, "Vendor ID"), _TRUNCATE);
	strncpy_s(pci->DeviceId, sizeof(pci->DeviceId), NWL_NodeAttrGet(dev, "Device ID"), _TRUNCATE);
	strncpy_s(pci->BDF, sizeof(pci->BDF), NWL_NodeAttrGet(dev, "BDF"), _TRUNCATE);
}

static void
GetChipsetInfo(NWLIB_MAINBOARD_INFO* info)
{
	PNWL_ARG_SET pciSet = NULL;
	NWL_ArgSetAddStr(&pciSet, "0600"); // Host Bridge
	NWL_ArgSetAddStr(&pciSet, "0601"); // ISA Bridge
	NWL_ArgSetAddStr(&pciSet, "0C05"); // SMBus

	PNODE root = NWL_NodeAlloc("Root", NFLG_TABLE);
	NWL_EnumPci(root, pciSet);

	PNODE pciHost = NULL;
	PNODE pciIsa = NULL;
	PNODE pciSmbus = NULL;
	INT pciCount = NWL_NodeChildCount(root);
	for (INT i = 0; i < pciCount; i++)
	{
		PNODE dev = NWL_NodeEnumChild(root, i);
		const char* cc = NWL_NodeAttrGet(dev, "Class Code");
		if (strncmp(cc, "0600", 4) == 0)
		{
			if (pciHost == NULL)
				pciHost = dev;
			else if (strcmp(NWL_NodeAttrGet(dev, "BDF"), "00:00.0") == 0)
				pciHost = dev;
		}
		else if (strncmp(cc, "0601", 4) == 0)
		{
			pciIsa = dev;
		}
		else if (strncmp(cc, "0C05", 4) == 0)
		{
			pciSmbus = dev;
		}
	}

	if (pciHost)
		FillPciDevInfo(&info->HostBridge, pciHost);
	if (pciSmbus)
		FillPciDevInfo(&info->Smbus, pciSmbus);
	if (pciIsa)
		FillPciDevInfo(&info->IsaBridge, pciIsa);

	LPCSTR vid = NULL;
	LPCSTR did = NULL;
	if (info->HostBridge.VendorId[0])
		vid = info->HostBridge.VendorId;
	if (info->Smbus.VendorId[0])
		vid = info->Smbus.VendorId;
	if (info->IsaBridge.VendorId[0])
	{
		vid = info->IsaBridge.VendorId;
		did = info->IsaBridge.DeviceId;
	}

	if (vid == NULL)
	{
		info->Chipset = "Unknown";
	}
	else if (strcmp(vid, "8086") == 0) // Intel
	{
		if (did)
		{
			for (size_t i = 0; i < ARRAYSIZE(INTEL_ISA_LIST); i++)
			{
				if (strcmp(did, INTEL_ISA_LIST[i].id) == 0)
				{
					info->Chipset = INTEL_ISA_LIST[i].name;
					break;
				}
			}
		}
		if (info->Chipset == NULL)
		{
			for (size_t i = 0; i < ARRAYSIZE(INTEL_CHIPSET_LIST); i++)
			{
				if (strstr(info->ProductStr, INTEL_CHIPSET_LIST[i]) != NULL)
				{
					info->Chipset = INTEL_CHIPSET_LIST[i];
					break;
				}
			}
		}
		if (info->Chipset == NULL)
			info->Chipset = "INTEL";
	}
	else if (strcmp(vid, "1022") == 0 || strcmp(vid, "1002") == 0) // AMD/ATI
	{
		for (size_t i = 0; i < ARRAYSIZE(AMD_CHIPSET_LIST); i++)
		{
			if (strstr(info->ProductStr, AMD_CHIPSET_LIST[i]) != NULL)
			{
				info->Chipset = AMD_CHIPSET_LIST[i];
				break;
			}
		}
		if (info->Chipset == NULL)
			info->Chipset = "AMD";
	}
	else if (strcmp(vid, "1039") == 0) // SiS
	{
		info->Chipset = "SiS";
	}
	else if (strcmp(vid, "10B9") == 0) // ULi
	{
		info->Chipset = "ULi";
	}
	else if (strcmp(vid, "10DE") == 0) // NVIDIA
	{
		info->Chipset = "NVIDIA";
	}
	else if (strcmp(vid, "1106") == 0) // VIA
	{
		info->Chipset = "VIA";
	}
	else if (strcmp(vid, "1D17") == 0) // Zhaoxin
	{
		info->Chipset = "Zhaoxin";
	}
	else if (strcmp(vid, "1D94") == 0) // Hygon
	{
		info->Chipset = "Hygon";
	}
	else
	{
		info->Chipset = "Unknown";
	}

	NWL_ArgSetFree(pciSet);
	NWL_NodeFree(root, 1);
}

static LPCSTR
CheckDMIString(LPCSTR str)
{
	if (str == NULL)
		goto fail;
	if (_stricmp(str, "NULL") == 0)
		goto fail;
	if (_stricmp(str, "To be filled by O.E.M.") == 0)
		goto fail;
	if (_stricmp(str, "Default string") == 0)
		goto fail;
	if (_stricmp(str, "System Version") == 0)
		goto fail;
	if (_stricmp(str, "System Product Name") == 0)
		goto fail;
	if (_stricmp(str, "System Serial Number") == 0)
		goto fail;
	if (_stricmp(str, "Base Board Product Name") == 0)
		goto fail;
	if (_stricmp(str, "Base Board Serial Number") == 0)
		goto fail;
	return str;
fail:
	return "";
}

BOOL NWL_GetMainboardInfo(NWLIB_MAINBOARD_INFO* info)
{
	LPBYTE p;
	LPBYTE lastAddress;
	PSMBIOSHEADER pHeader;
	PBIOSInfo pBios = NULL;
	PSystemInfo pSystem = NULL;
	PBoardInfo pBoard = NULL;
	PSystemEnclosure pEnclosure = NULL;
	PNWL_ARG_SET dmiSet = NULL;

	if (info == NULL || NWLC->NwSmbios == NULL)
		return FALSE;

	p = (LPBYTE)NWLC->NwSmbios->Data;
	lastAddress = p + NWLC->NwSmbios->Length;
	NWL_ArgSetAddU64(&dmiSet, 0);
	NWL_ArgSetAddU64(&dmiSet, 1);
	NWL_ArgSetAddU64(&dmiSet, 2);
	NWL_ArgSetAddU64(&dmiSet, 3);
	while ((pHeader = NWL_GetNextDmiTable(&p, lastAddress, dmiSet)) != NULL)
	{
		switch (pHeader->Type)
		{
		case 0:
			if (pBios == NULL && pHeader->Length >= 0x12)
				pBios = (PBIOSInfo)pHeader;
			break;
		case 1:
			if (pSystem == NULL && pHeader->Length >= 0x08)
				pSystem = (PSystemInfo)pHeader;
			break;
		case 2:
			if (pBoard == NULL && pHeader->Length >= 0x08)
			{
				PBoardInfo pInfo = (PBoardInfo)pHeader;
				if (pInfo->Header.Length >= 0x0E && pInfo->Type != 0x0A)
					break;
				pBoard = pInfo;
			}
			break;
		case 3:
			if (pEnclosure == NULL && pHeader->Length >= 0x09)
				pEnclosure = (PSystemEnclosure)pHeader;
			break;
		}
	}
	NWL_ArgSetFree(dmiSet);

	if (pBoard == NULL)
		return FALSE;
	ZeroMemory(info, sizeof(NWLIB_MAINBOARD_INFO));

	if (pBios != NULL)
	{
		info->BiosVendor = NWL_GetDmiString((UINT8*)pBios, pBios->Vendor);
		info->BiosVersion = NWL_GetDmiString((UINT8*)pBios, pBios->Version);
		info->BiosDate = NWL_GetDmiString((UINT8*)pBios, pBios->ReleaseDate);
	}
	info->BiosVendor = CheckDMIString(info->BiosVendor);
	info->BiosVersion = CheckDMIString(info->BiosVersion);
	info->BiosDate = CheckDMIString(info->BiosDate);

	if (pSystem != NULL)
	{
		info->SystemVendor = NWL_GetDmiString((UINT8*)pSystem, pSystem->Manufacturer);
		info->SystemProduct = NWL_GetDmiString((UINT8*)pSystem, pSystem->ProductName);
		info->SystemVersion = NWL_GetDmiString((UINT8*)pSystem, pSystem->Version);
		info->SystemSerial = NWL_GetDmiString((UINT8*)pSystem, pSystem->SN);
		if (pSystem->Header.Length >= 0x19)
			memcpy(info->SystemUuid, pSystem->UUID, sizeof(info->SystemUuid));
	}
	info->SystemVendor = CheckDMIString(info->SystemVendor);
	info->SystemProduct = CheckDMIString(info->SystemProduct);
	info->SystemVersion = CheckDMIString(info->SystemVersion);
	info->SystemSerial = CheckDMIString(info->SystemSerial);

	if (pEnclosure != NULL)
		info->EnclosureType = NWL_SystemEnclosureTypeToStr(pEnclosure->Type);
	else
		info->EnclosureType = "Unknown";

	info->VendorStr = CheckDMIString(NWL_GetDmiString((UINT8*)pBoard, pBoard->Manufacturer));
	info->ProductStr = CheckDMIString(NWL_GetDmiString((UINT8*)pBoard, pBoard->Product));
	info->VersionStr = CheckDMIString(NWL_GetDmiString((UINT8*)pBoard, pBoard->Version));
	info->SerialStr = CheckDMIString(NWL_GetDmiString((UINT8*)pBoard, pBoard->SN));
	info->VendorId = GetBoardVendor(info->VendorStr);
	if (info->VendorId == VENDOR_UNKNOWN)
		info->VendorName = info->VendorStr;
	else
		info->VendorName = DMI_VENDOR_NAME[info->VendorId];

	GetChipsetInfo(info);

	return TRUE;
}

static BOOL
FormatTpmSpecDate(const NWL_TPM_INFO* info, CHAR* buffer, size_t bufferSize)
{
	static const UINT8 daysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	UINT32 dayOfYear = 0;
	UINT32 month = 1;
	UINT32 day = 0;
	BOOL leapYear = FALSE;

	if (info == NULL || buffer == NULL || bufferSize == 0)
		return FALSE;
	if (info->SpecYear == 0 || info->SpecDayOfYear == 0)
		return FALSE;

	dayOfYear = info->SpecDayOfYear;
	leapYear = (info->SpecYear % 4U == 0U) &&
		((info->SpecYear % 100U != 0U) || (info->SpecYear % 400U == 0U));
	if (dayOfYear > (leapYear ? 366U : 365U))
	{
		snprintf(buffer, bufferSize, "%u / day %u", info->SpecYear, info->SpecDayOfYear);
		return TRUE;
	}

	for (size_t i = 0; i < ARRAYSIZE(daysInMonth); i++)
	{
		UINT32 monthDays = daysInMonth[i];
		if (i == 1 && leapYear)
			monthDays++;
		if (dayOfYear <= monthDays)
		{
			day = dayOfYear;
			break;
		}
		dayOfYear -= monthDays;
		month++;
	}

	if (day == 0)
		return FALSE;

	snprintf(buffer, bufferSize, "%04u-%02u-%02u", info->SpecYear, month, day);
	return TRUE;
}

static void
GetTpmInfo(PNODE node)
{
	NWL_TPM_INFO tpmInfo = { 0 };
	CHAR specDate[16] = { 0 };
	LPSTR algorithms = NULL;
	TBS_RESULT rc = 0;

	rc = NWL_TPMGetInfo(&tpmInfo);
	if (rc != TBS_SUCCESS)
		return;
	NWL_NodeAttrSet(node, "TPM Version", NWL_TPMGetVersionStr(tpmInfo.TpmVersion), 0);
	if (tpmInfo.ManufacturerId[0] != '\0')
		NWL_NodeAttrSet(node, "TPM Manufacturer ID", tpmInfo.ManufacturerId, 0);
	if (tpmInfo.ManufacturerName != NULL)
		NWL_NodeAttrSet(node, "TPM Manufacturer", tpmInfo.ManufacturerName, 0);
	if (tpmInfo.VendorString[0] != '\0')
		NWL_NodeAttrSet(node, "TPM Vendor String", tpmInfo.VendorString, 0);
	if (tpmInfo.FirmwareVersion[0] != '\0')
		NWL_NodeAttrSet(node, "TPM Firmware Version", tpmInfo.FirmwareVersion, 0);
	if (tpmInfo.SpecFamily[0] != '\0')
		NWL_NodeAttrSet(node, "TPM Spec Family", tpmInfo.SpecFamily, 0);
	if (tpmInfo.SpecLevel != 0)
		NWL_NodeAttrSetf(node, "TPM Spec Level", NAFLG_FMT_NUMERIC, "%u", tpmInfo.SpecLevel);
	if (tpmInfo.SpecRevision != 0)
		NWL_NodeAttrSetf(node, "TPM Spec Revision", NAFLG_FMT_NUMERIC, "%u", tpmInfo.SpecRevision);
	if (FormatTpmSpecDate(&tpmInfo, specDate, sizeof(specDate)))
		NWL_NodeAttrSet(node, "TPM Spec Date", specDate, 0);

	if (tpmInfo.AlgorithmCount != 0)
	{
		for (UINT32 i = 0; i < tpmInfo.AlgorithmCount; i++)
			NWL_NodeAppendMultiSz(&algorithms, tpmInfo.Algorithms[i].Name);
		if (algorithms != NULL)
			NWL_NodeAttrSetMulti(node, "TPM Algorithms", algorithms, 0);
	}

	free(algorithms);
	NWL_TPMFreeInfo(&tpmInfo);
}

PNODE NW_Mainboard(BOOL bAppend)
{
	PNODE node = NWL_NodeAlloc("Mainboard", 0);
	if (bAppend)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	NWLIB_MAINBOARD_INFO info = { 0 };
	if (!NWL_GetMainboardInfo(&info))
		return node;
	NWL_NodeAttrSet(node, "Manufacturer", info.VendorName, 0);
	NWL_NodeAttrSet(node, "Board Name", info.ProductStr, 0);
	NWL_NodeAttrSet(node, "Board Version", info.VersionStr, 0);
	NWL_NodeAttrSet(node, "Serial Number", info.SerialStr, NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(node, "BIOS Vendor", info.BiosVendor, 0);
	NWL_NodeAttrSet(node, "BIOS Version", info.BiosVersion, 0);
	NWL_NodeAttrSet(node, "BIOS Date", info.BiosDate, 0);
	NWL_NodeAttrSet(node, "System Manufacturer", info.SystemVendor, 0);
	NWL_NodeAttrSet(node, "System Product", info.SystemProduct, 0);
	NWL_NodeAttrSet(node, "System Version", info.SystemVersion, 0);
	NWL_NodeAttrSet(node, "System Serial Number", info.SystemSerial, NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(node, "System UUID", NWL_GuidToStr(info.SystemUuid), NAFLG_FMT_GUID | NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(node, "Enclosure Type", info.EnclosureType, 0);

	if (info.HostBridge.VendorId[0])
	{
		NWL_NodeAttrSet(node, "Host Bridge", info.HostBridge.Name, 0);
		NWL_NodeAttrSet(node, "Host Bridge VID", info.HostBridge.VendorId, 0);
		NWL_NodeAttrSet(node, "Host Bridge DID", info.HostBridge.DeviceId, 0);
		NWL_NodeAttrSet(node, "Host Bridge BDF", info.HostBridge.BDF, 0);
	}
	if (info.Smbus.VendorId[0])
	{
		NWL_NodeAttrSet(node, "SMBus", info.Smbus.Name, 0);
		NWL_NodeAttrSet(node, "SMBus VID", info.Smbus.VendorId, 0);
		NWL_NodeAttrSet(node, "SMBus DID", info.Smbus.DeviceId, 0);
		NWL_NodeAttrSet(node, "SMBus BDF", info.Smbus.BDF, 0);
	}
	if (info.IsaBridge.VendorId[0])
	{
		NWL_NodeAttrSet(node, "ISA Bridge", info.IsaBridge.Name, 0);
		NWL_NodeAttrSet(node, "ISA Bridge VID", info.IsaBridge.VendorId, 0);
		NWL_NodeAttrSet(node, "ISA Bridge DID", info.IsaBridge.DeviceId, 0);
		NWL_NodeAttrSet(node, "ISA Bridge BDF", info.IsaBridge.BDF, 0);
	}
	NWL_NodeAttrSet(node, "Chipset", info.Chipset, 0);

	PNWLIB_LPC lpc = NWL_InitLpc(&info);
	if (lpc)
	{
		for (enum LPCIO_CHIP_SLOT i = 0; i < LPCIO_SLOT_MAX; i++)
		{
			if (lpc->slots[i].chip == CHIP_UNKNOWN)
				continue;
			CHAR buf[5];
			snprintf(buf, sizeof(buf), "LPC%d", i);
			NWL_NodeAttrSet(node, buf, lpc->slots[i].name, 0);
		}
		NWL_FreeLpc(lpc);
	}

	GetTpmInfo(node);

	return node;
}
