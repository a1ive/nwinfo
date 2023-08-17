// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winioctl.h>
#include "libnw.h"
#include "utils.h"

HANDLE
NWL_NtCreateFile(LPCWSTR lpFileName, BOOL bWrite)
{
	HANDLE hFile;
	NTSTATUS rc;
	UNICODE_STRING str;
	ACCESS_MASK mask;
	IO_STATUS_BLOCK status;
	OBJECT_ATTRIBUTES attributes = { 0 };
	VOID(NTAPI * OsRtlInitUnicodeString)(PUNICODE_STRING Dst, PCWSTR Src) = NULL;
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

#define MB_TO_BYTES(mb) (1024ULL * 1024ULL * mb)
BOOL
NWL_NtCreatePageFile(WCHAR wDrive, LPCWSTR lpPath, UINT64 minSizeInMb, UINT64 maxSizeInMb)
{
	NTSTATUS rc;
	UNICODE_STRING str;
	WCHAR device[] = L"C:";
	WCHAR buf[MAX_PATH];
	ULARGE_INTEGER minSize, maxSize;
	VOID(NTAPI * OsRtlInitUnicodeString)(PUNICODE_STRING Dst, PCWSTR Src) = NULL;
	NTSTATUS(NTAPI * NtCreatePagingFile)(PUNICODE_STRING Path, PULARGE_INTEGER MinSize, PULARGE_INTEGER MaxSize, ULONG Priority) = NULL;
	HMODULE hModule = GetModuleHandleW(L"ntdll");
	if (!hModule)
		return FALSE;
	*(FARPROC*)&NtCreatePagingFile = GetProcAddress(hModule, "NtCreatePagingFile");
	if (!NtCreatePagingFile)
		return FALSE;
	*(FARPROC*)&OsRtlInitUnicodeString = GetProcAddress(hModule, "RtlInitUnicodeString");
	if (!OsRtlInitUnicodeString)
		return FALSE;
	if (NWL_ObtainPrivileges(SE_CREATE_PAGEFILE_NAME) != ERROR_SUCCESS)
		return FALSE;
	if (maxSizeInMb < minSizeInMb)
		maxSizeInMb = minSizeInMb;
	if (lpPath == NULL)
		lpPath = L"\\pagefile.sys";
	device[0] = towupper(wDrive);
	if (QueryDosDeviceW(device, buf, MAX_PATH) > 0)
		wcscat_s(buf, MAX_PATH, lpPath);
	else
		return FALSE;
	OsRtlInitUnicodeString(&str, buf);
	minSize.QuadPart = MB_TO_BYTES(minSizeInMb);
	maxSize.QuadPart = MB_TO_BYTES(maxSizeInMb);
	rc = NtCreatePagingFile(&str, &minSize, &maxSize, 0);
	if (!NT_SUCCESS(rc))
		return FALSE;
	return TRUE;
}

VOID* NWL_NtGetRegValue(HKEY Key, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPDWORD lpdwSize, LPDWORD lpType)
{
	HKEY hKey = NULL;
	DWORD cbSize = 0;
	LSTATUS lRet;
	VOID* lpData = NULL;

	lRet = RegOpenKeyExW(Key, lpSubKey, 0, KEY_QUERY_VALUE, &hKey);
	if (lRet != ERROR_SUCCESS)
		goto fail;
	lRet = RegQueryValueExW(hKey, lpValueName, NULL, lpType, NULL, &cbSize);
	if (lRet != ERROR_SUCCESS || cbSize == 0)
		goto fail;
	lpData = malloc(cbSize);
	if (!lpData)
		goto fail;
	lRet = RegQueryValueExW(hKey, lpValueName, NULL, lpType, lpData, &cbSize);
	if (lRet != ERROR_SUCCESS)
		goto fail;
	RegCloseKey(hKey);
	if (lpdwSize)
		*lpdwSize = cbSize;
	return lpData;
fail:
	if (hKey)
		RegCloseKey(hKey);
	if (lpData)
		free(lpData);
	if (lpdwSize)
		*lpdwSize = 0;
	return NULL;
}

BOOL
NWL_NtSetRegValue(HKEY Key, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPCVOID lpData, DWORD dwSize, DWORD dwType)
{
	HKEY hKey = NULL;
	LSTATUS lRet;
	lRet = RegOpenKeyExW(Key, lpSubKey, 0, KEY_SET_VALUE, &hKey);
	if (lRet != ERROR_SUCCESS)
		return FALSE;
	lRet = RegSetValueExW(hKey, lpValueName, 0, dwType, lpData, dwSize);
	RegCloseKey(hKey);
	return lRet == ERROR_SUCCESS;
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
	return NWL_Ucs2ToUtf8(puName->Buffer);
}

BOOL NWL_NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength)
{
	NTSTATUS (NTAPI * OsNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS SystemInformationClass,
				PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength) = NULL;
	HMODULE hModule = GetModuleHandleW(L"ntdll");
	if (!hModule)
		return FALSE;
	*(FARPROC*)&OsNtQuerySystemInformation = GetProcAddress(hModule, "NtQuerySystemInformation");
	if (!OsNtQuerySystemInformation)
		return FALSE;
	return NT_SUCCESS(OsNtQuerySystemInformation(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength));
}

BOOL NWL_NtSetSystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation, ULONG SystemInformationLength)
{
	NTSTATUS(NTAPI * OsNtSetSystemInformation)(SYSTEM_INFORMATION_CLASS SystemInformationClass,
		PVOID SystemInformation, ULONG SystemInformationLength) = NULL;
	HMODULE hModule = GetModuleHandleW(L"ntdll");
	if (!hModule)
		return FALSE;
	*(FARPROC*)&OsNtSetSystemInformation = GetProcAddress(hModule, "NtSetSystemInformation");
	if (!OsNtSetSystemInformation)
		return FALSE;
	return NT_SUCCESS(OsNtSetSystemInformation(SystemInformationClass, SystemInformation, SystemInformationLength));
}

VOID NWL_NtGetVersion(LPOSVERSIONINFOEXW osInfo)
{
	NTSTATUS(WINAPI * RtlGetVersion)(LPOSVERSIONINFOEXW) = NULL;
	HMODULE hMod = GetModuleHandleW(L"ntdll");

	ZeroMemory(osInfo, sizeof(OSVERSIONINFOEXW));

	if (hMod)
		*(FARPROC*)&RtlGetVersion = GetProcAddress(hMod, "RtlGetVersion");

	if (RtlGetVersion)
	{
		osInfo->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
		RtlGetVersion(osInfo);
	}
}
