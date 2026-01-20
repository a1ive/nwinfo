// SPDX-License-Identifier: Unlicense

#include "nvapi.h"

#ifdef _WIN64
#define NVAPI_DLL_NAME L"nvapi64.dll"
#else
#define NVAPI_DLL_NAME L"nvapi.dll"
#endif

typedef INT_PTR(__cdecl* NVAPI_PROC)();
typedef NVAPI_PROC(__cdecl* NVAPI_QUERYINTERFACE)(unsigned int);

static struct NVAPI_DLL_HANDLE
{
	HINSTANCE Dll;
	NVAPI_QUERYINTERFACE NvApiQuery;
} gNvDll;

NVAPI_INTERFACE NvAPI_Initialize(void)
{
	NVAPI_INTERFACE(__cdecl *pfn)(void) = NULL;

	ZeroMemory(&gNvDll, sizeof(struct NVAPI_DLL_HANDLE));
	gNvDll.Dll = LoadLibraryW(NVAPI_DLL_NAME);
	if (gNvDll.Dll == NULL)
		return NVAPI_LIBRARY_NOT_FOUND;
	gNvDll.NvApiQuery = (NVAPI_QUERYINTERFACE)GetProcAddress(gNvDll.Dll, "nvapi_QueryInterface");
	if (gNvDll.NvApiQuery == NULL)
		goto fail;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x0150e828);
	if (pfn == NULL)
		goto fail;
	return pfn();
fail:
	if (gNvDll.Dll)
		FreeLibrary(gNvDll.Dll);
	ZeroMemory(&gNvDll, sizeof(struct NVAPI_DLL_HANDLE));
	return NVAPI_NO_IMPLEMENTATION;
}

NVAPI_INTERFACE NvAPI_Unload(void)
{
	NVAPI_INTERFACE(__cdecl * pfn)(void) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0xd22bdd7e);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	NvAPI_Status rc = pfn();
	if (rc != NVAPI_OK)
		return rc;
	FreeLibrary(gNvDll.Dll);
	ZeroMemory(&gNvDll, sizeof(struct NVAPI_DLL_HANDLE));
	return NVAPI_OK;
}

// Deprecated
NVAPI_INTERFACE NvAPI_GPU_GetMemoryInfo(NvPhysicalGpuHandle hPhysicalGpu, NV_DISPLAY_DRIVER_MEMORY_INFO* pMemoryInfo)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NV_DISPLAY_DRIVER_MEMORY_INFO*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x07f9b368);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, pMemoryInfo);
}

NVAPI_INTERFACE NvAPI_GPU_GetMemoryInfoEx(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_MEMORY_INFO_EX* pMemoryInfo)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NV_GPU_MEMORY_INFO_EX*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0xc0599498);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, pMemoryInfo);
}

NVAPI_INTERFACE NvAPI_EnumPhysicalGPUs(NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32* pGpuCount)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0xe5ac921f);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(nvGPUHandle, pGpuCount);
}

NVAPI_INTERFACE NvAPI_EnumNvidiaDisplayHandle(NvU32 thisEnum, NvDisplayHandle* pNvDispHandle)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvU32, NvDisplayHandle*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x9abdd40d);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(thisEnum, pNvDispHandle);
}

NVAPI_INTERFACE NvAPI_GetPhysicalGPUsFromDisplay(NvDisplayHandle hNvDisp, NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32* pGpuCount)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvDisplayHandle, NvPhysicalGpuHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x34ef9506);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hNvDisp, nvGPUHandle, pGpuCount);
}

NVAPI_INTERFACE NvAPI_GPU_GetFullName(NvPhysicalGpuHandle hPhysicalGpu, NvAPI_ShortString szName)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NvAPI_ShortString) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0xceee8e9f);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, szName);
}

NVAPI_INTERFACE NvAPI_GPU_GetPCIIdentifiers(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pDeviceId, NvU32* pSubSystemId, NvU32* pRevisionId, NvU32* pExtDeviceId)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NvU32*, NvU32*, NvU32*, NvU32*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x2ddfb66e);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, pDeviceId, pSubSystemId, pRevisionId, pExtDeviceId);
}

NVAPI_INTERFACE NvAPI_GPU_GetGPUType(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_TYPE* pGpuType)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NV_GPU_TYPE*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0xc33baeb1);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, pGpuType);
}

NVAPI_INTERFACE NvAPI_GPU_GetBusType(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_BUS_TYPE* pBusType)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NV_GPU_BUS_TYPE*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x1bb18724);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, pBusType);
}

NVAPI_INTERFACE NvAPI_GPU_GetBusId(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pBusId)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NvU32*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x1be0b8e5);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, pBusId);
}

NVAPI_INTERFACE NvAPI_GPU_GetBusSlotId(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pBusSlotId)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NvU32*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x2a0a350f);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, pBusSlotId);
}

NVAPI_INTERFACE NvAPI_GPU_GetVbiosRevision(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pBiosRevision)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NvU32*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0xacc3da0a);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, pBiosRevision);
}

