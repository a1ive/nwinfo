// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdint.h>

#pragma pack(1)

struct fat_bpb
{
	uint8_t jmp_boot[3];
	uint8_t oem_name[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t num_reserved_sectors;
	uint8_t num_fats;
	uint16_t num_root_entries;
	uint16_t num_total_sectors_16;
	uint8_t media;
	uint16_t sectors_per_fat_16;
	uint16_t sectors_per_track;
	uint16_t num_heads;
	uint32_t num_hidden_sectors;
	uint32_t num_total_sectors_32;
	union
	{
		struct
		{
			uint8_t num_ph_drive;
			uint8_t reserved;
			uint8_t boot_sig;
			uint32_t num_serial;
			uint8_t label[11];
			uint8_t fstype[8];
		} fat12_or_fat16;
		struct
		{
			uint32_t sectors_per_fat_32;
			uint16_t extended_flags;
			uint16_t fs_version;
			uint32_t root_cluster;
			uint16_t fs_info;
			uint16_t backup_boot_sector;
			uint8_t reserved[12];
			uint8_t num_ph_drive;
			uint8_t reserved1;
			uint8_t boot_sig;
			uint32_t num_serial;
			uint8_t label[11];
			uint8_t fstype[8];
		} fat32;
	} version;
};

struct exfat_bpb
{
	uint8_t jmp_boot[3];
	uint8_t oem_name[8];
	uint8_t mbz[53];
	uint64_t num_hidden_sectors;
	uint64_t num_total_sectors;
	uint32_t num_reserved_sectors;
	uint32_t sectors_per_fat;
	uint32_t cluster_offset;
	uint32_t cluster_count;
	uint32_t root_cluster;
	uint32_t num_serial;
	uint16_t fs_revision;
	uint16_t volume_flags;
	uint8_t bytes_per_sector_shift;
	uint8_t sectors_per_cluster_shift;
	uint8_t num_fats;
	uint8_t num_ph_drive;
	uint8_t reserved[8];
};

struct ntfs_bpb
{
	uint8_t jmp_boot[3];
	uint8_t oem_name[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint8_t reserved_1[7];
	uint8_t media;
	uint16_t reserved_2;
	uint16_t sectors_per_track;
	uint16_t num_heads;
	uint32_t num_hidden_sectors;
	uint32_t reserved_3;
	uint8_t bios_drive;
	uint8_t reserved_4[3];
	uint64_t num_total_sectors;
	uint64_t mft_lcn;
	uint64_t mft_mirr_lcn;
	int8_t clusters_per_mft;
	int8_t reserved_5[3];
	int8_t clusters_per_index;
	int8_t reserved_6[3];
	uint64_t num_serial;
	uint32_t checksum;
};

union VOLUME_BOOT_RECORD
{
	uint8_t Raw[512];
	struct fat_bpb Fat;
	struct exfat_bpb Exfat;
	struct ntfs_bpb Ntfs;
};

#pragma pack()
