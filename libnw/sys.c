// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>

#include "libnw.h"
#include "utils.h"
#include "efivars.h"

#define PSAPI_VERSION 1
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

static const CHAR* Win10BuildNumber(DWORD dwBuildNumber)
{
	switch (dwBuildNumber)
	{
	case 22631U:
		return "11 23H2"; // Sun Valley 3
	case 22621U:
		return "11 22H2"; // Sun Valley 2
	case 22000U:
		return "11 21H2"; // Sun Valley
	case 19045U:
		return "10 22H2"; // Vibranium 22H2
	case 19044U:
		return "10 21H2"; // Vibranium 21H2
	case 19043U:
		return "10 21H1"; // Vibranium 21H1
	case 19042U:
		return "10 20H2"; // Vibranium 20H2
	case 19041U:
		return "10 2004"; // Vibranium 20H1
	case 18363U:
		return "10 1909"; // Vanadium 19H2
	case 18362U:
		return "10 1903"; // 19H1
	case 17763U:
		return "10 1809"; // Redstone 5
	case 17134U:
		return "10 1803"; // Redstone 4
	case 16299U:
		return "10 1709"; // Redstone 3
	case 15063U:
		return "10 1703"; // Redstone 2
	case 14393U:
		return "10 1607"; // Redstone
	case 10586U:
		return "10 1511"; // Threshold 2
	case 10240U:
		return "10 1507"; // Threshold
	}
	if (dwBuildNumber >= 22000U)
		return "11";
	return "10";
}

static const CHAR* WinServer2016BuildNumber(DWORD dwBuildNumber)
{
	switch (dwBuildNumber)
	{
	case 20348U:
		return "Server 2022";
	case 19042U:
		return "Server, version 20H2"; // WTF?
	case 19041U:
		return "Server, version 2004"; // WTF?
	case 18363U:
		return "Server, version 1909"; // WTF?
	case 18362U:
		return "Server, version 1903"; // WTF?
	case 17763U:
		return "Server 2019"; // Server, version 1809 ?
	case 17134U:
		return "Server, version 1803"; // WTF?
	case 16299U:
		return "Server, version 1709"; // WTF?
	case 14393U:
		return "Server 2016";
	}
	if (dwBuildNumber >= 20348U)
		return "Server 2022";
	else if (dwBuildNumber >= 17763U)
		return "Server 2019";
	return "Server 2016";
}

static const CHAR*
OsVersionToStr(OSVERSIONINFOEXW* p)
{
	if (p->dwMajorVersion == 10 && p->dwMinorVersion == 0)
	{
		// FUCK YOU MICROSOFT
		if (p->wProductType != VER_NT_WORKSTATION)
			return WinServer2016BuildNumber(p->dwBuildNumber);
		else
			return Win10BuildNumber(p->dwBuildNumber);
	}
	if (p->dwMajorVersion == 6 && p->dwMinorVersion == 3)
	{
		if (p->wProductType != VER_NT_WORKSTATION)
			return "Server 2012 R2";
		else
			return "8.1";
	}
	if (p->dwMajorVersion == 6 && p->dwMinorVersion == 2)
	{
		if (p->wProductType != VER_NT_WORKSTATION)
			return "Server 2012";
		else
			return "8";
	}
	if (p->dwMajorVersion == 6 && p->dwMinorVersion == 1)
	{
		if (p->wProductType != VER_NT_WORKSTATION)
			return "Server 2008 R2";
		else
			return "7";
	}
	if (p->dwMajorVersion == 6 && p->dwMinorVersion == 0)
	{
		if (p->wProductType != VER_NT_WORKSTATION)
			return "Server 2008";
		else
			return "Vista";
	}
	if (p->dwMajorVersion == 5 && p->dwMinorVersion == 2)
	{
		if (p->wSuiteMask & VER_SUITE_WH_SERVER)
			return "Home Server";
		else if (p->wProductType == VER_NT_WORKSTATION)
			return "XP x64";
		else
			return "Server 2003";
	}
	if (p->dwMajorVersion == 5 && p->dwMinorVersion == 1)
	{
		return "XP";
	}
#if 0
	if (p->dwMajorVersion == 5 && p->dwMinorVersion == 0)
	{
		return "2000";
	}
#endif
	return "Unknown";
}

