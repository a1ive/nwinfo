// SPDX-License-Identifier: Unlicense

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <d3dkmthk.h>
#include <dxgi.h>
#include "gpu.h"
#include "../utils.h"
#include "../libnw.h"

#pragma comment(lib, "dxguid.lib")

#define GDID3D "D3DKMT"

#define MAX_D3D_ADAPTER MAX_ENUM_ADAPTERS

#define D3D_ADAPTER_NAME_LEN 256

struct D3D_GPU_DATA
{
	BOOL Valid;
	CHAR Name[D3D_ADAPTER_NAME_LEN];
	D3DKMT_OPENADAPTERFROMLUID OpenAdapter;
	D3DKMT_QUERYADAPTERINFO Qi;
	D3DKMT_DRIVER_DESCRIPTION DriverDesc;
	D3DKMT_ADAPTERREGISTRYINFO RegInfo;
	D3DKMT_ADAPTERTYPE AdapterType;
	D3DKMT_QUERY_DEVICE_IDS DeviceIds;
	D3DKMT_ADAPTERADDRESS Bdf;
	D3DKMT_SEGMENTSIZEINFO SegSize;
	D3DKMT_ADAPTER_PERFDATA PerfData;
	D3DKMT_QUERYSTATISTICS Stats;
	ULONG NbSegments;
	ULONG NodeCount;
	UINT64 TimeStamp;
	UINT64 RunningTime;
};

struct D3D_GPU_CTX
{
	HMODULE Gdi;
	NTSTATUS(APIENTRY* EnumAdapters)(D3DKMT_ENUMADAPTERS*);
	NTSTATUS(APIENTRY* QueryAdapterInfo)(const D3DKMT_QUERYADAPTERINFO*);
	NTSTATUS(APIENTRY* OpenAdapterFromLuid)(D3DKMT_OPENADAPTERFROMLUID*);
	NTSTATUS(APIENTRY* QueryStatistics)(const D3DKMT_QUERYSTATISTICS*);
	NTSTATUS(APIENTRY* CloseAdapter)(const D3DKMT_CLOSEADAPTER*);
	D3DKMT_ENUMADAPTERS Adapters;
	NTSTATUS Result;
	ULONG Count;
	struct D3D_GPU_DATA Adapter[MAX_D3D_ADAPTER];
};

static NTSTATUS
query_adapter_info(struct D3D_GPU_CTX* ctx, struct D3D_GPU_DATA* gpu, KMTQUERYADAPTERINFOTYPE type, void* ptr, UINT size)
{
	memset(ptr, 0, size);
	memset(&gpu->Qi, 0, sizeof(D3DKMT_QUERYADAPTERINFO));
	gpu->Qi.hAdapter = gpu->OpenAdapter.hAdapter;
	gpu->Qi.Type = type;
	gpu->Qi.pPrivateDriverData = ptr;
	gpu->Qi.PrivateDriverDataSize = size;
	return ctx->QueryAdapterInfo(&gpu->Qi);
}

static BOOL
is_duplicated_gpu(PNWLIB_GPU_INFO info, D3DKMT_ADAPTERADDRESS* bdf, D3DKMT_DEVICE_IDS* ids)
{
	for (uint32_t i = 0; i < info->DeviceCount; i++)
	{
		NWLIB_GPU_DEV* dev = &info->Device[i];
		if (dev->PciBus == bdf->BusNumber &&
			dev->PciDevice == bdf->DeviceNumber &&
			dev->PciFunction == bdf->FunctionNumber)
			return TRUE;
		if (dev->VendorId == ids->VendorID &&
			dev->DeviceId == ids->DeviceID &&
			dev->Subsys == ids->SubSystemID &&
			dev->RevId == ids->RevisionID)
			return TRUE;
	}
	return FALSE;
}

static void
get_ids_from_dxgi(LUID* luid, D3DKMT_DEVICE_IDS* ids)
{
	HMODULE dxgi;
	HRESULT(WINAPI * pfn_create_factory)(REFIID, void**);
	IDXGIFactory* factory;
	IDXGIAdapter* adapter;

	dxgi = LoadLibraryW(L"dxgi.dll");
	if (dxgi == NULL)
		return;
	*(FARPROC*)&pfn_create_factory = GetProcAddress(dxgi, "CreateDXGIFactory");
	if (pfn_create_factory == NULL)
		goto fail;
	if (FAILED(pfn_create_factory(&IID_IDXGIFactory, (void**)&factory)))
		goto fail;

	for (UINT i = 0; ; i++)
	{
		DXGI_ADAPTER_DESC desc;
		if (FAILED(factory->lpVtbl->EnumAdapters(factory, i, &adapter)))
			break;
		HRESULT hr = adapter->lpVtbl->GetDesc(adapter, &desc);
		adapter->lpVtbl->Release(adapter);
		if (FAILED(hr))
			continue;
		if (memcmp(luid, &desc.AdapterLuid, sizeof(LUID)) == 0)
		{
			NWL_Debug(GDID3D, "Found adapter in DXGI adapter %u", i);
			ids->VendorID = desc.VendorId;
			ids->DeviceID = desc.DeviceId;
			ids->SubSystemID = desc.SubSysId;
			ids->RevisionID = desc.Revision;
			break;
		}
	}
	factory->lpVtbl->Release(factory);

fail:
	if (dxgi)
		FreeLibrary(dxgi);
}

