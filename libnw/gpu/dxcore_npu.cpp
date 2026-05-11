// SPDX-License-Identifier: Unlicense

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <d3dkmthk.h>
#include <dxcore.h>
#include "gpu.h"

extern "C"
{
extern void(*NWL_Debug)(const char* condition, char const* const format, ...);
}

#define DXCORE "DXCORE"

#define MAX_DXCORE_NPU NWL_GPU_MAX_COUNT

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

typedef HRESULT(WINAPI* PFN_DXCORE_CREATE_ADAPTER_FACTORY)(REFIID, void**);

struct DXCORE_NPU_DATA
{
	BOOL Valid;
	CHAR Name[MAX_GPU_STR];
	LUID Luid;
	DXCoreHardwareIDParts HardwareId;
	BOOL Integrated;
	D3DKMT_ADAPTERTYPE AdapterType;
	D3DKMT_OPENADAPTERFROMLUID OpenAdapter;
	D3DKMT_QUERYADAPTERINFO Qi;
	D3DKMT_ADAPTERADDRESS Bdf;
	D3DKMT_SEGMENTSIZEINFO SegSize;
	D3DKMT_QUERYSTATISTICS Stats;
	ULONG NbSegments;
	ULONG NodeCount;
	UINT64 TimeStamp;
	UINT64 RunningTime;
};

struct DXCORE_NPU_CTX
{
	HMODULE DxCore;
	HMODULE Gdi;
	PFN_DXCORE_CREATE_ADAPTER_FACTORY CreateAdapterFactory;
	NTSTATUS(APIENTRY* QueryAdapterInfo)(const D3DKMT_QUERYADAPTERINFO*);
	NTSTATUS(APIENTRY* OpenAdapterFromLuid)(D3DKMT_OPENADAPTERFROMLUID*);
	NTSTATUS(APIENTRY* QueryStatistics)(const D3DKMT_QUERYSTATISTICS*);
	NTSTATUS(APIENTRY* CloseAdapter)(const D3DKMT_CLOSEADAPTER*);
	IDXCoreAdapterFactory* Factory;
	IDXCoreAdapterList* AdapterList;
	HRESULT Hr;
	NTSTATUS Result;
	uint32_t Count;
	DXCORE_NPU_DATA Adapter[MAX_DXCORE_NPU];
};

static uint32_t
pack_subsys(const DXCoreHardwareIDParts* ids)
{
	return (ids->subSystemID << 16) | ids->subVendorID;
}

static NTSTATUS
query_adapter_info(DXCORE_NPU_CTX* ctx, DXCORE_NPU_DATA* npu, KMTQUERYADAPTERINFOTYPE type, void* ptr, UINT size)
{
	memset(ptr, 0, size);
	memset(&npu->Qi, 0, sizeof(D3DKMT_QUERYADAPTERINFO));
	npu->Qi.hAdapter = npu->OpenAdapter.hAdapter;
	npu->Qi.Type = type;
	npu->Qi.pPrivateDriverData = ptr;
	npu->Qi.PrivateDriverDataSize = size;
	return ctx->QueryAdapterInfo(&npu->Qi);
}

static BOOL
is_duplicated_gpu(PNWLIB_GPU_INFO info, DXCORE_NPU_DATA* npu)
{
	for (uint32_t i = 0; i < info->DeviceCount; i++)
	{
		NWLIB_GPU_DEV* dev = &info->Device[i];
		if (dev->PciBus == npu->Bdf.BusNumber &&
			dev->PciDevice == npu->Bdf.DeviceNumber &&
			dev->PciFunction == npu->Bdf.FunctionNumber)
			return TRUE;
		if (dev->VendorId == npu->HardwareId.vendorID &&
			dev->DeviceId == npu->HardwareId.deviceID &&
			dev->Subsys == pack_subsys(&npu->HardwareId) &&
			dev->RevId == npu->HardwareId.revisionID)
			return TRUE;
	}
	return FALSE;
}

