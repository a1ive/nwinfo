// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include "gpu.h"
#include "nvapi.h"

#define NVDL "NV"

struct NV_GPU_DATA
{
	bool Valid;
	char Name[NVAPI_SHORT_STRING_MAX];
	NvU32 DeviceId; // (did << 16 | vid)
	NvU32 Subsys;
	NvU32 RevId;
	NvU32 ExtDeviceId;
	NvU32 VendorId;
	NvU32 BusId;
	NvU32 SlotId;
	NV_GPU_MEMORY_INFO_EX MemEx;
	NV_DISPLAY_DRIVER_MEMORY_INFO Mem;
	NV_GPU_DYNAMIC_PSTATES_INFO_EX Pstates;
	NV_USAGES Usages;
	NV_POWER_TOPOLOGY Power;
	NV_GPU_CLOCK_FREQUENCIES Clock;

	NV_GPU_PERF_PSTATE_ID CurPstate;
	NV_GPU_PERF_PSTATES20_INFO Pstates20;
	NV_GPU_PERF_PSTATES_INFO PstatesL;

	union
	{
		NV_GPU_THERMAL_SETTINGS ThermalV2;
		NV_GPU_THERMAL_SETTINGS_V1 ThermalV1;
	};
	NV_THERMAL_SENSORS ThermalSensors;
	NV_FAN_COOLER_STATUS FanStatus;
};

struct NV_GPU_CTX
{
	NvAPI_Status Result;
	NvU32 GpuCount;
	NvPhysicalGpuHandle GpuHandle[NVAPI_MAX_PHYSICAL_GPUS];
	struct NV_GPU_DATA* List;
};

static void* nv_gpu_init(PNWLIB_GPU_INFO info)
{
	struct NV_GPU_CTX* ctx = calloc(1, sizeof(struct NV_GPU_CTX));
	if (ctx == NULL)
		return NULL;

	ctx->Result = NvAPI_Initialize();
	if (ctx->Result != NVAPI_OK)
	{
		GPU_DBG(NVDL, "NvAPI_Initialize failed %d", ctx->Result);
		free(ctx);
		return NULL;
	}
	GPU_DBG(NVDL, "NvAPI Initialized");

	ctx->Result = NvAPI_EnumPhysicalGPUs(ctx->GpuHandle, &ctx->GpuCount);
	if (ctx->Result != NVAPI_OK)
		goto fail;
	GPU_DBG(NVDL, "Found %lu devices", (unsigned long)ctx->GpuCount);

	ctx->List = calloc(ctx->GpuCount, sizeof(struct NV_GPU_DATA));
	if (ctx->List == NULL)
		goto fail;
	for (NvU32 i = 0; i < ctx->GpuCount; i++)
	{
		NvAPI_GPU_GetFullName(ctx->GpuHandle[i], ctx->List[i].Name);
		NvAPI_GPU_GetPCIIdentifiers(ctx->GpuHandle[i],
			&ctx->List[i].DeviceId, &ctx->List[i].Subsys, &ctx->List[i].RevId, &ctx->List[i].ExtDeviceId);
		ctx->List[i].VendorId = ctx->List[i].DeviceId & 0xFFFF;
		GPU_DBG(NVDL, "Found [%d] %s (%08X-%04X-REV_%02X-EXT_%X)", i, ctx->List[i].Name,
			ctx->List[i].DeviceId, ctx->List[i].Subsys, ctx->List[i].RevId, ctx->List[i].ExtDeviceId);
		if (ctx->List[i].Name[0] != '\0' && ctx->List[i].VendorId == 0x10DE)
			ctx->List[i].Valid = NV_TRUE;
		NvAPI_GPU_GetBusId(ctx->GpuHandle[i], &ctx->List[i].BusId);
		NvAPI_GPU_GetBusSlotId(ctx->GpuHandle[i], &ctx->List[i].SlotId);
		GPU_DBG(NVDL, "Valid GPU [%d] %04X:%04X Bus %X Slot %X", i,
			ctx->List[i].VendorId, ctx->List[i].ExtDeviceId, ctx->List[i].BusId, ctx->List[i].SlotId);

		ctx->List[i].MemEx.version = NV_GPU_MEMORY_INFO_EX_VER;
		// ctx->List[i].Mem -> 2, 3
		ctx->List[i].Mem.version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER;

		ctx->List[i].Pstates.version = NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER;
		ctx->List[i].Usages.version = NV_USAGES_VER;
		ctx->List[i].Power.version = NV_POWER_TOPOLOGY_VER;
		// ctx->List[i].Clock -> 1, 2, 3
		ctx->List[i].Clock.ClockType = NV_GPU_CLOCK_FREQUENCIES_CURRENT_FREQ;
		// ctx->List[i].Thermal -> 1, 2
		ctx->List[i].ThermalSensors.Version = NV_THERMAL_SENSORS_VER;
		ctx->List[i].FanStatus.Version = NV_FAN_COOLER_STATUS_VER;
	}
	return ctx;

fail:
	GPU_DBG(NVDL, "INIT ERR %d", ctx->Result);
	NvAPI_Unload();
	return NULL;
}

