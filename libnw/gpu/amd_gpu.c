// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include "gpu.h"
#include "adl.h"

#define ATIADL "ADL"

typedef struct ADL_GPU_INFO
{
	AdapterInfo* AdapterInfo;
	ADLGcnInfo GcnInfo;

	uint32_t DeviceId;
	uint32_t Subsys;
	uint32_t RevId;

	int OdSupported;
	int OdApiLevel;

	int PMLogStarted;
	unsigned int PMLogDevice; // PMLog device handle
	ADLPMLogSupportInfo PMLogSupportInfo;
	ADLPMLogStartInput PMLogStartInput;
	ADLPMLogStartOutput PMLogStartOutput;
	ADLODNPerformanceStatus OdnPerf;
	ADLOD6Capabilities Od6Caps;
	ADLOD6CurrentStatus Od6Status;
	ADLODParameters Od5Params;
	ADLTemperature Od5Temp;
	ADLPMActivity Od5Activity;
	ADLFanSpeedValue Od5Fan;

	int VRamUsed; // MB
	ADLMemoryInfoX4 MemX4;
	ADLMemoryInfo2 Mem2;

	int Od8LogExists;
} ADL_GPU_INFO;

typedef struct ADL_CTX
{
	void* AdlHandle;
	int AdapterInfoCount;
	int Result;
	int GpuCount;
	AdapterInfo* AdapterInfoList;
	ADL_GPU_INFO* Devices;
} ADL_CTX;

static bool
is_pmlog_supported(const ADL_GPU_INFO* gpu, ADL_PMLOG_SENSORS sensor_type)
{
	if (!gpu->PMLogStarted || sensor_type == ADL_SENSOR_MAXTYPES)
		return false;

	for (int i = 0; i < ADL_PMLOG_MAX_SENSORS; i++)
	{
		if (gpu->PMLogSupportInfo.usSensors[i] == (unsigned short)sensor_type)
			return true;
		if (gpu->PMLogSupportInfo.usSensors[i] == (unsigned short)ADL_SENSOR_MAXTYPES)
			break;
	}
	return false;
}

static bool
get_adl_sensor(ADL_GPU_INFO* gpu, const ADLPMLogData* pmlog_data, const ADLPMLogDataOutput* od8_log,
	ADL_PMLOG_SENSORS sensor_type, double* out_value, double factor)
{
	bool support_pmlog = is_pmlog_supported(gpu, sensor_type);
	bool support_od8 = false;
	if (gpu->Od8LogExists
		&& sensor_type < ADL_PMLOG_MAX_SENSORS && sensor_type >= 0
		&& od8_log->sensors[sensor_type].supported)
		support_od8 = true;

	GPU_DBG(ATIADL, "%s sensor type %d PMLog %d OD8 %d", gpu->AdapterInfo->strAdapterName, sensor_type, support_pmlog, support_od8);

	if (support_pmlog)
	{
		if (pmlog_data && pmlog_data->ulLastUpdated != 0)
		{
			for (int k = 0; k < ADL_PMLOG_MAX_SENSORS; k++)
			{
				if (pmlog_data->ulValues[k][0] == ADL_SENSOR_MAXTYPES)
					break;
				if (pmlog_data->ulValues[k][0] == sensor_type)
				{
					*out_value = (double)pmlog_data->ulValues[k][1] * factor;
					return true;
				}
			}
		}
	}

	if (support_od8)
	{
		*out_value = (double)od8_log->sensors[sensor_type].value * factor;
		return true;
	}

	return false;
}

static void
init_pmlog(ADL_CTX* ctx, ADL_GPU_INFO* gpu, int adapter_index)
{
	if (gpu->GcnInfo.ASICFamilyId < FAMILY_AI)
		return;
	if (ADL2_Device_PMLog_Device_Create(ctx->AdlHandle, adapter_index, &gpu->PMLogDevice) != ADL_OK)
		return;
	if (ADL2_Adapter_PMLog_Support_Get(ctx->AdlHandle, adapter_index, &gpu->PMLogSupportInfo) != ADL_OK)
		goto fail;

	memset(&gpu->PMLogStartInput, 0, sizeof(ADLPMLogStartInput));
	gpu->PMLogStartInput.ulSampleRate = 1000;
	for (int k = 0; k < ADL_PMLOG_MAX_SENSORS; k++)
	{
		gpu->PMLogStartInput.usSensors[k] = gpu->PMLogSupportInfo.usSensors[k];
		if (gpu->PMLogSupportInfo.usSensors[k] == ADL_SENSOR_MAXTYPES)
			break;
	}
	if (ADL2_Adapter_PMLog_Start(ctx->AdlHandle, adapter_index, &gpu->PMLogStartInput, &gpu->PMLogStartOutput, gpu->PMLogDevice) == ADL_OK)
	{
		gpu->PMLogStarted = 1;
		GPU_DBG(ATIADL, "PMLog started");
		return;
	}

fail:
	GPU_DBG(ATIADL, "PMLog failed");
	ADL2_Device_PMLog_Device_Destroy(ctx->AdlHandle, gpu->PMLogDevice);
	gpu->PMLogDevice = 0;
}

