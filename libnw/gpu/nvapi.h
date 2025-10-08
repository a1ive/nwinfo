// SPDX-License-Identifier: MIT

#pragma once

#define VC_EXTRALEAN
#include <windows.h>

#pragma pack(push,8)

typedef enum _NvAPI_Status
{
	NVAPI_OK = 0,      //!< Success. Request is completed.
	NVAPI_ERROR = -1,      //!< Generic error
	NVAPI_LIBRARY_NOT_FOUND = -2,      //!< NVAPI support library cannot be loaded.
	NVAPI_NO_IMPLEMENTATION = -3,      //!< not implemented in current driver installation
	NVAPI_API_NOT_INITIALIZED = -4,      //!< NvAPI_Initialize has not been called (successfully)
	NVAPI_INVALID_ARGUMENT = -5,      //!< The argument/parameter value is not valid or NULL.
	NVAPI_NVIDIA_DEVICE_NOT_FOUND = -6,      //!< No NVIDIA display driver, or NVIDIA GPU driving a display, was found.
	NVAPI_END_ENUMERATION = -7,      //!< No more items to enumerate
	NVAPI_INVALID_HANDLE = -8,      //!< Invalid handle
	NVAPI_INCOMPATIBLE_STRUCT_VERSION = -9,      //!< An argument's structure version is not supported
	NVAPI_HANDLE_INVALIDATED = -10,     //!< The handle is no longer valid (likely due to GPU or display re-configuration)
	NVAPI_OPENGL_CONTEXT_NOT_CURRENT = -11,     //!< No NVIDIA OpenGL context is current (but needs to be)
	NVAPI_INVALID_POINTER = -14,     //!< An invalid pointer, usually NULL, was passed as a parameter
	NVAPI_NO_GL_EXPERT = -12,     //!< OpenGL Expert is not supported by the current drivers
	NVAPI_INSTRUMENTATION_DISABLED = -13,     //!< OpenGL Expert is supported, but driver instrumentation is currently disabled
	NVAPI_NO_GL_NSIGHT = -15,     //!< OpenGL does not support Nsight

	NVAPI_EXPECTED_LOGICAL_GPU_HANDLE = -100,    //!< Expected a logical GPU handle for one or more parameters
	NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE = -101,    //!< Expected a physical GPU handle for one or more parameters
	NVAPI_EXPECTED_DISPLAY_HANDLE = -102,    //!< Expected an NV display handle for one or more parameters
	NVAPI_INVALID_COMBINATION = -103,    //!< The combination of parameters is not valid. 
	NVAPI_NOT_SUPPORTED = -104,    //!< Requested feature is not supported in the selected GPU
	NVAPI_PORTID_NOT_FOUND = -105,    //!< No port ID was found for the I2C transaction
	NVAPI_EXPECTED_UNATTACHED_DISPLAY_HANDLE = -106,    //!< Expected an unattached display handle as one of the input parameters.
	NVAPI_INVALID_PERF_LEVEL = -107,    //!< Invalid perf level 
	NVAPI_DEVICE_BUSY = -108,    //!< Device is busy; request not fulfilled
	NVAPI_NV_PERSIST_FILE_NOT_FOUND = -109,    //!< NV persist file is not found
	NVAPI_PERSIST_DATA_NOT_FOUND = -110,    //!< NV persist data is not found
	NVAPI_EXPECTED_TV_DISPLAY = -111,    //!< Expected a TV output display
	NVAPI_EXPECTED_TV_DISPLAY_ON_DCONNECTOR = -112,    //!< Expected a TV output on the D Connector - HDTV_EIAJ4120.
	NVAPI_NO_ACTIVE_SLI_TOPOLOGY = -113,    //!< SLI is not active on this device.
	NVAPI_SLI_RENDERING_MODE_NOTALLOWED = -114,    //!< Setup of SLI rendering mode is not possible right now.
	NVAPI_EXPECTED_DIGITAL_FLAT_PANEL = -115,    //!< Expected a digital flat panel.
	NVAPI_ARGUMENT_EXCEED_MAX_SIZE = -116,    //!< Argument exceeds the expected size.
	NVAPI_DEVICE_SWITCHING_NOT_ALLOWED = -117,    //!< Inhibit is ON due to one of the flags in NV_GPU_DISPLAY_CHANGE_INHIBIT or SLI active.
	NVAPI_TESTING_CLOCKS_NOT_SUPPORTED = -118,    //!< Testing of clocks is not supported.
	NVAPI_UNKNOWN_UNDERSCAN_CONFIG = -119,    //!< The specified underscan config is from an unknown source (e.g. INF)
	NVAPI_TIMEOUT_RECONFIGURING_GPU_TOPO = -120,    //!< Timeout while reconfiguring GPUs
	NVAPI_DATA_NOT_FOUND = -121,    //!< Requested data was not found
	NVAPI_EXPECTED_ANALOG_DISPLAY = -122,    //!< Expected an analog display
	NVAPI_NO_VIDLINK = -123,    //!< No SLI video bridge is present
	NVAPI_REQUIRES_REBOOT = -124,    //!< NVAPI requires a reboot for the settings to take effect
	NVAPI_INVALID_HYBRID_MODE = -125,    //!< The function is not supported with the current Hybrid mode.
	NVAPI_MIXED_TARGET_TYPES = -126,    //!< The target types are not all the same
	NVAPI_SYSWOW64_NOT_SUPPORTED = -127,    //!< The function is not supported from 32-bit on a 64-bit system.
	NVAPI_IMPLICIT_SET_GPU_TOPOLOGY_CHANGE_NOT_ALLOWED = -128,    //!< There is no implicit GPU topology active. Use NVAPI_SetHybridMode to change topology.
	NVAPI_REQUEST_USER_TO_CLOSE_NON_MIGRATABLE_APPS = -129,      //!< Prompt the user to close all non-migratable applications.    
	NVAPI_OUT_OF_MEMORY = -130,    //!< Could not allocate sufficient memory to complete the call.
	NVAPI_WAS_STILL_DRAWING = -131,    //!< The previous operation that is transferring information to or from this surface is incomplete.
	NVAPI_FILE_NOT_FOUND = -132,    //!< The file was not found.
	NVAPI_TOO_MANY_UNIQUE_STATE_OBJECTS = -133,    //!< There are too many unique instances of a particular type of state object.
	NVAPI_INVALID_CALL = -134,    //!< The method call is invalid. For example, a method's parameter may not be a valid pointer.
	NVAPI_D3D10_1_LIBRARY_NOT_FOUND = -135,    //!< d3d10_1.dll cannot be loaded.
	NVAPI_FUNCTION_NOT_FOUND = -136,    //!< Couldn't find the function in the loaded DLL.
	NVAPI_INVALID_USER_PRIVILEGE = -137,    //!< The application will require Administrator privileges to access this API.
	//!< The application can be elevated to a higher permission level by selecting "Run as Administrator".
	NVAPI_EXPECTED_NON_PRIMARY_DISPLAY_HANDLE = -138,    //!< The handle corresponds to GDIPrimary.
	NVAPI_EXPECTED_COMPUTE_GPU_HANDLE = -139,    //!< Setting Physx GPU requires that the GPU is compute-capable.
	NVAPI_STEREO_NOT_INITIALIZED = -140,    //!< The Stereo part of NVAPI failed to initialize completely. Check if the stereo driver is installed.
	NVAPI_STEREO_REGISTRY_ACCESS_FAILED = -141,    //!< Access to stereo-related registry keys or values has failed.
	NVAPI_STEREO_REGISTRY_PROFILE_TYPE_NOT_SUPPORTED = -142, //!< The given registry profile type is not supported.
	NVAPI_STEREO_REGISTRY_VALUE_NOT_SUPPORTED = -143,    //!< The given registry value is not supported.
	NVAPI_STEREO_NOT_ENABLED = -144,    //!< Stereo is not enabled and the function needed it to execute completely.
	NVAPI_STEREO_NOT_TURNED_ON = -145,    //!< Stereo is not turned on and the function needed it to execute completely.
	NVAPI_STEREO_INVALID_DEVICE_INTERFACE = -146,    //!< Invalid device interface.
	NVAPI_STEREO_PARAMETER_OUT_OF_RANGE = -147,    //!< Separation percentage or JPEG image capture quality is out of [0-100] range.
	NVAPI_STEREO_FRUSTUM_ADJUST_MODE_NOT_SUPPORTED = -148, //!< The given frustum adjust mode is not supported.
	NVAPI_TOPO_NOT_POSSIBLE = -149,    //!< The mosaic topology is not possible given the current state of the hardware.
	NVAPI_MODE_CHANGE_FAILED = -150,    //!< An attempt to do a display resolution mode change has failed.        
	NVAPI_D3D11_LIBRARY_NOT_FOUND = -151,    //!< d3d11.dll/d3d11_beta.dll cannot be loaded.
	NVAPI_INVALID_ADDRESS = -152,    //!< Address is outside of valid range.
	NVAPI_STRING_TOO_SMALL = -153,    //!< The pre-allocated string is too small to hold the result.
	NVAPI_MATCHING_DEVICE_NOT_FOUND = -154,    //!< The input does not match any of the available devices.
	NVAPI_DRIVER_RUNNING = -155,    //!< Driver is running.
	NVAPI_DRIVER_NOTRUNNING = -156,    //!< Driver is not running.
	NVAPI_ERROR_DRIVER_RELOAD_REQUIRED = -157,    //!< A driver reload is required to apply these settings.
	NVAPI_SET_NOT_ALLOWED = -158,    //!< Intended setting is not allowed.
	NVAPI_ADVANCED_DISPLAY_TOPOLOGY_REQUIRED = -159,    //!< Information can't be returned due to "advanced display topology".
	NVAPI_SETTING_NOT_FOUND = -160,    //!< Setting is not found.
	NVAPI_SETTING_SIZE_TOO_LARGE = -161,    //!< Setting size is too large.
	NVAPI_TOO_MANY_SETTINGS_IN_PROFILE = -162,    //!< There are too many settings for a profile. 
	NVAPI_PROFILE_NOT_FOUND = -163,    //!< Profile is not found.
	NVAPI_PROFILE_NAME_IN_USE = -164,    //!< Profile name is duplicated.
	NVAPI_PROFILE_NAME_EMPTY = -165,    //!< Profile name is empty.
	NVAPI_EXECUTABLE_NOT_FOUND = -166,    //!< Application not found in the Profile.
	NVAPI_EXECUTABLE_ALREADY_IN_USE = -167,    //!< Application already exists in the other profile.
	NVAPI_DATATYPE_MISMATCH = -168,    //!< Data Type mismatch 
	NVAPI_PROFILE_REMOVED = -169,    //!< The profile passed as parameter has been removed and is no longer valid.
	NVAPI_UNREGISTERED_RESOURCE = -170,    //!< An unregistered resource was passed as a parameter. 
	NVAPI_ID_OUT_OF_RANGE = -171,    //!< The DisplayId corresponds to a display which is not within the normal outputId range.
	NVAPI_DISPLAYCONFIG_VALIDATION_FAILED = -172,    //!< Display topology is not valid so the driver cannot do a mode set on this configuration.
	NVAPI_DPMST_CHANGED = -173,    //!< Display Port Multi-Stream topology has been changed.
	NVAPI_INSUFFICIENT_BUFFER = -174,    //!< Input buffer is insufficient to hold the contents.    
	NVAPI_ACCESS_DENIED = -175,    //!< No access to the caller.
	NVAPI_MOSAIC_NOT_ACTIVE = -176,    //!< The requested action cannot be performed without Mosaic being enabled.
	NVAPI_SHARE_RESOURCE_RELOCATED = -177,    //!< The surface is relocated away from video memory.
	NVAPI_REQUEST_USER_TO_DISABLE_DWM = -178,    //!< The user should disable DWM before calling NvAPI.
	NVAPI_D3D_DEVICE_LOST = -179,    //!< D3D device status is D3DERR_DEVICELOST or D3DERR_DEVICENOTRESET - the user has to reset the device.
	NVAPI_INVALID_CONFIGURATION = -180,    //!< The requested action cannot be performed in the current state.
	NVAPI_STEREO_HANDSHAKE_NOT_DONE = -181,    //!< Call failed as stereo handshake not completed.
	NVAPI_EXECUTABLE_PATH_IS_AMBIGUOUS = -182,    //!< The path provided was too short to determine the correct NVDRS_APPLICATION
	NVAPI_DEFAULT_STEREO_PROFILE_IS_NOT_DEFINED = -183,    //!< Default stereo profile is not currently defined
	NVAPI_DEFAULT_STEREO_PROFILE_DOES_NOT_EXIST = -184,    //!< Default stereo profile does not exist
	NVAPI_CLUSTER_ALREADY_EXISTS = -185,    //!< A cluster is already defined with the given configuration.
	NVAPI_DPMST_DISPLAY_ID_EXPECTED = -186,    //!< The input display id is not that of a multi stream enabled connector or a display device in a multi stream topology 
	NVAPI_INVALID_DISPLAY_ID = -187,    //!< The input display id is not valid or the monitor associated to it does not support the current operation
	NVAPI_STREAM_IS_OUT_OF_SYNC = -188,    //!< While playing secure audio stream, stream goes out of sync
	NVAPI_INCOMPATIBLE_AUDIO_DRIVER = -189,    //!< Older audio driver version than required
	NVAPI_VALUE_ALREADY_SET = -190,    //!< Value already set, setting again not allowed.
	NVAPI_TIMEOUT = -191,    //!< Requested operation timed out 
	NVAPI_GPU_WORKSTATION_FEATURE_INCOMPLETE = -192,    //!< The requested workstation feature set has incomplete driver internal allocation resources
	NVAPI_STEREO_INIT_ACTIVATION_NOT_DONE = -193,    //!< Call failed because InitActivation was not called.
	NVAPI_SYNC_NOT_ACTIVE = -194,    //!< The requested action cannot be performed without Sync being enabled.    
	NVAPI_SYNC_MASTER_NOT_FOUND = -195,    //!< The requested action cannot be performed without Sync Master being enabled.
	NVAPI_INVALID_SYNC_TOPOLOGY = -196,    //!< Invalid displays passed in the NV_GSYNC_DISPLAY pointer.
	NVAPI_ECID_SIGN_ALGO_UNSUPPORTED = -197,    //!< The specified signing algorithm is not supported. Either an incorrect value was entered or the current installed driver/hardware does not support the input value.
	NVAPI_ECID_KEY_VERIFICATION_FAILED = -198,    //!< The encrypted public key verification has failed.
	NVAPI_FIRMWARE_OUT_OF_DATE = -199,    //!< The device's firmware is out of date.
	NVAPI_FIRMWARE_REVISION_NOT_SUPPORTED = -200,    //!< The device's firmware is not supported.
	NVAPI_LICENSE_CALLER_AUTHENTICATION_FAILED = -201,    //!< The caller is not authorized to modify the License.
	NVAPI_D3D_DEVICE_NOT_REGISTERED = -202,    //!< The user tried to use a deferred context without registering the device first  
	NVAPI_RESOURCE_NOT_ACQUIRED = -203,    //!< Head or SourceId was not reserved for the VR Display before doing the Modeset or the dedicated display.
	NVAPI_TIMING_NOT_SUPPORTED = -204,    //!< Provided timing is not supported.
	NVAPI_HDCP_ENCRYPTION_FAILED = -205,    //!< HDCP Encryption Failed for the device. Would be applicable when the device is HDCP Capable.
	NVAPI_PCLK_LIMITATION_FAILED = -206,    //!< Provided mode is over sink device pclk limitation.
	NVAPI_NO_CONNECTOR_FOUND = -207,    //!< No connector on GPU found. 
	NVAPI_HDCP_DISABLED = -208,    //!< When a non-HDCP capable HMD is connected, we would inform user by this code.
	NVAPI_API_IN_USE = -209,    //!< Atleast an API is still being called
	NVAPI_NVIDIA_DISPLAY_NOT_FOUND = -210,    //!< No display found on Nvidia GPU(s).
	NVAPI_PRIV_SEC_VIOLATION = -211,    //!< Priv security violation, improper access to a secured register.
	NVAPI_INCORRECT_VENDOR = -212,    //!< NVAPI cannot be called by this vendor
	NVAPI_DISPLAY_IN_USE = -213,    //!< DirectMode Display is already in use
	NVAPI_UNSUPPORTED_CONFIG_NON_HDCP_HMD = -214,    //!< The Config is having Non-NVidia GPU with Non-HDCP HMD connected
	NVAPI_MAX_DISPLAY_LIMIT_REACHED = -215,    //!< GPU's Max Display Limit has Reached
	NVAPI_INVALID_DIRECT_MODE_DISPLAY = -216,    //!< DirectMode not Enabled on the Display
	NVAPI_GPU_IN_DEBUG_MODE = -217,    //!< GPU is in debug mode, OC is NOT allowed.
	NVAPI_D3D_CONTEXT_NOT_FOUND = -218,    //!< No NvAPI context was found for this D3D object
	NVAPI_STEREO_VERSION_MISMATCH = -219,    //!< there is version mismatch between stereo driver and dx driver
	NVAPI_GPU_NOT_POWERED = -220,    //!< GPU is not powered and so the request cannot be completed.
	NVAPI_ERROR_DRIVER_RELOAD_IN_PROGRESS = -221,    //!< The display driver update in progress.
	NVAPI_WAIT_FOR_HW_RESOURCE = -222,    //!< Wait for HW resources allocation
	NVAPI_REQUIRE_FURTHER_HDCP_ACTION = -223,    //!< operation requires further HDCP action
	NVAPI_DISPLAY_MUX_TRANSITION_FAILED = -224,    //!< Dynamic Mux transition failure
	NVAPI_INVALID_DSC_VERSION = -225,    //!< Invalid DSC version
	NVAPI_INVALID_DSC_SLICECOUNT = -226,    //!< Invalid DSC slice count
	NVAPI_INVALID_DSC_OUTPUT_BPP = -227,    //!< Invalid DSC output BPP
	NVAPI_FAILED_TO_LOAD_FROM_DRIVER_STORE = -228,    //!< There was an error while loading nvapi.dll from the driver store.
	NVAPI_NO_VULKAN = -229,    //!< OpenGL does not export Vulkan fake extensions
	NVAPI_REQUEST_PENDING = -230,    //!< A request for NvTOPPs telemetry CData has already been made and is pending a response.
	NVAPI_RESOURCE_IN_USE = -231,    //!< Operation cannot be performed because the resource is in use.
	NVAPI_INVALID_IMAGE = -232,    //!< Device kernel image is invalid
	NVAPI_INVALID_PTX = -233,    //!< PTX JIT compilation failed
	NVAPI_NVLINK_UNCORRECTABLE = -234,    //!< Uncorrectable NVLink error was detected during the execution
	NVAPI_JIT_COMPILER_NOT_FOUND = -235,    //!< PTX JIT compiler library was not found.
	NVAPI_INVALID_SOURCE = -236,    //!< Device kernel source is invalid.
	NVAPI_ILLEGAL_INSTRUCTION = -237,    //!< While executing a kernel, the device encountered an illegal instruction.
	NVAPI_INVALID_PC = -238,    //!< While executing a kernel, the device program counter wrapped its address space
	NVAPI_LAUNCH_FAILED = -239,    //!< An exception occurred on the device while executing a kernel
	NVAPI_NOT_PERMITTED = -240,    //!< Attempted operation is not permitted.
	NVAPI_CALLBACK_ALREADY_REGISTERED = -241,    //!< The callback function has already been registered.
	NVAPI_CALLBACK_NOT_FOUND = -242,    //!< The callback function is not found or not registered.
	NVAPI_INVALID_OUTPUT_WIRE_FORMAT = -243,    //!< Invalid Wire Format for the VR HMD
} NvAPI_Status;

