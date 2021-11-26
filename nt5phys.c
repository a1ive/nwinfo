// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "nwinfo.h"

#ifndef _WIN64
typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef enum _SECTION_INHERIT
{
	ViewShare = 1,
	ViewUnmap = 2
} SECTION_INHERIT, * PSECTION_INHERIT;

typedef struct _OBJECT_ATTRIBUTES
{
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES ); \
    (p)->RootDirectory = r; \
    (p)->Attributes = a; \
    (p)->ObjectName = n; \
    (p)->SecurityDescriptor = s; \
    (p)->SecurityQualityOfService = NULL; \
}

typedef NTSTATUS(WINAPI* ZwOpenSectionProc)
(
	PHANDLE SectionHandle,
	DWORD DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes
	);
typedef NTSTATUS(WINAPI* ZwMapViewOfSectionProc)
(
	HANDLE SectionHandle,
	HANDLE ProcessHandle,
	PVOID* BaseAddress,
	ULONG ZeroBits,
	ULONG CommitSize,
	PLARGE_INTEGER SectionOffset,
	PULONG ViewSize,
	SECTION_INHERIT InheritDisposition,
	ULONG AllocationType,
	ULONG Protect
	);
typedef NTSTATUS(WINAPI* ZwUnmapViewOfSectionProc)
(
	HANDLE ProcessHandle,
	PVOID BaseAddress
	);
typedef VOID(WINAPI* RtlInitUnicodeStringProc)
(
	IN OUT PUNICODE_STRING DestinationString,
	IN PCWSTR SourceString
	);

#define STATUS_SUCCESS 0x00000000UL

static HMODULE hModule = NULL;
static HANDLE hPhysicalMemory = NULL;
static ZwOpenSectionProc ZwOpenSection;
static ZwMapViewOfSectionProc ZwMapViewOfSection;
static ZwUnmapViewOfSectionProc ZwUnmapViewOfSection;
static RtlInitUnicodeStringProc RtlInitUnicodeString;

BOOL InitPhysicalMemory(void)
{
	if (!(hModule = LoadLibrary("ntdll.dll")))
	{
		return FALSE;
	}

	if (!(ZwOpenSection = (ZwOpenSectionProc)GetProcAddress(hModule, "ZwOpenSection")))
	{
		return FALSE;
	}

	if (!(ZwMapViewOfSection = (ZwMapViewOfSectionProc)GetProcAddress(hModule, "ZwMapViewOfSection")))
	{
		return FALSE;
	}

	if (!(ZwUnmapViewOfSection = (ZwUnmapViewOfSectionProc)GetProcAddress(hModule, "ZwUnmapViewOfSection")))
	{
		return FALSE;
	}

	if (!(RtlInitUnicodeString = (RtlInitUnicodeStringProc)GetProcAddress(hModule, "RtlInitUnicodeString")))
	{
		return FALSE;
	}

	WCHAR PhysicalMemoryName[] = L"\\Device\\PhysicalMemory";
	UNICODE_STRING PhysicalMemoryString;
	OBJECT_ATTRIBUTES attributes = { 0 };
	RtlInitUnicodeString(&PhysicalMemoryString, PhysicalMemoryName);
	InitializeObjectAttributes(&attributes, &PhysicalMemoryString, 0, NULL, NULL);
	NTSTATUS status = ZwOpenSection(&hPhysicalMemory, SECTION_MAP_READ, &attributes);

	return (status >= 0);
}

void ExitPhysicalMemory(void)
{
	if (hPhysicalMemory != NULL)
	{
		CloseHandle(hPhysicalMemory);
	}

	if (hModule != NULL)
	{
		FreeLibrary(hModule);
	}
}

BOOL ReadPhysicalMemory(PVOID buffer, DWORD address, DWORD length)
{
	DWORD outlen;
	PVOID vaddress;
	NTSTATUS status;
	LARGE_INTEGER base;
	BOOL ret = FALSE;

	vaddress = 0;
	outlen = length;
	base.QuadPart = (ULONGLONG)(address);

	status = ZwMapViewOfSection(hPhysicalMemory,
		(HANDLE)-1,
		(PVOID*)&vaddress,
		0,
		length,
		&base,
		&outlen,
		ViewShare,
		0,
		PAGE_READONLY);

	if (status != STATUS_SUCCESS)
		return ret;

	memmove(buffer, vaddress, length);

	status = ZwUnmapViewOfSection((HANDLE)-1, (PVOID)vaddress);
	if (status == STATUS_SUCCESS)
		ret = TRUE;
	return ret;
}
#endif
