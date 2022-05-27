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
	NWL_FindId(nd, Ids, IdsSize, VendorID, DeviceID, NULL, 1);
	return 1;
}

static void
ListUsb(PNODE node, CHAR* Ids, DWORD IdsSize)
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
			PNODE nusb = NWL_NodeAppendNew(node, "Device", NFLG_TABLE_ROW);
			NWL_NodeAttrSet(nusb, "HWID", BufferHw, 0);
			ParseHwid(nusb, Ids, IdsSize, BufferHw);
		}
		free(BufferHw);
	}
	SetupDiDestroyDeviceInfoList(Info);
}

PNODE NW_Usb(VOID)
{
	HANDLE Fp = INVALID_HANDLE_VALUE;
	CHAR* Ids = NULL;
	DWORD dwSize = 0;
	BOOL bRet = TRUE;
	CHAR* FilePath = NWLC->NwBuf;
	size_t i = 0;
	PNODE node = NWL_NodeAlloc("USB", NFLG_TABLE);
	if (NWLC->UsbInfo)
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
		ListUsb(node, Ids, dwSize);
	}
	else
		fprintf(stderr, "usb.ids read error\n");
	free(Ids);
	return node;
}