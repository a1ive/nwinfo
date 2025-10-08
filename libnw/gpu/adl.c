// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include "adl.h"

typedef int (*ADL2_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int, void**);
typedef int (*ADL2_MAIN_CONTROL_DESTROY)(void*);
typedef int (*ADL2_ADAPTER_NUMBEROFADAPTERS_GET)(void*, int*);
typedef int (*ADL2_ADAPTER_ADAPTERINFO_GET)(void*, LPAdapterInfo, int);
typedef int (*ADL2_GCNASICINFO_GET)(void*, int, ADLGcnInfo*);
typedef int (*ADL2_ADAPTER_ACTIVE_GET)(void*, int, int*);
typedef int (*ADL2_OVERDRIVE_CAPS)(void*, int, int*, int*, int*);
typedef int (*ADL2_OVERDRIVE5_ODPARAMETERS_GET)(void*, int, ADLODParameters*);
typedef int (*ADL2_OVERDRIVE5_TEMPERATURE_GET)(void*, int, int, ADLTemperature*);
typedef int (*ADL2_OVERDRIVE5_FANSPEED_GET)(void*, int, int, ADLFanSpeedValue*);
typedef int (*ADL2_OVERDRIVE5_CURRENTACTIVITY_GET)(void*, int, ADLPMActivity*);
typedef int (*ADL2_OVERDRIVE6_CAPABILITIES_GET)(void*, int, ADLOD6Capabilities*);
typedef int (*ADL2_OVERDRIVE6_CURRENTPOWER_GET)(void*, int, ADLODNCurrentPowerType, int*);
typedef int (*ADL2_OVERDRIVEN_TEMPERATURE_GET)(void*, int, int, int*);
typedef int (*ADL2_OVERDRIVEN_PERFORMANCESTATUS_GET)(void*, int, ADLODNPerformanceStatus*);
typedef int (*ADL2_ADAPTER_DEDICATEDVRAMUSAGE_GET)(void*, int, int*);
typedef int (*ADL2_NEW_QUERYPMLOGDATA_GET)(void*, int, ADLPMLogDataOutput*);
typedef int (*ADL2_DEVICE_PMLOG_DEVICE_CREATE)(void*, int, unsigned int*);
typedef int (*ADL2_DEVICE_PMLOG_DEVICE_DESTROY)(void*, unsigned int);
typedef int (*ADL2_ADAPTER_PMLOG_SUPPORT_GET)(void*, int, ADLPMLogSupportInfo*);
typedef int (*ADL2_ADAPTER_PMLOG_START)(void*, int, ADLPMLogStartInput*, ADLPMLogStartOutput*, unsigned int);
typedef int (*ADL2_ADAPTER_PMLOG_STOP)(void*, int, unsigned int);
typedef int (*ADL2_ADAPTER_MEMORYINFOX4_GET)(void*, int, ADLMemoryInfoX4*);
typedef int (*ADL2_ADAPTER_MEMORYINFO2_GET)(void*, int, ADLMemoryInfo2*);

HINSTANCE g_DLL = NULL;

static void* __stdcall ADL_Alloc(int size)
{
	return malloc(size);
}

int ADL2_Main_Control_Create(int enumConnectedAdapters, void** ppContext)
{
	if (!g_DLL)
		g_DLL = LoadLibraryW(L"atiadlxx.dll");
	if (!g_DLL)
		return ADL_ERR;
	ADL2_MAIN_CONTROL_CREATE pfn = (ADL2_MAIN_CONTROL_CREATE)GetProcAddress(g_DLL, "ADL2_Main_Control_Create");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(ADL_Alloc, enumConnectedAdapters, ppContext);
}

int ADL2_Main_Control_Destroy(void* pContext)
{
	if (!g_DLL)
		return ADL_ERR;
	int rc = ADL_ERR_NOT_SUPPORTED;
	ADL2_MAIN_CONTROL_DESTROY pfn = (ADL2_MAIN_CONTROL_DESTROY)GetProcAddress(g_DLL, "ADL2_Main_Control_Destroy");
	if (pfn)
		rc = pfn(pContext);
	FreeLibrary(g_DLL);
	g_DLL = NULL;
	return rc;
}

int ADL2_Adapter_NumberOfAdapters_Get(void* pContext, int* numAdapters)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_ADAPTER_NUMBEROFADAPTERS_GET pfn = (ADL2_ADAPTER_NUMBEROFADAPTERS_GET)GetProcAddress(g_DLL, "ADL2_Adapter_NumberOfAdapters_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, numAdapters);
}

