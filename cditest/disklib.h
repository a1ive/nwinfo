// SPDX-License-Identifier: Unlicense
#pragma once

#define VC_EXTRALEAN
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DISK_PROP_STR_LEN 42

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
	// Partition Info
	UINT64 StartLba;
	DWORD PartNum;
	WCHAR PartType[DISK_PROP_STR_LEN];
	WCHAR PartId[DISK_PROP_STR_LEN];
	WCHAR PartFlag[DISK_PROP_STR_LEN];
	BOOL BootIndicator;
}DISK_VOL_INFO;

typedef struct _PHY_DRIVE_INFO
{
	DWORD Index;
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

static inline LPCSTR
GetPartMapName(PARTITION_STYLE partMap)
{
	switch (partMap)
	{
	case PARTITION_STYLE_MBR: return "MBR";
	case PARTITION_STYLE_GPT: return "GPT";
	}
	return "RAW";
}

static inline LPCSTR
GetBusTypeName(STORAGE_BUS_TYPE busType)
{
	switch (busType)
	{
	case BusTypeUnknown: return "Unknown";
	case BusTypeScsi: return "SCSI";
	case BusTypeAtapi: return "Atapi";
	case BusTypeAta: return "ATA";
	case BusType1394: return "1394";
	case BusTypeSsa: return "SSA";
	case BusTypeFibre: return "Fibre";
	case BusTypeUsb: return "USB";
	case BusTypeRAID: return "RAID";
	case BusTypeiScsi: return "iSCSI";
	case BusTypeSas: return "SAS";
	case BusTypeSata: return "SATA";
	case BusTypeSd: return "SD";
	case BusTypeMmc: return "MMC";
	case BusTypeVirtual: return "Virtual";
	case BusTypeFileBackedVirtual: return "File";
	case BusTypeSpaces: return "Spaces";
	case BusTypeNvme: return "NVMe";
	case BusTypeSCM: return "SCM";
	case BusTypeUfs: return "UFS";
	}
	return "Unknown";
}

DWORD GetDriveInfoList(BOOL bIsCdRom, PHY_DRIVE_INFO** pDriveList);
VOID DestoryDriveInfoList(PHY_DRIVE_INFO* pInfo, DWORD dwCount);

VOID GetNtVersion(LPOSVERSIONINFOEXW osInfo);
LPCSTR Ucs2ToUtf8(LPCWSTR src);
LPCSTR GetHumanSize(UINT64 ullSize, UINT64 ullBase);

#ifdef __cplusplus
}
#endif