static void* d3d_gpu_init(PNWLIB_GPU_INFO info)
{
	struct D3D_GPU_CTX* ctx = calloc(1, sizeof(struct D3D_GPU_CTX));
	if (ctx == NULL)
		return NULL;

	ctx->Gdi = LoadLibraryW(L"gdi32.dll");
	if (ctx->Gdi == NULL)
	{
		NWL_Debug(GDID3D, "Cannot load gdi32.dll");
		free(ctx);
		return NULL;
	}

	*(FARPROC*)&ctx->EnumAdapters = GetProcAddress(ctx->Gdi, "D3DKMTEnumAdapters");
	*(FARPROC*)&ctx->QueryAdapterInfo = GetProcAddress(ctx->Gdi, "D3DKMTQueryAdapterInfo");
	*(FARPROC*)&ctx->OpenAdapterFromLuid = GetProcAddress(ctx->Gdi, "D3DKMTOpenAdapterFromLuid");
	*(FARPROC*)&ctx->QueryStatistics = GetProcAddress(ctx->Gdi, "D3DKMTQueryStatistics");
	*(FARPROC*)&ctx->CloseAdapter = GetProcAddress(ctx->Gdi, "D3DKMTCloseAdapter");
	if (ctx->EnumAdapters == NULL
		|| ctx->QueryAdapterInfo == NULL
		|| ctx->OpenAdapterFromLuid == NULL
		|| ctx->QueryStatistics == NULL
		|| ctx->CloseAdapter == NULL)
	{
		NWL_Debug(GDID3D, "Cannot get D3DKMT functions");
		goto fail;
	}

	ctx->Result = ctx->EnumAdapters(&ctx->Adapters);
	if (!NT_SUCCESS(ctx->Result))
		goto fail;
	ctx->Count = min(ctx->Adapters.NumAdapters, MAX_D3D_ADAPTER);

	for (ULONG i = 0; i < MAX_D3D_ADAPTER; i++)
	{
		struct D3D_GPU_DATA* gpu = &ctx->Adapter[i];
		gpu->OpenAdapter.AdapterLuid = ctx->Adapters.Adapters[i].AdapterLuid;
		ctx->Result = ctx->OpenAdapterFromLuid(&gpu->OpenAdapter);
		if (!NT_SUCCESS(ctx->Result))
			continue;

		ctx->Result = query_adapter_info(ctx, gpu, KMTQAITYPE_DRIVER_DESCRIPTION, &gpu->DriverDesc, sizeof(D3DKMT_DRIVER_DESCRIPTION));
		if (NT_SUCCESS(ctx->Result) && gpu->DriverDesc.DriverDescription[0] != L'\0')
			strcpy_s(gpu->Name, D3D_ADAPTER_NAME_LEN, NWL_Ucs2ToUtf8(gpu->DriverDesc.DriverDescription));
		ctx->Result = query_adapter_info(ctx, gpu, KMTQAITYPE_ADAPTERREGISTRYINFO, &gpu->RegInfo, sizeof(D3DKMT_ADAPTERREGISTRYINFO));
		if (NT_SUCCESS(ctx->Result) && gpu->Name[0] == L'\0')
			strcpy_s(gpu->Name, D3D_ADAPTER_NAME_LEN, NWL_Ucs2ToUtf8(gpu->RegInfo.AdapterString));
		
		NWL_Debug(GDID3D, "Querying adapter [%u]: %s", i, gpu->Name);

		ctx->Result = query_adapter_info(ctx, gpu, KMTQAITYPE_ADAPTERADDRESS, &gpu->Bdf, sizeof(D3DKMT_ADAPTERADDRESS));
		if (!NT_SUCCESS(ctx->Result))
			continue;

		NWL_Debug(GDID3D, "BDF %u:%u:%u", gpu->Bdf.BusNumber, gpu->Bdf.DeviceNumber, gpu->Bdf.FunctionNumber);

		// KMTQAITYPE_PHYSICALADAPTERDEVICEIDS requires Windows 10 or later
		ctx->Result = query_adapter_info(ctx, gpu, KMTQAITYPE_PHYSICALADAPTERDEVICEIDS, &gpu->DeviceIds, sizeof(D3DKMT_QUERY_DEVICE_IDS));
		if (!NT_SUCCESS(ctx->Result))
			get_ids_from_dxgi(&gpu->OpenAdapter.AdapterLuid, &gpu->DeviceIds.DeviceIds);
		else
		{
			gpu->DeviceIds.DeviceIds.SubSystemID <<= 16;
			gpu->DeviceIds.DeviceIds.SubSystemID |= gpu->DeviceIds.DeviceIds.SubVendorID;
		}
		NWL_Debug(GDID3D, "Device ID [%04X-%04X SUBSYS %08X REV %02X ]", gpu->DeviceIds.DeviceIds.VendorID,
			gpu->DeviceIds.DeviceIds.DeviceID, gpu->DeviceIds.DeviceIds.SubSystemID, gpu->DeviceIds.DeviceIds.RevisionID);

		if (gpu->DeviceIds.DeviceIds.VendorID == 0x1414)
		{
			NWL_Debug(GDID3D, "Skipping Microsoft Device %s", gpu->Name);
			continue;
		}

		// KMTQAITYPE_ADAPTERTYPE requires Windows 8 or later
		query_adapter_info(ctx, gpu, KMTQAITYPE_ADAPTERTYPE, &gpu->AdapterType, sizeof(D3DKMT_ADAPTERTYPE));
		if (gpu->AdapterType.SoftwareDevice)
		{
			NWL_Debug(GDID3D, "Skipping software adapter %s", gpu->Name);
			continue;
		}

		if (is_duplicated_gpu(info, &gpu->Bdf, &gpu->DeviceIds.DeviceIds))
		{
			NWL_Debug(GDID3D, "Skipping duplicated adapter [%04X-%04X SUBSYS %08X REV %02X ] %s",
				gpu->DeviceIds.DeviceIds.VendorID, gpu->DeviceIds.DeviceIds.DeviceID,
				gpu->DeviceIds.DeviceIds.SubSystemID, gpu->DeviceIds.DeviceIds.RevisionID, gpu->Name);
			continue;
		}

		gpu->Valid = TRUE;
		NWL_Debug(GDID3D, "Found [%lu] [%04X-%04X SUBSYS %08X REV %02X ] %s", i,
			gpu->DeviceIds.DeviceIds.VendorID, gpu->DeviceIds.DeviceIds.DeviceID,
			gpu->DeviceIds.DeviceIds.SubSystemID, gpu->DeviceIds.DeviceIds.RevisionID,
			gpu->Name);

		memset(&gpu->Stats, 0, sizeof(D3DKMT_QUERYSTATISTICS));
		gpu->Stats.Type = D3DKMT_QUERYSTATISTICS_ADAPTER;
		gpu->Stats.AdapterLuid = gpu->OpenAdapter.AdapterLuid;
		ctx->Result = ctx->QueryStatistics(&gpu->Stats);
		if (NT_SUCCESS(ctx->Result))
		{
			gpu->NbSegments = gpu->Stats.QueryResult.AdapterInformation.NbSegments;
			gpu->NodeCount = gpu->Stats.QueryResult.AdapterInformation.NodeCount;
			NWL_Debug(GDID3D, "%u segments, %u nodes", gpu->NbSegments, gpu->NodeCount);
		}
	}

	return ctx;

fail:
	NWL_Debug(GDID3D, "INIT ERR %x", ctx->Result);
	FreeLibrary(ctx->Gdi);
	free(ctx);
	return NULL;
}

