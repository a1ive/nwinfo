// SPDX-License-Identifier: Unlicense
#pragma once

#define VC_EXTRALEAN
#include <windows.h>

#pragma pack(1)

#define RSDP_SIGNATURE "RSD PTR "
#define RSDP_SIGNATURE_SIZE 8

struct acpi_rsdp_v1
{
	UINT8 signature[RSDP_SIGNATURE_SIZE];
	UINT8 checksum;
	UINT8 oemid[6];
	UINT8 revision;
	UINT32 rsdt_addr;
};

struct acpi_rsdp_v2
{
	struct acpi_rsdp_v1 rsdpv1;
	UINT32 length;
	UINT64 xsdt_addr;
	UINT8 checksum;
	UINT8 reserved[3];
};

struct acpi_table_header
{
	UINT8 signature[4];
	UINT32 length;
	UINT8 revision;
	UINT8 checksum;
	UINT8 oemid[6];
	UINT8 oemtable[8];
	UINT32 oemrev;
	UINT8 creator_id[4];
	UINT32 creator_rev;
};

struct acpi_rsdt
{
	struct acpi_table_header header;
	UINT32 entry[0];
};

struct acpi_xsdt
{
	struct acpi_table_header header;
	UINT64 entry[0];
};

// Microsoft Data Management table structure
struct acpi_msdm
{
	struct acpi_table_header header;
	UINT32 version;
	UINT32 reserved;
	UINT32 data_type;
	UINT32 data_reserved;
	UINT32 data_length;
	CHAR data[29];
};

struct acpi_madt_entry_header
{
	UINT8 type;
	UINT8 len;
};

struct acpi_madt
{
	struct acpi_table_header header;
	UINT32 lapic_addr;
	UINT32 flags;
	struct acpi_madt_entry_header entries[0];
};

struct acpi_bgrt
{
	struct acpi_table_header header;
	// 2-bytes (16 bit) version ID. This value must be 1.
	UINT16 version;
	// 1-byte status field indicating current status about the table.
	// Bits[7:1] = Reserved (must be zero)
	// Bit [0] = Valid. A one indicates the boot image graphic is valid.
	UINT8 status;
	// 0 = Bitmap
	// 1 - 255  Reserved (for future use)
	UINT8 type;
	// physical address pointing to the firmware's in-memory copy of the image.
	UINT64 addr;
	// (X, Y) display offset of the top left corner of the boot image.
	// The top left corner of the display is at offset (0, 0).
	UINT32 x;
	UINT32 y;
};

struct acpi_wpbt
{
	struct acpi_table_header header;
	/* The size of the handoff memory buffer
	 * containing a platform binary.*/
	UINT32 binary_size;
	/* The 64-bit physical address of a memory
	 * buffer containing a platform binary. */
	UINT64 binary_addr;
	/* Description of the layout of the handoff memory buffer.
	 * Possible values include:
	 * 1 - Image location points to a single
	 *     Portable Executable (PE) image at
	 *     offset 0 of the specified memory
	 *     location. The image is a flat image
	 *     where sections have not been expanded
	 *     and relocations have not been applied.
	 */
	UINT8 content_layout;
	/* Description of the content of the binary
	 * image and the usage model of the
	 * platform binary. Possible values include:
	 * 1 - The platform binary is a native usermode application
	 *     that should be executed by the Windows Session
	 *     Manager during operating system initialization.
	 */
	UINT8 content_type;
	UINT16 cmdline_length;
	UINT16 cmdline[0];
};

struct acpi_gas
{
	UINT8 address_space;
	UINT8 bit_width;
	UINT8 bit_offset;
	UINT8 access_size;
	UINT64 address;
};

struct acpi_fadt
{
	struct acpi_table_header header;
	UINT32 facs_addr;
	UINT32 dsdt_addr;
	UINT8  reserved; //INT_MODEL
	UINT8  preferred_pm_profile;
	UINT16 sci_int;
	UINT32 smi_cmd;
	UINT8  acpi_enable;
	UINT8  acpi_disable;
	UINT8  s4bios_req;
	UINT8  pstate_cnt;
	UINT32 pm1a_evt_blk;
	UINT32 pm1b_evt_blk;
	UINT32 pm1a_cnt_blk;
	UINT32 pm1b_cnt_blk;
	UINT32 pm2_cnt_blk;
	UINT32 pm_tmr_blk;
	UINT32 gpe0_blk;
	UINT32 gpe1_blk;
	UINT8  pm1__evt_len;
	UINT8  pm1_cnt_len;
	UINT8  pm2_cnt_len;
	UINT8  pm_tmr_len;
	UINT8  gpe0_len;
	UINT8  gpe1_len;
	UINT8  gpe1_base;
	UINT8  cst_cnt;
	UINT16 p_lvl2_lat;
	UINT16 p_lvl3_lat;
	UINT16 flush_size;
	UINT16 flush_stride;
	UINT8  duty_offset;
	UINT8  duty_width;
	UINT8  day_alarm;
	UINT8  month_alarm;
	UINT8  century;
	// reserved in ACPI 1.0; used since ACPI 2.0+
	UINT16 iapc_boot_arch;
	UINT8  reserved2;
	UINT32 flags;
	struct acpi_gas reset_reg;
	UINT8  reset_value;
	UINT16 arm_boot_arch;
	UINT8  fadt_minor_ver;
	// 64bit pointers - Available on ACPI 2.0+
	UINT64 facs_xaddr;
	UINT64 dsdt_xaddr;
	struct acpi_gas x_pm1a_evt_blk;
	struct acpi_gas x_pm1b_evt_blk;
	struct acpi_gas x_pm1a_cnt_blk;
	struct acpi_gas x_pm1b_cnt_blk;
	struct acpi_gas x_pm2_cnt_blk;
	struct acpi_gas x_pm_tmr_blk;
	struct acpi_gas x_gpe0_blk;
	struct acpi_gas x_gpe1_blk;
};

#pragma pack()