static HRESULT
get_hardware_id(IDXCoreAdapter* adapter, DXCoreHardwareIDParts* ids)
{
	memset(ids, 0, sizeof(DXCoreHardwareIDParts));
	if (adapter->IsPropertySupported(DXCoreAdapterProperty::HardwareIDParts))
	{
		HRESULT hr = adapter->GetProperty(DXCoreAdapterProperty::HardwareIDParts, sizeof(DXCoreHardwareIDParts), ids);
		if (SUCCEEDED(hr))
			return hr;
	}

	DXCoreHardwareID legacy_id = { 0 };
	HRESULT hr = adapter->GetProperty(DXCoreAdapterProperty::HardwareID, sizeof(DXCoreHardwareID), &legacy_id);
	if (SUCCEEDED(hr))
	{
		ids->vendorID = legacy_id.vendorID;
		ids->deviceID = legacy_id.deviceID;
		ids->subVendorID = legacy_id.subSysID & 0xFFFF;
		ids->subSystemID = legacy_id.subSysID >> 16;
		ids->revisionID = legacy_id.revision;
	}
	return hr;
}

static void
get_adapter_name(IDXCoreAdapter* adapter, DXCORE_NPU_DATA* npu)
{
	CHAR desc[MAX_GPU_STR];

	memset(desc, 0, sizeof(desc));
	if (SUCCEEDED(adapter->GetProperty(DXCoreAdapterProperty::DriverDescription, sizeof(desc), desc)) &&
		desc[0] != '\0')
		strncpy_s(npu->Name, MAX_GPU_STR, desc, _TRUNCATE);
	if (npu->Name[0] == '\0')
		snprintf(npu->Name, MAX_GPU_STR, "NPU %04X:%04X", npu->HardwareId.vendorID, npu->HardwareId.deviceID);
}

static void
close_d3dkmt_adapter(DXCORE_NPU_CTX* ctx, DXCORE_NPU_DATA* npu)
{
	D3DKMT_CLOSEADAPTER close_adapter = { 0 };
	close_adapter.hAdapter = npu->OpenAdapter.hAdapter;
	ctx->CloseAdapter(&close_adapter);
}

static BOOL
init_d3dkmt_adapter(DXCORE_NPU_CTX* ctx, DXCORE_NPU_DATA* npu)
{
	npu->OpenAdapter.AdapterLuid = npu->Luid;
	ctx->Result = ctx->OpenAdapterFromLuid(&npu->OpenAdapter);
	if (!NT_SUCCESS(ctx->Result))
		return FALSE;

	ctx->Result = query_adapter_info(ctx, npu, KMTQAITYPE_ADAPTERADDRESS, &npu->Bdf, sizeof(D3DKMT_ADAPTERADDRESS));
	if (!NT_SUCCESS(ctx->Result))
	{
		NWL_Debug(DXCORE, "KMTQAITYPE_ADAPTERADDRESS failed %x", ctx->Result);
		goto fail;
	}
	if (npu->Bdf.BusNumber > 0xFF || npu->Bdf.DeviceNumber > 0x1F || npu->Bdf.FunctionNumber > 0x7)
	{
		NWL_Debug(DXCORE, "Invalid BDF %u:%u:%u", npu->Bdf.BusNumber, npu->Bdf.DeviceNumber, npu->Bdf.FunctionNumber);
		ctx->Result = (NTSTATUS)0xC000000D;
		goto fail;
	}

	ctx->Result = query_adapter_info(ctx, npu, KMTQAITYPE_ADAPTERTYPE, &npu->AdapterType, sizeof(D3DKMT_ADAPTERTYPE));
	if (!NT_SUCCESS(ctx->Result))
	{
		NWL_Debug(DXCORE, "KMTQAITYPE_ADAPTERTYPE failed %x", ctx->Result);
		goto fail;
	}

	memset(&npu->Stats, 0, sizeof(D3DKMT_QUERYSTATISTICS));
	npu->Stats.Type = D3DKMT_QUERYSTATISTICS_ADAPTER;
	npu->Stats.AdapterLuid = npu->OpenAdapter.AdapterLuid;
	ctx->Result = ctx->QueryStatistics(&npu->Stats);
	if (!NT_SUCCESS(ctx->Result))
		goto fail;

	npu->NbSegments = npu->Stats.QueryResult.AdapterInformation.NbSegments;
	npu->NodeCount = npu->Stats.QueryResult.AdapterInformation.NodeCount;
	return TRUE;

fail:
	close_d3dkmt_adapter(ctx, npu);
	return FALSE;
}

