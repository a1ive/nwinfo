// SPDX-License-Identifier: Unlicense
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define ACPI_SIG(a, b, c, d) ((UINT32)((a) | ((b) << 8) | ((c) << 16) | ((d) << 24)))

#pragma pack(1)

typedef struct DESC_HEADER
{
	CHAR Signature[4];
	UINT32 Length;
	UINT8 Revision;
	UINT8 Checksum;
	CHAR OemId[6];
	CHAR OemTableId[8];
	UINT32 OemRevision;
	CHAR CreatorId[4];
	UINT32 CreatorRevision;
} DESC_HEADER;

typedef struct GEN_ADDR
{
	UINT8 AddressSpaceId;
	UINT8 RegisterBitWidth;
	UINT8 RegisterBitOffset;
	UINT8 AccessSize;
	UINT64 Address;
} GEN_ADDR;

// Used in EINJ and ERST
typedef struct ACTION_ENTRY
{
	UINT8 Action;
	UINT8 Instruction;
	UINT8 Flags;
	UINT8 Reserved;
	GEN_ADDR RegisterRegion;
	UINT64 Value;
	UINT64 Mask;
} ACTION_ENTRY;

#define RSDP_SIGNATURE "RSD PTR "
#define RSDP_SIGNATURE_SIZE 8

// RSDP: Root System Description Pointer (RSD PTR )
typedef struct ACPI_RSDP_V1
{
	UINT8 Signature[RSDP_SIGNATURE_SIZE];
	UINT8 Checksum;
	UINT8 OemId[6];
	UINT8 Revision;
	UINT32 RsdtAddr;
} ACPI_RSDP_V1;

typedef struct ACPI_RSDP_V2
{
	struct ACPI_RSDP_V1 RsdpV1;
	UINT32 Length;
	UINT64 XsdtAddr;
	UINT8 Checksum;
	UINT8 Reserved[3];
} ACPI_RSDP_V2;

// RSDT: Root System Description Table (RSDT)
typedef struct ACPI_RSDT
{
	DESC_HEADER Header;
	UINT32 Entry[ANYSIZE_ARRAY];
} ACPI_RSDT;

// XSDT: Extended System Description Table (XSDT)
typedef struct ACPI_XSDT
{
	DESC_HEADER Header;
	UINT64 Entry[ANYSIZE_ARRAY];
} ACPI_XSDT;

// AAFT: ASRock ACPI Firmware Table (AAFT)
// TODO
typedef struct ACPI_AAFT
{
	DESC_HEADER Header;
} ACPI_AAFT;

// AEST: Arm Error Source Table (AEST)
// NOT SUPPORTED

// AGDI: Arm Generic Diagnostic Dump and Reset Interface (AGDI)
// NOT SUPPORTED

// MADT: Multiple APIC Description Table (APIC)
typedef struct ACPI_MADT
{
	DESC_HEADER Header;
	UINT32 LocalApicAddress;
	UINT32 Flags;
} ACPI_MADT;

typedef enum ACPI_MADT_TYPE
{
	ACPI_MADT_TYPE_PROCESSOR_LOCAL_APIC = 0,
	ACPI_MADT_TYPE_IO_APIC = 1,
	ACPI_MADT_TYPE_INTERRUPT_SOURCE_OVERRIDE = 2,
	ACPI_MADT_TYPE_NMI_SOURCE = 3,
	ACPI_MADT_TYPE_LOCAL_APIC_NMI = 4,
	ACPI_MADT_TYPE_LOCAL_APIC_ADDR_OVERRIDE = 5,
#if 0 // IA-64 only
	ACPI_MADT_TYPE_IO_SAPIC = 6,
	ACPI_MADT_TYPE_LOCAL_SAPIC = 7,
	ACPI_MADT_TYPE_PLATFORM_INTERRUPT_SOURCE = 8,
#endif
	ACPI_MADT_TYPE_PROCESSOR_LOCAL_X2APIC = 9,
	ACPI_MADT_TYPE_LOCAL_X2APIC_NMI = 10,
#if 0 // ARM only
	ACPI_MADT_TYPE_GIC_CPU_INTERFACE = 11,
	ACPI_MADT_TYPE_GIC_DISTRIBUTOR = 12,
	ACPI_MADT_TYPE_GIC_MSI_FRAME = 13,
	ACPI_MADT_TYPE_GIC_REDISTRIBUTOR = 14,
	ACPI_MADT_TYPE_GIC_INTERRUPT_TRANSLATION = 15,
#endif
	ACPI_MADT_TYPE_MULTIPROCESSOR_WAKEUP = 16,
#if 0 // RISC-V, LoongArch etc
	ACPI_MADT_TYPE_CORE_PIC = 17,
	ACPI_MADT_TYPE_LIO_PIC = 18,
	ACPI_MADT_TYPE_HT_PIC = 19,
	ACPI_MADT_TYPE_EIO_PIC = 20,
	ACPI_MADT_TYPE_MSI_PIC = 21,
	ACPI_MADT_TYPE_BIO_PIC = 22,
	ACPI_MADT_TYPE_LPC_PIC = 23,
#endif
} ACPI_MADT_TYPE;

typedef struct APIC_HEADER
{
	UINT8 Type;
	UINT8 Length;
} APIC_HEADER;

// TYPE 0: Processor Local APIC
typedef struct PROCESSOR_LOCAL_APIC
{
	APIC_HEADER Header;
	UINT8 AcpiProcUid;
	UINT8 ApicId;
	UINT32 Flags;
} PROCESSOR_LOCAL_APIC;

// TYPE 1: I/O APIC
typedef struct IO_APIC
{
	APIC_HEADER Header;
	UINT8 IoApicId;
	UINT8 Reserved;
	UINT32 IoApicAddr;
	UINT32 GlobalSystemInterruptBase;
} IO_APIC;

// TYPE 2: Interrupt Source Override
typedef struct INTERRUPT_SOURCE_OVERRIDE
{
	APIC_HEADER Header;
	UINT8 Bus;
	UINT8 Source;
	UINT32 GlobalSysInt;
	UINT16 Flags;
} INTERRUPT_SOURCE_OVERRIDE;

// TYPE 3: NMI Source
typedef struct NMI_SOURCE
{
	APIC_HEADER Header;
	UINT16 Flags;
	UINT32 GlobalSysInt;
} NMI_SOURCE;

// TYPE 4: Local APIC NMI
typedef struct LOCAL_APIC_NMI
{
	APIC_HEADER Header;
	UINT8 AcpiProcUid;
	UINT16 Flags;
	UINT8 LocalApicLint;
} LOCAL_APIC_NMI;

// TYPE 5: Local APIC Address Override
typedef struct LOCAL_APIC_ADDR_OVERRIDE
{
	APIC_HEADER Header;
	UINT16 Reserved;
	UINT64 LocalApicAddr;
} LOCAL_APIC_ADDR_OVERRIDE;

// TYPE 9: Processor Local x2APIC
typedef struct PROCESSOR_LOCAL_X2APIC
{
	APIC_HEADER Header;
	UINT16 Reserved;
	UINT32 X2ApicId;
	UINT32 Flags;
	UINT32 AcpiProcUid;
} PROCESSOR_LOCAL_X2APIC;

// TYPE 10: Local x2APIC NMI
typedef struct LOCAL_X2APIC_NMI
{
	APIC_HEADER Header;
	UINT16 Flags;
	UINT32 AcpiProcUid;
	UINT8 LocalX2ApicLint;
	UINT8 Reserved[3];
} LOCAL_X2APIC_NMI;

// TYPE 16: Multiprocessor Wakeup
typedef struct MULTIPROCESSOR_WAKEUP
{
	APIC_HEADER Header;
	UINT16 MailboxVersion;
	UINT32 Reserved;
	UINT64 MailboxAddress;
} MULTIPROCESSOR_WAKEUP;

// APMT: Arm Performance Monitoring Unit Table (APMT)
// NOT SUPPORTED

// ASF: Alert Standard Format Table (ASF!)
typedef struct ACPI_ASF
{
	DESC_HEADER Header;
	// Information Record #
} ACPI_ASF;

typedef struct ASF_RECORD
{
	UINT8 RecordType;
	UINT8 Reserved;
	UINT16 RecordLength;
	UINT8 RecordData[ANYSIZE_ARRAY];
} ASF_RECORD;

// ASPT: AMD Secure Processor Table (ASPT)
#define ASPT_REVISION_V1 0x01
#define ASPT_REVISION_V2 0x02

#define ASPT_ENTRY_TYPE_ASP_GLOBAL_REGISTERS    0
#define ASPT_ENTRY_TYPE_SEV_MAILBOX_REGISTERS   1
#define ASPT_ENTRY_TYPE_ACPI_MAILBOX_REGISTERS  2

#define ASPT_ASP_MAILBOX_INTERRUPT_ID_SEV_MAILBOX   1
typedef struct ASPT_ENTRY_HEADER
{
	UINT16 Type;
	UINT16 Length;
} ASPT_ENTRY_HEADER;

typedef struct ASPT_ENTRY_ASP_GLOBAL_REGISTERS_V1
{
	ASPT_ENTRY_HEADER Header;
	UINT32 Reserved;
	UINT64 FeatureRegisterAddress;
	UINT64 InterruptEnableRegisterAddress;
	UINT64 InterruptStatusRegisterAddress;
} ASPT_ENTRY_ASP_GLOBAL_REGISTERS_V1;

typedef struct ASPT_ENTRY_SEV_MAILBOX_REGISTERS_V1
{
	ASPT_ENTRY_HEADER Header;
	UINT8  MailboxInterruptId;
	UINT8  Reserved[3];
	UINT64 CmdRespRegisterAddress;
	UINT64 CmdBufAddrLoRegisterAddress;
	UINT64 CmdBufAddrHiRegisterAddress;
} ASPT_ENTRY_SEV_MAILBOX_REGISTERS_V1;

