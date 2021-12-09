// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "nwinfo.h"

#define MAX_PHY_DRIVE 128

#define CHECK_CLOSE_HANDLE(Handle) \
{\
	if (Handle == 0) \
		Handle = INVALID_HANDLE_VALUE; \
    if (Handle != INVALID_HANDLE_VALUE) \
    {\
        CloseHandle(Handle); \
        Handle = INVALID_HANDLE_VALUE; \
    }\
}

static const char* d_human_sizes[6] =
{ "B", "KB", "MB", "GB", "TB", "PB", };

static const CHAR* GetBusTypeString(STORAGE_BUS_TYPE Type)
{
	switch (Type)
	{
	case BusTypeUnknown: return "unknown";
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
	case BusTypeFileBackedVirtual: return "FileBackedVirtual";
	case BusTypeSpaces: return "Spaces";
	case BusTypeNvme: return "NVMe";
	case BusTypeSCM: return "SCM";
	case BusTypeUfs: return "UFS";
	}
	return "unknown";
}

static DWORD GetDriveCount(void)
{
	DWORD Value = 0;
	if (GetRegDwordValue(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\disk\\Enum", "Count", &Value) != 0)
		Value = 0;
	return Value;
}

static HANDLE GetHandleByLetter(CHAR Letter)
{
	CHAR PhyPath[] = "\\\\.\\A:";
	snprintf(PhyPath, sizeof(PhyPath), "\\\\.\\%C:", Letter);
	return CreateFileA(PhyPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
}

static HANDLE GetHandleById(DWORD Id)
{
	CHAR PhyPath[] = "\\\\.\\PhysicalDrive4294967295";
	snprintf(PhyPath, sizeof(PhyPath), "\\\\.\\PhysicalDrive%u", Id);
	return CreateFileA(PhyPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
}

static CHAR *GetDriveHwId(DWORD Drive)
{
	CHAR drvRegKey[] = "4294967295";
	snprintf(drvRegKey, sizeof(drvRegKey), "%u", Drive);
	return GetRegSzValue(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\disk\\Enum", drvRegKey);
}

static CHAR* GetDriveHwName(const CHAR* HwId)
{
	CHAR* HwName = NULL;
	CHAR* drvRegKey = malloc (2048);
	if (!drvRegKey)
		return NULL;
	snprintf(drvRegKey, 2048, "SYSTEM\\CurrentControlSet\\Enum\\%s", HwId);
	HwName = GetRegSzValue(HKEY_LOCAL_MACHINE, drvRegKey, "FriendlyName");
	if (!HwName)
		HwName = GetRegSzValue(HKEY_LOCAL_MACHINE, drvRegKey, "DeviceDesc");
	return HwName;
}

static BOOL GetDriveByLetter(CHAR Letter, DWORD* pDrive)
{
	BOOL Ret;
	DWORD dwSize = 0;
	VOLUME_DISK_EXTENTS DiskExtents = { 0 };
	HANDLE Handle = GetHandleByLetter(Letter);
	if (Handle == INVALID_HANDLE_VALUE)
		return FALSE;
	Ret = DeviceIoControl(Handle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
		NULL, 0, &DiskExtents, (DWORD)(sizeof(DiskExtents)), &dwSize, NULL);
	if (!Ret || DiskExtents.NumberOfDiskExtents == 0)
	{
		CHECK_CLOSE_HANDLE(Handle);
		return FALSE;
	}
	CHECK_CLOSE_HANDLE(Handle);
	*pDrive = DiskExtents.Extents[0].DiskNumber;
	return TRUE;
}

static void AddLetter(CHAR* LogLetter, CHAR Letter)
{
	UINT i;
	if (Letter < 'A' || Letter > 'Z')
		return;
	for (i = 0; i < 26; i++)
	{
		if (LogLetter[i] == 0)
		{
			LogLetter[i] = Letter;
			break;
		}
	}
}

static const UINT8 GPT_MAGIC[8] = { 0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54 };

static int GetDriveInfoList(PHY_DRIVE_INFO* pDriveList, DWORD* pDriveCount)
{
	DWORD i;
	DWORD Count;
	DWORD id;
	CHAR Letter = 'A';
	BOOL  bRet;
	DWORD dwBytes;
	DWORD DriveCount = 0;
	HANDLE Handle = INVALID_HANDLE_VALUE;
	PHY_DRIVE_INFO* CurDrive = pDriveList;
	GET_LENGTH_INFORMATION LengthInfo = { 0 };
	STORAGE_PROPERTY_QUERY Query = { 0 };
	STORAGE_DESCRIPTOR_HEADER DevDescHeader = { 0 };
	STORAGE_DEVICE_DESCRIPTOR* pDevDesc;
	DWORD PhyDriveId[MAX_PHY_DRIVE] = { 0 };
	CHAR LogLetter[MAX_PHY_DRIVE][26] = { 0 };
	UINT8 *Sector = NULL;
	struct mbr_header* MBR = NULL;
	struct gpt_header* GPT = NULL;

	Sector = malloc(512 + 512);
	if (!Sector)
		return 1;
	MBR = (struct mbr_header*)Sector;
	GPT = (struct gpt_header*)(Sector + 512);

	ZeroMemory(LogLetter, sizeof(LogLetter));

	Count = GetDriveCount();
	if (Count > MAX_PHY_DRIVE)
		Count = MAX_PHY_DRIVE;

	for (i = 0; i < Count; i++)
		PhyDriveId[i] = i;

	dwBytes = GetLogicalDrives();
	while (dwBytes)
	{
		if (dwBytes & 0x01)
		{
			if (GetDriveByLetter(Letter, &id))
			{
				for (i = 0; i < Count; i++)
				{
					if (PhyDriveId[i] == id)
					{
						AddLetter(LogLetter[i], Letter);
						break;
					}
				}
				if (i >= Count)
				{
					PhyDriveId[Count] = id;
					AddLetter(LogLetter[Count], Letter);
					Count++;
				}
			}
		}
		Letter++;
		dwBytes >>= 1;
	}

	for (i = 0; i < Count && DriveCount < MAX_PHY_DRIVE; i++)
	{
		CHECK_CLOSE_HANDLE(Handle);

		Handle = GetHandleById(PhyDriveId[i]);

		if (Handle == INVALID_HANDLE_VALUE)
			continue;

		bRet = DeviceIoControl(Handle, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0,
			&LengthInfo, sizeof(LengthInfo), &dwBytes, NULL);
		if (!bRet)
			continue;

		Query.PropertyId = StorageDeviceProperty;
		Query.QueryType = PropertyStandardQuery;

		bRet = DeviceIoControl(Handle, IOCTL_STORAGE_QUERY_PROPERTY, &Query, sizeof(Query),
			&DevDescHeader, sizeof(STORAGE_DESCRIPTOR_HEADER), &dwBytes, NULL);
		if (!bRet)
			continue;

		if (DevDescHeader.Size < sizeof(STORAGE_DEVICE_DESCRIPTOR))
			continue;

		pDevDesc = (STORAGE_DEVICE_DESCRIPTOR*)malloc(DevDescHeader.Size);
		if (!pDevDesc)
			continue;

		bRet = DeviceIoControl(Handle, IOCTL_STORAGE_QUERY_PROPERTY, &Query, sizeof(Query),
			pDevDesc, DevDescHeader.Size, &dwBytes, NULL);
		if (!bRet)
		{
			free(pDevDesc);
			continue;
		}

		CurDrive->PhyDrive = i;
		CurDrive->SizeInBytes = LengthInfo.Length.QuadPart;
		CurDrive->DeviceType = pDevDesc->DeviceType;
		CurDrive->RemovableMedia = pDevDesc->RemovableMedia;
		CurDrive->BusType = pDevDesc->BusType;
		CurDrive->HwID = GetDriveHwId(i);

		if (pDevDesc->VendorIdOffset)
		{
			strcpy_s(CurDrive->VendorId, sizeof (CurDrive->VendorId),
				(char*)pDevDesc + pDevDesc->VendorIdOffset);
			TrimString(CurDrive->VendorId);
		}

		if (pDevDesc->ProductIdOffset)
		{
			strcpy_s(CurDrive->ProductId, sizeof(CurDrive->ProductId),
				(char*)pDevDesc + pDevDesc->ProductIdOffset);
			TrimString(CurDrive->ProductId);
		}

		if (pDevDesc->ProductRevisionOffset)
		{
			strcpy_s(CurDrive->ProductRev, sizeof(CurDrive->ProductRev),
				(char*)pDevDesc + pDevDesc->ProductRevisionOffset);
			TrimString(CurDrive->ProductRev);
		}

		if (pDevDesc->SerialNumberOffset)
		{
			strcpy_s(CurDrive->SerialNumber, sizeof(CurDrive->SerialNumber),
				(char*)pDevDesc + pDevDesc->SerialNumberOffset);
			TrimString(CurDrive->SerialNumber);
		}
		DWORD dwSize = 512 + 512;
		ZeroMemory(Sector, dwSize);
		CurDrive->PartStyle = 0;
		SetFilePointer(Handle, 0, NULL, FILE_BEGIN);
		bRet = ReadFile(Handle, Sector, dwSize, &dwSize, NULL);
		if (!bRet)
			goto next_drive;

		if (MBR->signature == 0xaa55)
		{
			CurDrive->PartStyle = 1;
			memcpy(CurDrive->MbrSignature, MBR->unique_signature, 4);
			for (int j = 0; j < 4; j++)
			{
				if (MBR->entries[i].type == 0xee)
				{
					CurDrive->PartStyle = 2;
					break;
				}
			}
		}
		if (memcmp(GPT->magic, GPT_MAGIC, sizeof(GPT_MAGIC)) == 0)
		{
			memcpy(CurDrive->GptGuid, GPT->guid, 16);
			CurDrive->PartStyle = 2;
		}
		memcpy(CurDrive->DriveLetters, LogLetter[CurDrive->PhyDrive], 26);
next_drive:
		CurDrive++;
		DriveCount++;

		free(pDevDesc);

		CHECK_CLOSE_HANDLE(Handle);
	}

	*pDriveCount = DriveCount;

	return 0;
}

void nwinfo_disk(void)
{
	PHY_DRIVE_INFO* PhyDriveList = NULL;
	DWORD PhyDriveCount = 0, i = 0;
	PHY_DRIVE_INFO* CurDrive = NULL;
	PhyDriveList = (PHY_DRIVE_INFO*)malloc(sizeof(PHY_DRIVE_INFO) * MAX_PHY_DRIVE);
	if (NULL == PhyDriveList)
	{
		printf("Failed to alloc phy drive memory\n");
		return;
	}
	memset(PhyDriveList, 0, sizeof(PHY_DRIVE_INFO) * MAX_PHY_DRIVE);
	if (GetDriveInfoList(PhyDriveList, &PhyDriveCount) == 0)
	{
		for (i = 0, CurDrive = PhyDriveList; i < PhyDriveCount; i++, CurDrive++)
		{
			printf("\\\\.\\PhysicalDrive%u\n", CurDrive->PhyDrive);
			if (CurDrive->HwID)
			{
				CHAR* hwName = NULL;
				printf("  HWID: %s\n", CurDrive->HwID);
				hwName = GetDriveHwName(CurDrive->HwID);
				if (hwName)
				{
					printf("  HW Name: %s\n", hwName);
					free(hwName);
				}
				free(CurDrive->HwID);
			}
			if (CurDrive->VendorId[0])
				printf("  Vendor ID: %s\n", CurDrive->VendorId);
			if (CurDrive->ProductId[0])
				printf("  Product ID: %s\n", CurDrive->ProductId);
			if (CurDrive->ProductRev[0])
				printf("  Product Rev: %s\n", CurDrive->ProductRev);
			if (CurDrive->SerialNumber[0])
				printf("  Serial Number: %s\n", CurDrive->SerialNumber);
			printf("  Type: %s%s\n", GetBusTypeString(CurDrive->BusType),
				CurDrive->RemovableMedia ? " Removable" : "");
			printf("  Size: %s\n", GetHumanSize(CurDrive->SizeInBytes, d_human_sizes, 1024));
			if (CurDrive->PartStyle == 1)
			{
				printf("  PartMap: MBR\n");
				printf("  MBR Signature: %02X %02X %02X %02X\n",
					CurDrive->MbrSignature[0], CurDrive->MbrSignature[1],
					CurDrive->MbrSignature[2], CurDrive->MbrSignature[3]);
			}
			else if (CurDrive->PartStyle == 2)
			{
				printf("  PartMap: GPT\n");
				printf("  GPT GUID: %s\n", GuidToStr(CurDrive->GptGuid));
			}
			if (CurDrive->DriveLetters[0])
			{
				printf("  Drive Letters:");
				for (int j = 0;  CurDrive->DriveLetters[j] && j < 26; j++)
				{
					printf(" %c", CurDrive->DriveLetters[j]);
				}
				printf("\n");
			}
		}
	}

	free(PhyDriveList);
}
