// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"
#include "efivars.h"

#include "libcpuid.h"
#include "../libcdi/libcdi.h"
#include "smbus/smbus.h"
#include "gpu/gpu.h"
#include "cpu/rdmsr.h"
#include "sensor/sensors.h"
#include "pci_ids.h"
#include "pnp_ids.h"
#include "usb_ids.h"
#include "spd_ids.h"

PNWLIB_CONTEXT NWLC = NULL;

static const char* NWL_HS_BYTE[] =
{ "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB" };

static char NWL_DEFAULT_PCI_IDS[] = PCI_IDS_DEFAULT;
static char NWL_DEFAULT_PNP_IDS[] = PNP_IDS_DEFAULT;
static char NWL_DEFAULT_USB_IDS[] = USB_IDS_DEFAULT;
static char NWL_DEFAULT_SPD_IDS[] = SPD_IDS_DEFAULT;

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
	NWLC->NwIsEfi = NWL_IsEfi();
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
	NWLC->NwSmbios = NWL_GetSmbios();
	NWLC->NwSmart = cdi_create_smart();
	NWLC->NwSmartFlags = CDI_FLAG_DEFAULT;
	NWLC->NwCpuRaw = calloc(1, sizeof(struct cpu_raw_data_array_t));
	NWLC->NwCpuid = calloc(1, sizeof(struct system_id_t));
	if (!NWLC->NwCpuRaw || !NWLC->NwCpuid)
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Cannot allocate memory");
	if (NWLC->Debug)
		cpuid_set_verbosiness_level(2);
	cpuid_set_warn_function(NWL_Debug);
	NWLC->NwSmartInit = FALSE;
	NWLC->NwUnits = NWL_HS_BYTE;
	if (!NWL_LoadIdsToMemory(L"pci.ids", &NWLC->NwPciIds))
	{
		NWLC->NwPciIds.Ids = NWL_DEFAULT_PCI_IDS;
		NWLC->NwPciIds.Size = ARRAYSIZE(NWL_DEFAULT_PCI_IDS);
		NWLC->NwPciIds.Alloc = FALSE;
	}
	if (!NWL_LoadIdsToMemory(L"usb.ids", &NWLC->NwUsbIds))
	{
		NWLC->NwUsbIds.Ids = NWL_DEFAULT_USB_IDS;
		NWLC->NwUsbIds.Size = ARRAYSIZE(NWL_DEFAULT_USB_IDS);
		NWLC->NwUsbIds.Alloc = FALSE;
	}
	if (!NWL_LoadIdsToMemory(L"pnp.ids", &NWLC->NwPnpIds))
	{
		NWLC->NwPnpIds.Ids = NWL_DEFAULT_PNP_IDS;
		NWLC->NwPnpIds.Size = ARRAYSIZE(NWL_DEFAULT_PNP_IDS);
		NWLC->NwPnpIds.Alloc = FALSE;
	}
	if (!NWL_LoadIdsToMemory(L"jep106.ids", &NWLC->NwJep106))
	{
		NWLC->NwJep106.Ids = NWL_DEFAULT_SPD_IDS;
		NWLC->NwJep106.Size = ARRAYSIZE(NWL_DEFAULT_SPD_IDS);
		NWLC->NwJep106.Alloc = FALSE;
	}
}

VOID NW_Print(LPCSTR lpFileName)
{
	if (lpFileName && fopen_s(&NWLC->NwFile, lpFileName, "w"))
		NWL_ErrExit(ERROR_OPEN_FAILED, "Cannot open file");
	if (!NWLC->NwFile)
		return;
	if (NWLC->AcpiInfo)
		NW_Acpi(TRUE);
	if (NWLC->CpuInfo)
		NW_Cpuid(TRUE);
	if (NWLC->DiskInfo)
		NW_Disk(TRUE);
	if (NWLC->EdidInfo)
		NW_Edid(TRUE);
	if (NWLC->NetInfo)
		NW_Network(TRUE);
	if (NWLC->PciInfo)
		NW_Pci(TRUE);
	if (NWLC->DmiInfo)
		NW_Smbios(TRUE);
	if (NWLC->SysInfo)
		NW_System(TRUE);
	if (NWLC->UsbInfo)
		NW_Usb(TRUE);
	if (NWLC->SpdInfo)
		NW_Spd(TRUE);
	if (NWLC->BatteryInfo)
		NW_Battery(TRUE);
	if (NWLC->UefiInfo)
		NW_Uefi(TRUE);
	if (NWLC->ShareInfo)
		NW_NetShare(TRUE);
	if (NWLC->AudioInfo)
		NW_Audio(TRUE);
	if (NWLC->PublicIpInfo)
		NW_PublicIp(TRUE);
	if (NWLC->ProductPolicyInfo)
		NW_ProductPolicy(TRUE);
	if (NWLC->GpuInfo)
		NW_Gpu(TRUE);
	if (NWLC->FontInfo)
		NW_Font(TRUE);
	if (NWLC->DevTree)
		NW_DevTree(TRUE);
	if (NWLC->Sensors)
		NW_Sensors(TRUE);
	NW_Libinfo();
	NW_Export(NWLC->NwRoot, NWLC->NwFile);
}

VOID NW_Fini(VOID)
{
	NWL_ArgSetFree(NWLC->SmbiosTypes);
	NWL_ArgSetFree(NWLC->PciClasses);
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
	if (NWLC->NwMsr)
	{
		for (uint8_t i = 0; i < NWLC->NwCpuid->num_cpu_types; i++)
			NWL_MsrFini(&NWLC->NwMsr[i]);
		free(NWLC->NwMsr);
	}
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
	NWL_UnloadIds(&NWLC->NwPciIds);
	NWL_UnloadIds(&NWLC->NwUsbIds);
	NWL_UnloadIds(&NWLC->NwPnpIds);
	NWL_UnloadIds(&NWLC->NwJep106);
	NWL_Debug("NW", "Exit");
	NWL_Debug = FakeDebugPrint;
	ZeroMemory(NWLC, sizeof(NWLIB_CONTEXT));
	NWLC = NULL;
}