typedef struct ASPT_ENTRY_ACPI_MAILBOX_REGISTERS_V1
{
	ASPT_ENTRY_HEADER Header;
	UINT32 Reserved1;
	UINT64 CmdRespRegisterAddress;
	UINT64 Reserved2[2];
} ASPT_ENTRY_ACPI_MAILBOX_REGISTERS_V1;

typedef struct ASPT_ENTRY_ASP_GLOBAL_REGISTERS_V2
{
	ASPT_ENTRY_HEADER Header;
	UINT32 Reserved;
	UINT32 FeatureRegisterOffset;
	UINT32 InterruptEnableRegisterOffset;
	UINT32 InterruptStatusRegisterOffset;
} ASPT_ENTRY_ASP_GLOBAL_REGISTERS_V2;

typedef struct ASPT_ENTRY_SEV_MAILBOX_REGISTERS_V2
{
	ASPT_ENTRY_HEADER Header;
	UINT8  MailboxInterruptId;
	UINT8  Reserved[3];
	UINT32 CmdRespRegisterOffset;
	UINT32 CmdBufAddrLoRegisterOffset;
	UINT32 CmdBufAddrHiRegisterOffset;
} ASPT_ENTRY_SEV_MAILBOX_REGISTERS_V2;

typedef struct ASPT_ENTRY_ACPI_MAILBOX_REGISTERS_V2
{
	ASPT_ENTRY_HEADER Header;
	UINT32 Reserved1;
	UINT32 CmdRespRegisterOffset;
	UINT64 Reserved2;
} ASPT_ENTRY_ACPI_MAILBOX_REGISTERS_V2;

typedef union ASPT_ENTRY
{
	ASPT_ENTRY_HEADER Header;

	ASPT_ENTRY_ASP_GLOBAL_REGISTERS_V1 AspGlobalRegistersV1;
	ASPT_ENTRY_SEV_MAILBOX_REGISTERS_V1 SevMailboxRegistersV1;
	ASPT_ENTRY_ACPI_MAILBOX_REGISTERS_V1 AcpiMailboxRegistersV1;

	ASPT_ENTRY_ASP_GLOBAL_REGISTERS_V2 AspGlobalRegistersV2;
	ASPT_ENTRY_SEV_MAILBOX_REGISTERS_V2 SevMailboxRegistersV2;
	ASPT_ENTRY_ACPI_MAILBOX_REGISTERS_V2 AcpiMailboxRegistersV2;
} ASPT_ENTRY;

typedef struct ACPI_ASPT_V1
{
	DESC_HEADER Header;
	UINT32 NumberOfAsptEntries;
	//ASPT_ENTRY AsptEntries[NumberOfAsptEntries];
} ACPI_ASPT_V1;

typedef struct ACPI_ASPT_V2
{
	DESC_HEADER Header;
	UINT64 AspRegisterBaseAddress;
	UINT32 AspRegisterSpacePages;
	UINT32 NumberOfAsptEntries;
	//ASPT_ENTRY AsptEntries[NumberOfAsptEntries];
} ACPI_ASPT_V2;

// ATKG: ??? (ATKG)
// NOT SUPPORTED

// BBRT: Boot Background Resource Table (BBRT)
typedef struct ACPI_BBRT
{
	DESC_HEADER Header;
	UINT32 Background;
	UINT32 Foreground;
} ACPI_BBRT;

// BDAT: BIOS Data Table (BDAT)
// TODO
typedef struct ACPI_BDAT
{
	DESC_HEADER Header;
	GEN_ADDR BdatGas;
} ACPI_BDAT;

typedef struct
{
	UINT8 BiosDataSignature[8]; // "BDATHEAD"
	UINT32 BiosDataStructSize;
	UINT16 Crc16;
	UINT16 Reserved;
	UINT16 PrimaryVersion;
	UINT16 SecondaryVersion;
	UINT32 OemOffset;
	UINT32 Reserved1;
	UINT32 Reserved2;
} BDAT_HEADER_STRUCTURE;

// BERT: Boot Error Record Table (BERT)
// TODO
typedef struct ACPI_BERT
{
	DESC_HEADER Header;
	UINT32 BootErrorRegionLength;
	UINT64 BootErrorRegion;
} ACPI_BERT;

// BGRT: Boot Graphics Resource Table (BGRT)
typedef struct ACPI_BGRT
{
	DESC_HEADER Header;
	UINT16 Version; // 1
	UINT8 Status;
	UINT8 ImageType;
	UINT64 ImageAddress; // EfiBootServicesData
	UINT32 ImageOffsetX;
	UINT32 ImageOffsetY;
} ACPI_BGRT;

// BOOT: Simple Boot Flag Table (BOOT)
typedef struct ACPI_BOOT
{
	DESC_HEADER Header;
	UINT8 CmosIndex;
	UINT8 Reserved[3];
} ACPI_BOOT;

// CCEL: CC-Event-Log (CCEL)
// NOT SUPPORTED

// CDIT: ??? (CDIT)
// NOT SUPPORTED

// CEDT: CXL Early Discovery Table (CEDT)
// NOT SUPPORTED

// CPEP: Corrected Platform Error Polling Table (CPEP)
// NOT SUPPORTED

// CRAT: ??? (CRAT)
// NOT SUPPORTED

// CSRT: Core System Resource Table (CSRT)
typedef struct ACPI_CSRT
{
	DESC_HEADER Header;
} ACPI_CSRT;

typedef struct CSRT_RES_GROUP_HEADER
{
	UINT32 Length;
	UINT32 VendorId;
	UINT32 SubvendorId;
	UINT16 DeviceId;
	UINT16 SubdeviceId;
	UINT16 Revision;
	UINT16 Instance;
	UINT32 SharedInfoLength;
} CSRT_RES_GROUP_HEADER;

#define CSRT_RD_TYPE_UNKNOWN 0
#define CSRT_RD_SUBTYPE_UNKNOWN 0

#define CSRT_RD_TYPE_ANY 0xFFFF
#define CSRT_RD_SUBTYPE_ANY 0xFFFF

#define CSRT_RD_TYPE_INTERRUPT 1
#define CSRT_RD_SUBTYPE_INTERRUPT_LINES 0
#define CSRT_RD_SUBTYPE_INTERRUPT_CONTROLLER 1

#define CSRT_RD_TYPE_TIMER 2
#define CSRT_RD_SUBTYPE_TIMER 0

#define CSRT_RD_TYPE_DMA 3
#define CSRT_RD_SUBTYPE_DMA_CHANNEL 0
#define CSRT_RD_SUBTYPE_DMA_CONTROLLER 1

#define CSRT_RD_TYPE_CACHE 4
#define CSRT_RD_SUBTYPE_CACHE 0

#define CSRT_RD_TYPE_POWER 5
#define CSRT_RD_SUBTYPE_POWER_TELEMETRY 1

#define CSRT_RD_UID_ANY 0xFFFF

typedef struct CSRT_RES_DESC_HEADER
{
	UINT32 Length;
	UINT16 Type;
	UINT16 Subtype;
	UINT32 Uid;
} CSRT_RES_DESC_HEADER;

// DBGP: Debug Port Table (DBGP)
typedef struct ACPI_DBGP
{
	DESC_HEADER Header;
	UINT8 InterfaceType;
	UINT8 Reserved[3];
	GEN_ADDR BaseAddr;
} ACPI_DBGP;

// DBG2: Debug Port Table Version 2 (DBG2)
typedef struct ACPI_DBG2
{
	DESC_HEADER Header;
	UINT32 DbgInfoOffset;
	UINT32 DbgInfoCount;
} ACPI_DBG2;

typedef struct DEBUG_DEVICE_INFO
{
	UINT8 Revision;
	UINT16 Length;
	UINT8 NumOfGenAddrReg;
	UINT16 NameSpaceStringLength;
	UINT16 NameSpaceStringOffset;
	UINT16 OemDataLength;
	UINT16 OemDataOffset;
	UINT16 PortType;
	UINT16 PortSubtype;
	UINT16 Reserved;
	UINT16 BaseAddrRegOffset;
	UINT16 AddrSizeOffset;
} DEBUG_DEVICE_INFO;

// DMAR: DMA Remapping Table (DMAR)
typedef struct ACPI_DMAR
{
	DESC_HEADER Header;
	UINT8 HostAddrWidth;
	UINT8 Flags;
	UINT8 Reserved[10];
} ACPI_DMAR;

// DRTM: Dynamic Root of Trust for Measurement Table (DRTM)
// NOT SUPPORTED

// DSDT: Differentiated System Description Table (DSDT)
typedef struct ACPI_DSDT
{
	DESC_HEADER Header;
	UINT8 DefBlock[ANYSIZE_ARRAY]; // AML code
} ACPI_DSDT;

// DTPR: DMA TXT Protected Range (DTPR)
typedef struct ACPI_DTPR
{
	DESC_HEADER Header;
	UINT32 Flags;
	UINT32 TprInstanceCount;
} ACPI_DTPR;

typedef struct TPR_REGISTERS
{
	UINT64 BaseAddress;
	UINT64 Limit;
} TPR_REGISTERS;

typedef struct TPR_INSTANCE
{
	UINT32 Flags;
	UINT32 TprCount;
	UINT64 TprArray[ANYSIZE_ARRAY];
} TPR_INSTANCE;

// ECDT: Embedded Controller Boot Resources Table (ECDT)
typedef struct ACPI_ECDT
{
	DESC_HEADER Header;
	GEN_ADDR Control;
	GEN_ADDR Data;
	UINT32 Uid;
	UINT8 GpeBit;
	CHAR Id[ANYSIZE_ARRAY];
} ACPI_ECDT;

// EINJ: Error Injection Table (EINJ)
typedef struct ACPI_EINJ
{
	DESC_HEADER Header;
	UINT32 InjectionHeaderSize;
	UINT32 InjectionFlags;
	UINT32 InjectionEntryCount;
	ACTION_ENTRY Entry[ANYSIZE_ARRAY];
} ACPI_EINJ;

// ERDT: Enhanced Resource Director Technology (ERDT)
// NOT SUPPORTED

