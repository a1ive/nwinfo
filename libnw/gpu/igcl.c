// SPDX-License-Identifier: MIT
// Copyright (C) 2025 Intel Corporation

#include <windows.h>
#include <wchar.h>

#include "igcl.h"

typedef ctl_result_t(CTL_APICALL* ctl_pfnInit_t)(
	ctl_init_args_t*,
	ctl_api_handle_t*
	);

typedef ctl_result_t(CTL_APICALL* ctl_pfnClose_t)(
	ctl_api_handle_t
	);

typedef ctl_result_t(CTL_APICALL* ctl_pfnCheckDriverVersion_t)(
	ctl_device_adapter_handle_t,
	ctl_version_info_t
	);

typedef ctl_result_t(CTL_APICALL* ctl_pfnEnumerateDevices_t)(
	ctl_api_handle_t,
	uint32_t*,
	ctl_device_adapter_handle_t*
	);

typedef ctl_result_t(CTL_APICALL* ctl_pfnGetDeviceProperties_t)(
	ctl_device_adapter_handle_t,
	ctl_device_adapter_properties_t*
	);

typedef ctl_result_t(CTL_APICALL* ctl_pfnPowerTelemetryGet_t)(
	ctl_device_adapter_handle_t,
	ctl_power_telemetry_t*
	);

typedef ctl_result_t(CTL_APICALL* ctl_pfnPciGetProperties_t)(
	ctl_device_adapter_handle_t,
	ctl_pci_properties_t*
	);

typedef ctl_result_t(CTL_APICALL* ctl_pfnPciGetState_t)(
	ctl_device_adapter_handle_t,
	ctl_pci_state_t*
	);

typedef ctl_result_t(CTL_APICALL* ctl_pfnEnumMemoryModules_t)(
	ctl_device_adapter_handle_t,
	uint32_t*,
	ctl_mem_handle_t*
	);

typedef ctl_result_t(CTL_APICALL* ctl_pfnMemoryGetState_t)(
	ctl_mem_handle_t,
	ctl_mem_state_t*
	);

static HINSTANCE hinstLib = NULL;

static inline HINSTANCE GetLoaderHandle(void)
{
	return hinstLib;
}

#if defined(_WIN64)
	#define CTL_DLL_NAME L"ControlLib"
#else
	#define CTL_DLL_NAME L"ControlLib32"
#endif
#define CTL_DLL_PATH_LEN 512

static inline ctl_result_t GetControlAPIDLLPath(ctl_init_args_t* pInitArgs, wchar_t* pwcDLLPath)
{
	// Load the requested DLL based on major version in init args
	uint16_t majorVersion = CTL_MAJOR_VERSION(pInitArgs->AppVersion);

	// If caller's major version is higher than the DLL's, then simply not support the caller!
	// This is not supposed to happen as wrapper is part of the app itself which includes igcl_api.h with right major version
	if (majorVersion > CTL_IMPL_MAJOR_VERSION)
		return CTL_RESULT_ERROR_UNSUPPORTED_VERSION;

#if (CTL_IMPL_MAJOR_VERSION > 1)
	if (majorVersion > 1)
		swprintf(pwcDLLPath,CTL_DLL_PATH_LEN,L"%s%d.dll", CTL_DLL_NAME, majorVersion);
	else // just control_api.dll
		swprintf(pwcDLLPath,CTL_DLL_PATH_LEN,L"%s.dll", CTL_DLL_NAME);
#else
	swprintf(pwcDLLPath,CTL_DLL_PATH_LEN,L"%s.dll", CTL_DLL_NAME);
#endif

	return CTL_RESULT_SUCCESS;
}

