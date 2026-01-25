// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>
#include <devpkey.h>

#include "libnw.h"
#include "utils.h"
#include "stb_ds.h"

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

static BOOL
MatchPciFilter(PNWL_ARG_SET pciFilters, const char* hwClass, const char* vendorId)
{
	BOOL hasVendor = FALSE;
	BOOL hasClass = FALSE;
	BOOL matchVendor = FALSE;
	BOOL matchClass = FALSE;

	if (!pciFilters)
		return TRUE;

	for (ptrdiff_t i = 0; i < hmlen(pciFilters); i++)
	{
		const char* token = pciFilters[i].key.Str;
		if (!token || !*token)
			continue;

		if ((token[0] == 'v' || token[0] == 'V') && token[1])
		{
			size_t vendorLen = strnlen_s(token + 1, 4);
			hasVendor = TRUE;
			if (vendorId && *vendorId && vendorLen == 4
				&& _strnicmp(token + 1, vendorId, vendorLen) == 0)
				matchVendor = TRUE;
		}
		else
		{
			size_t classLen = strnlen_s(token, 6);
			hasClass = TRUE;
			if (_strnicmp(token, hwClass, classLen) == 0)
				matchClass = TRUE;
		}
	}

	if (hasVendor && !matchVendor)
		return FALSE;
	if (hasClass && !matchClass)
		return FALSE;

	return TRUE;
}

PNODE NWL_EnumPci(PNODE pNode, PNWL_ARG_SET pciFilters)
{
	HDEVINFO hInfo = NULL;
	DWORD i = 0;
	SP_DEVINFO_DATA spData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
	DWORD dwFlags = DIGCF_PRESENT | DIGCF_ALLCLASSES;
	hInfo = SetupDiGetClassDevsW(NULL, L"PCI", NULL, dwFlags);
	if (hInfo == INVALID_HANDLE_VALUE)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "SetupDiGetClassDevs failed");
		goto fail;
	}
	for (i = 0; SetupDiEnumDeviceInfo(hInfo, i, &spData); i++)
	{
		CHAR hwClass[7] = { 0 };
		CHAR vendorId[5] = { 0 };
		PNODE npci;
		if (!SetupDiGetDeviceRegistryPropertyW(hInfo, &spData,
			SPDRP_HARDWAREID, NULL, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL))
			continue;
		for (LPCWSTR p = NWLC->NwBufW; p[0]; p += wcslen(p) + 1)
		{
			//PCI\VEN_XXXX&DEV_XXXX&CC_XXXXXX
			if (!vendorId[0])
			{
				LPCWSTR v = wcsstr(p, L"VEN_");
				if (v && v[4] && v[5] && v[6] && v[7])
				{
					for (int j = 0; j < 4; j++)
						vendorId[j] = (CHAR)v[4 + j];
					vendorId[4] = '\0';
				}
			}
			LPCWSTR s = wcsstr(p, L"&CC_");
			if (s != NULL)
			{
				strcpy_s(hwClass, sizeof(hwClass), NWL_Ucs2ToUtf8(s + 4));
				break;
			}
		}
		if (!MatchPciFilter(pciFilters, hwClass, vendorId))
			continue;
		npci = NWL_NodeAppendNew(pNode, "Device", NFLG_TABLE_ROW);

		NWL_NodeAttrSet(npci, "HWID", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);
		NWL_ParseHwid(npci, &NWLC->NwPciIds, NWLC->NwBufW, 0);

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
		NWL_FindClass(npci, &NWLC->NwPciIds, hwClass, 0);
	}
	SetupDiDestroyDeviceInfoList(hInfo);
fail:
	return pNode;
}

PNODE NW_Pci(BOOL bAppend)
{
	PNODE node = NWL_NodeAlloc("PCI", NFLG_TABLE);
	if (bAppend)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	return NWL_EnumPci(node, NWLC->PciFilters);
}
