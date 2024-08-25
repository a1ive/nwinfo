// SPDX-License-Identifier: Unlicense

#pragma once

#include <stddef.h>
#define VC_EXTRALEAN
#include <windows.h>

#pragma pack(1)

typedef WCHAR CHAR16;
typedef CHAR  CHAR8;

typedef struct
{
	UINT8    Type;
	///< 0x01 Hardware Device Path.
	///< 0x02 ACPI Device Path.
	///< 0x03 Messaging Device Path.
	///< 0x04 Media Device Path.
	///< 0x05 BIOS Boot Specification Device Path.
	///< 0x7F End of Hardware Device Path.

	UINT8    SubType;
	///< Varies by Type
	///< 0xFF End Entire Device Path, or
	///< 0x01 End This Instance of a Device Path and start a new
	///< Device Path.

	UINT8    Length[2];
	///< Specific Device Path data. Type and Sub-Type define
	///< type of data. Size of data is included in Length.
} EFI_DEVICE_PATH_PROTOCOL;

///
/// Device Path protocol definition for backward-compatible with EFI1.1.
///
typedef EFI_DEVICE_PATH_PROTOCOL EFI_DEVICE_PATH;

///
/// Device Type
///
#define HARDWARE_DEVICE_PATH              0x01
#define ACPI_DEVICE_PATH                  0x02
#define MESSAGING_DEVICE_PATH             0x03
#define MEDIA_DEVICE_PATH                 0x04 // => This
#define BBS_DEVICE_PATH                   0x05
#define END_DEVICE_PATH_TYPE              0x7f

///
/// Media Device SubType
///
#define MEDIA_HARDDRIVE_DP              0x01 // => HD
#define MEDIA_CDROM_DP                  0x02 // => CD
#define MEDIA_VENDOR_DP                 0x03
#define MEDIA_FILEPATH_DP               0x04 // => FILE
#define MEDIA_PROTOCOL_DP               0x05
#define MEDIA_PIWG_FW_FILE_DP           0x06
#define MEDIA_PIWG_FW_VOL_DP            0x07
#define MEDIA_RELATIVE_OFFSET_RANGE_DP  0x08
#define MEDIA_RAM_DISK_DP               0x09

typedef struct
{
	EFI_DEVICE_PATH_PROTOCOL    Header;
	///
	/// Describes the entry in a partition table, starting with entry 1.
	/// Partition number zero represents the entire device. Valid
	/// partition numbers for a MBR partition are [1, 4]. Valid
	/// partition numbers for a GPT partition are [1, NumberOfPartitionEntries].
	///
	UINT32                      PartitionNumber;
	///
	/// Starting LBA of the partition on the hard drive.
	///
	UINT64                      PartitionStart;
	///
	/// Size of the partition in units of Logical Blocks.
	///
	UINT64                      PartitionSize;
	///
	/// Signature unique to this partition:
	/// If SignatureType is 0, this field has to be initialized with 16 zeros.
	/// If SignatureType is 1, the MBR signature is stored in the first 4 bytes of this field.
	/// The other 12 bytes are initialized with zeros.
	/// If SignatureType is 2, this field contains a 16 byte signature.
	///
	UINT8                       Signature[16];
	///
	/// Partition Format: (Unused values reserved).
	/// 0x01 - PC-AT compatible legacy MBR.
	/// 0x02 - GUID Partition Table.
	///
	UINT8                       MBRType;
	///
	/// Type of Disk Signature: (Unused values reserved).
	/// 0x00 - No Disk Signature.
	/// 0x01 - 32-bit signature from address 0x1b8 of the type 0x01 MBR.
	/// 0x02 - GUID signature.
	///
	UINT8                       SignatureType;
} HARDDRIVE_DEVICE_PATH;

#define MBR_TYPE_PCAT                        0x01
#define MBR_TYPE_EFI_PARTITION_TABLE_HEADER  0x02

#define NO_DISK_SIGNATURE    0x00
#define SIGNATURE_TYPE_MBR   0x01
#define SIGNATURE_TYPE_GUID  0x02

typedef struct
{
	EFI_DEVICE_PATH_PROTOCOL    Header;
	///
	/// Boot Entry number from the Boot Catalog. The Initial/Default entry is defined as zero.
	///
	UINT32                      BootEntry;
	///
	/// Starting RBA of the partition on the medium. CD-ROMs use Relative logical Block Addressing.
	///
	UINT64                      PartitionStart;
	///
	/// Size of the partition in units of Blocks, also called Sectors.
	///
	UINT64                      PartitionSize;
} CDROM_DEVICE_PATH;

typedef struct
{
	EFI_DEVICE_PATH_PROTOCOL    Header;
	///
	/// A NULL-terminated Path string including directory and file names.
	///
	CHAR16                      PathName[1];
} FILEPATH_DEVICE_PATH;

#define SIZE_OF_FILEPATH_DEVICE_PATH  offsetof(FILEPATH_DEVICE_PATH,PathName)

///
/// Union of all possible Device Paths and pointers to Device Paths.
///
typedef union
{
	EFI_DEVICE_PATH_PROTOCOL                   DevPath;
	HARDDRIVE_DEVICE_PATH                      HardDrive;
	CDROM_DEVICE_PATH                          CD;
	FILEPATH_DEVICE_PATH                       FilePath;
} EFI_DEV_PATH;

