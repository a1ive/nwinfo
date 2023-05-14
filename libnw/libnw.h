// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdio.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "format.h"

#define NWINFO_BUFSZ 65535

struct wr0_drv_t;
struct acpi_rsdp_v2;
struct acpi_rsdt;
struct acpi_xsdt;

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

	DWORD AcpiTable;
	BOOL ActiveNet;
	UINT8 SmbiosType;
	LPCSTR PciClass;

	VOID (*SpdProgress) (LPCSTR lpszText);

	struct acpi_rsdp_v2* NwRsdp;
	struct acpi_rsdt* NwRsdt;
	struct acpi_xsdt* NwXsdt;

	struct wr0_drv_t* NwDrv;
	struct _NODE* NwRoot;
	enum
	{
		FORMAT_YAML = 0,
		FORMAT_JSON,
		FORMAT_LUA,
	} NwFormat;
	FILE* NwFile;
	CHAR* ErrLog;
	VOID (*ErrLogCallback) (INT nExitCode, LPCSTR lpszText);
	UCHAR NwBuf[NWINFO_BUFSZ];
} NWLIB_CONTEXT, *PNWLIB_CONTEXT;

extern PNWLIB_CONTEXT NWLC;

VOID NW_Init(PNWLIB_CONTEXT pContext);
VOID NW_Print(LPCSTR lpFileName);
VOID NW_Fini(VOID);

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

PNODE NW_UpdateSystem(PNODE node);
PNODE NW_UpdateCpuid(PNODE node);
PNODE NW_UpdateNetwork(PNODE node);

#ifdef __cplusplus
} /* extern "C" */
#endif
