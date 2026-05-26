// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winbase.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "libnw.h"
#include "utils.h"
#include "efivars.h"

typedef DWORD(WINAPI* PFN_CERT_GET_NAME_STRING_W)(PCCERT_CONTEXT, DWORD, DWORD, void*, LPWSTR, DWORD);
typedef DWORD(WINAPI* PFN_CERT_NAME_TO_STR_W)(DWORD, PCERT_NAME_BLOB, DWORD, LPWSTR, DWORD);
typedef PCCERT_CONTEXT(WINAPI* PFN_CERT_CREATE_CERTIFICATE_CONTEXT)(DWORD, const BYTE*, DWORD);
typedef BOOL(WINAPI* PFN_CERT_FREE_CERTIFICATE_CONTEXT)(PCCERT_CONTEXT);

static void PrintBootEnv(PNODE node)
{
	LPCSTR FirmwareType = "UNKNOWN";
	SYSTEM_BOOT_ENVIRONMENT_INFORMATION BootInfo = { 0 };
	if (!NWL_NtQuerySystemInformation(SystemBootEnvironmentInformation, &BootInfo, sizeof(BootInfo), NULL))
		return;
	NWL_NodeAttrSet(node, "Boot Identifier", NWL_WinGuidToStr(TRUE, &BootInfo.BootIdentifier), NAFLG_FMT_GUID);
	switch (BootInfo.FirmwareType)
	{
	case FirmwareTypeBios: FirmwareType = "BIOS"; break;
	case FirmwareTypeUefi: FirmwareType = "UEFI"; break;
	}
	NWL_NodeAttrSet(node, "Firmware Type", FirmwareType, 0);
	NWL_NodeAttrSetf(node, "Boot Flags", 0, "0x%016llX", BootInfo.BootFlags);
}

static BOOL GetEfiGlobalVar(LPCWSTR name, LPVOID var, DWORD size)
{
	DWORD ret;
	ZeroMemory(var, size);
	ret = NWL_GetEfiVar(name, NULL, var, size, NULL);
	return (GetLastError() == ERROR_SUCCESS) && (ret == size);
}

static void PrintSecureBoot(PNODE node)
{
	UINT8 SecureBoot = 0;
	if (GetEfiGlobalVar(L"SecureBoot", &SecureBoot, sizeof(UINT8)))
		NWL_NodeAttrSet(node, "Secure Boot", SecureBoot ? "ENABLED" : "DISABLED", 0);
	else
		NWL_NodeAttrSet(node, "Secure Boot", "UNSUPPORTED", 0);
}

static CHAR* GetHexString(const UINT8* data, DWORD size)
{
	static const CHAR hex[] = "0123456789ABCDEF";

	if (size > (NWINFO_BUFSZ - 1) / 2)
		size = (NWINFO_BUFSZ - 1) / 2;
	for (DWORD i = 0; i < size; i++)
	{
		NWLC->NwBuf[i * 2] = hex[data[i] >> 4];
		NWLC->NwBuf[i * 2 + 1] = hex[data[i] & 0x0F];
	}
	NWLC->NwBuf[size * 2] = '\0';
	return NWLC->NwBuf;
}

typedef enum _SECURE_BOOT_SIGNATURE_FORMAT
{
	SECURE_BOOT_SIGNATURE_UNKNOWN,
	SECURE_BOOT_SIGNATURE_DATA,
	SECURE_BOOT_SIGNATURE_IMAGE_HASH,
	SECURE_BOOT_SIGNATURE_X509_CERTIFICATE,
	SECURE_BOOT_SIGNATURE_X509_HASH,
} SECURE_BOOT_SIGNATURE_FORMAT;

typedef struct _SECURE_BOOT_SIGNATURE_INFO
{
	const GUID* Type;
	LPCSTR Name;
	LPCSTR HashAlgorithm;
	DWORD HashSize;
	DWORD HashOffset;
	DWORD TimeOffset;
	DWORD DataSize;
	SECURE_BOOT_SIGNATURE_FORMAT Format;
} SECURE_BOOT_SIGNATURE_INFO;

