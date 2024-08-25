// SPDX-License-Identifier: Unlicense
#pragma once

#define VC_EXTRALEAN
#include <windows.h>

#pragma pack(1)

struct smbios_ieps
{
	UINT8 anchor[5]; /* "_DMI_" */
	UINT8 checksum;
	UINT16 table_length;
	UINT32 table_address;
	UINT16 structures;
	UINT8 revision;
};

struct smbios_eps
{
	UINT8 anchor[4]; /* "_SM_" */
	UINT8 checksum;
	UINT8 length; /* 0x1f */
	UINT8 version_major;
	UINT8 version_minor;
	UINT16 maximum_structure_size;
	UINT8 revision;
	UINT8 formatted[5];
	struct smbios_ieps intermediate;
};

struct smbios_eps3
{
	UINT8 anchor[5]; /* "_SM3_" */
	UINT8 checksum;
	UINT8 length; /* 0x18 */
	UINT8 version_major;
	UINT8 version_minor;
	UINT8 docrev;
	UINT8 revision;
	UINT8 reserved;
	UINT32 maximum_table_length;
	UINT64 table_address;
};

struct RAW_SMBIOS_DATA
{
	UINT8 Used20CallingMethod;
	UINT8 MajorVersion;
	UINT8 MinorVersion;
	UINT8 DmiRevision;
	UINT32 Length;
	UINT8 Data[];
};

typedef struct _SMBIOSHEADER_
{
	UINT8 Type;
	UINT8 Length;
	UINT16 Handle;
} SMBIOSHEADER, *PSMBIOSHEADER;

typedef struct _TYPE_0_
{
	SMBIOSHEADER Header;
	UINT8 Vendor;
	UINT8 Version;
	UINT16 StartingAddrSeg;
	UINT8 ReleaseDate;
	UINT8 ROMSize;
	UINT64 Characteristics;
	UINT8 Extension[2]; // spec. 2.3
	UINT8 MajorRelease;
	UINT8 MinorRelease;
	UINT8 ECFirmwareMajor;
	UINT8 ECFirmwareMinor;
} BIOSInfo, *PBIOSInfo;

typedef struct _TYPE_1_
{
	SMBIOSHEADER Header;
	UINT8 Manufacturer;
	UINT8 ProductName;
	UINT8 Version;
	UINT8 SN;
	UINT8 UUID[16];
	UINT8 WakeUpType;
	UINT8 SKUNumber;
	UINT8 Family;
} SystemInfo, *PSystemInfo;

typedef struct _TYPE_2_
{
	SMBIOSHEADER Header;
	UINT8 Manufacturer;
	UINT8 Product;
	UINT8 Version;
	UINT8 SN;
	UINT8 AssetTag;
	UINT8 FeatureFlags;
	UINT8 LocationInChassis;
	UINT16 ChassisHandle;
	UINT8 Type;
	UINT8 NumObjHandle;
	UINT16* pObjHandle;
} BoardInfo, *PBoardInfo;

typedef struct _TYPE_3_
{
	SMBIOSHEADER Header;
	UINT8 Manufacturer;
	UINT8 Type;
	UINT8 Version;
	UINT8 SN;
	UINT8 AssetTag;
	UINT8 BootupState;
	UINT8 PowerSupplyState;
	UINT8 ThermalState;
	UINT8 SecurityStatus;
	UINT32 OEMDefine;
	UINT8 Height;
	UINT8 NumPowerCord;
	UINT8 ElementCount;
	UINT8 ElementRecordLength;
	UINT8 pElements;
} SystemEnclosure, * PSystemEnclosure;

typedef struct _TYPE_4_
{
	SMBIOSHEADER Header;
	UINT8 SocketDesignation;
	UINT8 Type;
	UINT8 Family;
	UINT8 Manufacturer;
	UINT64 ID;
	UINT8 Version;
	UINT8 Voltage;
	UINT16 ExtClock;
	UINT16 MaxSpeed;
	UINT16 CurrentSpeed;
	UINT8 Status;
	UINT8 ProcessorUpgrade;
	UINT16 L1CacheHandle;
	UINT16 L2CacheHandle;
	UINT16 L3CacheHandle;
	UINT8 Serial;
	UINT8 AssetTag;
	UINT8 PartNum;
	UINT8 CoreCount;
	UINT8 CoreEnabled;
	UINT8 ThreadCount;
	UINT16 ProcessorChar;
	UINT16 Family2;
	UINT16 CoreCount2;
	UINT16 CoreEnabled2;
	UINT16 ThreadCount2;
} ProcessorInfo, * PProcessorInfo;