typedef union
{
	EFI_DEVICE_PATH_PROTOCOL*                  DevPath;
	HARDDRIVE_DEVICE_PATH*                     HardDrive;
	CDROM_DEVICE_PATH*                         CD;
	FILEPATH_DEVICE_PATH*                      FilePath;
	UINT8*                                     Raw;
} EFI_DEV_PATH_PTR;

#define EFI_LOAD_OPTION_ACTIVE           0x00000001
#define EFI_LOAD_OPTION_FORCE_RECONNECT  0x00000002
#define EFI_LOAD_OPTION_HIDDEN           0x00000008
#define EFI_LOAD_OPTION_CATEGORY         0x00001F00

#define EFI_LOAD_OPTION_CATEGORY_BOOT    0x00000000
#define EFI_LOAD_OPTION_CATEGORY_APP     0x00000100

typedef struct _EFI_LOAD_OPTION
{
	UINT32 Attributes;
	UINT16 FilePathListLength;
	WCHAR Description[ANYSIZE_ARRAY];
	// EFI_DEVICE_PATH FilePathList[];
	// UINT8 OptionalData[];
} EFI_LOAD_OPTION;

#pragma pack()

#define END_ENTIRE_DEVICE_PATH_SUBTYPE    0xFF
#define END_INSTANCE_DEVICE_PATH_SUBTYPE  0x01

#define EFI_GET_DP_NODE_TYPE(x) \
	((x)->Type & 0x7f)
#define EFI_GET_DP_NODE_SUBTYPE(x) \
	((x)->SubType)
#define EFI_GET_DP_NODE_LENGTH(x) \
	((x)->Length[0] | ((x)->Length[1] << 8))
#define EFI_IS_DP_NODE_VALID(x) \
	((x) != NULL && EFI_GET_DP_NODE_LENGTH(x) >= 4)
#define EFI_IS_END_ENTIRE_DP(x) \
	(!EFI_IS_DP_NODE_VALID (x) || \
	 ( \
	  (EFI_GET_DP_NODE_TYPE (x) == END_DEVICE_PATH_TYPE) && \
	  (EFI_GET_DP_NODE_SUBTYPE (x) == END_ENTIRE_DEVICE_PATH_SUBTYPE) \
	 ) \
	)
#define EFI_GET_NEXT_DP_NODE(x) \
	(EFI_IS_DP_NODE_VALID (x) ? \
	 ((EFI_DEVICE_PATH *) ((PUINT8) (x) + EFI_GET_DP_NODE_LENGTH (x))) \
	 : NULL \
	)

#define EFI_VARIABLE_NON_VOLATILE                           0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS                     0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS                         0x00000004
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD                  0x00000008
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS             0x00000010
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS  0x00000020
#define EFI_VARIABLE_APPEND_WRITE                           0x00000040
#define EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS          0x00000080

#define EFI_BOOT_OPTION_SUPPORT_KEY     0x00000001
#define EFI_BOOT_OPTION_SUPPORT_APP     0x00000002
#define EFI_BOOT_OPTION_SUPPORT_SYSPREP 0x00000010
#define EFI_BOOT_OPTION_SUPPORT_COUNT   0x00000300

#define EFI_OS_INDICATIONS_BOOT_TO_FW_UI                    0x0000000000000001
#define EFI_OS_INDICATIONS_TIMESTAMP_REVOCATION             0x0000000000000002
#define EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED  0x0000000000000004
#define EFI_OS_INDICATIONS_FMP_CAPSULE_SUPPORTED            0x0000000000000008
#define EFI_OS_INDICATIONS_CAPSULE_RESULT_VAR_SUPPORTED     0x0000000000000010
#define EFI_OS_INDICATIONS_START_OS_RECOVERY                0x0000000000000020
#define EFI_OS_INDICATIONS_START_PLATFORM_RECOVERY          0x0000000000000040
#define EFI_OS_INDICATIONS_JSON_CONFIG_DATA_REFRESH         0x0000000000000080

typedef struct _VARIABLE_NAME
{
	ULONG NextEntryOffset;
	GUID VendorGuid;
	WCHAR Name[ANYSIZE_ARRAY];
} VARIABLE_NAME, * PVARIABLE_NAME;

extern GUID EFI_GV_GUID;
extern GUID EFI_EMPTY_GUID;

BOOL NWL_IsEfi(VOID);
DWORD NWL_GetEfiVar(LPCWSTR lpName, LPGUID lpGuid,
	PVOID pBuffer, DWORD nSize, PDWORD pdwAttribubutes);
VOID* NWL_GetEfiVarAlloc(LPCWSTR lpName, LPGUID lpGuid,
	PDWORD pdwSize, PDWORD pdwAttributes);
PVARIABLE_NAME NWL_EnumerateEfiVar(PULONG pulSize);
BOOL NWL_SetEfiVarEx(LPCWSTR lpName, LPGUID lpGuid,
	PVOID pBuffer, DWORD nSize, DWORD dwAttributes);
BOOL NWL_SetEfiVar(LPCWSTR lpName, LPGUID lpGuid, PVOID pBuffer, DWORD nSize);
BOOL NWL_DeleteEfiVar(LPCWSTR lpName, LPGUID lpGuid);

CHAR* NWL_GetEfiDpStr(EFI_DEVICE_PATH* pDp);