#define SB_SIG_DATA(guid, name) \
	{ &(guid), (name), NULL, 0, 0, 0, 0, SECURE_BOOT_SIGNATURE_DATA }
#define SB_IMAGE_HASH(guid, name, algorithm, hash_type) \
	{ &(guid), (name), (algorithm), (DWORD)sizeof(hash_type), 0, 0, \
	  (DWORD)sizeof(hash_type), SECURE_BOOT_SIGNATURE_IMAGE_HASH }
#define SB_X509_CERTIFICATE(guid, name) \
	{ &(guid), (name), NULL, 0, 0, 0, 0, SECURE_BOOT_SIGNATURE_X509_CERTIFICATE }
#define SB_X509_HASH(guid, name, algorithm, cert_type) \
	{ &(guid), (name), (algorithm), (DWORD)sizeof(((cert_type*)0)->ToBeSignedHash), \
	  (DWORD)offsetof(cert_type, ToBeSignedHash), (DWORD)offsetof(cert_type, TimeOfRevocation), \
	  (DWORD)sizeof(cert_type), SECURE_BOOT_SIGNATURE_X509_HASH }

static const SECURE_BOOT_SIGNATURE_INFO SecureBootSignatureInfo[] =
{
	SB_IMAGE_HASH(EFI_CERT_SHA256_GUID, "SHA-256 Image Hash", "SHA-256", EFI_SHA256_HASH),
	SB_SIG_DATA(EFI_CERT_RSA2048_GUID, "RSA-2048"),
	SB_SIG_DATA(EFI_CERT_RSA2048_SHA256_GUID, "RSA-2048 SHA-256"),
	SB_IMAGE_HASH(EFI_CERT_SHA1_GUID, "SHA-1 Image Hash", "SHA-1", EFI_SHA1_HASH),
	SB_IMAGE_HASH(EFI_CERT_SM3_GUID, "SM3 Image Hash", "SM3", EFI_SM3_HASH),
	SB_SIG_DATA(EFI_CERT_RSA2048_SHA1_GUID, "RSA-2048 SHA-1"),
	SB_X509_CERTIFICATE(EFI_CERT_X509_GUID, "X.509 Certificate"),
	SB_X509_HASH(EFI_CERT_X509_SM3_GUID, "X.509 SM3 Certificate Hash", "SM3", EFI_CERT_X509_SM3),
	SB_IMAGE_HASH(EFI_CERT_SHA224_GUID, "SHA-224 Image Hash", "SHA-224", EFI_SHA224_HASH),
	SB_IMAGE_HASH(EFI_CERT_SHA384_GUID, "SHA-384 Image Hash", "SHA-384", EFI_SHA384_HASH),
	SB_IMAGE_HASH(EFI_CERT_SHA512_GUID, "SHA-512 Image Hash", "SHA-512", EFI_SHA512_HASH),
	SB_X509_HASH(EFI_CERT_X509_SHA256_GUID, "X.509 SHA-256 Certificate Hash", "SHA-256", EFI_CERT_X509_SHA256),
	SB_X509_HASH(EFI_CERT_X509_SHA384_GUID, "X.509 SHA-384 Certificate Hash", "SHA-384", EFI_CERT_X509_SHA384),
	SB_X509_HASH(EFI_CERT_X509_SHA512_GUID, "X.509 SHA-512 Certificate Hash", "SHA-512", EFI_CERT_X509_SHA512),
	SB_SIG_DATA(EFI_CERT_TYPE_PKCS7_GUID, "PKCS#7"),
};

static const SECURE_BOOT_SIGNATURE_INFO SecureBootUnknownSignatureInfo =
	{ NULL, "UNKNOWN", NULL, 0, 0, 0, 0, SECURE_BOOT_SIGNATURE_UNKNOWN };

