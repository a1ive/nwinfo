// SPDX-License-Identifier: MIT
// Copyright (C) 2025 Intel Corporation

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

// Intel 'ctlApi' common types
#if !defined(__GNUC__)
#pragma region common
#endif

#ifndef CTL_MAKE_VERSION
#define CTL_MAKE_VERSION( _major, _minor )  (( _major << 16 )|( _minor & 0x0000ffff))
#endif // CTL_MAKE_VERSION

#ifndef CTL_MAJOR_VERSION
#define CTL_MAJOR_VERSION( _ver )  ( _ver >> 16 )
#endif // CTL_MAJOR_VERSION

#ifndef CTL_MINOR_VERSION
#define CTL_MINOR_VERSION( _ver )  ( _ver & 0x0000ffff )
#endif // CTL_MINOR_VERSION

#ifndef CTL_IMPL_MAJOR_VERSION
#define CTL_IMPL_MAJOR_VERSION  1
#endif // CTL_IMPL_MAJOR_VERSION

#ifndef CTL_IMPL_MINOR_VERSION
#define CTL_IMPL_MINOR_VERSION  1
#endif // CTL_IMPL_MINOR_VERSION

#ifndef CTL_IMPL_VERSION
#define CTL_IMPL_VERSION  CTL_MAKE_VERSION( CTL_IMPL_MAJOR_VERSION, CTL_IMPL_MINOR_VERSION )
#endif // CTL_IMPL_VERSION

#ifndef CTL_APICALL
#if defined(_WIN32)
#define CTL_APICALL  __cdecl
#else
#define CTL_APICALL
#endif // defined(_WIN32)
#endif // CTL_APICALL

#ifndef CTL_BIT
#define CTL_BIT( _i )  ( 1 << _i )
#endif // CTL_BIT

typedef uint32_t ctl_init_flags_t;
typedef enum _ctl_init_flag_t
{
	CTL_INIT_FLAG_USE_LEVEL_ZERO = CTL_BIT(0),
	CTL_INIT_FLAG_IGSC_FUL = CTL_BIT(1),
	CTL_INIT_FLAG_MAX = 0x80000000
} ctl_init_flag_t;

typedef uint32_t ctl_version_info_t;

typedef struct _ctl_api_handle_t *ctl_api_handle_t;

typedef struct _ctl_device_adapter_handle_t *ctl_device_adapter_handle_t;

typedef enum _ctl_result_t
{
	CTL_RESULT_SUCCESS = 0x00000000,
	CTL_RESULT_ERROR_NOT_INITIALIZED = 0x40000001,
	CTL_RESULT_ERROR_ALREADY_INITIALIZED = 0x40000002,
	CTL_RESULT_ERROR_DEVICE_LOST = 0x40000003,
	CTL_RESULT_ERROR_OUT_OF_HOST_MEMORY = 0x40000004,
	CTL_RESULT_ERROR_OUT_OF_DEVICE_MEMORY = 0x40000005,
	CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS = 0x40000006,
	CTL_RESULT_ERROR_NOT_AVAILABLE = 0x40000007,
	CTL_RESULT_ERROR_UNINITIALIZED = 0x40000008,
	CTL_RESULT_ERROR_UNSUPPORTED_VERSION = 0x40000009,
	CTL_RESULT_ERROR_UNSUPPORTED_FEATURE = 0x4000000a,
	CTL_RESULT_ERROR_INVALID_ARGUMENT = 0x4000000b,
	CTL_RESULT_ERROR_INVALID_NULL_HANDLE = 0x4000000d,
	CTL_RESULT_ERROR_INVALID_NULL_POINTER = 0x4000000e,
	CTL_RESULT_ERROR_NOT_IMPLEMENTED = 0x40000015,
	CTL_RESULT_ERROR_ZE_LOADER = 0x40000019,
	CTL_RESULT_ERROR_LOAD = 0x40000026,
	CTL_RESULT_ERROR_UNKNOWN = 0x4000FFFF,
} ctl_result_t;

#ifndef CTL_MAX_DEVICE_NAME_LEN
#define CTL_MAX_DEVICE_NAME_LEN  100
#endif // CTL_MAX_DEVICE_NAME_LEN

#ifndef CTL_MAX_RESERVED_SIZE
#define CTL_MAX_RESERVED_SIZE  108
#endif // CTL_MAX_RESERVED_SIZE