#define NVAPI_INTERFACE NvAPI_Status

#define NV_U8_MAX       (+255U)
#define NV_U16_MAX      (+65535U)
#define NV_S32_MAX      (+2147483647)
#define NV_U32_MIN      (0U)
#define NV_U32_MAX      (+4294967295U)
#define NV_U64_MAX      (+18446744073709551615ULL)

/* 64-bit types for compilers that support them, plus some obsolete variants */
#if defined(__GNUC__) || defined(__arm) || defined(__IAR_SYSTEMS_ICC__) || defined(__ghs__) || defined(_WIN64)
typedef unsigned long long NvU64; /* 0 to 18446744073709551615  */
typedef long long          NvS64; /* -9223372036854775808 to 9223372036854775807  */
#else
typedef unsigned __int64   NvU64; /* 0 to 18446744073709551615  */
typedef __int64            NvS64; /* -9223372036854775808 to 9223372036854775807  */
#endif

#ifndef NVAPI_USE_STDINT
#define NVAPI_USE_STDINT 0
#endif

#if NVAPI_USE_STDINT
typedef uint32_t           NvV32; /* "void": enumerated or multiple fields   */
typedef uint32_t           NvU32; /* 0 to 4294967295                         */
typedef  int32_t           NvS32; /* -2147483648 to 2147483647               */
#else
// mac os 32-bit still needs this
#if (defined(macintosh) || defined(__APPLE__)) && !defined(__LP64__)
typedef signed long        NvS32; /* -2147483648 to 2147483647  */
#else
typedef signed int         NvS32; /* -2147483648 to 2147483647 */
#endif

