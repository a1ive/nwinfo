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
	CHAR szSP[] = " SP65535.65535";
	if (NWLC->NwOsInfo.wServicePackMinor)
		snprintf(szSP, sizeof(szSP), " SP%u.%u", NWLC->NwOsInfo.wServicePackMajor, NWLC->NwOsInfo.wServicePackMinor);
	else if (NWLC->NwOsInfo.wServicePackMajor)
		snprintf(szSP, sizeof(szSP), " SP%u", NWLC->NwOsInfo.wServicePackMajor);
	else
		szSP[0] = '\0';
	NWL_NodeAttrSetf(node, "OS", 0, "Windows %s%s", OsVersionToStr(&NWLC->NwOsInfo), szSP);
	NWL_NodeAttrSetf(node, "Build Number", 0, "%lu.%lu.%lu",
		NWLC->NwOsInfo.dwMajorVersion, NWLC->NwOsInfo.dwMinorVersion, NWLC->NwOsInfo.dwBuildNumber);
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

static void PrintOsInfo(PNODE node)
{
	DWORD bufCharCount = NWINFO_WCS_SIZE;
	SYSTEM_INFO siInfo;
	WCHAR* infoBuf = (WCHAR*)NWLC->NwBuf;
	WCHAR* szHardwareId;
	if (GetComputerNameW(infoBuf, &bufCharCount))
		NWL_NodeAttrSet(node, "Computer Name", NWL_Ucs2ToUtf8(infoBuf), 0);
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
	szHardwareId = NWL_GetRegSzValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\SystemInformation", L"ComputerHardwareId");
	if (szHardwareId)
	{
		NWL_NodeAttrSet(node, "Computer Hardware Id", NWL_Ucs2ToUtf8(szHardwareId), NAFLG_FMT_NEED_QUOTE);
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
	struct acpi_table_header* AcpiHdr = NULL;
	AcpiHdr = NWL_GetAcpi('2MPT');
	if (AcpiHdr)
		NWL_NodeAttrSet(node, "TPM", "v2.0", 0);
	else
	{
		AcpiHdr = NWL_GetAcpi('APCT');
		if (AcpiHdr)
			NWL_NodeAttrSet(node, "TPM", "v1.2", 0);
	}
	if (AcpiHdr)
	{
		free(AcpiHdr);
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

static const char* mem_human_sizes[6] =
{ "B", "K", "M", "G", "T", "P", };

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
}

static void PrintMemInfo(PNODE node)
{
	PNODE nphy, npage;
	NWLIB_MEM_INFO info = { 0 };
	NWL_GetMemInfo(&info);

	NWL_NodeAttrSetf(node, "Page Size", NAFLG_FMT_NUMERIC, "%Id", info.PageSize);
	NWL_NodeAttrSetf(node, "Memory Usage", 0, "%lu%%", info.PhysUsage);
	nphy = NWL_NodeAppendNew(node, "Physical Memory", NFLG_ATTGROUP);
	NWL_NodeAttrSet(nphy, "Free", NWL_GetHumanSize(info.PhysAvail, mem_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
	NWL_NodeAttrSet(nphy, "Total", NWL_GetHumanSize(info.PhysTotal, mem_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
	NWL_NodeAttrSetf(node, "Paging File Usage", 0, "%lu%%", info.PageUsage);
	npage = NWL_NodeAppendNew(node, "Paging File", NFLG_ATTGROUP);
	NWL_NodeAttrSet(npage, "Free", NWL_GetHumanSize(info.PageAvail, mem_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
	NWL_NodeAttrSet(npage, "Total", NWL_GetHumanSize(info.PageTotal, mem_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
}

static void PrintBootInfo(PNODE node)
{
	DWORD dwType;
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
	DWORD dwBitLocker = 0;
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
	PrintSysMetrics(node);
	PrintBootInfo(node);
	NWL_NodeAttrSet(node, "Firmware", NWL_IsEfi() ? "UEFI" : "Legacy BIOS", 0);
	PrintTpmInfo(node);
	PrintMemInfo(node);
	return node;
}