static const SECURE_BOOT_SIGNATURE_INFO* SecureBootGetSignatureInfo(const GUID* type)
{
	for (size_t i = 0; i < ARRAYSIZE(SecureBootSignatureInfo); i++)
	{
		if (memcmp(type, SecureBootSignatureInfo[i].Type, sizeof(GUID)) == 0)
			return &SecureBootSignatureInfo[i];
	}
	return &SecureBootUnknownSignatureInfo;
}

static WCHAR* AllocCertDisplayName(HMODULE crypt32, PCCERT_CONTEXT cert)
{
	WCHAR* str;
	PFN_CERT_GET_NAME_STRING_W pfnCertGetNameStringW = NULL;

	*(FARPROC*)&pfnCertGetNameStringW = GetProcAddress(crypt32, "CertGetNameStringW");
	if (!pfnCertGetNameStringW)
		return NULL;
	DWORD size = pfnCertGetNameStringW(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0);
	if (size <= 1)
		return NULL;
	str = calloc(size, sizeof(WCHAR));
	if (!str)
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);
	if (!pfnCertGetNameStringW(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, str, size))
	{
		free(str);
		return NULL;
	}
	return str;
}

static WCHAR* AllocCertNameString(HMODULE crypt32, PCERT_NAME_BLOB name)
{
	WCHAR* str;
	PFN_CERT_NAME_TO_STR_W pfnCertNameToStrW = NULL;

	*(FARPROC*)&pfnCertNameToStrW = GetProcAddress(crypt32, "CertNameToStrW");
	if (!pfnCertNameToStrW)
		return NULL;
	DWORD size = pfnCertNameToStrW(X509_ASN_ENCODING, name,
		CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG, NULL, 0);
	if (size <= 1)
		return NULL;
	str = calloc(size, sizeof(WCHAR));
	if (!str)
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);
	if (!pfnCertNameToStrW(X509_ASN_ENCODING, name,
		CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG, str, size))
	{
		free(str);
		return NULL;
	}
	return str;
}

static void PrintX509Certificate(PNODE node, const UINT8* data, DWORD size)
{
	HMODULE crypt32;
	PCCERT_CONTEXT cert;
	PFN_CERT_CREATE_CERTIFICATE_CONTEXT pfnCertCreateCertificateContext = NULL;
	PFN_CERT_FREE_CERTIFICATE_CONTEXT pfnCertFreeCertificateContext = NULL;

	crypt32 = LoadLibraryW(L"crypt32.dll");
	if (!crypt32)
		return;
	*(FARPROC*)&pfnCertCreateCertificateContext = GetProcAddress(crypt32, "CertCreateCertificateContext");
	*(FARPROC*)&pfnCertFreeCertificateContext = GetProcAddress(crypt32, "CertFreeCertificateContext");
	if (!pfnCertCreateCertificateContext || !pfnCertFreeCertificateContext)
	{
		FreeLibrary(crypt32);
		return;
	}
	cert = pfnCertCreateCertificateContext(X509_ASN_ENCODING, data, size);
	if (!cert)
	{
		FreeLibrary(crypt32);
		return;
	}

	NWL_NodeAttrSetf(node, "Certificate Size", NAFLG_FMT_NUMERIC, "%lu", size);

	WCHAR* name = AllocCertDisplayName(crypt32, cert);
	if (name)
	{
		NWL_NodeAttrSet(node, "Name", NWL_Ucs2ToUtf8(name), 0);
		free(name);
	}
	WCHAR* subject = AllocCertNameString(crypt32, &cert->pCertInfo->Subject);
	if (subject)
	{
		NWL_NodeAttrSet(node, "Subject", NWL_Ucs2ToUtf8(subject), 0);
		free(subject);
	}
	WCHAR* issuer = AllocCertNameString(crypt32, &cert->pCertInfo->Issuer);
	if (issuer)
	{
		NWL_NodeAttrSet(node, "Issuer", NWL_Ucs2ToUtf8(issuer), 0);
		free(issuer);
	}
	pfnCertFreeCertificateContext(cert);
	FreeLibrary(crypt32);
}

