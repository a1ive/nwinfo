// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdio.h>
#include <windows.h>
#include <stdnoreturn.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "format.h"

#define NWINFO_BUFSZ 65535

struct wr0_drv_t;
struct acpi_rsdp_v2;
struct acpi_rsdt;
struct acpi_xsdt;
struct _CDI_SMART;
struct cpu_raw_data_array_t;
struct system_id_t;

typedef struct _NWLIB_CONTEXT
{
	BOOL HumanSize;

	BOOL SysInfo;
	BOOL CpuInfo;
	BOOL AcpiInfo;
	BOOL NetInfo;
	BOOL DmiInfo;
	BOOL DiskInfo;
	BOOL EdidInfo;
	BOOL PciInfo;
	BOOL UsbInfo;
	BOOL SpdInfo;
	BOOL BatteryInfo;
	BOOL UefiInfo;
	BOOL ShareInfo;
	BOOL AudioInfo;

	BOOL Debug;
	BOOL HideSensitive;

	DWORD AcpiTable;
	BOOL ActiveNet;
	BOOL SkipVirtualNet;
	UINT8 SmbiosType;
	LPCSTR PciClass;
	BOOL DisableSmart;

	VOID (*SpdProgress) (LPCSTR lpszText);

	OSVERSIONINFOEXW NwOsInfo;

	struct cpu_raw_data_array_t* NwCpuRaw;
	struct system_id_t* NwCpuid;

	struct acpi_rsdp_v2* NwRsdp;
	struct acpi_rsdt* NwRsdt;
	struct acpi_xsdt* NwXsdt;

	struct RAW_SMBIOS_DATA* NwSmbios;
	BOOL NwSmartInit;

	struct _CDI_SMART* NwSmart;
	UINT64 NwSmartFlags;

	struct wr0_drv_t* NwDrv;
	UINT CodePage;
	struct _NODE* NwRoot;
	enum
	{
		FORMAT_YAML = 0,
		FORMAT_JSON,
		FORMAT_LUA,
	} NwFormat;
	FILE* NwFile;
	LPCSTR* NwUnits;
	CHAR* ErrLog;
	VOID (*ErrLogCallback) (LPCSTR lpszText);
	UCHAR NwBuf[NWINFO_BUFSZ];
} NWLIB_CONTEXT, *PNWLIB_CONTEXT;

extern PNWLIB_CONTEXT NWLC;

VOID NW_Init(PNWLIB_CONTEXT pContext);
VOID NW_Export(PNODE node, FILE* file);
VOID NW_Print(LPCSTR lpFileName);
VOID NW_Fini(VOID);

noreturn VOID NWL_ErrExit(INT nExitCode, LPCSTR lpszText);

PNODE NW_Acpi(VOID);
PNODE NW_Cpuid(VOID);
PNODE NW_Disk(VOID);
PNODE NW_Edid(VOID);
PNODE NW_Network(VOID);
PNODE NW_Pci(VOID);
PNODE NW_Smbios(VOID);
PNODE NW_Spd(VOID);
PNODE NW_System(VOID);
PNODE NW_Usb(VOID);
PNODE NW_Battery(VOID);
PNODE NW_Libinfo(VOID);
PNODE NW_Uefi(VOID);
PNODE NW_NetShare(VOID);
PNODE NW_Audio(VOID);

VOID NWL_GetUptime(CHAR* szUptime, DWORD dwSize);
VOID NWL_GetHostname(CHAR* szHostname);

typedef struct _NWLIB_MEM_INFO
{
	DWORD PhysUsage;
	DWORD PageUsage;
	DWORD SfciUsage;
	DWORDLONG PhysTotal;
	DWORDLONG PhysInUse;
	DWORDLONG PhysAvail;
	DWORDLONG PageTotal;
	DWORDLONG PageInUse;
	DWORDLONG PageAvail;
	DWORDLONG SfciTotal;
	DWORDLONG SfciInUse;
	DWORDLONG SfciAvail;
	SIZE_T SystemCache;
	SIZE_T PageSize;
} NWLIB_MEM_INFO, *PNWLIB_MEM_INFO;

VOID NWL_GetMemInfo(PNWLIB_MEM_INFO pMemInfo);

#define NWL_Debugf(...) \
	do \
	{ \
		if (NWLC->Debug) \
			printf(__VA_ARGS__); \
	} while (0)

#ifdef __cplusplus
} /* extern "C" */
#endif
