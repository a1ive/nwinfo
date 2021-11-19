// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winbase.h>
#include <sysinfoapi.h>
#include <stdio.h>
#include <stdlib.h>
#include "nwinfo.h"

static const char* GV_GUID = "{8BE4DF61-93CA-11D2-AA0D-00E098032B8C}";

#define INFO_BUFFER_SIZE 32767

static void PrintOsInfo(void)
{
	DWORD bufCharCount = INFO_BUFFER_SIZE;
	SYSTEM_INFO SystemInfo;
	char* infoBuf = NULL;
	UINT64 Uptime = 0;
	infoBuf = malloc(INFO_BUFFER_SIZE);
	if (!infoBuf)
		return;
	if (GetComputerNameA(infoBuf, &bufCharCount))
		printf("Computer Name: %s\n", infoBuf);
	bufCharCount = INFO_BUFFER_SIZE;
	if (GetUserNameA(infoBuf, &bufCharCount))
		printf("User name: %s\n", infoBuf);
	if (GetSystemDirectoryA(infoBuf, INFO_BUFFER_SIZE))
		printf("System Directory: %s\n", infoBuf);
	if (GetWindowsDirectoryA(infoBuf, INFO_BUFFER_SIZE))
		printf("Windows Directory: %s\n", infoBuf);
	Uptime = GetTickCount64();
	{
		UINT64 Days = Uptime / 1000ULL / 3600ULL / 24ULL;
		UINT64 Hours = Uptime / 1000ULL / 3600ULL - Days * 24ULL;
		UINT64 Minutes = Uptime / 1000ULL / 60ULL - Days * 24ULL * 60ULL - Hours * 60ULL;
		UINT64 Seconds = Uptime / 1000ULL - Days * 24ULL * 3600ULL - Hours * 3600ULL - Minutes * 60ULL;
		printf("UpTime: %llu days, %llu hours, %llu min, %llu sec\n", Days, Hours, Minutes, Seconds);
	}
	GetNativeSystemInfo(&SystemInfo);
	switch (SystemInfo.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_AMD64:
		printf("Processor Architecture: x64\n");
		break;
	case PROCESSOR_ARCHITECTURE_INTEL:
		printf("Processor Architecture: x86\n");
		break;
	default:
		printf("Processor Architecture: Unknown\n");
		break;
	}
	printf("Page Size: %u\n", SystemInfo.dwPageSize);
	free(infoBuf);
}

static void PrintFwInfo(void)
{
	FIRMWARE_TYPE FwType = FirmwareTypeUnknown;
	DWORD VarSize = 0;
	UINT8 SecureBoot = 0;
	GetFirmwareEnvironmentVariableA("", "{00000000-0000-0000-0000-000000000000}", NULL, 0);
	if (GetLastError() == ERROR_INVALID_FUNCTION)
		printf("Firmware: Legacy BIOS\n");
	else
	{
		printf("Firmware: UEFI\n");
		VarSize = GetFirmwareEnvironmentVariableA("SecureBoot", GV_GUID, &SecureBoot, sizeof(uint8_t));
		if (VarSize)
			printf("Secure Boot: %s\n", SecureBoot ? "enabled" : "disabled");
		else
			printf("Secure Boot: unsupported\n");
		
	}
}

static const char* mem_human_sizes[6] =
{ "B", "K", "M", "G", "T", "P", };

static void PrintMemInfo(void)
{
	MEMORYSTATUSEX statex = { 0 };
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	printf("Memory in use: %u%%\n", statex.dwMemoryLoad);

	printf("Physical memory (free/total): %s", GetHumanSize(statex.ullAvailPhys, mem_human_sizes, 1024));
	printf("/%s\n", GetHumanSize(statex.ullTotalPhys, mem_human_sizes, 1024));

	printf("Paging file (free/total): %s", GetHumanSize(statex.ullAvailPageFile, mem_human_sizes, 1024));
	printf("/%s\n", GetHumanSize(statex.ullTotalPageFile, mem_human_sizes, 1024));
}

void nwinfo_sys(void)
{
	ObtainPrivileges(SE_SYSTEM_ENVIRONMENT_NAME);

	PrintOsInfo();
	PrintFwInfo();
	PrintMemInfo();
}