static void PrintOsVer(PNODE node)
{
	DWORD dwType, dwSize;
	CHAR szSP[] = " SP65535.65535";
	LPWSTR lpEdition = NULL;
	if (NWLC->NwOsInfo.wServicePackMinor)
		snprintf(szSP, sizeof(szSP), " SP%u.%u", NWLC->NwOsInfo.wServicePackMajor, NWLC->NwOsInfo.wServicePackMinor);
	else if (NWLC->NwOsInfo.wServicePackMajor)
		snprintf(szSP, sizeof(szSP), " SP%u", NWLC->NwOsInfo.wServicePackMajor);
	else
		szSP[0] = '\0';
	NWL_NodeAttrSetf(node, "OS", 0, "Windows %s%s", OsVersionToStr(&NWLC->NwOsInfo), szSP);
	NWL_NodeAttrSetf(node, "Build Number", 0, "%lu.%lu.%lu",
		NWLC->NwOsInfo.dwMajorVersion, NWLC->NwOsInfo.dwMinorVersion, NWLC->NwOsInfo.dwBuildNumber);
	lpEdition = NWL_NtGetRegValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"EditionID", &dwSize, &dwType);
	if (lpEdition)
	{
		NWL_NodeAttrSet(node, "Edition", NWL_Ucs2ToUtf8(lpEdition), 0);
		free(lpEdition);
	}
}

static void
PrintProductKey(PNODE node)
{
	DWORD dwType, dwSize;
	LPBYTE lpKey = NULL;
	lpKey = NWL_NtGetRegValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"ProductId", &dwSize, &dwType);
	if (lpKey)
	{
		NWL_NodeAttrSet(node, "Product Id", NWL_Ucs2ToUtf8((LPWSTR)lpKey), NAFLG_FMT_NEED_QUOTE | NAFLG_FMT_SENSITIVE);
		free(lpKey);
	}
	lpKey = NWL_NtGetRegValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SoftwareProtectionPlatform", L"BackupProductKeyDefault", &dwSize, &dwType);
	if (lpKey)
	{
		NWL_NodeAttrSet(node, "Product Key", NWL_Ucs2ToUtf8((LPWSTR)lpKey), NAFLG_FMT_NEED_QUOTE | NAFLG_FMT_SENSITIVE);
		free(lpKey);
		return;
	}
#if 1
	INT i, j, k;
	CHAR buf[] = "BCDFGHJKMPQRTVWXY2346789";
	CHAR szProductKey[30] = { 0 };
	BYTE bKey[15] = { 0 };
	lpKey = NWL_NtGetRegValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"DigitalProductId", &dwSize, &dwType);
	if (!lpKey)
		return;
	if (dwSize < 72)
	{
		free(lpKey);
		return;
	}
	memcpy(bKey, lpKey + 52, 15);
	for (i = 28; i >= 0; i--)
	{
		for (k = 0, j = 14; j >= 0; j--)
		{
			k = (k << 8) ^ bKey[j];
			bKey[j] = (BYTE)(k / 24);
			k %= 24;
		}
		if (i % 6 == 5)
			szProductKey[i--] = '-';
		szProductKey[i] = buf[k];
	}
	NWL_NodeAttrSet(node, "Product Key", szProductKey, NAFLG_FMT_NEED_QUOTE | NAFLG_FMT_SENSITIVE);
	free(lpKey);
#endif
}

VOID NWL_GetUptime(CHAR* szUptime, DWORD dwSize)
{
	UINT64 ullUptime = GetTickCount64();
	UINT64 ullDays = ullUptime / 1000ULL / 3600ULL / 24ULL;
	UINT64 ullHours = ullUptime / 1000ULL / 3600ULL - ullDays * 24ULL;
	UINT64 ullMinutes = ullUptime / 1000ULL / 60ULL - ullDays * 24ULL * 60ULL - ullHours * 60ULL;
	UINT64 ullSeconds = ullUptime / 1000ULL - ullDays * 24ULL * 3600ULL - ullHours * 3600ULL - ullMinutes * 60ULL;
	CHAR szDays[32] = "";
	CHAR szHours[12] = "";
	szUptime[0] = '\0';
	if (ullDays)
		snprintf(szDays, sizeof(szDays), "%llud ", ullDays);
	if (ullHours)
		snprintf(szHours, sizeof(szHours), "%lluh ", ullHours);
	snprintf(szUptime, dwSize, "%s%s%llum %llus", szDays, szHours, ullMinutes, ullSeconds);
}