typedef enum _ctl_units_t
{
	CTL_UNITS_FREQUENCY_MHZ = 0,
	CTL_UNITS_OPERATIONS_GTS = 1,
	CTL_UNITS_OPERATIONS_MTS = 2,
	CTL_UNITS_VOLTAGE_VOLTS = 3,
	CTL_UNITS_POWER_WATTS = 4,
	CTL_UNITS_TEMPERATURE_CELSIUS = 5,
	CTL_UNITS_ENERGY_JOULES = 6,
	CTL_UNITS_TIME_SECONDS = 7,
	CTL_UNITS_MEMORY_BYTES = 8,
	CTL_UNITS_ANGULAR_SPEED_RPM = 9,
	CTL_UNITS_POWER_MILLIWATTS = 10,
	CTL_UNITS_PERCENT = 11,
	CTL_UNITS_MEM_SPEED_GBPS = 12,
	CTL_UNITS_VOLTAGE_MILLIVOLTS = 13,
	CTL_UNITS_BANDWIDTH_MBPS = 14,
	CTL_UNITS_UNKNOWN = 0x4800FFFF,
	CTL_UNITS_MAX
} ctl_units_t;

typedef enum _ctl_data_type_t
{
	CTL_DATA_TYPE_INT8 = 0,
	CTL_DATA_TYPE_UINT8 = 1,
	CTL_DATA_TYPE_INT16 = 2,
	CTL_DATA_TYPE_UINT16 = 3,
	CTL_DATA_TYPE_INT32 = 4,
	CTL_DATA_TYPE_UINT32 = 5,
	CTL_DATA_TYPE_INT64 = 6,
	CTL_DATA_TYPE_UINT64 = 7,
	CTL_DATA_TYPE_FLOAT = 8,
	CTL_DATA_TYPE_DOUBLE = 9,
	CTL_DATA_TYPE_STRING_ASCII = 10,
	CTL_DATA_TYPE_STRING_UTF16 = 11,
	CTL_DATA_TYPE_STRING_UTF132 = 12,
	CTL_DATA_TYPE_UNKNOWN = 0x4800FFFF,
	CTL_DATA_TYPE_MAX
} ctl_data_type_t;

typedef union _ctl_data_value_t
{
	int8_t data8;
	uint8_t datau8;
	int16_t data16;
	uint16_t datau16;
	int32_t data32;
	uint32_t datau32;
	int64_t data64;
	uint64_t datau64;
	float datafloat;
	double datadouble;
} ctl_data_value_t;

typedef struct _ctl_application_id_t
{
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t Data4[8];
} ctl_application_id_t;

typedef struct _ctl_init_args_t
{
	uint32_t Size;
	uint8_t Version;
	ctl_version_info_t AppVersion;
	ctl_init_flags_t flags;
	ctl_version_info_t SupportedVersion;
	ctl_application_id_t ApplicationUID;
} ctl_init_args_t;

ctl_result_t CTL_APICALL
IGCL_Init(
	ctl_init_args_t* pInitDesc,                     ///< [in][out] App's control API version
	ctl_api_handle_t* phAPIHandle                   ///< [in][out][release] Control API handle
	);

ctl_result_t CTL_APICALL
IGCL_Close(
	ctl_api_handle_t hAPIHandle                     ///< [in][release] Control API handle obtained during init call
	);

typedef uint32_t ctl_supported_functions_flags_t;
typedef enum _ctl_supported_functions_flag_t
{
	CTL_SUPPORTED_FUNCTIONS_FLAG_DISPLAY = CTL_BIT(0),  ///< [out] Is Display supported
	CTL_SUPPORTED_FUNCTIONS_FLAG_3D = CTL_BIT(1),       ///< [out] Is 3D supported
	CTL_SUPPORTED_FUNCTIONS_FLAG_MEDIA = CTL_BIT(2),    ///< [out] Is Media supported
	CTL_SUPPORTED_FUNCTIONS_FLAG_MAX = 0x80000000
} ctl_supported_functions_flag_t;

typedef struct _ctl_firmware_version_t
{
	uint64_t major_version;
	uint64_t minor_version;
	uint64_t build_number;
} ctl_firmware_version_t;

typedef enum _ctl_device_type_t
{
	CTL_DEVICE_TYPE_GRAPHICS = 1,
	CTL_DEVICE_TYPE_SYSTEM = 2,
	CTL_DEVICE_TYPE_MAX
} ctl_device_type_t;

