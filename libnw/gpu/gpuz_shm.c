// SPDX-License-Identifier: Unlicense

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <shmem.h>
#include "gpu.h"
#include "../utils.h"
#include "../libnw.h"

#define GPUZ_SHMEM_NAME L"GPUZShMem"
#define GPUZ_MAX_RECORDS 128
#define GPUZ_STR_LEN 256
#define GPUZ_UNIT_LEN 8

#pragma pack(push, 1)
typedef struct
{
	WCHAR key[GPUZ_STR_LEN];
	WCHAR value[GPUZ_STR_LEN];
} GPUZ_RECORD;

typedef struct
{
	WCHAR name[GPUZ_STR_LEN];
	WCHAR unit[GPUZ_UNIT_LEN];
	UINT32 digits;
	double value;
} GPUZ_SENSOR_RECORD;

typedef struct
{
	UINT32 version; // 1
	volatile LONG busy; // Is data being accessed
	UINT32 lastUpdate; // GetTickCount() of last update
	GPUZ_RECORD data[GPUZ_MAX_RECORDS];
	GPUZ_SENSOR_RECORD sensors[GPUZ_MAX_RECORDS];
} GPUZ_SH_MEM;
#pragma pack(pop)

#define GPUZSHM "GPUZ"

struct GPUZ_SHM_CTX
{
	struct wr0_shmem_t ShMem;
	GPUZ_SH_MEM* Data;
	int Result;
	CHAR Name[MAX_GPU_STR];
	uint32_t IGP;
	uint32_t VendorId;
	uint32_t DeviceId;
	uint32_t Subsys;
	uint32_t RevId;
	uint32_t PciBus;
	uint32_t PciDevice;
	uint32_t PciFunction;
	NWLIB_GPU_DEV* Device;
};

static NWLIB_GPU_DEV*
search_gpu_device(PNWLIB_GPU_INFO info, struct GPUZ_SHM_CTX* ctx)
{
	for (uint32_t i = 0; i < info->DeviceCount; i++)
	{
		NWLIB_GPU_DEV* dev = &info->Device[i];
		if (dev->PciBus == ctx->PciBus &&
			dev->PciDevice == ctx->PciDevice &&
			dev->PciFunction == ctx->PciFunction)
		{
			NWL_Debug(GPUZSHM, "Matched by PCI BDF %u:%u:%u -> %s",
				ctx->PciBus, ctx->PciDevice, ctx->PciFunction, dev->Name);
			return dev;
		}
	}
	return NULL;
}

static void* gpuz_shm_init(PNWLIB_GPU_INFO info)
{
	struct GPUZ_SHM_CTX* ctx = calloc(1, sizeof(struct GPUZ_SHM_CTX));
	if (ctx == NULL)
		return NULL;

	ctx->Result = WR0_OpenShMem(&ctx->ShMem, GPUZ_SHMEM_NAME);
	if (ctx->Result != 0)
		goto fail;
	NWL_Debug(GPUZSHM, "Opened shared memory %s, size=%zu", NWL_Ucs2ToUtf8(GPUZ_SHMEM_NAME), ctx->ShMem.size);
	if (ctx->ShMem.size < sizeof(GPUZ_SH_MEM))
	{
		NWL_Debug(GPUZSHM, "Shared memory size too small %zu < %zu", ctx->ShMem.size, sizeof(GPUZ_SH_MEM));
		goto fail;
	}

	ctx->Data = (GPUZ_SH_MEM*)ctx->ShMem.addr;
	NWL_Debug(GPUZSHM, "GPU-Z SHM version=%u, lastUpdate=%u", ctx->Data->version, ctx->Data->lastUpdate);
	for (UINT i = 0; i < GPUZ_MAX_RECORDS; i++)
	{
		CHAR value[GPUZ_STR_LEN];
		GPUZ_RECORD* rec = &ctx->Data->data[i];
		if (rec->key[0] == L'\0')
			continue;
		strcpy_s(value, GPUZ_STR_LEN, NWL_Ucs2ToUtf8(rec->value));
		NWL_Debug(GPUZSHM, "DATA[%u]: %s = %s", i, NWL_Ucs2ToUtf8(rec->key), value);
		if (wcscmp(rec->key, L"CardName") == 0)
			strcpy_s(ctx->Name, MAX_GPU_STR, value);
		else if (wcscmp(rec->key, L"IGP") == 0)
			ctx->IGP = (uint32_t)strtoul(value, NULL, 10);
		else if (wcscmp(rec->key, L"VendorID") == 0)
			ctx->VendorId = (uint32_t)strtoul(value, NULL, 16);
		else if (wcscmp(rec->key, L"DeviceID") == 0)
			ctx->DeviceId = (uint32_t)strtoul(value, NULL, 16);
		else if (wcscmp(rec->key, L"SubsysID") == 0)
			ctx->Subsys |= (uint32_t)strtoul(value, NULL, 16) << 16;
		else if (wcscmp(rec->key, L"SubvendorID") == 0)
			ctx->Subsys |= (uint32_t)strtoul(value, NULL, 16) & 0xFFFF;
		else if (wcscmp(rec->key, L"RevisionID") == 0)
			ctx->RevId = (uint32_t)strtoul(value, NULL, 16);
		else if (wcscmp(rec->key, L"Bus") == 0)
			ctx->PciBus = (uint32_t)strtoul(value, NULL, 10);
		else if (wcscmp(rec->key, L"Dev") == 0)
			ctx->PciDevice = (uint32_t)strtoul(value, NULL, 10);
		else if (wcscmp(rec->key, L"Fn") == 0)
			ctx->PciFunction = (uint32_t)strtoul(value, NULL, 10);
	}
	NWL_Debug(GPUZSHM, "GPUZ GPU: %s [%04X-%04X SUBSYS %08X REV %02X] Bus %u, Device %u, Function %u",
		ctx->Name, ctx->VendorId, ctx->DeviceId, ctx->Subsys, ctx->RevId,
		ctx->PciBus, ctx->PciDevice, ctx->PciFunction);
	ctx->Device = search_gpu_device(info, ctx);
	if (ctx->Device == NULL)
	{
		NWL_Debug(GPUZSHM, "Cannot find matching GPU device");
		goto fail;
	}

	for (UINT i = 0; i < GPUZ_MAX_RECORDS; i++)
	{
		GPUZ_SENSOR_RECORD* rec = &ctx->Data->sensors[i];
		if (rec->name[0] == L'\0')
			continue;
		NWL_Debug(GPUZSHM, "SENSOR[%u]: %s = %.2f", i, NWL_Ucs2ToUtf8(rec->name), rec->value);
	}

	return ctx;

fail:
	NWL_Debug(GPUZSHM, "INIT ERR %d", ctx->Result);
	WR0_CloseShMem(&ctx->ShMem);
	free(ctx);
	return NULL;
}