#define NWINFO_WCS_SIZE (NWINFO_BUFSZ / sizeof(WCHAR))

VOID NWL_GetHostname(CHAR* szHostname)
{
	WCHAR szHostnameW[MAX_COMPUTERNAME_LENGTH + 1] = L"";
	DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameW(szHostnameW, &dwSize);
	memcpy(szHostname, NWL_Ucs2ToUtf8(szHostnameW), MAX_COMPUTERNAME_LENGTH + 1);
}

static void PrintOsInfo(PNODE node)
{
	DWORD dwType;
	DWORD bufCharCount = NWINFO_WCS_SIZE;
	SYSTEM_INFO siInfo;
	WCHAR* infoBuf = (WCHAR*)NWLC->NwBuf;
	WCHAR* szHardwareId;
	NWL_GetHostname((CHAR*)NWLC->NwBuf);
	NWL_NodeAttrSet(node, "Computer Name", NWLC->NwBuf, 0);
	bufCharCount = NWINFO_WCS_SIZE;
	if (GetUserNameW(infoBuf, &bufCharCount))
		NWL_NodeAttrSet(node, "Username", NWL_Ucs2ToUtf8(infoBuf), 0);
	bufCharCount = NWINFO_WCS_SIZE;
	if (GetComputerNameExW(ComputerNameDnsDomain, infoBuf, &bufCharCount))
		NWL_NodeAttrSet(node, "DNS Domain", NWL_Ucs2ToUtf8(infoBuf), 0);
	bufCharCount = NWINFO_WCS_SIZE;
	if (GetComputerNameExW(ComputerNameDnsHostname, infoBuf, &bufCharCount))
		NWL_NodeAttrSet(node, "DNS Hostname", NWL_Ucs2ToUtf8(infoBuf), 0);
	if (GetSystemDirectoryW(infoBuf, NWINFO_WCS_SIZE))
		NWL_NodeAttrSet(node, "System Directory", NWL_Ucs2ToUtf8(infoBuf), 0);
	if (GetWindowsDirectoryW(infoBuf, NWINFO_WCS_SIZE))
		NWL_NodeAttrSet(node, "Windows Directory", NWL_Ucs2ToUtf8(infoBuf), 0);
	NWL_GetUptime((CHAR*)NWLC->NwBuf, NWINFO_BUFSZ);
	NWL_NodeAttrSet(node, "Uptime", (CHAR*)NWLC->NwBuf, 0);
	GetNativeSystemInfo(&siInfo);
	switch (siInfo.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_AMD64:
		NWL_NodeAttrSet(node, "Processor Architecture", "x64", 0);
		break;
	case PROCESSOR_ARCHITECTURE_INTEL:
		NWL_NodeAttrSet(node, "Processor Architecture", "x86", 0);
		break;
	default:
		NWL_NodeAttrSet(node, "Processor Architecture", "UNKNOWN", 0);
		break;
	}
	szHardwareId = NWL_NtGetRegValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\SystemInformation", L"ComputerHardwareId", NULL, &dwType);
	if (szHardwareId)
	{
		NWL_NodeAttrSet(node, "Computer Hardware Id", NWL_Ucs2ToUtf8(szHardwareId), NAFLG_FMT_NEED_QUOTE | NAFLG_FMT_SENSITIVE);
		free(szHardwareId);
	}
}

static void PrintSysMetrics(PNODE node)
{
	NWL_NodeAttrSetBool(node, "Safe Mode", GetSystemMetrics(SM_CLEANBOOT), 0);
	NWL_NodeAttrSetBool(node, "DBCS Enabled", GetSystemMetrics(SM_DBCSENABLED), 0);
	NWL_NodeAttrSetBool(node, "Debug Version", GetSystemMetrics(SM_DEBUG), 0);
	NWL_NodeAttrSetBool(node, "Slow Processor", GetSystemMetrics(SM_SLOWMACHINE), 0);
	NWL_NodeAttrSetBool(node, "Network Presence", GetSystemMetrics(SM_NETWORK) & 0x01, 0);
}

