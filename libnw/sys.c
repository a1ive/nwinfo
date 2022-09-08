// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>

#include "libnw.h"
#include "utils.h"

static const char* GV_GUID = "{8BE4DF61-93CA-11D2-AA0D-00E098032B8C}";

static const CHAR*
OsVersionToStr(OSVERSIONINFOEXW* p)
{
	if (p->dwMajorVersion == 10 && p->dwMinorVersion == 0)
	{
		// FUCK YOU MICROSOFT
		if (p->wProductType != VER_NT_WORKSTATION)
		{
			if (p->dwBuildNumber >= 17763U)
				return "Server 2019";
			else
				return "Server 2016";
		}
		else
		{
			if (p->dwBuildNumber >= 22000U)
				return "11";
			else
				return "10";
		}
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
		else
			return "Server 2003";
	}
	if (p->dwMajorVersion == 5 && p->dwMinorVersion == 1)
	{
		return "XP";
	}
	return "Unknown";
}

static void PrintOsVer(PNODE node)
{
	NTSTATUS(WINAPI * RtlGetVersion)(LPOSVERSIONINFOEXW) = NULL;
	OSVERSIONINFOEXW osInfo = { 0 };
	HMODULE hMod = GetModuleHandleA("ntdll");

	if (hMod)
		*(FARPROC*)&RtlGetVersion = GetProcAddress(hMod, "RtlGetVersion");

	if (RtlGetVersion)
	{
		osInfo.dwOSVersionInfoSize = sizeof(osInfo);
		RtlGetVersion(&osInfo);
		NWL_NodeAttrSetf(node, "OS", 0,
			"Windows %s (%lu.%lu.%lu)", OsVersionToStr(&osInfo),
			osInfo.dwMajorVersion, osInfo.dwMinorVersion, osInfo.dwBuildNumber);
	}
}

static void PrintOsInfo(PNODE node)
{
	DWORD bufCharCount = NWINFO_BUFSZ;
	SYSTEM_INFO SystemInfo;
	char* infoBuf = NWLC->NwBuf;
	UINT64 Uptime = 0;
	if (GetComputerNameA(infoBuf, &bufCharCount))
		NWL_NodeAttrSet(node, "Computer Name", infoBuf, 0);
	bufCharCount = NWINFO_BUFSZ;
	if (GetUserNameA(infoBuf, &bufCharCount))
		NWL_NodeAttrSet(node, "Username", infoBuf, 0);
	if (GetComputerNameExA(ComputerNameDnsDomain, infoBuf, &bufCharCount))
		NWL_NodeAttrSet(node, "DNS Domain", infoBuf, 0);
	if (GetComputerNameExA(ComputerNameDnsHostname, infoBuf, &bufCharCount))
		NWL_NodeAttrSet(node, "DNS Hostname", infoBuf, 0);
	if (GetSystemDirectoryA(infoBuf, NWINFO_BUFSZ))
		NWL_NodeAttrSet(node, "System Directory", infoBuf, 0);
	if (GetWindowsDirectoryA(infoBuf, NWINFO_BUFSZ))
		NWL_NodeAttrSet(node, "Windows Directory", infoBuf, 0);
	Uptime = GetTickCount64();
	{
		UINT64 Days = Uptime / 1000ULL / 3600ULL / 24ULL;
		UINT64 Hours = Uptime / 1000ULL / 3600ULL - Days * 24ULL;
		UINT64 Minutes = Uptime / 1000ULL / 60ULL - Days * 24ULL * 60ULL - Hours * 60ULL;
		UINT64 Seconds = Uptime / 1000ULL - Days * 24ULL * 3600ULL - Hours * 3600ULL - Minutes * 60ULL;
		NWL_NodeAttrSetf(node,"Uptime", 0, "%llu days, %llu hours, %llu min, %llu sec", Days, Hours, Minutes, Seconds);
	}
	GetNativeSystemInfo(&SystemInfo);
	switch (SystemInfo.wProcessorArchitecture)
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
	NWL_NodeAttrSetf(node, "Page Size", NAFLG_FMT_NUMERIC, "%u", SystemInfo.dwPageSize);
}