int ADL2_Adapter_AdapterInfo_Get(void* pContext, LPAdapterInfo lpInfo, int iInputSize)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_ADAPTER_ADAPTERINFO_GET pfn = (ADL2_ADAPTER_ADAPTERINFO_GET)GetProcAddress(g_DLL, "ADL2_Adapter_AdapterInfo_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, lpInfo, iInputSize);
}

int ADL2_GcnAsicInfo_Get(void* pContext, int iAdapterIndex, ADLGcnInfo* lpGcnInfo)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_GCNASICINFO_GET pfn = (ADL2_GCNASICINFO_GET)GetProcAddress(g_DLL, "ADL2_GcnAsicInfo_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, lpGcnInfo);
}

int ADL2_Adapter_Active_Get(void* pContext, int iAdapterIndex, int* lpStatus)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_ADAPTER_ACTIVE_GET pfn = (ADL2_ADAPTER_ACTIVE_GET)GetProcAddress(g_DLL, "ADL2_Adapter_Active_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, lpStatus);
}

int ADL2_Overdrive_Caps(void* pContext, int iAdapterIndex, int* odSupported, int* odEnabled, int* odVersion)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_OVERDRIVE_CAPS pfn = (ADL2_OVERDRIVE_CAPS)GetProcAddress(g_DLL, "ADL2_Overdrive_Caps");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, odSupported, odEnabled, odVersion);
}
int ADL2_Overdrive5_ODParameters_Get(void* pContext, int iAdapterIndex, ADLODParameters* lpOdParameters)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_OVERDRIVE5_ODPARAMETERS_GET pfn = (ADL2_OVERDRIVE5_ODPARAMETERS_GET)GetProcAddress(g_DLL, "ADL2_Overdrive5_ODParameters_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, lpOdParameters);
}


int ADL2_Overdrive5_Temperature_Get(void* pContext, int iAdapterIndex, int iThermalControllerIndex, ADLTemperature* lpTemperature)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_OVERDRIVE5_TEMPERATURE_GET pfn = (ADL2_OVERDRIVE5_TEMPERATURE_GET)GetProcAddress(g_DLL, "ADL2_Overdrive5_Temperature_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, iThermalControllerIndex, lpTemperature);
}

int ADL2_Overdrive5_FanSpeed_Get(void* pContext, int iAdapterIndex, int iThermalControllerIndex, ADLFanSpeedValue* lpFanSpeedValue)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_OVERDRIVE5_FANSPEED_GET pfn = (ADL2_OVERDRIVE5_FANSPEED_GET)GetProcAddress(g_DLL, "ADL2_Overdrive5_FanSpeed_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, iThermalControllerIndex, lpFanSpeedValue);
}

int ADL2_Overdrive5_CurrentActivity_Get(void* pContext, int iAdapterIndex, ADLPMActivity* lpActivity)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_OVERDRIVE5_CURRENTACTIVITY_GET pfn = (ADL2_OVERDRIVE5_CURRENTACTIVITY_GET)GetProcAddress(g_DLL, "ADL2_Overdrive5_CurrentActivity_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, lpActivity);
}

int ADL2_Overdrive6_Capabilities_Get(void* pContext, int iAdapterIndex, ADLOD6Capabilities* lpODCapabilities)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_OVERDRIVE6_CAPABILITIES_GET pfn = (ADL2_OVERDRIVE6_CAPABILITIES_GET)GetProcAddress(g_DLL, "ADL2_Overdrive6_Capabilities_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, lpODCapabilities);
}

int ADL2_Overdrive6_CurrentPower_Get(void* pContext, int iAdapterIndex, ADLODNCurrentPowerType powerType, int* lpCurrentValue)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_OVERDRIVE6_CURRENTPOWER_GET pfn = (ADL2_OVERDRIVE6_CURRENTPOWER_GET)GetProcAddress(g_DLL, "ADL2_Overdrive6_CurrentPower_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, powerType, lpCurrentValue);
}

int ADL2_OverdriveN_Temperature_Get(void* pContext, int iAdapterIndex, int iTemperatureType, int* lpTemperature)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_OVERDRIVEN_TEMPERATURE_GET pfn = (ADL2_OVERDRIVEN_TEMPERATURE_GET)GetProcAddress(g_DLL, "ADL2_OverdriveN_Temperature_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, iTemperatureType, lpTemperature);
}

