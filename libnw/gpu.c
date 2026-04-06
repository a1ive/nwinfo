// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <devpkey.h>

#include <ioctl.h>
#include "libnw.h"
#include "utils.h"
#include "devtree.h"
#include "gpu/gpu.h"

extern NWLIB_GPU_DRV gpu_drv_intel;
extern NWLIB_GPU_DRV gpu_drv_amd;
extern NWLIB_GPU_DRV gpu_drv_nvidia;
extern NWLIB_GPU_DRV gpu_drv_d3d;

#define PCI_CFG_VENDOR_ID         0x00
#define PCI_CFG_STATUS            0x06
#define PCI_CFG_HEADER_TYPE       0x0E
#define PCI_CFG_CAP_PTR           0x34
#define PCI_CFG_CB_CAP_PTR        0x14

#define PCI_STATUS_CAP_LIST       0x0010

#define PCI_HEADER_TYPE_MASK      0x7F
#define PCI_HEADER_TYPE_CARDBUS   0x02

#define PCI_CAP_ID_PCI_EXP        0x10
#define PCI_CAP_NEXT_MASK         0xFC

#define PCI_EXP_LNKCAP            0x0C
#define PCI_EXP_LNKSTA            0x12

#define PCI_EXP_LNKCAP_SLS        0x0000000F
#define PCI_EXP_LNKCAP_MLW        0x000003F0
#define PCI_EXP_LNKCAP_MLW_SHIFT  4

#define PCI_EXP_LNKSTA_CLS        0x000F
#define PCI_EXP_LNKSTA_NLW        0x03F0
#define PCI_EXP_LNKSTA_NLW_SHIFT  4

static inline uint8_t FindPcieCapability(uint32_t pci_addr)
{
	uint16_t status = WR0_RdPciConf16(NWLC->NwDrv, pci_addr, PCI_CFG_STATUS);
	if ((status & PCI_STATUS_CAP_LIST) == 0)
		return 0;

	uint8_t header_type = WR0_RdPciConf8(NWLC->NwDrv, pci_addr, PCI_CFG_HEADER_TYPE) & PCI_HEADER_TYPE_MASK;
	uint8_t cap_ptr = WR0_RdPciConf8(NWLC->NwDrv, pci_addr,
		header_type == PCI_HEADER_TYPE_CARDBUS ? PCI_CFG_CB_CAP_PTR : PCI_CFG_CAP_PTR);
	cap_ptr &= PCI_CAP_NEXT_MASK;

	for (uint32_t i = 0; i < 48 && cap_ptr >= 0x40; i++)
	{
		uint16_t cap_hdr = WR0_RdPciConf16(NWLC->NwDrv, pci_addr, cap_ptr);
		if ((cap_hdr & 0xFF) == PCI_CAP_ID_PCI_EXP)
			return cap_ptr;
		cap_ptr = (uint8_t)((cap_hdr >> 8) & PCI_CAP_NEXT_MASK);
	}

	return 0;
}