static void PrintFwInfo(PNODE node)
{
	DWORD VarSize = 0;
	UINT8 SecureBoot = 0;
	NWL_GetFirmwareEnvironmentVariable("", "{00000000-0000-0000-0000-000000000000}", NULL, 0);
	if (GetLastError() == ERROR_INVALID_FUNCTION)
		NWL_NodeAttrSet(node, "Firmware", "Legacy BIOS", 0);
	else
	{
		NWL_NodeAttrSet(node, "Firmware", "UEFI", 0);
		VarSize = NWL_GetFirmwareEnvironmentVariable("SecureBoot", GV_GUID, &SecureBoot, sizeof(UINT8));
		if (VarSize)
			NWL_NodeAttrSetf(node, "Secure Boot", 0, "%s", SecureBoot ? "ENABLED" : "DISABLED");
		else
			NWL_NodeAttrSet(node, "Secure Boot", "UNSUPPORTED", 0);
	}
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
	HMODULE hL = LoadLibraryA("tbs.dll");
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
	if (hL)
		*(FARPROC*)&GetTpmInfo = GetProcAddress(hL, "Tbsi_GetDeviceInfo");
	if (GetTpmInfo) {
		UINT32 dwRet = GetTpmInfo(sizeof(tpmInfo), &tpmInfo);
		NWL_NodeAttrSetf(node, "TPM", 0, "%s", (dwRet == 0) ? TpmVersion(tpmInfo.tpmVersion) : "NOT FOUND");
	}
	else
		NWL_NodeAttrSet(node, "TPM", "UNSUPPORTED", 0);
}

static void
PrintPowerInfo(PNODE node)
{
	SYSTEM_POWER_STATUS Power;
	if (!GetSystemPowerStatus(&Power)
		|| Power.ACLineStatus == 255
		|| Power.BatteryFlag == 255
		|| Power.BatteryLifePercent > 100)
	{
		NWL_NodeAttrSet(node, "Power Status", "UNKNOWN", 0);
		return;
	}
	if (Power.BatteryFlag == 128)
	{
		NWL_NodeAttrSet(node, "Power Status", "NO BATTERY", 0);
		return;
	}
		
	if (Power.BatteryLifeTime != -1)
	{
		UINT32 Hours = Power.BatteryLifeTime / 3600U;
		UINT32 Minutes = Power.BatteryLifeTime / 60ULL - Hours * 60ULL;
		//UINT32 Seconds = Power.BatteryLifeTime - Hours * 3600ULL - Minutes * 60ULL;
		NWL_NodeAttrSetf(node, "Power Status", 0, "%u%%, %lu hours %lu min left%s",
			Power.BatteryLifePercent, Hours, Minutes,
			(Power.ACLineStatus == 1 || Power.BatteryFlag == 8) ? ", Charging" : "");
	}
	else
	{
		NWL_NodeAttrSetf(node, "Power Status", 0, "%u%%%s",
			Power.BatteryLifePercent,
			(Power.ACLineStatus == 1 || Power.BatteryFlag == 8) ? ", Charging" : "");
	}
}

static const char* mem_human_sizes[6] =
{ "B", "K", "M", "G", "T", "P", };

static void PrintMemInfo(PNODE node)
{
	PNODE nphy, npage;
	MEMORYSTATUSEX statex = { 0 };
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	NWL_NodeAttrSetf(node, "Memory Usage", 0, "%u%%", statex.dwMemoryLoad);
	nphy = NWL_NodeAppendNew(node, "Physical Memory", NFLG_ATTGROUP);
	NWL_NodeAttrSet(nphy, "Free", NWL_GetHumanSize(statex.ullAvailPhys, mem_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
	NWL_NodeAttrSet(nphy, "Total", NWL_GetHumanSize(statex.ullTotalPhys, mem_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
	npage = NWL_NodeAppendNew(node, "Paging File", NFLG_ATTGROUP);
	NWL_NodeAttrSet(npage, "Free", NWL_GetHumanSize(statex.ullAvailPageFile, mem_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
	NWL_NodeAttrSet(npage, "Total", NWL_GetHumanSize(statex.ullTotalPageFile, mem_human_sizes, 1024), NAFLG_FMT_HUMAN_SIZE);
}

static void PrintBootDev(PNODE node)
{
	DWORD dwType;
	HANDLE hFile;
	WCHAR wArcName[MAX_PATH];
	WCHAR* pFwBootDev = NWL_NtGetRegValue(HKEY_LOCAL_MACHINE,
		L"SYSTEM\\CurrentControlSet\\Control", L"FirmwareBootDevice", &dwType);
	if (!pFwBootDev)
		return;
	swprintf(wArcName, MAX_PATH, L"\\ArcName\\%s", pFwBootDev);
	free(pFwBootDev);
	hFile = NWL_NtCreateFile(wArcName, FALSE);
	NWL_NodeAttrSet(node, "Boot Device", NWL_NtGetPathFromHandle(hFile), 0);
	CloseHandle(hFile);
}

PNODE NW_System(VOID)
{
	PNODE node = NWL_NodeAlloc("System", 0);
	if (NWLC->SysInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	PrintOsVer(node);
	PrintOsInfo(node);
	PrintBootDev(node);
	PrintPowerInfo(node);
	PrintFwInfo(node);
	PrintTpmInfo(node);
	PrintMemInfo(node);
	return node;
}
