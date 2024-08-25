// SPDX-License-Identifier: Unlicense
#pragma once

#define VC_EXTRALEAN
#include <windows.h>
#include "format.h"
#include "nt.h"

struct RAW_SMBIOS_DATA;
struct acpi_rsdp_v2;
struct acpi_rsdt;
struct acpi_xsdt;

BOOL NWL_IsAdmin(void);
DWORD NWL_ObtainPrivileges(LPWSTR privilege);
LPCSTR NWL_GetHumanSize(UINT64 size, LPCSTR human_sizes[6], UINT64 base);

BOOL NWL_ReadMemory(PVOID buffer, DWORD_PTR address, DWORD length);

void NWL_ConvertLengthToIpv4Mask(ULONG MaskLength, ULONG* Mask);

UINT NWL_GetSystemFirmwareTable(DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID,
	PVOID pFirmwareTableBuffer, DWORD BufferSize);

struct RAW_SMBIOS_DATA* NWL_GetSmbios(void);
struct acpi_rsdp_v2* NWL_GetRsdp(VOID);
struct acpi_rsdt* NWL_GetRsdt(VOID);
struct acpi_xsdt* NWL_GetXsdt(VOID);
PVOID NWL_GetAcpi(DWORD TableId);
PVOID NWL_GetAcpiByAddr(DWORD_PTR Addr);

UINT8 NWL_AcpiChecksum(VOID* base, UINT size);
INT NWL_GetRegDwordValue(HKEY Key, LPCWSTR SubKey, LPCWSTR ValueName, DWORD* pValue);
HANDLE NWL_GetDiskHandleById(BOOL Cdrom, BOOL Write, DWORD Id);
LPCSTR NWL_GetBusTypeString(STORAGE_BUS_TYPE Type);
LPCSTR NWL_GuidToStr(UCHAR Guid[16]);
LPCSTR NWL_WinGuidToStr(BOOL bBracket, GUID* pGuid);
BOOL NWL_StrToGuid(const CHAR* cchText, GUID* pGuid);
HMONITOR NWL_GetMonitorFromName(LPCWSTR lpDevice);
LPCSTR NWL_UnixTimeToStr(INT nix);
LPCSTR NWL_Ucs2ToUtf8(LPCWSTR src);
LPCWSTR NWL_Utf8ToUcs2(LPCSTR src);

HANDLE NWL_NtCreateFile(LPCWSTR lpFileName, BOOL bWrite);
BOOL NWL_NtCreatePageFile(WCHAR wDrive, LPCWSTR lpPath, UINT64 minSizeInMb, UINT64 maxSizeInMb);
VOID* NWL_NtGetRegValue(HKEY Key, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPDWORD lpdwSize, LPDWORD lpType);
BOOL NWL_NtSetRegValue(HKEY Key, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPCVOID lpData, DWORD dwSize, DWORD dwType);
LPCSTR NWL_NtGetPathFromHandle(HANDLE hFile);
BOOL NWL_NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
BOOL NWL_NtSetSystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation, ULONG SystemInformationLength);
VOID NWL_NtGetVersion(LPOSVERSIONINFOEXW osInfo);

VOID NWL_FindId(PNODE nd, CHAR* Ids, DWORD IdsSize, CONST CHAR* v, CONST CHAR* d, CONST CHAR* s, INT usb);
BOOL NWL_ParseHwid(PNODE nd, CHAR* Ids, DWORD IdsSize, LPCWSTR Hwid, INT usb);
VOID NWL_FindClass(PNODE nd, CHAR* Ids, DWORD IdsSize, CONST CHAR* Class, INT usb);
VOID NWL_GetPnpManufacturer(PNODE nd, CHAR* Ids, DWORD IdsSize, CONST CHAR* Code);
VOID NWL_GetSpdManufacturer(PNODE nd, CHAR* Ids, DWORD IdsSize, UINT Bank, UINT Item);
CHAR* NWL_LoadIdsToMemory(LPCWSTR lpFileName, LPDWORD lpSize);
const CHAR* NWL_GetIdsDate(LPCWSTR lpFileName);