typedef uint32_t ctl_adapter_properties_flags_t;
typedef enum _ctl_adapter_properties_flag_t
{
	CTL_ADAPTER_PROPERTIES_FLAG_INTEGRATED = CTL_BIT(0),    ///< [out] Is Integrated Graphics adapter
	CTL_ADAPTER_PROPERTIES_FLAG_LDA_PRIMARY = CTL_BIT(1),   ///< [out] Is Primary (Lead) adapter in a Linked Display Adapter chain
	CTL_ADAPTER_PROPERTIES_FLAG_LDA_SECONDARY = CTL_BIT(2), ///< [out] Is Secondary (Linked) adapter in a Linked Display Adapter chain
	CTL_ADAPTER_PROPERTIES_FLAG_MAX = 0x80000000
} ctl_adapter_properties_flag_t;

typedef struct _ctl_adapter_bdf_t
{
	uint8_t bus;
	uint8_t device;
	uint8_t function;
} ctl_adapter_bdf_t;

typedef struct _ctl_device_adapter_properties_t
{
	uint32_t Size;                                  ///< [in] size of this structure
	uint8_t Version;                                ///< [in] version of this structure
	void* pDeviceID;                                ///< [in,out] OS specific Device ID
	uint32_t device_id_size;                        ///< [in] size of the device ID
	ctl_device_type_t device_type;                  ///< [out] Device Type
	ctl_supported_functions_flags_t supported_subfunction_flags;///< [out] Supported functions
	uint64_t driver_version;                        ///< [out] Driver version
	ctl_firmware_version_t firmware_version;        ///< [out] Global Firmware version for discrete adapters. Not implemented
	uint32_t pci_vendor_id;                         ///< [out] PCI Vendor ID
	uint32_t pci_device_id;                         ///< [out] PCI Device ID
	uint32_t rev_id;                                ///< [out] PCI Revision ID
	uint32_t num_eus_per_sub_slice;                 ///< [out] Number of EUs per sub-slice
	uint32_t num_sub_slices_per_slice;              ///< [out] Number of sub-slices per slice
	uint32_t num_slices;                            ///< [out] Number of slices
	char name[CTL_MAX_DEVICE_NAME_LEN];             ///< [out] Device name
	ctl_adapter_properties_flags_t graphics_adapter_properties; ///< [out] Graphics Adapter Properties
	uint32_t Frequency;                             ///< [out] This represents the average frequency an end user may see in the
													///< typical gaming workload. Also referred as Graphics Clock. Supported
													///< only for Version > 0
	uint16_t pci_subsys_id;                         ///< [out] PCI SubSys ID, Supported only for Version > 1
	uint16_t pci_subsys_vendor_id;                  ///< [out] PCI SubSys Vendor ID, Supported only for Version > 1
	ctl_adapter_bdf_t adapter_bdf;                  ///< [out] Pci Bus, Device, Function. Supported only for Version > 1
	uint32_t num_xe_cores;                          ///< [out] Number of Xe Cores. Supported only for Version > 2
	char reserved[CTL_MAX_RESERVED_SIZE];           ///< [out] Reserved
} ctl_device_adapter_properties_t;

ctl_result_t CTL_APICALL
IGCL_CheckDriverVersion(
	ctl_device_adapter_handle_t hDeviceAdapter,     ///< [in][release] handle to control device adapter
	ctl_version_info_t version_info                 ///< [in][release] Driver version info
	);

ctl_result_t CTL_APICALL
IGCL_EnumerateDevices(
	ctl_api_handle_t hAPIHandle,					///< [in][release] Applications should pass the Control API handle returned
													///< by the CtlInit function 
	uint32_t* pCount,								///< [in,out][release] pointer to the number of device instances. If count
													///< is zero, then the api will update the value with the total
													///< number of drivers available. If count is non-zero, then the api will
													///< only retrieve the number of drivers.
													///< If count is larger than the number of drivers available, then the api
													///< will update the value with the correct number of drivers available.
	ctl_device_adapter_handle_t* phDevices			///< [in,out][optional][release][range(0, *pCount)] array of driver
													///< instance handles
	);

ctl_result_t CTL_APICALL
IGCL_GetDeviceProperties(
	ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to control device adapter
	ctl_device_adapter_properties_t* pProperties    ///< [in,out][release] Query result for device properties
	);

