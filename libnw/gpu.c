// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <devpkey.h>
#include <d3dcommon.h>

#include "libnw.h"
#include "utils.h"
#include "devtree.h"
#include "gpu/gpu.h"

extern NWLIB_GPU_DRV gpu_drv_intel;
extern NWLIB_GPU_DRV gpu_drv_amd;
extern NWLIB_GPU_DRV gpu_drv_nvidia;
extern NWLIB_GPU_DRV gpu_drv_dxcore;
extern NWLIB_GPU_DRV gpu_drv_d3d;

PNWLIB_GPU_INFO NWL_InitGpu(void)
{
	PNWLIB_GPU_INFO info = calloc(1, sizeof(NWLIB_GPU_INFO));
	if (info == NULL)
		return NULL;

	info->Driver[NWLIB_GPU_DRV_INTEL] = &gpu_drv_intel;
	info->Driver[NWLIB_GPU_DRV_AMD] = &gpu_drv_amd;
	info->Driver[NWLIB_GPU_DRV_NVIDIA] = &gpu_drv_nvidia;
	info->Driver[NWLIB_GPU_DRV_DXCORE] = &gpu_drv_dxcore;
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

	info->Initialized = 1;

	// TODO: Get GPU vendor/device from pci.ids
	PNWL_ARG_SET pciClasses = NULL;
	NWL_ArgSetAddStr(&pciClasses, "03");
	info->PciList = NWL_EnumPci(NWL_NodeAlloc("PCI", NFLG_TABLE), pciClasses);
	NWL_ArgSetFree(pciClasses);
	return info;
}

void NWL_GetGpuInfo(PNWLIB_GPU_INFO info)
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

void NWL_FreeGpu(PNWLIB_GPU_INFO info)
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

static const char* GetDxVersionStr(uint32_t version)
{
	switch (version)
	{
	case D3D_FEATURE_LEVEL_12_2: return "12.2";
	case D3D_FEATURE_LEVEL_12_1: return "12.1";
	case D3D_FEATURE_LEVEL_12_0: return "12.0";
	case D3D_FEATURE_LEVEL_11_1: return "11.1";
	case D3D_FEATURE_LEVEL_11_0: return "11.0";
	case D3D_FEATURE_LEVEL_10_1: return "10.1";
	case D3D_FEATURE_LEVEL_10_0: return "10.0";
	case D3D_FEATURE_LEVEL_9_3: return "9.3";
	case D3D_FEATURE_LEVEL_9_2: return "9.2";
	case D3D_FEATURE_LEVEL_9_1: return "9.1";
	case D3D_FEATURE_LEVEL_1_0_CORE: return "Core";
	case D3D_FEATURE_LEVEL_1_0_GENERIC: return "Generic";
	default: return "Unknown";
	}
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
		NWL_NodeAttrSetBool(gpu, "NPU", (dev->Flags & NWLIB_GPU_FLAG_NPU), 0);

		PrintPcieSpeed(gpu, "PCIe Current Link", &dev->CurSpeed);
		PrintPcieSpeed(gpu, "PCIe Max Link", &dev->MaxSpeed);

		if (dev->DxVersion > 0)
			NWL_NodeAttrSet(gpu, "DirectX Support", GetDxVersionStr(dev->DxVersion), 0);

		NWL_NodeAttrSetf(gpu, "GPU Utilization", 0, "%.1f%%", dev->UsagePercent);
		NWL_NodeAttrSetf(gpu, NWL_GetTemperatureLabel(), NAFLG_FMT_NUMERIC, "%.1f", NWL_GetTemperature((float)dev->Temperature));
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
