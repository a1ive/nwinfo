// SPDX-License-Identifier: Unlicense

#include "disklib.h"
#include <stdio.h>
#include <setupapi.h>
#include "../libcdi/libcdi.h"

#define DISKLIB_BUFSZ 0x10000
#define DISKLIB_BUFSZW (DISKLIB_BUFSZ / sizeof(WCHAR))
#define DISKLIB_BUFSZB (DISKLIB_BUFSZW * sizeof(WCHAR))
static union
{
	CHAR c[DISKLIB_BUFSZ];
	WCHAR w[DISKLIB_BUFSZW];
} mBuf;

static VOID
ErrorMsg(LPCSTR lpszText, INT nExitCode)
{
	fprintf(stderr, "Error: %s\n", lpszText);
	exit(nExitCode);
}

static HANDLE
GetDiskHandleById(BOOL bCdrom, BOOL bWrite, DWORD dwId)
{
	WCHAR PhyPath[28]; // L"\\\\.\\PhysicalDrive4294967295"
	if (bCdrom)
		swprintf(PhyPath, 28, L"\\\\.\\CdRom%u", dwId);
	else
		swprintf(PhyPath, 28, L"\\\\.\\PhysicalDrive%u", dwId);
	return CreateFileW(PhyPath, bWrite ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, 0);
}

static LPCWSTR
GuidToStr(BOOL bBracket, GUID* pGuid)
{
	static WCHAR cchGuid[39] = { 0 };
	swprintf(cchGuid, 39, L"%s%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X%s",
		bBracket ? L"{" : L"",
		pGuid->Data1, pGuid->Data2, pGuid->Data3,
		pGuid->Data4[0], pGuid->Data4[1], pGuid->Data4[2], pGuid->Data4[3],
		pGuid->Data4[4], pGuid->Data4[5], pGuid->Data4[6], pGuid->Data4[7],
		bBracket ? L"}" : L"");
	return cchGuid;
}

static LPCWSTR
GetGptFlag(GUID* pGuid)
{
	LPCWSTR lpszGuid = GuidToStr(FALSE, pGuid);
	if (_wcsicmp(lpszGuid, L"c12a7328-f81f-11d2-ba4b-00a0c93ec93b") == 0)
		return L"ESP";
	else if (_wcsicmp(lpszGuid, L"e3c9e316-0b5c-4db8-817d-f92df00215ae") == 0)
		return L"MSR";
	else if (_wcsicmp(lpszGuid, L"de94bba4-06d1-4d40-a16a-bfd50179d6ac") == 0)
		return L"WINRE";
	else if (_wcsicmp(lpszGuid, L"21686148-6449-6e6f-744e-656564454649") == 0)
		return L"BIOS";
	else if (_wcsicmp(lpszGuid, L"024dee41-33e7-11d3-9d69-0008c781f39f") == 0)
		return L"MBR";
	return L"DATA";
}

static LPCWSTR
GetMbrFlag(LONGLONG llStartingOffset, PHY_DRIVE_INFO* pParent)
{
	INT i;
	LONGLONG llLba = llStartingOffset >> 9;
	for (i = 0; i < 4; i++)
	{
		if (llLba == pParent->MbrLba[i])
			return L"PRIMARY";
	}
	return L"EXTENDED";
}

static VOID
FillPartitionInfo(DISK_VOL_INFO* pInfo, LPCWSTR lpszPath, PHY_DRIVE_INFO* pParent)
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
	pInfo->StartLba = partInfo.StartingOffset.QuadPart >> 9;
	pInfo->PartNum = partInfo.PartitionNumber;
	switch (partInfo.PartitionStyle)
	{
	case PARTITION_STYLE_MBR:
		swprintf(pInfo->PartType, DISK_PROP_STR_LEN, L"%02X", partInfo.Mbr.PartitionType);
		wcscpy_s(pInfo->PartId, DISK_PROP_STR_LEN, GuidToStr(TRUE, &partInfo.Mbr.PartitionId));
		wcscpy_s(pInfo->PartFlag, DISK_PROP_STR_LEN, GetMbrFlag(partInfo.StartingOffset.QuadPart, pParent));
		pInfo->BootIndicator = partInfo.Mbr.BootIndicator;
		break;
	case PARTITION_STYLE_GPT:
		wcscpy_s(pInfo->PartType, DISK_PROP_STR_LEN, GuidToStr(TRUE, &partInfo.Gpt.PartitionType));
		wcscpy_s(pInfo->PartId, DISK_PROP_STR_LEN, GuidToStr(TRUE, &partInfo.Gpt.PartitionId));
		wcscpy_s(pInfo->PartFlag, DISK_PROP_STR_LEN, GetGptFlag(&partInfo.Gpt.PartitionType));
		pInfo->BootIndicator = (wcscmp(pInfo->PartFlag, L"ESP") == 0) ? TRUE : FALSE;
		break;
	}
}

