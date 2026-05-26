// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "libnw.h"
#include "utils.h"
#include "efivars.h"

GUID EFI_GV_GUID = { 0x8BE4DF61UL, 0x93CA, 0x11D2, { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C } };

GUID EFI_IMAGE_SECURITY_DATABASE_GUID =
{ 0xD719B2CBUL, 0x3D3A, 0x4596, { 0xA3, 0xBC, 0xDA, 0xD0, 0x0E, 0x67, 0x65, 0x6F } };

GUID EFI_CERT_SHA256_GUID =
{ 0xC1C41626UL, 0x504C, 0x4092, { 0xAC, 0xA9, 0x41, 0xF9, 0x36, 0x93, 0x43, 0x28 } };

GUID EFI_CERT_RSA2048_GUID =
{ 0x3C5766E8UL, 0x269C, 0x4E34, { 0xAA, 0x14, 0xED, 0x77, 0x6E, 0x85, 0xB3, 0xB6 } };

GUID EFI_CERT_RSA2048_SHA256_GUID =
{ 0xE2B36190UL, 0x879B, 0x4A3D, { 0xAD, 0x8D, 0xF2, 0xE7, 0xBB, 0xA3, 0x27, 0x84 } };

GUID EFI_CERT_SHA1_GUID =
{ 0x826CA512UL, 0xCF10, 0x4AC9, { 0xB1, 0x87, 0xBE, 0x01, 0x49, 0x66, 0x31, 0xBD } };

GUID EFI_CERT_SM3_GUID =
{ 0x57347F87UL, 0x7A9B, 0x403A, { 0xB9, 0x3C, 0xDC, 0x4A, 0xFB, 0x7A, 0x0E, 0xBC } };

GUID EFI_CERT_RSA2048_SHA1_GUID =
{ 0x67F8444FUL, 0x8743, 0x48F1, { 0xA3, 0x28, 0x1E, 0xAA, 0xB8, 0x73, 0x60, 0x80 } };

GUID EFI_CERT_X509_GUID =
{ 0xA5C059A1UL, 0x94E4, 0x4AA7, { 0x87, 0xB5, 0xAB, 0x15, 0x5C, 0x2B, 0xF0, 0x72 } };

GUID EFI_CERT_X509_SM3_GUID =
{ 0x60D807E5UL, 0x10B4, 0x49A9, { 0x93, 0x31, 0xE4, 0x04, 0x37, 0x88, 0x8D, 0x37 } };

GUID EFI_CERT_SHA224_GUID =
{ 0x0B6E5233UL, 0xA65C, 0x44C9, { 0x94, 0x07, 0xD9, 0xAB, 0x83, 0xBF, 0xC8, 0xBD } };

GUID EFI_CERT_SHA384_GUID =
{ 0xFF3E5307UL, 0x9FD0, 0x48C9, { 0x85, 0xF1, 0x8A, 0xD5, 0x6C, 0x70, 0x1E, 0x01 } };

GUID EFI_CERT_SHA512_GUID =
{ 0x093E0FAEUL, 0xA6C4, 0x4F50, { 0x9F, 0x1B, 0xD4, 0x1E, 0x2B, 0x89, 0xC1, 0x9A } };

GUID EFI_CERT_X509_SHA256_GUID =
{ 0x3BD2A492UL, 0x96C0, 0x4079, { 0xB4, 0x20, 0xFC, 0xF9, 0x8E, 0xF1, 0x03, 0xED } };

GUID EFI_CERT_X509_SHA384_GUID =
{ 0x7076876EUL, 0x80C2, 0x4EE6, { 0xAA, 0xD2, 0x28, 0xB3, 0x49, 0xA6, 0x86, 0x5B } };

GUID EFI_CERT_X509_SHA512_GUID =
{ 0x446DBF63UL, 0x2502, 0x4CDA, { 0xBC, 0xFA, 0x24, 0x65, 0xD2, 0xB0, 0xFE, 0x9D } };

GUID EFI_CERT_TYPE_PKCS7_GUID =
{ 0x4AAFD29DUL, 0x68DF, 0x49EE, { 0x8A, 0xA9, 0x34, 0x7D, 0x37, 0x56, 0x65, 0xA7 } };

BOOL
NWL_IsEfi(VOID)
{
	GUID EFI_EMPTY_GUID = { 0 };
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
					strncpy_s(cchSignature, sizeof(cchSignature), "0", _TRUNCATE);
					break;
				case SIGNATURE_TYPE_MBR:
					snprintf(cchSignature, sizeof(cchSignature), "%02X%02X%02X%02X",
						pDpPtr.HardDrive->Signature[0],
						pDpPtr.HardDrive->Signature[1],
						pDpPtr.HardDrive->Signature[2],
						pDpPtr.HardDrive->Signature[3]);
					break;
				case SIGNATURE_TYPE_GUID:
					strncpy_s(cchSignature, sizeof(cchSignature), NWL_GuidToStr(pDpPtr.HardDrive->Signature), _TRUNCATE);
					break;
				default:
					strncpy_s(cchSignature, sizeof(cchSignature), "Unknown", _TRUNCATE);
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