#if !((defined(NV_UNIX)) ||  (defined(__unix)))
// mac os 32-bit still needs this
#if ( (defined(macintosh) && defined(__LP64__) && (__NVAPI_RESERVED0__)) || \
      (!defined(macintosh) && defined(__NVAPI_RESERVED0__)) ) 
typedef unsigned int       NvU32; /* 0 to 4294967295                         */
#elif defined(__clang__)
typedef unsigned int       NvU32; /* 0 to 4294967295                         */
#else
typedef unsigned long      NvU32; /* 0 to 4294967295                         */
#endif
#else
typedef unsigned int       NvU32; /* 0 to 4294967295                         */
#endif
#endif

typedef unsigned long    temp_NvU32; /* 0 to 4294967295                         */
typedef signed   short   NvS16;
typedef unsigned short   NvU16;
typedef unsigned char    NvU8;
typedef signed   char    NvS8;
typedef float            NvF32;
typedef double           NvF64;

/*!
 * Macro to convert NvU32 to NvF32.
 */
#define NvU32TONvF32(_pData) *(NvF32 *)(_pData)
 /*!
  * Macro to convert NvF32 to NvU32.
  */
#define NvF32TONvU32(_pData) *(NvU32 *)(_pData)

#define NVAPI_SDK_VERSION 58087
  /* Boolean type */
typedef NvU8 NvBool;
#define NV_TRUE           ((NvBool)(0 == 0))
#define NV_FALSE          ((NvBool)(0 != 0))

#define NV_DECLARE_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name

NV_DECLARE_HANDLE(NvPhysicalGpuHandle);            //!< A single physical GPU
NV_DECLARE_HANDLE(NvDisplayHandle);                //!< Display Device driven by NVIDIA GPU(s) (an attached display)

#define NV_BIT(x)    (1 << (x))

#define NVAPI_GENERIC_STRING_MAX    4096
#define NVAPI_LONG_STRING_MAX       256
#define NVAPI_SHORT_STRING_MAX      64

#ifndef NvGUID_Defined
#define NvGUID_Defined
typedef struct
{
	NvU32 data1;
	NvU16 data2;
	NvU16 data3;
	NvU8  data4[8];
} NvGUID, NvLUID;
#endif //#ifndef NvGUID_Defined

#define NVAPI_MAX_PHYSICAL_GPUS             64

#define NVAPI_MAX_PHYSICAL_BRIDGES          100
#define NVAPI_PHYSICAL_GPUS                 32
#define NVAPI_MAX_LOGICAL_GPUS              64
#define NVAPI_MAX_AVAILABLE_GPU_TOPOLOGIES  256
#define NVAPI_MAX_AVAILABLE_SLI_GROUPS      256
#define NVAPI_MAX_GPU_TOPOLOGIES            NVAPI_MAX_PHYSICAL_GPUS
#define NVAPI_MAX_GPU_PER_TOPOLOGY          8
#define NVAPI_MAX_DISPLAY_HEADS             2
#define NVAPI_ADVANCED_DISPLAY_HEADS        4
#define NVAPI_MAX_DISPLAYS                  NVAPI_PHYSICAL_GPUS * NVAPI_ADVANCED_DISPLAY_HEADS
#define NVAPI_MAX_ACPI_IDS                  16
#define NVAPI_MAX_VIEW_MODES                8

typedef char NvAPI_String[NVAPI_GENERIC_STRING_MAX];
typedef char NvAPI_LongString[NVAPI_LONG_STRING_MAX];
typedef char NvAPI_ShortString[NVAPI_SHORT_STRING_MAX];
typedef NvU16 NvAPI_UnicodeShortString[NVAPI_SHORT_STRING_MAX];

#define MAKE_NVAPI_VERSION(typeName,ver) (NvU32)(sizeof(typeName) | ((ver)<<16))
#define GET_NVAPI_VERSION(ver) (NvU32)((ver)>>16)
#define GET_NVAPI_SIZE(ver) (NvU32)((ver) & 0xffff)

#define NVAPI_UNICODE_STRING_MAX                             2048
#define NVAPI_BINARY_DATA_MAX                                4096

typedef NvU16 NvAPI_UnicodeString[NVAPI_UNICODE_STRING_MAX];
typedef const NvU16* NvAPI_LPCWSTR;

#ifndef _WIN32
#define __cdecl
#endif

NVAPI_INTERFACE NvAPI_Initialize(void);

NVAPI_INTERFACE NvAPI_Unload(void);

typedef struct
{
	NvU32   version;                        //!< Version info
	NvU32   dedicatedVideoMemory;           //!< Size(in kb) of the physical framebuffer.
	NvU32   availableDedicatedVideoMemory;  //!< Size(in kb) of the available physical framebuffer for allocating video memory surfaces.
	NvU32   systemVideoMemory;              //!< Size(in kb) of system memory the driver allocates at load time.
	NvU32   sharedSystemMemory;             //!< Size(in kb) of shared system memory that driver is allowed to commit for surfaces across all allocations.
} NV_DISPLAY_DRIVER_MEMORY_INFO_V1;

typedef struct
{
	NvU32   version;                           //!< Version info
	NvU32   dedicatedVideoMemory;              //!< Size(in kb) of the physical framebuffer.
	NvU32   availableDedicatedVideoMemory;     //!< Size(in kb) of the available physical framebuffer for allocating video memory surfaces.
	NvU32   systemVideoMemory;                 //!< Size(in kb) of system memory the driver allocates at load time.
	NvU32   sharedSystemMemory;                //!< Size(in kb) of shared system memory that driver is allowed to commit for surfaces across all allocations.
	NvU32   curAvailableDedicatedVideoMemory;  //!< Size(in kb) of the current available physical framebuffer for allocating video memory surfaces.
} NV_DISPLAY_DRIVER_MEMORY_INFO_V2;

typedef struct
{
	NvU32   version;                           //!< Version info
	NvU32   dedicatedVideoMemory;              //!< Size(in kb) of the physical framebuffer.
	NvU32   availableDedicatedVideoMemory;     //!< Size(in kb) of the available physical framebuffer for allocating video memory surfaces.
	NvU32   systemVideoMemory;                 //!< Size(in kb) of system memory the driver allocates at load time.
	NvU32   sharedSystemMemory;                //!< Size(in kb) of shared system memory that driver is allowed to commit for surfaces across all allocations.
	NvU32   curAvailableDedicatedVideoMemory;  //!< Size(in kb) of the current available physical framebuffer for allocating video memory surfaces.
	NvU32   dedicatedVideoMemoryEvictionsSize; //!< Size(in kb) of the total size of memory released as a result of the evictions.
	NvU32   dedicatedVideoMemoryEvictionCount; //!< Indicates the number of eviction events that caused an allocation to be removed from dedicated video memory to free GPU
	//!< video memory to make room for other allocations.
} NV_DISPLAY_DRIVER_MEMORY_INFO_V3;

typedef NV_DISPLAY_DRIVER_MEMORY_INFO_V3 NV_DISPLAY_DRIVER_MEMORY_INFO;

#define NV_DISPLAY_DRIVER_MEMORY_INFO_VER_1  MAKE_NVAPI_VERSION(NV_DISPLAY_DRIVER_MEMORY_INFO_V1,1)

#define NV_DISPLAY_DRIVER_MEMORY_INFO_VER_2  MAKE_NVAPI_VERSION(NV_DISPLAY_DRIVER_MEMORY_INFO_V2,2)