static VOID
FillVolumeInfo(DISK_VOL_INFO* pInfo, LPCWSTR lpszVolume, PHY_DRIVE_INFO* pParent)
{
	DWORD dwSize = 0;

	swprintf(pInfo->VolPath, MAX_PATH, L"%s\\", lpszVolume);

	if (GetVolumeInformationW(pInfo->VolPath, pInfo->VolLabel, MAX_PATH,
		NULL, NULL, NULL,
		pInfo->VolFs, MAX_PATH))
	{
		FillPartitionInfo(pInfo, lpszVolume, pParent);
	}
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

static BOOL
GetDriveByVolume(BOOL bIsCdRom, HANDLE hVolume, DWORD* pDrive)
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
	DRIVE_LAYOUT_INFORMATION_EX* pLayout = (DRIVE_LAYOUT_INFORMATION_EX*)mBuf.c;
	DWORD dwBytes = DISKLIB_BUFSZ;

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
	return TRUE;

fail:
	pInfo->PartMap = PARTITION_STYLE_RAW;
	return FALSE;
}

static BOOL
CheckSsd(HANDLE hDisk, PHY_DRIVE_INFO* pInfo)
{
	OSVERSIONINFOEXW osInfo = { 0 };
	GetNtVersion(&osInfo);
	if (osInfo.dwMajorVersion >= 6)
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

static int __cdecl
CompareDiskId(const void* a, const void* b)
{
	return ((int)((const PHY_DRIVE_INFO*)a)->Index) - ((int)((const PHY_DRIVE_INFO*)b)->Index);
}

typedef struct
{
	DWORD  cbSize;
	WCHAR  DevicePath[512];
} MY_DEVIF_DETAIL_DATA;

DWORD GetDriveInfoList(BOOL bIsCdRom, PHY_DRIVE_INFO** pDriveList)
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
		ErrorMsg("SetupDiGetClassDevsW failed", -3);

	while (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &devGuid, dwCount, &ifData))
		dwCount++;
	if (dwCount == 0)
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return 0;
	}

	*pDriveList = calloc(dwCount, sizeof(PHY_DRIVE_INFO));
	if (!*pDriveList)
		ErrorMsg("out of memory", -2);

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

		if (SetupDiGetDeviceInstanceIdW(hDevInfo, &infoData, mBuf.w, DISKLIB_BUFSZB, NULL))
			wcsncpy_s(pInfo[i].HwID, MAX_PATH, mBuf.w, MAX_PATH);
		if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &infoData, SPDRP_FRIENDLYNAME,
			NULL, (PBYTE)mBuf.w, DISKLIB_BUFSZB, NULL) ||
			SetupDiGetDeviceRegistryPropertyW(hDevInfo, &infoData, SPDRP_DEVICEDESC,
				NULL, (PBYTE)mBuf.w, DISKLIB_BUFSZB, NULL))
			wcsncpy_s(pInfo[i].HwName, MAX_PATH, mBuf.w, MAX_PATH);

		hDrive = GetDiskHandleById(bIsCdRom, FALSE, pInfo[i].Index);

		if (!hDrive || hDrive == INVALID_HANDLE_VALUE)
			goto next_drive;

		if (!bIsCdRom)
			pInfo[i].Ssd = CheckSsd(hDrive, &pInfo[i]);

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

		if (pDevDesc->VendorIdOffset)
		{
			strcpy_s(pInfo[i].VendorId, MAX_PATH,
				(char*)pDevDesc + pDevDesc->VendorIdOffset);
			TrimString(pInfo[i].VendorId);
		}

		if (pDevDesc->ProductIdOffset)
		{
			strcpy_s(pInfo[i].ProductId, MAX_PATH,
				(char*)pDevDesc + pDevDesc->ProductIdOffset);
			TrimString(pInfo[i].ProductId);
		}

		if (pDevDesc->ProductRevisionOffset)
		{
			strcpy_s(pInfo[i].ProductRev, MAX_PATH,
				(char*)pDevDesc + pDevDesc->ProductRevisionOffset);
			TrimString(pInfo[i].ProductRev);
		}

		if (pDevDesc->SerialNumberOffset)
		{
			strcpy_s(pInfo[i].SerialNumber, MAX_PATH,
				(char*)pDevDesc + pDevDesc->SerialNumberOffset);
			TrimString(pInfo[i].SerialNumber);
		}

		GetDiskPartMap(hDrive, bIsCdRom, &pInfo[i]);

