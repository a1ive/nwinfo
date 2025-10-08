// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include "igcl.h"
#include "gpu.h"

#define IGCL "IGCL"

#define MAX_MEM_HANDLE 8

struct INTEL_GPU_DATA
{
	BOOL Valid;
	ctl_device_adapter_properties_t Props;
	LUID Luid;
	ctl_pci_properties_t Pci;
	double TimeStamp;
	uint32_t MemoryCount;
	ctl_mem_handle_t Memory[MAX_MEM_HANDLE];
};

struct INTEL_GPU_CTX
{
	ctl_init_args_t InitArgs;
	ctl_api_handle_t ApiHandle;
	ctl_result_t Result;
	uint32_t AdapterCount;
	ctl_power_telemetry_t Pt;
	ctl_device_adapter_handle_t* Devices;
	struct INTEL_GPU_DATA* List;
};

static void* igcl_gpu_init(void)
{
	struct INTEL_GPU_CTX* ctx = calloc(1, sizeof(struct INTEL_GPU_CTX));
	if (ctx == NULL)
		return NULL;

	ctx->InitArgs.AppVersion = CTL_MAKE_VERSION(CTL_IMPL_MAJOR_VERSION, CTL_IMPL_MINOR_VERSION);
	ctx->InitArgs.flags = CTL_INIT_FLAG_USE_LEVEL_ZERO;
	ctx->InitArgs.Size = sizeof(ctl_init_args_t);
	ctx->InitArgs.Version = 0;
	memset(&ctx->InitArgs.ApplicationUID, 0, sizeof(ctl_application_id_t));

	ctx->Result = ctlInit(&ctx->InitArgs, &ctx->ApiHandle);
	if (ctx->Result != CTL_RESULT_SUCCESS)
		goto fail;

	ctx->Result = ctlEnumerateDevices(ctx->ApiHandle, &ctx->AdapterCount, NULL);
	if (ctx->Result != CTL_RESULT_SUCCESS)
		goto fail;
	GPU_DBG(IGCL, "Found %u devices", ctx->AdapterCount);

	ctx->Devices = calloc(ctx->AdapterCount, sizeof(ctl_device_adapter_handle_t));
	if (ctx->Devices == NULL)
		goto fail;

	ctx->Result = ctlEnumerateDevices(ctx->ApiHandle, &ctx->AdapterCount, ctx->Devices);
	if (ctx->Result != CTL_RESULT_SUCCESS)
		goto fail;

	ctx->List = calloc(ctx->AdapterCount, sizeof(struct INTEL_GPU_DATA));
	if (ctx->List == NULL)
		goto fail;

	for (uint32_t i = 0; i < ctx->AdapterCount; i++)
	{
		ctx->List[i].Props.pDeviceID = &ctx->List[i].Luid;
		ctx->List[i].Props.Size = sizeof(ctl_device_adapter_properties_t);
		ctx->List[i].Props.device_id_size = sizeof(LUID);
		ctx->List[i].Props.Version = 2;
		ctx->Result = ctlGetDeviceProperties(ctx->Devices[i], &ctx->List[i].Props);
		GPU_DBG(IGCL, "GetDeviceProperties [%u] %X", i, ctx->Result);
		if (ctx->Result != CTL_RESULT_SUCCESS)
			continue;

		if (ctx->List[i].Props.device_type != CTL_DEVICE_TYPE_GRAPHICS
			|| ctx->List[i].Props.pci_vendor_id != 0x8086)
		{
			GPU_DBG(IGCL, "Skip [%u] TYPE %u VID %04X", i, ctx->List[i].Props.device_type, ctx->List[i].Props.pci_vendor_id);
			continue;
		}

		ctx->List[i].Valid = TRUE;
		GPU_DBG(IGCL, "Found Intel GPU %s (%04X-%04X)",
			ctx->List[i].Props.name, ctx->List[i].Props.pci_vendor_id, ctx->List[i].Props.pci_device_id);

		ctx->List[i].Pci.Size = sizeof(ctl_pci_properties_t);
		ctx->Result = ctlPciGetProperties(ctx->Devices[i], &ctx->List[i].Pci);
		GPU_DBG(IGCL, "BDF %u:%u:%u",
			ctx->List[i].Pci.address.bus, ctx->List[i].Pci.address.device, ctx->List[i].Pci.address.function);

		ctx->Result = ctlEnumMemoryModules(ctx->Devices[i], &ctx->List[i].MemoryCount, NULL);
		GPU_DBG(IGCL, "%u memory handle(s)", ctx->List[i].MemoryCount);
		if (ctx->List[i].MemoryCount > MAX_MEM_HANDLE)
			ctx->List[i].MemoryCount = MAX_MEM_HANDLE;
	}
	return ctx;

fail:
	GPU_DBG(IGCL, "INIT ERR %x", ctx->Result);
	free(ctx->List);
	free(ctx->Devices);
	if (ctx->ApiHandle)
		ctlClose(ctx->ApiHandle);
	free(ctx);
	return NULL;
}