static inline double
get_gpuz_temperature(GPUZ_SENSOR_RECORD* rec)
{
	if (rec->unit[1] == L'F')
		return (rec->value - 32.0) * 5.0 / 9.0; // Fahrenheit to Celsius
	else
		return rec->value;
}

static uint32_t gpuz_shm_get(void* data, NWLIB_GPU_DEV* dev, uint32_t dev_count)
{
	uint32_t count = 0;
	struct GPUZ_SHM_CTX* ctx = data;

	if (ctx == NULL)
		return 0;

	for (UINT i = 0; i < GPUZ_MAX_RECORDS; i++)
	{
		GPUZ_SENSOR_RECORD* rec = &ctx->Data->sensors[i];
		if (rec->name[0] == L'\0')
			continue;
		if (ctx->Device->Frequency == 0.0 && wcscmp(rec->name, L"GPU Clock") == 0)
			ctx->Device->Frequency = rec->value;
		else if (ctx->Device->Temperature == 0.0 && wcscmp(rec->name, L"GPU Temperature") == 0)
			ctx->Device->Temperature = get_gpuz_temperature(rec);
		else if (ctx->Device->Temperature == 0.0 && wcscmp(rec->name, L"Hot Spot") == 0)
			ctx->Device->Temperature = get_gpuz_temperature(rec);
		else if (ctx->Device->FanSpeed == 0 && wcscmp(rec->name, L"Fan Speed (RPM)") == 0)
			ctx->Device->FanSpeed = (uint64_t)rec->value;
		else if (wcscmp(rec->name, L"GPU Load") == 0)
			ctx->Device->UsagePercent = rec->value;
		else if (wcscmp(rec->name, L"Memory Controller Load") == 0)
			ctx->Device->MemoryPercent = (uint64_t)rec->value;
		else if (ctx->Device->Power == 0.0 && wcscmp(rec->name, L"GPU Power") == 0)
			ctx->Device->Power = rec->value;
		else if (ctx->Device->Power == 0.0 && wcscmp(rec->name, L"GPU Power Draw") == 0)
			ctx->Device->Power = rec->value;
		else if (ctx->Device->Power == 0.0 && wcscmp(rec->name, L"GPU Chip Power Draw") == 0)
			ctx->Device->Power = rec->value;
		else if (ctx->Device->Power == 0.0 && wcscmp(rec->name, L"Board Power Draw") == 0)
			ctx->Device->Power = rec->value;
		else if (ctx->Device->Voltage == 0.0 && wcscmp(rec->name, L"GPU Voltage") == 0)
			ctx->Device->Voltage = rec->value;
		else if (ctx->Device->Voltage == 0.0 && wcscmp(rec->name, L"VDDC") == 0)
			ctx->Device->Voltage = rec->value;
		else if (ctx->IGP && ctx->Device->Temperature == 0.0 && wcscmp(rec->name, L"CPU Temperature") == 0)
			ctx->Device->Temperature = get_gpuz_temperature(rec);
	}

	return 0;
}

static void gpuz_shm_free(void* data)
{
	struct GPUZ_SHM_CTX* ctx = data;
	if (ctx == NULL)
		return;
	WR0_CloseShMem(&ctx->ShMem);
	free(ctx);
}

NWLIB_GPU_DRV gpu_drv_gpuz =
{
	.Name = GPUZSHM,
	.Init = gpuz_shm_init,
	.GetInfo = gpuz_shm_get,
	.Free = gpuz_shm_free,
};