// ERST: Error Record Serialization Table (ERST)
typedef struct ACPI_ERST
{
	DESC_HEADER Header;
	UINT32 SerializationHeaderSize;
	UINT32 Reserved;
	UINT32 InstructionEntryCount;
	ACTION_ENTRY Entry[ANYSIZE_ARRAY];
} ACPI_ERST;

// ETDT: Event Timer Description Table (ETDT)
// OBSOLETE
// Replaced by HPET

// FADT: Fixed ACPI Description Table (FACP)
typedef struct ACPI_FADT
{
	DESC_HEADER Header;
	UINT32 FwCtrl;
	UINT32 DsdtAddr;
	UINT8 Reserved1;
	UINT8 PreferredPmProfile;
	UINT16 SciInt;
	UINT32 SmiCmd;
	UINT8 AcpiEnable;
	UINT8 AcpiDisable;
	UINT8 S4BiosReq;
	UINT8 PstateCnt;
	UINT32 Pm1aEvtBlk;
	UINT32 Pm1bEvtBlk;
	UINT32 Pm1aCntBlk;
	UINT32 Pm1bCntBlk;
	UINT32 Pm2CntBlk;
	UINT32 PmTmrBlk;
	UINT32 Gpe0Blk;
	UINT32 Gpe1Blk;
	UINT8 Pm1EvtLen;
	UINT8 Pm1CntLen;
	UINT8 Pm2CntLen;
	UINT8 PmTmrLen;
	UINT8 Gpe0BlkLen;
	UINT8 Gpe1BlkLen;
	UINT8 Gpe1Base;
	UINT8 CstCnt;
	UINT16 PLvl2Lat;
	UINT16 PLvl3Lat;
	UINT16 FlushSize;
	UINT16 FlushStride;
	UINT8 DutyOffset;
	UINT8 DutyWidth;
	UINT8 DayAlrm;
	UINT8 MonAlrm;
	UINT8 Century;
	UINT16 IapcBootArch;
	UINT8 Reserved2;
	UINT32 Flags;
	GEN_ADDR ResetReg;
	UINT8 ResetValue;
	UINT16 ArmBootArch;
	UINT8 MinorVersion;
	UINT64 XFwCtrl;
	UINT64 XDsdt;
	GEN_ADDR XPm1aEvtBlk;
	GEN_ADDR XPm1bEvtBlk;
	GEN_ADDR XPm1aCntBlk;
	GEN_ADDR XPm1bCntBlk;
	GEN_ADDR XPm2CntBlk;
	GEN_ADDR XPmTmrBlk;
	GEN_ADDR XGpe0Blk;
	GEN_ADDR XGpe1Blk;
	GEN_ADDR SleepControlReg;
	GEN_ADDR SleepStatusReg;
	UINT64 HypervisorVendorId;
} ACPI_FADT;

// FACS: Firmware ACPI Control Structure (FACS)
typedef struct ACPI_FACS
{
	CHAR Signature[4];
	UINT32 Length;
	UINT32 HwSignature;
	UINT32 FwWakingVector;
	UINT32 GlobalLock;
	UINT32 Flags;
	UINT64 XFwWakingVector;
	UINT8 Version;
	UINT8 Reserved[3];
	UINT32 OspmFlags;
	UINT8 Reserved2[24];
} ACPI_FACS;

// FIDT: ??? (FIDT)
// NOT SUPPORTED

// FPDT: Firmware Performance Data Table (FPDT)
// TODO
typedef struct ACPI_FPDT
{
	DESC_HEADER Header;
} ACPI_FPDT;

typedef enum FPDT_RECORD_TYPE
{
	FPDT_TYPE_BOOT_TABLE_PTR = 0,
	FPDT_TYPE_S3_TABLE_PTR = 1
} FPDT_RECORD_TYPE;

typedef struct FPDT_RECORD_HEADER
{
	UINT16 RecordType;
	CHAR RecordLength;
	CHAR Revision;
	UINT32 Reserved;
} FPDT_RECORD_HEADER;

typedef struct FPDT_RECORD
{
	FPDT_RECORD_HEADER RecordHeader;
	union
	{
		struct
		{
			UINT64 PhysicalAddress; // FBPT
		} S3TablePointer;
		struct
		{
			UINT64 PhysicalAddress; // S3PT
		} BasicBootPointer;
	}u;
} FPDT_RECORD;

// GSCI: GMCH SCI Table (GSCI)
// NOT SUPPORTED

// GTDT: Generic Timer Description Table (GTDT)
// TODO
typedef struct ACPI_GTDT
{
	DESC_HEADER Header;
	UINT64 CntCtrlBaseAddr;
	UINT32 Reserved;
	UINT32 SecEl1TimerGsiv;
	UINT32 SecEl1TimerFlags;
	UINT32 NonSecEl1TimerGsiv;
	UINT32 NonSecEl1TimerFlags;
	UINT32 VirtEl1TimerGsiv;
	UINT32 VirtEl1TimerFlags;
	UINT32 El2TimerGsiv;
	UINT32 El2TimerFlags;
	UINT64 CntReadBaseAddr;
	UINT32 PlatformTimerCount;
	UINT32 PlatformTimerOffset;
	UINT32 VirtEl2TimerGsiv;
	UINT32 VirtEl2TimerFlags;
} ACPI_GTDT;

// HEST: Hardware Error Source Table (HEST)
// TODO
typedef struct ACPI_HEST
{
	DESC_HEADER Header;
	UINT32 ErrorSourceCount;
} ACPI_HEST;

// HMAT: Heterogeneous Memory Attribute Table (HMAT)
typedef struct ACPI_HMAT
{
	DESC_HEADER Header;
	UINT32 Reserved;
	// HMAT_ENTRY Entries[];
} ACPI_HMAT;

#define HMAT_ENTRY_TYPE_MSAR    0
#define HMAT_ENTRY_TYPE_SLLBI   1
#define HMAT_ENTRY_TYPE_MSCI    2

#define HMAT_SLLBI_DATA_TYPE_ACCESS_LATENCY     0
#define HMAT_SLLBI_DATA_TYPE_READ_LATENCY       1
#define HMAT_SLLBI_DATA_TYPE_WRITE_LATENCY      2
#define HMAT_SLLBI_DATA_TYPE_ACCESS_BANDWIDTH   3
#define HMAT_SLLBI_DATA_TYPE_READ_BANDWIDTH     4
#define HMAT_SLLBI_DATA_TYPE_WRITE_BANDWIDTH    5

#define HMAT_SLLBI_HIERARHCY_MEMORY             0
#define HMAT_SLLBI_HIERARCHY_CACHE_LEVEL_ONE    1
#define HMAT_SLLBI_HIERARCHY_CACHE_LEVEL_TWO    2
#define HMAT_SLLBI_HIERARCHY_CACHE_LEVEL_THREE  3

#define HMAT_MSCI_CACHEATTRIBUTES_LEVELS_NONE                   0
#define HMAT_MSCI_CACHEATTRIBUTES_LEVELS_ONE                    1
#define HMAT_MSCI_CACHEATTRIBUTES_LEVELS_TWO                    2
#define HMAT_MSCI_CACHEATTRIBUTES_LEVELS_THREE                  3

#define HMAT_MSCI_CACHEATTRIBUTES_ASSOCIATIVITY_NONE            0
#define HMAT_MSCI_CACHEATTRIBUTES_ASSOCIATIVITY_DIRECT_MAPPED   1
#define HMAT_MSCI_CACHEATTRIBUTES_ASSOCIATIVITY_COMPLEX         2

#define HMAT_MSCI_CACHEATTRIBUTES_WRITE_POLICY_NONE             0
#define HMAT_MSCI_CACHEATTRIBUTES_WRITE_POLICY_WRITE_BACK       1
#define HMAT_MSCI_CACHEATTRIBUTES_WRITE_POLICY_WRITE_THROUGH    2

typedef struct HMAT_ENTRY
{
	UINT16 Type;
	UINT16 Reserved;
	UINT32 Length;
	union
	{
		struct
		{
			union
			{
				struct
				{
					UINT16 ProcessorProximityDomainValid : 1;
					UINT16 Reserved0 : 1;
					UINT16 Reserved1 : 1;
					UINT16 Reserved : 13;
				} DUMMYSTRUCTNAME;
				UINT16 AsWORD;
			} Flags;
			UINT16 Reserved1;
			UINT32 ProcessorProximityDomain;
			UINT32 MemoryProximityDomain;
			UINT32 Reserved2;
			DWORDLONG Reserved3;
			DWORDLONG Reserved4;
		} Msar;
		struct
		{
			union
			{
				struct
				{
					UINT8 MemoryHierarchy : 4;
					UINT8 MinTransferSizeToAchieveValues : 1;
					UINT8 NonSequentialTransfers : 1;
					UINT8 Reserved : 2;
				} DUMMYSTRUCTNAME;
				UINT8 AsBYTE;
			} Flags;
			UINT8 DataType;
			UINT8 MinTransferSize;
			UINT8 Reserved1;
			UINT32 NumberOfInitiatorProximityDomains;
			UINT32 NumberOfTargetProximityDomains;
			UINT32 Reserved2;
			DWORDLONG EntryBaseUnit;
			// DWORD InitiatorProximityDomainList[NumberOfInitiatorProximityDomains]
			// DWORD TargetProximityDomainList[NumberOfTargetProximityDomains]
			// WORD Entry[NumberOfInitiatorProximityDomains][NumberOfTargetProximityDomains]
		} Sllbi;
		struct
		{
			UINT32 MemoryProximityDomain;
			UINT32 Reserved1;
			DWORDLONG MemorySideCacheSize;
			union
			{
				struct
				{
					UINT32 TotalCacheLevels : 4;
					UINT32 CacheLevel : 4;
					UINT32 CacheAssociativity : 4;
					UINT32 WritePolicy : 4;
					UINT32 CacheLineSize : 16;
				} DUMMYSTRUCTNAME;
				UINT32 AsDWORD;
			} CacheAttributes;
			UINT16 Reserved2;
			UINT16 NumberOfSmBiosHandles;
			// WORD SmBiosHandles[NumberOfSmBiosHandles];
		} Msci;
	} DUMMYUNIONNAME;
} HMAT_ENTRY;