static NvAPI_Status
get_legacy_mem_info(NvPhysicalGpuHandle gpu, NV_DISPLAY_DRIVER_MEMORY_INFO* mem)
{
	mem->version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER_3;
	if (NvAPI_GPU_GetMemoryInfo(gpu, mem) == NVAPI_OK)
		return NVAPI_OK;
	mem->version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER_2;
	return NvAPI_GPU_GetMemoryInfo(gpu, mem);
}

static NvU32
get_khz_frequency(NvPhysicalGpuHandle gpu, NV_GPU_CLOCK_FREQUENCIES* clock)
{
	for (NvU64 ver = 1; ver <= 3; ver++)
	{
		clock->version = MAKE_NVAPI_VERSION(NV_GPU_CLOCK_FREQUENCIES, ver);
		if (NvAPI_GPU_GetAllClockFrequencies(gpu, clock) != NVAPI_OK)
			continue;
		for (int i = 0; i < NVAPI_MAX_GPU_PUBLIC_CLOCKS; i++)
		{
			if (clock->domain[i].bIsPresent)
				return clock->domain[i].frequency;
		}
	}
	return 0;
}

static inline NvS32
calc_pstate20_uv_voltage(NV_GPU_PERF_PSTATES20_INFO* pstates20, NV_GPU_PERF_PSTATE_ID cur_pstate)
{
	NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1* volt = NULL;
	if (pstates20->numPstates > NVAPI_MAX_GPU_PSTATE20_PSTATES)
		pstates20->numPstates = NVAPI_MAX_GPU_PSTATE20_PSTATES;
	for (NvU32 i = 0; i < pstates20->numPstates; i++)
	{
		volt = &pstates20->pstates[i].baseVoltages[0];
		if (pstates20->pstates[i].pstateId == cur_pstate)
			break;
	}
	if (volt == NULL)
		return 0;
	return ((NvS32)volt->volt_uV + volt->voltDelta_uV.value);
}

static inline NvS32
calc_pstatel_uv_voltage(NV_GPU_PERF_PSTATES_INFO* pstatesl, NV_GPU_PERF_PSTATE_ID cur_pstate)
{
	NvU32 mv = 0;
	if (pstatesl->numPstates > NVAPI_MAX_GPU_PERF_PSTATES)
		pstatesl->numPstates = NVAPI_MAX_GPU_PERF_PSTATES;
	for (NvU32 i = 0; i < pstatesl->numPstates; i++)
	{
		mv = pstatesl->pstates[i].voltages[0].mvolt;
		if (pstatesl->pstates[i].pstateId == cur_pstate)
			break;
	}
	return (NvS32)(mv * 1000);
}

static NvS32
get_uv_voltage(NvPhysicalGpuHandle gpu, struct NV_GPU_DATA* data)
{
	if (NvAPI_GPU_GetCurrentPstate(gpu, &data->CurPstate) != NVAPI_OK)
		return 0;
	data->Pstates20.version = NV_GPU_PERF_PSTATES20_INFO_VER;
	if (NvAPI_GPU_GetPstates20(gpu, &data->Pstates20) == NVAPI_OK)
		return calc_pstate20_uv_voltage(&data->Pstates20, data->CurPstate);
	data->Pstates20.version = NV_GPU_PERF_PSTATES20_INFO_VER2;
	if (NvAPI_GPU_GetPstates20(gpu, &data->Pstates20) == NVAPI_OK)
		return calc_pstate20_uv_voltage(&data->Pstates20, data->CurPstate);
	data->Pstates20.version = NV_GPU_PERF_PSTATES20_INFO_VER1;
	if (NvAPI_GPU_GetPstates20(gpu, &data->Pstates20) == NVAPI_OK)
		return calc_pstate20_uv_voltage(&data->Pstates20, data->CurPstate);
	data->PstatesL.version = NV_GPU_PERF_PSTATES_INFO_VER;
	if (NvAPI_GPU_GetPstatesInfoEx(gpu, &data->PstatesL, 0) == NVAPI_OK)
		return calc_pstatel_uv_voltage(&data->PstatesL, data->CurPstate);
	data->PstatesL.version = NV_GPU_PERF_PSTATES_INFO_VER2;
	if (NvAPI_GPU_GetPstatesInfoEx(gpu, &data->PstatesL, 0) == NVAPI_OK)
		return calc_pstatel_uv_voltage(&data->PstatesL, data->CurPstate);
	return 0;
}