#define NV_DISPLAY_DRIVER_MEMORY_INFO_VER_3  MAKE_NVAPI_VERSION(NV_DISPLAY_DRIVER_MEMORY_INFO_V3,3)

#define NV_DISPLAY_DRIVER_MEMORY_INFO_VER    NV_DISPLAY_DRIVER_MEMORY_INFO_VER_3

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetMemoryInfo
//
//!   DESCRIPTION: This function retrieves the available driver memory footprint for the specified GPU. 
//!                If the GPU is in TCC Mode, only dedicatedVideoMemory will be returned in pMemoryInfo (NV_DISPLAY_DRIVER_MEMORY_INFO).
//!
//! \deprecated  Do not use this function - it is deprecated in release 520. Instead, use NvAPI_GPU_GetMemoryInfoEx.
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! TCC_SUPPORTED
//!
//! \since Release: 177
//!
//!  \param [in]   hPhysicalGpu  Handle of the physical GPU for which the memory information is to be extracted.
//!  \param [out]  pMemoryInfo   The memory footprint available in the driver. See NV_DISPLAY_DRIVER_MEMORY_INFO.
//!
//!  \retval       NVAPI_INVALID_ARGUMENT             pMemoryInfo is NULL.
//!  \retval       NVAPI_OK                           Call successful.
//!  \retval       NVAPI_NVIDIA_DEVICE_NOT_FOUND      No NVIDIA GPU driving a display was found.
//!  \retval       NVAPI_INCOMPATIBLE_STRUCT_VERSION  NV_DISPLAY_DRIVER_MEMORY_INFO structure version mismatch.
//!
//!  \ingroup  driverapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetMemoryInfo(NvPhysicalGpuHandle hPhysicalGpu, NV_DISPLAY_DRIVER_MEMORY_INFO* pMemoryInfo);


typedef struct
{
	NvU32   version;                           //!< Structure version
	NvU64   dedicatedVideoMemory;              //!< Size(in bytes) of the physical framebuffer. Refers to the dedicated video memory on discrete GPUs.
	//!  It is more performant for GPU operations than the reserved systemVideoMemory.
	NvU64   availableDedicatedVideoMemory;     //!< Size(in bytes) of the available physical framebuffer for allocating video memory surfaces.
	NvU64   systemVideoMemory;                 //!< Size(in bytes) of system memory the driver allocates at load time. It is a substitute for dedicated video memory.
	//!< Typically used with integrated GPUs that do not have dedicated video memory.
	NvU64   sharedSystemMemory;                //!< Size(in bytes) of shared system memory that driver is allowed to commit for surfaces across all allocations.
	//!< On discrete GPUs, it is used to utilize system memory for various operations. It does not need to be reserved during boot.
	//!< It may be used by both GPU and CPU, and has an "on-demand" type of usage.
	NvU64   curAvailableDedicatedVideoMemory;  //!< Size(in bytes) of the current available physical framebuffer for allocating video memory surfaces.
	NvU64   dedicatedVideoMemoryEvictionsSize; //!< Size(in bytes) of the total size of memory released as a result of the evictions.
	NvU64   dedicatedVideoMemoryEvictionCount; //!< Indicates the number of eviction events that caused an allocation to be removed from dedicated video memory to free GPU
	//!< video memory to make room for other allocations.
	NvU64 dedicatedVideoMemoryPromotionsSize;  //!< Size(in bytes) of the total size of memory allocated as a result of the promotions.
	NvU64 dedicatedVideoMemoryPromotionCount;  //!< Indicates the number of promotion events that caused an allocation to be promoted to dedicated video memory 
} NV_GPU_MEMORY_INFO_EX_V1;
typedef NV_GPU_MEMORY_INFO_EX_V1 NV_GPU_MEMORY_INFO_EX;

#define NV_GPU_MEMORY_INFO_EX_VER_1  MAKE_NVAPI_VERSION(NV_GPU_MEMORY_INFO_EX_V1,1)

#define NV_GPU_MEMORY_INFO_EX_VER    NV_GPU_MEMORY_INFO_EX_VER_1

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetMemoryInfoEx
//
//!   DESCRIPTION: This function retrieves the available driver memory footprint for the specified GPU. 
//!                If the GPU is in TCC Mode, only dedicatedVideoMemory will be returned in pMemoryInfo (NV_GPU_MEMORY_INFO_EX).
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//!
//! \since Release: 520
//!
//!  \param [in]   hPhysicalGpu  Handle of the physical GPU for which the memory information is to be extracted.
//!  \param [out]  pMemoryInfo   The memory footprint available in the driver. See NV_GPU_MEMORY_INFO_EX.
//!
//!  \retval       NVAPI_INVALID_ARGUMENT             pMemoryInfo is NULL.
//!  \retval       NVAPI_OK                           Call successful.
//!  \retval       NVAPI_NVIDIA_DEVICE_NOT_FOUND      No NVIDIA GPU driving a display was found.
//!  \retval       NVAPI_INCOMPATIBLE_STRUCT_VERSION  NV_GPU_MEMORY_INFO_EX structure version mismatch.
//!
//!  \ingroup  driverapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetMemoryInfoEx(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_MEMORY_INFO_EX* pMemoryInfo);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_EnumPhysicalGPUs
//
//! This function returns an array of physical GPU handles.
//! Each handle represents a physical GPU present in the system.
//! That GPU may be part of an SLI configuration, or may not be visible to the OS directly.
//!
//! At least one GPU must be present in the system and running an NVIDIA display driver.
//!
//! The array nvGPUHandle will be filled with physical GPU handle values. The returned
//! gpuCount determines how many entries in the array are valid.
//!
//! \note In drivers older than 105.00, all physical GPU handles get invalidated on a
//!       modeset. So the calling applications need to renum the handles after every modeset.\n
//!       With drivers 105.00 and up, all physical GPU handles are constant.
//!       Physical GPU handles are constant as long as the GPUs are not physically moved and
//!       the SBIOS VGA order is unchanged.
//!
//!       For GPU handles in TCC MODE please use NvAPI_EnumTCCPhysicalGPUs()
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! \par Introduced in
//! \since Release: 80
//!
//! \retval NVAPI_INVALID_ARGUMENT         nvGPUHandle or pGpuCount is NULL
//! \retval NVAPI_OK                       One or more handles were returned
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND  No NVIDIA GPU driving a display was found
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_EnumPhysicalGPUs(NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32* pGpuCount);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_EnumNvidiaDisplayHandle
//
//! This function returns the handle of the NVIDIA display specified by the enum
//!                index (thisEnum). The client should keep enumerating until it
//!                returns error.
//!
//!                Note: Display handles can get invalidated on a modeset, so the calling applications need to
//!                renum the handles after every modeset.
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! \since Release: 80
//!
//! \param [in]  thisEnum      The index of the NVIDIA display.
//! \param [out] pNvDispHandle Pointer to the NVIDIA display handle.
//!
//! \retval NVAPI_INVALID_ARGUMENT        Either the handle pointer is NULL or enum index too big
//! \retval NVAPI_OK                      Return a valid NvDisplayHandle based on the enum index
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND No NVIDIA device found in the system
//! \retval NVAPI_END_ENUMERATION         No more display device to enumerate
//! \ingroup disphandle
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_EnumNvidiaDisplayHandle(NvU32 thisEnum, NvDisplayHandle* pNvDispHandle);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetPhysicalGPUsFromDisplay
//
//! This function returns an array of physical GPU handles associated with the specified display.
//!
//! At least one GPU must be present in the system and running an NVIDIA display driver.
//!
//! The array nvGPUHandle will be filled with physical GPU handle values.  The returned
//! gpuCount determines how many entries in the array are valid.
//!
//! If the display corresponds to more than one physical GPU, the first GPU returned
//! is the one with the attached active output.
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! \since Release: 80
//!
//! \retval NVAPI_INVALID_ARGUMENT         hNvDisp is not valid; nvGPUHandle or pGpuCount is NULL
//! \retval NVAPI_OK                       One or more handles were returned
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND  no NVIDIA GPU driving a display was found
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetPhysicalGPUsFromDisplay(NvDisplayHandle hNvDisp, NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32* pGpuCount);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetFullName
//
//!  This function retrieves the full GPU name as an ASCII string - for example, "Quadro FX 1400".
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//!
//! \since Release: 90
//!
//! \return  NVAPI_ERROR or NVAPI_OK
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetFullName(NvPhysicalGpuHandle hPhysicalGpu, NvAPI_ShortString szName);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetPCIIdentifiers
//
//!  This function returns the PCI identifiers associated with this GPU.
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//!
//! \since Release: 90
//!
//! \param   DeviceId      The internal PCI device identifier for the GPU.
//! \param   SubSystemId   The internal PCI subsystem identifier for the GPU.
//! \param   RevisionId    The internal PCI device-specific revision identifier for the GPU.
//! \param   ExtDeviceId   The external PCI device identifier for the GPU.
//!
//! \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or an argument is NULL
//! \retval  NVAPI_OK                            Arguments are populated with PCI identifiers
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetPCIIdentifiers(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pDeviceId, NvU32* pSubSystemId, NvU32* pRevisionId, NvU32* pExtDeviceId);

typedef enum _NV_GPU_TYPE
{
	NV_SYSTEM_TYPE_GPU_UNKNOWN = 0,
	NV_SYSTEM_TYPE_IGPU = 1, //!< Integrated GPU
	NV_SYSTEM_TYPE_DGPU = 2, //!< Discrete GPU
} NV_GPU_TYPE;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetGPUType
//
//!  DESCRIPTION: This function returns the GPU type (integrated or discrete).
//!               See ::NV_GPU_TYPE.
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//!
//! \since Release: 173
//!
//! \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu
//! \retval  NVAPI_OK                           *pGpuType contains the GPU type
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//!
//!  \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetGPUType(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_TYPE* pGpuType);