#define HMAT_ENTRY_HEADER_LENGTH RTL_SIZEOF_THROUGH_FIELD(HMAT_ENTRY, Length);
#define HMAT_ENTRY_LENGTH(_Type) RTL_SIZEOF_THROUGH_FIELD(HMAT_ENTRY, _Type);

// HPET: High Precision Event Timer Table (HPET)
// TODO
typedef struct ACPI_HPET
{
	DESC_HEADER Header;
	UINT32 EventTimerBlockId;
	GEN_ADDR Address;
	UINT8 HpetNumber;
	UINT16 MinimumPeriodicTickCount;
	UINT8 PageProtection;
} ACPI_HPET;

// iBFT: iSCSI Boot Firmware Table (iBFT)
// TODO
typedef struct ACPI_IBFT
{
	DESC_HEADER Header;
	UINT8 Reserved[12];
} ACPI_IBFT;

// IEIT: ??? (IEIT)
// NOT SUPPORTED

// IERS: Inline Encryption Reporting Structure (IERS)
// NOT SUPPORTED

// IORT: I/O Remapping Table (IORT)
// TODO
typedef struct ACPI_IORT
{
	DESC_HEADER Header;
	UINT32 NodeCount;
	UINT32 NodeArrayOffset;
	UINT8 Reserved[4];
	// Optional padding
	// Array of IORT nodes
} ACPI_IORT;

// IVRS: I/O Virtualization Reporting Structure (IVRS)
// TODO
typedef union IVRS_IVINFO
{
	UINT32 AsUINT32;
	struct
	{
		UINT32 EFRSup : 1;
		UINT32 DmaGuardOptIn : 1;
		UINT32 ReservedZ0 : 3;
		UINT32 GVASize : 3;
		UINT32 PASize : 7;
		UINT32 VASize : 7;
		UINT32 HtAtsResv : 1;
		UINT32 ReservedZ1 : 9;
	} DUMMYSTRUCTNAME;
} IVRS_IVINFO;

typedef struct ACPI_IVRS
{
	DESC_HEADER Header;
	IVRS_IVINFO IVInfo;
	UINT64 Reserved;
	UINT8 DefinitionBlocks[1];
} ACPI_IVRS;

// KEYP: Key Programming Interface for Root Complex Integrity and Data Encryption (IDE) (KEYP)
// NOT SUPPORTED

// LPIT: Low Power Idle Table (LPIT)
typedef union LPI_STATE_FLAGS
{
	struct
	{
		UINT32 Disabled : 1;
		UINT32 CounterUnavailable : 1;
		UINT32 Reserved : 30;
	};
	UINT32 AsDWORD;
} LPI_STATE_FLAGS;

typedef struct LPI_STATE_DESC
{
	UINT32 Type;
	UINT32 Length;
	UINT16 UniqueId;
	UINT8 Reserved[2];
	LPI_STATE_FLAGS Flags;
	GEN_ADDR EntryTrigger;
	UINT32 Residency;
	UINT32 Latency;
	GEN_ADDR ResidencyCounter;
	DWORDLONG ResidencyCounterFrequency;
} LPI_STATE_DESC;

typedef struct ACPI_LPIT
{
	DESC_HEADER Header;
	LPI_STATE_DESC LpiStates[ANYSIZE_ARRAY];
} ACPI_LPIT;

// MATR: Memory Address Translation Table (MATR)
// NOT SUPPORTED

// MCFG: PCIe Memory Mapped Configuration Table (MCFG)
typedef struct MCFG_ENTRY
{
	DWORDLONG BaseAddress;
	UINT16 SegmentNumber;
	UINT8 StartBusNumber;
	UINT8 EndBusNumber;
	UINT32 Reserved;
} MCFG_ENTRY;

typedef struct ACPI_MCFG
{
	DESC_HEADER Header;
	UINT32 Reserved[2];
	MCFG_ENTRY TableEntry[ANYSIZE_ARRAY];
} ACPI_MCFG;

// MCHI: Management Controller Host Interface Table
// NOT SUPPORTED

// MHSP: Microsoft Pluton Security Processor Table
// TODO
typedef struct MHSP_CHANNEL
{
	UINT64 ChannelBaseAddress;
	UINT64 RequestDoorbellAddress;
	UINT64 ReplyDoorbellAddress;
	UINT32 ChannelSize;
	UINT32 IrqResource;
	UINT32 ChannelParameters[2];
} MHSP_CHANNEL;

typedef struct ACPI_MHSP
{
	DESC_HEADER Header;
	UINT32 ProtocolId;
	MHSP_CHANNEL Channels[4];
} ACPI_MHSP;

// MISC: Miscellaneous GUIDed Table Entries (MISC)
typedef struct ACPI_MISC
{
	DESC_HEADER Header;
	// GUIDed Entries
} ACPI_MISC;

typedef struct MISC_GUID_ENTRY
{
	GUID Guid;
	UINT32 Length;
	UINT32 Revision;
	UINT32 ProducerId;
	UINT8 Data[ANYSIZE_ARRAY];
} MISC_GUID_ENTRY;

// MRRM: Memory Range and Region Mapping Table (MRRM)
// NOT SUPPORTED

// MPAM: Arm Memory Partitioning And Monitoring (MPAM)
// NOT SUPPORTED

// MPST: Memory Power State Table (MPST)
typedef struct MEMORY_POWER_STATE
{
	UINT8 PowerStateValue;
	UINT8 PowerStateInformationIndex;
} MEMORY_POWER_STATE;

typedef struct MEMORY_POWER_NODE
{
	UINT8 Flag;
	UINT8 Reserved1;
	UINT16 MpnId;
	UINT32 Length;
	UINT32 BaseAddressLow;
	UINT32 BaseAddressHigh;
	UINT32 LengthLow;
	UINT32 LengthHigh;
	UINT32 PowerStateCount;
	UINT32 PhysicalComponentCount;
	MEMORY_POWER_STATE MpState; // Start of PowerStateCount structures,
	// followed by 'PhysicalComponentCount'
	// physical component identifiers.
} MEMORY_POWER_NODE;

typedef struct ACPI_MPST
{
	DESC_HEADER Header;
	UINT8 SubspaceId;
	UINT8 Reserved2[3];
	UINT16 MpnCount;
	UINT8 Reserved[2];
	MEMORY_POWER_NODE Mpn; // MpnCount Mpn structures begin here.
	// Followed by a USHORT memory power
	// characteristic count.
	// Followed by an array of memory power
	// state characteristics structures.
} ACPI_MPST;

typedef struct POWER_STATE_CHARACTERISTICS
{
	union
	{
		UINT8 AsUINT8;
		struct
		{
			UINT8 Value : 6;
			UINT8 Revision : 2;
		} DUMMYSTRUCTNAME;
	} ID;
	UINT8 Flag;
	UINT16 Reserved1;
	UINT32 PowerInMPS0;
	UINT32 PowerSavingToMPS0;
	UINT64 ExitLatencyNs;
	UINT8 Reserved2[8];
} POWER_STATE_CHARACTERISTICS;

// MSCT: Maximum System Characteristics Table (MSCT)
typedef struct ACPI_MSCT
{
	DESC_HEADER Header;
	UINT32 DomainInfoOffset;
	UINT32 ProximityDomainCount;
	UINT32 ClockDomainCount;
	UINT64 MaximumPhysicalAddress;
} ACPI_MSCT;

typedef struct MSCT_ENTRY
{
	UINT8 Revision;
	UINT8 Length;
	UINT32 DomainIdRangeLow;
	UINT32 DomainIdRangeHigh;
	UINT32 ProcessorCapacity;
	UINT64 MemoryCapacity;
} MSCT_ENTRY;

#define ACPI_MSCT_MINIMUM_LENGTH sizeof(ACPI_MSCT)

// MSDM: Microsoft Data Management Table (MSDM)
typedef struct ACPI_MSDM
{
	DESC_HEADER Header;
	UINT32 Version;
	UINT32 Reserved;
	UINT32 DataType;
	UINT32 DataReserved;
	UINT32 DataLength;
	UINT8 Data[ANYSIZE_ARRAY];
} ACPI_MSDM;

// NBFT: NVMe-oF Boot Firmware Table
// NOT SUPPORTED

// NFIT: NVDIMM Firmware Interface Table (NFIT)
// TODO
typedef struct ACPI_NFIT
{
	DESC_HEADER Header;
	UINT32 Reserved;
	// NFIT structures
} ACPI_NFIT;

typedef struct NFIT_STRUCT_HEADER
{
	UINT16 Type;
	UINT16 Length;
} NFIT_STRUCT_HEADER;

typedef enum
{
	NFIT_SYS_PHYS_ADDR_RANGE = 0,
	NFIT_NVDIMM_REGION_MAPPING = 1,
	NFIT_INTERLEAVE = 2,
	NFIT_SMBIOS_MANAGEMENT_INFO = 3,
	NFIT_NVDIMM_CTRL_REGION = 4,
	NFIT_NVDIMM_BLK_DATA_WIN_REGION = 5,
	NFIT_FLUSH_HINT_ADDR = 6,
	NFIT_PLATFORM_CAPABILITIES = 7,
	NFIT_MAX
} NFIT_STRUCTURE_TYPE;

// NHLT: Non HD Audio Link Table (NHLT)
// NOT SUPPORTED

// OEM: OEM Specific Information Tables (OEMx)
// TODO

// OSDT: Override System Description Table (OSDT)
// NOT SUPPORTED

// PCCT: Platform Communications Channel Table (PCCT)
typedef struct ACPI_PCCT
{
	DESC_HEADER Header;
	union
	{
		UINT32 AsULong;
		struct
		{
			UINT32 SciSupported : 1;
			UINT32 Reserved : 31;
		} DUMMYSTRUCTNAME;
	} Flags;
	UINT64 Reserved;
} ACPI_PCCT;