static uint64_t
get_used_memory(DXCORE_NPU_CTX* ctx, DXCORE_NPU_DATA* npu, BOOL shared)
{
	uint64_t used_memory = 0;
	for (ULONG i = 0; i < npu->NbSegments; i++)
	{
		memset(&npu->Stats, 0, sizeof(D3DKMT_QUERYSTATISTICS));
		npu->Stats.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;
		npu->Stats.AdapterLuid = npu->OpenAdapter.AdapterLuid;
		npu->Stats.QuerySegment.SegmentId = i;
		ctx->Result = ctx->QueryStatistics(&npu->Stats);
		if (!NT_SUCCESS(ctx->Result))
			break;
		if ((BOOL)npu->Stats.QueryResult.SegmentInformation.Aperture != shared)
			continue;
		used_memory += npu->Stats.QueryResult.SegmentInformation.BytesResident;
	}
	return used_memory;
}

static double
get_usage_percent(DXCORE_NPU_CTX* ctx, DXCORE_NPU_DATA* npu)
{
	double usage = 0.0;
	uint64_t current_stamp = 0;
	uint64_t current_running = 0;
	for (ULONG i = 0; i < npu->NodeCount; i++)
	{
		memset(&npu->Stats, 0, sizeof(D3DKMT_QUERYSTATISTICS));
		npu->Stats.Type = D3DKMT_QUERYSTATISTICS_NODE;
		npu->Stats.AdapterLuid = npu->OpenAdapter.AdapterLuid;
		npu->Stats.QueryNode.NodeId = i;
		ctx->Result = ctx->QueryStatistics(&npu->Stats);
		if (!NT_SUCCESS(ctx->Result))
			break;
		current_stamp += GetTickCount64();
		current_running += npu->Stats.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart;
	}
	double diff_time = (double)current_stamp - npu->TimeStamp;
	double diff_running = (double)current_running - npu->RunningTime;
	if (diff_time != 0.0)
		usage = 0.1 * diff_running / diff_time;
	if (usage < 0.0)
		usage = 0.0;
	else if (usage > 100.0)
		usage = 100.0;
	npu->RunningTime = current_running;
	npu->TimeStamp = current_stamp;
	return usage;
}

static void
free_context(DXCORE_NPU_CTX* ctx)
{
	for (uint32_t i = 0; i < ctx->Count; i++)
		close_d3dkmt_adapter(ctx, &ctx->Adapter[i]);
	if (ctx->AdapterList)
		ctx->AdapterList->Release();
	if (ctx->Factory)
		ctx->Factory->Release();
	if (ctx->Gdi)
		FreeLibrary(ctx->Gdi);
	if (ctx->DxCore)
		FreeLibrary(ctx->DxCore);
	free(ctx);
}

static HRESULT
create_npu_adapter_list(DXCORE_NPU_CTX* ctx)
{
	static const GUID npu_attr = DXCORE_HARDWARE_TYPE_ATTRIBUTE_NPU;

	ctx->Hr = ctx->Factory->CreateAdapterList(1, &npu_attr,
		__uuidof(IDXCoreAdapterList), reinterpret_cast<void**>(&ctx->AdapterList));
	if (SUCCEEDED(ctx->Hr))
		NWL_Debug(DXCORE, "Found %u NPU adapter(s)", ctx->AdapterList->GetAdapterCount());
	return ctx->Hr;
}

