// SPDX-License-Identifier: Unlicense
#pragma once

#include <windows.h>
#include "format.h"

struct acpi_rsdp_v2;
struct acpi_rsdt;
struct acpi_xsdt;

BOOL NWL_IsAdmin(void);
DWORD NWL_ObtainPrivileges(LPCSTR privilege);
LPCSTR NWL_GetHumanSize(UINT64 size, LPCSTR human_sizes[6], UINT64 base);

BOOL NWL_ReadMemory(PVOID buffer, DWORD_PTR address, DWORD length);

void NWL_ConvertLengthToIpv4Mask(ULONG MaskLength, ULONG* Mask);

UINT NWL_GetSystemFirmwareTable(DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID,
	PVOID pFirmwareTableBuffer, DWORD BufferSize);

struct acpi_rsdp_v2* NWL_GetRsdp(VOID);
struct acpi_rsdt* NWL_GetRsdt(VOID);
struct acpi_xsdt* NWL_GetXsdt(VOID);
PVOID NWL_GetAcpi(DWORD TableId);
PVOID NWL_GetAcpiByAddr(DWORD_PTR Addr);

DWORD NWL_GetFirmwareEnvironmentVariable(LPCSTR lpName, LPCSTR lpGuid,
	PVOID pBuffer, DWORD nSize);

UINT8 NWL_AcpiChecksum(VOID* base, UINT size);
VOID NWL_TrimString(CHAR* String);
INT NWL_GetRegDwordValue(HKEY Key, LPCSTR SubKey, LPCSTR ValueName, DWORD* pValue);
CHAR* NWL_GetRegSzValue(HKEY Key, LPCSTR SubKey, LPCSTR ValueName);
HANDLE NWL_GetDiskHandleById(BOOL Cdrom, BOOL Write, DWORD Id);
LPCSTR NWL_GetBusTypeString(STORAGE_BUS_TYPE Type);
VOID NWL_FindId(PNODE nd, CHAR* Ids, DWORD IdsSize, CONST CHAR* v, CONST CHAR* d, CONST CHAR* s, INT usb);
VOID NWL_FindClass(PNODE nd, CHAR* Ids, DWORD IdsSize, CONST CHAR* Class, INT usb);
CHAR* NWL_LoadFileToMemory(LPCSTR lpFileName, LPDWORD lpSize);
LPCSTR NWL_GuidToStr(UCHAR Guid[16]);
LPCSTR NWL_WcsToMbs(PWCHAR Wcs);

HANDLE NWL_NtCreateFile(LPCWSTR lpFileName, BOOL bWrite);
VOID* NWL_NtGetRegValue(HKEY Key, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPDWORD lpType);
LPCSTR NWL_NtGetPathFromHandle(HANDLE hFile);