// PCCT Subspace Common Header
typedef struct PCC_SUBSPACE_HEADER
{
	UINT8 Type;
	UINT8 Length;
} PCC_SUBSPACE_HEADER;

#define PCC_SUBSPACE_TYPE_GENERIC       0
#define PCC_SUBSPACE_TYPE_REDUCED_1     1
#define PCC_SUBSPACE_TYPE_REDUCED_2     2
#define PCC_SUBSPACE_TYPE_EXTENDED_3    3
#define PCC_SUBSPACE_TYPE_EXTENDED_4    4

typedef struct PCC_GENERIC_SUBSPACE
{
	PCC_SUBSPACE_HEADER Header;
	UINT16 Reserved1;
	UINT32 Reserved2;
	UINT64 BaseAddress;
	UINT64 Length;
	GEN_ADDR DoorbellRegister;
	UINT64 DoorbellPreserve;
	UINT64 DoorbellWrite;
	UINT32 NominalLatency;
	UINT32 MaximumPeriodicAccessRate;
	UINT16 MinimumRequestTurnaroundTime;
} PCC_GENERIC_SUBSPACE;

#define PCC_PLATFORM_INTERRUPT_POLARITY_ACTIVE_HIGH     0
#define PCC_PLATFORM_INTERRUPT_POLARITY_ACTIVE_LOW      1

#define PCC_PLATFORM_INTERRUPT_MODE_LEVEL_TRIGGERED     0
#define PCC_PLATFORM_INTERRUPT_MODE_EDGE_TRIGGERED      1

typedef struct PCC_REDUCED_1_SUBSPACE
{
	PCC_SUBSPACE_HEADER Header;
	UINT32 PlatformInterruptGsiv;
	union
	{
		struct
		{
			UINT8 PlatformInterruptPolarity : 1;
			UINT8 PlatformInterruptMode : 1;
			UINT8 Reserved1 : 6;
		};
		UINT8 PlatformInterruptFlags;
	};

	UINT8 Reserved2;
	UINT64 BaseAddress;
	UINT64 Length;
	GEN_ADDR DoorbellRegister;
	UINT64 DoorbellPreserve;
	UINT64 DoorbellWrite;
	UINT32 NominalLatency;
	UINT32 MaximumPeriodicAccessRate;
	UINT16 MinimumRequestTurnaroundTime;
} PCC_REDUCED_1_SUBSPACE;

typedef struct PCC_REDUCED_2_SUBSPACE
{
	PCC_SUBSPACE_HEADER Header;
	UINT32 PlatformInterruptGsiv;
	union
	{
		struct
		{
			UINT8 PlatformInterruptPolarity : 1;
			UINT8 PlatformInterruptMode : 1;
			UINT8 Reserved1 : 6;
		};
		UINT8 PlatformInterruptFlags;
	};
	UINT8 Reserved2;
	UINT64 BaseAddress;
	UINT64 Length;
	GEN_ADDR DoorbellRegister;
	UINT64 DoorbellPreserve;
	UINT64 DoorbellWrite;
	UINT32 NominalLatency;
	UINT32 MaximumPeriodicAccessRate;
	UINT16 MinimumRequestTurnaroundTime;
	GEN_ADDR PlatformInterruptAckRegister;
	UINT64 PlatformInterruptAckPreserve;
	UINT64 PlatformInterruptAckWrite;
} PCC_REDUCED_2_SUBSPACE;

#define PCC_GENERIC_SHARED_REGION_SIGNATURE 0x50434300 // " CCP"

typedef struct PCC_GENREIC_SHARED_REGION
{
	UINT32 Signature;
	union
	{
		struct
		{
			UINT16 CommandCode : 8; // 7:0
			UINT16 ReservedZ : 7;   // 14:8
			UINT16 SciDoorbell : 1; // 15 - Notify on completion
		};
		UINT16 AsUShort;
	} Command;
	union
	{
		struct
		{
			UINT16 CommandComplete : 1;           // 0
			UINT16 SciReceived : 1;               // 1 - Platform interrupt received
			UINT16 Error : 1;                     // 2
			UINT16 PlatformNotification : 1;      // 3
			UINT16 Reserved : 12;                 // 15:4
		};
		UINT16 AsUShort;
	} Status;
	UINT8 CommunicationSpace[ANYSIZE_ARRAY];
} PCC_GENERIC_SHARED_REGION;

typedef struct PCC_EXTENDED_3_4_SUBSPACE
{
	PCC_SUBSPACE_HEADER Header;
	UINT32 PlatformInterruptGsiv;
	union
	{
		struct
		{
			UINT8 PlatformInterruptPolarity : 1;
			UINT8 PlatformInterruptMode : 1;
			UINT8 Reserved1 : 6;
		};
		UINT8 PlatformInterruptFlags;
	};
	UINT8 Reserved2;
	UINT64 BaseAddress;
	UINT32 Length;
	GEN_ADDR DoorbellRegister;
	UINT64 DoorbellPreserve;
	UINT64 DoorbellWrite;
	UINT32 NominalLatency;
	UINT32 MaximumPeriodicAccessRate;
	UINT32 MinimumRequestTurnaroundTime;
	GEN_ADDR PlatformInterruptAckRegister;
	UINT64 PlatformInterruptAckPreserve;
	UINT64 PlatformInterruptAckWrite;
	UINT64 Reserved3;
	GEN_ADDR CommandCompleteCheckRegister;
	UINT64 CommandCompleteCheckMask;
	GEN_ADDR CommandCompleteUpdateRegister;
	UINT64 CommandCompleteUpdatePreserve;
	UINT64 CommandCompleteUpdateWrite;
	GEN_ADDR ErrorStatusRegister;
	UINT64 ErrorStatusMask;
} PCC_EXTENDED_3_4_SUBSPACE;

#define PCC_EXTENDED_3_4_SUBSPACE_SIZE 164

typedef struct PCC_EXTENDED_SHARED_REGION
{
	UINT32 Signature;
	union
	{
		struct
		{
			UINT32 NotifyOnComplete : 1;
			UINT32 Reserved : 31;
		};
		UINT32 AsUlong;
	} Flags;
	UINT32 Length;
	UINT32 CommandCode;
	UINT8 CommunicationSpace[ANYSIZE_ARRAY];
} PCC_EXTENDED_SHARED_REGION;

#define PCC_EXTENDED_SHARED_REGION_COMM_SPACE_OFFSET 16

// PDTT: Platform Debug Trigger Table (PDTT)
typedef struct ACPI_PDTT
{
	DESC_HEADER Header;
	UINT8 TriggerCount;
	UINT8 Reserved[3];
	UINT32 TriggerOffset;
	// PDTT pcc sub channel indetifiers
} ACPI_PDTT;

typedef struct PDTT_PCC_SUBCHANNEL_INDENTIFIER
{
	UINT8 SubChannelId;
	UINT8 Runtime : 1;
	UINT8 WaitCompletion : 1;
	UINT8 Reserved : 6;
} PDTT_PCC_SUBCHANNEL_INDENTIFIER;

// PHAT: Platform Health Assessment Table (PHAT)
typedef struct ACPI_PHAT
{
	DESC_HEADER Header;
} ACPI_PHAT;

typedef struct PHAT_RECORD_HEADER
{
	UINT16 Type;
	UINT16 Length;
	UINT8 Revision;
} PHAT_RECORD_HEADER;

#define FW_VERSION_DATA_RECORD_TYPE 0x0000
#define FW_HEALTH_DATA_RECORD_TYPE  0x0001

typedef struct FW_VERSION_ELEMENT
{
	GUID ComponentId;
	UINT64 VersionValue;
	UINT32 ProducerId; // 'ABCD'
} FW_VERSION_ELEMENT;

typedef struct FW_VERSION_DATA_RECORD
{
	PHAT_RECORD_HEADER Header;
	UINT8 Reserved[3];
	UINT32 RecordCount;
	FW_VERSION_ELEMENT Element[ANYSIZE_ARRAY];
} FW_VERSION_DATA_RECORD;

typedef struct FW_HEALTH_DATA_RECORD
{
	PHAT_RECORD_HEADER Header;
	UINT8 Reserved[2];
	UINT8 AmHealthy; // 0- Errors found, 1 = No errors, 2 = Unknown, 3 = Advisory
	GUID DeviceSignature;
	UINT32 DeviceSpecificDataOffset;
	// BYTE UefiDevicePath[];
	// BYTE DeviceSpecificData[];
} FW_HEALTH_DATA_RECORD;

// PMTT: Platform Memory Topology Table (PMTT)
typedef struct ACPI_PMTT
{
	DESC_HEADER Header;
	UINT32 NumOfMemoryDevices;
	// PMTT_COMMON_DEVICE Device[]
} ACPI_PMTT;

// PMTT Common Header for all device structures
typedef struct PMTT_COMMON_DEVICE
{
	UINT8 Type;
	UINT8 Reserved1;
	UINT16 Length;
	UINT16 Flags;
	UINT16 Reserved2;
	UINT32 NumOfAssocDevices;
	// Type Specific Data
	// PMTT_COMMON_DEVICE Assoc[]
} PMTT_COMMON_DEVICE;

// PMTT Type 0: Socket
typedef struct PMTT_SOCKET
{
	PMTT_COMMON_DEVICE Header;
	UINT16 Id;
	UINT16 Reserved;
} PMTT_SOCKET;

// PMTT Type 1: Memory Controller
typedef struct PMTT_MEMORY_CONTROLLER
{
	PMTT_COMMON_DEVICE Header;
	UINT16 Id;
	UINT16 Reserved;
} PMTT_MEMORY_CONTROLLER;

// PMTT Type 2: DIMM
typedef struct PMTT_DIMM
{
	PMTT_COMMON_DEVICE Header;
	UINT32 SmbiosHandle;
} PMTT_DIMM;

