// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include "nwinfo.h"

static PNODE node;

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

static void PrintOsVer(void)
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
		node_setf(node, "OS", 0,
			"Windows %s (%lu.%lu.%lu)", OsVersionToStr(&osInfo),
			osInfo.dwMajorVersion, osInfo.dwMinorVersion, osInfo.dwBuildNumber);
	}
}

static void PrintOsInfo(void)
{
	DWORD bufCharCount = NWINFO_BUFSZ;
	SYSTEM_INFO SystemInfo;
	char* infoBuf = nwinfo_buffer;
	UINT64 Uptime = 0;
	if (GetComputerNameA(infoBuf, &bufCharCount))
		node_att_set(node, "Computer Name", infoBuf, 0);
	bufCharCount = NWINFO_BUFSZ;
	if (GetUserNameA(infoBuf, &bufCharCount))
		node_att_set(node, "Username", infoBuf, 0);
	if (GetComputerNameExA(ComputerNameDnsDomain, infoBuf, &bufCharCount))
		node_att_set(node, "DNS Domain", infoBuf, 0);
	if (GetComputerNameExA(ComputerNameDnsHostname, infoBuf, &bufCharCount))
		node_att_set(node, "DNS Hostname", infoBuf, 0);
	if (GetSystemDirectoryA(infoBuf, NWINFO_BUFSZ))
		node_att_set(node, "System Directory", infoBuf, 0);
	if (GetWindowsDirectoryA(infoBuf, NWINFO_BUFSZ))
		node_att_set(node, "Windows Directory", infoBuf, 0);
	Uptime = NT5GetTickCount();
	{
		UINT64 Days = Uptime / 1000ULL / 3600ULL / 24ULL;
		UINT64 Hours = Uptime / 1000ULL / 3600ULL - Days * 24ULL;
		UINT64 Minutes = Uptime / 1000ULL / 60ULL - Days * 24ULL * 60ULL - Hours * 60ULL;
		UINT64 Seconds = Uptime / 1000ULL - Days * 24ULL * 3600ULL - Hours * 3600ULL - Minutes * 60ULL;
		node_setf(node,"UpTime", 0, "%llu days, %llu hours, %llu min, %llu sec", Days, Hours, Minutes, Seconds);
	}
	GetNativeSystemInfo(&SystemInfo);
	switch (SystemInfo.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_AMD64:
		node_att_set(node, "Processor Architecture", "x64", 0);
		break;
	case PROCESSOR_ARCHITECTURE_INTEL:
		node_att_set(node, "Processor Architecture", "x86", 0);
		break;
	default:
		node_att_set(node, "Processor Architecture", "UNKNOWN", 0);
		break;
	}
	node_setf(node, "Page Size", 0, "%u", SystemInfo.dwPageSize);
}

static void PrintFwInfo(void)
{
	DWORD VarSize = 0;
	UINT8 SecureBoot = 0;
	NT5GetFirmwareEnvironmentVariable("", "{00000000-0000-0000-0000-000000000000}", NULL, 0);
	if (GetLastError() == ERROR_INVALID_FUNCTION)
		node_att_set(node, "Firmware", "Legacy BIOS", 0);
	else
	{
		node_att_set(node, "Firmware", "UEFI", 0);
		VarSize = NT5GetFirmwareEnvironmentVariable("SecureBoot", GV_GUID, &SecureBoot, sizeof(uint8_t));
		if (VarSize)
			node_setf(node, "Secure Boot", 0, "%s", SecureBoot ? "ENABLED" : "DISABLED");
		else
			node_att_set(node, "Secure Boot", "UNSUPPORTED", 0);
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

static void PrintTpmInfo(void)
{
	UINT32 (WINAPI *GetTpmInfo) (UINT32 Size, VOID *Info) = NULL;
	HINSTANCE hL = LoadLibraryA("tbs.dll");
	TPM_DEVICE_INFO tpmInfo = { 0 };
	struct acpi_table_header* AcpiHdr = NULL;
	AcpiHdr = GetAcpi('2MPT');
	if (AcpiHdr)
		node_att_set(node, "TPM", "v2.0", 0);
	else
	{
		AcpiHdr = GetAcpi('APCT');
		if (AcpiHdr)
			node_att_set(node, "TPM", "v1.2", 0);
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
		node_setf(node, "TPM", 0, "%s", (dwRet == 0) ? TpmVersion(tpmInfo.tpmVersion) : "NOT FOUND");
	}
	else
		node_att_set(node, "TPM", "UNSUPPORTED", 0);
}

static void
PrintPowerInfo(void)
{
	SYSTEM_POWER_STATUS Power;
	if (!GetSystemPowerStatus(&Power)
		|| Power.ACLineStatus == 255
		|| Power.BatteryFlag == 255
		|| Power.BatteryLifePercent > 100)
	{
		node_att_set(node, "Power status", "UNKNOWN", 0);
		return;
	}
	if (Power.BatteryFlag == 128)
	{
		node_att_set(node, "Power status", "NO BATTERY", 0);
		return;
	}

	
		
	if (Power.BatteryLifeTime != -1)
	{
		UINT32 Hours = Power.BatteryLifeTime / 3600U;
		UINT32 Minutes = Power.BatteryLifeTime / 60ULL - Hours * 60ULL;
		//UINT32 Seconds = Power.BatteryLifeTime - Hours * 3600ULL - Minutes * 60ULL;
		node_setf(node, "Power status", 0, "%u%%, %lu hours %lu min left%s",
			Power.BatteryLifePercent, Hours, Minutes,
			(Power.ACLineStatus == 1 || Power.BatteryFlag == 8) ? ", Charging" : "");
	}
	else
	{
		node_setf(node, "Power status", 0, "%u%%%s",
			Power.BatteryLifePercent,
			(Power.ACLineStatus == 1 || Power.BatteryFlag == 8) ? ", Charging" : "");
	}
}

static const char* mem_human_sizes[6] =
{ "B", "K", "M", "G", "T", "P", };

static void PrintMemInfo(void)
{
	PNODE nphy, npage;
	MEMORYSTATUSEX statex = { 0 };
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	node_setf(node, "Memory in use", 0, "%u%%", statex.dwMemoryLoad);
	nphy = node_append_new(node, "Physical memory", NFLG_ATTGROUP);
	node_att_set(nphy, "Free", GetHumanSize(statex.ullAvailPhys, mem_human_sizes, 1024), 0);
	node_att_set(nphy, "Total", GetHumanSize(statex.ullTotalPhys, mem_human_sizes, 1024), 0);
	npage = node_append_new(node, "Paging file", NFLG_ATTGROUP);
	node_att_set(npage, "Free", GetHumanSize(statex.ullAvailPageFile, mem_human_sizes, 1024), 0);
	node_att_set(npage, "Total", GetHumanSize(statex.ullTotalPageFile, mem_human_sizes, 1024), 0);
}

PNODE nwinfo_sys(void)
{
	node = node_alloc("System", 0);
	PrintOsVer();
	PrintOsInfo();
	PrintPowerInfo();
	PrintFwInfo();
	PrintTpmInfo();
	PrintMemInfo();
	return node;
}
