// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winioctl.h>
#include "libnw.h"
#include "utils.h"
#include "nt.h"

HANDLE
NWL_NtCreateFile(LPCWSTR lpFileName, BOOL bWrite)
{
	HANDLE hFile;
	NTSTATUS rc;
	UNICODE_STRING str;
	ACCESS_MASK mask;
	IO_STATUS_BLOCK status;
	OBJECT_ATTRIBUTES attributes = { 0 };
	VOID(NTAPI * OsRtlInitUnicodeString)(PUNICODE_STRING Src, PCWSTR Dst) = NULL;
	NTSTATUS(NTAPI * OsNtCreateFile)(PHANDLE FileHandle, ACCESS_MASK DesiredAccess,
		POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock,
		PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess,
		ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength) = NULL;
	HMODULE hModule = GetModuleHandleW(L"ntdll");
	if (!hModule)
		return INVALID_HANDLE_VALUE;
	*(FARPROC*)&OsNtCreateFile = GetProcAddress(hModule, "NtCreateFile");
	if (!OsNtCreateFile)
		return INVALID_HANDLE_VALUE;
	*(FARPROC*)&OsRtlInitUnicodeString = GetProcAddress(hModule, "RtlInitUnicodeString");
	if (!OsRtlInitUnicodeString)
		return INVALID_HANDLE_VALUE;
	OsRtlInitUnicodeString(&str, lpFileName);
	InitializeObjectAttributes(&attributes, &str, 0, NULL, NULL);
	if (bWrite)
		mask = GENERIC_WRITE | GENERIC_READ;
	else
		mask = GENERIC_READ;
	rc = OsNtCreateFile(&hFile, mask, &attributes, &status,
		NULL, FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, 0, NULL, 0);
	if (NT_SUCCESS(rc))
		return hFile;
	return NULL;
}

VOID* NWL_NtGetRegValue(HKEY Key, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPDWORD lpType)
{
	HKEY hKey;
	DWORD cbSize = 0;
	LSTATUS lRet;
	VOID* lpData = NULL;
	lRet = RegOpenKeyExW(Key, lpSubKey, 0, KEY_QUERY_VALUE, &hKey);
	if (lRet != ERROR_SUCCESS)
		return NULL;
	lRet = RegQueryValueExW(hKey, lpValueName, NULL, lpType, NULL, &cbSize);
	if (lRet != ERROR_SUCCESS || cbSize == 0)
		return NULL;
	lpData = malloc(cbSize);
	if (!lpData)
		return NULL;
	lRet = RegQueryValueExW(hKey, lpValueName, NULL, lpType, lpData, &cbSize);
	if (lRet != ERROR_SUCCESS)
	{
		free(lpData);
		return NULL;
	}
	RegCloseKey(hKey);
	return lpData;
}

LPCSTR NWL_NtGetPathFromHandle(HANDLE hFile)
{
	BYTE  tmp[2048] = { 0 };
	DWORD dwLength = 0;
	PUNICODE_STRING puName = NULL;
	NTSTATUS (NTAPI *OsNtQueryObject)(HANDLE Handle, OBJECT_INFORMATION_CLASS Info,
		PVOID Buffer, ULONG BufferSize, PULONG ReturnLength) = NULL;
	HMODULE hModule = GetModuleHandleW(L"ntdll");
	if (!hModule || !hFile || hFile == INVALID_HANDLE_VALUE)
		return NULL;
	*(FARPROC*)&OsNtQueryObject = GetProcAddress(hModule, "NtQueryObject");
	if (!OsNtQueryObject)
		return NULL;
	OsNtQueryObject(hFile, ObjectNameInformation, tmp, sizeof(tmp), &dwLength);
	puName = &((POBJECT_NAME_INFORMATION)tmp)->Name;
	if (!puName->Length || !puName->Buffer)
		return NULL;
	return NWL_WcsToMbs(puName->Buffer);
}

BOOL NWL_NtQuerySystemInformation(INT SystemInformationClass,
	PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength)
{
	NTSTATUS (NTAPI * OsNtQuerySystemInformation)(INT SystemInformationClass,
				PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength) = NULL;
	HMODULE hModule = GetModuleHandleW(L"ntdll");
	if (!hModule)
		return FALSE;
	*(FARPROC*)&OsNtQuerySystemInformation = GetProcAddress(hModule, "NtQuerySystemInformation");
	if (!OsNtQuerySystemInformation)
		return FALSE;
	return NT_SUCCESS(OsNtQuerySystemInformation(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength));
}
