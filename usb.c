// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>
#include "nwinfo.h"

static int
ParseHwid(PNODE nd, const CHAR *Hwid)
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
	FindId(nd, VendorID, DeviceID, NULL, 1);
	return 1;
}

static void
ListUsb(PNODE node)
{
	HDEVINFO Info = NULL;
	DWORD i = 0;
	SP_DEVINFO_DATA DeviceInfoData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
	DWORD Flags = DIGCF_PRESENT | DIGCF_ALLCLASSES;
	Info = SetupDiGetClassDevsExA(NULL, "USB", NULL, Flags, NULL, NULL, NULL);
	if (Info == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "SetupDiGetClassDevs failed.\n");
		return;
	}
	for (i = 0; SetupDiEnumDeviceInfo(Info, i, &DeviceInfoData); i++)
	{
		CHAR* BufferHw = NULL;
		DWORD BufferHwLen = 0;
		SetupDiGetDeviceRegistryPropertyA(Info, &DeviceInfoData, SPDRP_HARDWAREID, NULL, NULL, 0, &BufferHwLen);
		if (BufferHwLen == 0)
			continue;
		BufferHw = malloc(BufferHwLen);
		if (!BufferHw)
			continue;
		if (SetupDiGetDeviceRegistryPropertyA(Info, &DeviceInfoData, SPDRP_HARDWAREID, NULL, BufferHw, BufferHwLen, NULL))
		{
			PNODE nusb = node_append_new(node, "Device", NFLG_TABLE_ROW);
			node_att_set(nusb, "HWID", BufferHw, 0);
			ParseHwid(nusb, BufferHw);
		}
		free(BufferHw);
	}
	SetupDiDestroyDeviceInfoList(Info);
}

PNODE nwinfo_usb(void)
{
	HANDLE Fp = INVALID_HANDLE_VALUE;
	DWORD dwSize = 0;
	BOOL bRet = TRUE;
	CHAR* FilePath = nwinfo_buffer;
	size_t i = 0;
	PNODE node = node_alloc("USB", NFLG_TABLE);
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
	snprintf(FilePath, MAX_PATH, "%s\\usb.ids", FilePath);
	Fp = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (Fp == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Cannot open %s\n", FilePath);
		return node;
	}
	dwSize = GetFileSize(Fp, NULL);
	if (dwSize == INVALID_FILE_SIZE || dwSize == 0)
	{
		fprintf(stderr, "bad usb.ids file\n");
		CloseHandle(Fp);
		return node;
	}
	IDS = malloc(dwSize);
	if (!IDS)
	{
		fprintf(stderr, "out of memory\n");
		CloseHandle(Fp);
		return node;
	}
	IDS_SIZE = dwSize;
	bRet = ReadFile(Fp, IDS, dwSize, &dwSize, NULL);
	CloseHandle(Fp);
	if (bRet)
	{
		ListUsb(node);
	}
	else
		fprintf(stderr, "usb.ids read error\n");
	free(IDS);
	IDS_SIZE = 0;
	return node;
}