typedef struct _TPM_DEVICE_INFO
{
	UINT32 structVersion;
	UINT32 tpmVersion;
	UINT32 tpmInterfaceType;
	UINT32 tpmImpRevision;
} TPM_DEVICE_INFO, *PTPM_DEVICE_INFO;

static const CHAR*
TpmVersion(UINT32 ver)
{
	switch (ver)
	{
	case 1: return "v1.2";
	case 2: return "v2.0";
	}
	return "UNKNOWN";
}

static void PrintTpmInfo(PNODE node)
{
	UINT32 (WINAPI *GetTpmInfo) (UINT32 Size, VOID *Info) = NULL;
	HMODULE hL = NULL;
	TPM_DEVICE_INFO tpmInfo = { 0 };
	struct acpi_table_header* acpiHdr = NULL;
	acpiHdr = NWL_GetAcpi('2MPT');
	if (acpiHdr)
		NWL_NodeAttrSet(node, "TPM", "v2.0", 0);
	else
	{
		acpiHdr = NWL_GetAcpi('APCT');
		if (acpiHdr)
			NWL_NodeAttrSet(node, "TPM", "v1.2", 0);
	}
	if (acpiHdr)
	{
		free(acpiHdr);
		return;
	}
	hL = LoadLibraryW(L"tbs.dll");
	if (hL)
		*(FARPROC*)&GetTpmInfo = GetProcAddress(hL, "Tbsi_GetDeviceInfo");
	if (GetTpmInfo) {
		UINT32 dwRet = GetTpmInfo(sizeof(tpmInfo), &tpmInfo);
		NWL_NodeAttrSetf(node, "TPM", 0, "%s", (dwRet == 0) ? TpmVersion(tpmInfo.tpmVersion) : "NOT FOUND");
	}
	else
		NWL_NodeAttrSet(node, "TPM", "UNSUPPORTED", 0);
	if (hL)
		FreeLibrary(hL);
}

static BOOL WINAPI
EnumPageFileCallback(LPVOID pContext, PENUM_PAGE_FILE_INFORMATION pPageFileInfo, LPCWSTR lpFilename)
{
	PNWLIB_MEM_INFO pMemInfo = (PNWLIB_MEM_INFO)pContext;
	pMemInfo->PageInUse += pPageFileInfo->TotalInUse;
	pMemInfo->PageTotal += pPageFileInfo->TotalSize;
	return TRUE;
}

VOID NWL_GetMemInfo(PNWLIB_MEM_INFO pMemInfo)
{
	MEMORYSTATUSEX statex = { .dwLength = sizeof(MEMORYSTATUSEX) };
	PERFORMANCE_INFORMATION perf = { .cb = sizeof(PERFORMANCE_INFORMATION) };
	SYSTEM_FILECACHE_INFORMATION sfci = { 0 };

	ZeroMemory(pMemInfo, sizeof(NWLIB_MEM_INFO));

	GlobalMemoryStatusEx(&statex);
	pMemInfo->PhysUsage = statex.dwMemoryLoad;
	pMemInfo->PhysAvail = statex.ullAvailPhys;
	pMemInfo->PhysTotal = statex.ullTotalPhys;
	pMemInfo->PhysInUse = pMemInfo->PhysTotal - pMemInfo->PhysAvail;

	GetPerformanceInfo(&perf, sizeof(PERFORMANCE_INFORMATION));
	pMemInfo->SystemCache = perf.SystemCache;
	pMemInfo->PageSize = perf.PageSize;

	EnumPageFilesW(EnumPageFileCallback, pMemInfo);
	pMemInfo->PageAvail = pMemInfo->PageTotal - pMemInfo->PageInUse;
	if (pMemInfo->PageTotal && pMemInfo->PageTotal >= pMemInfo->PageInUse)
		pMemInfo->PageUsage = (DWORD)(pMemInfo->PageInUse * 100 / pMemInfo->PageTotal);
	pMemInfo->PageAvail *= pMemInfo->PageSize;
	pMemInfo->PageInUse *= pMemInfo->PageSize;
	pMemInfo->PageTotal *= pMemInfo->PageSize;

	NWL_NtQuerySystemInformation(SystemFileCacheInformation, &sfci, sizeof(sfci), NULL);
	pMemInfo->SfciInUse = sfci.CurrentSize;
	pMemInfo->SfciTotal = sfci.PeakSize;
	pMemInfo->SfciAvail = sfci.PeakSize - sfci.CurrentSize;
	if (pMemInfo->SfciTotal && pMemInfo->SfciTotal >= pMemInfo->SfciInUse)
		pMemInfo->SfciUsage = (DWORD)(pMemInfo->SfciInUse * 100 / pMemInfo->SfciTotal);
}

