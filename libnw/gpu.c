// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <devpkey.h>

#include "libnw.h"
#include "utils.h"
#include "devtree.h"
#include "gpu/gpu.h"

extern NWLIB_GPU_DRV gpu_drv_intel;
extern NWLIB_GPU_DRV gpu_drv_amd;
extern NWLIB_GPU_DRV gpu_drv_nvidia;
extern NWLIB_GPU_DRV gpu_drv_d3d;
extern NWLIB_GPU_DRV gpu_drv_gpuz;

PNWLIB_GPU_INFO NWL_InitGpu(VOID)
{
	PNWLIB_GPU_INFO info = calloc(1, sizeof(NWLIB_GPU_INFO));
	if (info == NULL)
		return NULL;

	info->Driver[NWLIB_GPU_DRV_INTEL] = &gpu_drv_intel;
	info->Driver[NWLIB_GPU_DRV_AMD] = &gpu_drv_amd;
	info->Driver[NWLIB_GPU_DRV_NVIDIA] = &gpu_drv_nvidia;
	info->Driver[NWLIB_GPU_DRV_D3D] = &gpu_drv_d3d;
	info->Driver[NWLIB_GPU_DRV_GPUZ] = &gpu_drv_gpuz;

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

	info->Initialized = 1;

	// TODO: Get GPU vendor/device from pci.ids
	info->PciList = NWL_EnumPci(NWL_NodeAlloc("PCI", NFLG_TABLE), "03");
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

PNODE NW_Gpu(VOID)
{
	PNODE node = NWL_NodeAlloc("GPU", NFLG_TABLE);
	if (NWLC->GpuInfo)
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

		NWL_NodeAttrSetf(gpu, "GPU Utilization", 0, "%.1f%%", dev->UsagePercent);
		NWL_NodeAttrSetf(gpu, "Temperature (C)", NAFLG_FMT_NUMERIC, "%.1f", dev->Temperature);
		NWL_NodeAttrSet(gpu, "Total Memory", NWL_GetHumanSize(dev->TotalMemory, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		NWL_NodeAttrSet(gpu, "Free Memory", NWL_GetHumanSize(dev->FreeMemory, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		
		NWL_NodeAttrSetf(gpu, "Memory Usage", 0, "%u%%", (unsigned)dev->MemoryPercent);

		NWL_NodeAttrSetf(gpu, "Power (W)", NAFLG_FMT_NUMERIC, "%.1f", dev->Power);
		NWL_NodeAttrSetf(gpu, "Frequency (MHz)", NAFLG_FMT_NUMERIC, "%.1f", dev->Frequency);
		NWL_NodeAttrSetf(gpu, "Voltage (V)", NAFLG_FMT_NUMERIC, "%.2f", dev->Voltage);
		NWL_NodeAttrSetf(gpu, "Fan Speed (RPM)", NAFLG_FMT_NUMERIC, "%llu", (unsigned long long)dev->FanSpeed);
	}

	return node;
}