static DWORD PrintSecureBootDatabase(PNODE root, LPCWSTR name, LPGUID guid)
{
	DWORD attributes = 0;
	DWORD size = 0;
	DWORD offset = 0;
	DWORD list_index = 0;
	DWORD signature_count = 0;
	PBYTE data;
	PNODE var_node;
	PNODE sig_node;

	data = NWL_GetEfiVarAlloc(name, guid, &size, &attributes);
	if (!data)
		return 0;

	var_node = NWL_NodeAppendNew(root, NWL_Ucs2ToUtf8(name), 0);
	NWL_NodeAttrSet(var_node, "Namespace", NWL_WinGuidToStr(TRUE, guid), NAFLG_FMT_GUID);
	NWL_NodeAttrSetf(var_node, "Attributes", 0, "0x%08lX", attributes);
	NWL_NodeAttrSetf(var_node, "Size", NAFLG_FMT_NUMERIC, "%lu", size);
	sig_node = NWL_NodeAppendNew(var_node, "Signatures", NFLG_TABLE);

	while (offset < size)
	{
		EFI_SIGNATURE_LIST* list;
		DWORD list_header_size = (DWORD)sizeof(EFI_SIGNATURE_LIST);
		DWORD remaining = size - offset;
		DWORD list_size;
		DWORD signature_size;
		DWORD entry_offset;
		DWORD list_end;
		DWORD entry_index = 0;

		if (remaining < list_header_size)
		{
			NWL_Debug("UEFI", "Truncated signature list in %s", NWL_Ucs2ToUtf8(name));
			break;
		}

		list = (EFI_SIGNATURE_LIST*)(data + offset);
		list_size = list->SignatureListSize;
		signature_size = list->SignatureSize;
		if (list_size < list_header_size || list_size > remaining ||
			list->SignatureHeaderSize > list_size - list_header_size ||
			signature_size <= (DWORD)sizeof(GUID))
		{
			NWL_Debug("UEFI", "Invalid signature list in %s", NWL_Ucs2ToUtf8(name));
			break;
		}

		entry_offset = offset + list_header_size + list->SignatureHeaderSize;
		list_end = offset + list_size;
		while (signature_size <= list_end - entry_offset)
		{
			EFI_SIGNATURE_DATA* signature = (EFI_SIGNATURE_DATA*)(data + entry_offset);
			DWORD signature_data_size = signature_size - (DWORD)sizeof(GUID);
			PNODE row = NWL_NodeAppendNew(sig_node, "Signature", NFLG_TABLE_ROW);
			const SECURE_BOOT_SIGNATURE_INFO* info = SecureBootGetSignatureInfo(&list->SignatureType);

			NWL_NodeAttrSetf(row, "List", NAFLG_FMT_NUMERIC, "%lu", list_index);
			NWL_NodeAttrSetf(row, "Index", NAFLG_FMT_NUMERIC, "%lu", entry_index);
			NWL_NodeAttrSet(row, "Type", info->Name, 0);
			NWL_NodeAttrSet(row, "Type GUID", NWL_WinGuidToStr(TRUE, &list->SignatureType), NAFLG_FMT_GUID);
			NWL_NodeAttrSet(row, "Owner", NWL_WinGuidToStr(TRUE, &signature->SignatureOwner), NAFLG_FMT_GUID);
			if (info->HashAlgorithm)
				NWL_NodeAttrSet(row, "Hash Algorithm", info->HashAlgorithm, 0);
			switch (info->Format)
			{
			case SECURE_BOOT_SIGNATURE_IMAGE_HASH:
				if (signature_data_size == info->DataSize)
					NWL_NodeAttrSet(row, "Image Hash", GetHexString(signature->SignatureData, signature_data_size), 0);
				break;
			case SECURE_BOOT_SIGNATURE_X509_CERTIFICATE:
				PrintX509Certificate(row, signature->SignatureData, signature_data_size);
				break;
			case SECURE_BOOT_SIGNATURE_X509_HASH:
				if (signature_data_size == info->DataSize)
				{
					const EFI_TIME* time = (const EFI_TIME*)(signature->SignatureData + info->TimeOffset);
					NWL_NodeAttrSet(row, "Hash Algorithm", info->HashAlgorithm, 0);
					NWL_NodeAttrSet(row, "Certificate Hash", GetHexString(signature->SignatureData + info->HashOffset, info->HashSize), 0);
					if (time->Year != 0 && time->Month != 0 && time->Day != 0)
						NWL_NodeAttrSetf(row, "Revocation Time", 0, "%04u-%02u-%02u %02u:%02u:%02u",
							time->Year, time->Month, time->Day, time->Hour, time->Minute, time->Second);
				}
				break;
			case SECURE_BOOT_SIGNATURE_DATA:
			default:
				break;
			}
			NWL_NodeAttrSetf(row, "Data Size", NAFLG_FMT_NUMERIC, "%lu", signature_data_size);

			signature_count++;
			entry_index++;
			entry_offset += signature_size;
		}
		if (entry_offset != list_end)
		{
			NWL_Debug("UEFI", "Truncated signature entry in %s", NWL_Ucs2ToUtf8(name));
			break;
		}
		offset += list_size;
		list_index++;
	}

	NWL_NodeAttrSetf(var_node, "List Count", NAFLG_FMT_NUMERIC, "%lu", list_index);
	NWL_NodeAttrSetf(var_node, "Signature Count", NAFLG_FMT_NUMERIC, "%lu", signature_count);
	free(data);
	return signature_count;
}

