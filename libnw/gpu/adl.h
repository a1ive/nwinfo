// SPDX-License-Identifier: MIT
// Copyright (c) 2016 - 2022 Advanced Micro Devices, Inc. All rights reserved.
//
// MIT LICENSE:
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once



/// Memory Allocation Call back 
typedef void* (__stdcall* ADL_MAIN_MALLOC_CALLBACK)(int);

/// Defines ADL_TRUE
#define ADL_TRUE    1
/// Defines ADL_FALSE
#define ADL_FALSE        0

/// Defines the maximum string length
#define ADL_MAX_CHAR                                    4096
/// Defines the maximum string length
#define ADL_MAX_PATH                                    256
/// Defines the maximum number of supported adapters
#define ADL_MAX_ADAPTERS                               250
/// Defines the maxumum number of supported displays
#define ADL_MAX_DISPLAYS                                150
/// Defines the maxumum string length for device name
#define ADL_MAX_DEVICENAME                                32

/// All OK, but need to wait
#define ADL_OK_WAIT                4
/// All OK, but need restart
#define ADL_OK_RESTART                3
/// All OK but need mode change
#define ADL_OK_MODE_CHANGE            2
/// All OK, but with warning
#define ADL_OK_WARNING                1
/// ADL function completed successfully
#define ADL_OK                    0
/// Generic Error. Most likely one or more of the Escape calls to the driver failed!
#define ADL_ERR                    -1
/// ADL not initialized
#define ADL_ERR_NOT_INIT            -2
/// One of the parameter passed is invalid
#define ADL_ERR_INVALID_PARAM            -3
/// One of the parameter size is invalid
#define ADL_ERR_INVALID_PARAM_SIZE        -4
/// Invalid ADL index passed
#define ADL_ERR_INVALID_ADL_IDX            -5
/// Invalid controller index passed
#define ADL_ERR_INVALID_CONTROLLER_IDX        -6
/// Invalid display index passed
#define ADL_ERR_INVALID_DIPLAY_IDX        -7
/// Function  not supported by the driver
#define ADL_ERR_NOT_SUPPORTED            -8
/// Null Pointer error
#define ADL_ERR_NULL_POINTER            -9
/// Call can't be made due to disabled adapter
#define ADL_ERR_DISABLED_ADAPTER        -10
/// Invalid Callback
#define ADL_ERR_INVALID_CALLBACK            -11
/// Display Resource conflict
#define ADL_ERR_RESOURCE_CONFLICT                -12
//Failed to update some of the values. Can be returned by set request that include multiple values if not all values were successfully committed.
#define ADL_ERR_SET_INCOMPLETE                 -20
/// There's no Linux XDisplay in Linux Console environment
#define ADL_ERR_NO_XDISPLAY                    -21
/// escape call failed becuse of incompatiable driver found in driver store
#define ADL_ERR_CALL_TO_INCOMPATIABLE_DRIVER            -22
/// not running as administrator
#define ADL_ERR_NO_ADMINISTRATOR_PRIVILEGES            -23
/// Feature Sync Start api is not called yet
#define ADL_ERR_FEATURESYNC_NOT_STARTED            -24
/// Adapter is in an invalid power state
#define ADL_ERR_INVALID_POWER_STATE             -25
/// The RPC server is in busy state
#define ADL_ERR_SERVER_BUSY             -26
/// The GPU is occupied by application which cannot be powered off
#define ADL_ERR_GPU_IN_USE            -27
/// The general RPC connection error
#define ADL_ERR_RPC            -28

#define ADL_PMLOG_MAX_SENSORS  256

typedef struct AdapterInfo
{
	/// \WIN_STRUCT_MEM

	/// Size of the structure.
	int iSize;
	/// The ADL index handle. One GPU may be associated with one or two index handles
	int iAdapterIndex;
	/// The unique device ID associated with this adapter.
	char strUDID[ADL_MAX_PATH];
	/// The BUS number associated with this adapter.
	int iBusNumber;
	/// The driver number associated with this adapter.
	int iDeviceNumber;
	/// The function number.
	int iFunctionNumber;
	/// The vendor ID associated with this adapter.
	int iVendorID;
	/// Adapter name.
	char strAdapterName[ADL_MAX_PATH];
	/// Display name. For example, "\\\\Display0" for Windows.
	char strDisplayName[ADL_MAX_PATH];
	/// Present or not; 1 if present and 0 if not present.It the logical adapter is present, the display name such as \\\\.\\Display1 can be found from OS
	int iPresent;

#if defined (_WIN32) || defined (_WIN64)
	/// \WIN_STRUCT_MEM

	/// Exist or not; 1 is exist and 0 is not present.
	int iExist;
	/// Driver registry path.
	char strDriverPath[ADL_MAX_PATH];
	/// Driver registry path Ext for.
	char strDriverPathExt[ADL_MAX_PATH];
	/// PNP string from Windows.
	char strPNPString[ADL_MAX_PATH];
	/// It is generated from EnumDisplayDevices.
	int iOSDisplayIndex;

#endif /* (_WIN32) || (_WIN64) */

} AdapterInfo, * LPAdapterInfo;

