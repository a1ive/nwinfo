// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdint.h>
#include <windows.h>

enum wr0_driver_type
{
	WR0_DRIVER_NONE = 0,
	WR0_DRIVER_WINRING0,
	WR0_DRIVER_HWRWDRV,
	WR0_DRIVER_PAWNIO,
};

struct wr0_drv_t
{
	LPCWSTR driver_name;
	enum wr0_driver_type driver_type;
	LPCWSTR driver_id;
	LPCWSTR driver_obj;
	WCHAR driver_path[MAX_PATH];
	SC_HANDLE scManager;
	SC_HANDLE scDriver;
	HANDLE hhDriver;
	int errorcode;
	int debug;
};

int WR0_RdMsr(struct wr0_drv_t* drv, uint32_t msr_index, uint64_t* result);
int WR0_WrMsr(struct wr0_drv_t* drv, uint32_t msr_index, DWORD eax, DWORD edx);
uint8_t WR0_RdIo8(struct wr0_drv_t* drv, uint16_t port);
uint16_t WR0_RdIo16(struct wr0_drv_t* drv, uint16_t port);
uint32_t WR0_RdIo32(struct wr0_drv_t* drv, uint16_t port);
void WR0_WrIo8(struct wr0_drv_t* drv, uint16_t port, uint8_t value);
void WR0_WrIo16(struct wr0_drv_t* drv, uint16_t port, uint16_t value);
void WR0_WrIo32(struct wr0_drv_t* drv, uint16_t port, uint32_t value);
int WR0_RdPciConf(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, void* value, uint32_t size);
uint8_t WR0_RdPciConf8(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg);
uint16_t WR0_RdPciConf16(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg);
uint32_t WR0_RdPciConf32(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg);
int WR0_WrPciConf(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, void* value, uint32_t size);
void WR0_WrPciConf8(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, uint8_t value);
void WR0_WrPciConf16(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, uint16_t value);
void WR0_WrPciConf32(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, uint32_t value);
uint32_t WR0_FindPciById(struct wr0_drv_t* drv, uint16_t vid, uint16_t did, uint8_t index);
uint32_t WR0_FindPciByClass(struct wr0_drv_t* drv, uint8_t base, uint8_t sub, uint8_t prog, uint8_t index);
DWORD WR0_RdMem(struct wr0_drv_t* drv, DWORD_PTR address, PBYTE buffer, DWORD count, DWORD unitSize);

int WR0_LoadPawn(struct wr0_drv_t* drv, PVOID blob, DWORD size);
int WR0_ExecPawn(struct wr0_drv_t* drv, LPCSTR fn, const ULONG64* in, SIZE_T in_size, PULONG64 out, SIZE_T out_size, PSIZE_T return_size);

struct wr0_drv_t* WR0_OpenDriver(int debug);
int WR0_CloseDriver(struct wr0_drv_t* drv);
