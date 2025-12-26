// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdio.h>
#define VC_EXTRALEAN
#include <windows.h>
#include <stdnoreturn.h>

#include "nwapi.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "node.h"

#define NWINFO_BUFSZ 65535
#define NWINFO_BUFSZW (NWINFO_BUFSZ / sizeof(WCHAR))
#define NWINFO_BUFSZB (NWINFO_BUFSZW * sizeof(WCHAR))

struct ACPI_RSDP_V2;
struct ACPI_RSDT;
struct ACPI_XSDT;
struct wr0_drv_t;
struct _CDI_SMART;
struct cpu_raw_data_array_t;
struct system_id_t;
struct smbus_context;
struct _NWLIB_GPU_INFO;

typedef struct _NWLIB_IDS
{
	CHAR* Ids;
	DWORD Size;
} NWLIB_IDS, * PNWLIB_IDS;

typedef struct _NWLIB_CONTEXT
{
	BOOL HumanSize;
	enum
	{
		BIN_FMT_NONE = 0,
		BIN_FMT_BASE64,
		BIN_FMT_HEX,
	} BinaryFormat;

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
	BOOL PublicIpInfo;
	BOOL ProductPolicyInfo;
	BOOL GpuInfo;
	BOOL FontInfo;
	BOOL DevTree;
	BOOL Sensors;

	BOOL Debug;
	BOOL HideSensitive;

	LPCSTR DevTreeFilter;
	DWORD AcpiTable;
	UINT8 SmbiosType;
	LPCSTR PciClass;
	LPSTR DiskPath;
	LPSTR NetGuid;
	LPCSTR ProductPolicy;
	LPCSTR CpuDump;
	LPCSTR SpdDump;

#define NW_NET_ACTIVE (1 << 0)
#define NW_NET_PHYS   (1 << 1)
#define NW_NET_IPV4   (1 << 2)
#define NW_NET_IPV6   (1 << 3)
#define NW_NET_ETH    (1 << 4)
#define NW_NET_WLAN   (1 << 5)
	UINT64 NetFlags;
#define NW_DISK_NO_SMART (1 << 0)
#define NW_DISK_PHYS     (1 << 1)
#define NW_DISK_HD       (1 << 2)
#define NW_DISK_CD       (1 << 3)
#define NW_DISK_NVME     (1 << 11)
#define NW_DISK_SATA     (1 << 12)
#define NW_DISK_SCSI     (1 << 13)
#define NW_DISK_SAS      (1 << 14)
#define NW_DISK_USB      (1 << 15)
	UINT64 DiskFlags;
#define NW_UEFI_VARS     (1 << 0)
#define NW_UEFI_MENU     (1 << 1)
	UINT64 UefiFlags;

	VOID (*SpdProgress) (LPCSTR lpszText);

	OSVERSIONINFOEXW NwOsInfo;
	SYSTEM_INFO NwSi;

	struct cpu_raw_data_array_t* NwCpuRaw;
	struct system_id_t* NwCpuid;

	struct ACPI_RSDP_V2* NwRsdp;
	struct ACPI_RSDT* NwRsdt;
	struct ACPI_XSDT* NwXsdt;

	struct RAW_SMBIOS_DATA* NwSmbios;
	BOOL NwSmartInit;

	struct _CDI_SMART* NwSmart;
	UINT64 NwSmartFlags;

	struct smbus_context* NwSmbus;

	struct _NWLIB_GPU_INFO* NwGpu;

	UINT64 NwSensorFlags;

	struct wr0_drv_t* NwDrv;
	UINT CodePage;
	struct _NODE* NwRoot;
	enum
	{
		FORMAT_YAML = 0,
		FORMAT_JSON,
		FORMAT_LUA,
		FORMAT_TREE,
		FORMAT_HTML,
	} NwFormat;

	FILE* NwFile;
	LPCSTR* NwUnits;
	CHAR* ErrLog;
	VOID (*ErrLogCallback) (LPCSTR lpszText);

	NWLIB_IDS NwPciIds;
	NWLIB_IDS NwUsbIds;
	NWLIB_IDS NwPnpIds;
	NWLIB_IDS NwJep106;

	union
	{
		WCHAR NwBufW[NWINFO_BUFSZW];
		CHAR NwBuf[NWINFO_BUFSZ];
	};
} NWLIB_CONTEXT, *PNWLIB_CONTEXT;

extern PNWLIB_CONTEXT NWLC;

LIBNW_API VOID NW_Init(PNWLIB_CONTEXT pContext);
LIBNW_API VOID NW_Export(PNODE node, FILE* file);
LIBNW_API VOID NW_Print(LPCSTR lpFileName);
LIBNW_API VOID NW_Fini(VOID);