typedef struct _ctl_oc_telemetry_item_t
{
	bool bSupported;                                ///< [out] Indicates if the value is supported.
	ctl_units_t units;                              ///< [out] Indicates the units of the value.
	ctl_data_type_t type;                           ///< [out] Indicates the data type.
	ctl_data_value_t value;                         ///< [out] The value of type ::ctl_data_type_t and units ::ctl_units_t.
} ctl_oc_telemetry_item_t;

#ifndef CTL_PSU_COUNT
#define CTL_PSU_COUNT  5
#endif // CTL_PSU_COUNT

typedef enum _ctl_psu_type_t
{
	CTL_PSU_TYPE_PSU_NONE = 0,                      ///< Type of the PSU is unknown.
	CTL_PSU_TYPE_PSU_PCIE = 1,                      ///< Type of the PSU is PCIe
	CTL_PSU_TYPE_PSU_6PIN = 2,                      ///< Type of the PSU is 6 PIN
	CTL_PSU_TYPE_PSU_8PIN = 3,                      ///< Type of the PSU is 8 PIN
	CTL_PSU_TYPE_MAX
} ctl_psu_type_t;

typedef struct _ctl_psu_info_t
{
	bool bSupported;                                ///< [out] Indicates if this PSU entry is supported.
	ctl_psu_type_t psuType;                         ///< [out] Type of the PSU.
	ctl_oc_telemetry_item_t energyCounter;          ///< [out] Snapshot of the monotonic energy counter maintained by hardware.
													///< It measures the total energy consumed this power source. By taking the
													///< delta between two snapshots and dividing by the delta time in seconds,
													///< an application can compute the average power.
	ctl_oc_telemetry_item_t voltage;                ///< [out] Instantaneous snapshot of the voltage of this power source.
} ctl_psu_info_t;

#ifndef CTL_FAN_COUNT
#define CTL_FAN_COUNT  5
#endif // CTL_FAN_COUNT

