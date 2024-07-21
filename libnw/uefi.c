// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>

#include "libnw.h"
#include "utils.h"
#include "efivars.h"

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

static void PrintEfiBootOption(PNODE node, LPGUID guid, WCHAR* name)
{
	PNODE nb = NULL;
	EFI_LOAD_OPTION* option = NULL;
	DWORD size = 0;
	if (memcmp(guid, &EFI_GV_GUID, sizeof(GUID)) != 0)
		return;
	if (name[0] != L'B' || name[1] != L'o' || name[2] != L'o' || name[3] != L't' ||
		!iswxdigit(name[4]) || !iswxdigit(name[5]) || !iswxdigit(name[6]) || !iswxdigit(name[7]) ||
		name[8] != L'\0')
		return;
	nb = NWL_NodeAppendNew(node, NWL_Ucs2ToUtf8(name), NFLG_ATTGROUP);
	option = NWL_GetEfiVarAlloc(name, guid, &size, NULL);
	if (!option)
		return;
	PrintLoadOption(nb, option, size);
	free(option);
}

static void PrintEfiVarAndSize(PNODE node, LPGUID guid, WCHAR* name)
{
	DWORD VarSize = 0;
	LPCSTR lpszGuid = NWL_WinGuidToStr(TRUE, guid);
	PNODE ng = NWL_NodeGetChild(node, lpszGuid);
	if (!ng)
		ng = NWL_NodeAppendNew(node, lpszGuid, NFLG_TABLE_ROW);
	VarSize = NWL_GetEfiVar(name, guid, NULL, 0, NULL);
	NWL_NodeAttrSetf(ng, NWL_Ucs2ToUtf8(name), NAFLG_FMT_NUMERIC, "%lu", VarSize);
}

static void EnumEfiVars(PNODE node, void (*func)(PNODE node, LPGUID guid, WCHAR* name))
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
		func(node, &p->VendorGuid, p->Name);
		if (p->NextEntryOffset == 0)
			break;
	}
	free(VarNamePtr);
}

static void PrintBootXXXX(PNODE node)
{
	PNODE nv = NWL_NodeAppendNew(node, "Boot Menu", 0);
	EnumEfiVars(nv, PrintEfiBootOption);
}

static void PrintEfiVars(PNODE node)
{
	PNODE nv = NWL_NodeAppendNew(node, "Variables", NFLG_TABLE);
	EnumEfiVars(nv, PrintEfiVarAndSize);
}

PNODE NW_Uefi(VOID)
{
	PNODE node = NWL_NodeAlloc("UEFI", 0);
	if (NWLC->UefiInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	PrintBootEnv(node);
	PrintSecureBoot(node);
	if (NWL_IsEfi() == FALSE)
		return node;
	PrintOsIndicationsSupported(node);
	PrintBootOptionSupport(node);
	PrintBootInfo(node);
	if (NWLC->UefiFlags & NW_UEFI_VARS)
		PrintEfiVars(node);
	if (NWLC->UefiFlags & NW_UEFI_MENU)
		PrintBootXXXX(node);
	return node;
}