static void* dxcore_gpu_init(PNWLIB_GPU_INFO info)
{
	DXCORE_NPU_CTX* ctx = static_cast<DXCORE_NPU_CTX*>(calloc(1, sizeof(DXCORE_NPU_CTX)));
	uint32_t adapter_count = 0;
	if (ctx == NULL)
		return NULL;

	ctx->DxCore = LoadLibraryW(L"dxcore.dll");
	if (ctx->DxCore == NULL)
	{
		NWL_Debug(DXCORE, "Cannot load dxcore.dll");
		goto fail;
	}

	*(FARPROC*)&ctx->CreateAdapterFactory = GetProcAddress(ctx->DxCore, "DXCoreCreateAdapterFactory");
	if (ctx->CreateAdapterFactory == NULL)
	{
		NWL_Debug(DXCORE, "Cannot get DXCoreCreateAdapterFactory");
		goto fail;
	}

	ctx->Gdi = LoadLibraryW(L"gdi32.dll");
	if (ctx->Gdi == NULL)
	{
		NWL_Debug(DXCORE, "Cannot load gdi32.dll");
		goto fail;
	}

	*(FARPROC*)&ctx->QueryAdapterInfo = GetProcAddress(ctx->Gdi, "D3DKMTQueryAdapterInfo");
	*(FARPROC*)&ctx->OpenAdapterFromLuid = GetProcAddress(ctx->Gdi, "D3DKMTOpenAdapterFromLuid");
	*(FARPROC*)&ctx->QueryStatistics = GetProcAddress(ctx->Gdi, "D3DKMTQueryStatistics");
	*(FARPROC*)&ctx->CloseAdapter = GetProcAddress(ctx->Gdi, "D3DKMTCloseAdapter");
	if (ctx->QueryAdapterInfo == NULL
		|| ctx->OpenAdapterFromLuid == NULL
		|| ctx->QueryStatistics == NULL
		|| ctx->CloseAdapter == NULL)
	{
		NWL_Debug(DXCORE, "Cannot get D3DKMT functions");
		goto fail;
	}

	ctx->Hr = ctx->CreateAdapterFactory(__uuidof(IDXCoreAdapterFactory), reinterpret_cast<void**>(&ctx->Factory));
	if (FAILED(ctx->Hr))
		goto fail;

	ctx->Hr = create_npu_adapter_list(ctx);
	if (FAILED(ctx->Hr))
		goto fail;

	adapter_count = ctx->AdapterList->GetAdapterCount();
	NWL_Debug(DXCORE, "Found %u NPU adapter(s)", adapter_count);
	if (adapter_count == 0)
		goto fail;

	for (uint32_t i = 0; i < adapter_count && ctx->Count < MAX_DXCORE_NPU; i++)
	{
		IDXCoreAdapter* adapter;
		DXCORE_NPU_DATA* npu = &ctx->Adapter[ctx->Count];

		ctx->Hr = ctx->AdapterList->GetAdapter(i, __uuidof(IDXCoreAdapter), reinterpret_cast<void**>(&adapter));
		if (FAILED(ctx->Hr))
			goto fail;
		if (!adapter->IsValid())
		{
			adapter->Release();
			ctx->Hr = E_FAIL;
			goto fail;
		}
		if (adapter->IsAttributeSupported(DXCORE_HARDWARE_TYPE_ATTRIBUTE_GPU) ||
			adapter->IsAttributeSupported(DXCORE_ADAPTER_ATTRIBUTE_D3D11_GRAPHICS) ||
			adapter->IsAttributeSupported(DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS))
		{
			NWL_Debug(DXCORE, "Skipping graphics adapter [%u]", i);
			adapter->Release();
			continue;
		}

		ctx->Hr = adapter->GetProperty(DXCoreAdapterProperty::InstanceLuid, sizeof(LUID), &npu->Luid);
		if (FAILED(ctx->Hr))
		{
			adapter->Release();
			goto fail;
		}

		ctx->Hr = get_hardware_id(adapter, &npu->HardwareId);
		if (FAILED(ctx->Hr))
		{
			adapter->Release();
			goto fail;
		}

		get_adapter_name(adapter, npu);
		if (adapter->IsPropertySupported(DXCoreAdapterProperty::IsIntegrated))
		{
			bool integrated = false;
			ctx->Hr = adapter->GetProperty(DXCoreAdapterProperty::IsIntegrated, sizeof(bool), &integrated);
			if (SUCCEEDED(ctx->Hr) && integrated)
				npu->Integrated = TRUE;
		}
		adapter->Release();

		NWL_Debug(DXCORE, "Querying adapter [%u]: %s", i, npu->Name);
		if (!init_d3dkmt_adapter(ctx, npu))
			goto fail;

		NWL_Debug(DXCORE, "BDF %u:%u:%u", npu->Bdf.BusNumber, npu->Bdf.DeviceNumber, npu->Bdf.FunctionNumber);
		if (npu->AdapterType.DisplaySupported || (npu->AdapterType.RenderSupported && !npu->AdapterType.ComputeOnly))
		{
			NWL_Debug(DXCORE, "Skipping render/display adapter [%04X-%04X SUBSYS %08X REV %02X] %s",
				npu->HardwareId.vendorID, npu->HardwareId.deviceID,
				pack_subsys(&npu->HardwareId), npu->HardwareId.revisionID, npu->Name);
			close_d3dkmt_adapter(ctx, npu);
			memset(npu, 0, sizeof(DXCORE_NPU_DATA));
			continue;
		}
		if (is_duplicated_gpu(info, npu))
		{
			NWL_Debug(DXCORE, "Skipping duplicated adapter [%04X-%04X SUBSYS %08X REV %02X] %s",
				npu->HardwareId.vendorID, npu->HardwareId.deviceID,
				pack_subsys(&npu->HardwareId), npu->HardwareId.revisionID, npu->Name);
			close_d3dkmt_adapter(ctx, npu);
			memset(npu, 0, sizeof(DXCORE_NPU_DATA));
			continue;
		}

		npu->Valid = TRUE;
		ctx->Count++;
		NWL_Debug(DXCORE, "Found [%u] [%04X-%04X SUBSYS %08X REV %02X] %s, %u segments, %u nodes",
			i, npu->HardwareId.vendorID, npu->HardwareId.deviceID,
			pack_subsys(&npu->HardwareId), npu->HardwareId.revisionID,
			npu->Name, npu->NbSegments, npu->NodeCount);
	}

	if (ctx->Count == 0)
		goto fail;
	return ctx;

fail:
	NWL_Debug(DXCORE, "INIT ERR %x %x", ctx->Hr, ctx->Result);
	free_context(ctx);
	return NULL;
}