static NvU32
get_usage_percent(NvPhysicalGpuHandle gpu, NV_GPU_DYNAMIC_PSTATES_INFO_EX* pstates, NV_USAGES* usages)
{
	if (NvAPI_GPU_GetDynamicPstatesInfoEx(gpu, pstates) != NVAPI_OK)
		pstates->flags = 0;
	if (pstates->flags & 0x01)
	{
		for (int i = 0; i < NVAPI_MAX_GPU_UTILIZATIONS; i++)
		{
			if (pstates->utilization[i].bIsPresent)
				return pstates->utilization[i].percentage;
		}
	}
	if (NvAPI_GPU_GetUsages(gpu, usages) == NVAPI_OK)
	{
		for (int i = 0; i < NVAPI_MAX_GPU_UTILIZATIONS; i++)
		{
			if (usages->Entries[i].IsPresent)
				return usages->Entries[i].Percentage;
		}
	}
	return 0;
}

static NvU32
get_mw_power(NvPhysicalGpuHandle gpu, NV_POWER_TOPOLOGY* power)
{
	if (NvAPI_GPU_ClientPowerTopologyGetStatus(gpu, power) != NVAPI_OK)
		return 0;
	if (power->Count > NVAPI_MAX_POWER_TOPOLOGIES)
		power->Count = NVAPI_MAX_POWER_TOPOLOGIES;
	for (NvU32 i = 0; i < power->Count; i++)
	{
		if (power->Entries[i].Domain == NV_POWER_DOMAIN_BOARD)
			return power->Entries[i].PowerUsage;
	}
	return power->Entries[0].PowerUsage;
}

static double
get_temperature(NvPhysicalGpuHandle gpu, struct NV_GPU_DATA* data)
{
	data->ThermalV2.version = NV_GPU_THERMAL_SETTINGS_VER_2;
	if (NvAPI_GPU_GetThermalSettings(gpu, NVAPI_THERMAL_TARGET_ALL, &data->ThermalV2) == NVAPI_OK)
	{
		if (data->ThermalV2.count > NVAPI_MAX_THERMAL_SENSORS_PER_GPU)
			data->ThermalV2.count = NVAPI_MAX_THERMAL_SENSORS_PER_GPU;
		for (NvU32 i = 0; i < data->ThermalV2.count; i++)
		{
			if (data->ThermalV2.sensor[i].target == NVAPI_THERMAL_TARGET_GPU)
				return (double)data->ThermalV2.sensor[i].currentTemp;
		}
		return (double)data->ThermalV2.sensor[0].currentTemp;
	}
	data->ThermalV1.version = NV_GPU_THERMAL_SETTINGS_VER_1;
	if (NvAPI_GPU_GetThermalSettings(gpu, NVAPI_THERMAL_TARGET_ALL, &data->ThermalV2) == NVAPI_OK)
	{
		if (data->ThermalV1.count > NVAPI_MAX_THERMAL_SENSORS_PER_GPU)
			data->ThermalV1.count = NVAPI_MAX_THERMAL_SENSORS_PER_GPU;
		for (NvU32 i = 0; i < data->ThermalV1.count; i++)
		{
			if (data->ThermalV1.sensor[i].target == NVAPI_THERMAL_TARGET_GPU)
				return (double)data->ThermalV1.sensor[i].currentTemp;
		}
		return (double)data->ThermalV1.sensor[0].currentTemp;
	}
	for (NvU32 bit = 0; bit < 32; bit++)
	{
		data->ThermalSensors.Mask = 1U << bit;
		if (NvAPI_GPU_GetThermalSensors(gpu, &data->ThermalSensors) != NVAPI_OK)
			continue;
		return (double)data->ThermalSensors.Temperatures[1] / 256.0;
	}
	return 0.0;
}

