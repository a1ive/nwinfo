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

VOID NWL_InitGpu(PNWLIB_GPU_INFO info)
{
	ZeroMemory(info, sizeof(NWLIB_GPU_INFO));
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
}

VOID NWL_GetGpuInfo(PNWLIB_GPU_INFO info)
{
	if (!info->Initialized)
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
	if (!info->Initialized)
		return;
	for (int i = 0; i < NWLIB_GPU_DRV_COUNT; i++)
		info->Driver[i]->Free(info->Driver[i]->Data);
	NWL_NodeFree(info->PciList, 1);
	ZeroMemory(info, sizeof(NWLIB_GPU_INFO));
}

PNODE NW_Gpu(VOID)
{
	NWLIB_GPU_INFO info;
	NWL_InitGpu(&info);

	PNODE node = NWL_NodeAlloc("GPU", NFLG_TABLE);
	if (NWLC->GpuInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	NWL_GetGpuInfo(&info);
	for (uint32_t i = 0; i < info.DeviceCount; i++)
	{
		PNODE gpu = NWL_NodeAppendNew(node, "Device", NFLG_TABLE_ROW);
		//NWL_NodeAttrSet(gpu, "HWID", info.Device[i].HwId, 0);
		NWL_NodeAttrSet(gpu, "Device", info.Device[i].Name, 0);
		NWL_NodeAttrSetf(gpu, "Location", NAFLG_FMT_NEED_QUOTE, "Bus %u, Device %u, Function %u",
			info.Device[i].PciBus, info.Device[i].PciDevice, info.Device[i].PciFunction);
		NWL_NodeAttrSetf(gpu, "PnP ID", 0, "PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X&REV_%02X",
			info.Device[i].VendorId, info.Device[i].DeviceId, info.Device[i].Subsys, info.Device[i].RevId);

		NWL_NodeAttrSetf(gpu, "GPU Utilization", 0, "%.1f%%", info.Device[i].UsagePercent);
		NWL_NodeAttrSetf(gpu, "Temperature (C)", NAFLG_FMT_NUMERIC, "%.1f", info.Device[i].Temperature);
		NWL_NodeAttrSet(gpu, "Total Memory", NWL_GetHumanSize(info.Device[i].TotalMemory, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		NWL_NodeAttrSet(gpu, "Free Memory", NWL_GetHumanSize(info.Device[i].FreeMemory, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		
		NWL_NodeAttrSetf(gpu, "Memory Usage", 0, "%u%%", (unsigned)info.Device[i].MemoryPercent);

		NWL_NodeAttrSetf(gpu, "Power (W)", NAFLG_FMT_NUMERIC, "%.1f", info.Device[i].Power);
		NWL_NodeAttrSetf(gpu, "Frequency (MHz)", NAFLG_FMT_NUMERIC, "%.1f", info.Device[i].Frequency);
		NWL_NodeAttrSetf(gpu, "Voltage (V)", NAFLG_FMT_NUMERIC, "%.2f", info.Device[i].Voltage);
		NWL_NodeAttrSetf(gpu, "Fan Speed (RPM)", NAFLG_FMT_NUMERIC, "%llu", (unsigned long long)info.Device[i].FanSpeed);
	}

	NWL_FreeGpu(&info);
	return node;
}