typedef struct ADLMemoryInfo2
{
	/// Memory size in bytes.
	long long iMemorySize;
	/// Memory type in string.
	char strMemoryType[ADL_MAX_PATH];
	/// Highest default performance level Memory bandwidth in Mbytes/s
	long long iMemoryBandwidth;
	/// HyperMemory size in bytes.
	long long iHyperMemorySize;

	/// Invisible Memory size in bytes.
	long long iInvisibleMemorySize;
	/// Visible Memory size in bytes.
	long long iVisibleMemorySize;
} ADLMemoryInfo2, * LPADLMemoryInfo2;

typedef struct ADLMemoryInfo3
{
	/// Memory size in bytes.
	long long iMemorySize;
	/// Memory type in string.
	char strMemoryType[ADL_MAX_PATH];
	/// Highest default performance level Memory bandwidth in Mbytes/s
	long long iMemoryBandwidth;
	/// HyperMemory size in bytes.
	long long iHyperMemorySize;

	/// Invisible Memory size in bytes.
	long long iInvisibleMemorySize;
	/// Visible Memory size in bytes.
	long long iVisibleMemorySize;
	/// Vram vendor ID
	long long iVramVendorRevId;
} ADLMemoryInfo3, * LPADLMemoryInfo3;

typedef struct ADLMemoryInfoX4
{
	/// Memory size in bytes.
	long long iMemorySize;
	/// Memory type in string.
	char strMemoryType[ADL_MAX_PATH];
	/// Highest default performance level Memory bandwidth in Mbytes/s
	long long iMemoryBandwidth;
	/// HyperMemory size in bytes.
	long long iHyperMemorySize;

	/// Invisible Memory size in bytes.
	long long iInvisibleMemorySize;
	/// Visible Memory size in bytes.
	long long iVisibleMemorySize;
	/// Vram vendor ID
	long long iVramVendorRevId;
	/// Memory Bandiwidth that is calculated and finalized on the driver side, grab and go.
	long long iMemoryBandwidthX2;
	/// Memory Bit Rate that is calculated and finalized on the driver side, grab and go.
	long long iMemoryBitRateX2;
} ADLMemoryInfoX4, * LPADLMemoryInfoX4;

typedef struct ADLGcnInfo
{
	int CuCount; //Number of compute units on the ASIC.
	int TexCount; //Number of texture mapping units.
	int RopCount; //Number of Render backend Units.
	int ASICFamilyId; //Such SI, VI. See /inc/asic_reg/atiid.h for family ids
	int ASICRevisionId; //Such as Ellesmere, Fiji.   For example - VI family revision ids are stored in /inc/asic_reg/vi_id.h
}ADLGcnInfo;

typedef struct ADLODParameterRange
{
	/// Minimum parameter value.
	int iMin;
	/// Maximum parameter value.
	int iMax;
	/// Parameter step value.
	int iStep;
} ADLODParameterRange;

typedef struct ADLODParameters
{
	/// Must be set to the size of the structure
	int iSize;
	/// Number of standard performance states.
	int iNumberOfPerformanceLevels;
	/// Indicates whether the GPU is capable to measure its activity.
	int iActivityReportingSupported;
	/// Indicates whether the GPU supports discrete performance levels or performance range.
	int iDiscretePerformanceLevels;
	/// Reserved for future use.
	int iReserved;
	/// Engine clock range.
	ADLODParameterRange sEngineClock;
	/// Memory clock range.
	ADLODParameterRange sMemoryClock;
	/// Core voltage range.
	ADLODParameterRange sVddc;
} ADLODParameters;

typedef struct ADLTemperature
{
	/// Must be set to the size of the structure
	int iSize;
	/// Temperature in millidegrees Celsius.
	int iTemperature;
} ADLTemperature;