static void PrintSecureBootSignatures(PNODE node)
{
	PNODE root = NWL_NodeAppendNew(node, "Secure Boot Signatures", NFLG_ATTGROUP);
	PrintSecureBootDatabase(root, L"PK", &EFI_GV_GUID);
	PrintSecureBootDatabase(root, L"KEK", &EFI_GV_GUID);
	PrintSecureBootDatabase(root, L"db", &EFI_IMAGE_SECURITY_DATABASE_GUID);
	PrintSecureBootDatabase(root, L"dbx", &EFI_IMAGE_SECURITY_DATABASE_GUID);
	PrintSecureBootDatabase(root, L"dbt", &EFI_IMAGE_SECURITY_DATABASE_GUID);
}

static void PrintOsIndicationsSupported(PNODE node)
{
	char* features = NULL;
	UINT64 OsIndicationsSupported = 0;
	GetEfiGlobalVar(L"OsIndicationsSupported", &OsIndicationsSupported, sizeof(UINT64));
	NWL_NodeAttrSetf(node,  "OsIndicationsSupported", 0, "0x%016llX", OsIndicationsSupported);
	if (OsIndicationsSupported & EFI_OS_INDICATIONS_BOOT_TO_FW_UI)
		NWL_NodeAppendMultiSz(&features, "BootToFwUI");
	if (OsIndicationsSupported & EFI_OS_INDICATIONS_TIMESTAMP_REVOCATION)
		NWL_NodeAppendMultiSz(&features, "TimestampRevocation");
	if (OsIndicationsSupported & EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED)
		NWL_NodeAppendMultiSz(&features, "FileCapsuleDelivery");
	if (OsIndicationsSupported & EFI_OS_INDICATIONS_FMP_CAPSULE_SUPPORTED)
		NWL_NodeAppendMultiSz(&features, "FmpCapsule");
	if (OsIndicationsSupported & EFI_OS_INDICATIONS_CAPSULE_RESULT_VAR_SUPPORTED)
		NWL_NodeAppendMultiSz(&features, "CapsuleResultVar");
	if (OsIndicationsSupported & EFI_OS_INDICATIONS_START_OS_RECOVERY)
		NWL_NodeAppendMultiSz(&features, "StartOsRecovery");
	if (OsIndicationsSupported & EFI_OS_INDICATIONS_START_PLATFORM_RECOVERY)
		NWL_NodeAppendMultiSz(&features, "StartPlatformRecovery");
	if (OsIndicationsSupported & EFI_OS_INDICATIONS_JSON_CONFIG_DATA_REFRESH)
		NWL_NodeAppendMultiSz(&features, "JsonConfigDataRefresh");
	NWL_NodeAttrSetMulti(node, "Supported OS Indication Features", features, 0);
	free(features);
}

