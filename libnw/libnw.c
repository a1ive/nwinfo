// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"

#include <libcpuid.h>

PNWLIB_CONTEXT NWLC = NULL;

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
	NWLC->NwRoot = NWL_NodeAlloc("NWinfo", 0);
	NWLC->NwDrv = wr0_driver_open();
	NWLC->NwRsdp = NWL_GetRsdp();
	NWLC->NwRsdt = NWL_GetRsdt();
	NWLC->NwXsdt = NWL_GetXsdt();
	NWLC->ErrLog = NULL;
	if (NWL_IsAdmin() != TRUE)
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Administrator required");
	if (NWL_ObtainPrivileges(SE_SYSTEM_ENVIRONMENT_NAME) != ERROR_SUCCESS)
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "SeSystemEnvironmentPrivilege required");
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
	NW_Libinfo();
	switch (NWLC->NwFormat)
	{
	case FORMAT_YAML:
		NWL_NodeToYaml(NWLC->NwRoot, NWLC->NwFile);
		break;
	case FORMAT_JSON:
		NWL_NodeToJson(NWLC->NwRoot, NWLC->NwFile);
		break;
	case FORMAT_LUA:
		NWL_NodeToLua(NWLC->NwRoot, NWLC->NwFile);
		break;
	}
}

VOID NW_Fini(VOID)
{
	if (NWLC->NwRsdp)
		free(NWLC->NwRsdp);
	if (NWLC->NwRsdt)
		free(NWLC->NwRsdt);
	if (NWLC->NwXsdt)
		free(NWLC->NwXsdt);
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