typedef enum _NV_GPU_BUS_TYPE
{
	NVAPI_GPU_BUS_TYPE_UNDEFINED = 0,
	NVAPI_GPU_BUS_TYPE_PCI = 1,
	NVAPI_GPU_BUS_TYPE_AGP = 2,
	NVAPI_GPU_BUS_TYPE_PCI_EXPRESS = 3,
	NVAPI_GPU_BUS_TYPE_FPCI = 4,
	NVAPI_GPU_BUS_TYPE_AXI = 5,
} NV_GPU_BUS_TYPE;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetBusType
//
//!  This function returns the type of bus associated with this GPU.
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//!
//! \since Release: 90
//!
//! \return      This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with
//!              specific meaning for this API, they are listed below.
//! \retval      NVAPI_INVALID_ARGUMENT             hPhysicalGpu or pBusType is NULL.
//! \retval      NVAPI_OK                          *pBusType contains bus identifier.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetBusType(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_BUS_TYPE* pBusType);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetBusId
//
//!   DESCRIPTION: Returns the ID of the bus associated with this GPU.
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//!
//! \since Release: 167
//!
//!  \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pBusId is NULL.
//!  \retval  NVAPI_OK                           *pBusId contains the bus ID.
//!  \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//!  \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//!
//!  \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetBusId(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pBusId);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetBusSlotId
//
//!   DESCRIPTION: Returns the ID of the bus slot associated with this GPU.
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//!
//! \since Release: 167
//!
//!  \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pBusSlotId is NULL.
//!  \retval  NVAPI_OK                           *pBusSlotId contains the bus slot ID.
//!  \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//!  \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//!
//!  \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetBusSlotId(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pBusSlotId);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetVbiosRevision
//
//!  This function returns the revision of the video BIOS associated with this GPU.
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//!
//! \since Release: 90
//!
//! \retval    NVAPI_INVALID_ARGUMENT               hPhysicalGpu or pBiosRevision is NULL.
//! \retval    NVAPI_OK                            *pBiosRevision contains revision number.
//! \retval    NVAPI_NVIDIA_DEVICE_NOT_FOUND        No NVIDIA GPU driving a display was found.
//! \retval    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE   hPhysicalGpu was not a physical GPU handle.
//! \ingroup   gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetVbiosRevision(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pBiosRevision);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetVbiosOEMRevision
//
//!  This function returns the OEM revision of the video BIOS associated with this GPU.
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//!
//! \since Release: 90
//!
//! \retval    NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pBiosRevision is NULL
//! \retval    NVAPI_OK                           *pBiosRevision contains revision number
//! \retval    NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle
//! \ingroup   gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetVbiosOEMRevision(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pBiosRevision);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetVbiosVersionString
//
//!  This function returns the full video BIOS version string in the form of xx.xx.xx.xx.yy where
//!  - xx numbers come from NvAPI_GPU_GetVbiosRevision() and
//!  - yy comes from NvAPI_GPU_GetVbiosOEMRevision().
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//!
//! \since Release: 90
//!
//! \retval   NVAPI_INVALID_ARGUMENT              hPhysicalGpu is NULL.
//! \retval   NVAPI_OK                            szBiosRevision contains version string.
//! \retval   NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval   NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetVbiosVersionString(NvPhysicalGpuHandle hPhysicalGpu, NvAPI_ShortString szBiosRevision);

#define NVAPI_MAX_GPU_CLOCKS            32
#define NVAPI_MAX_GPU_PUBLIC_CLOCKS     32
#define NVAPI_MAX_GPU_PERF_CLOCKS       32
#define NVAPI_MAX_GPU_PERF_VOLTAGES     16
#define NVAPI_MAX_GPU_PERF_PSTATES      16

typedef enum _NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID
{
	NVAPI_GPU_PERF_VOLTAGE_INFO_DOMAIN_CORE = 0,
	NVAPI_GPU_PERF_VOLTAGE_INFO_DOMAIN_UNDEFINED = NVAPI_MAX_GPU_PERF_VOLTAGES,
} NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID;

typedef enum _NV_GPU_PUBLIC_CLOCK_ID
{
	NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS = 0,
	NVAPI_GPU_PUBLIC_CLOCK_MEMORY = 4,
	NVAPI_GPU_PUBLIC_CLOCK_PROCESSOR = 7,
	NVAPI_GPU_PUBLIC_CLOCK_VIDEO = 8,
	NVAPI_GPU_PUBLIC_CLOCK_UNDEFINED = NVAPI_MAX_GPU_PUBLIC_CLOCKS,
} NV_GPU_PUBLIC_CLOCK_ID;

typedef enum _NV_GPU_PERF_PSTATE_ID
{
	NVAPI_GPU_PERF_PSTATE_P0 = 0,
	NVAPI_GPU_PERF_PSTATE_P1,
	NVAPI_GPU_PERF_PSTATE_P2,
	NVAPI_GPU_PERF_PSTATE_P3,
	NVAPI_GPU_PERF_PSTATE_P4,
	NVAPI_GPU_PERF_PSTATE_P5,
	NVAPI_GPU_PERF_PSTATE_P6,
	NVAPI_GPU_PERF_PSTATE_P7,
	NVAPI_GPU_PERF_PSTATE_P8,
	NVAPI_GPU_PERF_PSTATE_P9,
	NVAPI_GPU_PERF_PSTATE_P10,
	NVAPI_GPU_PERF_PSTATE_P11,
	NVAPI_GPU_PERF_PSTATE_P12,
	NVAPI_GPU_PERF_PSTATE_P13,
	NVAPI_GPU_PERF_PSTATE_P14,
	NVAPI_GPU_PERF_PSTATE_P15,
	NVAPI_GPU_PERF_PSTATE_UNDEFINED = NVAPI_MAX_GPU_PERF_PSTATES,
	NVAPI_GPU_PERF_PSTATE_ALL,
} NV_GPU_PERF_PSTATE_ID;

typedef struct
{
	NvU32   version;
	NvU32   flags;           //!< - bit 0 indicates if perfmon is enabled or not
	//!< - bit 1 indicates if dynamic Pstate is capable or not
	//!< - bit 2 indicates if dynamic Pstate is enable or not
	//!< - all other bits must be set to 0
	NvU32   numPstates;      //!< The number of available p-states
	NvU32   numClocks;       //!< The number of clock domains supported by each P-State
	struct
	{
		NV_GPU_PERF_PSTATE_ID   pstateId; //!< ID of the p-state.
		NvU32                   flags;    //!< - bit 0 indicates if the PCIE limit is GEN1 or GEN2
		//!< - bit 1 indicates if the Pstate is overclocked or not
		//!< - bit 2 indicates if the Pstate is overclockable or not
		//!< - all other bits must be set to 0
		struct
		{
			NV_GPU_PUBLIC_CLOCK_ID           domainId;  //!< ID of the clock domain
			NvU32                               flags;  //!< Reserved. Must be set to 0
			NvU32                                freq;  //!< Clock frequency in kHz

		} clocks[NVAPI_MAX_GPU_PERF_CLOCKS];
	} pstates[NVAPI_MAX_GPU_PERF_PSTATES];

} NV_GPU_PERF_PSTATES_INFO_V1;

typedef struct
{
	NvU32   version;
	NvU32   flags;             //!< - bit 0 indicates if perfmon is enabled or not
	//!< - bit 1 indicates if dynamic Pstate is capable or not
	//!< - bit 2 indicates if dynamic Pstate is enable or not
	//!< - all other bits must be set to 0
	NvU32   numPstates;        //!< The number of available p-states
	NvU32   numClocks;         //!< The number of clock domains supported by each P-State
	NvU32   numVoltages;
	struct
	{
		NV_GPU_PERF_PSTATE_ID   pstateId;  //!< ID of the p-state.
		NvU32                   flags;     //!< - bit 0 indicates if the PCIE limit is GEN1 or GEN2
		//!< - bit 1 indicates if the Pstate is overclocked or not
		//!< - bit 2 indicates if the Pstate is overclockable or not
		//!< - all other bits must be set to 0
		struct
		{
			NV_GPU_PUBLIC_CLOCK_ID            domainId;
			NvU32                                flags; //!< bit 0 indicates if this clock is overclockable
			//!< all other bits must be set to 0
			NvU32                                 freq;

		} clocks[NVAPI_MAX_GPU_PERF_CLOCKS];
		struct
		{
			NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID domainId; //!< ID of the voltage domain, containing flags and mvolt info
			NvU32                       flags;           //!< Reserved for future use. Must be set to 0
			NvU32                       mvolt;           //!< Voltage in mV

		} voltages[NVAPI_MAX_GPU_PERF_VOLTAGES];

	} pstates[NVAPI_MAX_GPU_PERF_PSTATES];  //!< Valid index range is 0 to numVoltages-1

} NV_GPU_PERF_PSTATES_INFO_V2;

typedef  NV_GPU_PERF_PSTATES_INFO_V2 NV_GPU_PERF_PSTATES_INFO;

#define NV_GPU_PERF_PSTATES_INFO_VER1  MAKE_NVAPI_VERSION(NV_GPU_PERF_PSTATES_INFO_V1,1)

#define NV_GPU_PERF_PSTATES_INFO_VER2  MAKE_NVAPI_VERSION(NV_GPU_PERF_PSTATES_INFO_V2,2)

#define NV_GPU_PERF_PSTATES_INFO_VER3  MAKE_NVAPI_VERSION(NV_GPU_PERF_PSTATES_INFO_V2,3)