static void
get_overdrive_version(ADL_CTX* ctx, ADL_GPU_INFO* gpu, int adapter_index)
{
	int enabled = 0;
	gpu->OdApiLevel = 0;
	gpu->OdSupported = ADL_FALSE;

	if (ADL2_Overdrive_Caps(ctx->AdlHandle, adapter_index, &gpu->OdSupported, &enabled, &gpu->OdApiLevel) == ADL_OK)
		return;

	if (ADL2_Overdrive6_Capabilities_Get(ctx->AdlHandle, adapter_index, &gpu->Od6Caps) == ADL_OK
		&& gpu->Od6Caps.iCapabilities > 0)
	{
		gpu->OdSupported = 1;
		gpu->OdApiLevel = 6;
		return;
	}

	gpu->Od5Params.iSize = sizeof(ADLODParameters);
	if (ADL2_Overdrive5_ODParameters_Get(ctx->AdlHandle, adapter_index, &gpu->Od5Params) == ADL_OK
		&& gpu->Od5Params.iActivityReportingSupported > 0)
	{
		gpu->OdSupported = 1;
		gpu->OdApiLevel = 5;
	}
}

static void
get_gpu_mem(void* adl, int adapter, ADL_GPU_INFO* gpu, NWLIB_GPU_DEV* info)
{
	info->FreeMemory = 0;
	info->TotalMemory = 0;

	if (ADL2_Adapter_MemoryInfoX4_Get(adl, adapter, &gpu->MemX4) == ADL_OK)
	{
		if (gpu->MemX4.iMemorySize > 0)
			info->TotalMemory = gpu->MemX4.iMemorySize;
	}
	else if (ADL2_Adapter_MemoryInfo2_Get(adl, adapter, &gpu->Mem2) == ADL_OK)
	{
		if (gpu->Mem2.iMemorySize > 0)
			info->TotalMemory = gpu->Mem2.iMemorySize;
	}

	if (ADL2_Adapter_DedicatedVRAMUsage_Get(adl, adapter, &gpu->VRamUsed) != ADL_OK)
		return;
	if (gpu->VRamUsed < 0)
		return;

	uint64_t used_mem = ((uint64_t)gpu->VRamUsed) * 1024 * 1024;
	if (used_mem < info->TotalMemory)
	{
		info->FreeMemory = info->TotalMemory - used_mem;
		info->MemoryPercent = used_mem * 100ULL / info->TotalMemory;
	}
}

static void adl_gpu_free(void* data)
{
	ADL_CTX* ctx = data;
	if (ctx == NULL)
		return;
	if (ctx->Devices)
	{
		for (int i = 0; i < ctx->GpuCount; i++)
		{
			ADL_GPU_INFO* gpu = &ctx->Devices[i];
			if (gpu->PMLogStarted && gpu->PMLogDevice != 0)
				ADL2_Adapter_PMLog_Stop(ctx->AdlHandle, gpu->AdapterInfo->iAdapterIndex, gpu->PMLogDevice);
			if (gpu->PMLogDevice != 0)
				ADL2_Device_PMLog_Device_Destroy(ctx->AdlHandle, gpu->PMLogDevice);
		}
	}
	if (ctx->AdlHandle)
		ADL2_Main_Control_Destroy(ctx->AdlHandle);
	free(ctx->AdapterInfoList);
	free(ctx->Devices);
	free(ctx);
}