typedef struct _TYPE_5_
{
	SMBIOSHEADER Header;
	UINT8 ErrDetecting;
	UINT8 ErrCorrection;
	UINT8 SupportedInterleave;
	UINT8 CurrentInterleave;
	UINT8 MaxMemModuleSize;
	UINT16 SupportedSpeeds;
	UINT16 SupportedMemTypes;
	UINT8 MemModuleVoltage;
	UINT8 NumOfSlots;
} MemCtrlInfo, * PMemCtrlInfo;

typedef struct _TYPE_6_
{
	SMBIOSHEADER Header;
	UINT8 SocketDesignation;
	UINT8 BankConnections;
	UINT8 CurrentSpeed;
	UINT16 CurrentMemType;
	UINT8 InstalledSize;
	UINT8 EnabledSize;
	UINT8 ErrStatus;
} MemModuleInfo, * PMemModuleInfo;

typedef struct _TYPE_7_
{
	SMBIOSHEADER Header;
	UINT8 SocketDesignation;
	UINT16 Configuration;
	UINT16 MaxSize;
	UINT16 InstalledSize;
	UINT16 SupportSRAMType;
	UINT16 CurrentSRAMType;
	UINT8 Speed;
	UINT8 ErrorCorrectionType;
	UINT8 SystemCacheType;
	UINT8 Associativity;
	UINT32 MaxSize2;
	UINT32 InstalledSize2;
} CacheInfo, * PCacheInfo;

typedef struct _TYPE_8_
{
	SMBIOSHEADER Header;
	UINT8 IntDesignator;
	UINT8 IntConnectorType;
	UINT8 ExtDesignator;
	UINT8 ExtConnectorType;
	UINT8 PortType;
} PortConnectInfo, * PPortConnectInfo;

typedef struct _TYPE_9_
{
	SMBIOSHEADER Header;
	UINT8 SlotDesignation;
	UINT8 SlotType;
	UINT8 SlotDataBusWidth;
	UINT8 CurrentUsage;
	UINT8 SlotLength;
	UINT16 SlotID;
	UINT8 SlotCharacteristics1;
	UINT8 SlotCharacteristics2;
} SystemSlots, * PSystemSlots;

typedef struct _TYPE_10_
{
	SMBIOSHEADER Header;
	struct _TYPE_10_DEVICE_INFO
	{
		UINT8 DeviceType;
		UINT8 Description;
	} DeviceInfo[];
} OnBoardDevicesInfo, * POnBoardDevicesInfo;

typedef struct _TYPE_11_12_
{
	SMBIOSHEADER Header;
	UINT8 Count;
} OEMString, * POEMString;

typedef struct _TYPE_13_
{
	SMBIOSHEADER Header;
	UINT8 InstallableLang;
	UINT8 Flags;
	UINT8 Reserved[15];
	UINT8 CurrentLang;
} BIOSLangInfo, * PBIOSLangInfo;

typedef struct _TYPE_14_
{
	SMBIOSHEADER Header;
	UINT8 GroupName;
	struct _TYPE_14_ITEM
	{
		UINT8 ItemType;
		UINT16 ItemHandle;
	} GAItem[0];
} GroupAssoc, * PGroupAssoc;

typedef struct _TYPE_15_
{
	SMBIOSHEADER Header;
	UINT16 LogAreaLength;
	UINT16 LogHdrStartOffset;
	UINT16 LogDataStartOffset;
	UINT8 AccessMethod;
	UINT8 LogStatus;
	UINT32 LogChangeToken;
	UINT32 AccessMethodAddr;
} SystemEventLog, * PSystemEventLog;

typedef struct _TYPE_16_
{
	SMBIOSHEADER Header;
	UINT8 Location;
	UINT8 Use;
	UINT8 ErrCorrection;
	UINT32 MaxCapacity;
	UINT16 ErrInfoHandle;
	UINT16 NumOfMDs;
	UINT64 ExtMaxCapacity;
} MemoryArray, * PMemoryArray;

