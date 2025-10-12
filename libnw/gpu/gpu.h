// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdint.h>

#define MAX_GPU_STR 256

typedef struct _NWLIB_GPU_DEV
{
	char Name[MAX_GPU_STR];
	//char HwId[MAX_GPU_STR];
	uint32_t VendorId;
	uint32_t DeviceId;
	uint32_t Subsys;
	uint32_t RevId;
	uint32_t PciBus;
	uint32_t PciDevice;
	uint32_t PciFunction;
	uint64_t TotalMemory;
	uint64_t FreeMemory;
	uint64_t MemoryPercent;
	double UsageCounter;
	double UsagePercent;
	double Energy;
	double Power;
	double Frequency; // MHz
	double Voltage;
	double Temperature;
	uint64_t FanSpeed;
} NWLIB_GPU_DEV;

typedef struct _NWLIB_GPU_DRV
{
	const char* Name;
	void* (*Init)(void);
	uint32_t(*GetInfo)(void* data, NWLIB_GPU_DEV* dev, uint32_t dev_count);
	void (*Free)(void* data);
	void* Data;
} NWLIB_GPU_DRV;

#define NWL_GPU_MAX_COUNT 8

enum _NWLIB_GPU_DRV_TYPE
{
	NWLIB_GPU_DRV_INTEL = 0,
	NWLIB_GPU_DRV_AMD,
	NWLIB_GPU_DRV_NVIDIA,
	NWLIB_GPU_DRV_COUNT
};

typedef struct _NODE NODE, * PNODE;

typedef struct _NWLIB_GPU_INFO
{
	int Initialized;
	uint32_t DeviceCount;
	NWLIB_GPU_DEV Device[NWL_GPU_MAX_COUNT];
	NWLIB_GPU_DRV* Driver[NWLIB_GPU_DRV_COUNT];
	PNODE PciList;
} NWLIB_GPU_INFO, * PNWLIB_GPU_INFO;

VOID NWL_InitGpu(PNWLIB_GPU_INFO info);

VOID NWL_GetGpuInfo(PNWLIB_GPU_INFO info);

VOID NWL_FreeGpu(PNWLIB_GPU_INFO info);

void GPU_DBG(LPCSTR drv, LPCSTR _Printf_format_string_ format, ...);
