// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"

#include <libcpuid.h>
#include "../libcdi/libcdi.h"

PNWLIB_CONTEXT NWLC = NULL;

static const char* NWL_HS_BYTE[] =
{ "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB" };

noreturn VOID NWL_ErrExit(INT nExitCode, LPCSTR lpszText)
{
	if (!NWLC->ErrLogCallback)
		fprintf(stderr, "Error: %s\n", lpszText);
	else
		NWLC->ErrLogCallback(lpszText);
	exit(nExitCode);
}

VOID NW_Init(PNWLIB_CONTEXT pContext)
{
	NWLC = pContext;
	NWLC->NwFile = stdout;
	NWLC->AcpiTable = 0;
	NWLC->SmbiosType = 127;
	NWLC->DiskPath = NULL;
	NWL_NtGetVersion(&NWLC->NwOsInfo);
	GetNativeSystemInfo(&NWLC->NwSi);
	NWLC->NwRoot = NWL_NodeAlloc("NWinfo", 0);
	NWLC->NwDrv = wr0_driver_open();
	NWLC->NwRsdp = NWL_GetRsdp();
	NWLC->NwRsdt = NWL_GetRsdt();
	NWLC->NwXsdt = NWL_GetXsdt();
	NWLC->NwSmbios = NWL_GetSmbios();
	NWLC->NwSmart = cdi_create_smart();
	NWLC->NwSmartFlags = CDI_FLAG_DEFAULT;
	NWLC->NwCpuRaw = calloc(1, sizeof(struct cpu_raw_data_array_t));
	NWLC->NwCpuid = calloc(1, sizeof(struct system_id_t));
	if (!NWLC->NwCpuRaw || !NWLC->NwCpuid)
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Cannot allocate memory");
	NWLC->NwSmartInit = FALSE;
	NWLC->NwUnits = NWL_HS_BYTE;
	NWLC->ErrLog = NULL;
	if (NWL_IsAdmin() != TRUE)
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Administrator required");
	if (NWL_ObtainPrivileges(SE_SYSTEM_ENVIRONMENT_NAME) != ERROR_SUCCESS)
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "SeSystemEnvironmentPrivilege required");

	NWL_PdhInit();
}

VOID NW_Print(LPCSTR lpFileName)
{
	if (lpFileName && fopen_s(&NWLC->NwFile, lpFileName, "w"))
		NWL_ErrExit(ERROR_OPEN_FAILED, "Cannot open file");
	if (!NWLC->NwFile)
		return;
	if (NWLC->AcpiInfo)
		NW_Acpi();
	if (NWLC->CpuInfo)
		NW_Cpuid();
	if (NWLC->DiskInfo)
		NW_Disk();
	if (NWLC->EdidInfo)
		NW_Edid();
	if (NWLC->NetInfo)
		NW_Network();
	if (NWLC->PciInfo)
		NW_Pci();
	if (NWLC->DmiInfo)
		NW_Smbios();
	if (NWLC->SpdInfo)
		NW_Spd();
	if (NWLC->SysInfo)
		NW_System();
	if (NWLC->UsbInfo)
		NW_Usb();
	if (NWLC->BatteryInfo)
		NW_Battery();
	if (NWLC->UefiInfo)
		NW_Uefi();
	if (NWLC->ShareInfo)
		NW_NetShare();
	if (NWLC->AudioInfo)
		NW_Audio();
	if (NWLC->PublicIpInfo)
		NW_PublicIp();
	if (NWLC->ProductPolicyInfo)
		NW_ProductPolicy();
	if (NWLC->GpuInfo)
		NW_Gpu();
	if (NWLC->FontInfo)
		NW_Font();
	NW_Libinfo();
	NW_Export(NWLC->NwRoot, NWLC->NwFile);
}

VOID NW_Fini(VOID)
{
	NWL_PdhFini();
	if (NWLC->NwRsdp)
		free(NWLC->NwRsdp);
	if (NWLC->NwRsdt)
		free(NWLC->NwRsdt);
	if (NWLC->NwXsdt)
		free(NWLC->NwXsdt);
	if (NWLC->NwSmbios)
		free(NWLC->NwSmbios);
	if (NWLC->NwSmart)
		cdi_destroy_smart(NWLC->NwSmart);
	cpuid_free_system_id(NWLC->NwCpuid);
	free(NWLC->NwCpuid);
	cpuid_free_raw_data_array(NWLC->NwCpuRaw);
	free(NWLC->NwCpuRaw);
	if (NWLC->NwDrv)
		wr0_driver_close(NWLC->NwDrv);
	if (NWLC->NwRoot)
		NWL_NodeFree(NWLC->NwRoot, 1);
	if (NWLC->NwFile && NWLC->NwFile != stdout)
		fclose(NWLC->NwFile);
	ZeroMemory(NWLC, sizeof(NWLIB_CONTEXT));
	free(NWLC->ErrLog);
	NWLC = NULL;
}
