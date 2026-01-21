// SPDX-License-Identifier: Unlicense
#pragma once

#define VC_EXTRALEAN
#include <windows.h>
#include <cfgmgr32.h>
#include "node.h"
#include "nt.h"
#include "nwapi.h"

struct RAW_SMBIOS_DATA;
struct ACPI_RSDP_V2;
struct ACPI_RSDT;
struct ACPI_XSDT;
struct ACPI_FADT;
struct ACPI_FACS;
struct _NWLIB_IDS;

LIBNW_API BOOL NWL_IsAdmin(void);
LIBNW_API DWORD NWL_ObtainPrivileges(LPWSTR privilege);
LIBNW_API LPCSTR NWL_GetHumanSize(UINT64 size, LPCSTR human_sizes[6], UINT64 base);
LIBNW_API LPCSTR NWL_GetHumanTime(UINT64 ullSecond);

LIBNW_API BOOL NWL_ReadMemory(PVOID buffer, DWORD_PTR address, DWORD length);

void NWL_ConvertLengthToIpv4Mask(ULONG MaskLength, ULONG* Mask);

LIBNW_API struct RAW_SMBIOS_DATA* NWL_GetSmbios(void);
LIBNW_API struct ACPI_RSDP_V2* NWL_GetRsdp(VOID);
LIBNW_API struct ACPI_RSDT* NWL_GetRsdt(VOID);
LIBNW_API struct ACPI_XSDT* NWL_GetXsdt(VOID);
LIBNW_API PVOID NWL_GetSysAcpi(DWORD TableId);
LIBNW_API PVOID NWL_GetAcpiByAddr(DWORD_PTR Addr);

LIBNW_API UINT8 NWL_AcpiChecksum(VOID* base, UINT size);
INT NWL_GetRegDwordValue(HKEY Key, LPCWSTR SubKey, LPCWSTR ValueName, DWORD* pValue);
HANDLE NWL_GetDiskHandleById(BOOL Cdrom, BOOL Write, DWORD Id);
LPCSTR NWL_GetBusTypeString(STORAGE_BUS_TYPE Type);
LPCSTR NWL_GuidToStr(UCHAR Guid[16]);
LPCSTR NWL_WinGuidToStr(BOOL bBracket, GUID* pGuid);
BOOL NWL_StrToGuid(const CHAR* cchText, GUID* pGuid);
PBYTE NWL_LoadDump(LPCSTR pPath, DWORD minSize, DWORD* outSize);
#if 0
LIBNW_API HMONITOR NWL_GetMonitorFromName(LPCWSTR lpDevice);
#endif
LPCSTR NWL_UnixTimeToStr(INT nix);
LPCSTR NWL_Ucs2ToUtf8(LPCWSTR src);
LPCWSTR NWL_Utf8ToUcs2(LPCSTR src);

HANDLE NWL_NtCreateFile(LPCWSTR lpFileName, BOOL bWrite);
LIBNW_API BOOL NWL_NtCreatePageFile(WCHAR wDrive, LPCWSTR lpPath, UINT64 minSizeInMb, UINT64 maxSizeInMb);
VOID* NWL_NtGetRegValue(HKEY Key, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPDWORD lpdwSize, LPDWORD lpType);
BOOL NWL_NtSetRegValue(HKEY Key, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPCVOID lpData, DWORD dwSize, DWORD dwType);
LPCSTR NWL_NtGetPathFromHandle(HANDLE hFile);
BOOL NWL_NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
BOOL NWL_NtSetSystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation, ULONG SystemInformationLength);
LIBNW_API VOID NWL_NtGetVersion(LPOSVERSIONINFOEXW osInfo);
LIBNW_API PPROCESSOR_POWER_INFORMATION NWL_NtPowerInformation(size_t* szCount);

BOOL NWL_ParseHwid(PNODE nd, struct _NWLIB_IDS* Ids, LPCWSTR Hwid, INT usb);
VOID NWL_FindClass(PNODE nd, struct _NWLIB_IDS* Ids, CONST CHAR* Class, INT usb);
VOID NWL_GetPnpManufacturer(PNODE nd, struct _NWLIB_IDS* Ids, CONST CHAR* Code);
VOID NWL_GetSpdManufacturer(PNODE nd, LPCSTR Key, struct _NWLIB_IDS* Ids, UINT Bank, UINT Item);
LIBNW_API BOOL NWL_LoadIdsToMemory(LPCWSTR lpFileName, struct _NWLIB_IDS* lpIds);
LIBNW_API VOID NWL_UnloadIds(struct _NWLIB_IDS* lpIds);
const CHAR* NWL_GetIdsDate(struct _NWLIB_IDS* Ids);