static void* adl_gpu_init(void)
{
	ADL_CTX* ctx = calloc(1, sizeof(ADL_CTX));
	if (ctx == NULL)
		return NULL;

	ctx->Result = ADL2_Main_Control_Create(1, &ctx->AdlHandle);
	if (ctx->Result != ADL_OK)
		goto fail;
	GPU_DBG(ATIADL, "ADL2 main control created");

	ctx->Result = ADL2_Adapter_NumberOfAdapters_Get(ctx->AdlHandle, &ctx->AdapterInfoCount);
	if (ctx->Result != ADL_OK)
		goto fail;
	GPU_DBG(ATIADL, "Found %d devices", ctx->AdapterInfoCount);
	if (ctx->AdapterInfoCount <= 0)
		goto fail;

	ctx->AdapterInfoList = calloc((size_t)ctx->AdapterInfoCount, sizeof(AdapterInfo));
	if (ctx->AdapterInfoList == NULL)
		goto fail;
	ctx->Devices = calloc(ctx->AdapterInfoCount, sizeof(ADL_GPU_INFO));
	if (ctx->Devices == NULL)
		goto fail;

	for (int i = 0; i < ctx->AdapterInfoCount; i++)
		ctx->AdapterInfoList[i].iSize = sizeof(AdapterInfo);
	ctx->Result = ADL2_Adapter_AdapterInfo_Get(ctx->AdlHandle, ctx->AdapterInfoList, (size_t)ctx->AdapterInfoCount * sizeof(AdapterInfo));
	if (ctx->Result != ADL_OK)
		goto fail;
	GPU_DBG(ATIADL, "ADL2_Adapter_AdapterInfo_Get OK");

	// Correct vendor ids and skip invalid devices
	for (int i = 0; i < ctx->AdapterInfoCount; i++)
	{
		AdapterInfo* adapter = &ctx->AdapterInfoList[i];
		GPU_DBG(ATIADL, "Parsing [%d] %s (%04X)", i, adapter->strUDID, adapter->iVendorID);
		if (adapter->strUDID[0] == '\0')
			continue;
		// Fuck AMD -- ADLAdapterInfo.VendorID field reported by ADL is wrong.
		// AMD vendor id == 0x1002 (reported value is 0x3EA)
		char* id_start = strstr(adapter->strUDID, "PCI_VEN_");
		if (id_start)
		{
			id_start += sizeof("PCI_VEN_") - 1;
			adapter->iVendorID = (int)strtol(id_start, NULL, 16);
			GPU_DBG(ATIADL, "Correct VID %04X", adapter->iVendorID);
		}
		if (adapter->iVendorID != 0x1002)
			continue;

		// Remove duplicated devices
		int is_duplicate = 0;
		for (int j = 0; j < ctx->GpuCount; j++)
		{
			if (ctx->Devices[j].AdapterInfo->iBusNumber == adapter->iBusNumber &&
				ctx->Devices[j].AdapterInfo->iDeviceNumber == adapter->iDeviceNumber)
			{
				is_duplicate = 1;
				break;
			}
		}
		if (is_duplicate)
		{
			GPU_DBG(ATIADL, "GPU at Bus %d, Device %d is a duplicate. Skipping.", adapter->iBusNumber, adapter->iDeviceNumber);
			continue;
		}

		ADL_GPU_INFO* gpu = &ctx->Devices[ctx->GpuCount];
		gpu->AdapterInfo = adapter;
		GPU_DBG(ATIADL, "PNP id %s", adapter->strPNPString);

		id_start = strstr(adapter->strUDID, "&DEV_");
		if (id_start)
		{
			id_start += sizeof("&DEV_") - 1;
			gpu->DeviceId = (uint32_t)strtol(id_start, NULL, 16);
		}
		id_start = strstr(adapter->strUDID, "&SUBSYS_");
		if (id_start)
		{
			id_start += sizeof("&SUBSYS_") - 1;
			gpu->Subsys = (uint32_t)strtol(id_start, NULL, 16);
		}
		id_start = strstr(adapter->strUDID, "&REV_");
		if (id_start)
		{
			id_start += sizeof("&REV_") - 1;
			gpu->RevId = (uint32_t)strtol(id_start, NULL, 16);
		}

		GPU_DBG(ATIADL, "Initializing GPU [%d] PMLog", i);
		if (ADL2_GcnAsicInfo_Get(ctx->AdlHandle, adapter->iAdapterIndex, &gpu->GcnInfo) != ADL_OK)
			GPU_DBG(ATIADL, "ADL2_GcnAsicInfo_Get failed");
		else
		{
			GPU_DBG(ATIADL, "ADL2_GcnAsicInfo_Get ASICFamilyId %d", gpu->GcnInfo.ASICFamilyId);
			init_pmlog(ctx, gpu, adapter->iAdapterIndex);
		}

		// Get Overdrive API version
		get_overdrive_version(ctx, gpu, adapter->iAdapterIndex);
		GPU_DBG(ATIADL, "Overdrive API supported: %d, version %d", gpu->OdSupported, gpu->OdApiLevel);

		ctx->GpuCount++;
	}

	if (ctx->GpuCount <= 0)
		goto fail;

	GPU_DBG(ATIADL, "Found %d valid GPU(s)", ctx->GpuCount);
	return ctx;

fail:
	GPU_DBG(ATIADL, "ADL INIT ERR %d", ctx->Result);
	adl_gpu_free(ctx);
	return NULL;
}