static uint32_t dxcore_gpu_get(void* data, NWLIB_GPU_DEV* dev, uint32_t dev_count)
{
	uint32_t count = 0;
	DXCORE_NPU_CTX* ctx = static_cast<DXCORE_NPU_CTX*>(data);

	if (ctx->Count == 0)
		return 0;

	for (uint32_t i = 0; i < ctx->Count; i++)
	{
		if (count >= dev_count)
			break;

		DXCORE_NPU_DATA* npu = &ctx->Adapter[i];
		NWLIB_GPU_DEV* info = &dev[count];
		count++;

		strncpy_s(info->Name, MAX_GPU_STR, npu->Name, _TRUNCATE);
		info->VendorId = npu->HardwareId.vendorID;
		info->DeviceId = npu->HardwareId.deviceID;
		info->Subsys = pack_subsys(&npu->HardwareId);
		info->RevId = npu->HardwareId.revisionID;
		info->PciBus = npu->Bdf.BusNumber;
		info->PciDevice = npu->Bdf.DeviceNumber;
		info->PciFunction = npu->Bdf.FunctionNumber;
		info->Flags |= NWLIB_GPU_FLAG_NPU;
		if (npu->Integrated)
			info->Flags |= NWLIB_GPU_FLAG_INTEGRATED;

		ctx->Result = query_adapter_info(ctx, npu, KMTQAITYPE_GETSEGMENTSIZE, &npu->SegSize, sizeof(D3DKMT_SEGMENTSIZEINFO));
		if (NT_SUCCESS(ctx->Result))
		{
			if (npu->SegSize.SharedSystemMemorySize > 0)
				info->TotalMemory = npu->SegSize.SharedSystemMemorySize;
			else if (npu->SegSize.DedicatedSystemMemorySize > 0)
				info->TotalMemory = npu->SegSize.DedicatedSystemMemorySize;
			else
				info->TotalMemory = npu->SegSize.DedicatedVideoMemorySize;
		}

		info->UsedMemory = get_used_memory(ctx, npu, npu->SegSize.SharedSystemMemorySize > 0);
		if (info->UsedMemory < info->TotalMemory)
		{
			info->FreeMemory = info->TotalMemory - info->UsedMemory;
			info->MemoryPercent = 100ULL * info->UsedMemory / info->TotalMemory;
		}
		info->UsagePercent = get_usage_percent(ctx, npu);
	}

	return count;
}

static void dxcore_gpu_free(void* data)
{
	if (data == NULL)
		return;
	free_context(static_cast<DXCORE_NPU_CTX*>(data));
}

extern "C" NWLIB_GPU_DRV gpu_drv_dxcore =
{
	.Name = DXCORE,
	.Init = dxcore_gpu_init,
	.GetInfo = dxcore_gpu_get,
	.Free = dxcore_gpu_free,
};