typedef struct ADLFanSpeedValue
{
	/// Must be set to the size of the structure
	int iSize;
	/// Possible valies: \ref ADL_DL_FANCTRL_SPEED_TYPE_PERCENT or \ref ADL_DL_FANCTRL_SPEED_TYPE_RPM
	int iSpeedType;
	/// Fan speed value
	int iFanSpeed;
	/// The only flag for now is: \ref ADL_DL_FANCTRL_FLAG_USER_DEFINED_SPEED
	int iFlags;
} ADLFanSpeedValue;

typedef struct ADLPMActivity
{
	/// Must be set to the size of the structure
	int iSize;
	/// Current engine clock.
	int iEngineClock;
	/// Current memory clock.
	int iMemoryClock;
	/// Current core voltage.
	int iVddc;
	/// GPU utilization.
	int iActivityPercent;
	/// Performance level index.
	int iCurrentPerformanceLevel;
	/// Current PCIE bus speed.
	int iCurrentBusSpeed;
	/// Number of PCIE bus lanes.
	int iCurrentBusLanes;
	/// Maximum number of PCIE bus lanes.
	int iMaximumBusLanes;
	/// Reserved for future purposes.
	int iReserved;
} ADLPMActivity;

typedef struct ADLOD6ParameterRange
{
	/// The starting value of the clock range
	int     iMin;
	/// The ending value of the clock range
	int     iMax;
	/// The minimum increment between clock values
	int     iStep;
} ADLOD6ParameterRange;

typedef struct ADLOD6Capabilities
{
	/// Contains a bitmap of the OD6 capability flags.  Possible values: \ref ADL_OD6_CAPABILITY_SCLK_CUSTOMIZATION,
	/// \ref ADL_OD6_CAPABILITY_MCLK_CUSTOMIZATION, \ref ADL_OD6_CAPABILITY_GPU_ACTIVITY_MONITOR
	int     iCapabilities;
	/// Contains a bitmap indicating the power states
	/// supported by OD6.  Currently only the performance state
	/// is supported. Possible Values: \ref ADL_OD6_SUPPORTEDSTATE_PERFORMANCE
	int     iSupportedStates;
	/// Number of levels. OD6 will always use 2 levels, which describe
	/// the minimum to maximum clock ranges.
	/// The 1st level indicates the minimum clocks, and the 2nd level
	/// indicates the maximum clocks.
	int     iNumberOfPerformanceLevels;
	/// Contains the hard limits of the sclk range.  Overdrive
	/// clocks cannot be set outside this range.
	ADLOD6ParameterRange     sEngineClockRange;
	/// Contains the hard limits of the mclk range.  Overdrive
	/// clocks cannot be set outside this range.
	ADLOD6ParameterRange     sMemoryClockRange;

	/// Value for future extension
	int     iExtValue;
	/// Mask for future extension
	int     iExtMask;
} ADLOD6Capabilities;

typedef struct ADLOD6CurrentStatus
{
	/// Current engine clock in 10 KHz.
	int     iEngineClock;
	/// Current memory clock in 10 KHz.
	int     iMemoryClock;
	/// Current GPU activity in percent.  This
	/// indicates how "busy" the GPU is.
	int     iActivityPercent;
	/// Not used.  Reserved for future use.
	int     iCurrentPerformanceLevel;
	/// Current PCI-E bus speed
	int     iCurrentBusSpeed;
	/// Current PCI-E bus # of lanes
	int     iCurrentBusLanes;
	/// Maximum possible PCI-E bus # of lanes
	int     iMaximumBusLanes;

	/// Value for future extension
	int     iExtValue;
	/// Mask for future extension
	int     iExtMask;
} ADLOD6CurrentStatus;

typedef enum ADLODNCurrentPowerType
{
	ODN_GPU_TOTAL_POWER = 0,
	ODN_GPU_PPT_POWER,
	ODN_GPU_SOCKET_POWER,
	ODN_GPU_CHIP_POWER
} ADLODNCurrentPowerType;

typedef enum ADLODNTemperatureType
{
	ODN_GPU_EDGE_TEMP = 1,
	ODN_GPU_MEM_TEMP = 2,
	ODN_GPU_VRVDDC_TEMP = 3,
	ODN_GPU_VRMVDD_TEMP = 4,
	ODN_GPU_LIQUID_TEMP = 5,
	ODN_GPU_PLX_TEMP = 6,
	ODN_GPU_HOTSPOT_TEMP = 7
} ADLODNTemperatureType;

