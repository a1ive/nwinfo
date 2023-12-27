// SPDX-License-Identifier: LGPL
/* Copyright (C) 2018-2019 Jiaxun Yang <jiaxun.yang@flygoat.com> */
/* Access PCI Config Space - winring0 */


#include "nb_smu_ops.h"

#include <windows.h>

#include "../winring0/winring0.h"

static struct wr0_drv_t* ols = NULL;

bool nb_pci_obj = true;
uint32_t nb_pci_address = 0x0;
BYTE saved_buf[0x1000];

pci_obj_t init_pci_obj(void)
{
	ols = wr0_driver_open();
	if (ols == NULL)
		return NULL;
	return &nb_pci_obj;
}

nb_t get_nb(pci_obj_t obj)
{
	return &nb_pci_address;
}

void free_nb(nb_t nb)
{
	return;
}

void free_pci_obj(pci_obj_t obj)
{
	wr0_driver_close(ols);
	ols = NULL;
}

u32 smn_reg_read(nb_t nb, u32 addr)
{
	pci_conf_write32(ols, *nb, NB_PCI_REG_ADDR_ADDR, addr & (~0x3));
	return pci_conf_read32(ols, *nb, NB_PCI_REG_DATA_ADDR);
}

void smn_reg_write(nb_t nb, u32 addr, u32 data)
{
	pci_conf_write32(ols, *nb, NB_PCI_REG_ADDR_ADDR, addr);
	pci_conf_write32(ols, *nb, NB_PCI_REG_DATA_ADDR, data);
}

mem_obj_t init_mem_obj(uintptr_t physAddr)
{
	memset(saved_buf, 0, 0x1000);
	phymem_read(ols, physAddr, saved_buf, 0x1000, 1);
	return saved_buf;
}

void free_mem_obj(mem_obj_t hInpOutDll)
{
	return;
}

int copy_pm_table(void* buffer, size_t size)
{
	if (size > 0x1000)
		return -1;
	memcpy(buffer, saved_buf, size);
	return 0;
}

int compare_pm_table(void* buffer, size_t size)
{
	if (size > 0x1000)
		return -1;
	return memcmp(buffer, saved_buf, size);
}

bool is_using_smu_driver()
{
	return false;
}