#define NV_GPU_PERF_PSTATES_INFO_VER   NV_GPU_PERF_PSTATES_INFO_VER3

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetPstatesInfoEx
//
//! DESCRIPTION:     This API retrieves all performance states (P-States) information.
//!
//!                  P-States are GPU active/executing performance capability and power consumption states.
//!
//!                  P-States ranges from P0 to P15, with P0 being the highest performance/power state, and
//!                  P15 being the lowest performance/power state. Each P-State, if available, maps to a
//!                  performance level. Not all P-States are available on a given system. The definitions
//!                  of each P-State are currently as follows: \n
//!                  - P0/P1 - Maximum 3D performance
//!                  - P2/P3 - Balanced 3D performance-power
//!                  - P8 - Basic HD video playback
//!                  - P10 - DVD playback
//!                  - P12 - Minimum idle power consumption
//!
//! \deprecated  Do not use this function - it is deprecated in release 304. Instead, use NvAPI_GPU_GetPstates20.
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! \param [in]     hPhysicalGPU       GPU selection.
//! \param [out]    pPerfPstatesInfo   P-States information retrieved, as detailed below: \n
//!                  - flags is reserved for future use.
//!                  - numPstates is the number of available P-States
//!                  - numClocks is the number of clock domains supported by each P-State
//!                  - pstates has valid index range from 0 to numPstates - 1
//!                  - pstates[i].pstateId is the ID of the P-State,
//!                      containing the following info:
//!                    - pstates[i].flags containing the following info:
//!                        - bit 0 indicates if the PCIE limit is GEN1 or GEN2
//!                        - bit 1 indicates if the Pstate is overclocked or not
//!                        - bit 2 indicates if the Pstate is overclockable or not
//!                    - pstates[i].clocks has valid index range from 0 to numClocks -1
//!                    - pstates[i].clocks[j].domainId is the public ID of the clock domain,
//!                        containing the following info:
//!                      - pstates[i].clocks[j].flags containing the following info:
//!                          bit 0 indicates if the clock domain is overclockable or not
//!                      - pstates[i].clocks[j].freq is the clock frequency in kHz
//!                    - pstates[i].voltages has a valid index range from 0 to numVoltages - 1
//!                    - pstates[i].voltages[j].domainId is the ID of the voltage domain,
//!                        containing the following info:
//!                      - pstates[i].voltages[j].flags is reserved for future use.
//!                      - pstates[i].voltages[j].mvolt is the voltage in mV
//!                  inputFlags(IN)   - This can be used to select various options:
//!                    - if bit 0 is set, pPerfPstatesInfo would contain the default settings
//!                        instead of the current, possibily overclocked settings.
//!                    - if bit 1 is set, pPerfPstatesInfo would contain the maximum clock
//!                        frequencies instead of the nominal frequencies.
//!                    - if bit 2 is set, pPerfPstatesInfo would contain the minimum clock
//!                        frequencies instead of the nominal frequencies.
//!                    - all other bits must be set to 0.
//!
//! \retval ::NVAPI_OK                            Completed request
//! \retval ::NVAPI_ERROR                         Miscellaneous error occurred
//! \retval ::NVAPI_HANDLE_INVALIDATED            Handle passed has been invalidated (see user guide)
//! \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  Handle passed is not a physical GPU handle
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION   The version of the NV_GPU_PERF_PSTATES struct is not supported
//!
//! \ingroup gpupstate
///////////////////////////////////////////////////////////////////////////////
// Do not use this function - it is deprecated in release 304. Instead, use NvAPI_GPU_GetPstates20.
NVAPI_INTERFACE NvAPI_GPU_GetPstatesInfoEx(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_PERF_PSTATES_INFO* pPerfPstatesInfo, NvU32 inputFlags);

#define NVAPI_MAX_GPU_PSTATE20_PSTATES          16
#define NVAPI_MAX_GPU_PSTATE20_CLOCKS           8
#define NVAPI_MAX_GPU_PSTATE20_BASE_VOLTAGES    4

typedef enum
{
	//! Clock domains that use single frequency value within given pstate
	NVAPI_GPU_PERF_PSTATE20_CLOCK_TYPE_SINGLE = 0,

	//! Clock domains that allow range of frequency values within given pstate
	NVAPI_GPU_PERF_PSTATE20_CLOCK_TYPE_RANGE,
} NV_GPU_PERF_PSTATE20_CLOCK_TYPE_ID;

typedef struct
{
	//! Value of parameter delta (in respective units [kHz, uV])
	NvS32       value;

	struct
	{
		//! Min value allowed for parameter delta (in respective units [kHz, uV])
		NvS32   min;

		//! Max value allowed for parameter delta (in respective units [kHz, uV])
		NvS32   max;
	} valueRange;
} NV_GPU_PERF_PSTATES20_PARAM_DELTA;

typedef struct
{
	//! ID of the clock domain
	NV_GPU_PUBLIC_CLOCK_ID                      domainId;

	//! Clock type ID
	NV_GPU_PERF_PSTATE20_CLOCK_TYPE_ID          typeId;
	NvU32                                       bIsEditable : 1;

	//! These bits are reserved for future use (must be always 0)
	NvU32                                       reserved : 31;

	//! Current frequency delta from nominal settings in (kHz)
	NV_GPU_PERF_PSTATES20_PARAM_DELTA           freqDelta_kHz;

	//! Clock domain type dependant information
	union
	{
		struct
		{
			//! Clock frequency within given pstate in (kHz)
			NvU32                               freq_kHz;
		} single;

		struct
		{
			//! Min clock frequency within given pstate in (kHz)
			NvU32                               minFreq_kHz;

			//! Max clock frequency within given pstate in (kHz)
			NvU32                               maxFreq_kHz;

			//! Voltage domain ID and value range in (uV) required for this clock
			NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID  domainId;
			NvU32                               minVoltage_uV;
			NvU32                               maxVoltage_uV;
		} range;
	} data;
} NV_GPU_PSTATE20_CLOCK_ENTRY_V1;

typedef struct
{
	//! ID of the voltage domain
	NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID  domainId;
	NvU32                               bIsEditable : 1;

	//! These bits are reserved for future use (must be always 0)
	NvU32                               reserved : 31;

	//! Current base voltage settings in [uV]
	NvU32                               volt_uV;

	NV_GPU_PERF_PSTATES20_PARAM_DELTA   voltDelta_uV; // Current base voltage delta from nominal settings in [uV]
} NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1;

typedef struct
{
	//! Version info of the structure (NV_GPU_PERF_PSTATES20_INFO_VER<n>)
	NvU32   version;

	NvU32   bIsEditable : 1;

	//! These bits are reserved for future use (must be always 0)
	NvU32   reserved : 31;

	//! Number of populated pstates
	NvU32   numPstates;

	//! Number of populated clocks (per pstate)
	NvU32   numClocks;

	//! Number of populated base voltages (per pstate)
	NvU32   numBaseVoltages;

	//! Performance state (P-State) settings
	//! Valid index range is 0 to numPstates-1
	struct
	{
		//! ID of the P-State
		NV_GPU_PERF_PSTATE_ID                   pstateId;

		NvU32                                   bIsEditable : 1;

		//! These bits are reserved for future use (must be always 0)
		NvU32                                   reserved : 31;

		//! Array of clock entries
		//! Valid index range is 0 to numClocks-1
		NV_GPU_PSTATE20_CLOCK_ENTRY_V1          clocks[NVAPI_MAX_GPU_PSTATE20_CLOCKS];

		//! Array of baseVoltage entries
		//! Valid index range is 0 to numBaseVoltages-1
		NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1   baseVoltages[NVAPI_MAX_GPU_PSTATE20_BASE_VOLTAGES];
	} pstates[NVAPI_MAX_GPU_PSTATE20_PSTATES];
} NV_GPU_PERF_PSTATES20_INFO_V1;

typedef struct _NV_GPU_PERF_PSTATES20_INFO_V2
{
	//! Version info of the structure (NV_GPU_PERF_PSTATES20_INFO_VER<n>)
	NvU32   version;

	NvU32   bIsEditable : 1;

	//! These bits are reserved for future use (must be always 0)
	NvU32   reserved : 31;

	//! Number of populated pstates
	NvU32   numPstates;

	//! Number of populated clocks (per pstate)
	NvU32   numClocks;

	//! Number of populated base voltages (per pstate)
	NvU32   numBaseVoltages;

	//! Performance state (P-State) settings
	//! Valid index range is 0 to numPstates-1
	struct
	{
		//! ID of the P-State
		NV_GPU_PERF_PSTATE_ID                   pstateId;

		NvU32                                   bIsEditable : 1;

		//! These bits are reserved for future use (must be always 0)
		NvU32                                   reserved : 31;

		//! Array of clock entries
		//! Valid index range is 0 to numClocks-1
		NV_GPU_PSTATE20_CLOCK_ENTRY_V1          clocks[NVAPI_MAX_GPU_PSTATE20_CLOCKS];

		//! Array of baseVoltage entries
		//! Valid index range is 0 to numBaseVoltages-1
		NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1   baseVoltages[NVAPI_MAX_GPU_PSTATE20_BASE_VOLTAGES];
	} pstates[NVAPI_MAX_GPU_PSTATE20_PSTATES];

	//! OV settings - Please refer to NVIDIA over-volting recommendation to understand impact of this functionality
	//! Valid index range is 0 to numVoltages-1
	struct
	{
		//! Number of populated voltages
		NvU32                                 numVoltages;

		//! Array of voltage entries
		//! Valid index range is 0 to numVoltages-1
		NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1 voltages[NVAPI_MAX_GPU_PSTATE20_BASE_VOLTAGES];
	} ov;
} NV_GPU_PERF_PSTATES20_INFO_V2;

