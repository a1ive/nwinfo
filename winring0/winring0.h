#pragma once

#include <stdint.h>
#include <windows.h>

struct wr0_drv_t
{
	LPCWSTR driver_name;
	LPCWSTR driver_id;
	LPCWSTR driver_obj;
	WCHAR driver_path[MAX_PATH];
	SC_HANDLE scManager;
	SC_HANDLE scDriver;
	HANDLE hhDriver;
	int errorcode;
};

int WR0_RdMsr(struct wr0_drv_t* driver, uint32_t msr_index, uint64_t* result);
int WR0_WrMsr(struct wr0_drv_t* driver, uint32_t msr_index, DWORD eax, DWORD edx);
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

struct wr0_drv_t* WR0_OpenDriver(void);
int WR0_CloseDriver(struct wr0_drv_t* drv);