static void GetGpuPcieSpeed(NWLIB_GPU_DEV* dev)
{
	if (NWLC->NwDrv == NULL || NWLC->NwDrv->type == WR0_DRIVER_PAWNIO)
		return;
	if (dev->PciBus > 0xFF || dev->PciDevice > 0x1F || dev->PciFunction > 0x07)
		return;
	if (!WR0_WaitPciBus(500))
		return;
	NWL_Debug("GPU", "Getting PCIe speed for device at %02X:%02X.%d", dev->PciBus, dev->PciDevice, dev->PciFunction);
	uint32_t pci_addr = PciBusDevFunc(dev->PciBus, dev->PciDevice, dev->PciFunction);
	uint16_t vendor_id = WR0_RdPciConf16(NWLC->NwDrv, pci_addr, PCI_CFG_VENDOR_ID);
	if (vendor_id == 0xFFFF || vendor_id == 0x0000)
		goto exit;

	uint8_t pcie_cap = FindPcieCapability(pci_addr);
	if (pcie_cap == 0)
		goto exit;

	uint32_t link_cap = WR0_RdPciConf32(NWLC->NwDrv, pci_addr, pcie_cap + PCI_EXP_LNKCAP);
	uint16_t link_sta = WR0_RdPciConf16(NWLC->NwDrv, pci_addr, pcie_cap + PCI_EXP_LNKSTA);

	uint16_t cur_gen = (uint16_t)(link_sta & PCI_EXP_LNKSTA_CLS);
	uint16_t cur_lanes = (uint16_t)((link_sta & PCI_EXP_LNKSTA_NLW) >> PCI_EXP_LNKSTA_NLW_SHIFT);
	uint16_t max_gen = (uint16_t)(link_cap & PCI_EXP_LNKCAP_SLS);
	uint16_t max_lanes = (uint16_t)((link_cap & PCI_EXP_LNKCAP_MLW) >> PCI_EXP_LNKCAP_MLW_SHIFT);
	NWL_Debug("GPU", "Device at %02X:%02X.%d - Current Gen: %u, Current Lanes: %u, Max Gen: %u, Max Lanes: %u",
		dev->PciBus, dev->PciDevice, dev->PciFunction, cur_gen, cur_lanes, max_gen, max_lanes);

	if (cur_gen > 0)
		dev->CurSpeed.Gen = cur_gen;
	if (cur_lanes > 0)
		dev->CurSpeed.Lanes = cur_lanes;
	if (max_gen > 0)
		dev->MaxSpeed.Gen = max_gen;
	if (max_lanes > 0)
		dev->MaxSpeed.Lanes = max_lanes;

exit:
	WR0_ReleasePciBus();
}

PNWLIB_GPU_INFO NWL_InitGpu(VOID)
{
	PNWLIB_GPU_INFO info = calloc(1, sizeof(NWLIB_GPU_INFO));
	if (info == NULL)
		return NULL;

	info->Driver[NWLIB_GPU_DRV_INTEL] = &gpu_drv_intel;
	info->Driver[NWLIB_GPU_DRV_AMD] = &gpu_drv_amd;
	info->Driver[NWLIB_GPU_DRV_NVIDIA] = &gpu_drv_nvidia;
	info->Driver[NWLIB_GPU_DRV_D3D] = &gpu_drv_d3d;

	for (int i = 0; i < NWLIB_GPU_DRV_COUNT; i++)
	{
		if (info->DeviceCount >= NWL_GPU_MAX_COUNT)
			break;
		info->Driver[i]->Data = info->Driver[i]->Init(info);
		if (info->Driver[i]->Data == NULL)
			continue;
		uint32_t info_cnt = NWL_GPU_MAX_COUNT - info->DeviceCount;
		uint32_t dev_count = info->Driver[i]->GetInfo(info->Driver[i]->Data, &info->Device[info->DeviceCount], info_cnt);
		info->DeviceCount += dev_count;
	}

	for (uint32_t i = 0; i < info->DeviceCount; i++)
	{
		NWLIB_GPU_DEV* dev = &info->Device[i];
		if (dev->CurSpeed.Gen == 0 || dev->MaxSpeed.Gen == 0)
			GetGpuPcieSpeed(dev);
	}

	info->Initialized = 1;

	// TODO: Get GPU vendor/device from pci.ids
	PNWL_ARG_SET pciClasses = NULL;
	NWL_ArgSetAddStr(&pciClasses, "03");
	info->PciList = NWL_EnumPci(NWL_NodeAlloc("PCI", NFLG_TABLE), pciClasses);
	NWL_ArgSetFree(pciClasses);
	return info;
}

VOID NWL_GetGpuInfo(PNWLIB_GPU_INFO info)
{
	if (!info || !info->Initialized)
		return;
	info->DeviceCount = 0;
	for (int i = 0; i < NWLIB_GPU_DRV_COUNT; i++)
	{
		if (info->DeviceCount >= NWL_GPU_MAX_COUNT)
			break;
		if (info->Driver[i]->Data == NULL)
			continue;
		uint32_t info_cnt = NWL_GPU_MAX_COUNT - info->DeviceCount;
		uint32_t dev_count = info->Driver[i]->GetInfo(info->Driver[i]->Data, &info->Device[info->DeviceCount], info_cnt);
		info->DeviceCount += dev_count;
	}
}