typedef struct ADLODNPerformanceStatus
{
	int iCoreClock;
	int iMemoryClock;
	int iDCEFClock;
	int iGFXClock;
	int iUVDClock;
	int iVCEClock;
	int iGPUActivityPercent;
	int iCurrentCorePerformanceLevel;
	int iCurrentMemoryPerformanceLevel;
	int iCurrentDCEFPerformanceLevel;
	int iCurrentGFXPerformanceLevel;
	int iUVDPerformanceLevel;
	int iVCEPerformanceLevel;
	int iCurrentBusSpeed;
	int iCurrentBusLanes;
	int iMaximumBusLanes;
	int iVDDC;
	int iVDDCI;
} ADLODNPerformanceStatus;

typedef struct ADLSingleSensorData
{
	int supported;
	int  value;
} ADLSingleSensorData;

typedef struct ADLPMLogDataOutput
{
	int size;
	ADLSingleSensorData sensors[ADL_PMLOG_MAX_SENSORS];
}ADLPMLogDataOutput;

enum { ADL_PMLOG_MAX_SUPPORTED_SENSORS = 256 };

typedef struct ADLPMLogSupportInfo
{
	/// list of sensors defined by ADL_PMLOG_SENSORS
	unsigned short usSensors[ADL_PMLOG_MAX_SUPPORTED_SENSORS];
	/// Reserved
	int ulReserved[16];
} ADLPMLogSupportInfo;

typedef struct ADLPMLogStartInput
{
	/// list of sensors defined by ADL_PMLOG_SENSORS
	unsigned short usSensors[ADL_PMLOG_MAX_SUPPORTED_SENSORS];
	/// Sample rate in milliseconds
	unsigned long ulSampleRate;
	/// Reserved
	int ulReserved[15];
} ADLPMLogStartInput;

typedef struct ADLPMLogData
{
	/// Structure version
	unsigned int ulVersion;
	/// Current driver sample rate
	unsigned int ulActiveSampleRate;
	/// Timestamp of last update
	unsigned long long ulLastUpdated;
	/// 2D array of senesor and values
	unsigned int ulValues[ADL_PMLOG_MAX_SUPPORTED_SENSORS][2];
	/// Reserved
	unsigned int ulReserved[256];
} ADLPMLogData;

typedef struct ADLPMLogStartOutput
{
	/// Pointer to memory address containing logging data
	union
	{
		void* pLoggingAddress;
		unsigned long long ptr_LoggingAddress;
	};
	/// Reserved
	int ulReserved[14];
} ADLPMLogStartOutput;