// PMTT Type 0xFF: Vendor Specific
typedef struct PMTT_VENDOR_SPECIFIC
{
	PMTT_COMMON_DEVICE Header;
	GUID TypeUuid;
	UINT8 VendorData[ANYSIZE_ARRAY];
} PMTT_VENDOR_SPECIFIC;

// POAT: Un-Initialized Phoenix OEM activation Table (POAT)
// NOT SUPPORTED

// PPTT: Processor Properties Topology Table (PPTT)
typedef struct ACPI_PPTT
{
	DESC_HEADER Header;
	// PROC_TOPOLOGY_NODE Node[ANYSIZE_ARRAY];
} ACPI_PPTT;

typedef union PROC_TOPOLOGY_NODE_FLAGS
{
	struct
	{
		UINT32 PhysicalPackage : 1;
		UINT32 ACPIProcessorIdValid : 1;
		UINT32 ProcessorIsThread : 1;
		UINT32 IsLeaf : 1;
		UINT32 IdenticalImplementation : 1;
		UINT32 Reserved : 27;
	};
	UINT32 AsULONG;
} PROC_TOPOLOGY_NODE_FLAGS;

typedef union PROC_TOPOLOGY_CACHE_FLAGS
{
	struct
	{
		UINT32 SizeValid : 1;
		UINT32 SetsValid : 1;
		UINT32 AssociativityValid : 1;
		UINT32 AllocationTypeValid : 1;
		UINT32 CacheTypeValid : 1;
		UINT32 WritePolicyValid : 1;
		UINT32 LineSizeValid : 1;
		UINT32 CacheIdValid : 1;
		UINT32 Reserved : 24;
	};
	UINT32 AsULONG;
} PROC_TOPOLOGY_CACHE_FLAGS;

#define PROC_TOPOLOGY_CACHE_TYPE_DATA 0
#define PROC_TOPOLOGY_CACHE_TYPE_INSTRUCTION 1
#define PROC_TOPOLOGY_CACHE_TYPE_UNIFIED 2

#define PROC_TOPOLOGY_NODE_CACHE_TYPE_DATA(CacheType) \
    (CacheType == 0)

#define PROC_TOPOLOGY_NODE_CACHE_TYPE_INSTRUCTION(CacheType) \
    (CacheType == 1)

#define PROC_TOPOLOGY_NODE_CACHE_TYPE_UNIFIED(CacheType) \
    ((CacheType == 2) || (CacheType == 3))

typedef union PROC_TOPOLOGY_CACHE_ATTRIBUTES
{
	struct
	{
		UINT8 ReadAllocate : 1;
		UINT8 WriteAllocate : 1;
		UINT8 CacheType : 2;
		UINT8 WritePolicy : 1;
		UINT8 Reserved : 3;
	};
	UINT8 AsUCHAR;
} PROC_TOPOLOGY_CACHE_ATTRIBUTES;

#define PROC_TOPOLOGY_NODE_HIERARCHY 0
#define PROC_TOPOLOGY_NODE_CACHE 1
#define PROC_TOPOLOGY_NODE_ID 2

typedef struct PROC_TOPOLOGY_NODE
{
	struct
	{
		UINT8 Type;
		UINT8 Length;
		UINT8 Reserved[2];
	};
	union
	{
		struct
		{
			PROC_TOPOLOGY_NODE_FLAGS Flags;
			UINT32 Parent;
			UINT32 ACPIProcessorId;
			UINT32 NumberPrivateResources;
			UINT32 PrivateResources[ANYSIZE_ARRAY];
		} HierarchyNode;
		struct
		{
			PROC_TOPOLOGY_CACHE_FLAGS Flags;
			UINT32 NextLevelCacheOffset;
			UINT32 Size;
			UINT32 Sets;
			UINT8 Associativity;
			PROC_TOPOLOGY_CACHE_ATTRIBUTES Attributes;
			UINT16 LineSize;
			UINT32 CacheId;
		} CacheNode;
		struct
		{
			UINT32 Vendor;
			UINT64 Level1;
			UINT64 Level2;
			UINT16 Major;
			UINT16 Minor;
			UINT16 Spin;
		} IdNode;
	};
} PROC_TOPOLOGY_NODE;

// PRMT: Platform Runtime Mechanism Table (PRMT)
typedef struct ACPI_PRMT
{
	DESC_HEADER Header;
	GUID PlatformGuid;
	UINT32 ModuleInfoOffset;
	UINT32 ModuleInfoCount;
} ACPI_PRMT;

typedef struct PRM_HANDLER_INFORMATION
{
	UINT16 Revision;
	UINT16 Length;
	GUID Identifier;
	UINT64 PhysicalAddress;
	UINT64 StaticBufferPhysicalAddress;
	UINT64 AcpiParameterPhysicalAddress;
} PRM_HANDLER_INFORMATION;

typedef struct PRM_MODULE_INFORMATION
{
	UINT16 Revision;
	UINT16 Length;
	GUID Identifier;
	UINT16 MajorVersion;
	UINT16 MinorVersion;
	UINT16 HandlerCount;
	UINT32 HandlerOffset;
	UINT64 MmioRangesPhysicalAddress;
} PRM_MODULE_INFORMATION;

// PSDT: Persistent System Description Table (PSDT)
// NOT SUPPORTED
// ACPI 1.0 only

// PTDT: ??? (PTDT)
// NOT SUPPORTED

// RAS: RAS Feature Table (RASF)
// TODO
typedef struct ACPI_RASF
{
	DESC_HEADER Header;
	UINT8 PccId[12];
} ACPI_RASF;

// RAS2: RAS2 Feature Table (RAS2)
// TODO
typedef struct ACPI_RAS2
{
	DESC_HEADER Header;
	UINT8 Reserved[2];
	UINT16 NumOfPccDesc;
	// RAS2_PCC_DESC PccDesc[]
} ACPI_RAS2;

typedef struct RAS2_PCC_DESC
{
	UINT8 PccId;
	UINT8 Reserved[2];
	UINT8 FeatureType;
	UINT32 Instance;
} RAS2_PCC_DESC;

// RGRT: Regulatory Graphics Resource Table (RGRT)
// NOT SUPPORTED

// RHCT: RISC-V Hart Capabilities Table (RHCT)
// NOT SUPPORTED

// RIMT: RISC-V IO Mapping Table (RIMT)
// NOT SUPPORTED

// Smart Battery Specification Table (SBST)
typedef struct ACPI_SBST
{
	DESC_HEADER Header;
	UINT32 WarningLevel; // mWh
	UINT32 LowLevel;
	UINT32 CriticalLevel;
} ACPI_SBST;

// Software Delegated Exceptions Interface (SDEI)
// NOT SUPPORTED

// SDEV: Secure Devices Table (SDEV)
typedef struct ACPI_SDEV
{
	DESC_HEADER Header;
	// SDEVTables
} ACPI_SDEV;

#define SDEV_SECURE_ACPI_TYPE 0
#define SDEV_SECURE_PCI_TYPE 1

#define SDEV_ENTRY_FLAG_OPTIONALLY_SECURE 1
#define SDEV_ENTRY_FLAG_SECURE_RESOURCES_PRESENT 2

#define SDEV_SECURE_RESOURCE_ID_TYPE 0
#define SDEV_SECURE_RESOURCE_MEMORY_TYPE 1

typedef struct SDEV_ENTRY_HEADER
{
	UINT8 Type;
	UINT8 Flags;
	UINT16 Length;
} SDEV_ENTRY_HEADER;

typedef struct SDEV_SECURE_PCI_INFO_ENTRY
{
	SDEV_ENTRY_HEADER Header;
	UINT16 PciSegmentNumber;
	UINT16 StartBusNumber;
	UINT16 PciPathOffset;
	UINT16 PciPathLength;
	UINT16 VendorInfoOffset;
	UINT16 VendorInfoLength;
} SDEV_SECURE_PCI_INFO_ENTRY;

typedef struct SDEV_SECURE_ACPI_INFO_ENTRY
{
	SDEV_ENTRY_HEADER Header;
	UINT16 IdentifierOffset;
	UINT16 IdentifierLength;
	UINT16 VendorInfoOffset;
	UINT16 VendorInfoLength;
	UINT16 SecureResourcesOffset;
	UINT16 SecureResourcesLength;
} SDEV_SECURE_ACPI_INFO_ENTRY;

typedef struct SDEV_SECURE_RESOURCE_ID_ENTRY
{
	SDEV_ENTRY_HEADER Header;
	UINT16 HardwareIdentifierOffset;
	UINT16 HardwareIdentifierLength;
	UINT16 SubsystemIdentifierOffset;
	UINT16 SubsystemIdentifierLength;
	UINT16 HardwareRevision;
	UINT8 HardwareRevisionPresent;
} SDEV_SECURE_RESOURCE_ID_ENTRY;

typedef struct SDEV_SECURE_RESOURCE_MEMORY_ENTRY
{
	SDEV_ENTRY_HEADER Header;
	UINT32 Reserved;
	UINT64 MemoryAddressBase;
	UINT64 MemoryAddressLength;
} SDEV_SECURE_RESOURCE_MEMORY_ENTRY;

// SLIC: Microsoft Software Licensing Table (SLIC)
typedef struct ACPI_SLIC
{
	DESC_HEADER Header;
	// Public Key (156 Bytes)
	// WindowsMarker (182 Bytes)
} ACPI_SLIC;

typedef struct SLIC_HEADER
{
	UINT32 Type; // 0 = Public Key, 1 = Marker
	UINT32 Length;
} SLIC_HEADER;

typedef struct SLIC_PUBLIC_KEY
{
	SLIC_HEADER Header;
	UINT8 KeyType;
	UINT8 Version;
	UINT16 Reserved;
	UINT32 Algorithm;
	UINT8 Magic[4];
	UINT32 BitLength;
	UINT32 Exponent;
	UINT8 Modulus[128];
} SLIC_PUBLIC_KEY;