/**
* @brief Control Api Init
* 
* @details
*     - Control Api Init
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pInitDesc`
*         + `nullptr == phAPIHandle`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
IGCL_Init(
	ctl_init_args_t* pInitDesc,                     ///< [in][out] App's control API version
	ctl_api_handle_t* phAPIHandle                   ///< [in][out][release] Control API handle
	)
{
	ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
	
	// special code - only for IGCL_Init()
	if (NULL == hinstLib)
	{
		wchar_t dllPath[CTL_DLL_PATH_LEN];

		result = GetControlAPIDLLPath(pInitDesc, dllPath);
		if (result == CTL_RESULT_SUCCESS)
		{
			DWORD dwFlags = LOAD_LIBRARY_SEARCH_SYSTEM32;
			hinstLib = LoadLibraryExW(dllPath, NULL, dwFlags);
			if (NULL == hinstLib)
				result = CTL_RESULT_ERROR_LOAD;
		}
	}

	HINSTANCE hinstLibPtr = GetLoaderHandle();

	if (NULL != hinstLibPtr)
	{
		ctl_pfnInit_t pfnInit = (ctl_pfnInit_t)GetProcAddress(hinstLibPtr, "ctlInit");
		if (pfnInit)
			result = pfnInit(pInitDesc, phAPIHandle);
	}

	return result;
}

/**
* @brief Control Api Destroy
* 
* @details
*     - Control Api Close
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hAPIHandle`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
IGCL_Close(
	ctl_api_handle_t hAPIHandle                     ///< [in][release] Control API handle obtained during init call
	)
{
	ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;

	HINSTANCE hinstLibPtr = GetLoaderHandle();

	if (NULL != hinstLibPtr)
	{
		ctl_pfnClose_t pfnClose = (ctl_pfnClose_t)GetProcAddress(hinstLibPtr, "ctlClose");
		if (pfnClose)
			result = pfnClose(hAPIHandle);
	}

	// special code - only for IGCL_Close()
	// might get CTL_RESULT_SUCCESS_STILL_OPEN_BY_ANOTHER_CALLER
	// if its open by another caller do not free the instance handle
	if(result == CTL_RESULT_SUCCESS)
	{
		if (NULL != hinstLib)
		{
			FreeLibrary(hinstLib);
			hinstLib = NULL;
		}
	}

	return result;
}

/**
* @brief Check Driver version
* 
* @details
*     - The application checks driver version
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceAdapter`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
IGCL_CheckDriverVersion(
	ctl_device_adapter_handle_t hDeviceAdapter,     ///< [in][release] handle to control device adapter
	ctl_version_info_t version_info                 ///< [in][release] Driver version info
	)
{
	ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;

	HINSTANCE hinstLibPtr = GetLoaderHandle();

	if (NULL != hinstLibPtr)
	{
		ctl_pfnCheckDriverVersion_t pfnCheckDriverVersion = (ctl_pfnCheckDriverVersion_t)GetProcAddress(hinstLibPtr, "ctlCheckDriverVersion");
		if (pfnCheckDriverVersion)
			result = pfnCheckDriverVersion(hDeviceAdapter, version_info);
	}
	return result;
}

/**
* @brief Enumerate devices
* 
* @details
*     - The application enumerates all device adapters in the system
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hAPIHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCount`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
IGCL_EnumerateDevices(
	ctl_api_handle_t hAPIHandle,                    ///< [in][release] Applications should pass the Control API handle returned
													///< by the CtlInit function 
	uint32_t* pCount,                               ///< [in,out][release] pointer to the number of device instances. If count
													///< is zero, then the api will update the value with the total
													///< number of drivers available. If count is non-zero, then the api will
													///< only retrieve the number of drivers.
													///< If count is larger than the number of drivers available, then the api
													///< will update the value with the correct number of drivers available.
	ctl_device_adapter_handle_t* phDevices          ///< [in,out][optional][release][range(0, *pCount)] array of driver
													///< instance handles
	)
{
	ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
	
	HINSTANCE hinstLibPtr = GetLoaderHandle();

	if (NULL != hinstLibPtr)
	{
		ctl_pfnEnumerateDevices_t pfnEnumerateDevices = (ctl_pfnEnumerateDevices_t)GetProcAddress(hinstLibPtr, "ctlEnumerateDevices");
		if (pfnEnumerateDevices)
			result = pfnEnumerateDevices(hAPIHandle, pCount, phDevices);
	}
	return result;
}

/**
* @brief Get Device Properties
* 
* @details
*     - The application gets device properties
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
IGCL_GetDeviceProperties(
	ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to control device adapter
	ctl_device_adapter_properties_t* pProperties    ///< [in,out][release] Query result for device properties
	)
{
	ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;

	HINSTANCE hinstLibPtr = GetLoaderHandle();

	if (NULL != hinstLibPtr)
	{
		ctl_pfnGetDeviceProperties_t pfnGetDeviceProperties = (ctl_pfnGetDeviceProperties_t)GetProcAddress(hinstLibPtr, "ctlGetDeviceProperties");
		if (pfnGetDeviceProperties)
			result = pfnGetDeviceProperties(hDAhandle, pProperties);
	}
	return result;
}

/**
* @brief Get Power Telemetry.
* 
* @details
*     - Limited rate of 50 ms, any call under 50 ms will return the same
*       information.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pTelemetryInfo`
*/
ctl_result_t CTL_APICALL
IGCL_PowerTelemetryGet(
	ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
	ctl_power_telemetry_t* pTelemetryInfo           ///< [out] The overclocking properties for the specified domain.
	)
{
	ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;

	HINSTANCE hinstLibPtr = GetLoaderHandle();

	if (NULL != hinstLibPtr)
	{
		ctl_pfnPowerTelemetryGet_t pfnPowerTelemetryGet = (ctl_pfnPowerTelemetryGet_t)GetProcAddress(hinstLibPtr, "ctlPowerTelemetryGet");
		if (pfnPowerTelemetryGet)
			result = pfnPowerTelemetryGet(hDeviceHandle, pTelemetryInfo);
	}
	return result;
}