static void PrintMemInfo(PNODE node)
{
	PNODE nphy, npage;
	NWLIB_MEM_INFO info = { 0 };
	NWL_GetMemInfo(&info);

	NWL_NodeAttrSetf(node, "Page Size", NAFLG_FMT_NUMERIC, "%Id", info.PageSize);
	NWL_NodeAttrSetf(node, "Memory Usage", 0, "%lu%%", info.PhysUsage);
	nphy = NWL_NodeAppendNew(node, "Physical Memory", NFLG_ATTGROUP);
	NWL_NodeAttrSet(nphy, "Free", NWL_GetHumanSize(info.PhysAvail, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
	NWL_NodeAttrSet(nphy, "Total", NWL_GetHumanSize(info.PhysTotal, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
	NWL_NodeAttrSetf(node, "Paging File Usage", 0, "%lu%%", info.PageUsage);
	npage = NWL_NodeAppendNew(node, "Paging File", NFLG_ATTGROUP);
	NWL_NodeAttrSet(npage, "Free", NWL_GetHumanSize(info.PageAvail, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
	NWL_NodeAttrSet(npage, "Total", NWL_GetHumanSize(info.PageTotal, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
}

static void PrintBootInfo(PNODE node)
{
	DWORD dwType;
	DWORD dwBitLocker = 0;
	HANDLE hFile;
	WCHAR wArcName[MAX_PATH];
	WCHAR* pFwBootDev = NWL_NtGetRegValue(HKEY_LOCAL_MACHINE,
		L"SYSTEM\\CurrentControlSet\\Control", L"FirmwareBootDevice", NULL, &dwType);
	WCHAR* pSysBootDev = NWL_NtGetRegValue(HKEY_LOCAL_MACHINE,
		L"SYSTEM\\CurrentControlSet\\Control", L"SystemBootDevice", NULL, &dwType);
	WCHAR* pStartOption = NWL_NtGetRegValue(HKEY_LOCAL_MACHINE,
		L"SYSTEM\\CurrentControlSet\\Control", L"SystemStartOptions", NULL, &dwType);
	if (pFwBootDev)
	{
		swprintf(wArcName, MAX_PATH, L"\\ArcName\\%s", pFwBootDev);
		free(pFwBootDev);
		hFile = NWL_NtCreateFile(wArcName, FALSE);
		NWL_NodeAttrSet(node, "Boot Device", NWL_NtGetPathFromHandle(hFile), 0);
		CloseHandle(hFile);
	}
	if (pSysBootDev)
	{
		swprintf(wArcName, MAX_PATH, L"\\ArcName\\%s", pSysBootDev);
		free(pSysBootDev);
		hFile = NWL_NtCreateFile(wArcName, FALSE);
		NWL_NodeAttrSet(node, "System Device", NWL_NtGetPathFromHandle(hFile), 0);
		CloseHandle(hFile);
	}
	if (pStartOption)
	{
		NWL_NodeAttrSet(node, "Start Options", NWL_Ucs2ToUtf8(pStartOption), NAFLG_FMT_NEED_QUOTE | NAFLG_FMT_STRING);
		free(pStartOption);
	}
	NWL_GetRegDwordValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\BitlockerStatus", L"BootStatus", &dwBitLocker);
	NWL_NodeAttrSetBool(node, "BitLocker Boot", dwBitLocker, 0);
}

PNODE NW_System(VOID)
{
	PNODE node = NWL_NodeAlloc("System", 0);
	if (NWLC->SysInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	PrintOsVer(node);
	PrintOsInfo(node);
	PrintProductKey(node);
	PrintSysMetrics(node);
	PrintBootInfo(node);
	NWL_NodeAttrSet(node, "Firmware", NWL_IsEfi() ? "UEFI" : "Legacy BIOS", 0);
	PrintTpmInfo(node);
	PrintMemInfo(node);
	return node;
}