static uint64_t get_used_memory(struct D3D_GPU_CTX* ctx, struct D3D_GPU_DATA* gpu)
{
	uint64_t used_memory = 0;
	for (ULONG i = 0; i < gpu->NbSegments; i++)
	{
		memset(&gpu->Stats, 0, sizeof(D3DKMT_QUERYSTATISTICS));
		gpu->Stats.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;
		gpu->Stats.AdapterLuid = gpu->OpenAdapter.AdapterLuid;
		gpu->Stats.QuerySegment.SegmentId = i;
		ctx->Result = ctx->QueryStatistics(&gpu->Stats);
		if (!NT_SUCCESS(ctx->Result))
			break;
		if (gpu->Stats.QueryResult.SegmentInformation.Aperture) // Shared
			continue;
		else
			used_memory += gpu->Stats.QueryResult.SegmentInformation.BytesResident;
	}
	return used_memory;
}

static double get_usage_percent(struct D3D_GPU_CTX* ctx, struct D3D_GPU_DATA* gpu)
{
	double usage = 0.0;
	uint64_t current_stamp = 0;
	uint64_t current_running = 0;
	for (ULONG i = 0; i < gpu->NodeCount; i++)
	{
		memset(&gpu->Stats, 0, sizeof(D3DKMT_QUERYSTATISTICS));
		gpu->Stats.Type = D3DKMT_QUERYSTATISTICS_NODE;
		gpu->Stats.AdapterLuid = gpu->OpenAdapter.AdapterLuid;
		gpu->Stats.QueryNode.NodeId = i;
		ctx->Result = ctx->QueryStatistics(&gpu->Stats);
		if (!NT_SUCCESS(ctx->Result))
			break;
		current_stamp += GetTickCount64();
		current_running += gpu->Stats.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart;
	}
	double diff_time = (double)current_stamp - gpu->TimeStamp;
	double diff_running = (double)current_running - gpu->RunningTime; // in microseconds
	if (diff_time != 0.0)
		usage = 0.1 * diff_running / diff_time;
	if (usage < 0.0)
		usage = 0.0;
	else if (usage > 100.0)
		usage = 100.0;
	gpu->RunningTime = current_running;
	gpu->TimeStamp = current_stamp;
	return usage;
}