typedef struct _TYPE_17_
{
	SMBIOSHEADER Header;
	UINT16 PhysicalArrayHandle;
	UINT16 ErrorInformationHandle;
	UINT16 TotalWidth;
	UINT16 DataWidth;
	UINT16 Size;
	UINT8 FormFactor;
	UINT8 DeviceSet;
	UINT8 DeviceLocator;
	UINT8 BankLocator;
	UINT8 MemoryType;
	UINT16 TypeDetail;
	UINT16 Speed;
	UINT8 Manufacturer;
	UINT8 SN;
	UINT8 AssetTag;
	UINT8 PN;
	UINT8 Attributes;
	UINT32 ExtendedSize;
	UINT16 ConfiguredMemSpeed;
	UINT16 MinVoltage;
	UINT16 MaxVoltage;
	UINT16 ConfiguredVoltage;
	UINT8 MemoryTechnology;
	UINT16 MemoryOpModeCapability;
	UINT8 FirmwareVersion;
	UINT16 ModuleVID;
	UINT16 ModulePID;
	UINT16 MemorySubsysControllerVID;
	UINT16 MemorySubsysControllerPID;
	UINT64 NonVolatileSize;
	UINT64 VolatileSize;
	UINT64 CacheSize;
	UINT64 LogicalSize;
	UINT32 ExtendedSpeed;
	UINT16 ExtendedConfiguredMemSpeed;
} MemoryDevice, * PMemoryDevice;

typedef struct _TYPE_18_
{
	SMBIOSHEADER Header;
	UINT8 ErrType;
	UINT8 ErrGranularity;
	UINT8 ErrOperation;
	UINT32 VendorSyndrome;
	UINT32 MemArrayErrAddr;
	UINT32 DevErrAddr;
	UINT32 ErrResolution;
} MemoryErrInfo, * PMemoryErrInfo;

typedef struct _TYPE_19_
{
	SMBIOSHEADER Header;
	UINT32 StartAddr;
	UINT32 EndAddr;
	UINT16 Handle;
	UINT8 PartitionWidth;
	UINT64 ExtStartAddr;
	UINT64 ExtEndAddr;
} MemoryArrayMappedAddress, * PMemoryArrayMappedAddress;

typedef struct _TYPE_20_
{
	SMBIOSHEADER Header;
	UINT32 StartAddr;
	UINT32 EndAddr;
	UINT16 MDHandle;
	UINT16 MAMAHandle;
	UINT8 PartitionRowPos;
	UINT8 InterleavePos;
	UINT8 InterleavedDataDepth;
	UINT64 ExtStartAddr;
	UINT64 ExtEndAddr;
} MemoryDeviceMappedAddress, * PMemoryDeviceMappedAddress;

typedef struct _TYPE_21_
{
	SMBIOSHEADER Header;
	UINT8 Type;
	UINT8 Interface;
	UINT8 NumOfButtons;
} BuiltinPointing, * PBuiltinPointing;

typedef struct _TYPE_22_
{
	SMBIOSHEADER Header;
	UINT8 Location;
	UINT8 Manufacturer;
	UINT8 Date;
	UINT8 SN;
	UINT8 DeviceName;
	UINT8 DeviceChemistry;
	UINT16 DesignCapacity;
	UINT16 DesignVoltage;
	UINT8 SBDSVer;
	UINT8 MaxErr;
	UINT16 SBDSSerial;
	UINT16 SBDSManufactureDate;
	UINT8 SBDSChemistry;
	UINT8 DesignCapacityMultiplier;
	UINT32 OEMInfo;
} PortableBattery, * PPortableBattery;

typedef struct _TYPE_23_
{
	SMBIOSHEADER Header;
	UINT8 Capabilities;
	UINT16 ResetCount;
	UINT16 ResetLimit;
	UINT16 TimerInterval;
	UINT16 Timeout;
} SysReset, * PSysReset;

typedef struct _TYPE_24_
{
	SMBIOSHEADER Header;
	UINT8 Settings;
} HwSecurity, * PHwSecurity;

typedef struct _TYPE_25_
{
	SMBIOSHEADER Header;
	UINT8 NextPwrOnMonth;
	UINT8 NextPwrOnDay;
	UINT8 NextPwrOnHour;
	UINT8 NextPwrOnMinute;
	UINT8 NextPwrOnSecond;
} SysPowerCtrl, * PSysPowerCtrl;

