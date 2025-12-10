// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdint.h>
#include <windows.h>
#include "../libnw/nwapi.h"

enum wr0_driver_type
{
	WR0_DRIVER_NONE = 0,
	WR0_DRIVER_WINRING0,
	WR0_DRIVER_HWIO,
	WR0_DRIVER_CPUZ161,
	WR0_DRIVER_PAWNIO,

	WR0_DRIVER_MAX
};

struct pio_mod_t
{
	HANDLE hd;
	void* blob;
	DWORD size;
};

struct wr0_drv_t
{
	LPCWSTR name;
	enum wr0_driver_type type;
	LPCWSTR id;
	LPCWSTR obj;

	BOOL(*load)(struct wr0_drv_t* drv);
	BOOL(*install)(struct wr0_drv_t* drv);
	void(*uninstall)(struct wr0_drv_t* drv);

	WCHAR path[MAX_PATH];
	SC_HANDLE scm;
	SC_HANDLE sch;
	HANDLE handle;
	BOOL installed;

	struct pio_mod_t pio_amd0f;
	struct pio_mod_t pio_amd10;
	struct pio_mod_t pio_amd17;
	struct pio_mod_t pio_intel;
	struct pio_mod_t pio_rysmu;
	struct pio_mod_t pio_smi801;
	struct pio_mod_t pio_smpiix4;
};

// Bus Number, Device Number and Function Number to PCI Device Address
#define PciBusDevFunc(Bus, Dev, Func)	((Bus&0xFF)<<8) | ((Dev&0x1F)<<3) | (Func&7)
// PCI Device Address to Bus Number
#define PciGetBus(address)				((address>>8) & 0xFF)
// PCI Device Address to Device Number
#define PciGetDev(address)				((address>>3) & 0x1F)
// PCI Device Address to Function Number
#define PciGetFunc(address)				(address&7)

LIBNW_API BOOL WR0_CheckPawnIO(void);
LIBNW_API BOOL WR0_InstallPawnIO(void);

LIBNW_API int WR0_RdMsr(struct wr0_drv_t* drv, uint32_t msr_index, uint64_t* result);
LIBNW_API int WR0_WrMsr(struct wr0_drv_t* drv, uint32_t msr_index, DWORD eax, DWORD edx);
LIBNW_API uint8_t WR0_RdIo8(struct wr0_drv_t* drv, uint16_t port);
LIBNW_API uint16_t WR0_RdIo16(struct wr0_drv_t* drv, uint16_t port);
LIBNW_API uint32_t WR0_RdIo32(struct wr0_drv_t* drv, uint16_t port);
LIBNW_API void WR0_WrIo8(struct wr0_drv_t* drv, uint16_t port, uint8_t value);
LIBNW_API void WR0_WrIo16(struct wr0_drv_t* drv, uint16_t port, uint16_t value);
LIBNW_API void WR0_WrIo32(struct wr0_drv_t* drv, uint16_t port, uint32_t value);
LIBNW_API int WR0_RdPciConf(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, void* value, uint32_t size);
LIBNW_API uint8_t WR0_RdPciConf8(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg);
LIBNW_API uint16_t WR0_RdPciConf16(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg);
LIBNW_API uint32_t WR0_RdPciConf32(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg);
LIBNW_API int WR0_WrPciConf(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, void* value, uint32_t size);
LIBNW_API void WR0_WrPciConf8(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, uint8_t value);
LIBNW_API void WR0_WrPciConf16(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, uint16_t value);
LIBNW_API void WR0_WrPciConf32(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, uint32_t value);
LIBNW_API uint32_t WR0_FindPciById(struct wr0_drv_t* drv, uint16_t vid, uint16_t did, uint8_t index);
LIBNW_API uint32_t WR0_FindPciByClass(struct wr0_drv_t* drv, uint8_t base, uint8_t sub, uint8_t prog, uint8_t index);
LIBNW_API int WR0_RdMmIo(struct wr0_drv_t* drv, uint64_t addr, void* value, uint32_t size);
LIBNW_API DWORD WR0_RdMem(struct wr0_drv_t* drv, DWORD_PTR address, PBYTE buffer, DWORD count, DWORD unitSize);

enum wr0_smn_type
{
	WR0_SMN_AMD15H = 1, // 0xB8/0xBC
	WR0_SMN_ZENSMU = 2, // 0xC4/0xC8
	WR0_SMN_AMD17H = 3, // 0x60/0x64
};
LIBNW_API DWORD WR0_RdAmdSmn(struct wr0_drv_t* drv, enum wr0_smn_type smn, DWORD reg);
LIBNW_API int WR0_SendSmuCmd(struct wr0_drv_t* drv, uint32_t cmd, uint32_t rsp, uint32_t arg, uint32_t fn, uint32_t args[6]);

LIBNW_API int WR0_ExecPawn(struct wr0_drv_t* drv, struct pio_mod_t* mod, LPCSTR fn, const ULONG64* in, SIZE_T in_size, PULONG64 out, SIZE_T out_size, PSIZE_T return_size);

LIBNW_API struct wr0_drv_t* WR0_OpenDriver(void);
LIBNW_API void WR0_CloseDriver(struct wr0_drv_t* drv);

LIBNW_API void WR0_MicroSleep(unsigned int usec);

LIBNW_API void WR0_OpenMutexes(void);
LIBNW_API void WR0_CloseMutexes(void);
LIBNW_API BOOL WR0_WaitPciBus(DWORD timeout);
LIBNW_API void WR0_ReleasePciBus(void);
LIBNW_API BOOL WR0_WaitSmBus(DWORD timeout);
LIBNW_API void WR0_ReleaseSmBus(void);

LIBNW_API BOOL WR0_IsWoW64(void);
LPCSTR WR0_GetWineVersion(void);
void WR0_GetWineHost(const CHAR** sysname, const CHAR** release);