typedef enum ADL_PMLOG_SENSORS
{
	ADL_SENSOR_MAXTYPES = 0,
	ADL_PMLOG_CLK_GFXCLK = 1,
	ADL_PMLOG_CLK_MEMCLK = 2,
	ADL_PMLOG_CLK_SOCCLK = 3,
	ADL_PMLOG_CLK_UVDCLK1 = 4,
	ADL_PMLOG_CLK_UVDCLK2 = 5,
	ADL_PMLOG_CLK_VCECLK = 6,
	ADL_PMLOG_CLK_VCNCLK = 7,
	ADL_PMLOG_TEMPERATURE_EDGE = 8,
	ADL_PMLOG_TEMPERATURE_MEM = 9,
	ADL_PMLOG_TEMPERATURE_VRVDDC = 10,
	ADL_PMLOG_TEMPERATURE_VRMVDD = 11,
	ADL_PMLOG_TEMPERATURE_LIQUID = 12,
	ADL_PMLOG_TEMPERATURE_PLX = 13,
	ADL_PMLOG_FAN_RPM = 14,
	ADL_PMLOG_FAN_PERCENTAGE = 15,
	ADL_PMLOG_SOC_VOLTAGE = 16,
	ADL_PMLOG_SOC_POWER = 17,
	ADL_PMLOG_SOC_CURRENT = 18,
	ADL_PMLOG_INFO_ACTIVITY_GFX = 19,
	ADL_PMLOG_INFO_ACTIVITY_MEM = 20,
	ADL_PMLOG_GFX_VOLTAGE = 21,
	ADL_PMLOG_MEM_VOLTAGE = 22,
	ADL_PMLOG_ASIC_POWER = 23,
	ADL_PMLOG_TEMPERATURE_VRSOC = 24,
	ADL_PMLOG_TEMPERATURE_VRMVDD0 = 25,
	ADL_PMLOG_TEMPERATURE_VRMVDD1 = 26,
	ADL_PMLOG_TEMPERATURE_HOTSPOT = 27,
	ADL_PMLOG_TEMPERATURE_GFX = 28,
	ADL_PMLOG_TEMPERATURE_SOC = 29,
	ADL_PMLOG_GFX_POWER = 30,
	ADL_PMLOG_GFX_CURRENT = 31,
	ADL_PMLOG_TEMPERATURE_CPU = 32,
	ADL_PMLOG_CPU_POWER = 33,
	ADL_PMLOG_CLK_CPUCLK = 34,
	ADL_PMLOG_THROTTLER_STATUS = 35,   // GFX
	ADL_PMLOG_CLK_VCN1CLK1 = 36,
	ADL_PMLOG_CLK_VCN1CLK2 = 37,
	ADL_PMLOG_SMART_POWERSHIFT_CPU = 38,
	ADL_PMLOG_SMART_POWERSHIFT_DGPU = 39,
	ADL_PMLOG_BUS_SPEED = 40,
	ADL_PMLOG_BUS_LANES = 41,
	ADL_PMLOG_TEMPERATURE_LIQUID0 = 42,
	ADL_PMLOG_TEMPERATURE_LIQUID1 = 43,
	ADL_PMLOG_CLK_FCLK = 44,
	ADL_PMLOG_THROTTLER_STATUS_CPU = 45,
	ADL_PMLOG_SSPAIRED_ASICPOWER = 46, // apuPower
	ADL_PMLOG_SSTOTAL_POWERLIMIT = 47, // Total Power limit
	ADL_PMLOG_SSAPU_POWERLIMIT = 48, // APU Power limit
	ADL_PMLOG_SSDGPU_POWERLIMIT = 49, // DGPU Power limit
	ADL_PMLOG_TEMPERATURE_HOTSPOT_GCD = 50,
	ADL_PMLOG_TEMPERATURE_HOTSPOT_MCD = 51,
	ADL_PMLOG_THROTTLE_PERCENTAGE_TEMP_GFX = 52,
	ADL_PMLOG_THROTTLE_PERCENTAGE_TEMP_MEM = 53,
	ADL_PMLOG_THROTTLE_PERCENTAGE_TEMP_VR = 54,
	ADL_PMLOG_THROTTLE_PERCENTAGE_POWER = 55,
	ADL_PMLOG_THROTTLE_PERCENTAGE_TDC = 56,
	ADL_PMLOG_THROTTLE_PERCENTAGE_VMAX = 57,
	ADL_PMLOG_BUS_CURR_MAX_SPEED = 58,
	ADL_PMLOG_RESERVED_1 = 59, //Currently Unused
	ADL_PMLOG_RESERVED_2 = 60, //Currently Unused
	ADL_PMLOG_RESERVED_3 = 61, //Currently Unused
	ADL_PMLOG_RESERVED_4 = 62, //Currently Unused
	ADL_PMLOG_RESERVED_5 = 63, //Currently Unused
	ADL_PMLOG_RESERVED_6 = 64, //Currently Unused
	ADL_PMLOG_RESERVED_7 = 65, //Currently Unused
	ADL_PMLOG_RESERVED_8 = 66, //Currently Unused
	ADL_PMLOG_RESERVED_9 = 67, //Currently Unused
	ADL_PMLOG_RESERVED_10 = 68, //Currently Unused
	ADL_PMLOG_RESERVED_11 = 69, //Currently Unused
	ADL_PMLOG_RESERVED_12 = 70, //Currently Unused
	ADL_PMLOG_CLK_NPUCLK = 71,      //NPU frequency
	ADL_PMLOG_NPU_BUSY_AVG = 72,    //NPU busy
	ADL_PMLOG_BOARD_POWER = 73,
	ADL_PMLOG_TEMPERATURE_INTAKE = 74,
	ADL_PMLOG_MAX_SENSORS_REAL
} ADL_PMLOG_SENSORS;

///\defgroup define_fanctrl Fan speed cotrol
/// Values for ADLFanSpeedInfo.iFlags
/// @{
#define ADL_DL_FANCTRL_SUPPORTS_PERCENT_READ     1
#define ADL_DL_FANCTRL_SUPPORTS_PERCENT_WRITE    2
#define ADL_DL_FANCTRL_SUPPORTS_RPM_READ         4
#define ADL_DL_FANCTRL_SUPPORTS_RPM_WRITE        8
/// @}

//values for ADLFanSpeedValue.iSpeedType
#define ADL_DL_FANCTRL_SPEED_TYPE_PERCENT    1
#define ADL_DL_FANCTRL_SPEED_TYPE_RPM        2