typedef struct _TYPE_26_28_29_
{
	SMBIOSHEADER Header;
	UINT8 Description;
	UINT8 LocationStatus;
	UINT16 MaxValue;
	UINT16 MinValue;
	UINT16 Resolution;
	UINT16 Tolerance;
	UINT16 Accuracy;
	UINT32 OEMDefined;
	UINT16 NominalValue;
} VoltageProbe, * PVoltageProbe;

typedef struct _TYPE_27_
{
	SMBIOSHEADER Header;
	UINT16 TempProbeHandle;
	UINT8 DeviceTypeStatus;
	UINT8 CoolingUnitGroup;
	UINT32 OEMDefined;
	UINT16 NominalSpeed;
	UINT8 Description;
} CoolingDevice, * PCoolingDevice;

typedef struct _TYPE_26_28_29_ TempProbe, * PTempProbe;

typedef struct _TYPE_26_28_29_ ElecCurrentProbe, * PElecCurrentProbe;

typedef struct _TYPE_30_
{
	SMBIOSHEADER Header;
	UINT8 Manufacturer;
	UINT8 Connections;
} OutOfBandRemoteAccess, * POutOfBandRemoteAccess;

typedef struct _TYPE_31_
{
	SMBIOSHEADER Header;
} BISEntryPoint, * PBISEntryPoint;

typedef struct _TYPE_32_
{
	SMBIOSHEADER Header;
	UINT8 Reserved[6];
	UINT8 BootStatus[];
} SysBootInfo, * PSysBootInfo;

typedef struct _TYPE_33_
{
	SMBIOSHEADER Header;
	UINT8 ErrType;
	UINT8 ErrGranularity;
	UINT8 ErrOperation;
	UINT32 VendorSyndrome;
	UINT64 MemArrayErrAddr;
	UINT64 DevErrAddr;
	UINT32 ErrResolution;
} MemoryErrInfo64, * PMemoryErrInfo64;

typedef struct _TYPE_34_
{
	SMBIOSHEADER Header;
	UINT8 Description;
	UINT8 Type;
	UINT32 Address;
	UINT8 AddressType;
} ManagementDevice, * PManagementDevice;

typedef struct _TYPE_35_
{
	SMBIOSHEADER Header;
	UINT8 Description;
	UINT16 DeviceHandle;
	UINT16 ComponentHandle;
	UINT16 ThresholdHandle;
} ManagementDeviceComponent, * PManagementDeviceComponent;

typedef struct _TYPE_40_
{
	SMBIOSHEADER Header;
	UINT8 NumofEntries;
} AdditionalInformation, * PAdditionalInformation;

typedef struct _TYPE_41_
{
	SMBIOSHEADER Header;
	UINT8 RefDesignation;
	UINT8 DeviceType;
	UINT8 DeviceTypeInstance;
	UINT16 SegmentGroupNum;
	UINT8 BusNum;
	UINT8 DevFunNum;
} OnBoardDevicesExtInfo, * POnBoardDevicesExtInfo;

typedef struct _TYPE_43_
{
	SMBIOSHEADER Header;
	UINT8 Vendor[4];
	UINT8 MajorSpecVer;
	UINT8 MinorSpecVer;
	UINT32 FwVer1;
	UINT32 FwVer2;
	UINT8 Description;
	UINT64 Characteristics;
	UINT32 OEM;
} TPMDevice, * PTPMDevice;

typedef struct _TYPE_44_
{
	SMBIOSHEADER Header;
	UINT16 RefHandle;
	UINT8 BlockLength;
	UINT8 ProcessorType;
	UINT8 ProcessorSpecificData[0];
} ProcessorAdditionalInfo, * PProcessorAdditionalInfo;

typedef struct _TYPE_45_
{
	SMBIOSHEADER Header;
	UINT8 ComponentName;
	UINT8 Version;
	UINT8 VersionFormat;
	UINT8 ID;
	UINT8 IDFormat;
	UINT8 ReleaseDate;
	UINT8 Manufacturer;
	UINT8 LowestSupportedVersion;
	UINT64 ImageSize;
	UINT16 Characteristics;
	UINT8 State;
	UINT8 NumOfAssociatedComponents;
	UINT16 AssociatedComponentHandle[0];
} FirmwareInventoryInfo, * PFirmwareInventoryInfo;

typedef struct _TYPE_46_
{
	SMBIOSHEADER Header;
	UINT16 ID;
	UINT8 Value;
	UINT16 ParentHandle;
} StringProperty, * PStringProperty;

#pragma pack()