static NvU32
get_rpm_fan(NvPhysicalGpuHandle gpu, struct NV_GPU_DATA* data)
{
	NvU32 speed = 0;
	if (NvAPI_GPU_GetTachReading(gpu, &speed) == NVAPI_OK)
		return speed;
	if (NvAPI_GPU_ClientFanCoolersGetStatus(gpu, &data->FanStatus) == NVAPI_OK)
	{
		if (data->FanStatus.Count > NVAPI_MAX_FAN_COOLERS_STATUS_ITEMS)
			data->FanStatus.Count = NVAPI_MAX_FAN_COOLERS_STATUS_ITEMS;
		for (NvU32 i = 0; i < data->FanStatus.Count; i++)
		{
			if (data->FanStatus.Items[i].CurrentRpm)
				return data->FanStatus.Items[i].CurrentRpm;
		}
	}
	return 0;
}

static uint32_t nv_gpu_get(void* data, NWLIB_GPU_DEV* dev, uint32_t dev_count)
{
	uint32_t count = 0;
	struct NV_GPU_CTX* ctx = data;

	if (ctx == NULL || ctx->GpuCount == 0)
		return 0;

	for (NvU32 i = 0; i < ctx->GpuCount; i++)
	{
		if (count >= dev_count)
			break;
		if (ctx->List[i].Valid == FALSE)
			continue;

		NWLIB_GPU_DEV* info = &dev[count];
		count++;

		strcpy_s(info->Name, MAX_GPU_STR, ctx->List[i].Name);
		info->VendorId = (uint32_t)ctx->List[i].VendorId;
		info->DeviceId = (uint32_t)ctx->List[i].ExtDeviceId;
		info->Subsys = (uint32_t)ctx->List[i].Subsys;
		info->RevId = (uint32_t)ctx->List[i].RevId;
		info->PciBus = (uint32_t)ctx->List[i].BusId;
		info->PciDevice = (uint32_t)ctx->List[i].SlotId;
		info->PciFunction = 0; // ?

		if (NvAPI_GPU_GetMemoryInfoEx(ctx->GpuHandle[i], &ctx->List[i].MemEx) == NVAPI_OK)
		{
			info->TotalMemory = ctx->List[i].MemEx.dedicatedVideoMemory;
			info->FreeMemory = ctx->List[i].MemEx.curAvailableDedicatedVideoMemory;
		}
		else if (get_legacy_mem_info(ctx->GpuHandle[i], &ctx->List[i].Mem) == NVAPI_OK)
		{
			// KiB to Bytes
			info->TotalMemory = ctx->List[i].Mem.dedicatedVideoMemory * 1024ULL;
			info->FreeMemory = ctx->List[i].Mem.curAvailableDedicatedVideoMemory * 1024ULL;
		}
		if (info->FreeMemory < info->TotalMemory)
		{
			info->UsedMemory = info->TotalMemory - info->FreeMemory;
			info->MemoryPercent = 100ULL * info->UsedMemory / info->TotalMemory;
		}

		info->UsagePercent = (double)get_usage_percent(ctx->GpuHandle[i], &ctx->List[i].Pstates, &ctx->List[i].Usages);

		info->Power = (double)get_mw_power(ctx->GpuHandle[i], &ctx->List[i].Power) * 0.001;

		info->Frequency = (double)get_khz_frequency(ctx->GpuHandle[i], &ctx->List[i].Clock) * 0.001;

		info->Voltage = (double)get_uv_voltage(ctx->GpuHandle[i], &ctx->List[i]) * 0.000001;

		info->Temperature = get_temperature(ctx->GpuHandle[i], &ctx->List[i]);

		info->FanSpeed = get_rpm_fan(ctx->GpuHandle[i], &ctx->List[i]);
	}

	return count;

	return 0;
}

static void nv_gpu_free(void* data)
{
	struct NV_GPU_CTX* ctx = data;
	if (ctx == NULL)
		return;
	free(ctx->List);
	NvAPI_Unload();
}

NWLIB_GPU_DRV gpu_drv_nvidia =
{
	.Name = NVDL,
	.Init = nv_gpu_init,
	.GetInfo = nv_gpu_get,
	.Free = nv_gpu_free,
};

