// SPDX-License-Identifier: Unlicense
#pragma once

#include <windows.h>
#include "format.h"

BOOL NWL_IsAdmin(void);
DWORD NWL_ObtainPrivileges(LPCSTR privilege);
LPCSTR NWL_GetHumanSize(UINT64 size, LPCSTR human_sizes[6], UINT64 base);

BOOL NWL_NT5InitMemory(void);
void NWL_NT5ExitMemory(void);
BOOL NWL_NT5ReadMemory(PVOID buffer, DWORD address, DWORD length);

void NWL_ConvertLengthToIpv4Mask(ULONG MaskLength, ULONG* Mask);
UINT NWL_EnumSystemFirmwareTables(DWORD FirmwareTableProviderSignature, PVOID pFirmwareTableEnumBuffer, DWORD BufferSize);
UINT NWL_GetSystemFirmwareTable(DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID,
	PVOID pFirmwareTableBuffer, DWORD BufferSize);
DWORD NWL_GetFirmwareEnvironmentVariable(LPCSTR lpName, LPCSTR lpGuid,
	PVOID pBuffer, DWORD nSize);

PVOID NWL_GetAcpi(DWORD TableId);
UINT8 NWL_AcpiChecksum(VOID* base, UINT size);
VOID NWL_TrimString(CHAR* String);
INT NWL_GetRegDwordValue(HKEY Key, LPCSTR SubKey, LPCSTR ValueName, DWORD* pValue);
CHAR* NWL_GetRegSzValue(HKEY Key, LPCSTR SubKey, LPCSTR ValueName);
VOID NWL_FindId(PNODE nd, CHAR* Ids, DWORD IdsSize, CONST CHAR* v, CONST CHAR* d, CONST CHAR* s, INT usb);
VOID NWL_FindClass(PNODE nd, CHAR* Ids, DWORD IdsSize, CONST CHAR* Class);
LPCSTR NWL_GuidToStr(UCHAR Guid[16]);
LPCSTR NWL_WcsToMbs(PWCHAR Wcs);
