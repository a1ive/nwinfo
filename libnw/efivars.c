// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "libnw.h"
#include "utils.h"
#include "nt.h"
#include "efivars.h"

#if 0
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemEnvironmentValueEx(
	_In_ PUNICODE_STRING VariableName,
	_In_ GUID* VendorGuid,
	_Out_writes_bytes_opt_(*ValueLength) PVOID Value,
	_Inout_ PULONG ValueLength,
	_Out_opt_ PULONG Attributes // EFI_VARIABLE_*
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemEnvironmentValueEx(
	_In_ PUNICODE_STRING VariableName,
	_In_ PGUID VendorGuid,
	_In_reads_bytes_opt_(ValueLength) PVOID Value,
	_In_ ULONG ValueLength, // 0 = delete variable
	_In_ ULONG Attributes // EFI_VARIABLE_*
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateSystemEnvironmentValuesEx(
	_In_ ULONG InformationClass, // SYSTEM_ENVIRONMENT_INFORMATION_CLASS
	_Out_ PVOID Buffer,
	_Inout_ PULONG BufferLength
);
#endif
static GUID EFI_GV_GUID = { 0x8BE4DF61UL, 0x93CA, 0x11D2, { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C } };
static GUID EFI_EMPTY_GUID = { 0 };

BOOL
NWL_IsEfi(VOID)
{
	BOOL IsEfi = FALSE;
	SYSTEM_BOOT_ENVIRONMENT_INFORMATION BootInfo = { 0 };
	if (NWL_NtQuerySystemInformation(SystemBootEnvironmentInformation, &BootInfo, sizeof(BootInfo), NULL))
		IsEfi = (BootInfo.FirmwareType == FirmwareTypeUefi);
	else
	{
		NWL_GetEfiVar(L"", &EFI_EMPTY_GUID, NULL, 0, NULL);
		IsEfi = (GetLastError() != ERROR_INVALID_FUNCTION);
	}
	return IsEfi;
}

DWORD
NWL_GetEfiVar(LPCWSTR lpName, LPGUID lpGuid,
	PVOID pBuffer, DWORD nSize, PDWORD pdwAttribubutes)
{
	NTSTATUS rc;
	UNICODE_STRING str;
	ULONG(NTAPI * OsRtlNtStatusToDosError)(NTSTATUS Status) = NULL;
	VOID(NTAPI * OsRtlInitUnicodeString)(PUNICODE_STRING Src, PCWSTR Dst) = NULL;
	NTSTATUS(NTAPI * OsQuerySystemEnvironmentValueEx)(PUNICODE_STRING Name, LPGUID Guid, PVOID Value, PULONG Length, PULONG Attributes) = NULL;
	HMODULE hModule = GetModuleHandleW(L"ntdll");
	if (!hModule)
		goto fail;
	*(FARPROC*)&OsRtlNtStatusToDosError = GetProcAddress(hModule, "RtlNtStatusToDosError");
	if (!OsRtlNtStatusToDosError)
		goto fail;
	*(FARPROC*)&OsRtlInitUnicodeString = GetProcAddress(hModule, "RtlInitUnicodeString");
	if (!OsRtlInitUnicodeString)
		goto fail;
	*(FARPROC*)&OsQuerySystemEnvironmentValueEx = GetProcAddress(hModule, "NtQuerySystemEnvironmentValueEx");
	if (!OsQuerySystemEnvironmentValueEx)
		goto fail;
	OsRtlInitUnicodeString(&str, lpName);
	if (!lpGuid)
		lpGuid = &EFI_GV_GUID;
	rc = OsQuerySystemEnvironmentValueEx(&str, lpGuid, pBuffer, &nSize, pdwAttribubutes);
	if (!NT_SUCCESS(rc))
		SetLastError(OsRtlNtStatusToDosError(rc));
	return nSize;
fail:
	SetLastError(ERROR_INVALID_FUNCTION);
	return 0;
}

VOID*
NWL_GetEfiVarAlloc(LPCWSTR lpName, LPGUID lpGuid, PDWORD pdwSize, PDWORD pdwAttribubutes)
{
	DWORD nSize = 0;
	PVOID pBuffer = NULL;
	*pdwSize = 0;
	nSize = NWL_GetEfiVar(lpName, lpGuid, NULL, 0, pdwAttribubutes);
	if (!nSize)
		return NULL;
	pBuffer = calloc(nSize, 1);
	if (!pBuffer)
		return NULL;
	if (NWL_GetEfiVar(lpName, lpGuid, pBuffer, nSize, pdwAttribubutes) != nSize)
	{
		free(pBuffer);
		return NULL;
	}
	*pdwSize = nSize;
	return pBuffer;
}

static BOOL EnumerateEfiVar(ULONG InformationClass, PVOID Buffer, PULONG BufferLength)
{
	NTSTATUS rc;
	NTSTATUS(NTAPI * OsEnumerateSystemEnvironmentValuesEx)(ULONG InformationClass, PVOID Buffer, PULONG BufferLength) = NULL;
	HMODULE hModule = GetModuleHandleW(L"ntdll");
	if (!hModule)
		goto fail;
	*(FARPROC*)&OsEnumerateSystemEnvironmentValuesEx = GetProcAddress(hModule, "NtEnumerateSystemEnvironmentValuesEx");
	if (!OsEnumerateSystemEnvironmentValuesEx)
		goto fail;
	rc = OsEnumerateSystemEnvironmentValuesEx(InformationClass, Buffer, BufferLength);
	return NT_SUCCESS(rc);
fail:
	*BufferLength = 0;
	return FALSE;
}

PVARIABLE_NAME
NWL_EnumerateEfiVar(PULONG pulSize)
{
	PVOID VarNamePtr = NULL;
	EnumerateEfiVar(SystemEnvironmentNameInformation, NULL, pulSize);
	if (*pulSize == 0)
		goto fail;
	VarNamePtr = calloc(*pulSize, 1);
	if (!VarNamePtr)
		goto fail;
	if (!EnumerateEfiVar(SystemEnvironmentNameInformation, VarNamePtr, pulSize))
		goto fail;
	return VarNamePtr;
fail:
	if (VarNamePtr)
		free(VarNamePtr);
	*pulSize = 0;
	return NULL;
}

static LPCWSTR
GuidToStr(GUID* pGuid)
{
	static WCHAR GuidStr[39] = { 0 };
	swprintf(GuidStr, 39, L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
		pGuid->Data1, pGuid->Data2, pGuid->Data3,
		pGuid->Data4[0], pGuid->Data4[1], pGuid->Data4[2], pGuid->Data4[3],
		pGuid->Data4[4], pGuid->Data4[5], pGuid->Data4[6], pGuid->Data4[7]);
	return GuidStr;
}

BOOL
NWL_SetEfiVarEx(LPCWSTR lpName, LPGUID lpGuid, PVOID pBuffer, DWORD nSize, DWORD dwAttributes)
{
#if 0
	NTSTATUS rc;
	UNICODE_STRING str;
	ULONG(NTAPI * OsRtlNtStatusToDosError)(NTSTATUS Status) = NULL;
	VOID(NTAPI * OsRtlInitUnicodeString)(PUNICODE_STRING Src, PCWSTR Dst) = NULL;
	NTSTATUS(NTAPI * OsSetSystemEnvironmentValueEx)(PUNICODE_STRING Name, LPGUID Guid, PVOID Value, ULONG Length, ULONG Attributes) = NULL;
	HMODULE hModule = GetModuleHandleW(L"ntdll");
	if (!hModule)
		goto fail;
	*(FARPROC*)&OsRtlNtStatusToDosError = GetProcAddress(hModule, "RtlNtStatusToDosError");
	if (!OsRtlNtStatusToDosError)
		goto fail;
	*(FARPROC*)&OsRtlInitUnicodeString = GetProcAddress(hModule, "RtlInitUnicodeString");
	if (!OsRtlInitUnicodeString)
		goto fail;
	*(FARPROC*)&OsSetSystemEnvironmentValueEx = GetProcAddress(hModule, "NtSetSystemEnvironmentValueEx");
	if (!OsSetSystemEnvironmentValueEx)
		goto fail;
	OsRtlInitUnicodeString(&str, lpName);
	if (!lpGuid)
		lpGuid = &EFI_GV_GUID;
	rc = OsSetSystemEnvironmentValueEx(&str, lpGuid, pBuffer, nSize, dwAttributes);
	if (!NT_SUCCESS(rc))
		SetLastError(OsRtlNtStatusToDosError(rc));
	return NT_SUCCESS(rc);
#else
	BOOL(WINAPI * OsSetEfiVarExW)(LPCWSTR lpName, LPCWSTR lpGuid, PVOID pValue, DWORD nSize, DWORD dwAttributes) = NULL;
	HMODULE hModule = GetModuleHandleW(L"kernel32");
	if (!hModule)
		goto fail;
	*(FARPROC*)&OsSetEfiVarExW = GetProcAddress(hModule, "SetFirmwareEnvironmentVariableExW");
	if (!OsSetEfiVarExW)
		goto fail;
	if (!lpGuid)
		lpGuid = &EFI_GV_GUID;
	return OsSetEfiVarExW(lpName, GuidToStr(lpGuid), pBuffer, nSize, dwAttributes);
#endif
fail:
	SetLastError(ERROR_INVALID_FUNCTION);
	return FALSE;
}

BOOL
NWL_SetEfiVar(LPCWSTR lpName, LPGUID lpGuid, PVOID pBuffer, DWORD nSize)
{
	BOOL(WINAPI * OsSetEfiVarW)(LPCWSTR lpName, LPCWSTR lpGuid, PVOID pValue, DWORD nSize) = NULL;
	HMODULE hModule = GetModuleHandleW(L"kernel32");
	if (!hModule)
		goto fail;
	*(FARPROC*)&OsSetEfiVarW = GetProcAddress(hModule, "SetFirmwareEnvironmentVariableW");
	if (!OsSetEfiVarW)
		goto fail;
	if (!lpGuid)
		lpGuid = &EFI_GV_GUID;
	return OsSetEfiVarW(lpName, GuidToStr(lpGuid), pBuffer, nSize);
fail:
	SetLastError(ERROR_INVALID_FUNCTION);
	return FALSE;
}

BOOL
NWL_DeleteEfiVar(LPCWSTR lpName, LPGUID lpGuid)
{
	return NWL_SetEfiVar(lpName, lpGuid, NULL, 0);
}