typedef struct _ctl_power_telemetry_t
{
	uint32_t Size;                                  ///< [in] size of this structure
	uint8_t Version;                                ///< [in] version of this structure
	ctl_oc_telemetry_item_t timeStamp;              ///< [out] Snapshot of the timestamp counter that measures the total time
													///< since Jan 1, 1970 UTC. It is a decimal value in seconds with a minimum
													///< accuracy of 1 millisecond.
	ctl_oc_telemetry_item_t gpuEnergyCounter;       ///< [out] Snapshot of the monotonic energy counter maintained by hardware.
													///< It measures the total energy consumed by the GPU chip. By taking the
													///< delta between two snapshots and dividing by the delta time in seconds,
													///< an application can compute the average power.
	ctl_oc_telemetry_item_t gpuVoltage;             ///< [out] Instantaneous snapshot of the voltage feeding the GPU chip. It
													///< is measured at the power supply output - chip input will be lower.
	ctl_oc_telemetry_item_t gpuCurrentClockFrequency;   ///< [out] Instantaneous snapshot of the GPU chip frequency.
	ctl_oc_telemetry_item_t gpuCurrentTemperature;  ///< [out] Instantaneous snapshot of the GPU chip temperature, read from
													///< the sensor reporting the highest value.
	ctl_oc_telemetry_item_t globalActivityCounter;  ///< [out] Snapshot of the monotonic global activity counter. It measures
													///< the time in seconds (accurate down to 1 millisecond) that any GPU
													///< engine is busy. By taking the delta between two snapshots and dividing
													///< by the delta time in seconds, an application can compute the average
													///< percentage utilization of the GPU..
	ctl_oc_telemetry_item_t renderComputeActivityCounter;   ///< [out] Snapshot of the monotonic 3D/compute activity counter. It
													///< measures the time in seconds (accurate down to 1 millisecond) that any
													///< 3D render/compute engine is busy. By taking the delta between two
													///< snapshots and dividing by the delta time in seconds, an application
													///< can compute the average percentage utilization of all 3D
													///< render/compute blocks in the GPU.
	ctl_oc_telemetry_item_t mediaActivityCounter;   ///< [out] Snapshot of the monotonic media activity counter. It measures
													///< the time in seconds (accurate down to 1 millisecond) that any media
													///< engine is busy. By taking the delta between two snapshots and dividing
													///< by the delta time in seconds, an application can compute the average
													///< percentage utilization of all media blocks in the GPU.
	bool gpuPowerLimited;                           ///< [out] Instantaneous indication that the desired GPU frequency is being
													///< throttled because the GPU chip is exceeding the maximum power limits.
													///< Increasing the power limits using ::ctlOverclockPowerLimitSetV2() is
													///< one way to remove this limitation.
	bool gpuTemperatureLimited;                     ///< [out] Instantaneous indication that the desired GPU frequency is being
													///< throttled because the GPU chip is exceeding the temperature limits.
													///< Increasing the temperature limits using
													///< ::ctlOverclockTemperatureLimitSetV2() is one way to reduce this
													///< limitation. Improving the cooling solution is another way.
	bool gpuCurrentLimited;                         ///< [out] Instantaneous indication that the desired GPU frequency is being
													///< throttled because the GPU chip has exceeded the power supply current
													///< limits. A better power supply is required to reduce this limitation.
	bool gpuVoltageLimited;                         ///< [out] Instantaneous indication that the GPU frequency cannot be
													///< increased because the voltage limits have been reached. Increase the
													///< voltage offset using ::ctlOverclockGpuMaxVoltageOffsetSetV2() is one
													///< way to reduce this limitation.
	bool gpuUtilizationLimited;                     ///< [out] Instantaneous indication that due to lower GPU utilization, the
													///< hardware has lowered the GPU frequency.
	ctl_oc_telemetry_item_t vramEnergyCounter;      ///< [out] Snapshot of the monotonic energy counter maintained by hardware.
													///< It measures the total energy consumed by the local memory modules. By
													///< taking the delta between two snapshots and dividing by the delta time
													///< in seconds, an application can compute the average power.
	ctl_oc_telemetry_item_t vramVoltage;            ///< [out] Instantaneous snapshot of the voltage feeding the memory
													///< modules.
	ctl_oc_telemetry_item_t vramCurrentClockFrequency;  ///< [out] Instantaneous snapshot of the raw clock frequency driving the
													///< memory modules.
	ctl_oc_telemetry_item_t vramCurrentEffectiveFrequency;  ///< [out] Instantaneous snapshot of the effective data transfer rate that
													///< the memory modules can sustain based on the current clock frequency..
	ctl_oc_telemetry_item_t vramReadBandwidthCounter;   ///< [out] Instantaneous snapshot of the monotonic counter that measures
													///< the read traffic from the memory modules. By taking the delta between
													///< two snapshots and dividing by the delta time in seconds, an
													///< application can compute the average read bandwidth.
	ctl_oc_telemetry_item_t vramWriteBandwidthCounter;  ///< [out] Instantaneous snapshot of the monotonic counter that measures
													///< the write traffic to the memory modules. By taking the delta between
													///< two snapshots and dividing by the delta time in seconds, an
													///< application can compute the average write bandwidth.
	ctl_oc_telemetry_item_t vramCurrentTemperature; ///< [out] Instantaneous snapshot of the memory modules temperature, read
													///< from the sensor reporting the highest value.
	bool vramPowerLimited;                          ///< [out] Deprecated / Not-supported, will always returns false
	bool vramTemperatureLimited;                    ///< [out] Deprecated / Not-supported, will always returns false
	bool vramCurrentLimited;                        ///< [out] Deprecated / Not-supported, will always returns false
	bool vramVoltageLimited;                        ///< [out] Deprecated / Not-supported, will always returns false
	bool vramUtilizationLimited;                    ///< [out] Deprecated / Not-supported, will always returns false
	ctl_oc_telemetry_item_t totalCardEnergyCounter; ///< [out] Total Card Energy Counter.
	ctl_psu_info_t psu[CTL_PSU_COUNT];              ///< [out] PSU voltage and power.
	ctl_oc_telemetry_item_t fanSpeed[CTL_FAN_COUNT];///< [out] Fan speed.
	ctl_oc_telemetry_item_t gpuVrTemp;              ///< [out] GPU VR temperature. Supported for Version > 0.
	ctl_oc_telemetry_item_t vramVrTemp;             ///< [out] VRAM VR temperature. Supported for Version > 0.
	ctl_oc_telemetry_item_t saVrTemp;               ///< [out] SA VR temperature. Supported for Version > 0.
	ctl_oc_telemetry_item_t gpuEffectiveClock;      ///< [out] Effective frequency of the GPU. Supported for Version > 0.
	ctl_oc_telemetry_item_t gpuOverVoltagePercent;  ///< [out] OverVoltage as a percent between 0 and 100. Positive values
													///< represent fraction of the maximum over-voltage increment being
													///< currently applied. Zero indicates operation at or below default
													///< maximum frequency.  Supported for Version > 0.
	ctl_oc_telemetry_item_t gpuPowerPercent;        ///< [out] GPUPower expressed as a percent representing the fraction of the
													///< default maximum power being drawn currently. Values greater than 100
													///< indicate power draw beyond default limits. Values above OC Power limit
													///< imply throttling due to power. Supported for Version > 0.
	ctl_oc_telemetry_item_t gpuTemperaturePercent;  ///< [out] GPUTemperature expressed as a percent of the thermal margin.
													///< Values of 100 or greater indicate thermal throttling and 0 indicates
													///< device at 0 degree Celcius. Supported for Version > 0.
	ctl_oc_telemetry_item_t vramReadBandwidth;      ///< [out] VRAM Read Bandwidth. Supported for Version > 0.
	ctl_oc_telemetry_item_t vramWriteBandwidth;     ///< [out] VRAM Write Bandwidth. Supported for Version > 0.
} ctl_power_telemetry_t;

