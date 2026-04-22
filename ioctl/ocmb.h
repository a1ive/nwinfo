// SPDX-License-Identifier: Unlicense
#pragma once

#define MSR_OC_MAILBOX 0x150

#define MAILBOX_OC_CMD_GET_OC_CAPABILITIES              0x01
#define MAILBOX_OC_CMD_GET_PER_CORE_RATIO_LIMIT         0x02
#define MAILBOX_OC_CMD_GET_DDR_CAPABILITIES             0x03
#define MAILBOX_OC_CMD_GET_VR_TOPOLOGY                  0x04
#define MAILBOX_OC_CMD_GET_FUSED_P0_RATIO_VOLTAGE       0x07
#define MAILBOX_OC_CMD_GET_VOLTAGE_FREQUENCY            0x10
#define MAILBOX_OC_CMD_SET_VOLTAGE_FREQUENCY            0x11
#define MAILBOX_OC_CMD_GET_SVID                         0x12 // ?
#define MAILBOX_OC_CMD_SET_SVID                         0x13 // ?
#define MAILBOX_OC_CMD_GET_MISC_GLOBAL_CONFIG           0x14
#define MAILBOX_OC_CMD_SET_MISC_GLOBAL_CONFIG           0x15
#define MAILBOX_OC_CMD_GET_ICCMAX                       0x16
#define MAILBOX_OC_CMD_SET_ICCMAX                       0x17
#define MAILBOX_OC_CMD_GET_MISC_TURBO_CONTROL           0x18
#define MAILBOX_OC_CMD_SET_MISC_TURBO_CONTROL           0x19
#define MAILBOX_OC_CMD_GET_AVX_RATIO_OFFSET             0x1A
#define MAILBOX_OC_CMD_SET_AVX_RATIO_OFFSET             0x1B
#define MAILBOX_OC_CMD_GET_AVX_VOLTAGE_GUARDBAND        0x20
#define MAILBOX_OC_CMD_SET_AVX_VOLTAGE_GUARDBAND        0x21
#define MAILBOX_OC_CMD_GET_OC_TVB_CONFIG                0x24
#define MAILBOX_OC_CMD_SET_OC_TVB_CONFIG                0x25

#define MAILBOX_OC_COMPLETION_CODE_SUCCESS              0x00
#define MAILBOX_OC_COMPLETION_CODE_OC_LOCKED            0x01
#define MAILBOX_OC_COMPLETION_CODE_INVALID_DOMAIN       0x02
#define MAILBOX_OC_COMPLETION_CODE_MAX_RATIO_EXCEEDED   0x03
#define MAILBOX_OC_COMPLETION_CODE_MAX_VOLTAGE_EXCEEDED 0x04
#define MAILBOX_OC_COMPLETION_CODE_OC_NOT_SUPPORTED     0x05
#define MAILBOX_OC_COMPLETION_CODE_WRITE_FAILED         0x06
#define MAILBOX_OC_COMPLETION_CODE_READ_FAILED          0x07

#define MAILBOX_OC_DOMAIN_ID_DDR                0x00
#define MAILBOX_OC_DOMAIN_ID_IA_CORE            0x00
#define MAILBOX_OC_DOMAIN_ID_GT                 0x01
#define MAILBOX_OC_DOMAIN_ID_RING               0x02
#define MAILBOX_OC_DOMAIN_ID_RESERVED           0x03
#define MAILBOX_OC_DOMAIN_ID_SYSTEM_AGENT       0x04
#define MAILBOX_OC_DOMAIN_ID_L2_ATOM            0x05
#define MAILBOX_OC_DOMAIN_ID_MEMORY_CONTROLLER  0x06

typedef union _OC_MAILBOX_INTERFACE
{
	UINT32 InterfaceData;          ///< All bit fields as a 32-bit value.
	///
	/// Individual bit fields.
	///
	struct
	{
		UINT8 CommandCompletion : 8; ///< Command ID and completion code
		UINT8 Param1 : 8; ///< Parameter 1, generally used to specify the CPU Domain ID
		UINT8 Param2 : 8; ///< Parameter 2, only current usage is as a core index for ratio limits message
		UINT8 Reserved : 7; ///< Reserved for future use
		UINT8 RunBusy : 1; ///< Run/Busy bit. This bit is set by BIOS to indicate the mailbox buffer is ready. pcode will clear this bit after the message is consumed.
	} Fields;
} OC_MAILBOX_INTERFACE;

typedef struct _OC_MAILBOX_FULL
{
	UINT32               Data;      ///< OC Mailbox read/write data
	OC_MAILBOX_INTERFACE Interface; ///< OC mailbox interface
} OC_MAILBOX_FULL;

