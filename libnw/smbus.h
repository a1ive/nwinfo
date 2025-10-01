// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdint.h>
#include <winring0.h>

#define SM_OK             0
#define SM_ERR_GENERIC   -1
#define SM_ERR_NO_DEVICE -2
#define SM_ERR_TIMEOUT   -3
#define SM_ERR_BUS_ERROR -4
#define SM_ERR_PARAM     -5

struct smbus_controller;
typedef struct smbus_controller smctrl_t;
struct smbus_context;
typedef struct smbus_context smbus_t;

struct smbus_controller
{
	const char* name;
	int (*detect)(smbus_t* ctx);
	int (*init)(smbus_t* ctx);
	int (*read_byte)(smbus_t* ctx, uint8_t slave_addr, uint8_t command, uint8_t* value);
	int (*write_byte)(smbus_t* ctx, uint8_t slave_addr, uint8_t command, uint8_t value);
};

struct smbus_context
{
	struct wr0_drv_t* drv;
	const smctrl_t* ctrl;
	uint16_t base_addr;
	uint32_t pci_addr;
	uint32_t pci_id;
	uint8_t rev_id;
	int8_t spd_page;
	uint8_t last_dimm_addr;
};

smbus_t* SM_Init(struct wr0_drv_t* drv);

void SM_Free(smbus_t* ctx);

int SM_ReadByte(smbus_t* ctx, uint8_t slave_addr, uint8_t command, uint8_t* value);

int SM_WriteByte(smbus_t* ctx, uint8_t slave_addr, uint8_t command, uint8_t value);

#define SMBUS_DBG(...) \
	do \
	{ \
		if (ctx->drv->debug) \
		{ \
			printf("[SM] " __VA_ARGS__); \
			puts(""); \
		} \
	} while (0)

#define SMBUS_BASE_CLASS  0x0C
#define SMBUS_SUB_CLASS   0x05
#define SMBUS_PROG_IF     0x00

#define I2C_WRITE   0
#define I2C_READ    1

#define SMBHSTSTS   0
#define SMBHSTCNT   2
#define SMBHSTCMD   3
#define SMBHSTADD   4
#define SMBHSTDAT0  5
#define SMBHSTDAT1  6
#define SMBBLKDAT   7
#define SMBPEC      8
#define SMBAUXSTS   12
#define SMBAUXCTL   13

#define SPD_PAGE_SELECT_DDR4_0 0x36
#define SPD_PAGE_SELECT_DDR4_1 0x37

#define SPD5_MR11 11

#define SPD_MEMORY_TYPE_OFFSET   2

#define MEM_TYPE_DDR             0x07
#define MEM_TYPE_DDR2            0x08
#define MEM_TYPE_DDR3            0x0B
#define MEM_TYPE_DDR4            0x0C
#define MEM_TYPE_DDR4E           0x0E
#define MEM_TYPE_LPDDR3          0x0F
#define MEM_TYPE_LPDDR4          0x10
#define MEM_TYPE_DDR5            0x12
#define MEM_TYPE_LPDDR5          0x13

#define SPD_MAX_SIZE             1024
#define SPD_MAX_SLOT             8

int SM_GetSpd(smbus_t* ctx, uint8_t dimm_index, uint8_t data[SPD_MAX_SIZE]);