#ifdef noreturn
LIBNW_API noreturn VOID NWL_ErrExit(INT nExitCode, LPCSTR lpszText);
#else
LIBNW_API VOID NWL_ErrExit(INT nExitCode, LPCSTR lpszText);
#endif
extern void(*NWL_Debug)(const char* condition, _Printf_format_string_ char const* const format, ...);

LIBNW_API PNODE NW_Acpi(VOID);
LIBNW_API PNODE NW_Cpuid(VOID);
LIBNW_API PNODE NW_Disk(VOID);
LIBNW_API PNODE NW_Edid(VOID);
LIBNW_API PNODE NW_Network(VOID);
LIBNW_API PNODE NW_Pci(VOID);
LIBNW_API PNODE NW_Smbios(VOID);
LIBNW_API PNODE NW_Spd(VOID);
LIBNW_API PNODE NW_System(VOID);
LIBNW_API PNODE NW_Usb(VOID);
LIBNW_API PNODE NW_Battery(VOID);
LIBNW_API PNODE NW_Libinfo(VOID);
LIBNW_API PNODE NW_Uefi(VOID);
LIBNW_API PNODE NW_NetShare(VOID);
LIBNW_API PNODE NW_Audio(VOID);
LIBNW_API PNODE NW_PublicIp(VOID);
LIBNW_API PNODE NW_ProductPolicy(VOID);
LIBNW_API PNODE NW_Gpu(VOID);
LIBNW_API PNODE NW_Font(VOID);
LIBNW_API PNODE NW_DevTree(VOID);
LIBNW_API PNODE NW_Sensors(VOID);

LIBNW_API VOID NWL_GetUptime(CHAR* szUptime, DWORD dwSize);
LIBNW_API VOID NWL_GetHostname(CHAR* szHostname);

#define NWL_STR_SIZE 64

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
	CHAR StrPhysTotal[NWL_STR_SIZE];
	CHAR StrPhysAvail[NWL_STR_SIZE];
	CHAR StrPageTotal[NWL_STR_SIZE];
	CHAR StrPageAvail[NWL_STR_SIZE];
	CHAR StrSfciTotal[NWL_STR_SIZE];
	CHAR StrSfciAvail[NWL_STR_SIZE];
} NWLIB_MEM_INFO, *PNWLIB_MEM_INFO;

LIBNW_API VOID NWL_GetMemInfo(PNWLIB_MEM_INFO pMemInfo);

LIBNW_API double NWL_GetCpuUsage(VOID);
LIBNW_API DWORD NWL_GetCpuFreq(VOID);

typedef struct _NWLIB_CPU_INFO
{
	CHAR MsrMulti[NWL_STR_SIZE];
	int MsrTemp;
	double MsrVolt;
	double MsrFreq;
	double MsrBus;
	ULONGLONG Ticks;
	int MsrEnergy;
	double MsrPower;
	double MsrPl1;
	double MsrPl2;
	int GpuTemp;
	int GpuEnergy;
	double GpuPower;
	UINT32 BiosRev;
}NWLIB_CPU_INFO;
LIBNW_API VOID NWL_GetCpuMsr(int count, NWLIB_CPU_INFO* info);

typedef struct _NWLIB_NET_TRAFFIC
{
	UINT64 Recv;
	UINT64 Send;
	CHAR StrRecv[NWL_STR_SIZE];
	CHAR StrSend[NWL_STR_SIZE];
} NWLIB_NET_TRAFFIC;
LIBNW_API VOID NWL_GetNetTraffic(NWLIB_NET_TRAFFIC* info, BOOL bit);

LIBNW_API PNODE NWL_EnumPci(PNODE pNode, LPCSTR pciClass);

typedef struct _NWLIB_CUR_DISPLAY
{
	LONG Width;
	LONG Height;
	UINT Dpi;
	UINT Scale;
} NWLIB_CUR_DISPLAY;
LIBNW_API VOID NWL_GetCurDisplay(HWND wnd, NWLIB_CUR_DISPLAY* info);

typedef struct _NWLIB_MEM_SENSORS
{
	BOOL Initialized;
	UINT8 Count;
	struct
	{
		UINT8 Addr;
		UINT8 Type;
		float Temp;
	} Sensor[8];
} NWLIB_MEM_SENSORS;
LIBNW_API VOID NWL_GetMemSensors(struct smbus_context* ctx, NWLIB_MEM_SENSORS* info);

#ifdef __cplusplus
} /* extern "C" */
#endif
