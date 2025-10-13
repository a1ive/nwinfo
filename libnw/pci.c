// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>
#include <devpkey.h>

#include "libnw.h"
#include "utils.h"

static void
PrintDriverInfo(PNODE pNode, HDEVINFO hInfo, SP_DEVINFO_DATA* spData)
{
	FILETIME ft = { 0 };
	DEVPROPTYPE propType;
	if (SetupDiGetDevicePropertyW(hInfo, spData, &DEVPKEY_Device_DriverDesc, &propType, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL, 0)
		&& propType == DEVPROP_TYPE_STRING)
		NWL_NodeAttrSet(pNode, "Driver", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);
	if (SetupDiGetDevicePropertyW(hInfo, spData, &DEVPKEY_Device_DriverVersion, &propType, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL, 0)
		&& propType == DEVPROP_TYPE_STRING)
		NWL_NodeAttrSet(pNode, "Driver Version", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);
	if (SetupDiGetDevicePropertyW(hInfo, spData, &DEVPKEY_Device_DriverProvider, &propType, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL, 0)
		&& propType == DEVPROP_TYPE_STRING)
		NWL_NodeAttrSet(pNode, "Driver Provider", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);
	if (SetupDiGetDevicePropertyW(hInfo, spData, &DEVPKEY_Device_DriverDate, &propType, (PBYTE)&ft, sizeof(FILETIME), NULL, 0)
		&& propType == DEVPROP_TYPE_FILETIME)
	{
		SYSTEMTIME st;
		if (FileTimeToSystemTime(&ft, &st))
			NWL_NodeAttrSetf(pNode, "Driver Date", 0, "%u-%02u-%02u", st.wYear, st.wMonth, st.wDay);
	}
	if (SetupDiGetDevicePropertyW(hInfo, spData, &DEVPKEY_Device_DriverInfPath, &propType, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL, 0)
		&& propType == DEVPROP_TYPE_STRING)
		NWL_NodeAttrSet(pNode, "Inf Path", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);
	if (SetupDiGetDevicePropertyW(hInfo, spData, &DEVPKEY_Device_DriverInfSection, &propType, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL, 0)
		&& propType == DEVPROP_TYPE_STRING)
		NWL_NodeAttrSet(pNode, "Inf Section", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);
}

PNODE NWL_EnumPci(PNODE pNode, LPCSTR pciClass)
{
	HDEVINFO hInfo = NULL;
	DWORD i = 0;
	SP_DEVINFO_DATA spData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
	DWORD dwFlags = DIGCF_PRESENT | DIGCF_ALLCLASSES;
	DWORD idsSize = 0;
	CHAR* ids = NWL_LoadIdsToMemory(L"pci.ids", &idsSize);
	hInfo = SetupDiGetClassDevsW(NULL, L"PCI", NULL, dwFlags);
	if (hInfo == INVALID_HANDLE_VALUE)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "SetupDiGetClassDevs failed");
		goto fail;
	}
	for (i = 0; SetupDiEnumDeviceInfo(hInfo, i, &spData); i++)
	{
		CHAR hwClass[7] = { 0 };
		PNODE npci;
		if (!SetupDiGetDeviceRegistryPropertyW(hInfo, &spData,
			SPDRP_HARDWAREID, NULL, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL))
			continue;
		for (LPCWSTR p = NWLC->NwBufW; p[0]; p += wcslen(p) + 1)
		{
			//PCI\VEN_XXXX&DEV_XXXX&CC_XXXXXX
			LPCWSTR s = wcsstr(p, L"&CC_");
			if (s != NULL)
			{
				snprintf(hwClass, 7, "%ls", s + 4);
				break;
			}
		}
		if (pciClass)
		{
			size_t PciClassLen = strlen(pciClass);
			if (PciClassLen > 6)
				PciClassLen = 6;
			if (_strnicmp(pciClass, hwClass, PciClassLen) != 0)
				continue;
		}
		npci = NWL_NodeAppendNew(pNode, "Device", NFLG_TABLE_ROW);

		NWL_NodeAttrSet(npci, "HWID", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);
		NWL_ParseHwid(npci, ids, idsSize, NWLC->NwBufW, 0);

		if (SetupDiGetDeviceRegistryPropertyW(hInfo, &spData,
			SPDRP_DEVICEDESC, NULL, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL))
			NWL_NodeAttrSet(npci, "Description", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);

		PrintDriverInfo(npci, hInfo, &spData);

		ULONG busNum = 0, devFunc = 0;
		SetupDiGetDeviceRegistryPropertyW(hInfo, &spData,
			SPDRP_BUSNUMBER, NULL, (PBYTE)&busNum, sizeof(busNum), NULL);
		SetupDiGetDeviceRegistryPropertyW(hInfo, &spData,
			SPDRP_ADDRESS, NULL, (PBYTE)&devFunc, sizeof(devFunc), NULL);
		NWL_NodeAttrSetf(npci, "Location", NAFLG_FMT_NEED_QUOTE, "Bus %u, Device %u, Function %u",
			busNum & 0xFF, devFunc >> 16, devFunc & 0xFFFF);

		NWL_NodeAttrSet(npci, "Class Code", hwClass, 0);
		NWL_FindClass(npci, ids, idsSize, hwClass, 0);
	}
	SetupDiDestroyDeviceInfoList(hInfo);
fail:
	free(ids);
	return pNode;
}

PNODE NW_Pci(VOID)
{
	PNODE node = NWL_NodeAlloc("PCI", NFLG_TABLE);
	if (NWLC->PciInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	return NWL_EnumPci(node, NWLC->PciClass);
}
