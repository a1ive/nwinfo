// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>
#include "nwinfo.h"

static int
ParseHwid(const CHAR *Hwid)
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
		FindId(VendorID, DeviceID, NULL, 0);
		return 1;
	}
	snprintf(Subsys, 5, "%s", Hwid + 29);
	snprintf(Subsys + 4, 6, " %s", Hwid + 33);
	FindId(VendorID, DeviceID, Subsys, 0);
	return 1;
}

static void
ListPci(const GUID *Guid, const CHAR* PciClass)
{
	HDEVINFO Info = NULL;
	DWORD i = 0, j = 0;
	SP_DEVINFO_DATA DeviceInfoData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
	CHAR BufferHw[256] = { 0 };
	DWORD Flags = DIGCF_PRESENT;
	if (!Guid)
		Flags |= DIGCF_ALLCLASSES;
	Info = SetupDiGetClassDevsW(Guid, L"PCI", NULL, Flags);
	if (Info == INVALID_HANDLE_VALUE)
	{
		printf("SetupDiGetClassDevs failed.\n");
		return;
	}
	for (i = 0; SetupDiEnumDeviceInfo(Info, i, &DeviceInfoData); i++)
	{
		CHAR* p = BufferHw;
		size_t pLen = 0;
		ZeroMemory(BufferHw, sizeof(BufferHw));
		SetupDiGetDeviceRegistryPropertyA(Info, &DeviceInfoData, SPDRP_HARDWAREID, NULL, BufferHw, sizeof(BufferHw), NULL);
		if (!PciClass) {
			printf("%s\n", BufferHw);
			ParseHwid(BufferHw);
		}
		else {
			CHAR HwClass[11] = { 0 };
			snprintf(HwClass, 11, "&CC_%s", PciClass);
			while (p[0])
			{
				pLen = strlen(p) + 1;
				//PCI\VEN_XXXX&DEV_XXXX&CC_XXXXXX
				//printf("%s\n", p);
				if (pLen > 26 && _strnicmp(p + 21, HwClass, strlen(HwClass)) == 0) {
					printf("%s\n", BufferHw);
					ParseHwid(BufferHw);
					break;
				}
				p += pLen;
			}
		}
	}
	SetupDiDestroyDeviceInfoList(Info);
}

void nwinfo_pci(const GUID *Guid, const CHAR *PciClass)
{
	HANDLE Fp = INVALID_HANDLE_VALUE;
	DWORD dwSize = 0;
	BOOL bRet = TRUE;
	CHAR FilePath[MAX_PATH] = { 0 };
	size_t i = 0;
	if (!GetModuleFileNameA(NULL, FilePath, MAX_PATH) || strlen(FilePath) == 0)
	{
		printf("GetModuleFileName failed\n");
		return;
	}
	for (i = strlen(FilePath); i > 0; i--)
	{
		if (FilePath[i] == '\\')
		{
			FilePath[i] = 0;
			break;
		}
	}
	snprintf(FilePath, MAX_PATH, "%s\\pci.ids", FilePath);
	Fp = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (Fp == INVALID_HANDLE_VALUE)
	{
		printf("Cannot open %s\n", FilePath);
		return;
	}
	dwSize = GetFileSize(Fp, NULL);
	if (dwSize == INVALID_FILE_SIZE || dwSize == 0)
	{
		printf("bad pci.ids file\n");
		CloseHandle(Fp);
		return;
	}
	IDS = malloc(dwSize);
	if (!IDS)
	{
		printf("out of memory\n");
		CloseHandle(Fp);
		return;
	}
	IDS_SIZE = dwSize;
	bRet = ReadFile(Fp, IDS, dwSize, NULL, NULL);
	CloseHandle(Fp);
	if (bRet)
	{
		ListPci(Guid, PciClass);
	}
	else
		printf("pci.ids read error\n");
	free(IDS);
	IDS_SIZE = 0;
}