VOID NWL_FreeGpu(PNWLIB_GPU_INFO info)
{
	if (!info)
		return;
	if (info->Initialized)
	{
		for (int i = 0; i < NWLIB_GPU_DRV_COUNT; i++)
			info->Driver[i]->Free(info->Driver[i]->Data);
		NWL_NodeFree(info->PciList, 1);
	}
	free(info);
}

static inline void PrintPcieSpeed(PNODE node, const char* name, NWLIB_GPU_PCIE_SPEED* speed)
{
	if (speed->Gen == 0)
		return;
	if (speed->Lanes > 0)
		NWL_NodeAttrSetf(node, name, 0, "PCIe %u.0 x%u", speed->Gen, speed->Lanes);
	else
		NWL_NodeAttrSetf(node, name, 0, "PCIe %u.0", speed->Gen);
}

PNODE NW_Gpu(BOOL bAppend)
{
	PNODE node = NWL_NodeAlloc("GPU", NFLG_TABLE);
	if (bAppend)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	if (NWLC->NwGpu == NULL)
		NWLC->NwGpu = NWL_InitGpu();
	if (NWLC->NwGpu == NULL)
		return node;

	NWL_GetGpuInfo(NWLC->NwGpu);
	for (uint32_t i = 0; i < NWLC->NwGpu->DeviceCount; i++)
	{
		NWLIB_GPU_DEV* dev = &NWLC->NwGpu->Device[i];
		PNODE gpu = NWL_NodeAppendNew(node, "Device", NFLG_TABLE_ROW);
		//NWL_NodeAttrSet(gpu, "HWID", dev->HwId, 0);
		NWL_NodeAttrSet(gpu, "Device", dev->Name, 0);
		NWL_NodeAttrSetf(gpu, "Location", NAFLG_FMT_NEED_QUOTE, "Bus %u, Device %u, Function %u",
			dev->PciBus, dev->PciDevice, dev->PciFunction);
		NWL_NodeAttrSetf(gpu, "PnP ID", 0, "PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X&REV_%02X",
			dev->VendorId, dev->DeviceId, dev->Subsys, dev->RevId);

		NWL_NodeAttrSetBool(gpu, "Integrated GPU", (dev->Flags & NWLIB_GPU_FLAG_INTEGRATED), 0);

		PrintPcieSpeed(gpu, "PCIe Current Link", &dev->CurSpeed);
		PrintPcieSpeed(gpu, "PCIe Max Link", &dev->MaxSpeed);

		NWL_NodeAttrSetf(gpu, "GPU Utilization", 0, "%.1f%%", dev->UsagePercent);
		NWL_NodeAttrSetf(gpu, "Temperature (C)", NAFLG_FMT_NUMERIC, "%.1f", dev->Temperature);
		NWL_NodeAttrSet(gpu, "Total Memory", NWL_GetHumanSize(dev->TotalMemory, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		NWL_NodeAttrSet(gpu, "Free Memory", NWL_GetHumanSize(dev->FreeMemory, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		
		NWL_NodeAttrSetf(gpu, "Memory Usage", 0, "%u%%", (unsigned)dev->MemoryPercent);

		NWL_NodeAttrSetf(gpu, "Power (W)", NAFLG_FMT_NUMERIC, "%.1f", dev->Power);
		NWL_NodeAttrSetf(gpu, "Frequency (MHz)", NAFLG_FMT_NUMERIC, "%.1f", dev->Frequency);
		NWL_NodeAttrSetf(gpu, "Memory Frequency (MHz)", NAFLG_FMT_NUMERIC, "%.1f", dev->MemoryFrequency);
		NWL_NodeAttrSetf(gpu, "Voltage (V)", NAFLG_FMT_NUMERIC, "%.2f", dev->Voltage);
		NWL_NodeAttrSetf(gpu, "Fan Speed (RPM)", NAFLG_FMT_NUMERIC, "%llu", (unsigned long long)dev->FanSpeed);
	}

	return node;
}