static void PrintBootOptionSupport(PNODE node)
{
	char* features = NULL;
	UINT32 BootOptionSupport = 0;
	GetEfiGlobalVar(L"BootOptionSupport", &BootOptionSupport, sizeof(UINT32));
	NWL_NodeAttrSetf(node, "BootOptionSupport", 0, "0x%08X", BootOptionSupport);
	if (BootOptionSupport & EFI_BOOT_OPTION_SUPPORT_KEY)
	{
		char KeyCountStr[] = "KeyCount4";
		UINT32 KeyCount = (BootOptionSupport & EFI_BOOT_OPTION_SUPPORT_COUNT) >> 8;
		KeyCountStr[8] = '0' + KeyCount;
		NWL_NodeAppendMultiSz(&features, "Key");
		NWL_NodeAppendMultiSz(&features, KeyCountStr);
	}
	if (BootOptionSupport & EFI_BOOT_OPTION_SUPPORT_APP)
		NWL_NodeAppendMultiSz(&features, "App");
	if (BootOptionSupport & EFI_BOOT_OPTION_SUPPORT_SYSPREP)
		NWL_NodeAppendMultiSz(&features, "SysPrep");
	NWL_NodeAttrSetMulti(node, "Supported Boot Options", features, 0);
	free(features);
}

static void PrintBootInfo(PNODE node)
{
	UINT16 Var = 0;
	UINT16* BootOrder = NULL;
	DWORD BootOrderSize = 0;
	DWORD i;
	char tmp[] = "XXXX";
	char* BootOrderStr = NULL;
	GetEfiGlobalVar(L"Timeout", &Var, sizeof(UINT16));
	NWL_NodeAttrSetf(node, "Timeout", NAFLG_FMT_NUMERIC, "%u", Var);
	GetEfiGlobalVar(L"BootCurrent", &Var, sizeof(UINT16));
	NWL_NodeAttrSetf(node, "Boot Current", 0, "%04X", Var);
	GetEfiGlobalVar(L"BootNext", &Var, sizeof(UINT16));
	NWL_NodeAttrSetf(node, "Boot Next", 0, "%04X", Var);
	BootOrder = NWL_GetEfiVarAlloc(L"BootOrder", NULL, &BootOrderSize, NULL);
	if (!BootOrder)
		goto out;
	for (i = 0; i < BootOrderSize / sizeof(UINT16); i++)
	{
		snprintf(tmp, sizeof(tmp), "%04X", BootOrder[i]);
		NWL_NodeAppendMultiSz(&BootOrderStr, tmp);
	}
out:
	NWL_NodeAttrSetMulti(node, "Boot Order", BootOrderStr, 0);
	if (BootOrder)
		free(BootOrder);
	if (BootOrderStr)
		free(BootOrderStr);
}

static void PrintKeyOption(PNODE node, EFI_KEY_OPTION* option, DWORD size)
{
	char* mod_key = NULL;
	if (size < sizeof(EFI_KEY_OPTION))
		return;
	if (size < sizeof(EFI_KEY_OPTION) + option->KeyData.InputKeyCount * sizeof(EFI_INPUT_KEY))
		return;
	NWL_NodeAttrSetf(node, "Revision", NAFLG_FMT_NUMERIC, "%u", (unsigned)option->KeyData.Revision);
	if (option->KeyData.ControlPressed)
		NWL_NodeAppendMultiSz(&mod_key, "Ctrl");
	if (option->KeyData.AltPressed)
		NWL_NodeAppendMultiSz(&mod_key, "Alt");
	if (option->KeyData.LogoPressed)
		NWL_NodeAppendMultiSz(&mod_key, "Logo");
	if (option->KeyData.MenuPressed)
		NWL_NodeAppendMultiSz(&mod_key, "Menu");
	if (option->KeyData.SysReqPressed)
		NWL_NodeAppendMultiSz(&mod_key, "SysReq");
	NWL_NodeAttrSetMulti(node, "Modifier Keys", mod_key, 0);
	free(mod_key);
	NWL_NodeAttrSetf(node, "Key Count", NAFLG_FMT_NUMERIC, "%u", (unsigned)option->KeyData.InputKeyCount);
	NWL_NodeAttrSetf(node, "Boot Option", 0, "%04X", option->BootOption);
	for (DWORD i = 0; i < option->KeyData.InputKeyCount; i++)
	{
		char key_name[16];
		snprintf(key_name, sizeof(key_name), "Key %u", i);
		PNODE nk = NWL_NodeAppendNew(node, key_name, NFLG_ATTGROUP);
		NWL_NodeAttrSetf(nk, "Scan Code", 0, "0x%04X", option->Keys[i].ScanCode);
		NWL_NodeAttrSetf(nk, "Unicode Char", 0, "%lc", option->Keys[i].UnicodeChar);
	}
}

