// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "disk.h"
#include "utils.h"
#include "../libcdi/libcdi.h"

static const char* d_human_sizes[6] =
{ "B", "KB", "MB", "GB", "TB", "PB", };

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

static LPCSTR
GetGptFlag(GUID* pGuid)
{
	LPCSTR lpszGuid = NWL_WinGuidToStr(FALSE, pGuid);
	if (_stricmp(lpszGuid, "c12a7328-f81f-11d2-ba4b-00a0c93ec93b") == 0)
		return "ESP";
	else if (_stricmp(lpszGuid, "e3c9e316-0b5c-4db8-817d-f92df00215ae") == 0)
		return "MSR";
	else if (_stricmp(lpszGuid, "de94bba4-06d1-4d40-a16a-bfd50179d6ac") == 0)
		return "WINRE";
	else if (_stricmp(lpszGuid, "21686148-6449-6e6f-744e-656564454649") == 0)
		return "BIOS";
	else if (_stricmp(lpszGuid, "024dee41-33e7-11d3-9d69-0008c781f39f") == 0)
		return "MBR";
	return "DATA";
}

static LPCSTR
GetMbrFlag(LONGLONG llStartingOffset, PHY_DRIVE_INFO* pParent)
{
	INT i;
	LONGLONG llLba = llStartingOffset >> 9;
	for (i = 0; i < 4; i++)
	{
		if (llLba == pParent->MbrLba[i])
			return "PRIMARY";
	}
	return "EXTENDED";
}

