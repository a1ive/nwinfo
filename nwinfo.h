// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdio.h>
#include <stdint.h>
#include <windows.h>

void ObtainPrivileges(LPCTSTR privilege);
const char* GetHumanSize(UINT64 size, const char* human_sizes[6], UINT64 base);
PVOID GetAcpi(DWORD TableId);
UINT8 AcpiChecksum(void* base, UINT size);

void nwinfo_sys(void);
void nwinfo_cpuid(void);
void nwinfo_acpi(void);
void nwinfo_network(int active);
void nwinfo_smbios(void);

#pragma pack(1)

struct acpi_table_header
{
	uint8_t signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	uint8_t oemid[6];
	uint8_t oemtable[8];
	uint32_t oemrev;
	uint8_t creator_id[4];
	uint32_t creator_rev;
};

// Microsoft Data Management table structure
struct acpi_msdm
{
	struct acpi_table_header header;
	uint32_t version;
	uint32_t reserved;
	uint32_t data_type;
	uint32_t data_reserved;
	uint32_t data_length;
	char data[29];
};

struct RawSMBIOSData
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
} SMBIOSHEADER, * PSMBIOSHEADER;

typedef struct _TYPE_0_ {
	SMBIOSHEADER	Header;
	UCHAR	Vendor;
	UCHAR	Version;
	UINT16	StartingAddrSeg;
	UCHAR	ReleaseDate;
	UCHAR	ROMSize;
	ULONG64 Characteristics;
	UCHAR	Extension[2]; // spec. 2.3
	UCHAR	MajorRelease;
	UCHAR	MinorRelease;
	UCHAR	ECFirmwareMajor;
	UCHAR	ECFirmwareMinor;
} BIOSInfo, * PBIOSInfo;

typedef struct _TYPE_1_ {
	SMBIOSHEADER	Header;
	UCHAR	Manufacturer;
	UCHAR	ProductName;
	UCHAR	Version;
	UCHAR	SN;
	UCHAR	UUID[16];
	UCHAR	WakeUpType;
	UCHAR	SKUNumber;
	UCHAR	Family;
} SystemInfo, * PSystemInfo;

typedef struct _TYPE_2_ {
	SMBIOSHEADER	Header;
	UCHAR	Manufacturer;
	UCHAR	Product;
	UCHAR	Version;
	UCHAR	SN;
	UCHAR	AssetTag;
	UCHAR	FeatureFlags;
	UCHAR	LocationInChassis;
	UINT16	ChassisHandle;
	UCHAR	Type;
	UCHAR	NumObjHandle;
	UINT16* pObjHandle;
} BoardInfo, * PBoardInfo;

typedef struct _TYPE_3_ {
	SMBIOSHEADER Header;
	UCHAR	Manufacturer;
	UCHAR	Type;
	UCHAR	Version;
	UCHAR	SN;
	UCHAR	AssetTag;
	UCHAR	BootupState;
	UCHAR	PowerSupplyState;
	UCHAR	ThermalState;
	UCHAR	SecurityStatus;
	ULONG32	OEMDefine;
	UCHAR	Height;
	UCHAR	NumPowerCord;
	UCHAR	ElementCount;
	UCHAR	ElementRecordLength;
	UCHAR	pElements;
} SystemEnclosure, * PSystemEnclosure;

typedef struct _TYPE_4_ {
	SMBIOSHEADER Header;
	UCHAR	SocketDesignation;
	UCHAR	Type;
	UCHAR	Family;
	UCHAR	Manufacturer;
	ULONG64 ID;
	UCHAR	Version;
	UCHAR	Voltage;
	UINT16	ExtClock;
	UINT16	MaxSpeed;
	UINT16	CurrentSpeed;
	UCHAR   Status;
	UCHAR   ProcessorUpgrade;
	UINT16  L1CacheHandle;
	UINT16  L2CacheHandle;
	UINT16  L3CacheHandle;
	UCHAR   Serial;
	UCHAR   AssetTag;
	UCHAR   PartNum;
	UCHAR   CoreCount;
	UCHAR   CoreEnabled;
	UCHAR   ThreadCount;
	UINT16  ProcessorChar;
	UINT16  Family2;
	UINT16  CoreCount2;
	UINT16  CoreEnabled2;
	UINT16  ThreadCount2;
} ProcessorInfo, * PProcessorInfo;

typedef struct _TYPE_5_ {
	SMBIOSHEADER Header;
	// Todo, Here

} MemCtrlInfo, * PMemCtrlInfo;

typedef struct _TYPE_6_ {
	SMBIOSHEADER Header;
	UCHAR	SocketDesignation;
	UCHAR	BankConnections;
	UCHAR	CurrentSpeed;
	// Todo, Here
} MemModuleInfo, * PMemModuleInfo;

typedef struct _TYPE_7_ {
	SMBIOSHEADER Header;
	UCHAR	SocketDesignation;
	UINT16	Configuration;
	UINT16	MaxSize;
	UINT16	InstalledSize;
	UINT16	SupportSRAMType;
	UINT16	CurrentSRAMType;
	UCHAR	Speed;
	UCHAR	ErrorCorrectionType;
	UCHAR	SystemCacheType;
	UCHAR	Associativity;
	DWORD   MaxSize2;
	DWORD   InstalledSize2;
} CacheInfo, * PCacheInfo;

typedef struct _TYPE_16_ {
	SMBIOSHEADER Header;
	UCHAR	Location;
	UCHAR	Use;
	UCHAR	ErrCorrection;
	DWORD	MaxCapacity;
	UINT16	ErrInfoHandle;
	UINT16	NumOfMDs;
	UINT64	ExtMaxCapacity;
} MemoryArray, * PMemoryArray;

typedef struct _TYPE_17_ {
	SMBIOSHEADER Header;
	UINT16	PhysicalArrayHandle;
	UINT16	ErrorInformationHandle;
	UINT16	TotalWidth;
	UINT16	DataWidth;
	UINT16	Size;
	UCHAR	FormFactor;
	UCHAR	DeviceSet;
	UCHAR	DeviceLocator;
	UCHAR	BankLocator;
	UCHAR	MemoryType;
	UINT16	TypeDetail;
	UINT16	Speed;
	UCHAR	Manufacturer;
	UCHAR	SN;
	UCHAR	AssetTag;
	UCHAR	PN;
	UCHAR	Attributes;
} MemoryDevice, * PMemoryDevice;

typedef struct _TYPE_19_ {
	SMBIOSHEADER Header;
	ULONG32	Starting;
	ULONG32	Ending;
	UINT16	Handle;
	UCHAR	PartitionWidth;
} MemoryArrayMappedAddress, * PMemoryArrayMappedAddress;

typedef struct _TYPE_22_ {
	SMBIOSHEADER Header;
	UCHAR	Location;
	UCHAR	Manufacturer;
	UCHAR	Date;
	UCHAR	SN;
	UCHAR	DeviceName;
	UCHAR   DeviceChemistry;
	UINT16  DesignCapacity;
	UINT16  DesignVoltage;

} PortableBattery, * PPortableBattery;

typedef struct _TYPE_43_ {
	SMBIOSHEADER Header;
	UCHAR   Vendor[4];
	UCHAR   MajorSpecVer;
	UCHAR   MinorSpecVer;
	DWORD   FwVer1;
	DWORD   FwVer2;
	UCHAR   Description;
	UINT64  Characteristics;
	DWORD   OEM;
} TPMDevice, * PTMPDevice;

#pragma pack()

