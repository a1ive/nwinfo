// SPDX-License-Identifier: Unlicense

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <d3dkmthk.h>
#include <dxgi.h>
#include <ioctl.h>
#include "gpu.h"
#include "utils.h"
#include "libnw.h"

#pragma comment(lib, "dxguid.lib")

#define GDID3D "D3DKMT"

#define MAX_D3D_ADAPTER MAX_ENUM_ADAPTERS

#define D3D_ADAPTER_NAME_LEN 256

#define PCI_CFG_VENDOR_ID         0x00
#define PCI_CFG_STATUS            0x06
#define PCI_CFG_HEADER_TYPE       0x0E
#define PCI_CFG_CAP_PTR           0x34
#define PCI_CFG_CB_CAP_PTR        0x14

#define PCI_STATUS_CAP_LIST       0x0010

#define PCI_HEADER_TYPE_MASK      0x7F
#define PCI_HEADER_TYPE_CARDBUS   0x02

#define PCI_CAP_ID_PCI_EXP        0x10
#define PCI_CAP_NEXT_MASK         0xFC

#define PCI_EXP_LNKCAP            0x0C
#define PCI_EXP_LNKSTA            0x12

#define PCI_EXP_LNKCAP_SLS        0x0000000F
#define PCI_EXP_LNKCAP_MLW        0x000003F0
#define PCI_EXP_LNKCAP_MLW_SHIFT  4

#define PCI_EXP_LNKSTA_CLS        0x000F
#define PCI_EXP_LNKSTA_NLW        0x03F0
#define PCI_EXP_LNKSTA_NLW_SHIFT  4

struct D3D_GPU_DATA
{
	BOOL Valid;
	CHAR Name[D3D_ADAPTER_NAME_LEN];
	IDXGIAdapter* DxgiAdapter;
	D3DKMT_OPENADAPTERFROMLUID OpenAdapter;
	D3DKMT_QUERYADAPTERINFO Qi;
	D3DKMT_DRIVER_DESCRIPTION DriverDesc;
	D3DKMT_ADAPTERREGISTRYINFO RegInfo;
	D3DKMT_ADAPTERTYPE AdapterType;
	D3DKMT_QUERY_DEVICE_IDS DeviceIds;
	D3DKMT_ADAPTERADDRESS Bdf;
	NWLIB_GPU_PCIE_SPEED CurSpeed;
	NWLIB_GPU_PCIE_SPEED MaxSpeed;
	D3DKMT_SEGMENTSIZEINFO SegSize;
	D3DKMT_ADAPTER_PERFDATA PerfData;
	D3DKMT_QUERYSTATISTICS Stats;
	D3D_FEATURE_LEVEL FeatureLevel;
	ULONG NbSegments;
	ULONG NodeCount;
	UINT64 TimeStamp;
	UINT64 RunningTime;
};