typedef NV_GPU_PERF_PSTATES20_INFO_V2   NV_GPU_PERF_PSTATES20_INFO;

#define NV_GPU_PERF_PSTATES20_INFO_VER1 MAKE_NVAPI_VERSION(NV_GPU_PERF_PSTATES20_INFO_V1,1)

#define NV_GPU_PERF_PSTATES20_INFO_VER2 MAKE_NVAPI_VERSION(NV_GPU_PERF_PSTATES20_INFO_V2,2)

#define NV_GPU_PERF_PSTATES20_INFO_VER3 MAKE_NVAPI_VERSION(NV_GPU_PERF_PSTATES20_INFO_V2,3)

#define NV_GPU_PERF_PSTATES20_INFO_VER  NV_GPU_PERF_PSTATES20_INFO_VER3

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetPstates20
//
//! DESCRIPTION:    This API retrieves all performance states (P-States) 2.0 information.
//!
//!                 P-States are GPU active/executing performance capability states.
//!                 They range from P0 to P15, with P0 being the highest performance state,
//!                 and P15 being the lowest performance state. Each P-State, if available,
//!                 maps to a performance level. Not all P-States are available on a given system.
//!                 The definition of each P-States are currently as follow:
//!                 - P0/P1 - Maximum 3D performance
//!                 - P2/P3 - Balanced 3D performance-power
//!                 - P8 - Basic HD video playback
//!                 - P10 - DVD playback
//!                 - P12 - Minimum idle power consumption
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//!
//! \since Release: 295
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! \param [in]   hPhysicalGPU  GPU selection
//! \param [out]  pPstatesInfo  P-States information retrieved, as documented in declaration above
//!
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status.
//!          If there are return error codes with specific meaning for this API,
//!          they are listed below.
//!
//! \ingroup gpupstate
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetPstates20(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_PERF_PSTATES20_INFO* pPstatesInfo);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetCurrentPstate
//
//! DESCRIPTION:     This function retrieves the current performance state (P-State).
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! \since Release: 165
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//!
//! \param [in]      hPhysicalGPU     GPU selection
//! \param [out]     pCurrentPstate   The ID of the current P-State of the GPU - see \ref NV_GPU_PERF_PSTATES.
//!
//! \retval    NVAPI_OK                             Completed request
//! \retval    NVAPI_ERROR                          Miscellaneous error occurred.
//! \retval    NVAPI_HANDLE_INVALIDATED             Handle passed has been invalidated (see user guide).
//! \retval    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE   Handle passed is not a physical GPU handle.
//! \retval    NVAPI_NOT_SUPPORTED                  P-States is not supported on this setup.
//!
//! \ingroup   gpupstate
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetCurrentPstate(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_PERF_PSTATE_ID* pCurrentPstate);

#define NVAPI_MAX_GPU_UTILIZATIONS 8

typedef struct
{
	NvU32       version;        //!< Structure version
	NvU32       flags;          //!< bit 0 indicates if the dynamic Pstate is enabled or not
	struct
	{
		NvU32   bIsPresent : 1;   //!< Set if this utilization domain is present on this GPU
		NvU32   percentage;     //!< Percentage of time where the domain is considered busy in the last 1 second interval
	} utilization[NVAPI_MAX_GPU_UTILIZATIONS];
} NV_GPU_DYNAMIC_PSTATES_INFO_EX;

#define NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER MAKE_NVAPI_VERSION(NV_GPU_DYNAMIC_PSTATES_INFO_EX,1)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetDynamicPstatesInfoEx
//
//! DESCRIPTION:   This API retrieves the NV_GPU_DYNAMIC_PSTATES_INFO_EX structure for the specified physical GPU.
//!                Each domain's info is indexed in the array.  For example:
//!                - pDynamicPstatesInfo->utilization[NVAPI_GPU_UTILIZATION_DOMAIN_GPU] holds the info for the GPU domain. \p
//!                There are currently 4 domains for which GPU utilization and dynamic P-State thresholds can be retrieved:
//!                   graphic engine (GPU), frame buffer (FB), video engine (VID), and bus interface (BUS).
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//! \since Release: 185
//!
//! \retval ::NVAPI_OK
//! \retval ::NVAPI_ERROR
//! \retval ::NVAPI_INVALID_ARGUMENT  pDynamicPstatesInfo is NULL
//! \retval ::NVAPI_HANDLE_INVALIDATED
//! \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION The version of the INFO struct is not supported
//!
//! \ingroup gpupstate
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetDynamicPstatesInfoEx(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_DYNAMIC_PSTATES_INFO_EX* pDynamicPstatesInfoEx);

#define NVAPI_MAX_THERMAL_SENSORS_PER_GPU 3

typedef enum
{
	NVAPI_THERMAL_TARGET_NONE = 0,
	NVAPI_THERMAL_TARGET_GPU = 1,     //!< GPU core temperature requires NvPhysicalGpuHandle
	NVAPI_THERMAL_TARGET_MEMORY = 2,     //!< GPU memory temperature requires NvPhysicalGpuHandle
	NVAPI_THERMAL_TARGET_POWER_SUPPLY = 4,     //!< GPU power supply temperature requires NvPhysicalGpuHandle
	NVAPI_THERMAL_TARGET_BOARD = 8,     //!< GPU board ambient temperature requires NvPhysicalGpuHandle
	NVAPI_THERMAL_TARGET_VCD_BOARD = 9,     //!< Visual Computing Device Board temperature requires NvVisualComputingDeviceHandle
	NVAPI_THERMAL_TARGET_VCD_INLET = 10,    //!< Visual Computing Device Inlet temperature requires NvVisualComputingDeviceHandle
	NVAPI_THERMAL_TARGET_VCD_OUTLET = 11,    //!< Visual Computing Device Outlet temperature requires NvVisualComputingDeviceHandle

	NVAPI_THERMAL_TARGET_ALL = 15,
	NVAPI_THERMAL_TARGET_UNKNOWN = -1,
} NV_THERMAL_TARGET;

typedef enum
{
	NVAPI_THERMAL_CONTROLLER_NONE = 0,
	NVAPI_THERMAL_CONTROLLER_GPU_INTERNAL,
	NVAPI_THERMAL_CONTROLLER_ADM1032,
	NVAPI_THERMAL_CONTROLLER_MAX6649,
	NVAPI_THERMAL_CONTROLLER_MAX1617,
	NVAPI_THERMAL_CONTROLLER_LM99,
	NVAPI_THERMAL_CONTROLLER_LM89,
	NVAPI_THERMAL_CONTROLLER_LM64,
	NVAPI_THERMAL_CONTROLLER_ADT7473,
	NVAPI_THERMAL_CONTROLLER_SBMAX6649,
	NVAPI_THERMAL_CONTROLLER_VBIOSEVT,
	NVAPI_THERMAL_CONTROLLER_OS,
	NVAPI_THERMAL_CONTROLLER_UNKNOWN = -1,
} NV_THERMAL_CONTROLLER;

typedef struct
{
	NvU32   version;                //!< structure version
	NvU32   count;                  //!< number of associated thermal sensors
	struct
	{
		NV_THERMAL_CONTROLLER       controller;        //!< internal, ADM1032, MAX6649...
		NvU32                       defaultMinTemp;    //!< The min default temperature value of the thermal sensor in degree Celsius
		NvU32                       defaultMaxTemp;    //!< The max default temperature value of the thermal sensor in degree Celsius
		NvU32                       currentTemp;       //!< The current temperature value of the thermal sensor in degree Celsius
		NV_THERMAL_TARGET           target;            //!< Thermal sensor targeted @ GPU, memory, chipset, powersupply, Visual Computing Device, etc.
	} sensor[NVAPI_MAX_THERMAL_SENSORS_PER_GPU];
} NV_GPU_THERMAL_SETTINGS_V1;

typedef struct
{
	NvU32   version;                //!< structure version
	NvU32   count;                  //!< number of associated thermal sensors
	struct
	{
		NV_THERMAL_CONTROLLER       controller;         //!< internal, ADM1032, MAX6649...
		NvS32                       defaultMinTemp;     //!< Minimum default temperature value of the thermal sensor in degree Celsius
		NvS32                       defaultMaxTemp;     //!< Maximum default temperature value of the thermal sensor in degree Celsius
		NvS32                       currentTemp;        //!< Current temperature value of the thermal sensor in degree Celsius
		NV_THERMAL_TARGET           target;             //!< Thermal sensor targeted - GPU, memory, chipset, powersupply, Visual Computing Device, etc
	} sensor[NVAPI_MAX_THERMAL_SENSORS_PER_GPU];
} NV_GPU_THERMAL_SETTINGS_V2;

typedef NV_GPU_THERMAL_SETTINGS_V2  NV_GPU_THERMAL_SETTINGS;

#define NV_GPU_THERMAL_SETTINGS_VER_1   MAKE_NVAPI_VERSION(NV_GPU_THERMAL_SETTINGS_V1,1)

#define NV_GPU_THERMAL_SETTINGS_VER_2   MAKE_NVAPI_VERSION(NV_GPU_THERMAL_SETTINGS_V2,2)

