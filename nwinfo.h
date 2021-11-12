// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdio.h>
#include <stdint.h>
#include <windows.h>

void ObtainPrivileges(LPCTSTR privilege);
const char* GetHumanSize(UINT64 size, const char* human_sizes[6], UINT64 base);
PVOID GetAcpi(DWORD TableId);
UINT8 AcpiChecksum(void* base, UINT size);
void TrimString(CHAR* String);
int GetRegDwordValue(HKEY Key, LPCSTR SubKey, LPCSTR ValueName, DWORD* pValue);
extern CHAR* IDS;
extern DWORD IDS_SIZE;
void FindId(CONST CHAR* v, CONST CHAR* d, CONST CHAR* s, int usb);

void nwinfo_sys(void);
void nwinfo_cpuid(void);
void nwinfo_acpi(void);
void nwinfo_network(int active);
void nwinfo_smbios(UINT8 Type);
void nwinfo_disk(void);
void nwinfo_display(void);
void nwinfo_pci(const GUID* Guid, const CHAR *PciClass);
void nwinfo_usb(const GUID* Guid);

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

struct mbr_entry
{
	/* If active, 0x80, otherwise, 0x00.  */
	uint8_t flag;
	/* The head of the start.  */
	uint8_t start_head;
	/* (S | ((C >> 2) & 0xC0)) where S is the sector of the start and C
	   is the cylinder of the start. Note that S is counted from one.  */
	uint8_t start_sector;
	/* (C & 0xFF) where C is the cylinder of the start.  */
	uint8_t start_cylinder;
	/* The partition type.  */
	uint8_t type;
	/* The end versions of start_head, start_sector and start_cylinder,
	   respectively.  */
	uint8_t end_head;
	uint8_t end_sector;
	uint8_t end_cylinder;
	/* The start sector. Note that this is counted from zero.  */
	uint32_t start;
	/* The length in sector units.  */
	uint32_t length;
};

struct mbr_header
{
	char dummy1[11];/* normally there is a short JMP instuction(opcode is 0xEB) */
	uint16_t bytes_per_sector;/* seems always to be 512, so we just use 512 */
	uint8_t sectors_per_cluster;/* non-zero, the power of 2, i.e., 2^n */
	uint16_t reserved_sectors;/* FAT=non-zero, NTFS=0? */
	uint8_t number_of_fats;/* NTFS=0; FAT=1 or 2  */
	uint16_t root_dir_entries;/* FAT32=0, NTFS=0, FAT12/16=non-zero */
	uint16_t total_sectors_short;/* FAT32=0, NTFS=0, FAT12/16=any */
	uint8_t media_descriptor;/* range from 0xf0 to 0xff */
	uint16_t sectors_per_fat;/* FAT32=0, NTFS=0, FAT12/16=non-zero */
	uint16_t sectors_per_track;/* range from 1 to 63 */
	uint16_t total_heads;/* range from 1 to 256 */
	uint32_t hidden_sectors;/* any value */
	uint32_t total_sectors_long;/* FAT32=non-zero, NTFS=0, FAT12/16=any */
	uint32_t sectors_per_fat32;/* FAT32=non-zero, NTFS=any, FAT12/16=any */
	uint64_t total_sectors_long_long;/* NTFS=non-zero, FAT12/16/32=any */
	char dummy2[392];
	uint8_t unique_signature[4];
	uint8_t unknown[2];

	/* Four partition entries.  */
	struct mbr_entry entries[4];

	/* The signature 0xaa55.  */
	uint16_t signature;
};

struct gpt_header
{
	uint8_t magic[8];
	uint32_t version;
	uint32_t headersize;
	uint32_t crc32;
	uint32_t unused1;
	uint64_t header_lba;
	uint64_t alternate_lba;
	uint64_t start;
	uint64_t end;
	uint8_t guid[16];
	uint64_t partitions;
	uint32_t maxpart;
	uint32_t partentry_size;
	uint32_t partentry_crc32;
};

typedef struct PHY_DRIVE_INFO
{
	//int Id;
	int PhyDrive;
	int PartStyle;//0:UNKNOWN 1:MBR 2:GPT
	UINT64 SizeInBytes;
	BYTE DeviceType;
	BOOL RemovableMedia;
	CHAR VendorId[128];
	CHAR ProductId[128];
	CHAR ProductRev[128];
	CHAR SerialNumber[128];
	STORAGE_BUS_TYPE BusType;
	// MBR
	UCHAR MbrSignature[4];
	// GPT
	UCHAR GptGuid[16];

	CHAR DriveLetters[26];
}PHY_DRIVE_INFO;

#pragma pack()

