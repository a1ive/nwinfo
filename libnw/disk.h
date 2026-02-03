// SPDX-License-Identifier: Unlicense
#pragma once

#define VC_EXTRALEAN
#include <windows.h>

#include "nwapi.h"

#define DISK_PROP_STR_LEN 42
#define DISK_UUID_STR_LEN 17

typedef struct _DISK_VOL_INFO
{
	// Volume Info
	WCHAR VolPath[MAX_PATH];
	WCHAR VolLabel[MAX_PATH];
	WCHAR VolFs[MAX_PATH];
	ULARGE_INTEGER VolFreeSpace;
	ULARGE_INTEGER VolTotalSpace;
	double VolUsage;
	LPWCH VolNames;
	CHAR VolRealPath[MAX_PATH];
	CHAR VolFsUuid[DISK_UUID_STR_LEN];
	// Partition Info
	UINT64 StartLba;
	DWORD PartNum;
	CHAR PartType[DISK_PROP_STR_LEN];
	CHAR PartId[DISK_PROP_STR_LEN];
	CHAR PartFlag[DISK_PROP_STR_LEN];
	BOOL Bootable;
}DISK_VOL_INFO;

typedef struct _PHY_DRIVE_INFO
{
	DWORD Index;
	HANDLE Handle;
	PARTITION_STYLE PartMap; // 0:MBR 1:GPT 2:RAW
	UINT64 SizeInBytes;
	BYTE DeviceType;
	BOOL RemovableMedia;
	BOOL Ssd;
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

	DWORD VolCount;
	DISK_VOL_INFO* VolInfo;
}PHY_DRIVE_INFO;

LIBNW_API DWORD NWL_GetDriveInfoList(BOOL bIsCdRom, BOOL bGetVolume, PHY_DRIVE_INFO** pDriveList);
LIBNW_API DWORD NWL_GetDriveCount(BOOL bIsCdRom);
LIBNW_API VOID NWL_DestoryDriveInfoList(PHY_DRIVE_INFO* pInfo, DWORD dwCount);
int __cdecl NWL_CompareDiskId(const void* a, const void* b);