#define NV_GPU_THERMAL_SETTINGS_VER     NV_GPU_THERMAL_SETTINGS_VER_2

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetThermalSettings
//
//!  This function retrieves the thermal information of all thermal sensors or specific thermal sensor associated with the selected GPU.
//!  Thermal sensors are indexed 0 to NVAPI_MAX_THERMAL_SENSORS_PER_GPU-1.
//!
//!  - To retrieve specific thermal sensor info, set the sensorIndex to the required thermal sensor index.
//!  - To retrieve info for all sensors, set sensorIndex to NVAPI_THERMAL_TARGET_ALL.
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//!
//! \since Release: 85
//!
//! \param [in]   hPhysicalGPU      GPU selection.
//! \param [in]   sensorIndex       Explicit thermal sensor index selection.
//! \param [out]  pThermalSettings  Array of thermal settings.
//!
//! \retval   NVAPI_OK                           Completed request
//! \retval   NVAPI_ERROR                        Miscellaneous error occurred.
//! \retval   NVAPI_INVALID_ARGUMENT             pThermalInfo is NULL.
//! \retval   NVAPI_HANDLE_INVALIDATED           Handle passed has been invalidated (see user guide).
//! \retval   NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE Handle passed is not a physical GPU handle.
//! \retval   NVAPI_INCOMPATIBLE_STRUCT_VERSION  The version of the INFO struct is not supported.
//! \ingroup gputhermal
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetThermalSettings(NvPhysicalGpuHandle hPhysicalGpu, NvU32 sensorIndex, NV_GPU_THERMAL_SETTINGS* pThermalSettings);

typedef struct
{
	NvU32   version;    //!< Structure version
	NvU32   reserved;   //!< These bits are reserved for future use.
	struct
	{
		NvU32 bIsPresent : 1;         //!< Set if this domain is present on this GPU
		NvU32 reserved : 31;          //!< These bits are reserved for future use.
		NvU32 frequency;            //!< Clock frequency (kHz)
	}domain[NVAPI_MAX_GPU_PUBLIC_CLOCKS];
} NV_GPU_CLOCK_FREQUENCIES_V1;

#ifndef NV_GPU_MAX_CLOCK_FREQUENCIES
#define NV_GPU_MAX_CLOCK_FREQUENCIES 3
#endif

typedef enum
{
	NV_GPU_CLOCK_FREQUENCIES_CURRENT_FREQ = 0,
	NV_GPU_CLOCK_FREQUENCIES_BASE_CLOCK = 1,
	NV_GPU_CLOCK_FREQUENCIES_BOOST_CLOCK = 2,
	NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE_NUM = NV_GPU_MAX_CLOCK_FREQUENCIES
} NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE;

typedef struct
{
	NvU32   version;        //!< Structure version
	NvU32   ClockType : 4;    //!< One of NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE. Used to specify the type of clock to be returned.
	NvU32   reserved : 20;    //!< These bits are reserved for future use. Must be set to 0.
	NvU32   reserved1 : 8;    //!< These bits are reserved.
	struct
	{
		NvU32 bIsPresent : 1;         //!< Set if this domain is present on this GPU
		NvU32 reserved : 31;          //!< These bits are reserved for future use.
		NvU32 frequency;            //!< Clock frequency (kHz)
	}domain[NVAPI_MAX_GPU_PUBLIC_CLOCKS];
} NV_GPU_CLOCK_FREQUENCIES_V2;

typedef NV_GPU_CLOCK_FREQUENCIES_V2 NV_GPU_CLOCK_FREQUENCIES;

#define NV_GPU_CLOCK_FREQUENCIES_VER_1    MAKE_NVAPI_VERSION(NV_GPU_CLOCK_FREQUENCIES_V1,1)
#define NV_GPU_CLOCK_FREQUENCIES_VER_2    MAKE_NVAPI_VERSION(NV_GPU_CLOCK_FREQUENCIES_V2,2)
#define NV_GPU_CLOCK_FREQUENCIES_VER_3    MAKE_NVAPI_VERSION(NV_GPU_CLOCK_FREQUENCIES_V2,3)
#define NV_GPU_CLOCK_FREQUENCIES_VER      NV_GPU_CLOCK_FREQUENCIES_VER_3

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetAllClockFrequencies
//
//!   This function retrieves the NV_GPU_CLOCK_FREQUENCIES structure for the specified physical GPU.
//!
//!   For each clock domain:
//!      - bIsPresent is set for each domain that is present on the GPU
//!      - frequency is the domain's clock freq in kHz
//!
//!   Each domain's info is indexed in the array.  For example:
//!   clkFreqs.domain[NVAPI_GPU_PUBLIC_CLOCK_MEMORY] holds the info for the MEMORY domain.
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//!
//! \since Release: 295
//!
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status.
//!          If there are return error codes with specific meaning for this API,
//!          they are listed below.
//! \retval  NVAPI_INVALID_ARGUMENT     pClkFreqs is NULL.
//! \ingroup gpuclock
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetAllClockFrequencies(NvPhysicalGpuHandle hPhysicalGPU, NV_GPU_CLOCK_FREQUENCIES* pClkFreqs);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetTachReading
//
//!   DESCRIPTION: This API retrieves the fan speed tachometer reading for the specified physical GPU.
//!
//!   HOW TO USE:
//!                 - NvU32 Value = 0;
//!                 - ret = NvAPI_GPU_GetTachReading(hPhysicalGpu, &Value);
//!                 - On call success:
//!                 - Value contains the tachometer reading
//!
//! SUPPORTED OS:  Windows 10 and higher
//!
//!
//! TCC_SUPPORTED
//!
//! MCDM_SUPPORTED
//!
//! \param [in]    hPhysicalGpu   GPU selection.
//! \param [out]   pValue         Pointer to a variable to get the tachometer reading
//!
//! \retval ::NVAPI_OK - completed request
//! \retval ::NVAPI_ERROR - miscellaneous error occurred
//! \retval ::NVAPI_NOT_SUPPORTED - functionality not supported
//! \retval ::NVAPI_API_NOT_INTIALIZED - nvapi not initialized
//! \retval ::NVAPI_INVALID_ARGUMENT - invalid argument passed
//! \retval ::NVAPI_HANDLE_INVALIDATED - handle passed has been invalidated (see user guide)
//! \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE - handle passed is not a physical GPU handle
//!
//! \ingroup gpucooler
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetTachReading(NvPhysicalGpuHandle hPhysicalGPU, NvU32* pValue);

typedef struct
{
	NvU32 IsPresent;
	NvU32 Percentage;
	NvU32 reserved1;
	NvU32 reserved2;
} NV_USAGE_ENTRY;

typedef struct
{
	NvU32 version;
	NvU32 reserved;
	NV_USAGE_ENTRY Entries[NVAPI_MAX_GPU_UTILIZATIONS];
} NV_USAGES;

#define NV_USAGES_VER MAKE_NVAPI_VERSION(NV_USAGES,1)

// Removed
NVAPI_INTERFACE NvAPI_GPU_GetUsages(NvPhysicalGpuHandle hPhysicalGPU, NV_USAGES* pUsage);

// Removed
NVAPI_INTERFACE NvAPI_GPU_GetMemoryInfo2(NvDisplayHandle hNvDisp, NV_DISPLAY_DRIVER_MEMORY_INFO_V2* pMemoryInfo);

#define NVAPI_MAX_FAN_COOLERS_STATUS_ITEMS 32

typedef struct
{
	NvU32 CoolerId;
	NvU32 CurrentRpm;
	NvU32 CurrentMinLevel;
	NvU32 CurrentMaxLevel;
	NvU32 CurrentLevel;
	NvU32 _reserved[8];
} NV_FAN_COOLER_STATUS_ITEM;

typedef struct
{
	NvU32 Version;
	NvU32 Count;
	NvU32 Reserved1;
	NvU32 Reserved2;
	NvU32 Reserved3;
	NvU32 Reserved4;
	NV_FAN_COOLER_STATUS_ITEM Items[NVAPI_MAX_FAN_COOLERS_STATUS_ITEMS];
} NV_FAN_COOLER_STATUS;

#define NV_FAN_COOLER_STATUS_VER MAKE_NVAPI_VERSION(NV_FAN_COOLER_STATUS,1)

// Removed
NVAPI_INTERFACE NvAPI_GPU_ClientFanCoolersGetStatus(NvPhysicalGpuHandle hPhysicalGPU, NV_FAN_COOLER_STATUS* pStatus);

#define NVAPI_THERMAL_SENSOR_RESERVED_COUNT    8
#define NVAPI_THERMAL_SENSOR_TEMPERATURE_COUNT 32

typedef struct
{
	NvU32 Version;
	NvU32 Mask;
	NvU32 reserved[NVAPI_THERMAL_SENSOR_RESERVED_COUNT];
	NvS32 Temperatures[NVAPI_THERMAL_SENSOR_TEMPERATURE_COUNT];
} NV_THERMAL_SENSORS;

#define NV_THERMAL_SENSORS_VER MAKE_NVAPI_VERSION(NV_THERMAL_SENSORS,2)

// Removed
NVAPI_INTERFACE NvAPI_GPU_GetThermalSensors(NvPhysicalGpuHandle hPhysicalGPU, NV_THERMAL_SENSORS* pSensors);

#define NVAPI_MAX_POWER_TOPOLOGIES 4

typedef enum
{
	NV_POWER_DOMAIN_GPU = 0,
	NV_POWER_DOMAIN_BOARD
} NvPowerTopologyDomain;

typedef struct
{
	NvPowerTopologyDomain Domain;
	NvU32 reserved;
	NvU32 PowerUsage;
	NvU32 reserved1;
} NV_POWER_TOPOLOGY_ENTRY;

typedef struct
{
	NvU32 version;
	NvU32 Count;
	NV_POWER_TOPOLOGY_ENTRY Entries[NVAPI_MAX_POWER_TOPOLOGIES];
} NV_POWER_TOPOLOGY;

#define NV_POWER_TOPOLOGY_VER MAKE_NVAPI_VERSION(NV_POWER_TOPOLOGY,1)

// Removed
NVAPI_INTERFACE NvAPI_GPU_ClientPowerTopologyGetStatus(NvPhysicalGpuHandle hPhysicalGPU, NV_POWER_TOPOLOGY* pPowerTopology);

#pragma pack(pop)
