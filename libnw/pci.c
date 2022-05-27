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

static void
ListPci(PNODE node, CHAR* Ids, DWORD IdsSize, const CHAR* PciClass)
{
	HDEVINFO Info = NULL;
	DWORD i = 0;
	SP_DEVINFO_DATA DeviceInfoData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
	DWORD Flags = DIGCF_PRESENT | DIGCF_ALLCLASSES;
	Info = SetupDiGetClassDevsExA(NULL, "PCI", NULL, Flags, NULL, NULL, NULL);
	if (Info == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "SetupDiGetClassDevs failed.\n");
		return;
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
		if (PciClass)
		{
			size_t PciClassLen = strlen(PciClass);
			if (PciClassLen > 6)
				PciClassLen = 6;
			if (_strnicmp(PciClass, HwClass, PciClassLen) != 0)
				goto next_device;
		}
		npci = NWL_NodeAppendNew(node, "Device", NFLG_TABLE_ROW);
		NWL_NodeAttrSet(npci, "HWID", BufferHw, 0);
		NWL_FindClass(npci, Ids, IdsSize, HwClass);
		ParseHwid(npci, Ids, IdsSize, BufferHw);
next_device:
		free(BufferHw);
	}
	SetupDiDestroyDeviceInfoList(Info);
}

PNODE NW_Pci(VOID)
{
	HANDLE Fp = INVALID_HANDLE_VALUE;
	CHAR* Ids = NULL;
	DWORD dwSize = 0;
	BOOL bRet = TRUE;
	CHAR* FilePath = NWLC->NwBuf;
	size_t i = 0;
	PNODE node = NWL_NodeAlloc("PCI", NFLG_TABLE);
	if (NWLC->PciInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	if (!GetModuleFileNameA(NULL, FilePath, MAX_PATH) || strlen(FilePath) == 0)
	{
		fprintf(stderr, "GetModuleFileName failed\n");
		return node;
	}
	for (i = strlen(FilePath); i > 0; i--)
	{
		if (FilePath[i] == '\\')
		{
			FilePath[i] = 0;
			break;
		}
	}
	snprintf(FilePath, MAX_PATH,"%s\\pci.ids", FilePath);
	Fp = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (Fp == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Cannot open %s\n", FilePath);
		return node;
	}
	dwSize = GetFileSize(Fp, NULL);
	if (dwSize == INVALID_FILE_SIZE || dwSize == 0)
	{
		fprintf(stderr, "bad pci.ids file\n");
		CloseHandle(Fp);
		return node;
	}
	Ids = malloc(dwSize);
	if (!Ids)
	{
		fprintf(stderr, "out of memory\n");
		CloseHandle(Fp);
		return node;
	}
	bRet = ReadFile(Fp, Ids, dwSize, &dwSize, NULL);
	CloseHandle(Fp);
	if (bRet)
	{
		ListPci(node, Ids, dwSize, NWLC->PciClass);
	}
	else
		fprintf(stderr, "pci.ids read error\n");
	free(Ids);
	return node;
}