struct D3D_GPU_CTX
{
	PNWLIB_GPU_INFO Info;
	HMODULE Gdi;
	HMODULE Dxgi;
	IDXGIFactory* DxgiFactory;
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

static inline uint8_t
find_pcie_capability(uint32_t pci_addr)
{
	uint16_t status = WR0_RdPciConf16(NWLC->NwDrv, pci_addr, PCI_CFG_STATUS);
	if ((status & PCI_STATUS_CAP_LIST) == 0)
		return 0;

	uint8_t header_type = WR0_RdPciConf8(NWLC->NwDrv, pci_addr, PCI_CFG_HEADER_TYPE) & PCI_HEADER_TYPE_MASK;
	uint8_t cap_ptr = WR0_RdPciConf8(NWLC->NwDrv, pci_addr,
		header_type == PCI_HEADER_TYPE_CARDBUS ? PCI_CFG_CB_CAP_PTR : PCI_CFG_CAP_PTR);
	cap_ptr &= PCI_CAP_NEXT_MASK;

	for (uint32_t i = 0; i < 48 && cap_ptr >= 0x40; i++)
	{
		uint16_t cap_hdr = WR0_RdPciConf16(NWLC->NwDrv, pci_addr, cap_ptr);
		if ((cap_hdr & 0xFF) == PCI_CAP_ID_PCI_EXP)
			return cap_ptr;
		cap_ptr = (uint8_t)((cap_hdr >> 8) & PCI_CAP_NEXT_MASK);
	}

	return 0;
}

static void
get_gpu_pcie_speed(struct D3D_GPU_DATA* gpu)
{
	if (NWLC->NwDrv == NULL || NWLC->NwDrv->type == WR0_DRIVER_PAWNIO)
		return;
	if (!WR0_WaitPciBus(500))
		return;
	NWL_Debug(GDID3D, "Getting PCIe speed for device at %02X:%02X.%d",
		gpu->Bdf.BusNumber, gpu->Bdf.DeviceNumber, gpu->Bdf.FunctionNumber);
	uint32_t pci_addr = PciBusDevFunc(gpu->Bdf.BusNumber, gpu->Bdf.DeviceNumber, gpu->Bdf.FunctionNumber);
	uint16_t vendor_id = WR0_RdPciConf16(NWLC->NwDrv, pci_addr, PCI_CFG_VENDOR_ID);
	if (vendor_id == 0xFFFF || vendor_id == 0x0000)
		goto exit;

	uint8_t pcie_cap = find_pcie_capability(pci_addr);
	if (pcie_cap == 0)
		goto exit;

	uint32_t link_cap = WR0_RdPciConf32(NWLC->NwDrv, pci_addr, pcie_cap + PCI_EXP_LNKCAP);
	uint16_t link_sta = WR0_RdPciConf16(NWLC->NwDrv, pci_addr, pcie_cap + PCI_EXP_LNKSTA);

	uint16_t cur_gen = (uint16_t)(link_sta & PCI_EXP_LNKSTA_CLS);
	uint16_t cur_lanes = (uint16_t)((link_sta & PCI_EXP_LNKSTA_NLW) >> PCI_EXP_LNKSTA_NLW_SHIFT);
	uint16_t max_gen = (uint16_t)(link_cap & PCI_EXP_LNKCAP_SLS);
	uint16_t max_lanes = (uint16_t)((link_cap & PCI_EXP_LNKCAP_MLW) >> PCI_EXP_LNKCAP_MLW_SHIFT);
	NWL_Debug(GDID3D, "Device at %02X:%02X.%d - Current Gen: %u, Current Lanes: %u, Max Gen: %u, Max Lanes: %u",
		gpu->Bdf.BusNumber, gpu->Bdf.DeviceNumber, gpu->Bdf.FunctionNumber, cur_gen, cur_lanes, max_gen, max_lanes);

	if (cur_gen > 0)
		gpu->CurSpeed.Gen = cur_gen;
	if (cur_lanes > 0)
		gpu->CurSpeed.Lanes = cur_lanes;
	if (max_gen > 0)
		gpu->MaxSpeed.Gen = max_gen;
	if (max_lanes > 0)
		gpu->MaxSpeed.Lanes = max_lanes;

exit:
	WR0_ReleasePciBus();
}

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

static NWLIB_GPU_DEV*
find_duplicated_gpu(PNWLIB_GPU_INFO info, D3DKMT_ADAPTERADDRESS* bdf, D3DKMT_DEVICE_IDS* ids)
{
	for (uint32_t i = 0; i < info->DeviceCount; i++)
	{
		NWLIB_GPU_DEV* dev = &info->Device[i];
		if (dev->PciBus == bdf->BusNumber &&
			dev->PciDevice == bdf->DeviceNumber &&
			dev->PciFunction == bdf->FunctionNumber)
			return dev;
		if (dev->VendorId == ids->VendorID &&
			dev->DeviceId == ids->DeviceID &&
			dev->Subsys == ids->SubSystemID &&
			dev->RevId == ids->RevisionID)
			return dev;
	}
	return NULL;
}

static D3D_FEATURE_LEVEL
get_d3d_feature_level(struct D3D_GPU_DATA* gpu)
{
	D3D_FEATURE_LEVEL level = 0;
	const D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	if (gpu->DxgiAdapter == NULL)
		return level;

	HMODULE d3d12 = LoadLibraryW(L"d3d12.dll");
	if (d3d12)
	{
		HRESULT(WINAPI* pfn_d3d12_create_device)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**) = NULL;

		*(FARPROC*)&pfn_d3d12_create_device = GetProcAddress(d3d12, "D3D12CreateDevice");
		if (pfn_d3d12_create_device)
		{
			for (UINT i = 0; i < (UINT)ARRAYSIZE(levels) && levels[i] >= D3D_FEATURE_LEVEL_11_0; i++)
			{
				ID3D12Device* device = NULL;

				if (SUCCEEDED(pfn_d3d12_create_device((IUnknown*)gpu->DxgiAdapter, levels[i],
					&IID_ID3D12Device, (void**)&device)))
				{
					level = levels[i];
					device->lpVtbl->Release(device);
					break;
				}
			}
		}
		FreeLibrary(d3d12);
		if (level != 0)
			return level;
	}

