// SPDX-License-Identifier: Unlicense
#pragma once

#define VC_EXTRALEAN
#include <windows.h>

typedef struct _PHY_DRIVE_INFO
{
	DWORD Index;
	DWORD PartMap; // 0:MBR 1:GPT 2:RAW
	UINT64 SizeInBytes;
	BYTE DeviceType;
	BOOL RemovableMedia;
	WCHAR HwID[MAX_PATH];
	WCHAR HwName[MAX_PATH];
	CHAR VendorId[MAX_PATH];
	CHAR ProductId[MAX_PATH];
	CHAR ProductRev[MAX_PATH];
	CHAR SerialNumber[MAX_PATH];
	STORAGE_BUS_TYPE BusType;
	// MBR
	UCHAR MbrSignature[4];
	DWORD MbrLba[4];
	// GPT
	UCHAR GptGuid[16];

	DWORD VolumeCount;
	WCHAR Volumes[32][MAX_PATH];
}PHY_DRIVE_INFO;
