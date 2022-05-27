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

PNODE NW_Usb(VOID)
{
	HDEVINFO Info = NULL;
	DWORD i = 0;
	SP_DEVINFO_DATA DeviceInfoData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
	DWORD Flags = DIGCF_PRESENT | DIGCF_ALLCLASSES;
	CHAR* Ids = NULL;
	DWORD IdsSize = 0;
	PNODE node = NWL_NodeAlloc("USB", NFLG_TABLE);
	if (NWLC->UsbInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	Ids = NWL_LoadFileToMemory("usb.ids", &IdsSize);
	Info = SetupDiGetClassDevsExA(NULL, "USB", NULL, Flags, NULL, NULL, NULL);
	if (Info == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "SetupDiGetClassDevs failed.\n");
		goto fail;
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
fail:
	free(Ids);
	return node;
}