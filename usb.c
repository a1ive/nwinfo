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
	// USB\VID_XXXX&PID_XXXX
	// USB\ROOT_HUBXX&VIDXXXX&PIDXXXX
	if (!Hwid || strlen(Hwid) < 21)
		return 0;
	if (_strnicmp(Hwid, "USB\\VID_", 8) != 0)
	{
		CHAR* p = strstr(Hwid, "VID");
		if (p && strlen(p) > 7)
			snprintf(VendorID, 5, "%s", p + 3);
		else
			return 0;
	}
	else
		snprintf(VendorID, 5, "%s", Hwid + 8);
	if (_strnicmp(Hwid + 12, "&PID_", 5) != 0)
	{
		CHAR* p = strstr(Hwid, "PID");
		if (p && strlen(p) > 7)
			snprintf(DeviceID, 5, "%s", p + 3);
		else
			return 0;
	}
	else
		snprintf(DeviceID, 5, "%s", Hwid + 17);
	FindId(VendorID, DeviceID, NULL, 1);
	return 1;
}

static void
ListUsb(const GUID *Guid)
{
	HDEVINFO Info = NULL;
	DWORD i = 0, j = 0;
	SP_DEVINFO_DATA DeviceInfoData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
	CHAR BufferHw[256] = { 0 };
	DWORD Flags = DIGCF_PRESENT;
	if (!Guid)
		Flags |= DIGCF_ALLCLASSES;
	Info = SetupDiGetClassDevsW(Guid, L"USB", NULL, Flags);
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

void nwinfo_usb(const GUID *Guid)
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
	snprintf(FilePath, MAX_PATH, "%s\\usb.ids", FilePath);
	Fp = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (Fp == INVALID_HANDLE_VALUE)
	{
		printf("Cannot open %s\n", FilePath);
		return;
	}
	dwSize = GetFileSize(Fp, NULL);
	if (dwSize == INVALID_FILE_SIZE || dwSize == 0)
	{
		printf("bad usb.ids file\n");
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
		ListUsb(Guid);
	}
	else
		printf("usb.ids read error\n");
	free(IDS);
	IDS_SIZE = 0;
}