next_drive:
		if (pDevDesc)
			free(pDevDesc);
		if (hDrive && hDrive != INVALID_HANDLE_VALUE)
			CloseHandle(hDrive);
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);

	for (bRet = TRUE, hSearch = FindFirstVolumeW(cchVolume, MAX_PATH);
		bRet && hSearch != INVALID_HANDLE_VALUE;
		bRet = FindNextVolumeW(hSearch, cchVolume, MAX_PATH))
	{
		DWORD dwIndex;
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
		for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
		{
			if (dwBytes == pInfo[dwIndex].Index)
				break;
		}
		if (dwIndex >= dwCount)
			continue;
		if (pInfo[dwIndex].VolCount % 4 == 0)
			pInfo[dwIndex].VolInfo = realloc(pInfo[dwIndex].VolInfo,
				sizeof(DISK_VOL_INFO) * (pInfo[dwIndex].VolCount + 4));
		if (pInfo[dwIndex].VolInfo == NULL)
			ErrorMsg("out of memory", -1);
		FillVolumeInfo(&pInfo[dwIndex].VolInfo[pInfo[dwIndex].VolCount], cchVolume, &pInfo[dwIndex]);
		pInfo[dwIndex].VolCount++;
	}

	if (hSearch != INVALID_HANDLE_VALUE)
		FindVolumeClose(hSearch);

	qsort(pInfo, dwCount, sizeof(PHY_DRIVE_INFO), CompareDiskId);
	return dwCount;
}

VOID
DestoryDriveInfoList(PHY_DRIVE_INFO* pInfo, DWORD dwCount)
{
	if (pInfo == NULL)
		return;
	for (DWORD i = 0; i < dwCount; i++)
	{
		for (DWORD j = 0; j < pInfo[i].VolCount; j++)
		{
			free(pInfo[i].VolInfo[j].VolNames);
		}
		free(pInfo[i].VolInfo);
	}
	free(pInfo);
}


VOID
GetNtVersion(LPOSVERSIONINFOEXW osInfo)
{
	NTSTATUS(WINAPI * RtlGetVersion)(LPOSVERSIONINFOEXW) = NULL;
	HMODULE hMod = GetModuleHandleW(L"ntdll");

	ZeroMemory(osInfo, sizeof(OSVERSIONINFOEXW));

	if (hMod)
		*(FARPROC*)&RtlGetVersion = GetProcAddress(hMod, "RtlGetVersion");

	if (RtlGetVersion)
	{
		osInfo->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
		RtlGetVersion(osInfo);
	}
}

#define U_BUFSZ 0x10000
static CHAR mUtf8Buf[U_BUFSZ + 1];

LPCSTR
Ucs2ToUtf8(LPCWSTR src)
{
	size_t i;
	CHAR* p = mUtf8Buf;
	ZeroMemory(mUtf8Buf, sizeof(mUtf8Buf));
	for (i = 0; i < U_BUFSZ / 3; i++)
	{
		if (src[i] == 0x0000)
			break;
		else if (src[i] <= 0x007F)
			*p++ = (CHAR)src[i];
		else if (src[i] <= 0x07FF)
		{
			*p++ = (src[i] >> 6) | 0xC0;
			*p++ = (src[i] & 0x3F) | 0x80;
		}
		else if (src[i] >= 0xD800 && src[i] <= 0xDFFF)
		{
			*p++ = 0;
			break;
		}
		else
		{
			*p++ = (src[i] >> 12) | 0xE0;
			*p++ = ((src[i] >> 6) & 0x3F) | 0x80;
			*p++ = (src[i] & 0x3F) | 0x80;
		}
	}
	return mUtf8Buf;
}

LPCSTR
GetHumanSize(UINT64 ullSize, UINT64 ullBase)
{
	UINT64 fsize = ullSize, frac = 0;
	unsigned units = 0;
	static char buf[48];
	const char* umsg;
	const char* humanSizes[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB" };

	while (fsize >= ullBase && units < 5)
	{
		frac = fsize % ullBase;
		fsize = fsize / ullBase;
		units++;
	}

	umsg = humanSizes[units];

	if (units)
	{
		if (frac)
			frac = frac * 100 / ullBase;
		snprintf(buf, sizeof(buf), "%llu.%02llu %s", fsize, frac, umsg);
	}
	else
		snprintf(buf, sizeof(buf), "%llu %s", ullSize, umsg);
	return buf;
}
