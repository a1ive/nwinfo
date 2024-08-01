// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>

#include "libnw.h"
#include "utils.h"

PNODE NWL_EnumPci(PNODE pNode, LPCSTR pciClass)
{
	HDEVINFO Info = NULL;
	DWORD i = 0;
	SP_DEVINFO_DATA DeviceInfoData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
	DWORD Flags = DIGCF_PRESENT | DIGCF_ALLCLASSES;
	DWORD IdsSize = 0;
	CHAR* Ids = NWL_LoadIdsToMemory(L"pci.ids", &IdsSize);
	Info = SetupDiGetClassDevsW(NULL, L"PCI", NULL, Flags);
	if (Info == INVALID_HANDLE_VALUE)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "SetupDiGetClassDevs failed");
		goto fail;
	}
	for (i = 0; SetupDiEnumDeviceInfo(Info, i, &DeviceInfoData); i++)
	{
		CHAR HwClass[7] = { 0 };
		PNODE npci;
		if (!SetupDiGetDeviceRegistryPropertyW(Info, &DeviceInfoData,
			SPDRP_HARDWAREID, NULL, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL))
			continue;
		for (LPCWSTR p = NWLC->NwBufW; p[0]; p += wcslen(p) + 1)
		{
			//PCI\VEN_XXXX&DEV_XXXX&CC_XXXXXX
			LPCWSTR s = wcsstr(p, L"&CC_");
			if (s != NULL)
			{
				snprintf(HwClass, 7, "%ls", s + 4);
				break;
			}
		}
		if (pciClass)
		{
			size_t PciClassLen = strlen(pciClass);
			if (PciClassLen > 6)
				PciClassLen = 6;
			if (_strnicmp(pciClass, HwClass, PciClassLen) != 0)
				continue;
		}
		npci = NWL_NodeAppendNew(pNode, "Device", NFLG_TABLE_ROW);
		NWL_NodeAttrSet(npci, "HWID", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);
		NWL_NodeAttrSet(npci, "Class Code", HwClass, 0);
		NWL_FindClass(npci, Ids, IdsSize, HwClass, 0);
		NWL_ParseHwid(npci, Ids, IdsSize, NWLC->NwBufW, 0);
	}
	SetupDiDestroyDeviceInfoList(Info);
fail:
	free(Ids);
	return pNode;
}

PNODE NW_Pci(VOID)
{
	PNODE node = NWL_NodeAlloc("PCI", NFLG_TABLE);
	if (NWLC->PciInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	return NWL_EnumPci(node, NWLC->PciClass);
}