/**
* @brief Get PCI properties - address, max speed
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*/
ctl_result_t CTL_APICALL
IGCL_PciGetProperties(
	ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
	ctl_pci_properties_t* pProperties               ///< [in,out] Will contain the PCI properties.
	)
{
	ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;

	HINSTANCE hinstLibPtr = GetLoaderHandle();

	if (NULL != hinstLibPtr)
	{
		ctl_pfnPciGetProperties_t pfnPciGetProperties = (ctl_pfnPciGetProperties_t)GetProcAddress(hinstLibPtr, "ctlPciGetProperties");
		if (pfnPciGetProperties)
			result = pfnPciGetProperties(hDAhandle, pProperties);
	}

	return result;
}


/**
* @brief Get current PCI state - current speed
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pState`
*/
ctl_result_t CTL_APICALL
IGCL_PciGetState(
	ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
	ctl_pci_state_t* pState                         ///< [in,out] Will contain the PCI properties.
	)
{
	ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;

	HINSTANCE hinstLibPtr = GetLoaderHandle();

	if (NULL != hinstLibPtr)
	{
		ctl_pfnPciGetState_t pfnPciGetState = (ctl_pfnPciGetState_t)GetProcAddress(hinstLibPtr, "ctlPciGetState");
		if (pfnPciGetState)
			result = pfnPciGetState(hDAhandle, pState);
	}

	return result;
}

/**
* @brief Get handle of memory modules
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCount`
*/
ctl_result_t CTL_APICALL
IGCL_EnumMemoryModules(
	ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
	uint32_t* pCount,                               ///< [in,out] pointer to the number of components of this type.
													///< if count is zero, then the driver shall update the value with the
													///< total number of components of this type that are available.
													///< if count is greater than the number of components of this type that
													///< are available, then the driver shall update the value with the correct
													///< number of components.
	ctl_mem_handle_t* phMemory                      ///< [in,out][optional][range(0, *pCount)] array of handle of components of
													///< this type.
													///< if count is less than the number of components of this type that are
													///< available, then the driver shall only retrieve that number of
													///< component handles.
	)
{
	ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;

	HINSTANCE hinstLibPtr = GetLoaderHandle();

	if (NULL != hinstLibPtr)
	{
		ctl_pfnEnumMemoryModules_t pfnEnumMemoryModules = (ctl_pfnEnumMemoryModules_t)GetProcAddress(hinstLibPtr, "ctlEnumMemoryModules");
		if (pfnEnumMemoryModules)
			result = pfnEnumMemoryModules(hDAhandle, pCount, phMemory);
	}
	return result;
}

/**
* @brief Get memory state - health, allocated
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hMemory`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pState`
*/
ctl_result_t CTL_APICALL
IGCL_MemoryGetState(
	ctl_mem_handle_t hMemory,                       ///< [in] Handle for the component.
	ctl_mem_state_t* pState                         ///< [in,out] Will contain the current health and allocated memory.
	)
{
	ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;

	HINSTANCE hinstLibPtr = GetLoaderHandle();

	if (NULL != hinstLibPtr)
	{
		ctl_pfnMemoryGetState_t pfnMemoryGetState = (ctl_pfnMemoryGetState_t)GetProcAddress(hinstLibPtr, "ctlMemoryGetState");
		if (pfnMemoryGetState)
			result = pfnMemoryGetState(hMemory, pState);
	}
	return result;
}
