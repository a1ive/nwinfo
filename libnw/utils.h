// SPDX-License-Identifier: Unlicense
#pragma once

#include <windows.h>
#include "format.h"

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

struct acpi_rsdp_v2* NWL_GetRsdp(VOID);
struct acpi_rsdt* NWL_GetRsdt(VOID);
struct acpi_xsdt* NWL_GetXsdt(VOID);
PVOID NWL_GetAcpi(DWORD TableId);
PVOID NWL_GetAcpiByAddr(DWORD_PTR Addr);

#define EFI_VARIABLE_NON_VOLATILE 0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS 0x00000004
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD 0x00000008
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS 0x00000010
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x00000020
#define EFI_VARIABLE_APPEND_WRITE 0x00000040
#define EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS 0x00000080

typedef struct _VARIABLE_NAME
{
	ULONG NextEntryOffset;
	GUID VendorGuid;
	WCHAR Name[ANYSIZE_ARRAY];
} VARIABLE_NAME, * PVARIABLE_NAME;

BOOL NWL_IsEfi(VOID);
DWORD NWL_GetEfiVar(LPCWSTR lpName, LPGUID lpGuid,
	PVOID pBuffer, DWORD nSize, PDWORD pdwAttribubutes);
VOID* NWL_GetEfiVarAlloc(LPCWSTR lpName, LPGUID lpGuid,
	PDWORD pdwSize, PDWORD pdwAttribubutes);
BOOL NWL_EnumerateEfiVar(PVARIABLE_NAME pVarName, PULONG pulSize);

UINT8 NWL_AcpiChecksum(VOID* base, UINT size);
VOID NWL_TrimString(CHAR* String);
INT NWL_GetRegDwordValue(HKEY Key, LPCSTR SubKey, LPCSTR ValueName, DWORD* pValue);
CHAR* NWL_GetRegSzValue(HKEY Key, LPCSTR SubKey, LPCSTR ValueName);
HANDLE NWL_GetDiskHandleById(BOOL Cdrom, BOOL Write, DWORD Id);
LPCSTR NWL_GetBusTypeString(STORAGE_BUS_TYPE Type);
LPCSTR NWL_GuidToStr(UCHAR Guid[16]);
LPCSTR NWL_WinGuidToStr(BOOL bBracket, GUID* pGuid);
LPCSTR NWL_WcsToMbs(PWCHAR Wcs);

HANDLE NWL_NtCreateFile(LPCWSTR lpFileName, BOOL bWrite);
VOID* NWL_NtGetRegValue(HKEY Key, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPDWORD lpType);
LPCSTR NWL_NtGetPathFromHandle(HANDLE hFile);
BOOL NWL_NtQuerySystemInformation(INT SystemInformationClass,
	PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);

VOID NWL_FindId(PNODE nd, CHAR* Ids, DWORD IdsSize, CONST CHAR* v, CONST CHAR* d, CONST CHAR* s, INT usb);
VOID NWL_FindClass(PNODE nd, CHAR* Ids, DWORD IdsSize, CONST CHAR* Class, INT usb);
VOID NWL_GetPnpManufacturer(PNODE nd, CHAR* Ids, DWORD IdsSize, CONST CHAR* Code);
VOID NWL_GetSpdManufacturer(PNODE nd, CHAR* Ids, DWORD IdsSize, UINT Bank, UINT Item);
CHAR* NWL_LoadIdsToMemory(LPCWSTR lpFileName, LPDWORD lpSize);
const CHAR* NWL_GetIdsDate(LPCWSTR lpFileName);
