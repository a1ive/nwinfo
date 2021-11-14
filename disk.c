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
	case BusTypeNvme: return "Nvme";
	}
	return "unknown";
}

static UINT32 GetPhysicalDriveCount(void)
{
	DWORD Value = 0;
	int Count = 0;
	if (GetRegDwordValue(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\disk\\Enum", "Count", &Value) == 0)
	{
		Count = Value;
	}
	return Count;
}

static int GetPhyDriveByLogicalDrive(int DriveLetter)
{
	BOOL Ret;
	DWORD dwSize = 0;
	HANDLE Handle;
	VOLUME_DISK_EXTENTS DiskExtents = { 0 };
	CHAR PhyPath[128];

	snprintf(PhyPath, 128, "\\\\.\\%C:", (CHAR)DriveLetter);

	Handle = CreateFileA(PhyPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (Handle == INVALID_HANDLE_VALUE)
		return -1;

	Ret = DeviceIoControl(Handle,
		IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
		NULL,
		0,
		&DiskExtents,
		(DWORD)(sizeof(DiskExtents)),
		(LPDWORD)&dwSize,
		NULL);

	if (!Ret || DiskExtents.NumberOfDiskExtents == 0)
	{
		CHECK_CLOSE_HANDLE(Handle);
		return -1;
	}
	CHECK_CLOSE_HANDLE(Handle);

	return (int) DiskExtents.Extents[0].DiskNumber;
}

static void AddLetter(CHAR* LogLetter, int Letter)
{
	if (Letter < 'A' || Letter > 'Z')
		return;
	for (int i = 0; i < 26; i++)
	{
		if (LogLetter[i] == 0)
		{
			LogLetter[i] = (CHAR)Letter;
			break;
		}
	}
}

static const UINT8 GPT_MAGIC[8] = { 0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54 };

static int GetAllPhysicalDriveInfo(PHY_DRIVE_INFO* pDriveList, DWORD* pDriveCount)
{
	UINT32 i;
	UINT32 Count;
	int id;
	int Letter = 'A';
	BOOL  bRet;
	DWORD dwBytes;
	DWORD DriveCount = 0;
	HANDLE Handle = INVALID_HANDLE_VALUE;
	CHAR PhyDrive[128];
	PHY_DRIVE_INFO* CurDrive = pDriveList;
	GET_LENGTH_INFORMATION LengthInfo = { 0 };
	STORAGE_PROPERTY_QUERY Query = { 0 };
	STORAGE_DESCRIPTOR_HEADER DevDescHeader = { 0 };
	STORAGE_DEVICE_DESCRIPTOR* pDevDesc;
	int PhyDriveId[MAX_PHY_DRIVE] = { 0 };
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

	Count = GetPhysicalDriveCount();

	for (i = 0; i < Count && i < MAX_PHY_DRIVE; i++)
	{
		PhyDriveId[i] = i;
	}

	dwBytes = GetLogicalDrives();
	//printf("Logical Drives: 0x%x\n", dwBytes);
	while (dwBytes)
	{
		if (dwBytes & 0x01)
		{
			id = GetPhyDriveByLogicalDrive(Letter);
			//printf("%C --> %d\n", Letter, id);
			if (id >= 0)
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
					//printf("Add phy%d to list\n", i);
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

		snprintf(PhyDrive, 128, "\\\\.\\PhysicalDrive%d", PhyDriveId[i]);
		Handle = CreateFileA(PhyDrive, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

		if (Handle == INVALID_HANDLE_VALUE)
			continue;

		bRet = DeviceIoControl(Handle,
			IOCTL_DISK_GET_LENGTH_INFO, NULL,
			0,
			&LengthInfo,
			sizeof(LengthInfo),
			&dwBytes,
			NULL);
		if (!bRet)
			continue;

		Query.PropertyId = StorageDeviceProperty;
		Query.QueryType = PropertyStandardQuery;

		bRet = DeviceIoControl(Handle,
			IOCTL_STORAGE_QUERY_PROPERTY,
			&Query,
			sizeof(Query),
			&DevDescHeader,
			sizeof(STORAGE_DESCRIPTOR_HEADER),
			&dwBytes,
			NULL);
		if (!bRet)
			continue;

		if (DevDescHeader.Size < sizeof(STORAGE_DEVICE_DESCRIPTOR))
			continue;

		pDevDesc = (STORAGE_DEVICE_DESCRIPTOR*)malloc(DevDescHeader.Size);
		if (!pDevDesc)
			continue;

		bRet = DeviceIoControl(Handle,
			IOCTL_STORAGE_QUERY_PROPERTY,
			&Query,
			sizeof(Query),
			pDevDesc,
			DevDescHeader.Size,
			&dwBytes,
			NULL);
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

		ZeroMemory(Sector, 512 + 512);
		CurDrive->PartStyle = 0;
		SetFilePointer(Handle, 0, NULL, FILE_BEGIN);
		bRet = ReadFile(Handle, Sector, 512 + 512, NULL, NULL);
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
	if (GetAllPhysicalDriveInfo(PhyDriveList, &PhyDriveCount) == 0)
	{
		for (i = 0, CurDrive = PhyDriveList; i < PhyDriveCount; i++, CurDrive++)
		{
			printf("\\\\.\\PhysicalDrive%d\n", CurDrive->PhyDrive);
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
				printf("  GPT GUID: %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
					CurDrive->GptGuid[0], CurDrive->GptGuid[1], CurDrive->GptGuid[2], CurDrive->GptGuid[3],
					CurDrive->GptGuid[4], CurDrive->GptGuid[5], CurDrive->GptGuid[6], CurDrive->GptGuid[7],
					CurDrive->GptGuid[8], CurDrive->GptGuid[9], CurDrive->GptGuid[10], CurDrive->GptGuid[11],
					CurDrive->GptGuid[12], CurDrive->GptGuid[13], CurDrive->GptGuid[14], CurDrive->GptGuid[15]);
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
