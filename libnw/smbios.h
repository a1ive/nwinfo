// SPDX-License-Identifier: Unlicense
#pragma once

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
	BYTE Used20CallingMethod;
	BYTE MajorVersion;
	BYTE MinorVersion;
	BYTE DmiRevision;
	DWORD Length;
	BYTE Data[];
};

typedef struct _SMBIOSHEADER_
{
	BYTE Type;
	BYTE Length;
	WORD Handle;
} SMBIOSHEADER, *PSMBIOSHEADER;

typedef struct _TYPE_0_
{
	SMBIOSHEADER Header;
	UCHAR Vendor;
	UCHAR Version;
	UINT16 StartingAddrSeg;
	UCHAR ReleaseDate;
	UCHAR ROMSize;
	UINT64 Characteristics;
	UCHAR Extension[2]; // spec. 2.3
	UCHAR MajorRelease;
	UCHAR MinorRelease;
	UCHAR ECFirmwareMajor;
	UCHAR ECFirmwareMinor;
} BIOSInfo, *PBIOSInfo;

typedef struct _TYPE_1_
{
	SMBIOSHEADER Header;
	UCHAR Manufacturer;
	UCHAR ProductName;
	UCHAR Version;
	UCHAR SN;
	UCHAR UUID[16];
	UCHAR WakeUpType;
	UCHAR SKUNumber;
	UCHAR Family;
} SystemInfo, *PSystemInfo;

typedef struct _TYPE_2_
{
	SMBIOSHEADER Header;
	UCHAR Manufacturer;
	UCHAR Product;
	UCHAR Version;
	UCHAR SN;
	UCHAR AssetTag;
	UCHAR FeatureFlags;
	UCHAR LocationInChassis;
	UINT16 ChassisHandle;
	UCHAR Type;
	UCHAR NumObjHandle;
	UINT16* pObjHandle;
} BoardInfo, *PBoardInfo;

typedef struct _TYPE_3_
{
	SMBIOSHEADER Header;
	UCHAR Manufacturer;
	UCHAR Type;
	UCHAR Version;
	UCHAR SN;
	UCHAR AssetTag;
	UCHAR BootupState;
	UCHAR PowerSupplyState;
	UCHAR ThermalState;
	UCHAR SecurityStatus;
	ULONG32 OEMDefine;
	UCHAR Height;
	UCHAR NumPowerCord;
	UCHAR ElementCount;
	UCHAR ElementRecordLength;
	UCHAR pElements;
} SystemEnclosure, * PSystemEnclosure;

typedef struct _TYPE_4_
{
	SMBIOSHEADER Header;
	UCHAR SocketDesignation;
	UCHAR Type;
	UCHAR Family;
	UCHAR Manufacturer;
	ULONG64 ID;
	UCHAR Version;
	UCHAR Voltage;
	UINT16 ExtClock;
	UINT16 MaxSpeed;
	UINT16 CurrentSpeed;
	UCHAR Status;
	UCHAR ProcessorUpgrade;
	UINT16 L1CacheHandle;
	UINT16 L2CacheHandle;
	UINT16 L3CacheHandle;
	UCHAR Serial;
	UCHAR AssetTag;
	UCHAR PartNum;
	UCHAR CoreCount;
	UCHAR CoreEnabled;
	UCHAR ThreadCount;
	UINT16 ProcessorChar;
	UINT16 Family2;
	UINT16 CoreCount2;
	UINT16 CoreEnabled2;
	UINT16 ThreadCount2;
} ProcessorInfo, * PProcessorInfo;

typedef struct _TYPE_5_
{
	SMBIOSHEADER Header;
	UCHAR ErrDetecting;
	UCHAR ErrCorrection;
	UCHAR SupportedInterleave;
	UCHAR CurrentInterleave;
	UCHAR MaxMemModuleSize;
	UINT16 SupportedSpeeds;
	UINT16 SupportedMemTypes;
	UCHAR MemModuleVoltage;
	UCHAR NumOfSlots;
} MemCtrlInfo, * PMemCtrlInfo;

typedef struct _TYPE_6_
{
	SMBIOSHEADER Header;
	UCHAR SocketDesignation;
	UCHAR BankConnections;
	UCHAR CurrentSpeed;
	UINT16 CurrentMemType;
	UCHAR InstalledSize;
	UCHAR EnabledSize;
	UCHAR ErrStatus;
} MemModuleInfo, * PMemModuleInfo;

