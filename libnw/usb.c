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
ParseHwClass(PNODE nd, CHAR* Ids, DWORD IdsSize, const CHAR* BufferHw)
{
	// USB\Class_XX&SubClass_XX&Prot_XX
	// USB\DevClass_XX&SubClass_XX&Prot_XX
	CHAR HwClass[7] = { 0 };
	size_t len = strlen(BufferHw);
	size_t ofs = 0;
	NWL_NodeAttrSet(nd, "Compatiable ID", BufferHw, 0);
	if (len >= 12 && strncmp(BufferHw, "USB\\Class_", 10) == 0)
		memcpy(HwClass, &BufferHw[10], 2);
	else if (len >= 12 + 3 && strncmp(BufferHw, "USB\\DevClass_", 10 + 3) == 0)
	{
		ofs = 3;
		memcpy(HwClass, &BufferHw[10 + ofs], 2);
	}
	else
		return;
	if (len >= 24 + ofs && strncmp(&BufferHw[12 + ofs], "&SubClass_", 10) == 0)
	{
		memcpy(&HwClass[2], &BufferHw[22 + ofs], 2);
		if (len >= 31 + ofs && strncmp(&BufferHw[24 + ofs], "&Prot_", 6) == 0)
			memcpy(&HwClass[4], &BufferHw[30 + ofs], 2);
	}
	//NWL_NodeAttrSet(nd, "USB Class", HwClass, 0);
	NWL_FindClass(nd, Ids, IdsSize, HwClass, 1);
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
		PNODE nusb = NULL;
		CHAR* BufferHw = NULL;
		DWORD BufferHwLen = 0;
		SetupDiGetDeviceRegistryPropertyA(Info, &DeviceInfoData,
			SPDRP_HARDWAREID, NULL, NULL, 0, &BufferHwLen);
		if (BufferHwLen == 0)
			continue;
		BufferHw = malloc(BufferHwLen);
		if (!BufferHw)
			continue;
		if (!SetupDiGetDeviceRegistryPropertyA(Info, &DeviceInfoData,
			SPDRP_HARDWAREID, NULL, BufferHw, BufferHwLen, NULL))
		{
			free(BufferHw);
			continue;
		}
		nusb = NWL_NodeAppendNew(node, "Device", NFLG_TABLE_ROW);
		NWL_NodeAttrSet(nusb, "HWID", BufferHw, 0);
		ParseHwid(nusb, Ids, IdsSize, BufferHw);
		free(BufferHw);
		SetupDiGetDeviceRegistryPropertyA(Info, &DeviceInfoData,
			SPDRP_COMPATIBLEIDS, NULL, NULL, 0, &BufferHwLen);
		if (BufferHwLen == 0)
			continue;
		BufferHw = malloc(BufferHwLen);
		if (!BufferHw)
			continue;
		if (SetupDiGetDeviceRegistryPropertyA(Info, &DeviceInfoData,
			SPDRP_COMPATIBLEIDS, NULL, BufferHw, BufferHwLen, NULL)
			&& BufferHw && BufferHw[0])
		{
			ParseHwClass(nusb, Ids, IdsSize, BufferHw);
		}
		free(BufferHw);
	}
	SetupDiDestroyDeviceInfoList(Info);
fail:
	free(Ids);
	return node;
}