static void PrintLoadOption(PNODE node, EFI_LOAD_OPTION* option, DWORD size)
{
	EFI_DEVICE_PATH* dp = NULL;
	char* attr = NULL;
	char* dp_str = NULL;
	size_t desc_size;
	if (size < sizeof(EFI_LOAD_OPTION))
		return;
	desc_size = wcslen(option->Description) * sizeof(CHAR16);
	if (size < sizeof(EFI_LOAD_OPTION) + desc_size + option->FilePathListLength)
		return;
	if (option->Attributes & EFI_LOAD_OPTION_ACTIVE)
		NWL_NodeAppendMultiSz(&attr, "Active");
	if (option->Attributes & EFI_LOAD_OPTION_FORCE_RECONNECT)
		NWL_NodeAppendMultiSz(&attr, "ForceReconnect");
	if (option->Attributes & EFI_LOAD_OPTION_HIDDEN)
		NWL_NodeAppendMultiSz(&attr, "Hidden");
	if (option->Attributes & EFI_LOAD_OPTION_CATEGORY_APP)
		NWL_NodeAppendMultiSz(&attr, "App");
	if (option->Attributes & EFI_LOAD_OPTION_CATEGORY_BOOT)
		NWL_NodeAppendMultiSz(&attr, "Boot");
	NWL_NodeAttrSetMulti(node, "Attributes", attr, 0);
	free(attr);
	NWL_NodeAttrSet(node, "Name", NWL_Ucs2ToUtf8(option->Description), 0);
	dp = (EFI_DEVICE_PATH*)((LPBYTE)option + sizeof(EFI_LOAD_OPTION) + desc_size);
	dp_str = NWL_GetEfiDpStr(dp);
	NWL_NodeAttrSet(node, "Device Path", dp_str, NAFLG_FMT_NEED_QUOTE);
	free(dp_str);
}

typedef struct _BOOT_MENU_CTX
{
	PNODE nb;
	PNODE nd;
	PNODE nk;
} BOOT_MENU_CTX;

static inline BOOL
IsBootEntry(LPCWSTR prefix, LPCWSTR name)
{
	const size_t prefix_len = wcslen(prefix);
	const size_t name_len = wcslen(name);
	if (name_len != prefix_len + 4)
		return FALSE;
	if (_wcsnicmp(prefix, name, prefix_len) != 0)
		return FALSE;
	for (int i = 0; i < 4; ++i)
	{
		if (!iswxdigit(name[prefix_len + i]))
			return FALSE;
	}
	return TRUE;
}

