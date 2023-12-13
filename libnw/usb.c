// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>

#include "libnw.h"
#include "utils.h"

static void
ParseHwClass(PNODE nd, CHAR* Ids, DWORD IdsSize, LPCSTR BufferHw)
{
	// USB\Class_XX&SubClass_XX&Prot_XX
	// USB\DevClass_XX&SubClass_XX&Prot_XX
	CHAR HwClass[7] = { 0 };
	size_t len = strlen(BufferHw);
	size_t ofs = 0;
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
	NWL_NodeAttrSet(nd, "Class Code", HwClass, 0);
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
	Ids = NWL_LoadIdsToMemory(L"usb.ids", &IdsSize);
	Info = SetupDiGetClassDevsW(NULL, L"USB", NULL, Flags);
	if (Info == INVALID_HANDLE_VALUE)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "SetupDiGetClassDevs failed");
		goto fail;
	}
	for (i = 0; SetupDiEnumDeviceInfo(Info, i, &DeviceInfoData); i++)
	{
		PNODE nusb = NULL;
		if (!SetupDiGetDeviceRegistryPropertyW(Info, &DeviceInfoData,
			SPDRP_HARDWAREID, NULL, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL))
			continue;
		nusb = NWL_NodeAppendNew(node, "Device", NFLG_TABLE_ROW);
		NWL_NodeAttrSet(nusb, "HWID", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);
		NWL_ParseHwid(nusb, Ids, IdsSize, NWLC->NwBufW, 1);

		if (SetupDiGetDeviceRegistryPropertyW(Info, &DeviceInfoData,
			SPDRP_COMPATIBLEIDS, NULL, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL)
			&& NWLC->NwBufW[0])
		{
			LPCSTR BufferHw = NWL_Ucs2ToUtf8(NWLC->NwBufW);
			NWL_NodeAttrSet(nusb, "Compatiable ID", BufferHw, 0);
			ParseHwClass(nusb, Ids, IdsSize, BufferHw);
		}
	}
	SetupDiDestroyDeviceInfoList(Info);
fail:
	free(Ids);
	return node;
}