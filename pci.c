// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>
#include "nwinfo.h"

static CHAR* PCI_IDS = NULL;
static DWORD PCI_IDS_SIZE = 0;

static CHAR *
IdsGetline(DWORD *Offset)
{
	CHAR* Line = NULL;
	DWORD i = 0, Len = 0;
	if (*Offset >= PCI_IDS_SIZE)
		return NULL;
	for (i = *Offset; i < PCI_IDS_SIZE; i++)
	{
		if (PCI_IDS[i] == '\n' || PCI_IDS[i] == '\r')
			break;
	}
	Len = i - *Offset;
	Line = malloc((SIZE_T)Len + 1);
	if (!Line)
		return NULL;
	memcpy(Line, PCI_IDS + *Offset, Len);
	Line[Len] = 0;
	*Offset += Len;
	for (i = *Offset; i < PCI_IDS_SIZE; i++, (*Offset)++)
	{
		if (PCI_IDS[i] != '\n' && PCI_IDS[i] != '\r')
			break;
	}
	return Line;
}

static void
FindId(CONST CHAR *v, CONST CHAR *d, CONST CHAR *s)
{
	DWORD Offset = 0;
	CHAR* vLine = NULL;
	CHAR* dLine = NULL;
	CHAR* sLine = NULL;
	if (!v || !d)
		return;
	vLine = IdsGetline(&Offset);
	while (vLine)
	{
		if (!vLine[0] || vLine[0] == '#' || strlen(vLine) < 7)
		{
			free(vLine);
			vLine = IdsGetline(&Offset);
			continue;
		}
		if (_strnicmp(v, vLine, 4) != 0)
		{
			free(vLine);
			vLine = IdsGetline(&Offset);
			continue;
		}
		printf("  Vendor: %s\n", vLine + 6);
		free(vLine);
		dLine = IdsGetline(&Offset);
		while (dLine)
		{
			if (!dLine[0] || dLine[0] == '#')
			{
				free(dLine);
				dLine = IdsGetline(&Offset);
				continue;
			}
			if (dLine[0] != '\t' || strlen(dLine) < 8)
			{
				free(dLine);
				break;
			}
			if (_strnicmp(d, dLine + 1, 4) != 0)
			{
				free(dLine);
				dLine = IdsGetline(&Offset);
				continue;
			}
			printf("  Device: %s\n", dLine + 7);
			free(dLine);
			if (!s)
				break;
			sLine = IdsGetline(&Offset);
			while (sLine)
			{
				if (!sLine[0] || sLine[0] == '#')
				{
					free(sLine);
					sLine = IdsGetline(&Offset);
					continue;
				}
				if (sLine[0] != '\t' || !sLine[1] || sLine[1] != '\t' || strlen(dLine) < 14)
				{
					free(sLine);
					break;
				}
				if (_strnicmp(s, sLine + 2, 9) != 0)
				{
					free(sLine);
					sLine = IdsGetline(&Offset);
					continue;
				}
				printf("  Subsys: %s\n", sLine + 13);
				free(sLine);
				break;
			}
			break;
		}
		break;
	}
}

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
		FindId(VendorID, DeviceID, NULL);
		return 1;
	}
	snprintf(Subsys, 5, "%s", Hwid + 29);
	snprintf(Subsys + 4, 6, " %s", Hwid + 33);
	FindId(VendorID, DeviceID, Subsys);
	return 1;
}

static void
ListPci(const GUID *Guid)
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
		ZeroMemory(BufferHw, sizeof(BufferHw));
		SetupDiGetDeviceRegistryPropertyA(Info, &DeviceInfoData, SPDRP_HARDWAREID, NULL, BufferHw, sizeof(BufferHw), NULL);
		printf("%s\n", BufferHw);
		ParseHwid(BufferHw);
	}
	SetupDiDestroyDeviceInfoList(Info);
}

void nwinfo_pci(const GUID *Guid)
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
	PCI_IDS = malloc(dwSize);
	if (!PCI_IDS)
	{
		printf("out of memory\n");
		CloseHandle(Fp);
		return;
	}
	PCI_IDS_SIZE = dwSize;
	bRet = ReadFile(Fp, PCI_IDS, dwSize, NULL, NULL);
	CloseHandle(Fp);
	if (bRet)
	{
		ListPci(Guid);
	}
	else
		printf("pci.ids read error\n");
	free(PCI_IDS);
	PCI_IDS_SIZE = 0;
}