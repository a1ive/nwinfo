// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>

#include "libnw.h"
#include "utils.h"
#include "nt.h"
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
	return (ret == size);
}

static void PrintSecureBoot(PNODE node)
{
	UINT8 SecureBoot = 0;
	if (GetEfiGlobalVar(L"SecureBoot", &SecureBoot, sizeof(UINT8)))
		NWL_NodeAttrSetf(node, "Secure Boot", 0, "%s", SecureBoot ? "ENABLED" : "DISABLED");
	else
		NWL_NodeAttrSet(node, "Secure Boot", "UNSUPPORTED", 0);
}

static void PrintOsIndicationsSupported(PNODE node)
{
	char* features = NULL;
	UINT64 OsIndicationsSupported = 0;
	GetEfiGlobalVar(L"OsIndicationsSupported", &OsIndicationsSupported, sizeof(UINT64));
	NWL_NodeAttrSetf(node,  "Os Indications Supported", 0, "0x%016llX", OsIndicationsSupported);
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
	NWL_NodeAttrSetMulti(node, "Supported Features", features, 0);
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

static void PrintEfiVars(PNODE node)
{
	PNODE nv = NWL_NodeAppendNew(node, "Variables", NFLG_TABLE);
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
		DWORD VarSize = 0;
		LPCSTR Guid = NWL_WinGuidToStr(TRUE, &p->VendorGuid);
		PNODE ng = NWL_NodeGetChild(nv, Guid);
		if (!ng)
			ng = NWL_NodeAppendNew(nv, Guid, NFLG_TABLE_ROW);
		snprintf((CHAR*)NWLC->NwBuf, NWINFO_BUFSZ, "%S", p->Name);
		VarSize = NWL_GetEfiVar(p->Name, &p->VendorGuid, NULL, 0, NULL);
		NWL_NodeAttrSetf(ng, (CHAR*)NWLC->NwBuf, NAFLG_FMT_NUMERIC, "%lu", VarSize);
		if (p->NextEntryOffset == 0)
			break;
	}
	free(VarNamePtr);
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
	PrintBootInfo(node);
	PrintEfiVars(node);
	return node;
}