NVAPI_INTERFACE NvAPI_GPU_GetVbiosOEMRevision(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pBiosRevision)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NvU32*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x2d43fb31);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, pBiosRevision);
}

NVAPI_INTERFACE NvAPI_GPU_GetVbiosVersionString(NvPhysicalGpuHandle hPhysicalGpu, NvAPI_ShortString szBiosRevision)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NvAPI_ShortString) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0xa561fd7d);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, szBiosRevision);
}

// Deprecated
NVAPI_INTERFACE NvAPI_GPU_GetPstatesInfoEx(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_PERF_PSTATES_INFO* pPerfPstatesInfo, NvU32 inputFlags)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NV_GPU_PERF_PSTATES_INFO*, NvU32) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x843c0256);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, pPerfPstatesInfo, inputFlags);
}

NVAPI_INTERFACE NvAPI_GPU_GetPstates20(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_PERF_PSTATES20_INFO* pPstatesInfo)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NV_GPU_PERF_PSTATES20_INFO*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x6ff81213);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, pPstatesInfo);
}

NVAPI_INTERFACE NvAPI_GPU_GetCurrentPstate(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_PERF_PSTATE_ID* pCurrentPstate)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NV_GPU_PERF_PSTATE_ID*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x927da4f6);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, pCurrentPstate);
}

NVAPI_INTERFACE NvAPI_GPU_GetDynamicPstatesInfoEx(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_DYNAMIC_PSTATES_INFO_EX* pDynamicPstatesInfoEx)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NV_GPU_DYNAMIC_PSTATES_INFO_EX*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x60ded2ed);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, pDynamicPstatesInfoEx);
}

NVAPI_INTERFACE NvAPI_GPU_GetThermalSettings(NvPhysicalGpuHandle hPhysicalGpu, NvU32 sensorIndex, NV_GPU_THERMAL_SETTINGS* pThermalSettings)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NvU32, NV_GPU_THERMAL_SETTINGS*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0xe3640a56);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGpu, sensorIndex, pThermalSettings);
}

NVAPI_INTERFACE NvAPI_GPU_GetAllClockFrequencies(NvPhysicalGpuHandle hPhysicalGPU, NV_GPU_CLOCK_FREQUENCIES* pClkFreqs)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NV_GPU_CLOCK_FREQUENCIES*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0xdcb616c3);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGPU, pClkFreqs);
}

NVAPI_INTERFACE NvAPI_GPU_GetTachReading(NvPhysicalGpuHandle hPhysicalGPU, NvU32* pValue)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NvU32*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x5f608315);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGPU, pValue);
}

// Removed
NVAPI_INTERFACE NvAPI_GPU_GetUsages(NvPhysicalGpuHandle hPhysicalGPU, NV_USAGES* pUsage)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NV_USAGES*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x189a1fdf);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGPU, pUsage);
}

// Removed
NVAPI_INTERFACE NvAPI_GPU_GetMemoryInfo2(NvDisplayHandle hNvDisp, NV_DISPLAY_DRIVER_MEMORY_INFO_V2* pMemoryInfo)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvDisplayHandle, NV_DISPLAY_DRIVER_MEMORY_INFO_V2*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x774aa982);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hNvDisp, pMemoryInfo);
}

// Removed
NVAPI_INTERFACE NvAPI_GPU_ClientFanCoolersGetStatus(NvPhysicalGpuHandle hPhysicalGPU, NV_FAN_COOLER_STATUS* pStatus)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NV_FAN_COOLER_STATUS*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x35aed5e8);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGPU, pStatus);
}

// Removed
NVAPI_INTERFACE NvAPI_GPU_GetThermalSensors(NvPhysicalGpuHandle hPhysicalGPU, NV_THERMAL_SENSORS* pSensors)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NV_THERMAL_SENSORS*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x65fe3aad);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGPU, pSensors);
}

// Removed
NVAPI_INTERFACE NvAPI_GPU_ClientPowerTopologyGetStatus(NvPhysicalGpuHandle hPhysicalGPU, NV_POWER_TOPOLOGY* pPowerTopology)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NV_POWER_TOPOLOGY*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x0edcf624e);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGPU, pPowerTopology);
}

NVAPI_INTERFACE NvAPI_GPU_ClientVoltRailsGetStatus(NvPhysicalGpuHandle hPhysicalGPU, NV_GPU_CLIENT_VOLT_RAILS_STATUS_V1* pStatus)
{
	NVAPI_INTERFACE(__cdecl * pfn)(NvPhysicalGpuHandle, NV_GPU_CLIENT_VOLT_RAILS_STATUS_V1*) = NULL;
	*(NVAPI_PROC*)&pfn = gNvDll.NvApiQuery(0x465f9bcf);
	if (pfn == NULL)
		return NVAPI_NO_IMPLEMENTATION;
	return pfn(hPhysicalGPU, pStatus);
}
