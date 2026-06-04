// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "disk.h"

#include <setupapi.h>
#include <winioctl.h>

#include "utils.h"
#include "vbr.h"
#include "../libcdi/libcdi.h"

static LPCSTR GetRealVolumePath(LPCWSTR lpszVolume)
{
	LPCSTR lpszRealPath;
	HANDLE hFile = CreateFileW(lpszVolume, 0,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (!hFile || hFile == INVALID_HANDLE_VALUE)
		return NWL_Ucs2ToUtf8(lpszVolume);
	lpszRealPath = NWL_NtGetPathFromHandle(hFile);
	CloseHandle(hFile);
	return lpszRealPath ? lpszRealPath : NWL_Ucs2ToUtf8(lpszVolume);
}

static LPCSTR
GetGptFlag(GUID* pGuid, BOOL* pBootable)
{
	LPCSTR lpszGuid = NWL_WinGuidToStr(FALSE, pGuid);
	*pBootable = FALSE;
	if (_stricmp(lpszGuid, "c12a7328-f81f-11d2-ba4b-00a0c93ec93b") == 0)
	{
		*pBootable = TRUE;
		return "ESP";
	}
	else if (_stricmp(lpszGuid, "e3c9e316-0b5c-4db8-817d-f92df00215ae") == 0)
		return "MSR";
	else if (_stricmp(lpszGuid, "de94bba4-06d1-4d40-a16a-bfd50179d6ac") == 0)
		return "WINRE";
	else if (_stricmp(lpszGuid, "21686148-6449-6e6f-744e-656564454649") == 0)
		return "BIOS";
	else if (_stricmp(lpszGuid, "024dee41-33e7-11d3-9d69-0008c781f39f") == 0)
		return "MBR";
	else if (_stricmp(lpszGuid, "af9b60a0-1431-4f62-bc68-3311714a69ad") == 0)
		return "LDM";
	else if (_stricmp(lpszGuid, "5808c8aa-7e8f-42e0-85d2-e1e90434cfb3") == 0)
		return "LDM-META";
	else if (_stricmp(lpszGuid, "0fc63daf-8483-4772-8e79-3d69d8477de4") == 0)
		return "LINUX-DATA";
	else if (_stricmp(lpszGuid, "0657fd6d-a4ab-43c4-84e5-0933c84b4f4f") == 0)
		return "LINUX-SWAP";
	else if (_stricmp(lpszGuid, "ca7d7ccb-63ed-4c53-861c-1742536059cc") == 0)
		return "LUKS";
	else if (_stricmp(lpszGuid, "e6d6d379-f507-44c2-a23c-238f2a3df928") == 0)
		return "LVM";
	else if (_stricmp(lpszGuid, "48465300-0000-11aa-aa11-00306543ecac") == 0)
		return "APPLE-HFS";
	else if (_stricmp(lpszGuid, "7c3457ef-0000-11aa-aa11-00306543ecac") == 0)
		return "APPLE-APFS";
	else if (_stricmp(lpszGuid, "55465300-0000-11aa-aa11-00306543ecac") == 0)
		return "APPLE-UFS";
	else if (_stricmp(lpszGuid, "52414944-0000-11aa-aa11-00306543ecac") == 0)
		return "APPLE-RAID";
	else if (_stricmp(lpszGuid, "52414944-5f4f-11aa-aa11-00306543ecac") == 0)
		return "APPLE-RAID-OFFLINE";
	else if (_stricmp(lpszGuid, "426f6f74-0000-11aa-aa11-00306543ecac") == 0)
		return "APPLE-BOOT";
	else if (_stricmp(lpszGuid, "4c616265-6c00-11aa-aa11-00306543ecac") == 0)
		return "APPLE-LABEL";
	else if (_stricmp(lpszGuid, "5265636f-7665-11aa-aa11-00306543ecac") == 0)
		return "APPLE-TV-RECOVERY";
	else if (_stricmp(lpszGuid, "53746f72-6167-11aa-aa11-00306543ecac") == 0)
		return "APPLE-CORE-STORAGE";
	else if (_stricmp(lpszGuid, "69646961-6700-11aa-aa11-00306543ecac") == 0)
		return "APPLE-SILICON-BOOT";
	else if (_stricmp(lpszGuid, "52637672-7900-11aa-aa11-00306543ecac") == 0)
		return "APPLE-SILICON-RECOVERY";
	return "DATA";
}

static LPCSTR
GetMbrFlag(LONGLONG llStartingOffset, BYTE PartType, PHY_DRIVE_INFO* pParent)
{
	INT i;
	LONGLONG llLba = llStartingOffset >> 9;

	switch (PartType)
	{
	case 0x05:
	case 0x0F:
	case 0x85:
		return "EXTENDED";
	case 0x42:
		return "LDM";
	}

	for (i = 0; i < 4; i++)
	{
		if (llLba == pParent->MbrLba[i])
			return "PRIMARY";
	}
	return "LOGICAL";
}

static BOOL
IsValidPartitionEntry(PARTITION_INFORMATION_EX* pPartInfo)
{
	if (pPartInfo->PartitionLength.QuadPart <= 0)
		return FALSE;

	const GUID emptyGuid = { 0 };
	switch (pPartInfo->PartitionStyle)
	{
	case PARTITION_STYLE_MBR:
		return pPartInfo->Mbr.PartitionType != 0;
	case PARTITION_STYLE_GPT:
		return (memcmp(&pPartInfo->Gpt.PartitionType, &emptyGuid, sizeof(GUID)) != 0);
	}
	return FALSE;
}

static BOOL
FillPartitionInfoFromEntry(DISK_VOL_INFO* pInfo,
	PARTITION_INFORMATION_EX* pPartInfo, PHY_DRIVE_INFO* pParent)
{
	BOOL bBootable = FALSE;

	if (!IsValidPartitionEntry(pPartInfo))
		return FALSE;

	pInfo->StartLba = (UINT64)(pPartInfo->StartingOffset.QuadPart >> 9);
	pInfo->PartSize = (UINT64)pPartInfo->PartitionLength.QuadPart;
	pInfo->PartNum = pPartInfo->PartitionNumber;
	if (pParent && pInfo->PartNum)
		snprintf(pInfo->PartPath, MAX_PATH,
			"\\\\?\\GLOBALROOT\\Device\\Harddisk%lu\\Partition%lu",
			pParent->Index, pInfo->PartNum);

	switch (pPartInfo->PartitionStyle)
	{
	case PARTITION_STYLE_MBR:
		snprintf(pInfo->PartType, DISK_PROP_STR_LEN, "0x%02X", pPartInfo->Mbr.PartitionType);
		strncpy_s(pInfo->PartId, DISK_PROP_STR_LEN,
			NWL_WinGuidToStr(TRUE, &pPartInfo->Mbr.PartitionId), _TRUNCATE);
		strncpy_s(pInfo->PartFlag, DISK_PROP_STR_LEN,
			GetMbrFlag(pPartInfo->StartingOffset.QuadPart, pPartInfo->Mbr.PartitionType, pParent),
			_TRUNCATE);
		pInfo->Bootable = pPartInfo->Mbr.BootIndicator;
		break;
	case PARTITION_STYLE_GPT:
		strncpy_s(pInfo->PartType, DISK_PROP_STR_LEN,
			NWL_WinGuidToStr(TRUE, &pPartInfo->Gpt.PartitionType), _TRUNCATE);
		strncpy_s(pInfo->PartId, DISK_PROP_STR_LEN,
			NWL_WinGuidToStr(TRUE, &pPartInfo->Gpt.PartitionId), _TRUNCATE);
		strncpy_s(pInfo->PartFlag, DISK_PROP_STR_LEN,
			GetGptFlag(&pPartInfo->Gpt.PartitionType, &bBootable), _TRUNCATE);
		pInfo->Bootable = bBootable;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

static BOOL
FillPartitionInfo(DISK_VOL_INFO* pInfo, LPCWSTR lpszPath, PHY_DRIVE_INFO* pParent)
{
	BOOL bRet;
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	PARTITION_INFORMATION_EX partInfo = { 0 };
	DWORD dwPartInfo = sizeof(PARTITION_INFORMATION_EX);
	hDevice = CreateFileW(lpszPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (!hDevice || hDevice == INVALID_HANDLE_VALUE)
		return FALSE;
	bRet = DeviceIoControl(hDevice, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, &partInfo, dwPartInfo, &dwPartInfo, NULL);
	CloseHandle(hDevice);
	if (!bRet)
		return FALSE;
	if (partInfo.StartingOffset.QuadPart == 0) // CDROM
		return FALSE;
	return FillPartitionInfoFromEntry(pInfo, &partInfo, pParent);
}

static void
PrintPartitionInfo(PNODE pNode, const DISK_VOL_INFO* pInfo)
{
	if (pInfo->StartLba == 0 && pInfo->PartNum == 0 &&
		pInfo->PartType[0] == '\0' && pInfo->PartId[0] == '\0')
		return;
	NWL_NodeAttrSetf(pNode, "Starting LBA", NAFLG_FMT_NUMERIC, "%llu", pInfo->StartLba);
	NWL_NodeAttrSetf(pNode, "Partition Number", NAFLG_FMT_NUMERIC, "%lu", pInfo->PartNum);
	NWL_NodeAttrSet(pNode, "Partition Type", pInfo->PartType, NAFLG_FMT_NEED_QUOTE);
	NWL_NodeAttrSet(pNode, "Partition ID", pInfo->PartId, NAFLG_FMT_GUID);
	NWL_NodeAttrSet(pNode, "Partition Flag", pInfo->PartFlag, 0);
	NWL_NodeAttrSetBool(pNode, "Boot Indicator", pInfo->Bootable, 0);
	if (pInfo->PartSize)
		NWL_NodeAttrSet(pNode, "Partition Size", NWL_GetHumanSize(pInfo->PartSize, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
}

static void
GetVolumeFsUuid(LPCWSTR lpszVolume, CHAR cchUuid[DISK_UUID_STR_LEN])
{
	ZeroMemory(cchUuid, DISK_UUID_STR_LEN);

	union VOLUME_BOOT_RECORD vbrData;
	FILE* fd = _wfopen(lpszVolume, L"rb");

	if (fd == NULL)
		return;

	if (fread(&vbrData, sizeof(vbrData), 1, fd) != 1)
		goto out;

	if (memcmp(vbrData.Exfat.oem_name, "EXFAT", 5) == 0)
	{
		snprintf(cchUuid, DISK_UUID_STR_LEN, "%04x-%04x",
			(uint16_t)(vbrData.Exfat.num_serial >> 16),
			(uint16_t)vbrData.Exfat.num_serial);
	}
	else if (memcmp(vbrData.Ntfs.oem_name, "NTFS", 4) == 0)
	{
		snprintf(cchUuid, DISK_UUID_STR_LEN, "%016llx", (unsigned long long) vbrData.Ntfs.num_serial);
	}
	else if (memcmp(vbrData.Fat.version.fat12_or_fat16.fstype, "FAT12", 5) == 0)
	{
		snprintf(cchUuid, DISK_UUID_STR_LEN, "%04x-%04x",
			(uint16_t)(vbrData.Fat.version.fat12_or_fat16.num_serial >> 16),
			(uint16_t)vbrData.Fat.version.fat12_or_fat16.num_serial);
	}
	else if (memcmp(vbrData.Fat.version.fat12_or_fat16.fstype, "FAT16", 5) == 0)
	{
		snprintf(cchUuid, DISK_UUID_STR_LEN, "%04x-%04x",
			(uint16_t)(vbrData.Fat.version.fat12_or_fat16.num_serial >> 16),
			(uint16_t)vbrData.Fat.version.fat12_or_fat16.num_serial);
	}
	else if (memcmp(vbrData.Fat.version.fat32.fstype, "FAT32", 5) == 0)
	{
		snprintf(cchUuid, DISK_UUID_STR_LEN, "%04x-%04x",
			(uint16_t)(vbrData.Fat.version.fat32.num_serial >> 16),
			(uint16_t)vbrData.Fat.version.fat32.num_serial);
	}

out:
	fclose(fd);
}

static void
FillVolumeInfo(DISK_VOL_INFO* pInfo, LPCWSTR lpszVolume, PHY_DRIVE_INFO* pParent)
{
	DWORD dwSize = 0;

	pInfo->VolNames = NULL;
	swprintf(pInfo->VolPath, MAX_PATH, L"%s\\", lpszVolume);
	strncpy_s(pInfo->VolRealPath, MAX_PATH, GetRealVolumePath(lpszVolume), _TRUNCATE);
	GetVolumeFsUuid(lpszVolume, pInfo->VolFsUuid);
	FillPartitionInfo(pInfo, lpszVolume, pParent);

	GetVolumeInformationW(pInfo->VolPath, pInfo->VolLabel, MAX_PATH,
		NULL, NULL, NULL, pInfo->VolFs, MAX_PATH);
	GetDiskFreeSpaceExW(pInfo->VolPath, NULL, &pInfo->VolTotalSpace, &pInfo->VolFreeSpace);
	if (pInfo->VolTotalSpace.QuadPart != 0)
		pInfo->VolUsage = 100.0 - (pInfo->VolFreeSpace.QuadPart * 100.0) / pInfo->VolTotalSpace.QuadPart;
	GetVolumePathNamesForVolumeNameW(pInfo->VolPath, NULL, 0, &dwSize);
	if (GetLastError() == ERROR_MORE_DATA && dwSize)
	{
		pInfo->VolNames = calloc(dwSize, sizeof(WCHAR));
		if (pInfo->VolNames)
		{
			if (!GetVolumePathNamesForVolumeNameW(pInfo->VolPath, pInfo->VolNames, dwSize, &dwSize))
			{
				free(pInfo->VolNames);
				pInfo->VolNames = NULL;
			}
		}
	}
}

static void
PrintVolumeInfo(PNODE pNode, const DISK_VOL_INFO* pInfo)
{
	WCHAR cchGuid[MAX_PATH];

	if (pInfo->VolRealPath[0])
		NWL_NodeAttrSet(pNode, "Path", pInfo->VolRealPath, 0);
	else if (pInfo->VolPath[0])
		NWL_NodeAttrSet(pNode, "Path", NWL_Ucs2ToUtf8(pInfo->VolPath), 0);
	else if (pInfo->PartPath[0])
		NWL_NodeAttrSet(pNode, "Path", pInfo->PartPath, 0);
	if (pInfo->VolPath[0])
	{
		wcsncpy_s(cchGuid, MAX_PATH, pInfo->VolPath, _TRUNCATE);
		size_t len = wcslen(cchGuid);
		if (len > 0 && cchGuid[len - 1] == L'\\')
			cchGuid[len - 1] = L'\0';
		NWL_NodeAttrSet(pNode, "Volume GUID", NWL_Ucs2ToUtf8(cchGuid), 0);
	}

	PrintPartitionInfo(pNode, pInfo);
	if (pInfo->VolLabel[0])
		NWL_NodeAttrSet(pNode, "Label", NWL_Ucs2ToUtf8(pInfo->VolLabel), 0);
	if (pInfo->VolFs[0])
		NWL_NodeAttrSet(pNode, "Filesystem", NWL_Ucs2ToUtf8(pInfo->VolFs), 0);
	if (pInfo->VolFsUuid[0])
		NWL_NodeAttrSet(pNode, "FS UUID", pInfo->VolFsUuid, 0);
	if (pInfo->VolFreeSpace.QuadPart || pInfo->VolTotalSpace.QuadPart)
	{
		NWL_NodeAttrSet(pNode, "Free Space",
			NWL_GetHumanSize(pInfo->VolFreeSpace.QuadPart, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		NWL_NodeAttrSet(pNode, "Total Space",
			NWL_GetHumanSize(pInfo->VolTotalSpace.QuadPart, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
	}
	if (pInfo->VolTotalSpace.QuadPart != 0)
		NWL_NodeAttrSetf(pNode, "Usage", 0, "%.2f%%", pInfo->VolUsage);
	if (pInfo->VolNames)
	{
		PNODE mp = NWL_NodeAppendNew(pNode, "Volume Path Names", NFLG_TABLE);
		for (WCHAR* p = pInfo->VolNames; p[0] != L'\0'; p += wcslen(p) + 1)
		{
			PNODE mnt = NWL_NodeAppendNew(mp, "Mount Point", NFLG_TABLE_ROW);
			NWL_NodeAttrSet(mnt, wcslen(p) > 3 ? "Path" : "Drive Letter", NWL_Ucs2ToUtf8(p), 0);
		}
	}
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
	//case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
	case FILE_DEVICE_DVD:
		return bIsCdRom ? TRUE : FALSE;
	case FILE_DEVICE_DISK:
	//case FILE_DEVICE_DISK_FILE_SYSTEM:
	//case FILE_DEVICE_FILE_SYSTEM:
		return bIsCdRom ? FALSE : TRUE;
	}
	return FALSE;
}

static VOLUME_DISK_EXTENTS*
GetVolumeDiskExtents(HANDLE hVolume)
{
	DWORD dwSize = 0;
	DWORD dwBytes = 0;
	VOLUME_DISK_EXTENTS vde = { 0 };
	VOLUME_DISK_EXTENTS* pExtents = NULL;

	if (DeviceIoControl(hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
		NULL, 0, &vde, sizeof(VOLUME_DISK_EXTENTS), &dwBytes, NULL))
	{
		dwSize = (DWORD)sizeof(VOLUME_DISK_EXTENTS);
		pExtents = malloc(dwSize);
		if (pExtents)
			memcpy(pExtents, &vde, dwSize);
		return pExtents;
	}

	if (GetLastError() != ERROR_MORE_DATA || vde.NumberOfDiskExtents == 0)
		return NULL;

	dwSize = (DWORD)(sizeof(VOLUME_DISK_EXTENTS) + sizeof(DISK_EXTENT) * (vde.NumberOfDiskExtents - 1));
	pExtents = malloc(dwSize);
	if (!pExtents)
		return NULL;

	if (!DeviceIoControl(hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
		NULL, 0, pExtents, dwSize, &dwBytes, NULL))
	{
		free(pExtents);
		return NULL;
	}
	return pExtents;
}

static PHY_DRIVE_INFO*
FindDriveByIndex(PHY_DRIVE_INFO* pInfo, DWORD dwCount, DWORD dwIndex)
{
	for (DWORD i = 0; i < dwCount; i++)
	{
		if (pInfo[i].Index == dwIndex)
			return &pInfo[i];
	}
	return NULL;
}

static DISK_VOL_INFO*
FindVolumeInfo(PHY_DRIVE_INFO* pInfo, LPCWSTR lpszVolume)
{
	WCHAR cchVolume[MAX_PATH];

	swprintf(cchVolume, MAX_PATH, L"%s\\", lpszVolume);
	for (DWORD i = 0; i < pInfo->VolCount; i++)
	{
		if (_wcsicmp(pInfo->VolInfo[i].VolPath, cchVolume) == 0)
			return &pInfo->VolInfo[i];
	}
	return NULL;
}

static VOID
CopyPartitionInfo(DISK_VOL_INFO* pDst, const DISK_VOL_INFO* pSrc)
{
	strncpy_s(pDst->PartPath, MAX_PATH, pSrc->PartPath, _TRUNCATE);
	pDst->StartLba = pSrc->StartLba;
	pDst->PartSize = pSrc->PartSize;
	pDst->PartNum = pSrc->PartNum;
	strncpy_s(pDst->PartType, DISK_PROP_STR_LEN, pSrc->PartType, _TRUNCATE);
	strncpy_s(pDst->PartId, DISK_PROP_STR_LEN, pSrc->PartId, _TRUNCATE);
	strncpy_s(pDst->PartFlag, DISK_PROP_STR_LEN, pSrc->PartFlag, _TRUNCATE);
	pDst->Bootable = pSrc->Bootable;
}

static DISK_VOL_INFO*
FindPartitionByExtent(PHY_DRIVE_INFO* pInfo, const DISK_EXTENT* pExtent)
{
	UINT64 ullExtentStart = (UINT64)pExtent->StartingOffset.QuadPart;

	for (DWORD i = 0; i < pInfo->PartCount; i++)
	{
		DISK_VOL_INFO* pPart = &pInfo->PartInfo[i];
		UINT64 ullPartStart = pPart->StartLba << 9;
		UINT64 ullPartEnd = ullPartStart + pPart->PartSize;

		if (ullPartEnd < ullPartStart)
			ullPartEnd = ~(UINT64)0;
		if (ullExtentStart >= ullPartStart && ullExtentStart < ullPartEnd)
			return pPart;
	}
	return NULL;
}

static BOOL
HasVolumeForPartition(PHY_DRIVE_INFO* pInfo, const DISK_VOL_INFO* pPart)
{
	for (DWORD i = 0; i < pInfo->VolCount; i++)
	{
		DISK_VOL_INFO* pVol = &pInfo->VolInfo[i];
		if (pVol->StartLba == pPart->StartLba &&
			pVol->PartSize == pPart->PartSize &&
			pVol->PartNum == pPart->PartNum)
			return TRUE;
	}
	return FALSE;
}

static DISK_VOL_INFO*
AppendVolumeInfo(PHY_DRIVE_INFO* pInfo, LPCWSTR lpszVolume)
{
	DISK_VOL_INFO* pVol = FindVolumeInfo(pInfo, lpszVolume);
	if (pVol)
		return pVol;
	if (pInfo->VolCount % 4 == 0)
	{
		DISK_VOL_INFO* volInfo = realloc(pInfo->VolInfo,
			sizeof(DISK_VOL_INFO) * (pInfo->VolCount + 4));
		if (!volInfo)
			return NULL;
		pInfo->VolInfo = volInfo;
	}
	ZeroMemory(&pInfo->VolInfo[pInfo->VolCount], sizeof(DISK_VOL_INFO));
	FillVolumeInfo(&pInfo->VolInfo[pInfo->VolCount], lpszVolume, pInfo);
	pInfo->VolCount++;
	return &pInfo->VolInfo[pInfo->VolCount - 1];
}

static BOOL
AppendVolumeByDrive(PHY_DRIVE_INFO* pInfo, DWORD dwCount, DWORD dwDrive, LPCWSTR lpszVolume)
{
	PHY_DRIVE_INFO* pDrive = FindDriveByIndex(pInfo, dwCount, dwDrive);
	if (!pDrive)
		return FALSE;
	return AppendVolumeInfo(pDrive, lpszVolume) != NULL;
}

static BOOL
AppendVolumeByExtents(PHY_DRIVE_INFO* pInfo, DWORD dwCount, VOLUME_DISK_EXTENTS* pExtents, LPCWSTR lpszVolume)
{
	BOOL bAdded = FALSE;

	for (DWORD i = 0; i < pExtents->NumberOfDiskExtents; i++)
	{
		PHY_DRIVE_INFO* pDrive = FindDriveByIndex(pInfo, dwCount, pExtents->Extents[i].DiskNumber);
		if (pDrive)
		{
			DISK_VOL_INFO* pVol = AppendVolumeInfo(pDrive, lpszVolume);
			DISK_VOL_INFO* pPart = FindPartitionByExtent(pDrive, &pExtents->Extents[i]);
			if (pVol && pPart)
				CopyPartitionInfo(pVol, pPart);
			if (pVol)
				bAdded = TRUE;
		}
	}
	return bAdded;
}

static BOOL
AppendPartitionOnlyInfo(PHY_DRIVE_INFO* pInfo, const DISK_VOL_INFO* pPart)
{
	if (HasVolumeForPartition(pInfo, pPart))
		return TRUE;
	if (pInfo->VolCount % 4 == 0)
	{
		DISK_VOL_INFO* volInfo = realloc(pInfo->VolInfo,
			sizeof(DISK_VOL_INFO) * (pInfo->VolCount + 4));
		if (!volInfo)
			return FALSE;
		pInfo->VolInfo = volInfo;
	}
	ZeroMemory(&pInfo->VolInfo[pInfo->VolCount], sizeof(DISK_VOL_INFO));
	CopyPartitionInfo(&pInfo->VolInfo[pInfo->VolCount], pPart);
	pInfo->VolCount++;
	return TRUE;
}

static VOID
AppendMissingPartitionInfo(PHY_DRIVE_INFO* pInfo, DWORD dwCount)
{
	for (DWORD i = 0; i < dwCount; i++)
	{
		for (DWORD j = 0; j < pInfo[i].PartCount; j++)
			AppendPartitionOnlyInfo(&pInfo[i], &pInfo[i].PartInfo[j]);
	}
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

static BOOL
AppendPartitionEntry(PHY_DRIVE_INFO* pInfo, PARTITION_INFORMATION_EX* pPartInfo)
{
	if (!IsValidPartitionEntry(pPartInfo))
		return TRUE;
	if (pInfo->PartCount % 4 == 0)
	{
		DISK_VOL_INFO* partInfo = realloc(pInfo->PartInfo,
			sizeof(DISK_VOL_INFO) * (pInfo->PartCount + 4));
		if (!partInfo)
			return FALSE;
		pInfo->PartInfo = partInfo;
	}
	ZeroMemory(&pInfo->PartInfo[pInfo->PartCount], sizeof(DISK_VOL_INFO));
	if (!FillPartitionInfoFromEntry(&pInfo->PartInfo[pInfo->PartCount], pPartInfo, pInfo))
		return FALSE;
	pInfo->PartCount++;
	return TRUE;
}

static BOOL
GetDiskPartMap(HANDLE hDisk, BOOL bIsCdRom, PHY_DRIVE_INFO* pInfo)
{
	DRIVE_LAYOUT_INFORMATION_EX* pLayout = (VOID*)NWLC->NwBuf;
	DWORD dwBytes = NWINFO_BUFSZ;
	DWORD dwEntryOffset = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry);
	DWORD dwEntryCount;

	if (bIsCdRom)
		goto fail;

	if (!DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0,
		pLayout, dwBytes, &dwBytes, NULL))
		goto fail;
	if (dwBytes < dwEntryOffset)
		goto fail;
	dwEntryCount = (dwBytes - dwEntryOffset) / sizeof(PARTITION_INFORMATION_EX);
	if (dwEntryCount > pLayout->PartitionCount)
		dwEntryCount = pLayout->PartitionCount;

	pInfo->PartMap = pLayout->PartitionStyle;
	switch (pInfo->PartMap)
	{
	case PARTITION_STYLE_MBR:
		if (pLayout->PartitionCount >= 4)
		{
			int i;
			for (i = 0; i < 4; i++)
				pInfo->MbrLba[i] = (DWORD)(pLayout->PartitionEntry[i].StartingOffset.QuadPart >> 9);
		}
		memcpy(pInfo->MbrSignature, &pLayout->Mbr.Signature, sizeof(DWORD));
		break;
	case PARTITION_STYLE_GPT:
		memcpy(pInfo->GptGuid, &pLayout->Gpt.DiskId, sizeof(GUID));
		break;
	default:
		goto fail;
	}
	for (DWORD i = 0; i < dwEntryCount; i++)
		AppendPartitionEntry(pInfo, &pLayout->PartitionEntry[i]);
	return TRUE;

fail:
	pInfo->PartMap = PARTITION_STYLE_RAW;
	return FALSE;
}

static BOOL
CheckSsd(HANDLE hDisk, PHY_DRIVE_INFO* pInfo)
{
	if (NWLC->NwOsInfo.dwMajorVersion >= 6)
	{
		DWORD dwBytes;
		STORAGE_PROPERTY_QUERY propQuery = { .QueryType = PropertyStandardQuery, .PropertyId = StorageDeviceSeekPenaltyProperty };
		DEVICE_SEEK_PENALTY_DESCRIPTOR dspd = { 0 };
		if (hDisk && hDisk != INVALID_HANDLE_VALUE)
		{
			if (DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &propQuery, sizeof(propQuery),
				&dspd, sizeof(dspd), &dwBytes, NULL))
			{
				return (dspd.IncursSeekPenalty == FALSE);
			}
		}
	}
	if (pInfo->BusType == BusTypeNvme)
		return TRUE;
	return FALSE;
}

static VOID
RemoveTrailingBackslash(WCHAR* lpszPath)
{
	size_t len = wcslen(lpszPath);
	if (len < 1 || lpszPath[len - 1] != L'\\')
		return;
	lpszPath[len - 1] = L'\0';
}

static VOID
TrimString(CHAR* str)
{
	char* p1 = str;
	char* p2 = str;
	size_t len = strlen(str);
	while (len > 0)
	{
		if (!isblank(str[len - 1]))
			break;
		str[len - 1] = '\0';
		len--;
	}
	while (isblank(*p1))
		p1++;
	while (*p1)
	{
		if (!isprint(*p1))
			*p1 = '?';
		*p2++ = *p1++;
	}
	*p2++ = 0;

}

typedef struct
{
	DWORD  cbSize;
	WCHAR  DevicePath[512];
} MY_DEVIF_DETAIL_DATA;

DWORD NWL_GetDriveCount(BOOL bIsCdRom)
{
	DWORD dwCount = 0;
	SP_DEVICE_INTERFACE_DATA ifData = { .cbSize = sizeof(SP_DEVICE_INTERFACE_DATA) };
	GUID devGuid = bIsCdRom ? GUID_DEVINTERFACE_CDROM : GUID_DEVINTERFACE_DISK;
	HDEVINFO hDevInfo = SetupDiGetClassDevsW(&devGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
		return 0;

	while (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &devGuid, dwCount, &ifData))
		dwCount++;

	SetupDiDestroyDeviceInfoList(hDevInfo);
	return dwCount;
}

DWORD NWL_GetDriveInfoList(BOOL bIsCdRom, BOOL bGetVolume, PHY_DRIVE_INFO** pDriveList)
{
	DWORD i;
	BOOL bRet;
	DWORD dwBytes;
	PHY_DRIVE_INFO* pInfo;

	HANDLE hSearch;
	WCHAR cchVolume[MAX_PATH];

	DWORD dwCount = 0;
	SP_DEVICE_INTERFACE_DATA ifData = { .cbSize = sizeof(SP_DEVICE_INTERFACE_DATA) };
	MY_DEVIF_DETAIL_DATA detailData = { .cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) };
	SP_DEVINFO_DATA infoData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
	GUID devGuid = bIsCdRom ? GUID_DEVINTERFACE_CDROM : GUID_DEVINTERFACE_DISK;
	HDEVINFO hDevInfo = SetupDiGetClassDevsW(&devGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
		return 0;

	while (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &devGuid, dwCount, &ifData))
		dwCount++;
	if (dwCount == 0)
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return 0;
	}

	*pDriveList = calloc(dwCount, sizeof(PHY_DRIVE_INFO));
	if (!*pDriveList)
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return 0;
	}
	pInfo = *pDriveList;

	for (i = 0; i < dwCount; i++)
	{
		HANDLE hDrive = INVALID_HANDLE_VALUE;
		STORAGE_PROPERTY_QUERY Query = { 0 };
		STORAGE_DESCRIPTOR_HEADER DevDescHeader = { 0 };
		STORAGE_DEVICE_DESCRIPTOR* pDevDesc = NULL;
		STORAGE_DEVICE_NUMBER sdn;
		HANDLE hIfDev;

		if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &devGuid, i, &ifData))
			goto next_drive;
		if (!SetupDiGetDeviceInterfaceDetailW(hDevInfo, &ifData, (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)&detailData,
			sizeof(MY_DEVIF_DETAIL_DATA), NULL, &infoData))
			goto next_drive;
		hIfDev = CreateFileW(detailData.DevicePath,
			GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hIfDev == INVALID_HANDLE_VALUE || hIfDev == NULL)
			goto next_drive;
		bRet = DeviceIoControl(hIfDev, IOCTL_STORAGE_GET_DEVICE_NUMBER,
			NULL, 0, &sdn, (DWORD)(sizeof(STORAGE_DEVICE_NUMBER)),
			&dwBytes, NULL);
		CloseHandle(hIfDev);
		if (bRet == FALSE)
			goto next_drive;
		pInfo[i].Index = sdn.DeviceNumber;

		if (SetupDiGetDeviceInstanceIdW(hDevInfo, &infoData, NWLC->NwBufW, NWINFO_BUFSZB, NULL))
			wcsncpy_s(pInfo[i].HwID, MAX_PATH, NWLC->NwBufW, _TRUNCATE);
		if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &infoData, SPDRP_FRIENDLYNAME,
			NULL, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL) ||
			SetupDiGetDeviceRegistryPropertyW(hDevInfo, &infoData, SPDRP_DEVICEDESC,
				NULL, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL))
			wcsncpy_s(pInfo[i].HwName, MAX_PATH, NWLC->NwBufW, _TRUNCATE);

		hDrive = NWL_GetDiskHandleById(bIsCdRom, FALSE, pInfo[i].Index);
		pInfo[i].Handle = hDrive;
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
		if (!bIsCdRom)
			pInfo[i].Ssd = CheckSsd(hDrive, &pInfo[i]);

		if (pDevDesc->VendorIdOffset)
		{
			strncpy_s(pInfo[i].VendorId, MAX_PATH,
				(char*)pDevDesc + pDevDesc->VendorIdOffset, _TRUNCATE);
			TrimString(pInfo[i].VendorId);
		}

		if (pDevDesc->ProductIdOffset)
		{
			strncpy_s(pInfo[i].ProductId, MAX_PATH,
				(char*)pDevDesc + pDevDesc->ProductIdOffset, _TRUNCATE);
			TrimString(pInfo[i].ProductId);
		}

		if (pDevDesc->ProductRevisionOffset)
		{
			strncpy_s(pInfo[i].ProductRev, MAX_PATH,
				(char*)pDevDesc + pDevDesc->ProductRevisionOffset, _TRUNCATE);
			TrimString(pInfo[i].ProductRev);
		}

		if (pDevDesc->SerialNumberOffset)
		{
			strncpy_s(pInfo[i].SerialNumber, MAX_PATH,
				(char*)pDevDesc + pDevDesc->SerialNumberOffset, _TRUNCATE);
			TrimString(pInfo[i].SerialNumber);
		}

		GetDiskPartMap(hDrive, bIsCdRom, &pInfo[i]);

next_drive:
		if (pDevDesc)
			free(pDevDesc);
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);

	if (!bGetVolume)
		return dwCount;

	for (bRet = TRUE, hSearch = FindFirstVolumeW(cchVolume, MAX_PATH);
		bRet && hSearch != INVALID_HANDLE_VALUE;
		bRet = FindNextVolumeW(hSearch, cchVolume, MAX_PATH))
	{
		HANDLE hVolume;
		RemoveTrailingBackslash(cchVolume);
		hVolume = CreateFileW(cchVolume, 0,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (!hVolume || hVolume == INVALID_HANDLE_VALUE)
			continue;
		if (!bIsCdRom)
		{
			VOLUME_DISK_EXTENTS* pExtents = GetVolumeDiskExtents(hVolume);
			if (pExtents)
			{
				BOOL bAdded = AppendVolumeByExtents(pInfo, dwCount, pExtents, cchVolume);
				free(pExtents);
				if (bAdded)
				{
					CloseHandle(hVolume);
					continue;
				}
			}
		}

		if (GetDriveByVolume(bIsCdRom, hVolume, &dwBytes))
			AppendVolumeByDrive(pInfo, dwCount, dwBytes, cchVolume);
		CloseHandle(hVolume);
	}

	if (hSearch != INVALID_HANDLE_VALUE)
		FindVolumeClose(hSearch);

	AppendMissingPartitionInfo(pInfo, dwCount);

	return dwCount;
}

VOID
NWL_DestoryDriveInfoList(PHY_DRIVE_INFO* pInfo, DWORD dwCount)
{
	if (pInfo == NULL)
		return;
	for (DWORD i = 0; i < dwCount; i++)
	{
		if (pInfo[i].VolInfo != NULL)
		{
			for (DWORD j = 0; j < pInfo[i].VolCount; j++)
			{
				free(pInfo[i].VolInfo[j].VolNames);
			}
			free(pInfo[i].VolInfo);
		}
		free(pInfo[i].PartInfo);
		if (pInfo[i].Handle && pInfo[i].Handle != INVALID_HANDLE_VALUE)
			CloseHandle(pInfo[i].Handle);
	}
	free(pInfo);
}

static VOID
PrintSmartInfo(PNODE node, CDI_SMART* ptr, INT index)
{
	INT n;
	DWORD d;
	DWORD i, count;
	WCHAR* str;
	BOOL ssd = FALSE;
	BOOL nvme = FALSE;

	if (index < 0)
		return;
	cdi_update_smart(ptr, index);

	n = cdi_get_int(ptr, index, CDI_INT_TEMPERATURE);
	if (n >= -50)
		NWL_NodeAttrSetf(node, NWL_GetTemperatureLabel(), NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature((float)n));

	n = cdi_get_int(ptr, index, CDI_INT_LIFE);
	if (n >= 0)
		NWL_NodeAttrSetf(node, "Health Status", 0, "%s (%d%%)", cdi_get_health_status(cdi_get_int(ptr, index, CDI_INT_DISK_STATUS)), n);
	else
		NWL_NodeAttrSet(node, "Health Status", cdi_get_health_status(cdi_get_int(ptr, index, CDI_INT_DISK_STATUS)), 0);

	str = cdi_get_string(ptr, index, CDI_STRING_SN);
	NWL_NodeAttrSet(node, "Serial Number", NWL_Ucs2ToUtf8(str), NAFLG_FMT_SENSITIVE);
	cdi_free_string(str);

	str = cdi_get_string(ptr, index, CDI_STRING_TRANSFER_MODE_CUR);
	NWL_NodeAttrSet(node, "Current Transfer Mode", NWL_Ucs2ToUtf8(str), 0);
	cdi_free_string(str);

	str = cdi_get_string(ptr, index, CDI_STRING_TRANSFER_MODE_MAX);
	NWL_NodeAttrSet(node, "Max Transfer Mode", NWL_Ucs2ToUtf8(str), 0);
	cdi_free_string(str);

	str = cdi_get_string(ptr, index, CDI_STRING_VERSION_MAJOR);
	NWL_NodeAttrSet(node, "Standard", NWL_Ucs2ToUtf8(str), 0);
	cdi_free_string(str);

	{
		char* features = NULL;
		if (cdi_get_bool(ptr, index, CDI_BOOL_SMART))
			NWL_NodeAppendMultiSz(&features, "S.M.A.R.T.");
		if (cdi_get_bool(ptr, index, CDI_BOOL_LBA48))
			NWL_NodeAppendMultiSz(&features, "48bit LBA");
		if (cdi_get_bool(ptr, index, CDI_BOOL_AAM))
			NWL_NodeAppendMultiSz(&features, "AAM");
		if (cdi_get_bool(ptr, index, CDI_BOOL_APM))
			NWL_NodeAppendMultiSz(&features, "APM");
		if (cdi_get_bool(ptr, index, CDI_BOOL_NCQ))
			NWL_NodeAppendMultiSz(&features, "NCQ");
		if (cdi_get_bool(ptr, index, CDI_BOOL_NV_CACHE))
			NWL_NodeAppendMultiSz(&features, "NV Cache");
		if (cdi_get_bool(ptr, index, CDI_BOOL_DEVSLP))
			NWL_NodeAppendMultiSz(&features, "DEVSLP");
		if (cdi_get_bool(ptr, index, CDI_BOOL_STREAMING))
			NWL_NodeAppendMultiSz(&features, "Streaming");
		if (cdi_get_bool(ptr, index, CDI_BOOL_GPL))
			NWL_NodeAppendMultiSz(&features, "GPL");
		if (cdi_get_bool(ptr, index, CDI_BOOL_TRIM))
			NWL_NodeAppendMultiSz(&features, "TRIM");
		if (cdi_get_bool(ptr, index, CDI_BOOL_VOLATILE_WRITE_CACHE))
			NWL_NodeAppendMultiSz(&features, "VolatileWriteCache");
		NWL_NodeAttrSetMulti(node, "Features", features, 0);
		free(features);
	}

	ssd = cdi_get_bool(ptr, index, CDI_BOOL_SSD);
	NWL_NodeAttrSetBool(node, "SSD", ssd, 0);

	if (ssd)
	{
		nvme = cdi_get_bool(ptr, index, CDI_BOOL_SSD_NVME);
		n = cdi_get_int(ptr, index, CDI_INT_HOST_READS);
		if (n < 0)
			NWL_NodeAttrSet(node, "Total Host Reads", "-", 0);
		else
			NWL_NodeAttrSetf(node, "Total Host Reads", 0, "%d GB", n);

		n = cdi_get_int(ptr, index, CDI_INT_HOST_WRITES);
		if (n < 0)
			NWL_NodeAttrSet(node, "Total Host Writes", "-", 0);
		else
			NWL_NodeAttrSetf(node, "Total Host Writes", 0, "%d GB", n);
		if (nvme == FALSE)
		{
			n = cdi_get_int(ptr, index, CDI_INT_NAND_WRITES);
			if (n < 0)
				NWL_NodeAttrSet(node, "Total NAND Writes", "-", 0);
			else
				NWL_NodeAttrSetf(node, "Total NAND Writes", 0, "%d GB", n);
		}
	}
	else
	{
		d = cdi_get_dword(ptr, index, CDI_DWORD_BUFFER_SIZE);
		if (d >= 10 * 1024 * 1024) // 10 MB
			NWL_NodeAttrSetf(node, "Buffer Size", 0, "%lu MB", d / 1024 / 1024);
		else if (d > 1024)
			NWL_NodeAttrSetf(node, "Buffer Size", 0, "%lu KB", d / 1024);
		else
			NWL_NodeAttrSetf(node, "Buffer Size", 0, "%lu B", d);

		NWL_NodeAttrSetf(node, "Rotation Rate (RPM)",
			NAFLG_FMT_NUMERIC, "%lu", cdi_get_dword(ptr, index, CDI_DWORD_ROTATION_RATE));
	}

	NWL_NodeAttrSetf(node, "Power On Count",
		NAFLG_FMT_NUMERIC, "%lu", cdi_get_dword(ptr, index, CDI_DWORD_POWER_ON_COUNT));

	n = cdi_get_int(ptr, index, CDI_INT_POWER_ON_HOURS);
	if (n < 0)
		NWL_NodeAttrSet(node, "Power On Time (Hours)", "-", 0);
	else
		NWL_NodeAttrSetf(node, "Power On Time (Hours)",
			NAFLG_FMT_NUMERIC, "%d", n);

	if (cdi_get_bool(ptr, index, CDI_BOOL_AAM))
	{
		NWL_NodeAttrSetf(node, "Current AAM", NAFLG_FMT_NUMERIC, "%u", cdi_get_current_aam(ptr, index));
		NWL_NodeAttrSetf(node, "Recommended AAM", NAFLG_FMT_NUMERIC, "%u", cdi_get_recommend_aam(ptr, index));
	}
	if (cdi_get_bool(ptr, index, CDI_BOOL_APM))
	{
		NWL_NodeAttrSetf(node, "Current APM", NAFLG_FMT_NUMERIC, "%u", cdi_get_current_apm(ptr, index));
		NWL_NodeAttrSetf(node, "Recommended APM", NAFLG_FMT_NUMERIC, "%u", cdi_get_recommend_apm(ptr, index));
	}

	count = cdi_get_dword(ptr, index, CDI_DWORD_ATTR_COUNT);
	if (count)
	{
		str = cdi_get_smart_format(ptr, index);
		NWL_NodeAttrSet(node, "SMART Format", NWL_Ucs2ToUtf8(str), 0);
		cdi_free_string(str);
	}
	for (i = 0; i < count; i++)
	{
		char key[] = "SMART XX";
		char name[64];
		BYTE id = cdi_get_smart_id(ptr, index, i);
		if (id == 0)
			continue;
		str = cdi_get_smart_name(ptr, index, id);
		strncpy_s(name, sizeof(name), NWL_Ucs2ToUtf8(str), _TRUNCATE);
		cdi_free_string(str);
		str = cdi_get_smart_value(ptr, index, i, TRUE);
		snprintf(key, sizeof(key), "SMART %02X", id);
		NWL_NodeAttrSetf(node, key, 0, "%s %s", NWL_Ucs2ToUtf8(str), name);
		cdi_free_string(str);
	}
}

int __cdecl
NWL_CompareDiskId(const void* a, const void* b)
{
	return ((int)((const PHY_DRIVE_INFO*)a)->Index) - ((int)((const PHY_DRIVE_INFO*)b)->Index);
}

static BOOL
MatchBusType(STORAGE_BUS_TYPE bus)
{
	if ((NWLC->DiskFlags & NW_DISK_PHYS) && (bus == BusTypeVirtual || bus == BusTypeFileBackedVirtual))
		return FALSE;
	if (!(NWLC->DiskFlags & (NW_DISK_NVME | NW_DISK_SATA | NW_DISK_SCSI | NW_DISK_SAS | NW_DISK_USB)))
		return TRUE;
	if ((NWLC->DiskFlags & NW_DISK_NVME) && bus == BusTypeNvme)
		return TRUE;
	if ((NWLC->DiskFlags & NW_DISK_SATA) && bus == BusTypeSata)
		return TRUE;
	if ((NWLC->DiskFlags & NW_DISK_SCSI) && bus == BusTypeScsi)
		return TRUE;
	if ((NWLC->DiskFlags & NW_DISK_SAS) && bus == BusTypeSas)
		return TRUE;
	if ((NWLC->DiskFlags & NW_DISK_USB) && bus == BusTypeUsb)
		return TRUE;
	return FALSE;
}

static VOID
PrintDiskInfo(BOOL cdrom, BOOL volinfo, PNODE node, CDI_SMART* smart)
{
	PHY_DRIVE_INFO* PhyDriveList = NULL;
	DWORD PhyDriveCount = 0;
	INT SmartCount = 0;
	CHAR DiskPath[64];
	PhyDriveCount = NWL_GetDriveInfoList(cdrom, volinfo, &PhyDriveList);
	if (!(NWLC->DiskFlags & NW_DISK_NO_SMART) && smart && !cdrom)
		SmartCount = cdi_get_disk_count(smart);
	if (PhyDriveCount == 0)
		goto out;
	qsort(PhyDriveList, PhyDriveCount, sizeof(PHY_DRIVE_INFO), NWL_CompareDiskId);
	for (DWORD i = 0; i < PhyDriveCount; i++)
	{
		snprintf(DiskPath, sizeof(DiskPath),
			cdrom ? "\\\\.\\CdRom%lu" : "\\\\.\\PhysicalDrive%lu", PhyDriveList[i].Index);
		if (NWLC->DiskPath && _stricmp(NWLC->DiskPath, DiskPath) != 0)
			continue;
		if (!MatchBusType(PhyDriveList[i].BusType))
			continue;
		PNODE nd = NWL_NodeAppendNew(node, "Disk", NFLG_TABLE_ROW);
		NWL_NodeAttrSet(nd, "Path", DiskPath, 0);
		if (PhyDriveList[i].HwID[0])
			NWL_NodeAttrSet(nd, "HWID", NWL_Ucs2ToUtf8(PhyDriveList[i].HwID), 0);
		if (PhyDriveList[i].HwName[0])
			NWL_NodeAttrSet(nd, "HW Name", NWL_Ucs2ToUtf8(PhyDriveList[i].HwName), 0);
		if (PhyDriveList[i].VendorId[0])
			NWL_NodeAttrSet(nd, "Vendor ID", PhyDriveList[i].VendorId, 0);
		if (PhyDriveList[i].ProductId[0])
			NWL_NodeAttrSet(nd, "Product ID", PhyDriveList[i].ProductId, 0);
		if (PhyDriveList[i].ProductRev[0])
			NWL_NodeAttrSet(nd, "Product Rev", PhyDriveList[i].ProductRev, 0);
		if (PhyDriveList[i].SerialNumber[0])
		{
			NWL_NodeAttrSet(nd, "Serial Number", PhyDriveList[i].SerialNumber, NAFLG_FMT_SENSITIVE);
			NWL_NodeAttrSet(nd, "Serial Number (Raw)", PhyDriveList[i].SerialNumber, NAFLG_FMT_SENSITIVE);
		}
		NWL_NodeAttrSet(nd, "Type", NWL_GetBusTypeString(PhyDriveList[i].BusType), 0);
		NWL_NodeAttrSetBool(nd, "Removable", PhyDriveList[i].RemovableMedia, 0);
		NWL_NodeAttrSet(nd, "Size",
			NWL_GetHumanSize(PhyDriveList[i].SizeInBytes, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		switch (PhyDriveList[i].PartMap)
		{
		case PARTITION_STYLE_MBR:
			NWL_NodeAttrSet(nd, "Partition Table", "MBR", 0);
			NWL_NodeAttrSetf(nd, "MBR Signature", 0, "%02X %02X %02X %02X",
				PhyDriveList[i].MbrSignature[0], PhyDriveList[i].MbrSignature[1],
				PhyDriveList[i].MbrSignature[2], PhyDriveList[i].MbrSignature[3]);
			break;
		case PARTITION_STYLE_GPT:
			NWL_NodeAttrSet(nd, "Partition Table", "GPT", 0);
			NWL_NodeAttrSetf(nd, "GPT GUID", NAFLG_FMT_GUID, "{%s}", NWL_GuidToStr(PhyDriveList[i].GptGuid));
			break;
		}
		if (cdrom)
		{
			NWL_NodeAttrSetf(nd, "Short Name", 0, "CD%lu", i);
		}
		else
		{
			NWL_NodeAttrSetf(nd, "Short Name", 0, "%s%lu", PhyDriveList[i].RemovableMedia ? "RM" : "HD", i);
			NWL_NodeAttrSetBool(nd, "SSD", PhyDriveList[i].Ssd, 0);
		}
		for (INT j = 0; j < SmartCount; j++)
		{
			if (cdi_get_int(smart, j, CDI_INT_DISK_ID) == PhyDriveList[i].Index)
				PrintSmartInfo(nd, smart, j);
		}
		if (PhyDriveList[i].VolCount)
		{
			PNODE nv = NWL_NodeAppendNew(nd, "Volumes", NFLG_TABLE);
			for (DWORD j = 0; j < PhyDriveList[i].VolCount; j++)
			{
				PNODE vol = NWL_NodeAppendNew(nv, "Volume", NFLG_TABLE_ROW);
				PrintVolumeInfo(vol, &PhyDriveList[i].VolInfo[j]);
			}
		}
	}

out:
	NWL_DestoryDriveInfoList(PhyDriveList, PhyDriveCount);
	if (NWLC->DiskPath)
		return;
	for (INT i = 0; i < SmartCount; i++)
	{
		if (cdi_get_int(smart, i, CDI_INT_DISK_ID) >= 0)
			continue;
		STORAGE_BUS_TYPE bus = cdi_get_int(smart, i, CDI_INT_INTERFACE_TYPE);
		if (!MatchBusType(bus))
			continue;
		PNODE nd = NWL_NodeAppendNew(node, "Disk", NFLG_TABLE_ROW);
		WCHAR* str = cdi_get_string(smart, i, CDI_STRING_MODEL);
		NWL_NodeAttrSet(nd, "HW Name", NWL_Ucs2ToUtf8(str), 0);
		NWL_NodeAttrSet(nd, "Product ID", NWL_Ucs2ToUtf8(str), 0);
		cdi_free_string(str);

		NWL_NodeAttrSet(nd, "Type", NWL_GetBusTypeString(bus), 0);

		NWL_NodeAttrSet(nd, "Size",
			NWL_GetHumanSize(1000ULL * 1000ULL * cdi_get_dword(smart, i, CDI_DWORD_DISK_SIZE), NWLC->NwUnits, 1024),
			NAFLG_FMT_HUMAN_SIZE);

		NWL_NodeAttrSetf(nd, "Short Name", 0, "HD-%d", i);

		PrintSmartInfo(nd, smart, i);
	}
}

PNODE NW_Disk(BOOL bAppend)
{
	PNODE node = NWL_NodeAlloc("Disks", NFLG_TABLE);
	if (bAppend)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	BOOL volinfo = !(NWLC->DiskFlags & NW_DISK_NO_VOL);
	if (!(NWLC->DiskFlags & NW_DISK_NO_SMART) && NWLC->NwSmart && NWLC->NwSmartInit == FALSE)
	{
		cdi_init_smart(NWLC->NwSmart, NWLC->NwSmartFlags);
		NWLC->NwSmartInit = TRUE;
	}
	if (!(NWLC->DiskFlags & NW_DISK_DEFAULT))
		NWLC->DiskFlags |= NW_DISK_DEFAULT;
	if (NWLC->DiskFlags & NW_DISK_HD)
		PrintDiskInfo(FALSE, volinfo, node, NWLC->NwSmart);
	if (NWLC->DiskFlags & NW_DISK_CD)
		PrintDiskInfo(TRUE, volinfo, node, NWLC->NwSmart);
	return node;
}
