// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"

#include <libcpuid.h>
#include "../libcdi/libcdi.h"
#include "smbus/smbus.h"
#include "gpu/gpu.h"
#include "sensor/sensors.h"

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

static void RealDebugPrint(const char* condition, _Printf_format_string_ char const* const format, ...)
{
	// prefix
	fputc('[', stdout);
	fputs(condition, stdout);
	fputs("] ", stdout);

	// body
	va_list ap;
	va_start(ap, format);
	vfprintf(stdout, format, ap);
	va_end(ap);

	// ensure trailing '\n' if format doesn't end with one
	{
		size_t len = strlen(format);
		if (len == 0 || format[len - 1] != '\n')
			fputc('\n', stdout);
	}

	fflush(stdout);
}

static void FakeDebugPrint(const char* condition, _Printf_format_string_ char const* const format, ...)
{
	UNREFERENCED_PARAMETER(condition);
	UNREFERENCED_PARAMETER(format);
}

void(*NWL_Debug)(const char* condition, _Printf_format_string_ char const* const format, ...) = FakeDebugPrint;

VOID NW_Init(PNWLIB_CONTEXT pContext)
{
	NWLC = pContext;
	if (NWLC->Debug)
		NWL_Debug = RealDebugPrint;
	NWL_NtGetVersion(&NWLC->NwOsInfo);
	GetNativeSystemInfo(&NWLC->NwSi);
	NWLC->NwRoot = NWL_NodeAlloc("NWinfo", 0);
	WR0_OpenMutexes();

	NWLC->ErrLog = NULL;
	if (NWL_IsAdmin() != TRUE)
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Administrator required");
	if (NWL_ObtainPrivileges(SE_SYSTEM_ENVIRONMENT_NAME) != ERROR_SUCCESS)
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "SeSystemEnvironmentPrivilege required");
	if (NWL_ObtainPrivileges(SE_LOAD_DRIVER_NAME) != ERROR_SUCCESS)
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "SeLoadDriverPrivilege required");
	if (WR0_IsWoW64())
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Running under WoW64 mode");

	NWLC->NwDrv = WR0_OpenDriver();
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
	NWLC->NwPciIds.Ids = NWL_LoadIdsToMemory(L"pci.ids", &NWLC->NwPciIds.Size);
	NWLC->NwUsbIds.Ids = NWL_LoadIdsToMemory(L"usb.ids", &NWLC->NwUsbIds.Size);
	NWLC->NwPnpIds.Ids = NWL_LoadIdsToMemory(L"pnp.ids", &NWLC->NwPnpIds.Size);
	NWLC->NwJep106.Ids = NWL_LoadIdsToMemory(L"jep106.ids", &NWLC->NwJep106.Size);
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
	if (NWLC->SysInfo)
		NW_System();
	if (NWLC->UsbInfo)
		NW_Usb();
	if (NWLC->SpdInfo)
		NW_Spd();
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
	if (NWLC->DevTree)
		NW_DevTree();
	if (NWLC->Sensors)
		NW_Sensors();
	NW_Libinfo();
	NW_Export(NWLC->NwRoot, NWLC->NwFile);
}

VOID NW_Fini(VOID)
{
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
	SM_Free(NWLC->NwSmbus);
	NWL_FreeGpu(NWLC->NwGpu);
	cpuid_free_system_id(NWLC->NwCpuid);
	free(NWLC->NwCpuid);
	cpuid_free_raw_data_array(NWLC->NwCpuRaw);
	free(NWLC->NwCpuRaw);
	NWL_FreeSensors();
	WR0_CloseDriver(NWLC->NwDrv);
	WR0_CloseMutexes();
	NWL_NodeFree(NWLC->NwRoot, 1);
	if (NWLC->NwFile && NWLC->NwFile != stdout)
		fclose(NWLC->NwFile);
	free(NWLC->ErrLog);
	free(NWLC->NwPciIds.Ids);
	free(NWLC->NwUsbIds.Ids);
	free(NWLC->NwPnpIds.Ids);
	free(NWLC->NwJep106.Ids);
	NWL_Debug("NW", "Exit");
	NWL_Debug = FakeDebugPrint;
	ZeroMemory(NWLC, sizeof(NWLIB_CONTEXT));
	NWLC = NULL;
}