	HMODULE d3d11 = LoadLibraryW(L"d3d11.dll");
	if (d3d11)
	{
		HRESULT(WINAPI* pfn_d3d11_create_device)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
			const D3D_FEATURE_LEVEL*, UINT, UINT, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**) = NULL;

		*(FARPROC*)&pfn_d3d11_create_device = GetProcAddress(d3d11, "D3D11CreateDevice");
		if (pfn_d3d11_create_device)
		{
			for (UINT i = 1; i < (UINT)ARRAYSIZE(levels); i++)
			{
				ID3D11Device* device = NULL;
				ID3D11DeviceContext* device_context = NULL;
				D3D_FEATURE_LEVEL detected_level = 0;

				if (SUCCEEDED(pfn_d3d11_create_device(gpu->DxgiAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, &levels[i], 1,
					D3D11_SDK_VERSION, &device, &detected_level, &device_context)))
					level = detected_level;
				if (device_context)
					device_context->lpVtbl->Release(device_context);
				if (device)
					device->lpVtbl->Release(device);
				if (level != 0)
					break;
			}
		}
		FreeLibrary(d3d11);
	}

	return level;
}

static void* d3d_gpu_init(PNWLIB_GPU_INFO info)
{
	struct D3D_GPU_CTX* ctx = calloc(1, sizeof(struct D3D_GPU_CTX));
	if (ctx == NULL)
		return NULL;
	ctx->Info = info;

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

	ctx->Dxgi = LoadLibraryW(L"dxgi.dll");
	if (ctx->Dxgi)
	{
		HRESULT(WINAPI * pfn_create_factory)(REFIID, void**) = NULL;
		*(FARPROC*)&pfn_create_factory = GetProcAddress(ctx->Dxgi, "CreateDXGIFactory");
		if (pfn_create_factory)
			pfn_create_factory(&IID_IDXGIFactory, (void**)&ctx->DxgiFactory);
	}
	ctx->Result = ctx->EnumAdapters(&ctx->Adapters);
	if (!NT_SUCCESS(ctx->Result))
		goto fail;
	ctx->Count = min(ctx->Adapters.NumAdapters, MAX_D3D_ADAPTER);

	for (ULONG i = 0; i < ctx->Count; i++)
	{
		struct D3D_GPU_DATA* gpu = &ctx->Adapter[i];
		gpu->OpenAdapter.AdapterLuid = ctx->Adapters.Adapters[i].AdapterLuid;
		ctx->Result = ctx->OpenAdapterFromLuid(&gpu->OpenAdapter);
		if (!NT_SUCCESS(ctx->Result))
			continue;

		ctx->Result = query_adapter_info(ctx, gpu, KMTQAITYPE_DRIVER_DESCRIPTION, &gpu->DriverDesc, sizeof(D3DKMT_DRIVER_DESCRIPTION));
		if (NT_SUCCESS(ctx->Result) && gpu->DriverDesc.DriverDescription[0] != L'\0')
			strncpy_s(gpu->Name, D3D_ADAPTER_NAME_LEN, NWL_Ucs2ToUtf8(gpu->DriverDesc.DriverDescription), _TRUNCATE);
		ctx->Result = query_adapter_info(ctx, gpu, KMTQAITYPE_ADAPTERREGISTRYINFO, &gpu->RegInfo, sizeof(D3DKMT_ADAPTERREGISTRYINFO));
		if (NT_SUCCESS(ctx->Result) && gpu->Name[0] == L'\0')
			strncpy_s(gpu->Name, D3D_ADAPTER_NAME_LEN, NWL_Ucs2ToUtf8(gpu->RegInfo.AdapterString), _TRUNCATE);
		
		NWL_Debug(GDID3D, "Querying adapter [%u]: %s", i, gpu->Name);

		ctx->Result = query_adapter_info(ctx, gpu, KMTQAITYPE_ADAPTERADDRESS, &gpu->Bdf, sizeof(D3DKMT_ADAPTERADDRESS));
		if (!NT_SUCCESS(ctx->Result))
			continue;

		if (gpu->Bdf.BusNumber > 0xFF || gpu->Bdf.DeviceNumber > 0x1F || gpu->Bdf.FunctionNumber > 0x7)
		{
			NWL_Debug(GDID3D, "Skipping invalid BDF %u:%u:%u", gpu->Bdf.BusNumber, gpu->Bdf.DeviceNumber, gpu->Bdf.FunctionNumber);
			continue;
		}
		else
			NWL_Debug(GDID3D, "BDF %u:%u:%u", gpu->Bdf.BusNumber, gpu->Bdf.DeviceNumber, gpu->Bdf.FunctionNumber);

		if (ctx->DxgiFactory)
		{
			for (UINT j = 0; ; j++)
			{
				DXGI_ADAPTER_DESC desc = { 0 };
				IDXGIAdapter* adapter = NULL;
				if (FAILED(ctx->DxgiFactory->lpVtbl->EnumAdapters(ctx->DxgiFactory, j, &adapter)))
					break;
				if (SUCCEEDED(adapter->lpVtbl->GetDesc(adapter, &desc)) &&
					memcmp(&gpu->OpenAdapter.AdapterLuid, &desc.AdapterLuid, sizeof(LUID)) == 0)
				{
					gpu->DxgiAdapter = adapter;
					break;
				}
				adapter->lpVtbl->Release(adapter);
			}
		}

		// KMTQAITYPE_PHYSICALADAPTERDEVICEIDS requires Windows 10 or later
		ctx->Result = query_adapter_info(ctx, gpu, KMTQAITYPE_PHYSICALADAPTERDEVICEIDS, &gpu->DeviceIds, sizeof(D3DKMT_QUERY_DEVICE_IDS));
		if (!NT_SUCCESS(ctx->Result) && gpu->DxgiAdapter)
		{
			DXGI_ADAPTER_DESC desc = { 0 };
			if (SUCCEEDED(gpu->DxgiAdapter->lpVtbl->GetDesc(gpu->DxgiAdapter, &desc)))
			{
				NWL_Debug(GDID3D, "Found adapter in DXGI");
				gpu->DeviceIds.DeviceIds.VendorID = desc.VendorId;
				gpu->DeviceIds.DeviceIds.DeviceID = desc.DeviceId;
				gpu->DeviceIds.DeviceIds.SubSystemID = desc.SubSysId;
				gpu->DeviceIds.DeviceIds.RevisionID = desc.Revision;
			}
		}
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

		gpu->FeatureLevel = get_d3d_feature_level(gpu);
		get_gpu_pcie_speed(gpu);

		if (find_duplicated_gpu(info, &gpu->Bdf, &gpu->DeviceIds.DeviceIds))
			NWL_Debug(GDID3D, "Using duplicated adapter as fallback [%04X-%04X SUBSYS %08X REV %02X ] %s",
				gpu->DeviceIds.DeviceIds.VendorID, gpu->DeviceIds.DeviceIds.DeviceID,
				gpu->DeviceIds.DeviceIds.SubSystemID, gpu->DeviceIds.DeviceIds.RevisionID, gpu->Name);

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
	for (ULONG i = 0; i < ctx->Count; i++)
		if (ctx->Adapter[i].DxgiAdapter)
			ctx->Adapter[i].DxgiAdapter->lpVtbl->Release(ctx->Adapter[i].DxgiAdapter);
	if (ctx->DxgiFactory)
		ctx->DxgiFactory->lpVtbl->Release(ctx->DxgiFactory);
	if (ctx->Dxgi)
		FreeLibrary(ctx->Dxgi);
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

static void
fill_missing_gpu_info(NWLIB_GPU_DEV* info, const NWLIB_GPU_DEV* fallback)
{
	if (info->CurSpeed.Gen == 0)
		info->CurSpeed.Gen = fallback->CurSpeed.Gen;
	if (info->CurSpeed.Lanes == 0)
		info->CurSpeed.Lanes = fallback->CurSpeed.Lanes;
	if (info->MaxSpeed.Gen == 0)
		info->MaxSpeed.Gen = fallback->MaxSpeed.Gen;
	if (info->MaxSpeed.Lanes == 0)
		info->MaxSpeed.Lanes = fallback->MaxSpeed.Lanes;
	if (info->DxVersion == 0)
		info->DxVersion = fallback->DxVersion;

	if (info->TotalMemory == 0)
		info->TotalMemory = fallback->TotalMemory;
	if (info->FreeMemory == 0)
		info->FreeMemory = fallback->FreeMemory;
	if (info->UsedMemory == 0)
		info->UsedMemory = fallback->UsedMemory;
	if (info->MemoryPercent == 0)
		info->MemoryPercent = fallback->MemoryPercent;

	if (info->UsagePercent == 0.0)
		info->UsagePercent = fallback->UsagePercent;
	if (info->Temperature == 0.0)
		info->Temperature = fallback->Temperature;
	if (info->FanSpeed == 0)
		info->FanSpeed = fallback->FanSpeed;
}

static void
fill_d3d_gpu_info(struct D3D_GPU_CTX* ctx, struct D3D_GPU_DATA* gpu, NWLIB_GPU_DEV* info)
{
	strncpy_s(info->Name, MAX_GPU_STR, gpu->Name, _TRUNCATE);
	info->VendorId = gpu->DeviceIds.DeviceIds.VendorID;
	info->DeviceId = gpu->DeviceIds.DeviceIds.DeviceID;
	info->Subsys = gpu->DeviceIds.DeviceIds.SubSystemID;
	info->RevId = gpu->DeviceIds.DeviceIds.RevisionID;
	info->PciBus = gpu->Bdf.BusNumber;
	info->PciDevice = gpu->Bdf.DeviceNumber;
	info->PciFunction = gpu->Bdf.FunctionNumber;
	info->DxVersion = (uint32_t)gpu->FeatureLevel;
	info->CurSpeed = gpu->CurSpeed;
	info->MaxSpeed = gpu->MaxSpeed;

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

static uint32_t d3d_gpu_get(void* data, NWLIB_GPU_DEV* dev, uint32_t dev_count)
{
	uint32_t count = 0;
	struct D3D_GPU_CTX* ctx = data;

	if (ctx == NULL || ctx->Count == 0)
		return 0;

	for (UINT i = 0; i < ctx->Count; i++)
	{
		if (ctx->Adapter[i].Valid == FALSE)
			continue;

		struct D3D_GPU_DATA* gpu = &ctx->Adapter[i];
		NWLIB_GPU_DEV fallback = { 0 };
		fill_d3d_gpu_info(ctx, gpu, &fallback);

		NWLIB_GPU_DEV* info = find_duplicated_gpu(ctx->Info, &gpu->Bdf, &gpu->DeviceIds.DeviceIds);
		if (info)
		{
			fill_missing_gpu_info(info, &fallback);
			continue;
		}

		if (count >= dev_count)
			continue;
		info = &dev[count];
		*info = fallback;
		count++;
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
	for (ULONG i = 0; i < ctx->Count; i++)
		if (ctx->Adapter[i].DxgiAdapter)
			ctx->Adapter[i].DxgiAdapter->lpVtbl->Release(ctx->Adapter[i].DxgiAdapter);
	if (ctx->DxgiFactory)
		ctx->DxgiFactory->lpVtbl->Release(ctx->DxgiFactory);
	if (ctx->Dxgi)
		FreeLibrary(ctx->Dxgi);
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