static uint32_t igcl_gpu_get(void* data, NWLIB_GPU_DEV* dev, uint32_t dev_count)
{
	uint32_t count = 0;
	struct INTEL_GPU_CTX* ctx = data;

	if (ctx == NULL || ctx->AdapterCount == 0)
		return 0;

	ctx->Pt.Size = sizeof(ctl_power_telemetry_t);
	ctx->Pt.Version = 1;

	for (uint32_t i = 0; i < ctx->AdapterCount; i++)
	{
		if (count >= dev_count)
			break;
		if (ctx->List[count].Valid == FALSE)
			continue;

		NWLIB_GPU_DEV* info = &dev[count];
		count++;

		strcpy_s(info->Name, MAX_GPU_STR, ctx->List[i].Props.name);
		info->VendorId = ctx->List[i].Props.pci_vendor_id;
		info->DeviceId = ctx->List[i].Props.pci_device_id;
		info->Subsys = ctx->List[i].Props.pci_subsys_id;
		info->RevId = ctx->List[i].Props.rev_id;
		info->PciBus = ctx->List[i].Pci.address.bus;
		info->PciDevice = ctx->List[i].Pci.address.device;
		info->PciFunction = ctx->List[i].Pci.address.function;

		ctx->Result = ctlPowerTelemetryGet(ctx->Devices[i], &ctx->Pt);
		if (ctx->Result != CTL_RESULT_SUCCESS)
			continue;

		if (ctx->Pt.gpuCurrentTemperature.bSupported)
			info->Temperature = ctx->Pt.gpuCurrentTemperature.value.datadouble;
		else if (ctx->Pt.gpuVrTemp.bSupported)
			info->Temperature = ctx->Pt.gpuVrTemp.value.datadouble;

		double time_stamp = ctx->List[i].TimeStamp;
		double energy_counter = info->Energy;
		double usage_counter = info->UsageCounter;
		if (ctx->Pt.timeStamp.bSupported)
			time_stamp = ctx->Pt.timeStamp.value.datadouble;
		if (ctx->Pt.totalCardEnergyCounter.bSupported)
			energy_counter = ctx->Pt.totalCardEnergyCounter.value.datadouble;
		else if (ctx->Pt.gpuEnergyCounter.bSupported)
			energy_counter = ctx->Pt.gpuEnergyCounter.value.datadouble;
		if (ctx->Pt.globalActivityCounter.bSupported)
			usage_counter = ctx->Pt.globalActivityCounter.value.datadouble;
		double time_diff = time_stamp - ctx->List[i].TimeStamp;
		double energy_diff = energy_counter - info->Energy;
		double usage_diff = usage_counter - info->UsageCounter;
		if (time_diff > 0.0)
		{
			if (energy_diff > 0.0)
				info->Power = energy_diff / time_diff;
			if (usage_diff > 0.0)
				info->UsagePercent = usage_diff / time_diff * 100.0;
		}
		ctx->List[i].TimeStamp = time_stamp;
		info->Energy = energy_counter;
		info->UsageCounter = usage_counter;

		if (ctx->Pt.gpuVoltage.bSupported)
			info->Voltage = ctx->Pt.gpuVoltage.value.datadouble;

		if (ctx->Pt.gpuCurrentClockFrequency.bSupported)
			info->Frequency = ctx->Pt.gpuCurrentClockFrequency.value.datadouble;
		else
			info->Frequency = ctx->List[i].Props.Frequency;

		for (int j = 0; j < CTL_FAN_COUNT; j++)
		{
			if (ctx->Pt.fanSpeed[j].bSupported && ctx->Pt.fanSpeed[j].value.datadouble > 0.0f)
			{
				info->FanSpeed = (uint64_t)ctx->Pt.fanSpeed[j].value.datadouble;
				break;
			}
		}

		ctx->Result = ctlEnumMemoryModules(ctx->Devices[i], &ctx->List[i].MemoryCount, ctx->List[i].Memory);
		if (ctx->Result == CTL_RESULT_SUCCESS)
		{
			ctl_mem_state_t state = { 0 };
			state.Size = sizeof(ctl_mem_state_t);
			info->TotalMemory = 0;
			info->FreeMemory = 0;
			for (uint32_t j = 0; j < ctx->List[i].MemoryCount; j++)
			{
				ctx->Result = ctlMemoryGetState(ctx->List[i].Memory[j], &state);
				if (ctx->Result != CTL_RESULT_SUCCESS)
					continue;
				info->FreeMemory += state.free;
				info->TotalMemory += state.size;
			}
		}
	}

	return count;
}

static void igcl_gpu_free(void* data)
{
	struct INTEL_GPU_CTX* ctx = data;
	if (ctx == NULL)
		return;
	free(ctx->List);
	free(ctx->Devices);
	ctlClose(ctx->ApiHandle);
	free(ctx);
}

NWLIB_GPU_DRV gpu_drv_intel =
{
	.Name = IGCL,
	.Init = igcl_gpu_init,
	.GetInfo = igcl_gpu_get,
	.Free = igcl_gpu_free,
};