typedef struct _TYPE_7_
{
	SMBIOSHEADER Header;
	UCHAR SocketDesignation;
	UINT16 Configuration;
	UINT16 MaxSize;
	UINT16 InstalledSize;
	UINT16 SupportSRAMType;
	UINT16 CurrentSRAMType;
	UCHAR Speed;
	UCHAR ErrorCorrectionType;
	UCHAR SystemCacheType;
	UCHAR Associativity;
	DWORD MaxSize2;
	DWORD InstalledSize2;
} CacheInfo, * PCacheInfo;

typedef struct _TYPE_8_
{
	SMBIOSHEADER Header;
	UCHAR IntDesignator;
	UCHAR IntConnectorType;
	UCHAR ExtDesignator;
	UCHAR ExtConnectorType;
	UCHAR PortType;
} PortConnectInfo, * PPortConnectInfo;

typedef struct _TYPE_9_
{
	SMBIOSHEADER Header;
	UCHAR SlotDesignation;
	UCHAR SlotType;
	UCHAR SlotDataBusWidth;
	UCHAR CurrentUsage;
	UCHAR SlotLength;
	UINT16 SlotID;
	UCHAR SlotCharacteristics1;
	UCHAR SlotCharacteristics2;
} SystemSlots, * PSystemSlots;

typedef struct _TYPE_10_
{
	SMBIOSHEADER Header;
	struct _TYPE_10_DEVICE_INFO
	{
		UCHAR DeviceType;
		UCHAR Description;
	} DeviceInfo[];
} OnBoardDevicesInfo, * POnBoardDevicesInfo;

typedef struct _TYPE_11_12_
{
	SMBIOSHEADER Header;
	UCHAR Count;
} OEMString, * POEMString;

typedef struct _TYPE_13_
{
	SMBIOSHEADER Header;
	UCHAR InstallableLang;
	UCHAR Flags;
	UCHAR Reserved[15];
	UCHAR CurrentLang;
} BIOSLangInfo, * PBIOSLangInfo;

typedef struct _TYPE_14_
{
	SMBIOSHEADER Header;
	UCHAR GroupName;
	struct _TYPE_14_ITEM
	{
		UCHAR ItemType;
		WORD ItemHandle;
	} GAItem[0];
} GroupAssoc, * PGroupAssoc;

typedef struct _TYPE_15_
{
	SMBIOSHEADER Header;
	WORD LogAreaLength;
	WORD LogHdrStartOffset;
	WORD LogDataStartOffset;
	UCHAR AccessMethod;
	UCHAR LogStatus;
	DWORD LogChangeToken;
	DWORD AccessMethodAddr;
} SystemEventLog, * PSystemEventLog;

typedef struct _TYPE_16_
{
	SMBIOSHEADER Header;
	UCHAR Location;
	UCHAR Use;
	UCHAR ErrCorrection;
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
	UCHAR FormFactor;
	UCHAR DeviceSet;
	UCHAR DeviceLocator;
	UCHAR BankLocator;
	UCHAR MemoryType;
	UINT16 TypeDetail;
	UINT16 Speed;
	UCHAR Manufacturer;
	UCHAR SN;
	UCHAR AssetTag;
	UCHAR PN;
	UCHAR Attributes;
} MemoryDevice, * PMemoryDevice;

typedef struct _TYPE_19_
{
	SMBIOSHEADER Header;
	UINT32 StartAddr;
	UINT32 EndAddr;
	UINT16 Handle;
	UCHAR PartitionWidth;
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
	UCHAR PartitionRowPos;
	UCHAR InterleavePos;
	UCHAR InterleavedDataDepth;
	UINT64 ExtStartAddr;
	UINT64 ExtEndAddr;
} MemoryDeviceMappedAddress, * PMemoryDeviceMappedAddress;

typedef struct _TYPE_22_
{
	SMBIOSHEADER Header;
	UCHAR Location;
	UCHAR Manufacturer;
	UCHAR Date;
	UCHAR SN;
	UCHAR DeviceName;
	UCHAR DeviceChemistry;
	UINT16 DesignCapacity;
	UINT16 DesignVoltage;

} PortableBattery, * PPortableBattery;

typedef struct _TYPE_32_
{
	SMBIOSHEADER Header;
	UCHAR Reserved[6];
	UCHAR BootStatus[];
} SysBootInfo, * PSysBootInfo;

typedef struct _TYPE_43_
{
	SMBIOSHEADER Header;
	UCHAR Vendor[4];
	UCHAR MajorSpecVer;
	UCHAR MinorSpecVer;
	DWORD FwVer1;
	DWORD FwVer2;
	UCHAR Description;
	UINT64 Characteristics;
	DWORD OEM;
} TPMDevice, * PTPMDevice;

#pragma pack()
