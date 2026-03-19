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

static void
ConvertPciPath(const char* path, char* buf, size_t bufSize)
{
	size_t off = 0;
	buf[0] = '\0';
	while (*path)
	{
		const char* sep = strchr(path, '#');
		size_t len = sep ? (size_t)(sep - path) : strlen(path);
		unsigned int val;
		if (sscanf_s(path, "PCIROOT(%u)", &val) == 1)
			off += snprintf(buf + off, bufSize - off, "%sPciRoot(0x%x)", off ? "/" : "", val);
		else if (sscanf_s(path, "PCI(%4x)", &val) == 1 && len >= 8)
		{
			unsigned int dev = val & 0xFF;
			unsigned int func = (val >> 8) & 0xFF;
			off += snprintf(buf + off, bufSize - off, "%sPci(0x%x,0x%x)", off ? "/" : "", func, dev);
		}
		if (!sep)
			break;
		path = sep + 1;
	}
}

static void
ConvertAcpiPath(const char* path, char* buf, size_t bufSize)
{
	size_t off = 0;
	BOOL first = TRUE;
	buf[0] = '\0';
	while (*path)
	{
		const char* sep = strchr(path, '#');
		size_t len = sep ? (size_t)(sep - path) : strlen(path);
		char name[16] = { 0 };
		if (sscanf_s(path, "ACPI(%15[^)])", name, (unsigned)sizeof(name)) == 1)
		{
			if (first)
			{
				size_t nlen = strnlen_s(name, sizeof(name));
				if (nlen > 0)
					name[nlen - 1] = '\0';
				off += snprintf(buf + off, bufSize - off, "\\%s", name);
				first = FALSE;
			}
			else
				off += snprintf(buf + off, bufSize - off, ".%s", name);
		}
		(void)len;
		if (!sep)
			break;
		path = sep + 1;
	}
}

static void
PrintLocationPaths(PNODE pNode, HDEVINFO hInfo, SP_DEVINFO_DATA* spData)
{
	char convBuf[512];
	if (!SetupDiGetDeviceRegistryPropertyW(hInfo, spData,
		SPDRP_LOCATION_PATHS, NULL, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL))
		return;
	for (LPCWSTR p = NWLC->NwBufW; p[0]; p += wcslen(p) + 1)
	{
		const char* path = NWL_Ucs2ToUtf8(p);
		if (_strnicmp(path, "PCIROOT(", 8) == 0 || _strnicmp(path, "PCI(", 4) == 0)
		{
			NWL_NodeAttrSet(pNode, "PCI Path", path, NAFLG_FMT_NEED_QUOTE);
			ConvertPciPath(path, convBuf, sizeof(convBuf));
			NWL_NodeAttrSet(pNode, "UEFI Device Path", convBuf, NAFLG_FMT_NEED_QUOTE);
		}
		else if (_strnicmp(path, "ACPI(", 5) == 0)
		{
			NWL_NodeAttrSet(pNode, "ACPI Path", path, NAFLG_FMT_NEED_QUOTE);
			ConvertAcpiPath(path, convBuf, sizeof(convBuf));
			NWL_NodeAttrSet(pNode, "ASL Path", convBuf, NAFLG_FMT_NEED_QUOTE);
		}
	}
}

static BOOL
MatchPciClass(PNWL_ARG_SET pciClasses, const char* hwClass)
{
	if (!pciClasses)
		return TRUE;

	for (ptrdiff_t i = 0; i < hmlen(pciClasses); i++)
	{
		const char* pciClass = pciClasses[i].key.Str;
		size_t classLen = strnlen_s(pciClass, 6);

		if (_strnicmp(pciClass, hwClass, classLen) == 0)
			return TRUE;
	}

	return FALSE;
}

PNODE NWL_EnumPci(PNODE pNode, PNWL_ARG_SET pciClasses)
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
				strncpy_s(hwClass, sizeof(hwClass), NWL_Ucs2ToUtf8(s + 4), _TRUNCATE);
				break;
			}
		}
		if (!MatchPciClass(pciClasses, hwClass))
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

		if (SetupDiGetDeviceRegistryPropertyW(hInfo, &spData,
			SPDRP_LOCATION_INFORMATION, NULL, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL))
			NWL_NodeAttrSet(npci, "Location", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);
		else
			NWL_NodeAttrSetf(npci, "Location", NAFLG_FMT_NEED_QUOTE, "Bus %u, Device %u, Function %u",
				busNum & 0xFF, (devFunc >> 16) & 0x1F, devFunc & 0x07);

		NWL_NodeAttrSetf(npci, "BDF", NAFLG_FMT_NEED_QUOTE, "%02X:%02X.%u", busNum & 0xFF, (devFunc >> 16) & 0x1F, devFunc & 0x07);

		if (SetupDiGetDeviceRegistryPropertyW(hInfo, &spData,
			SPDRP_MFG, NULL, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL))
			NWL_NodeAttrSet(npci, "MFG", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);

		if (SetupDiGetDeviceRegistryPropertyW(hInfo, &spData,
			SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, NULL, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL))
			NWL_NodeAttrSet(npci, "PDO", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);

		NWL_NodeAttrSet(npci, "Class Code", hwClass, 0);
		NWL_FindClass(npci, &NWLC->NwPciIds, hwClass, 0);

		PrintLocationPaths(npci, hInfo, &spData);
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
	return NWL_EnumPci(node, NWLC->PciClasses);
}
