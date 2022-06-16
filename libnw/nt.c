// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winioctl.h>
#include "libnw.h"
#include "utils.h"

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _IO_STATUS_BLOCK
{
	union
	{
		NTSTATUS Status;
		PVOID Pointer;
	};
	ULONG_PTR Information;
} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;

typedef struct _OBJECT_ATTRIBUTES
{
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES ); \
    (p)->RootDirectory = r; \
    (p)->Attributes = a; \
    (p)->ObjectName = n; \
    (p)->SecurityDescriptor = s; \
    (p)->SecurityQualityOfService = NULL; \
}

#define FILE_SUPERSEDE                  0x00000000
#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE                  0x00000004
#define FILE_OVERWRITE_IF               0x00000005
#define FILE_MAXIMUM_DISPOSITION        0x00000005

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef enum _OBJECT_INFORMATION_CLASS
{
	ObjectBasicInformation,
	ObjectNameInformation,
	ObjectTypeInformation,
	ObjectAllInformation,
	ObjectDataInformation
} OBJECT_INFORMATION_CLASS;

typedef struct _OBJECT_NAME_INFORMATION
{
	UNICODE_STRING Name;
	WCHAR NameBuffer[0];
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

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
	HMODULE hModule = LoadLibraryA("ntdll.dll");
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
	HMODULE hModule = LoadLibraryA("ntdll.dll");
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