ctl_result_t CTL_APICALL
IGCL_PowerTelemetryGet(
	ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
	ctl_power_telemetry_t* pTelemetryInfo           ///< [out] The overclocking properties for the specified domain.
	);

typedef struct _ctl_pci_address_t
{
	uint32_t Size;                                  ///< [in] size of this structure
	uint8_t Version;                                ///< [in] version of this structure
	uint32_t domain;                                ///< [out] BDF domain
	uint32_t bus;                                   ///< [out] BDF bus
	uint32_t device;                                ///< [out] BDF device
	uint32_t function;                              ///< [out] BDF function
} ctl_pci_address_t;

typedef struct _ctl_pci_speed_t
{
	uint32_t Size;                                  ///< [in] size of this structure
	uint8_t Version;                                ///< [in] version of this structure
	int32_t gen;                                    ///< [out] The link generation. A value of -1 means that this property is
													///< unknown.
	int32_t width;                                  ///< [out] The number of lanes. A value of -1 means that this property is
													///< unknown.
	int64_t maxBandwidth;                           ///< [out] The maximum bandwidth in bytes/sec (sum of all lanes). A value
													///< of -1 means that this property is unknown.
} ctl_pci_speed_t;

typedef struct _ctl_pci_properties_t
{
	uint32_t Size;                                  ///< [in] size of this structure
	uint8_t Version;                                ///< [in] version of this structure
	ctl_pci_address_t address;                      ///< [out] The BDF address
	ctl_pci_speed_t maxSpeed;                       ///< [out] Fastest port configuration supported by the device (sum of all
													///< lanes)
	bool resizable_bar_supported;                   ///< [out] Support for Resizable Bar on this device.
	bool resizable_bar_enabled;                     ///< [out] Resizable Bar enabled on this device
} ctl_pci_properties_t;

typedef struct _ctl_pci_state_t
{
	uint32_t Size;                                  ///< [in] size of this structure
	uint8_t Version;                                ///< [in] version of this structure
	ctl_pci_speed_t speed;                          ///< [out] The current port configure speed
} ctl_pci_state_t;

ctl_result_t CTL_APICALL
IGCL_PciGetProperties(
	ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
	ctl_pci_properties_t* pProperties               ///< [in,out] Will contain the PCI properties.
	);

ctl_result_t CTL_APICALL
IGCL_PciGetState(
	ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
	ctl_pci_state_t* pState                         ///< [in,out] Will contain the PCI properties.
	);

typedef void* ctl_mem_handle_t;

typedef struct _ctl_mem_state_t
{
	uint32_t Size;                                  ///< [in] size of this structure
	uint8_t Version;                                ///< [in] version of this structure
	uint64_t free;                                  ///< [out] The free memory in bytes
	uint64_t size;                                  ///< [out] The total allocatable memory in bytes (can be less than
													///< ::ctl_mem_properties_t.physicalSize)
} ctl_mem_state_t;

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
	);

ctl_result_t CTL_APICALL
IGCL_MemoryGetState(
	ctl_mem_handle_t hMemory,                       ///< [in] Handle for the component.
	ctl_mem_state_t* pState                         ///< [in,out] Will contain the current health and allocated memory.
);

#if defined(__cplusplus)
} // extern "C"
#endif