static void PrintEfiBootMenu(void* ctx, LPGUID guid, WCHAR* name)
{
	BOOT_MENU_CTX* nodes = (BOOT_MENU_CTX*)ctx;
	DWORD size = 0;
	PNODE cur = NULL;
	if (memcmp(guid, &EFI_GV_GUID, sizeof(GUID)) != 0)
		return;
	if (IsBootEntry(L"Boot", name))
	{
		EFI_LOAD_OPTION* option = NWL_GetEfiVarAlloc(name, guid, &size, NULL);
		if (!option)
			return;
		cur = NWL_NodeAppendNew(nodes->nb, NWL_Ucs2ToUtf8(name), NFLG_ATTGROUP);
		PrintLoadOption(cur, option, size);
		free(option);
	}
	else if (IsBootEntry(L"Driver", name))
	{
		EFI_LOAD_OPTION* option = NWL_GetEfiVarAlloc(name, guid, &size, NULL);
		if (!option)
			return;
		cur = NWL_NodeAppendNew(nodes->nd, NWL_Ucs2ToUtf8(name), NFLG_ATTGROUP);
		PrintLoadOption(cur, option, size);
		free(option);
	}
	else if (IsBootEntry(L"Key", name))
	{
		EFI_KEY_OPTION* option = NWL_GetEfiVarAlloc(name, guid, &size, NULL);
		if (!option)
			return;
		cur = NWL_NodeAppendNew(nodes->nk, NWL_Ucs2ToUtf8(name), NFLG_ATTGROUP);
		PrintKeyOption(cur, option, size);
		free(option);
	}
}

static void PrintEfiVarAndSize(void* ctx, LPGUID guid, WCHAR* name)
{
	PNODE node = (PNODE)ctx;
	DWORD VarSize = 0;
	LPCSTR lpszGuid = NWL_WinGuidToStr(TRUE, guid);
	PNODE ng = NWL_NodeGetChild(node, lpszGuid);
	if (!ng)
		ng = NWL_NodeAppendNew(node, lpszGuid, NFLG_TABLE_ROW);
	VarSize = NWL_GetEfiVar(name, guid, NULL, 0, NULL);
	NWL_NodeAttrSetf(ng, NWL_Ucs2ToUtf8(name), NAFLG_FMT_NUMERIC, "%lu", VarSize);
}

static void EnumEfiVars(void* ctx, void (*func)(void* ctx, LPGUID guid, WCHAR* name))
{
	PVARIABLE_NAME VarNamePtr = NULL;
	PVARIABLE_NAME p;
	ULONG VarNameSize = 0;
	VarNamePtr = NWL_EnumerateEfiVar(&VarNameSize);
	if (!VarNamePtr || VarNameSize == 0)
		return;
	for (p = VarNamePtr;
		((LPBYTE)p < (LPBYTE)VarNamePtr + VarNameSize);
		p = (PVARIABLE_NAME)((LPBYTE)p + p->NextEntryOffset))
	{
		func(ctx, &p->VendorGuid, p->Name);
		if (p->NextEntryOffset == 0)
			break;
	}
	free(VarNamePtr);
}

static void PrintXXXX(PNODE node)
{
	BOOT_MENU_CTX ctx = { 0 };
	ctx.nb = NWL_NodeAppendNew(node, "Boot Menu", 0);
	ctx.nd = NWL_NodeAppendNew(node, "Driver Menu", 0);
	ctx.nk = NWL_NodeAppendNew(node, "Hot Keys", 0);
	EnumEfiVars(&ctx, PrintEfiBootMenu);
}

static void PrintEfiVars(PNODE node)
{
	PNODE nv = NWL_NodeAppendNew(node, "Variables", NFLG_TABLE);
	EnumEfiVars(nv, PrintEfiVarAndSize);
}

PNODE NW_Uefi(BOOL bAppend)
{
	PNODE node = NWL_NodeAlloc("UEFI", 0);
	if (bAppend)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	PrintBootEnv(node);
	PrintSecureBoot(node);
	if (NWLC->NwIsEfi == FALSE)
		return node;
	PrintOsIndicationsSupported(node);
	PrintBootOptionSupport(node);
	PrintBootInfo(node);
	if (NWLC->UefiFlags & NW_UEFI_CERT)
		PrintSecureBootSignatures(node);
	if (NWLC->UefiFlags & NW_UEFI_VARS)
		PrintEfiVars(node);
	if (NWLC->UefiFlags & NW_UEFI_MENU)
		PrintXXXX(node);
	return node;
}
