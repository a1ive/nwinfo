// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdio.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "format.h"

#define NWINFO_BUFSZ 65535

struct msr_driver_t;
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

	DWORD AcpiTable;
	BOOL ActiveNet;
	UINT8 SmbiosType;
	LPCSTR PciClass;

	struct acpi_rsdp_v2* NwRsdp;
	struct acpi_rsdt* NwRsdt;
	struct acpi_xsdt* NwXsdt;

	struct msr_driver_t* NwDrv;
	struct _NODE* NwRoot;
	enum
	{
		FORMAT_YAML = 0,
		FORMAT_JSON,
	} NwFormat;
	FILE* NwFile;
	UCHAR NwBuf[NWINFO_BUFSZ];
} NWLIB_CONTEXT, *PNWLIB_CONTEXT;

extern PNWLIB_CONTEXT NWLC;

BOOL NW_Init(PNWLIB_CONTEXT pContext);
VOID NW_Print(LPCSTR lpFileName);
VOID NW_Fini(VOID);

VOID NW_Beep(int argc, char* argv[]);

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

#ifdef __cplusplus
} /* extern "C" */
#endif