//values for ADLFanSpeedValue.iFlags
#define ADL_DL_FANCTRL_FLAG_USER_DEFINED_SPEED   1

enum GCNFamilies
{
	FAMILY_UNKNOWN = 0,
	FAMILY_TN = 105,
	FAMILY_SI = 110,
	FAMILY_CI = 120,
	FAMILY_KV = 125,
	FAMILY_VI = 130,
	FAMILY_CZ = 135,
	FAMILY_AI = 141, // Vega
	FAMILY_RV = 142,
	FAMILY_NV = 143,
	FAMILY_VGH = 144,
	FAMILY_NV3 = 145, // Navi 3x
	FAMILY_YC = 146,
	FAMILY_GC_11_0_1 = 148,
	FAMILY_GC_10_3_6 = 149,
	FAMILY_GC_11_5_0 = 150,
	FAMILY_GC_10_3_7 = 151,
};

int ADL2_Main_Control_Create(int enumConnectedAdapters, void** ppContext);
int ADL2_Main_Control_Destroy(void* pContext);
int ADL2_Adapter_NumberOfAdapters_Get(void* pContext, int* numAdapters);
int ADL2_Adapter_AdapterInfo_Get(void* pContext, LPAdapterInfo lpInfo, int iInputSize);
int ADL2_GcnAsicInfo_Get(void* pContext, int iAdapterIndex, ADLGcnInfo* lpGcnInfo);
int ADL2_Adapter_Active_Get(void* pContext, int iAdapterIndex, int* lpStatus);
int ADL2_Overdrive_Caps(void* pContext, int iAdapterIndex, int* odSupported, int* odEnabled, int* odVersion);
int ADL2_Overdrive5_ODParameters_Get(void* pContext, int iAdapterIndex, ADLODParameters* lpOdParameters);
int ADL2_Overdrive5_Temperature_Get(void* pContext, int iAdapterIndex, int iThermalControllerIndex, ADLTemperature* lpTemperature);
int ADL2_Overdrive5_FanSpeed_Get(void* pContext, int iAdapterIndex, int iThermalControllerIndex, ADLFanSpeedValue* lpFanSpeedValue);
int ADL2_Overdrive5_CurrentActivity_Get(void* pContext, int iAdapterIndex, ADLPMActivity* lpActivity);
int ADL2_Overdrive6_Capabilities_Get(void* pContext, int iAdapterIndex, ADLOD6Capabilities* lpODCapabilities);
int ADL2_Overdrive6_CurrentPower_Get(void* pContext, int iAdapterIndex, ADLODNCurrentPowerType powerType, int* lpCurrentValue);
int ADL2_Overdrive6_CurrentStatus_Get(void* pContext, int iAdapterIndex, ADLOD6CurrentStatus* lpCurrentStatus);
int ADL2_OverdriveN_Temperature_Get(void* pContext, int iAdapterIndex, int iTemperatureType, int* lpTemperature);
int ADL2_OverdriveN_PerformanceStatus_Get(void* pContext, int iAdapterIndex, ADLODNPerformanceStatus* lpOdPerformanceStatus);
int ADL2_Adapter_DedicatedVRAMUsage_Get(void* pContext, int iAdapterIndex, int* lpVRAMUsageInMB);
int ADL2_New_QueryPMLogData_Get(void* pContext, int iAdapterIndex, ADLPMLogDataOutput* lpPMLogDataOutput);
int ADL2_Device_PMLog_Device_Create(void* pContext, int iAdapterIndex, unsigned int* pDevice);
int ADL2_Device_PMLog_Device_Destroy(void* pContext, unsigned int iDevice);
int ADL2_Adapter_PMLog_Support_Get(void* pContext, int iAdapterIndex, ADLPMLogSupportInfo* pPMLogSupportInfo);
int ADL2_Adapter_PMLog_Start(void* pContext, int iAdapterIndex, ADLPMLogStartInput* pPMLogStartInput, ADLPMLogStartOutput* pPMLogStartOutput, unsigned int iDevice);
int ADL2_Adapter_PMLog_Stop(void* pContext, int iAdapterIndex, unsigned int iDevice);
int ADL2_Adapter_MemoryInfoX4_Get(void* pContext, int iAdapterIndex, ADLMemoryInfoX4* lpMemoryInfoX4);
int ADL2_Adapter_MemoryInfo2_Get(void* pContext, int iAdapterIndex, ADLMemoryInfo2* lpMemoryInfo2);
