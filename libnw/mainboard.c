// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libnw.h"
#include "utils.h"
#include "smbios.h"

#define MB_VENDOR_IMPL
#include "mb_vendor.h"
#define CHIPSET_IDS_IMPL
#include "chipset_ids.h"

#include "chip_ids.h"

#include "lpc/lpc.h"

static PBoardInfo
GetDMIType2(void)
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

BOOL NWL_GetMainboardInfo(NWLIB_MAINBOARD_INFO* info)
{
	if (info == NULL || NWLC->NwSmbios == NULL)
		return FALSE;

	PBoardInfo pBoard = GetDMIType2();
	if (pBoard == NULL)
		return FALSE;

	info->VendorStr = NWL_GetDmiString((UINT8*)pBoard, pBoard->Manufacturer);
	info->ProductStr = NWL_GetDmiString((UINT8*)pBoard, pBoard->Product);
	info->VersionStr = NWL_GetDmiString((UINT8*)pBoard, pBoard->Version);
	info->SerialStr = NWL_GetDmiString((UINT8*)pBoard, pBoard->SN);
	info->VendorId = GetBoardVendor(info->VendorStr);
	if (info->VendorId == VENDOR_UNKNOWN)
		info->VendorName = info->VendorStr;
	else
		info->VendorName = DMI_VENDOR_NAME[info->VendorId];

	GetChipsetInfo(info);

	return TRUE;
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

	return node;
}