// MAILBOX_OC_CMD_GET_OC_CAPABILITIES
typedef union
{
	UINT32 Data;
	struct
	{
		// [07:00]
		UINT32 MaxOcRatioLimit : 8;           ///< Max overclocking ratio limit of specified CPU domain
		// [08]
		UINT32 RatioOcSupported : 1;          ///< Ratio based overclocking support; 0: Not Supported, 1: Supported
		// [09]
		UINT32 VoltageOverridesSupported : 1; ///< Voltage override support; 0: Not Supported, 1: Supported
		// [10]
		UINT32 VoltageOffsetSupported : 1;    ///< Voltage offset support; 0: Not Supported, 1: Supported
		// [31:11]
		UINT32 Reserved : 21;
	} Fields;
} OCMB_CAPABILITIES;

// MAILBOX_OC_CMD_GET_PER_CORE_RATIO_LIMIT
typedef union
{
	UINT32 Data;
	struct
	{
		UINT32 CoreRatio : 8; ///< [07:0]
		UINT32 Reserved : 24; ///< [31:8]
	} Fields;
} OCMB_PER_CORE_RATIO_LIMIT;

// MAILBOX_OC_CMD_GET_VR_TOPOLOGY
typedef union
{
	UINT32 Data;
	struct
	{
		UINT32  Reserved1 : 8;   /// [07:00] Reserved
		UINT32  IaVrAddress : 4; /// [11:08] IA VR Address
		UINT32  IaVrType : 1;    /// [12]    IA VR Type
		UINT32  GtVrAddress : 4; /// [16:13] GT VR Address
		UINT32  GtVrType : 1;    /// [17]    GT VR Type
		UINT32  Reserved2 : 14;  /// [31:18] Reserved
	} Fields;
} OCMB_VR_TOPOLOGY;

// MAILBOX_OC_CMD_GET_FUSED_P0_RATIO_VOLTAGE
typedef union
{
	UINT32 Data;
	struct
	{
		UINT32 FusedP0Ratio : 8;    ///< [07:00]
		UINT32 FusedP0Voltage : 12; ///< [19:08]
		UINT32 Reserved : 12;       ///< [31:20]
	} Fields;
} OCMB_FUSED_P0_RATIO_VOLTAGE;

// MAILBOX_OC_CMD_GET_VOLTAGE_FREQUENCY
typedef union
{
	UINT32 Data;
	struct
	{
		// [07:00]
		UINT32 MaxOcRatio : 8;        ///< Max overclocking ratio limit for given CPU domain id
		// [19:08]
		UINT32 VoltageTargetU12 : 12; ///< Voltage target represented in unsigned fixed point U12.2.10V format
		// [20]
		UINT32 TargetMode : 1;        ///< Voltage mode selection; 0: Adaptive, 1: Override
		// [31:21]
		UINT32 VoltageOffsetS11 : 11; ///< Voltage offset represented in signed fixed point S11.0.10V format
	} Fields;
} OCMB_VOLTAGE_FREQUENCY;

// MAILBOX_OC_CMD_GET_SVID
typedef union
{
	UINT32 Data;
	struct
	{
		// [11:00]
		UINT32 Svid : 12;              ///< Voltage divided by 1024, 0 = Dynamic
		// [30:12]
		UINT32 Reserved : 19;          ///< Reserved
		// [31]
		UINT32 Lock : 1;               ///< Disable SVID comm's
	} Fields;
} OCMB_SVID;

// MAILBOX_OC_CMD_GET_MISC_GLOBAL_CONFIG
typedef union
{
	UINT32 Data;
	struct
	{
		// [00]
		UINT32 IgnoreFIVRFaults : 1;      /// ignore = 1
		// [01]
		UINT32 DisableFIVREfficiency : 1; /// disable = 1
		// [02]
		UINT32 BclkAdaptive : 1;          /// enable = 1
		// [03]
		UINT32 PerCoreVoltage : 1;        /// per-core = 1
		// [31:04]
		UINT32 Reserved : 28;             /// Reserved
	} Fields;
} OCMB_MISC_GLOBAL_CONFIG;

// MAILBOX_OC_CMD_GET_ICCMAX
typedef union
{
	UINT32 Data;
	struct
	{
		// [10:00]
		UINT32  IccMaxValue : 11;        /// Sets/Gets the maximum desired current
		// [30:11]
		UINT32  Reserved : 20;           /// Reserved
		// [31]
		UINT32  UnlimitedIccMaxMode : 1; /// Unlimited ICCMAX mode
	} Fields;
} OCMB_ICCMAX;

