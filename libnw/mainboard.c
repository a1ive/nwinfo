// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MB_VENDOR_IMPL
#include "libnw.h"
#include "utils.h"
#include "smbios.h"

#define CHIPSET_IDS_IMPL
#include "chipset_ids.h"

static PBoardInfo
GetDMIType2(PNODE node)
{
	LPBYTE p = (LPBYTE)NWLC->NwSmbios->Data;
	const LPBYTE lastAddress = p + NWLC->NwSmbios->Length;
	PBoardInfo pInfo;
	PBoardInfo ret = NULL;
	PNWL_ARG_SET dmiSet = NULL;
	NWL_ArgSetAddU64(&dmiSet, 2);
	while ((pInfo = (PBoardInfo)NWL_GetNextDmiTable(&p, lastAddress, dmiSet)) != NULL)
	{
		if (pInfo->Header.Length >= 0x0E && pInfo->Type != 0x0A)
			continue;
		ret = pInfo;
		break;
	}
	NWL_ArgSetFree(dmiSet);
	return ret;
}

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

LIBNW_API enum DMI_VENDOR_ID
NWL_GetBoardVendor(VOID)
{
	if (!NWLC->NwSmbios)
		return VENDOR_UNKNOWN;
	PBoardInfo pBoard = GetDMIType2(NULL);
	if (!pBoard)
		return VENDOR_UNKNOWN;
	return GetBoardVendor(NWL_GetDmiString((UINT8*)pBoard, pBoard->Manufacturer));
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

static LPCSTR
GetChipsetInfo(PNODE node, LPCSTR board)
{
	LPCSTR chipset = NULL;
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

	LPCSTR vid = NULL;
	LPCSTR did = NULL;
	if (pciHost)
	{
		vid = NWL_NodeAttrGet(pciHost, "Vendor ID");
		NWL_NodeAttrSet(node, "Host Bridge", GetPciDeviceName(pciHost), 0);
		NWL_NodeAttrSet(node, "Host Bridge VID", vid, 0);
		NWL_NodeAttrSet(node, "Host Bridge DID", NWL_NodeAttrGet(pciHost, "Device ID"), 0);
		NWL_NodeAttrSet(node, "Host Bridge BDF", NWL_NodeAttrGet(pciHost, "BDF"), 0);
	}
	if (pciSmbus)
	{
		vid = NWL_NodeAttrGet(pciSmbus, "Vendor ID");
		NWL_NodeAttrSet(node, "SMBus", GetPciDeviceName(pciSmbus), 0);
		NWL_NodeAttrSet(node, "SMBus VID", vid, 0);
		NWL_NodeAttrSet(node, "SMBus DID", NWL_NodeAttrGet(pciSmbus, "Device ID"), 0);
		NWL_NodeAttrSet(node, "SMBus BDF", NWL_NodeAttrGet(pciSmbus, "BDF"), 0);
	}
	if (pciIsa)
	{
		vid = NWL_NodeAttrGet(pciIsa, "Vendor ID");
		did = NWL_NodeAttrGet(pciIsa, "Device ID");
		NWL_NodeAttrSet(node, "ISA Bridge", GetPciDeviceName(pciIsa), 0);
		NWL_NodeAttrSet(node, "ISA Bridge VID", vid, 0);
		NWL_NodeAttrSet(node, "ISA Bridge DID", did, 0);
		NWL_NodeAttrSet(node, "ISA Bridge BDF", NWL_NodeAttrGet(pciIsa, "BDF"), 0);
	}

	if (vid == NULL)
	{
		chipset = "Unknown";
	}
	else if (strcmp(vid, "8086") == 0) // Intel
	{
#if 1
		if (pciIsa)
		{
			for (size_t i = 0; i < ARRAYSIZE(INTEL_ISA_LIST); i++)
			{
				if (strcmp(did, INTEL_ISA_LIST[i].id) == 0)
				{
					chipset = INTEL_ISA_LIST[i].name;
					break;
				}
			}
		}
#endif
		if (chipset == NULL)
		{
			for (size_t i = 0; i < ARRAYSIZE(INTEL_CHIPSET_LIST); i++)
			{
				if (strstr(board, INTEL_CHIPSET_LIST[i]) != NULL)
				{
					chipset = INTEL_CHIPSET_LIST[i];
					break;
				}
			}
		}
		if (chipset == NULL)
			chipset = "INTEL";
	}
	else if (strcmp(vid, "1022") == 0 || strcmp(vid, "1002") == 0) // AMD/ATI
	{
		for (size_t i = 0; i < ARRAYSIZE(AMD_CHIPSET_LIST); i++)
		{
			if (strstr(board, AMD_CHIPSET_LIST[i]) != NULL)
			{
				chipset = AMD_CHIPSET_LIST[i];
				break;
			}
		}
		if (chipset == NULL)
			chipset = "AMD";
	}
	else if (strcmp(vid, "1039") == 0) // SiS
	{
		chipset = "SiS";
	}
	else if (strcmp(vid, "10B9") == 0) // ULi
	{
		chipset = "ULi";
	}
	else if (strcmp(vid, "10DE") == 0) // NVIDIA
	{
		chipset = "NVIDIA";
	}
	else if (strcmp(vid, "1106") == 0) // VIA
	{
		chipset = "VIA";
	}
	else if (strcmp(vid, "1D17") == 0) // Zhaoxin
	{
		chipset = "Zhaoxin";
	}
	else if (strcmp(vid, "1D94") == 0) // Hygon
	{
		chipset = "Hygon";
	}
	else
	{
		chipset = "Unknown";
	}

	NWL_ArgSetFree(pciSet);
	NWL_NodeFree(root, TRUE);
	return chipset;
}

PNODE NW_Mainboard(BOOL bAppend)
{
	PNODE node = NWL_NodeAlloc("Mainboard", NFLG_TABLE);
	if (bAppend)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	PBoardInfo pBoard = GetDMIType2(node);
	if (!pBoard)
		return node;
	const char* vendorStr = NWL_GetDmiString((UINT8*)pBoard, pBoard->Manufacturer);
	const char* productStr = NWL_GetDmiString((UINT8*)pBoard, pBoard->Product);
	enum DMI_VENDOR_ID vendorId = NWL_GetBoardVendor();
	if (vendorId == VENDOR_UNKNOWN)
		NWL_NodeAttrSet(node, "Manufacturer", vendorStr, 0);
	else
		NWL_NodeAttrSet(node, "Manufacturer", DMI_VENDOR_NAME[vendorId], 0);

	NWL_NodeAttrSet(node, "Board Name", productStr, 0);
	NWL_NodeAttrSet(node, "Board Version", NWL_GetDmiString((UINT8*)pBoard, pBoard->Version), 0);
	NWL_NodeAttrSet(node, "Serial Number", NWL_GetDmiString((UINT8*)pBoard, pBoard->SN), NAFLG_FMT_SENSITIVE);

	const char* chipset = GetChipsetInfo(node, productStr);
	NWL_NodeAttrSet(node, "Chipset", chipset, 0);

	return node;
}