static void
PrintPartitionInfo(PNODE pNode, LPCSTR lpszPath, PHY_DRIVE_INFO* pParent)
{
	BOOL bRet;
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	PARTITION_INFORMATION_EX partInfo = { 0 };
	DWORD dwPartInfo = sizeof(PARTITION_INFORMATION_EX);
	hDevice = CreateFileA(lpszPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (!hDevice || hDevice == INVALID_HANDLE_VALUE)
		return;
	bRet = DeviceIoControl(hDevice, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, &partInfo, dwPartInfo, &dwPartInfo, NULL);
	CloseHandle(hDevice);
	if (!bRet)
		return;
	if (partInfo.StartingOffset.QuadPart == 0) // CDROM
		return;
	NWL_NodeAttrSetf(pNode, "Starting LBA", 0, "%llu", partInfo.StartingOffset.QuadPart >> 9);
	//NWL_NodeAttrSetf(pNode, "Partition Number", 0, "%u", partInfo.PartitionNumber);
	switch (partInfo.PartitionStyle)
	{
	case PARTITION_STYLE_MBR:
		NWL_NodeAttrSetf(pNode, "Partition Type", 0, "0x%02X", partInfo.Mbr.PartitionType);
		NWL_NodeAttrSet(pNode, "Partition ID", NWL_WinGuidToStr(TRUE, &partInfo.Mbr.PartitionId), NAFLG_FMT_GUID);
		NWL_NodeAttrSetBool(pNode, "Boot Indicator", partInfo.Mbr.BootIndicator, 0);
		NWL_NodeAttrSet(pNode, "Partition Flag", GetMbrFlag(partInfo.StartingOffset.QuadPart, pParent), 0);
		break;
	case PARTITION_STYLE_GPT:
		NWL_NodeAttrSet(pNode, "Partition Type", NWL_WinGuidToStr(TRUE, &partInfo.Gpt.PartitionType), NAFLG_FMT_GUID);
		NWL_NodeAttrSet(pNode, "Partition ID", NWL_WinGuidToStr(TRUE, &partInfo.Gpt.PartitionId), NAFLG_FMT_GUID);
		NWL_NodeAttrSet(pNode, "Partition Flag", GetGptFlag(&partInfo.Gpt.PartitionType), 0);
		break;
	}
}

static void
PrintVolumeInfo(PNODE pNode, LPCSTR lpszVolume, PHY_DRIVE_INFO* pParent)
{
	CHAR cchLabel[MAX_PATH];
	CHAR cchFs[MAX_PATH];
	CHAR cchPath[MAX_PATH];
	LPCH lpszVolumePathNames = NULL;
	DWORD dwSize = 0;
	ULARGE_INTEGER ulFreeSpace = { 0 };
	ULARGE_INTEGER ulTotalSpace = { 0 };

	snprintf(cchPath, MAX_PATH, "%s\\", lpszVolume);
	NWL_NodeAttrSet(pNode, "Path", GetRealVolumePath(lpszVolume), 0);
	NWL_NodeAttrSet(pNode, "Volume GUID", lpszVolume, 0);
	PrintPartitionInfo(pNode, lpszVolume, pParent);
	if (GetVolumeInformationA(cchPath, cchLabel, MAX_PATH, NULL, NULL, NULL, cchFs, MAX_PATH))
	{
		NWL_NodeAttrSet(pNode, "Label", cchLabel, 0);
		NWL_NodeAttrSet(pNode, "Filesystem", cchFs, 0);
	}
	if (GetDiskFreeSpaceExA(cchPath, NULL, NULL, &ulFreeSpace))
		NWL_NodeAttrSet(pNode, "Free Space",
			NWL_GetHumanSize(ulFreeSpace.QuadPart, d_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
	if (GetDiskFreeSpaceExA(cchPath, NULL, &ulTotalSpace, NULL))
		NWL_NodeAttrSet(pNode, "Total Space",
			NWL_GetHumanSize(ulTotalSpace.QuadPart, d_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
	if (ulTotalSpace.QuadPart != 0)
		NWL_NodeAttrSetf(pNode, "Usage", 0, "%.2f%%",
			100.0 - (ulFreeSpace.QuadPart * 100.0) / ulTotalSpace.QuadPart);
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

		hDrive = NWL_GetDiskHandleById(bIsCdRom, FALSE, i);

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
				pInfo[i].MbrLba[j] = MBR->entries[j].start;
				if (MBR->entries[j].type == 0xee)
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

static INT
GetSmartIndex(CDI_SMART* smart, DWORD Id)
{
	INT i, count;
	count = cdi_get_disk_count(smart);
	for (i = 0; i < count; i++)
	{
		if (cdi_get_int(smart, i, CDI_INT_DISK_ID) == Id)
			return i;
	}
	return -1;
}

static VOID
PrintSmartInfo(PNODE node, CDI_SMART* ptr, INT index)
{
	INT n;
	DWORD d;
	DWORD i, count;
	CHAR* str;
	BOOL ssd = FALSE;
	BOOL nvme = FALSE;
	CDI_SMART_ATTRIBUTE* attr;

	if (index < 0)
		return;
	cdi_update_smart(ptr, index);

	NWL_NodeAttrSetf(node, "Temperature (C)", NAFLG_FMT_NUMERIC, "%d", cdi_get_int(ptr, index, CDI_INT_TEMPERATURE));

	n = cdi_get_int(ptr, index, CDI_INT_LIFE);
	str = cdi_get_string(ptr, index, CDI_STRING_DISK_STATUS);
	if (n >= 0)
		NWL_NodeAttrSetf(node, "Health Status", 0, "%s (%d%%)", str, n);
	else
		NWL_NodeAttrSet(node, "Health Status", str, 0);
	cdi_free_string(str);

	str = cdi_get_string(ptr, index, CDI_STRING_TRANSFER_MODE_CUR);
	NWL_NodeAttrSet(node, "Current Transfer Mode", str, 0);
	cdi_free_string(str);

	str = cdi_get_string(ptr, index, CDI_STRING_TRANSFER_MODE_MAX);
	NWL_NodeAttrSet(node, "Max Transfer Mode", str, 0);
	cdi_free_string(str);

	str = cdi_get_string(ptr, index, CDI_STRING_VERSION_MAJOR);
	NWL_NodeAttrSet(node, "Standard", str, 0);
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
			NWL_NodeAttrSetf(node, "Buffer Size", 0, "%d MB", d / 1024 / 1024);
		else if (d > 1024)
			NWL_NodeAttrSetf(node, "Buffer Size", 0, "%d KB", d / 1024);
		else
			NWL_NodeAttrSetf(node, "Buffer Size", 0, "%d B", d);

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

	count = cdi_get_dword(ptr, index, CDI_DWORD_ATTR_COUNT);
	attr = cdi_get_smart_attribute(ptr, index);
	if (count)
	{
		str = cdi_get_smart_attribute_format(ptr, index);
		NWL_NodeAttrSet(node, "SMART Format", str, 0);
		cdi_free_string(str);
	}
	for (i = 0; i < count; i++)
	{
		char key[] = "SMART XX";
		char* val;
		if (attr[i].Id == 0)
			continue;
		str = cdi_get_smart_attribute_name(ptr, index, attr[i].Id);
		val = cdi_get_smart_attribute_value(ptr, index, i);
		snprintf(key, sizeof(key), "SMART %02X", attr[i].Id);
		NWL_NodeAttrSetf(node, key, 0, "%s %s", val, str);
		cdi_free_string(str);
		cdi_free_string(val);
	}
}

static VOID
PrintDiskInfo(BOOL cdrom, PNODE node, CDI_SMART* smart)
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
		NWL_NodeAttrSet(nd, "Type", NWL_GetBusTypeString(PhyDriveList[i].BusType), 0);
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
			NWL_NodeAttrSetf(nd, "GPT GUID", NAFLG_FMT_GUID, "{%s}", NWL_GuidToStr(PhyDriveList[i].GptGuid));
		}
		if (!cdrom && smart)
			PrintSmartInfo(nd, smart, GetSmartIndex(smart, i));
		if (PhyDriveList[i].VolumeCount)
		{
			DWORD j;
			PNODE nv = NWL_NodeAppendNew(nd, "Volumes", NFLG_TABLE);
			for (j = 0; j < PhyDriveList[i].VolumeCount; j++)
			{
				PNODE vol = NWL_NodeAppendNew(nv, "Volume", NFLG_TABLE_ROW);
				PrintVolumeInfo(vol, PhyDriveList[i].Volumes[j], &PhyDriveList[i]);
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
	if (NWLC->NwSmart && NWLC->NwSmbiosInit == FALSE)
	{
		cdi_init_smart(NWLC->NwSmart, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE);
		NWLC->NwSmbiosInit = TRUE;
	}
	PrintDiskInfo(FALSE, node, NWLC->NwSmart);
	PrintDiskInfo(TRUE, node, NWLC->NwSmart);
	return node;
}
