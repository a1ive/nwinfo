// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "libnw.h"
#include "utils.h"
#include "nt.h"
#include "efivars.h"

GUID EFI_GV_GUID = { 0x8BE4DF61UL, 0x93CA, 0x11D2, { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C } };
GUID EFI_EMPTY_GUID = { 0 };

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
	PVOID pBuffer, DWORD nSize, PDWORD pdwAttributes)
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
	rc = OsQuerySystemEnvironmentValueEx(&str, lpGuid, pBuffer, &nSize, pdwAttributes);
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
GuidToWcs(GUID* pGuid)
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
	BOOL(WINAPI * OsSetEfiVarExW)(LPCWSTR lpName, LPCWSTR lpGuid, PVOID pValue, DWORD nSize, DWORD dwAttributes) = NULL;
	HMODULE hModule = GetModuleHandleW(L"kernel32");
	if (!hModule)
		goto fail;
	*(FARPROC*)&OsSetEfiVarExW = GetProcAddress(hModule, "SetFirmwareEnvironmentVariableExW");
	if (!OsSetEfiVarExW)
		goto fail;
	if (!lpGuid)
		lpGuid = &EFI_GV_GUID;
	return OsSetEfiVarExW(lpName, GuidToWcs(lpGuid), pBuffer, nSize, dwAttributes);
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
	return OsSetEfiVarW(lpName, GuidToWcs(lpGuid), pBuffer, nSize);
fail:
	SetLastError(ERROR_INVALID_FUNCTION);
	return FALSE;
}

BOOL
NWL_DeleteEfiVar(LPCWSTR lpName, LPGUID lpGuid)
{
	return NWL_SetEfiVar(lpName, lpGuid, NULL, 0);
}

static CHAR*
AllocPrintf(LPCSTR _Printf_format_string_ format, ...)
{
	int sz;
	CHAR* buf = NULL;
	va_list ap;
	va_start(ap, format);
	sz = _vscprintf(format, ap) + 1;
	if (sz <= 0)
	{
		va_end(ap);
		NWL_ErrExit(ERROR_INVALID_DATA, "Failed to calculate string length in "__FUNCTION__);
	}
	buf = calloc(sizeof(CHAR), sz);
	if (!buf)
	{
		va_end(ap);
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);
	}
	vsnprintf(buf, sz, format, ap);
	va_end(ap);
	return buf;
}

CHAR*
NWL_GetEfiDpStr(EFI_DEVICE_PATH* pDp)
{
	CHAR* pStr = NULL;
	EFI_DEV_PATH_PTR pDpPtr;

	pDpPtr.DevPath = pDp;
	while(EFI_IS_DP_NODE_VALID(pDpPtr.DevPath))
	{
		CHAR* pNode = NULL;
		CHAR* pTmp = NULL;
		UINT8 uType = EFI_GET_DP_NODE_TYPE(pDpPtr.DevPath);
		UINT8 uSubType = EFI_GET_DP_NODE_SUBTYPE(pDpPtr.DevPath);
		UINT16 uLength = EFI_GET_DP_NODE_LENGTH(pDpPtr.DevPath);
		switch (uType)
		{
		case END_DEVICE_PATH_TYPE:
			switch (uSubType)
			{
			case END_ENTIRE_DEVICE_PATH_SUBTYPE:
				pNode = AllocPrintf("/EndEntire");
				break;
			case END_INSTANCE_DEVICE_PATH_SUBTYPE:
				pNode = AllocPrintf("/EndThis");
				break;
			default:
				pNode = AllocPrintf("/EndUnknown(%x)", uSubType);
				break;
			}
			break;
		case HARDWARE_DEVICE_PATH:
			pNode = AllocPrintf("/Hw(%x,%x)", uSubType, uLength);
			break;
		case ACPI_DEVICE_PATH:
			pNode = AllocPrintf("/Acpi(%x,%x)", uSubType, uLength);
			break;
		case MESSAGING_DEVICE_PATH:
			pNode = AllocPrintf("/Messaging(%x,%x)", uSubType, uLength);
			break;
		case MEDIA_DEVICE_PATH:
			switch (uSubType)
			{
			case MEDIA_HARDDRIVE_DP:
			{
				CHAR cchSignature[37];
				LPCSTR lpMBRType = "Unknown";
				switch (pDpPtr.HardDrive->SignatureType)
				{
				case NO_DISK_SIGNATURE:
					strcpy_s(cchSignature, sizeof(cchSignature), "0");
					break;
				case SIGNATURE_TYPE_MBR:
					snprintf(cchSignature, sizeof(cchSignature), "%02X%02X%02X%02X",
						pDpPtr.HardDrive->Signature[0],
						pDpPtr.HardDrive->Signature[1],
						pDpPtr.HardDrive->Signature[2],
						pDpPtr.HardDrive->Signature[3]);
					break;
				case SIGNATURE_TYPE_GUID:
					strcpy_s(cchSignature, sizeof(cchSignature), NWL_GuidToStr(pDpPtr.HardDrive->Signature));
					break;
				default:
					strcpy_s(cchSignature, sizeof(cchSignature), "Unknown");
					break;
				}
				switch (pDpPtr.HardDrive->MBRType)
				{
				case MBR_TYPE_PCAT: lpMBRType = "MBR"; break;
				case MBR_TYPE_EFI_PARTITION_TABLE_HEADER: lpMBRType = "GPT"; break;
				}
				pNode = AllocPrintf("/Hd(%u,%llx,%llx,%s,%s,%u)",
					pDpPtr.HardDrive->PartitionNumber,
					pDpPtr.HardDrive->PartitionStart,
					pDpPtr.HardDrive->PartitionSize,
					cchSignature,
					lpMBRType,
					pDpPtr.HardDrive->SignatureType);
			}
				break;
			case MEDIA_CDROM_DP:
				pNode = AllocPrintf("/Cd(%u,%llx,%llx)",
					pDpPtr.CD->BootEntry,
					pDpPtr.CD->PartitionStart,
					pDpPtr.CD->PartitionSize);
				break;
			case MEDIA_FILEPATH_DP:
				pNode = AllocPrintf("/FilePath(%s)",
					NWL_Ucs2ToUtf8(pDpPtr.FilePath->PathName));
				break;
			default:
				pNode = AllocPrintf("/Media(%x,%x)", uSubType, uLength);
				break;
			}
			break;
		case BBS_DEVICE_PATH:
			pNode = AllocPrintf("/Bbs(%x,%x)", uSubType, uLength);
			break;
		default:
			pNode = AllocPrintf("/Unknown(%x,%x,%x)", uType, uSubType, uLength);
			break;
		}
		pTmp = pStr;
		pStr = AllocPrintf("%s%s", pTmp ? pTmp : "", pNode);
		if (pTmp)
			free(pTmp);
		free(pNode);
		if (EFI_IS_END_ENTIRE_DP(pDpPtr.DevPath))
			break;
		pDpPtr.DevPath = EFI_GET_NEXT_DP_NODE(pDpPtr.DevPath);
	}
	return pStr;
}
