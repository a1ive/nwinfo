// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "disk.h"
#include "utils.h"

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

static LPCSTR GetRealVolumePath(LPCSTR lpszVolume)
{
	LPCSTR lpszRealPath;
	HANDLE hFile = CreateFileA(lpszVolume, 0,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (!hFile || hFile == INVALID_HANDLE_VALUE)
		return lpszVolume;
	lpszRealPath = NWL_NtGetPathFromHandle(hFile);
	CloseHandle(hFile);
	return lpszRealPath ? lpszRealPath : lpszVolume;
}

static void
PrintVolumeInfo(PNODE pNode, LPCSTR lpszVolume)
{
	CHAR cchLabel[MAX_PATH];
	CHAR cchFs[MAX_PATH];
	CHAR cchPath[MAX_PATH];
	LPCH lpszVolumePathNames = NULL;
	DWORD dwSize = 0;
	ULARGE_INTEGER Space;

	snprintf(cchPath, MAX_PATH, "%s\\", lpszVolume);
	NWL_NodeAttrSet(pNode, "Path", GetRealVolumePath(lpszVolume), 0);
	NWL_NodeAttrSet(pNode, "Volume GUID", lpszVolume, 0);
	if (GetVolumeInformationA(cchPath, cchLabel, MAX_PATH, NULL, NULL, NULL, cchFs, MAX_PATH))
	{
		NWL_NodeAttrSet(pNode, "Label", cchLabel, 0);
		NWL_NodeAttrSet(pNode, "Filesystem", cchFs, 0);
	}
	if (GetDiskFreeSpaceExA(cchPath, NULL, NULL, &Space))
		NWL_NodeAttrSet(pNode, "Free Space",
			NWL_GetHumanSize(Space.QuadPart, d_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
	if (GetDiskFreeSpaceExA(cchPath, NULL, &Space, NULL))
		NWL_NodeAttrSet(pNode, "Total Space",
			NWL_GetHumanSize(Space.QuadPart, d_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
	GetVolumePathNamesForVolumeNameA(cchPath, NULL, 0, &dwSize);
	if (GetLastError() == ERROR_MORE_DATA && dwSize)
	{
		lpszVolumePathNames = malloc(dwSize);
		if (lpszVolumePathNames)
		{
			if (GetVolumePathNamesForVolumeNameA(cchPath, lpszVolumePathNames, dwSize, &dwSize))
			{
				PNODE mp = NWL_NodeAppendNew(pNode, "Volume Path Names", NFLG_TABLE);
				for (CHAR* p = lpszVolumePathNames; p[0] != '\0'; p += strlen(p) + 1)
				{
					PNODE mnt = NWL_NodeAppendNew(mp, "Mount Point", NFLG_TABLE_ROW);
					NWL_NodeAttrSet(mnt, strlen(p) > 3 ? "Path" : "Drive Letter", p, 0);
				}
			}
			free(lpszVolumePathNames);
		}
	}
}

static DWORD GetDriveCount(BOOL Cdrom)
{
	DWORD Value = 0;
	LPCSTR Key = "SYSTEM\\CurrentControlSet\\Services\\disk\\Enum";
	if (Cdrom)
		Key = "SYSTEM\\CurrentControlSet\\Services\\cdrom\\Enum";
	if (NWL_GetRegDwordValue(HKEY_LOCAL_MACHINE, Key, "Count", &Value) != 0)
		Value = 0;
	return Value;
}

static HANDLE GetHandleByLetter(CHAR Letter)
{
	CHAR PhyPath[] = "\\\\.\\A:";
	snprintf(PhyPath, sizeof(PhyPath), "\\\\.\\%C:", Letter);
	return CreateFileA(PhyPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
}

static HANDLE GetHandleById(BOOL Cdrom, DWORD Id)
{
	CHAR PhyPath[] = "\\\\.\\PhysicalDrive4294967295";
	if (Cdrom)
		snprintf(PhyPath, sizeof(PhyPath), "\\\\.\\CdRom%u", Id);
	else
		snprintf(PhyPath, sizeof(PhyPath), "\\\\.\\PhysicalDrive%u", Id);
	return CreateFileA(PhyPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
}

static CHAR *GetDriveHwId(BOOL Cdrom, DWORD Drive)
{
	CHAR drvRegKey[] = "4294967295";
	LPCSTR key = "SYSTEM\\CurrentControlSet\\Services\\disk\\Enum";
	if (Cdrom)
		key = "SYSTEM\\CurrentControlSet\\Services\\cdrom\\Enum";
	snprintf(drvRegKey, sizeof(drvRegKey), "%u", Drive);
	return NWL_GetRegSzValue(HKEY_LOCAL_MACHINE, key, drvRegKey);
}

static CHAR* GetDriveHwName(const CHAR* HwId)
{
	CHAR* HwName = NULL;
	CHAR* drvRegKey = malloc (2048);
	if (!drvRegKey)
		return NULL;
	snprintf(drvRegKey, 2048, "SYSTEM\\CurrentControlSet\\Enum\\%s", HwId);
	HwName = NWL_GetRegSzValue(HKEY_LOCAL_MACHINE, drvRegKey, "FriendlyName");
	if (!HwName)
		HwName = NWL_GetRegSzValue(HKEY_LOCAL_MACHINE, drvRegKey, "DeviceDesc");
	return HwName;
}

static BOOL GetDriveByVolume(BOOL bIsCdRom, HANDLE hVolume, DWORD* pDrive)
{
	DWORD dwSize = 0;
	STORAGE_DEVICE_NUMBER sdnDiskNumber = { 0 };

	if (!DeviceIoControl(hVolume, IOCTL_STORAGE_GET_DEVICE_NUMBER,
		NULL, 0, &sdnDiskNumber, (DWORD)(sizeof(STORAGE_DEVICE_NUMBER)), &dwSize, NULL))
		return FALSE;
	*pDrive = sdnDiskNumber.DeviceNumber;
	switch (sdnDiskNumber.DeviceType)
	{
	case FILE_DEVICE_CD_ROM:
	case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
	case FILE_DEVICE_DVD:
		return bIsCdRom ? TRUE : FALSE;
	case FILE_DEVICE_DISK:
	case FILE_DEVICE_DISK_FILE_SYSTEM:
	case FILE_DEVICE_FILE_SYSTEM:
		return bIsCdRom ? FALSE : TRUE;
	}
	return FALSE;
}

static BOOL
DiskRead(HANDLE hDisk, UINT64 Sector, UINT64 Offset, DWORD Size, PVOID pBuf)
{
	__int64 distance = Offset + (Sector << 9);
	LARGE_INTEGER li = { 0 };

	li.QuadPart = distance;
	li.LowPart = SetFilePointer(hDisk, li.LowPart, &li.HighPart, FILE_BEGIN);
	if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return FALSE;

	return ReadFile(hDisk, pBuf, Size, &Size, NULL);
}

static UINT64
GetDiskSize(HANDLE hDisk)
{
	DWORD dwBytes;
	UINT64 Size = 0;
	GET_LENGTH_INFORMATION LengthInfo = { 0 };
	if (DeviceIoControl(hDisk, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0,
		&LengthInfo, sizeof(LengthInfo), &dwBytes, NULL))
		Size = LengthInfo.Length.QuadPart;
	return Size;
}

static VOID
RemoveTrailingBackslash(CHAR* lpszPath)
{
	size_t len = strlen(lpszPath);
	if (len < 1 || lpszPath[len - 1] != '\\')
		return;
	lpszPath[len - 1] = '\0';
}

static const UINT8 GPT_MAGIC[8] = { 0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54 };

static DWORD GetDriveInfoList(BOOL bIsCdRom, PHY_DRIVE_INFO** pDriveList)
{
	DWORD i;
	DWORD dwCount;
	BOOL  bRet;
	DWORD dwBytes;
	PHY_DRIVE_INFO* pInfo;

	UINT8 *pSector = NULL;
	struct mbr_header* MBR = NULL;
	struct gpt_header* GPT = NULL;

	HANDLE hSearch;
	CHAR cchVolume[MAX_PATH];

	pSector = malloc(512 + 512);
	if (!pSector)
		return 0;
	MBR = (struct mbr_header*)pSector;
	GPT = (struct gpt_header*)(pSector + 512);

	dwCount = GetDriveCount(bIsCdRom);
	*pDriveList = calloc(dwCount, sizeof(PHY_DRIVE_INFO));
	if (!*pDriveList)
	{
		free(pSector);
		return 0;
	}
	pInfo = *pDriveList;

	for (i = 0; i < dwCount; i++)
	{
		HANDLE hDrive = INVALID_HANDLE_VALUE;
		STORAGE_PROPERTY_QUERY Query = { 0 };
		STORAGE_DESCRIPTOR_HEADER DevDescHeader = { 0 };
		STORAGE_DEVICE_DESCRIPTOR* pDevDesc = NULL;

		hDrive = GetHandleById(bIsCdRom, i);

		if (!hDrive || hDrive == INVALID_HANDLE_VALUE)
			goto next_drive;

		Query.PropertyId = StorageDeviceProperty;
		Query.QueryType = PropertyStandardQuery;

		bRet = DeviceIoControl(hDrive, IOCTL_STORAGE_QUERY_PROPERTY, &Query, sizeof(Query),
			&DevDescHeader, sizeof(STORAGE_DESCRIPTOR_HEADER), &dwBytes, NULL);
		if (!bRet || DevDescHeader.Size < sizeof(STORAGE_DEVICE_DESCRIPTOR))
			goto next_drive;

		pDevDesc = (STORAGE_DEVICE_DESCRIPTOR*)malloc(DevDescHeader.Size);
		if (!pDevDesc)
			goto next_drive;

		bRet = DeviceIoControl(hDrive, IOCTL_STORAGE_QUERY_PROPERTY, &Query, sizeof(Query),
			pDevDesc, DevDescHeader.Size, &dwBytes, NULL);
		if (!bRet)
			goto next_drive;

		pInfo[i].SizeInBytes = GetDiskSize(hDrive);
		pInfo[i].DeviceType = pDevDesc->DeviceType;
		pInfo[i].RemovableMedia = pDevDesc->RemovableMedia;
		pInfo[i].BusType = pDevDesc->BusType;
		pInfo[i].HwID = GetDriveHwId(bIsCdRom, i);

		if (pDevDesc->VendorIdOffset)
		{
			strcpy_s(pInfo[i].VendorId, MAX_PATH,
				(char*)pDevDesc + pDevDesc->VendorIdOffset);
			NWL_TrimString(pInfo[i].VendorId);
		}

		if (pDevDesc->ProductIdOffset)
		{
			strcpy_s(pInfo[i].ProductId, MAX_PATH,
				(char*)pDevDesc + pDevDesc->ProductIdOffset);
			NWL_TrimString(pInfo[i].ProductId);
		}

		if (pDevDesc->ProductRevisionOffset)
		{
			strcpy_s(pInfo[i].ProductRev, MAX_PATH,
				(char*)pDevDesc + pDevDesc->ProductRevisionOffset);
			NWL_TrimString(pInfo[i].ProductRev);
		}

		if (pDevDesc->SerialNumberOffset)
		{
			strcpy_s(pInfo[i].SerialNumber, MAX_PATH,
				(char*)pDevDesc + pDevDesc->SerialNumberOffset);
			NWL_TrimString(pInfo[i].SerialNumber);
		}

		if (bIsCdRom)
		{
			pInfo[i].PartMap = 3;
			goto next_drive;
		}

		ZeroMemory(pSector, 512 + 512);
		bRet = DiskRead(hDrive, 0, 0, 512 + 512, pSector);
		if (!bRet)
			goto next_drive;

		if (MBR->signature == 0xaa55)
		{
			pInfo[i].PartMap = 1;
			memcpy(pInfo[i].MbrSignature, MBR->unique_signature, 4);
			for (int j = 0; j < 4; j++)
			{
				if (MBR->entries[i].type == 0xee)
				{
					pInfo[i].PartMap = 2;
					break;
				}
			}
		}
		if (memcmp(GPT->magic, GPT_MAGIC, sizeof(GPT_MAGIC)) == 0)
		{
			memcpy(pInfo[i].GptGuid, GPT->guid, 16);
			pInfo[i].PartMap = 2;
		}

next_drive:
		if (pDevDesc)
			free(pDevDesc);
		if (hDrive && hDrive != INVALID_HANDLE_VALUE)
			CloseHandle(hDrive);
	}

	free(pSector);

	for (bRet = TRUE, hSearch = FindFirstVolumeA(cchVolume, MAX_PATH);
		bRet && hSearch != INVALID_HANDLE_VALUE;
		bRet = FindNextVolumeA(hSearch, cchVolume, MAX_PATH))
	{
		HANDLE hVolume;
		RemoveTrailingBackslash(cchVolume);
		hVolume = CreateFileA(cchVolume, 0,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (!hVolume || hVolume == INVALID_HANDLE_VALUE)
			continue;
		if (!GetDriveByVolume(bIsCdRom, hVolume, &dwBytes))
		{
			CloseHandle(hVolume);
			continue;
		}
		CloseHandle(hVolume);
		if (dwBytes >= dwCount || pInfo[dwBytes].VolumeCount >= 32)
			continue;
		strcpy_s(pInfo[dwBytes].Volumes[pInfo[dwBytes].VolumeCount], MAX_PATH, cchVolume);
		pInfo[dwBytes].VolumeCount++;
	}

	if (hSearch != INVALID_HANDLE_VALUE)
		FindVolumeClose(hSearch);

	return dwCount;
}

static VOID
PrintDiskInfo(BOOL cdrom, PNODE node)
{
	PHY_DRIVE_INFO* PhyDriveList = NULL;
	DWORD PhyDriveCount = 0, i = 0;
	PhyDriveCount = GetDriveInfoList(cdrom, &PhyDriveList);
	if (PhyDriveCount == 0)
		goto out;

	for (i = 0; i < PhyDriveCount; i++)
	{
		PNODE nd = NWL_NodeAppendNew(node, "Disk", NFLG_TABLE_ROW);
		NWL_NodeAttrSetf(nd, "Path", 0,
			cdrom ? "\\\\.\\CdRom%u" : "\\\\.\\PhysicalDrive%u", i);
		if (PhyDriveList[i].HwID)
		{
			CHAR* hwName = NULL;
			NWL_NodeAttrSet(nd, "HWID", PhyDriveList[i].HwID, 0);
			hwName = GetDriveHwName(PhyDriveList[i].HwID);
			if (hwName)
			{
				NWL_NodeAttrSet(nd, "HW Name", hwName, 0);
				free(hwName);
			}
			free(PhyDriveList[i].HwID);
		}
		if (PhyDriveList[i].VendorId[0])
			NWL_NodeAttrSet(nd, "Vendor ID", PhyDriveList[i].VendorId, 0);
		if (PhyDriveList[i].ProductId[0])
			NWL_NodeAttrSet(nd, "Product ID", PhyDriveList[i].ProductId, 0);
		if (PhyDriveList[i].ProductRev[0])
			NWL_NodeAttrSet(nd, "Product Rev", PhyDriveList[i].ProductRev, 0);
		if (PhyDriveList[i].SerialNumber[0])
			NWL_NodeAttrSet(nd, "Serial Number", PhyDriveList[i].SerialNumber, 0);
		NWL_NodeAttrSet(nd, "Type", GetBusTypeString(PhyDriveList[i].BusType), 0);
		NWL_NodeAttrSetBool(nd, "Removable", PhyDriveList[i].RemovableMedia, 0);
		NWL_NodeAttrSet(nd, "Size",
			NWL_GetHumanSize(PhyDriveList[i].SizeInBytes, d_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
		if (PhyDriveList[i].PartMap == 1)
		{
			NWL_NodeAttrSet(nd, "Partition Table", "MBR", 0);
			NWL_NodeAttrSetf(nd, "MBR Signature", 0, "%02X %02X %02X %02X",
				PhyDriveList[i].MbrSignature[0], PhyDriveList[i].MbrSignature[1],
				PhyDriveList[i].MbrSignature[2], PhyDriveList[i].MbrSignature[3]);
		}
		else if (PhyDriveList[i].PartMap == 2)
		{
			NWL_NodeAttrSet(nd, "Partition Table", "GPT", 0);
			NWL_NodeAttrSet(nd, "GPT GUID", NWL_GuidToStr(PhyDriveList[i].GptGuid), NAFLG_FMT_GUID);
		}
		if (PhyDriveList[i].VolumeCount)
		{
			DWORD j;
			PNODE nv = NWL_NodeAppendNew(nd, "Volumes", NFLG_TABLE);
			for (j = 0; j < PhyDriveList[i].VolumeCount; j++)
			{
				PNODE vol = NWL_NodeAppendNew(nv, "Volume", NFLG_TABLE_ROW);
				PrintVolumeInfo(vol, PhyDriveList[i].Volumes[j]);
			}
		}
	}

out:
	if (PhyDriveList)
		free(PhyDriveList);
}

PNODE NW_Disk(VOID)
{
	PNODE node = NWL_NodeAlloc("Disks", NFLG_TABLE);
	if (NWLC->DiskInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	PrintDiskInfo(FALSE, node);
	PrintDiskInfo(TRUE, node);
	return node;
}