static uint32_t adl_gpu_get(void* data, NWLIB_GPU_DEV* dev, uint32_t dev_count)
{
	uint32_t count = 0;
	ADL_CTX* ctx = data;

	if (ctx == NULL || ctx->GpuCount <= 0)
		return 0;

	for (int i = 0; i < ctx->GpuCount; i++)
	{
		if (count >= dev_count)
			break;
		ADL_GPU_INFO* gpu = &ctx->Devices[i];
		int adapter_index = gpu->AdapterInfo->iAdapterIndex;
		NWLIB_GPU_DEV* info = &dev[count];
		count++;

		strcpy_s(info->Name, MAX_GPU_STR, gpu->AdapterInfo->strAdapterName);
		info->VendorId = (uint32_t)gpu->AdapterInfo->iVendorID;
		info->DeviceId = gpu->DeviceId;
		info->Subsys = gpu->Subsys;
		info->RevId = gpu->RevId;
		info->PciBus = (uint32_t)gpu->AdapterInfo->iBusNumber;
		info->PciDevice = (uint32_t)gpu->AdapterInfo->iDeviceNumber;
		info->PciFunction = (uint32_t)gpu->AdapterInfo->iFunctionNumber;

		get_gpu_mem(ctx->AdlHandle, adapter_index, gpu, info);

		if (gpu->OdApiLevel >= 5)
		{
			// Core temperature
			gpu->Od5Temp.iSize = sizeof(ADLTemperature);
			if (ADL2_Overdrive5_Temperature_Get(ctx->AdlHandle, adapter_index, 0, &gpu->Od5Temp) == ADL_OK)
				info->Temperature = (double)gpu->Od5Temp.iTemperature * 0.001f;
			// Core Voltage
			gpu->Od5Activity.iSize = sizeof(ADLPMActivity);
			if (ADL2_Overdrive5_CurrentActivity_Get(ctx->AdlHandle, adapter_index, &gpu->Od5Activity) == ADL_OK)
			{
				if (gpu->Od5Activity.iVddc > 0)
					info->Voltage = (double)gpu->Od5Activity.iVddc * 0.001f;
				if (gpu->Od5Activity.iActivityPercent > 0)
					info->UsagePercent = (double)gpu->Od5Activity.iActivityPercent;
			}
			// Fan speed RPM
			gpu->Od5Fan.iSize = sizeof(ADLFanSpeedValue);
			gpu->Od5Fan.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_RPM;
			if (ADL2_Overdrive5_FanSpeed_Get(ctx->AdlHandle, adapter_index, 0, &gpu->Od5Fan) == ADL_OK)
			{
				if (gpu->Od5Fan.iFanSpeed > 0)
					info->FanSpeed = (uint64_t)gpu->Od5Fan.iFanSpeed;
			}
		}

		if (gpu->OdApiLevel >= 6)
		{
			int val_d = 0;
			if (ADL2_Overdrive6_CurrentPower_Get(ctx->AdlHandle, adapter_index, ODN_GPU_TOTAL_POWER, &val_d) == ADL_OK)
				info->Power = (double)(val_d >> 8);
			if (ADL2_Overdrive6_CurrentStatus_Get(ctx->AdlHandle, adapter_index, &gpu->Od6Status) == ADL_OK)
			{
				if (gpu->Od6Status.iActivityPercent > 0)
					info->UsagePercent = (double)gpu->Od6Status.iActivityPercent;
			}
		}

		if (gpu->OdApiLevel >= 7)
		{
			int val_d = 0;
			if (ADL2_OverdriveN_Temperature_Get(ctx->AdlHandle, adapter_index, ODN_GPU_EDGE_TEMP, &val_d) == ADL_OK
				&& val_d >= -256000 && val_d <= 256000)
				info->Temperature = (float)val_d * 0.001f;
			else if (ADL2_OverdriveN_Temperature_Get(ctx->AdlHandle, adapter_index, ODN_GPU_HOTSPOT_TEMP, &val_d) == ADL_OK
				&& val_d >= -256000 && val_d <= 256000)
				info->Temperature = (float)val_d;

			if (ADL2_OverdriveN_PerformanceStatus_Get(ctx->AdlHandle, adapter_index, &gpu->OdnPerf))
			{
				if (gpu->OdnPerf.iVDDC > 0)
					info->Voltage = (double)gpu->OdnPerf.iVDDC * 0.001f;
				if (gpu->OdnPerf.iGPUActivityPercent > 0)
					info->UsagePercent = (double)gpu->OdnPerf.iGPUActivityPercent;
				if (gpu->OdnPerf.iGFXClock > 0)
					info->Frequency = (double)gpu->OdnPerf.iGFXClock * 0.01f;
			}
		}

		if (gpu->OdApiLevel >= 8 || !gpu->OdSupported)
		{
			ADLPMLogDataOutput od8_log = { .size = sizeof(ADLPMLogDataOutput) };
			if (ADL2_New_QueryPMLogData_Get(ctx->AdlHandle, adapter_index, &od8_log) == ADL_OK)
				gpu->Od8LogExists = ADL_TRUE;
			ADLPMLogData* pmlog_data = NULL;
			if (gpu->PMLogStarted)
				pmlog_data = (ADLPMLogData*)gpu->PMLogStartOutput.pLoggingAddress;

			double val_f;

			if (get_adl_sensor(gpu, pmlog_data, &od8_log, ADL_PMLOG_TEMPERATURE_EDGE, &val_f, 1.0))
				info->Temperature = val_f;
			else if (get_adl_sensor(gpu, pmlog_data, &od8_log, ADL_PMLOG_TEMPERATURE_HOTSPOT, &val_f, 1.0))
				info->Temperature = val_f;
			else if (get_adl_sensor(gpu, pmlog_data, &od8_log, ADL_PMLOG_TEMPERATURE_VRSOC, &val_f, 1.0))
				info->Temperature = val_f;
			else if (get_adl_sensor(gpu, pmlog_data, &od8_log, ADL_PMLOG_TEMPERATURE_SOC, &val_f, 1.0))
				info->Temperature = val_f;

			if (get_adl_sensor(gpu, pmlog_data, &od8_log, ADL_PMLOG_FAN_RPM, &val_f, 1.0))
				info->FanSpeed = (uint64_t)val_f;

			if (get_adl_sensor(gpu, pmlog_data, &od8_log, ADL_PMLOG_GFX_VOLTAGE, &val_f, 0.001))
				info->Voltage = val_f;
			else if (get_adl_sensor(gpu, pmlog_data, &od8_log, ADL_PMLOG_SOC_VOLTAGE, &val_f, 0.001))
				info->Voltage = val_f;

			if (get_adl_sensor(gpu, pmlog_data, &od8_log, ADL_PMLOG_BOARD_POWER, &val_f, 1.0))
				info->Power = val_f;
			else if (get_adl_sensor(gpu, pmlog_data, &od8_log, ADL_PMLOG_GFX_POWER, &val_f, 1.0))
				info->Power = val_f;
			else if (get_adl_sensor(gpu, pmlog_data, &od8_log, ADL_PMLOG_ASIC_POWER, &val_f, 1.0))
				info->Power = val_f;
			else if (get_adl_sensor(gpu, pmlog_data, &od8_log, ADL_PMLOG_SSPAIRED_ASICPOWER, &val_f, 1.0))
				info->Power = val_f;
			else if (get_adl_sensor(gpu, pmlog_data, &od8_log, ADL_PMLOG_SOC_POWER, &val_f, 1.0))
				info->Power = val_f;

			if (get_adl_sensor(gpu, pmlog_data, &od8_log, ADL_PMLOG_CLK_GFXCLK, &val_f, 1.0))
				info->Frequency = val_f;

			if (get_adl_sensor(gpu, pmlog_data, &od8_log, ADL_PMLOG_INFO_ACTIVITY_GFX, &val_f, 1.0))
				info->UsagePercent = val_f;
		}
	}

	return count;
}

NWLIB_GPU_DRV gpu_drv_amd =
{
	.Name = ATIADL,
	.Init = adl_gpu_init,
	.GetInfo = adl_gpu_get,
	.Free = adl_gpu_free,
};