int ADL2_OverdriveN_PerformanceStatus_Get(void* pContext, int iAdapterIndex, ADLODNPerformanceStatus* lpOdPerformanceStatus)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_OVERDRIVEN_PERFORMANCESTATUS_GET pfn = (ADL2_OVERDRIVEN_PERFORMANCESTATUS_GET)GetProcAddress(g_DLL, "ADL2_OverdriveN_PerformanceStatus_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, lpOdPerformanceStatus);
}

int ADL2_Adapter_DedicatedVRAMUsage_Get(void* pContext, int iAdapterIndex, int* lpVRAMUsageInMB)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_ADAPTER_DEDICATEDVRAMUSAGE_GET pfn = (ADL2_ADAPTER_DEDICATEDVRAMUSAGE_GET)GetProcAddress(g_DLL, "ADL2_Adapter_DedicatedVRAMUsage_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, lpVRAMUsageInMB);
}


int ADL2_New_QueryPMLogData_Get(void* pContext, int iAdapterIndex, ADLPMLogDataOutput* lpPMLogDataOutput)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_NEW_QUERYPMLOGDATA_GET pfn = (ADL2_NEW_QUERYPMLOGDATA_GET)GetProcAddress(g_DLL, "ADL2_New_QueryPMLogData_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, lpPMLogDataOutput);
}

int ADL2_Device_PMLog_Device_Create(void* pContext, int iAdapterIndex, unsigned int* pDevice)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_DEVICE_PMLOG_DEVICE_CREATE pfn = (ADL2_DEVICE_PMLOG_DEVICE_CREATE)GetProcAddress(g_DLL, "ADL2_Device_PMLog_Device_Create");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, pDevice);
}

int ADL2_Device_PMLog_Device_Destroy(void* pContext, unsigned int iDevice)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_DEVICE_PMLOG_DEVICE_DESTROY pfn = (ADL2_DEVICE_PMLOG_DEVICE_DESTROY)GetProcAddress(g_DLL, "ADL2_Device_PMLog_Device_Destroy");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iDevice);
}

int ADL2_Adapter_PMLog_Support_Get(void* pContext, int iAdapterIndex, ADLPMLogSupportInfo* pPMLogSupportInfo)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_ADAPTER_PMLOG_SUPPORT_GET pfn = (ADL2_ADAPTER_PMLOG_SUPPORT_GET)GetProcAddress(g_DLL, "ADL2_Adapter_PMLog_Support_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, pPMLogSupportInfo);
}

int ADL2_Adapter_PMLog_Start(void* pContext, int iAdapterIndex, ADLPMLogStartInput* pPMLogStartInput, ADLPMLogStartOutput* pPMLogStartOutput, unsigned int iDevice)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_ADAPTER_PMLOG_START pfn = (ADL2_ADAPTER_PMLOG_START)GetProcAddress(g_DLL, "ADL2_Adapter_PMLog_Start");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, pPMLogStartInput, pPMLogStartOutput, iDevice);
}

int ADL2_Adapter_PMLog_Stop(void* pContext, int iAdapterIndex, unsigned int iDevice)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_ADAPTER_PMLOG_STOP pfn = (ADL2_ADAPTER_PMLOG_STOP)GetProcAddress(g_DLL, "ADL2_Adapter_PMLog_Stop");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, iDevice);
}

int ADL2_Adapter_MemoryInfoX4_Get(void* pContext, int iAdapterIndex, ADLMemoryInfoX4* lpMemoryInfoX4)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_ADAPTER_MEMORYINFOX4_GET pfn = (ADL2_ADAPTER_MEMORYINFOX4_GET)GetProcAddress(g_DLL, "ADL2_Adapter_MemoryInfoX4_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, lpMemoryInfoX4);
}

int ADL2_Adapter_MemoryInfo2_Get(void* pContext, int iAdapterIndex, ADLMemoryInfo2* lpMemoryInfo2)
{
	if (!g_DLL)
		return ADL_ERR;
	ADL2_ADAPTER_MEMORYINFO2_GET pfn = (ADL2_ADAPTER_MEMORYINFO2_GET)GetProcAddress(g_DLL, "ADL2_Adapter_MemoryInfo2_Get");
	if (!pfn)
		return ADL_ERR_NOT_SUPPORTED;
	return pfn(pContext, iAdapterIndex, lpMemoryInfo2);
}
