// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>

#include "libnw.h"
#include "utils.h"

static int
ParseHwid(PNODE nd, CHAR* Ids, DWORD IdsSize, const CHAR *Hwid)
{
	CHAR VendorID[5] = { 0 };
	CHAR DeviceID[5] = { 0 };
	CHAR Subsys[10] = { 0 };
	// PCI\VEN_XXXX&DEV_XXXX
	if (!Hwid || strlen(Hwid) < 21)
		return 0;
	if (_strnicmp(Hwid, "PCI\\VEN_", 8) != 0)
		return 0;
	snprintf(VendorID, 5, "%s", Hwid + 8);
	if (_strnicmp(Hwid + 12, "&DEV_", 5) != 0)
		return 0;
	snprintf(DeviceID, 5, "%s", Hwid + 17);
	// PCI\VEN_XXXX&DEV_XXXX&SUBSYS_XXXXXXXX
	if (strlen(Hwid) < 37 || _strnicmp(Hwid + 21, "&SUBSYS_", 8) != 0)
	{
		NWL_FindId(nd, Ids, IdsSize, VendorID, DeviceID, NULL, 0);
		return 1;
	}
	snprintf(Subsys, 5, "%s", Hwid + 29);
	snprintf(Subsys + 4, 6, " %s", Hwid + 33);
	NWL_FindId(nd, Ids, IdsSize, VendorID, DeviceID, Subsys, 0);
	return 1;
}

PNODE NW_Pci(VOID)
{
	HDEVINFO Info = NULL;
	DWORD i = 0;
	SP_DEVINFO_DATA DeviceInfoData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
	DWORD Flags = DIGCF_PRESENT | DIGCF_ALLCLASSES;
	CHAR* Ids = NULL;
	DWORD IdsSize = 0;
	PNODE node = NWL_NodeAlloc("PCI", NFLG_TABLE);
	if (NWLC->PciInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	Ids = NWL_LoadIdsToMemory("pci.ids", &IdsSize);
	Info = SetupDiGetClassDevsExA(NULL, "PCI", NULL, Flags, NULL, NULL, NULL);
	if (Info == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "SetupDiGetClassDevs failed.\n");
		goto fail;
	}
	for (i = 0; SetupDiEnumDeviceInfo(Info, i, &DeviceInfoData); i++)
	{
		CHAR* BufferHw = NULL;
		DWORD BufferHwLen = 0;
		CHAR* p = NULL;
		size_t pLen = 0;
		CHAR HwClass[7] = { 0 };
		PNODE npci;
		SetupDiGetDeviceRegistryPropertyA(Info, &DeviceInfoData, SPDRP_HARDWAREID, NULL, NULL, 0, &BufferHwLen);
		if (BufferHwLen == 0)
			continue;
		BufferHw = malloc(BufferHwLen);
		if (!BufferHw)
			continue;
		p = BufferHw;
		if (!SetupDiGetDeviceRegistryPropertyA(Info, &DeviceInfoData, SPDRP_HARDWAREID, NULL, BufferHw, BufferHwLen, NULL))
			goto next_device;
		while (p[0])
		{
			pLen = strlen(p);
			//PCI\VEN_XXXX&DEV_XXXX&CC_XXXXXX
			if (pLen >= 29 && _strnicmp(p + 21, "&CC_", 4) == 0) {
				snprintf(HwClass, 7, "%s", p + 25);
				break;
			}
			p += pLen + 1;
		}
		if (NWLC->PciClass)
		{
			size_t PciClassLen = strlen(NWLC->PciClass);
			if (PciClassLen > 6)
				PciClassLen = 6;
			if (_strnicmp(NWLC->PciClass, HwClass, PciClassLen) != 0)
				goto next_device;
		}
		npci = NWL_NodeAppendNew(node, "Device", NFLG_TABLE_ROW);
		NWL_NodeAttrSet(npci, "HWID", BufferHw, 0);
		NWL_FindClass(npci, Ids, IdsSize, HwClass, 0);
		ParseHwid(npci, Ids, IdsSize, BufferHw);
	next_device:
		free(BufferHw);
	}
	SetupDiDestroyDeviceInfoList(Info);
fail:
	free(Ids);
	return node;
}
