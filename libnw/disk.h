// SPDX-License-Identifier: Unlicense
#pragma once

#include <windows.h>

#pragma pack(1)

struct mbr_entry
{
	/* If active, 0x80, otherwise, 0x00.  */
	UINT8 flag;
	/* The head of the start.  */
	UINT8 start_head;
	/* (S | ((C >> 2) & 0xC0)) where S is the sector of the start and C
	   is the cylinder of the start. Note that S is counted from one.  */
	UINT8 start_sector;
	/* (C & 0xFF) where C is the cylinder of the start.  */
	UINT8 start_cylinder;
	/* The partition type.  */
	UINT8 type;
	/* The end versions of start_head, start_sector and start_cylinder,
	   respectively.  */
	UINT8 end_head;
	UINT8 end_sector;
	UINT8 end_cylinder;
	/* The start sector. Note that this is counted from zero.  */
	UINT32 start;
	/* The length in sector units.  */
	UINT32 length;
};

struct mbr_header
{
	CHAR dummy1[11];/* normally there is a short JMP instuction(opcode is 0xEB) */
	UINT16 bytes_per_sector;/* seems always to be 512, so we just use 512 */
	UINT8 sectors_per_cluster;/* non-zero, the power of 2, i.e., 2^n */
	UINT16 reserved_sectors;/* FAT=non-zero, NTFS=0? */
	UINT8 number_of_fats;/* NTFS=0; FAT=1 or 2  */
	UINT16 root_dir_entries;/* FAT32=0, NTFS=0, FAT12/16=non-zero */
	UINT16 total_sectors_short;/* FAT32=0, NTFS=0, FAT12/16=any */
	UINT8 media_descriptor;/* range from 0xf0 to 0xff */
	UINT16 sectors_per_fat;/* FAT32=0, NTFS=0, FAT12/16=non-zero */
	UINT16 sectors_per_track;/* range from 1 to 63 */
	UINT16 total_heads;/* range from 1 to 256 */
	UINT32 hidden_sectors;/* any value */
	UINT32 total_sectors_long;/* FAT32=non-zero, NTFS=0, FAT12/16=any */
	UINT32 sectors_per_fat32;/* FAT32=non-zero, NTFS=any, FAT12/16=any */
	UINT64 total_sectors_long_long;/* NTFS=non-zero, FAT12/16/32=any */
	CHAR dummy2[392];
	UINT8 unique_signature[4];
	UINT8 unknown[2];

	/* Four partition entries.  */
	struct mbr_entry entries[4];

	/* The signature 0xaa55.  */
	UINT16 signature;
};

struct gpt_header
{
	UINT8 magic[8];
	UINT32 version;
	UINT32 headersize;
	UINT32 crc32;
	UINT32 unused1;
	UINT64 header_lba;
	UINT64 alternate_lba;
	UINT64 start;
	UINT64 end;
	UINT8 guid[16];
	UINT64 partitions;
	UINT32 maxpart;
	UINT32 partentry_size;
	UINT32 partentry_crc32;
};

#pragma pack()

typedef struct _PHY_DRIVE_INFO
{
	INT PartMap; // 0:UNKNOWN 1:MBR 2:GPT 3:ISO
	UINT64 SizeInBytes;
	BYTE DeviceType;
	BOOL RemovableMedia;
	CHAR* HwID;
	CHAR VendorId[MAX_PATH];
	CHAR ProductId[MAX_PATH];
	CHAR ProductRev[MAX_PATH];
	CHAR SerialNumber[MAX_PATH];
	STORAGE_BUS_TYPE BusType;
	// MBR
	UCHAR MbrSignature[4];
	DWORD MbrLba[4];
	// GPT
	UCHAR GptGuid[16];

	DWORD VolumeCount;
	CHAR Volumes[32][MAX_PATH];
}PHY_DRIVE_INFO;

VOID NWL_GetDiskProtocolSpecificInfo(PNODE pNode, DWORD dwIndex, STORAGE_BUS_TYPE busType);
