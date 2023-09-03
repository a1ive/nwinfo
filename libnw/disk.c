// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "disk.h"
#include "utils.h"
#include "../libcdi/libcdi.h"

static const char* d_human_sizes[6] =
{ "B", "KB", "MB", "GB", "TB", "PB", };

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
PrintPartitionInfo(PNODE pNode, LPCWSTR lpszPath, PHY_DRIVE_INFO* pParent)
{
	BOOL bRet;
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	PARTITION_INFORMATION_EX partInfo = { 0 };
	DWORD dwPartInfo = sizeof(PARTITION_INFORMATION_EX);
	hDevice = CreateFileW(lpszPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (!hDevice || hDevice == INVALID_HANDLE_VALUE)
		return;
	bRet = DeviceIoControl(hDevice, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, &partInfo, dwPartInfo, &dwPartInfo, NULL);
	CloseHandle(hDevice);
	if (!bRet)
		return;
	if (partInfo.StartingOffset.QuadPart == 0) // CDROM
		return;
	NWL_NodeAttrSetf(pNode, "Starting LBA", NAFLG_FMT_NUMERIC, "%llu", partInfo.StartingOffset.QuadPart >> 9);
	NWL_NodeAttrSetf(pNode, "Partition Number", NAFLG_FMT_NUMERIC, "%lu", partInfo.PartitionNumber);
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
PrintVolumeInfo(PNODE pNode, LPCWSTR lpszVolume, PHY_DRIVE_INFO* pParent)
{
	WCHAR cchLabel[MAX_PATH];
	WCHAR cchFs[MAX_PATH];
	WCHAR cchPath[MAX_PATH];
	LPWCH lpszVolumePathNames = NULL;
	DWORD dwSize = 0;
	ULARGE_INTEGER ulFreeSpace = { 0 };
	ULARGE_INTEGER ulTotalSpace = { 0 };

	swprintf(cchPath, MAX_PATH, L"%s\\", lpszVolume);
	NWL_NodeAttrSet(pNode, "Path", GetRealVolumePath(lpszVolume), 0);
	NWL_NodeAttrSet(pNode, "Volume GUID", NWL_Ucs2ToUtf8(lpszVolume), 0);
	PrintPartitionInfo(pNode, lpszVolume, pParent);
	if (GetVolumeInformationW(cchPath, cchLabel, MAX_PATH, NULL, NULL, NULL, cchFs, MAX_PATH))
	{
		NWL_NodeAttrSet(pNode, "Label", NWL_Ucs2ToUtf8(cchLabel), 0);
		NWL_NodeAttrSet(pNode, "Filesystem", NWL_Ucs2ToUtf8(cchFs), 0);
	}
	if (GetDiskFreeSpaceExW(cchPath, NULL, NULL, &ulFreeSpace))
		NWL_NodeAttrSet(pNode, "Free Space",
			NWL_GetHumanSize(ulFreeSpace.QuadPart, d_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
	if (GetDiskFreeSpaceExW(cchPath, NULL, &ulTotalSpace, NULL))
		NWL_NodeAttrSet(pNode, "Total Space",
			NWL_GetHumanSize(ulTotalSpace.QuadPart, d_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
	if (ulTotalSpace.QuadPart != 0)
		NWL_NodeAttrSetf(pNode, "Usage", 0, "%.2f%%",
			100.0 - (ulFreeSpace.QuadPart * 100.0) / ulTotalSpace.QuadPart);
	GetVolumePathNamesForVolumeNameW(cchPath, NULL, 0, &dwSize);
	if (GetLastError() == ERROR_MORE_DATA && dwSize)
	{
		lpszVolumePathNames = calloc(dwSize, sizeof(WCHAR));
		if (lpszVolumePathNames)
		{
			if (GetVolumePathNamesForVolumeNameW(cchPath, lpszVolumePathNames, dwSize, &dwSize))
			{
				PNODE mp = NWL_NodeAppendNew(pNode, "Volume Path Names", NFLG_TABLE);
				for (WCHAR* p = lpszVolumePathNames; p[0] != L'\0'; p += wcslen(p) + 1)
				{
					PNODE mnt = NWL_NodeAppendNew(mp, "Mount Point", NFLG_TABLE_ROW);
					NWL_NodeAttrSet(mnt, wcslen(p) > 3 ? "Path" : "Drive Letter", NWL_Ucs2ToUtf8(p), 0);
				}
			}
			free(lpszVolumePathNames);
		}
	}
}

static DWORD GetDriveCount(BOOL Cdrom)
{
	DWORD Value = 0;
	LPCWSTR Key = L"SYSTEM\\CurrentControlSet\\Services\\disk\\Enum";
	if (Cdrom)
		Key = L"SYSTEM\\CurrentControlSet\\Services\\cdrom\\Enum";
	if (NWL_GetRegDwordValue(HKEY_LOCAL_MACHINE, Key, L"Count", &Value) != 0)
		Value = 0;
	return Value;
}

static HANDLE GetHandleByLetter(WCHAR Letter)
{
	WCHAR PhyPath[] = L"\\\\.\\A:";
	PhyPath[4] = Letter;
	return CreateFileW(PhyPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
}

static WCHAR *GetDriveHwId(BOOL Cdrom, DWORD Drive)
{
	DWORD dwType;
	WCHAR drvRegKey[11]; // "4294967295"
	LPCWSTR key = L"SYSTEM\\CurrentControlSet\\Services\\disk\\Enum";
	if (Cdrom)
		key = L"SYSTEM\\CurrentControlSet\\Services\\cdrom\\Enum";
	swprintf(drvRegKey, 11, L"%u", Drive);
	return NWL_NtGetRegValue(HKEY_LOCAL_MACHINE, key, drvRegKey, NULL, &dwType);
}

static WCHAR* GetDriveHwName(const WCHAR* HwId)
{
	WCHAR* HwName = NULL;
	HKEY hKey = NULL;
	DWORD dwType;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Enum",
		0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
		return NULL;
	HwName = NWL_NtGetRegValue(hKey, HwId, L"FriendlyName", NULL, &dwType);
	if (!HwName)
		HwName = NWL_NtGetRegValue(hKey, HwId, L"DeviceDesc", NULL, &dwType);
	RegCloseKey(hKey);
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
GetDiskPartMap(HANDLE hDisk, BOOL bIsCdRom, PHY_DRIVE_INFO* pInfo)
{
	DRIVE_LAYOUT_INFORMATION_EX* pLayout = (VOID*)NWLC->NwBuf;
	DWORD dwBytes = NWINFO_BUFSZ;

	if (bIsCdRom)
		goto fail;

	if (!DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0,
		pLayout, dwBytes, &dwBytes, NULL))
		goto fail;
	if (dwBytes < sizeof(DRIVE_LAYOUT_INFORMATION_EX) - sizeof(PARTITION_INFORMATION_EX))
		goto fail;

	pInfo->PartMap = pLayout->PartitionStyle;
	switch (pInfo->PartMap)
	{
	case PARTITION_STYLE_MBR:
		memcpy(pInfo->MbrSignature, &pLayout->Mbr.Signature, sizeof(DWORD));
		break;
	case PARTITION_STYLE_GPT:
		memcpy(pInfo->GptGuid, &pLayout->Gpt.DiskId, sizeof(GUID));
		break;
	default:
		goto fail;
	}
	return TRUE;

fail:
	pInfo->PartMap = PARTITION_STYLE_RAW;
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

static DWORD GetDriveInfoList(BOOL bIsCdRom, PHY_DRIVE_INFO** pDriveList)
{
	DWORD i;
	DWORD dwCount;
	BOOL bRet;
	DWORD dwBytes;
	PHY_DRIVE_INFO* pInfo;

	HANDLE hSearch;
	WCHAR cchVolume[MAX_PATH];

	dwCount = GetDriveCount(bIsCdRom);
	*pDriveList = calloc(dwCount, sizeof(PHY_DRIVE_INFO));
	if (!*pDriveList)
		return 0;
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

		GetDiskPartMap(hDrive, bIsCdRom, &pInfo[i]);

next_drive:
		if (pDevDesc)
			free(pDevDesc);
		if (hDrive && hDrive != INVALID_HANDLE_VALUE)
			CloseHandle(hDrive);
	}

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
		if (!GetDriveByVolume(bIsCdRom, hVolume, &dwBytes))
		{
			CloseHandle(hVolume);
			continue;
		}
		CloseHandle(hVolume);
		if (dwBytes >= dwCount || pInfo[dwBytes].VolumeCount >= 32)
			continue;
		wcscpy_s(pInfo[dwBytes].Volumes[pInfo[dwBytes].VolumeCount], MAX_PATH, cchVolume);
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

	if (index < 0)
		return;
	cdi_update_smart(ptr, index);

	NWL_NodeAttrSetf(node, "Temperature (C)", NAFLG_FMT_NUMERIC, "%d", cdi_get_int(ptr, index, CDI_INT_TEMPERATURE));

	n = cdi_get_int(ptr, index, CDI_INT_LIFE);
	if (n >= 0)
		NWL_NodeAttrSetf(node, "Health Status", 0, "%s (%d%%)", cdi_get_health_status(cdi_get_int(ptr, index, CDI_INT_DISK_STATUS)), n);
	else
		NWL_NodeAttrSet(node, "Health Status", cdi_get_health_status(cdi_get_int(ptr, index, CDI_INT_DISK_STATUS)), 0);

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

	count = cdi_get_dword(ptr, index, CDI_DWORD_ATTR_COUNT);
	if (count)
	{
		str = cdi_get_smart_format(ptr, index);
		NWL_NodeAttrSet(node, "SMART Format", str, 0);
		cdi_free_string(str);
	}
	for (i = 0; i < count; i++)
	{
		char key[] = "SMART XX";
		char* val;
		int id = cdi_get_smart_id(ptr, index, i);
		if (id == 0)
			continue;
		str = cdi_get_smart_name(ptr, index, id);
		val = cdi_get_smart_value(ptr, index, i, TRUE);
		snprintf(key, sizeof(key), "SMART %02X", id);
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
			WCHAR* hwName = NULL;
			NWL_NodeAttrSet(nd, "HWID", NWL_Ucs2ToUtf8(PhyDriveList[i].HwID), 0);
			hwName = GetDriveHwName(PhyDriveList[i].HwID);
			if (hwName)
			{
				NWL_NodeAttrSet(nd, "HW Name", NWL_Ucs2ToUtf8(hwName), 0);
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
			NWL_NodeAttrSet(nd, "Serial Number", PhyDriveList[i].SerialNumber, NAFLG_FMT_SENSITIVE);
		NWL_NodeAttrSet(nd, "Type", NWL_GetBusTypeString(PhyDriveList[i].BusType), 0);
		NWL_NodeAttrSetBool(nd, "Removable", PhyDriveList[i].RemovableMedia, 0);
		NWL_NodeAttrSet(nd, "Size",
			NWL_GetHumanSize(PhyDriveList[i].SizeInBytes, d_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
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
		if (!cdrom && NWLC->DisableSmart == FALSE && smart)
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
	if (NWLC->DisableSmart == FALSE && NWLC->NwSmart && NWLC->NwSmartInit == FALSE)
	{
		cdi_init_smart(NWLC->NwSmart, NWLC->NwSmartFlags);
		NWLC->NwSmartInit = TRUE;
	}
	PrintDiskInfo(FALSE, node, NWLC->NwSmart);
	PrintDiskInfo(TRUE, node, NWLC->NwSmart);
	return node;
}