static uint32_t d3d_gpu_get(void* data, NWLIB_GPU_DEV* dev, uint32_t dev_count)
{
	uint32_t count = 0;
	struct D3D_GPU_CTX* ctx = data;

	if (ctx == NULL || ctx->Count == 0)
		return 0;

	for (UINT i = 0; i < ctx->Count; i++)
	{
		if (count >= dev_count)
			break;
		if (ctx->Adapter[i].Valid == FALSE)
			continue;

		struct D3D_GPU_DATA* gpu = &ctx->Adapter[i];
		NWLIB_GPU_DEV* info = &dev[count];
		count++;

		strcpy_s(info->Name, MAX_GPU_STR, gpu->Name);
		info->VendorId = gpu->DeviceIds.DeviceIds.VendorID;
		info->DeviceId = gpu->DeviceIds.DeviceIds.DeviceID;
		info->Subsys = gpu->DeviceIds.DeviceIds.SubSystemID;
		info->RevId = gpu->DeviceIds.DeviceIds.RevisionID;
		info->PciBus = gpu->Bdf.BusNumber;
		info->PciDevice = gpu->Bdf.DeviceNumber;
		info->PciFunction = gpu->Bdf.FunctionNumber;

		ctx->Result = query_adapter_info(ctx, gpu, KMTQAITYPE_GETSEGMENTSIZE, &gpu->SegSize, sizeof(D3DKMT_SEGMENTSIZEINFO));
		if (NT_SUCCESS(ctx->Result))
			info->TotalMemory = gpu->SegSize.DedicatedVideoMemorySize;
		ctx->Result = query_adapter_info(ctx, gpu, KMTQAITYPE_ADAPTERPERFDATA, &gpu->PerfData, sizeof(D3DKMT_ADAPTER_PERFDATA));
		if (NT_SUCCESS(ctx->Result))
		{
			info->Temperature = (double)gpu->PerfData.Temperature * 0.1; // deci-Celsius to Celsius
			info->FanSpeed = gpu->PerfData.FanRPM;
		}

		info->UsedMemory = get_used_memory(ctx, gpu);
		if (info->UsedMemory < info->TotalMemory)
		{
			info->FreeMemory = info->TotalMemory - info->UsedMemory;
			info->MemoryPercent = 100ULL * info->UsedMemory / info->TotalMemory;
		}
		info->UsagePercent = get_usage_percent(ctx, gpu);
	}

	return count;
}

static void d3d_gpu_free(void* data)
{
	struct D3D_GPU_CTX* ctx = data;
	if (ctx == NULL)
		return;
	for (ULONG i = 0; i < ctx->Count; i++)
	{
		D3DKMT_CLOSEADAPTER close_adapter = { 0 };
		close_adapter.hAdapter = ctx->Adapter[i].OpenAdapter.hAdapter;
		ctx->CloseAdapter(&close_adapter);
	}
	FreeLibrary(ctx->Gdi);
	free(ctx);
}

NWLIB_GPU_DRV gpu_drv_d3d =
{
	.Name = GDID3D,
	.Init = d3d_gpu_init,
	.GetInfo = d3d_gpu_get,
	.Free = d3d_gpu_free,
};