typedef struct SLIC_MARKER
{
	SLIC_HEADER Header;
	UINT32 Version;
	UINT8 OemId[6];
	UINT8 OemTableId[8];
	UINT8 WindowsFlag[8];
	UINT32 SlicVersion;
	UINT8 Reserved[16];
	UINT8 Signature[128];
} SLIC_MARKER;

// SLIT: System Locality Distance Information Table (SLIT)
typedef struct ACPI_SLIT
{
	DESC_HEADER Header;
	UINT64 LocalityCount;
	// UCHAR Entry[LocalityCount][LocalityCount]
} ACPI_SLIT;

// SPCR: Microsoft Serial Port Console Redirection Table (SPCR)
typedef struct ACPI_SPCR
{
	DESC_HEADER Header;
	UINT8 IfType;
	UINT8 Reserved1[3];
	GEN_ADDR BaseAddr;
	UINT8 IntType;
	UINT8 Irq;
	UINT32 GlobalSysInt;
	UINT8 BaudRate;
	UINT8 Parity;
	UINT8 StopBits;
	UINT8 FlowCtrl;
	UINT8 TermType;
	UINT8 Lang;
	UINT16 PciDeviceId;
	UINT16 PciVendorId;
	UINT8 PciBusNum;
	UINT8 PciDevNum;
	UINT8 PciFnNum;
	UINT32 PciFlags;
	UINT8 PciSegment;
	UINT32 UartClockFreq;
	UINT32 PreciseBaudRate;
	UINT16 NamespaceStrLength;
	UINT16 NamespaceStrOffset;
	// CHAR NamespaceString[]; ASCII string, null-terminated
} ACPI_SPCR;

// SPMI: Service Processor Management Interface Table (SPMI)
typedef struct ACPI_SPMI
{
	DESC_HEADER Header;
	UINT8 InterfaceType;
	UINT8 Reserved1;
	UINT16 SpecificationRevision;
	struct
	{
		UINT8 GpeSupported : 1;
		UINT8 ApicInterrupt : 1;
		UINT8 Reserved : 6;
	} InterruptType;
	UINT8 Gpe;
	UINT8 Reserved2;
	UINT8 PciDeviceFlag;
	UINT32 GlobalSystemInterrupt;
	GEN_ADDR BaseAddress;
	UINT8 PciSegmentGroupNumber;
	UINT8 PciBusNumber;
	UINT8 PciDeviceNumber;
	UINT8 PciFunctionNumber;
	UINT8 Reserved3;
} ACPI_SPMI;

// SRAT: System Resource Affinity Table (SRAT)
typedef struct ACPI_SRAT
{
	DESC_HEADER Header;
	UINT32 TableRevision;
	UINT32 Reserved[2];
} ACPI_SRAT;

typedef enum
{
	SRAT_ACPI_DEV_HANDLE,
	SRAT_PCI_DEV_HANDLE
} SRAT_DEVICE_HANDLE_TYPE;

typedef struct SRAT_ENTRY
{
	UINT8 Type;
	UINT8 Length;
	union
	{
		struct
		{
			UINT8 ProximityDomainLow;
			UINT8 ApicId;
			struct
			{
				UINT32 Enabled : 1;
				UINT32 Reserved : 31;
			} Flags;
			UINT8 SApicEid;
			UINT8 ProximityDomainHigh[3];
			UINT32 ClockDomain;
		} ApicAffinity;
		struct
		{
			UINT32 ProximityDomain;
			UINT8 Reserved[2];
			UINT64 Base;
			UINT64 Length;
			UINT32 Reserved2;
			struct
			{
				UINT32 Enabled : 1;
				UINT32 HotPlug : 1;
				UINT32 NonVolatile : 1;
				UINT32 SpecificPurpose : 1;
				UINT32 Reserved : 28;
			} Flags;
			UINT8 Reserved3[8];
		} MemoryAffinity;
		struct
		{
			UINT8 Reserved[2];
			UINT32 ProximityDomain;
			UINT32 ApicId;
			struct
			{
				UINT32 Enabled : 1;
				UINT32 Reserved : 31;
			} Flags;
			UINT32 ClockDomain;
			UINT32 Reserved2;
		} X2ApicAffinity;
		struct
		{
			UINT32 ProximityDomain;
			UINT32 ProcessorUid;
			struct
			{
				UINT32 Enabled : 1;
				UINT32 Reserved : 31;
			} Flags;
			UINT32 ClockDomain;
		} GiccAffinity;
		struct
		{
			UINT32 ProximityDomain;
			UINT8 Reserved[2];
			UINT32 ITSID;
		} GicItsAffinity;
		struct
		{
			UINT8 Reserved;
			UINT8 DeviceHandleType;
			UINT32 ProximityDomain;
			union
			{
				struct
				{
					UINT8 ACPI_HID[8];
					UINT32 ACPI_UID;
					UINT32 Reserved;
				} ACPI;
				struct
				{
					UINT16 PCISegment;
					UINT16 PCIBDFNumber;
					UINT8 Reserved[12];
				} PCI;
			} DeviceHandle;
			struct
			{
				UINT32 Enabled : 1;
				UINT32 ArchitecturalTransactions : 1;
				UINT32 Reserved : 30;
			} Flags;
			UINT32 Reserved2;
		} GenericInitiatorAffinity;
		struct
		{
			UINT8 Reserved;
			UINT8 DeviceHandleType;
			UINT32 ProximityDomain;
			union
			{
				struct
				{
					UINT8 ACPI_HID[8];
					UINT32 ACPI_UID;
					UINT32 Reserved;
				} ACPI;
				struct
				{
					UINT16 PCISegment;
					UINT16 PCIBDFNumber;
					UINT8 Reserved[12];
				} PCI;
			} DeviceHandle;
			struct
			{
				UINT32 Enabled : 1;
				UINT32 ArchitecturalTransactions : 1;
				UINT32 Reserved : 30;
			} Flags;
			UINT32 Reserved2;
		} GenericPortAffinity;
	} DUMMYUNIONNAME;
} SRAT_ENTRY;

#define SRAT_APIC_ENTRY_LENGTH                       \
    (FIELD_OFFSET(SRAT_ENTRY, ApicAffinity) +   \
     RTL_FIELD_SIZE(SRAT_ENTRY, ApicAffinity))

#define SRAT_MEMORY_ENTRY_LENGTH                     \
    (FIELD_OFFSET(SRAT_ENTRY, MemoryAffinity) + \
     RTL_FIELD_SIZE(SRAT_ENTRY, MemoryAffinity))

#define SRAT_X2APIC_ENTRY_LENGTH                     \
    (FIELD_OFFSET(SRAT_ENTRY, X2ApicAffinity) + \
     RTL_FIELD_SIZE(SRAT_ENTRY, X2ApicAffinity))

#define SRAT_GICC_ENTRY_LENGTH                     \
    (FIELD_OFFSET(SRAT_ENTRY, GiccAffinity) + \
     RTL_FIELD_SIZE(SRAT_ENTRY, GiccAffinity))

#define SRAT_GENERIC_PORT_ENTRY_LENGTH              \
    (FIELD_OFFSET(SRAT_ENTRY, GenericPortAffinity) + \
    RTL_FIELD_SIZE(SRAT_ENTRY, GenericPortAffinity))

#define PROXIMITY_DOMAIN(SratTable, SratEntry) \
    (((SratTable)->Header.Revision == 1) ? \
     PROXIMITY_DOMAIN_1(SratEntry) : PROXIMITY_DOMAIN_2(SratEntry))

#define PROXIMITY_DOMAIN_1(SratEntry) \
    (SratEntry)->ApicAffinity.ProximityDomainLow

#define PROXIMITY_DOMAIN_2(SratEntry) \
    (((SratEntry)->Type == SratProcessorLocalAPIC) ? \
     (((UINT32)((SratEntry)->ApicAffinity.ProximityDomainLow)) + \
      (((UINT32)((SratEntry)->ApicAffinity.ProximityDomainHigh[0])) << 8) + \
      (((UINT32)((SratEntry)->ApicAffinity.ProximityDomainHigh[1])) << 16) + \
      (((UINT32)((SratEntry)->ApicAffinity.ProximityDomainHigh[2])) << 24)) : \
     (((SratEntry)->Type == SratProcessorLocalX2APIC) ? \
      (SratEntry)->X2ApicAffinity.ProximityDomain : \
      (((SratEntry)->Type == SratMemory) ? \
       (SratEntry)->MemoryAffinity.ProximityDomain : \
       (SratEntry)->GiccAffinity.ProximityDomain)))

typedef enum
{
	SRAT_PROC_LOCAL_APIC,
	SRAT_Memory,
	SRAT_PROC_LOCAL_X2APIC,
	SRAT_GICC,
	SRAT_GIC_ITS,
	SRAT_GENERIC_INITIATOR,
	SRAT_GENERIC_PORT
} SRAT_ENTRY_TYPE;

// SSDT: Secondary System Description Table (SSDT)
#if 0
typedef struct ACPI_SSDT
{
	DESC_HEADER Header;
	BYTE DefBlock[ANYSIZE_ARRAY]; // AML code
} ACPI_SSDT;
#else
typedef ACPI_DSDT ACPI_SSDT;
#endif

// _STA: _STA Override Table (STAO)
typedef struct ACPI_STAO
{
	DESC_HEADER Header;
	UINT8 Uart;
	CHAR NameList[ANYSIZE_ARRAY]; // Null-terminated list of names
} ACPI_STAO;

// SVKL: Storage Volume Key Data Table (SVKL)
// NOT SUPPORTED

// SWFT: Sound Wire File Table (SWFT)
// NOT SUPPORTED

// TPM1: TCG Hardware Interface Table (TCPA)
// TPM 1.2
typedef struct TCPA_CLIENT
{
	DESC_HEADER Header;
	UINT16 PlatformClass;
	UINT32 LAML;
	UINT64 LASA;
} TCPA_CLIENT;

