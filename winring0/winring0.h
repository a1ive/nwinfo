#pragma once

#include <stdint.h>
#include <windows.h>

struct wr0_drv_t
{
	LPWSTR driver_name;
	LPWSTR driver_id;
	LPWSTR driver_obj;
	WCHAR driver_path[MAX_PATH];
	SC_HANDLE scManager;
	SC_HANDLE scDriver;
	HANDLE hhDriver;
	int errorcode;
};

int cpu_rdmsr(struct wr0_drv_t* driver, uint32_t msr_index, uint64_t* result);
uint8_t io_inb(struct wr0_drv_t* drv, uint16_t port);
uint16_t io_inw(struct wr0_drv_t* drv, uint16_t port);
uint32_t io_inl(struct wr0_drv_t* drv, uint16_t port);
void io_outb(struct wr0_drv_t* drv, uint16_t port, uint8_t value);
void io_outw(struct wr0_drv_t* drv, uint16_t port, uint16_t value);
void io_outl(struct wr0_drv_t* drv, uint16_t port, uint32_t value);
int pci_conf_read(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, void* value, uint32_t size);
uint8_t pci_conf_read8(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg);
uint16_t pci_conf_read16(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg);
uint32_t pci_conf_read32(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg);
int pci_conf_write(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, void* value, uint32_t size);
void pci_conf_write8(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, uint8_t value);
void pci_conf_write16(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, uint16_t value);
void pci_conf_write32(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, uint32_t value);
uint32_t pci_find_by_id(struct wr0_drv_t* drv, uint16_t vid, uint16_t did, uint8_t index);
uint32_t pci_find_by_class(struct wr0_drv_t* drv, uint8_t base, uint8_t sub, uint8_t prog, uint8_t index);
DWORD phymem_read(struct wr0_drv_t* drv, DWORD_PTR address, PBYTE buffer, DWORD count, DWORD unitSize);

struct wr0_drv_t* wr0_driver_open(void);
int wr0_driver_close(struct wr0_drv_t* drv);