typedef struct TCPA_SERVER
{
	DESC_HEADER Header;
	UINT16 PlatformClass;
	UINT16 Reserved1;
	UINT64 LAML;
	UINT64 LASA;
	UINT16 SpecificationRevision;
	UINT8 DeviceFlags;
	UINT8 InterruptFlags;
	UINT8 GPE;
	UINT8 Reserved2[3];
	UINT32 GlobalSystemInterupt;
	GEN_ADDR BaseAddress;
	UINT32 Reserved3;
	GEN_ADDR ConfigurationAddress;
	UINT8 PCISegmentGroupNumber;
	UINT8 PCIBusNumber;
	UINT8 PCIDeviceNumber;
	UINT8 PCIFunctionNumber;
} TCPA_SERVER;

typedef struct ACPI_TCPA
{
	union
	{
		TCPA_CLIENT ClientTable;
		TCPA_SERVER ServerTable;
	} u;
} ACPI_TCPA;

// TDEL: TD Event Log Table (TDEL)
// NOT SUPPORTED

// TPM2: Trusted Platform Module 2 Table (TPM2)
typedef struct ACPI_TPM2
{
	DESC_HEADER Header;
	union
	{
		struct
		{
			UINT32 UseMemoryDescriptors : 1;
			UINT32 CmdListCapable : 1;
			UINT32 NoDeviceIO : 1;
			UINT32 DeviceMemory : 1;
			UINT32 DevMemOnly : 1;
			UINT32 Reserved : 27;
		} Flags;
		UINT32 FlagBits;
	} u;
	UINT64 ControlAreaPA;
	UINT32 StartMethod;
	UINT32 PlatformParameters[8];
} ACPI_TPM2;

typedef enum
{
	TPM2_START_METHOD_INVALID = 0,
	TPM2_START_METHOD_SIM = 1,
	TPM2_START_METHOD_ACPI = 2,
	TPM2_START_METHOD_TZ1 = 3,
	TPM2_START_METHOD_TZ2 = 4,
	TPM2_START_METHOD_TZ3 = 5,
	TPM2_START_METHOD_TIS13 = 6,
	TPM2_START_METHOD_CR = 7,
	TPM2_START_METHOD_CR_WITH_ACPI = 8,
	TPM2_START_METHOD_CR_TREE = 9,
	TPM2_START_METHOD_SPB = 10,
	TPM2_START_METHOD_CR_SMC = 11,
	TPM2_START_METHOD_FIFO_I2C = 12,
	TPM2_START_METHOD_CR_HSP = 13,
	TPM2_START_METHOD_CR_SPU = 14,
} TPM20_START_METHOD;

// UEFI: UEFI ACPI Data Table (UEFI)
typedef struct ACPI_UEFI
{
	DESC_HEADER Header;
	GUID Id;
	UINT16 DataOffset;
	UINT8 Data[ANYSIZE_ARRAY];
} ACPI_UEFI;

// VFCT: AMD Video BIOS Firmware Content Table (VFCT)
typedef struct ACPI_VFCT
{
	DESC_HEADER Header;
	GUID Id;
	UINT32 VbiosImgOffset;
	UINT32 Lib1ImgOffset;
	UINT32 Reserved[4];
} ACPI_VFCT;

// VIOT: Virtual I/O Translation Table (VIOT)
// TODO
typedef struct ACPI_VIOT
{
	DESC_HEADER Header;
	UINT16 NodeCount;
	UINT16 NodeOffset;
	UINT8 Reserved[8];
} ACPI_VIOT;

// VENTOY: Ventoy OS Param (VTOY)
typedef struct ACPI_VTOY
{
	DESC_HEADER Header;
	// ventoy_os_param
	GUID Guid;
	UINT8 Checksum;
	UINT8 DiskGuid[16];
	UINT64 DiskSize;
	UINT16 DiskPartId; // from 1
	UINT16 DiskPartType; // 0 = exFAT, 1 = NTFS
	CHAR ImgPath[384];
	UINT64 ImgSize;
	UINT64 ImgLocationAddr;
	UINT32 ImgLocationLen;
	UINT8 Reserved1[32];
	UINT8 DiskSignature[4];
	UINT8 Reserved2[27];
} ACPI_VTOY;

// WAET: Windows ACPI Emulated Devices Table (WAET)
typedef struct ACPI_WAET
{
	DESC_HEADER Header;
	UINT32 EmulatedDeviceFlags;
} ACPI_WAET;

#define WAET_DEV_RTC_ENLIGHTENED_BIT    0
#define WAET_DEV_RTC_ENLIGHTENED        (1 << WAET_DEV_RTC_ENLIGHTENED_BIT)

#define WAET_DEV_PMTMR_GOOD_BIT 1
#define WAET_DEV_PMTMR_GOOD     (1 << WAET_DEV_PMTMR_GOOD_BIT)

#define WAET_REV_0_SIZE RTL_SIZEOF_THROUGH_FIELD(WAET, EmulatedDeviceFlags)

// WDAT: Microsoft Hardware Watch Dog Action Table. (WDAT)
typedef struct ACPI_WDAT
{
	DESC_HEADER Header;
	// Watchdog Header Start
	UINT32 WatchdogHeaderLength;
	UINT16 PciSegment;
	UINT8 PciBusNum;
	UINT8 PciDevNum;
	UINT8 PciFnNum;
	UINT8 Reserved1[3];
	UINT32 TimerPeriod; // in milliseconds
	UINT32 MaxCount;
	UINT32 MinCount;
	UINT8 Flags;
	UINT8 Reserved2[3];
	UINT32 EntryCount;
	// WDAT_ENTRY Entry[];
} ACPI_WDAT;

#define WDAT_FLAG_ENABLED           0x01
#define WDAT_FLAG_STOPPED           0x80

typedef struct WDAT_ENTRY
{
	UINT8 Action;
	UINT8 Instruction;
	UINT8 Reserved[2];
	GEN_ADDR RegisterRegion;
	UINT32 Value;
	UINT32 Mask;
} WDAT_ENTRY;

enum
{
	WDAT_ACT_RESET = 1,
	WDAT_ACT_GET_CURRENT_COUNTDOWN = 4,
	WDAT_ACT_GET_COUNTDOWN = 5,
	WDAT_ACT_SET_COUNTDOWN = 6,
	WDAT_ACT_GET_RUNNING_STATE = 8,
	WDAT_ACT_SET_RUNNING_STATE = 9,
	WDAT_ACT_GET_STOPPED_STATE = 10,
	WDAT_ACT_SET_STOPPED_STATE = 11,
	WDAT_ACT_GET_REBOOT = 16,
	WDAT_ACT_SET_REBOOT = 17,
	WDAT_ACT_GET_SHUTDOWN = 18,
	WDAT_ACT_SET_SHUTDOWN = 19,
	WDAT_ACT_GET_STATUS = 32,
	WDAT_ACT_SET_STATUS = 33,
	WDAT_ACT_ACTION_RESERVED = 34
} WDAT_ACTIONS;

enum
{
	WDAT_INSTRUCTION_READ_VALUE = 0,
	WDAT_INSTRUCTION_READ_COUNTDOWN = 1,
	WDAT_INSTRUCTION_WRITE_VALUE = 2,
	WDAT_INSTRUCTION_WRITE_COUNTDOWN = 3,
	WDAT_INSTRUCTION_INSTRUCTION_RESERVED = 4,
	WDAT_INSTRUCTION_PRESERVE_REGISTER = 0x80
} WDAT_INSTRUCTIONS;

// WDDT: Intel Watchdog Descriptor Table (WDDT)
typedef struct ACPI_WDDT
{
	DESC_HEADER Header;
	UINT16 TcoSpecVersion;
	UINT16 TcoDescTableVersion;
	UINT16 PciVendorId;
	GEN_ADDR TcoBaseAddr;
	UINT16 TimerMaxCount;
	UINT16 TimerMinCount;
	UINT16 TimerCountPeriod; // in milliseconds
	UINT16 Status;
	UINT16 Capability;
} ACPI_WDDT;

// WDRT: Windows 2k3 Watchdog Resource Table (WDRT)
typedef struct ACPI_WDRT
{
	DESC_HEADER Header;
	GEN_ADDR CtrlRegister;
	GEN_ADDR CountRegister;
	UINT16 PciDeviceId;
	UINT16 PciVendorId;
	UINT8 PciBusNum;
	UINT8 PciDevNum;
	UINT8 PciFnNum;
	UINT8 PciSegment;
	UINT16 MaxCount;
	UINT8 Units;
} ACPI_WDRT;

// WPBT: Windows Platform Binary Table (WPBT)
typedef struct ACPI_WPBT
{
	DESC_HEADER Header;
	UINT32 HandoffMemorySize;
	UINT64 HandoffMemoryLocation;
	UINT8 ContentLayout; // 1 = PE
	UINT8 ContentType; // 1 = Native
	UINT16 CommandLineArgumentsLength;
	WCHAR CommandLineArguments[ANYSIZE_ARRAY];
} ACPI_WPBT;

#define WPBT_MIN_SIZE              52
#define WPBT_BOOT_SEARCH_START     0x1000
#define WPBT_BOOT_SEARCH_END       0xA0000
#define WPBT_BOOT_SEARCH_INCREMENT 0x40

// Windows SMM Security Mitigations Table (WSMT)
typedef union WSMT_PROTECTION_FLAGS
{
	UINT32 AsUlong;
	struct
	{
		UINT32 FixedCommBuffers : 1;
		UINT32 CommBufferNestedPtrProtection : 1;
		UINT32 SystemResourceProtection : 1;
		UINT32 Reserved : 29;
	} DUMMYSTRUCTNAME;
} WSMT_PROTECTION_FLAGS;

typedef struct ACPI_WSMT
{
	DESC_HEADER Header;
	WSMT_PROTECTION_FLAGS ProtectionFlags;
} ACPI_WSMT;

// XENV: Xen Environment Table (XENV)
typedef struct ACPI_XENV
{
	DESC_HEADER Header;
	UINT64 GrantTableAddr;
	UINT64 GrantTableSize;
	UINT32 EventInterrupt;
	UINT8 Flags;
} ACPI_XENV;

#pragma pack()
