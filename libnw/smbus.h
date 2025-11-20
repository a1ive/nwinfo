// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <winring0.h>

#define SM_OK             0
#define SM_ERR_GENERIC    -1
#define SM_ERR_NO_DEVICE  -2
#define SM_ERR_TIMEOUT    -3
#define SM_ERR_BUS_ERROR  -4
#define SM_ERR_PARAM      -5

#define I2C_SMBUS_READ              1
#define I2C_SMBUS_WRITE             0

#define I2C_SMBUS_BLOCK_MAX         32

union i2c_smbus_data
{
	uint8_t u8data;
	uint16_t u16data;
	uint8_t block[I2C_SMBUS_BLOCK_MAX + 2]; /* block[0] is used for length */
};

// addr, rw
#define I2C_SMBUS_QUICK             0
// addr, rw, [data-w]
#define I2C_SMBUS_BYTE              1
#define I2C_SMBUS_BYTE_DATA         2
#define I2C_SMBUS_WORD_DATA         3
// addr, data
#define I2C_SMBUS_PROC_CALL         4
// addr, rw, data
#define I2C_SMBUS_BLOCK_DATA        5
// -
#define I2C_SMBUS_I2C_BLOCK_BROKEN  6
// addr, data
#define I2C_SMBUS_BLOCK_PROC_CALL   7
// -
#define I2C_SMBUS_I2C_BLOCK_DATA    8

struct smbus_controller;
typedef struct smbus_controller smctrl_t;
struct smbus_context;
typedef struct smbus_context smbus_t;

struct smbus_controller
{
	const char* name;
	int (*detect)(smbus_t* ctx);
	int (*init)(smbus_t* ctx);
	uint64_t (*get_clock)(smbus_t* ctx);
	int (*set_clock)(smbus_t* ctx, uint64_t freq);
	int (*xfer)(smbus_t* ctx, uint8_t addr, uint8_t read_write, uint8_t command, uint8_t protocol, union i2c_smbus_data* data);
};

struct smbus_context
{
	struct wr0_drv_t* drv;
	const smctrl_t* ctrl;
	uint32_t base_addr;
	uint32_t pci_addr;
	uint32_t pci_id;
	uint8_t rev_id;
	bool spd_wd;
	bool smbus_pec;
	bool block_read;
	uint8_t spd_type;
	uint8_t spd_page;
	uint8_t last_dimm_addr;
};

smbus_t* SM_Init(struct wr0_drv_t* drv);

void SM_Free(smbus_t* ctx);

int SM_WriteQuick(smbus_t* ctx, uint8_t slave_addr, uint8_t value);

int SM_ReadByte(smbus_t* ctx, uint8_t slave_addr, uint8_t* value);

int SM_WriteByte(smbus_t* ctx, uint8_t slave_addr, uint8_t value);

int SM_ReadByteData(smbus_t* ctx, uint8_t slave_addr, uint8_t offset, uint8_t* value);

int SM_WriteByteData(smbus_t* ctx, uint8_t slave_addr, uint8_t offset, uint8_t value);

int SM_ReadWordData(smbus_t* ctx, uint8_t slave_addr, uint8_t offset, uint16_t* value);

int SM_WriteWordData(smbus_t* ctx, uint8_t slave_addr, uint8_t offset, uint16_t value);

int SM_ProcCall(smbus_t* ctx, uint8_t slave_addr, uint8_t offset, uint16_t* value);

#define SMBUS_DBG(...) \
	do \
	{ \
		if (ctx->drv->debug) \
		{ \
			printf("[SM] " __VA_ARGS__); \
			puts(""); \
		} \
	} while (0)

#define SMBUS_BASE_CLASS        0x0C
#define SMBUS_SUB_CLASS         0x05
#define SMBUS_PROG_IF           0x00

#define SPD_MEMORY_TYPE_OFFSET  2

#define MEM_TYPE_FPM_DRAM       0x01
#define MEM_TYPE_EDO            0x02
#define MEM_TYPE_PNEDO          0x03
#define MEM_TYPE_SDRAM          0x04
#define MEM_TYPE_ROM            0x05
#define MEM_TYPE_SGRAM          0x06
#define MEM_TYPE_DDR            0x07
#define MEM_TYPE_DDR2           0x08
#define MEM_TYPE_DDR2_FB        0x09
#define MEM_TYPE_DDR2_FB_P      0x0A
#define MEM_TYPE_DDR3           0x0B
#define MEM_TYPE_DDR4           0x0C
#define MEM_TYPE_DDR4E          0x0E
#define MEM_TYPE_LPDDR3         0x0F
#define MEM_TYPE_LPDDR4         0x10
#define MEM_TYPE_LPDDR4X        0x11
#define MEM_TYPE_DDR5           0x12
#define MEM_TYPE_LPDDR5         0x13
#define MEM_TYPE_LPDDR5X        0x15
#define MEM_TYPE_UNKNOWN        0xFF

#define SPD_MAX_SIZE            1024
#define SPD_MAX_SLOT            8

#define SPD_DDR4_PAGE_SHIFT     8
#define SPD_DDR4_PAGE_MASK      0xFF
#define SPD_DDR4_ADDR_PAGE      0x36
#define SPD_DDR4_PAGE_MAX       1

#define SPD_DDR5_PAGE_SHIFT     7
#define SPD_DDR5_PAGE_MASK      0x7F
#define SPD_DDR5_PAGE_MAX       7
#define SPD5_HUB_ID_MSB         0x00    // MR0 - Device Type MSB
#define SPD5_HUB_ID_LSB         0x01    // MR1 - Device Type LSB (0x18 = w/ Temp Sensor)
#define SPD5_HUB_CAP            0x05    // MR5 - Device Capability (Bit 1 = Temp Sensor Support)
#define SPD5_HUB_I2C_CONF       0x0B    // MR11 - I2C Legacy Mode Device Configuration
#define SPD5_HUB_CONF           0x12    // MR18 - General Device Configuration
#define SPD5_HUB_TS_CONF        0x1A    // MR26 - Temperature Sensor Configuration (Bit 0 = 1 for Disable)
#define SPD5_HUB_TS_RES         0x24    // MR36 - TS Resolution (from 9- to 12-bit)
#define SPD5_HUB_STATUS         0x30    // MR48 - Device Status
#define SPD5_HUB_TS_LSB         0x31    // MR49 - TS Current Sensed Temperature - Low Byte
#define SPD5_HUB_TS_MSB         0x32    // MR50 - TS Current Sensed Temperature - High Byte
#define SPD5_HUB_TS_STATUS      0x33    // MR51 - Temperature Sensor Status

// slave_addr = SPD_SLABE_ADDR_BASE + dimm_index
#define SPD_SLABE_ADDR_BASE     0x50

bool SM_DDR4_IsAvailable(smbus_t* ctx, uint8_t slave_addr);
bool SM_DDR5_IsAvailable(smbus_t* ctx, uint8_t slave_addr);

int SM_DDR4_ReadByteAt(smbus_t* ctx, uint8_t slave_addr, uint16_t address, uint8_t* value);
int SM_DDR5_ReadByteAt(smbus_t* ctx, uint8_t slave_addr, uint16_t address, uint8_t* value);

bool SM_DDR4_IsThermalSensorPresent(smbus_t* ctx, uint8_t slave_addr);
bool SM_DDR5_IsThermalSensorPresent(smbus_t* ctx, uint8_t slave_addr);

float SM_DDR4_GetTemperature(smbus_t* ctx, uint8_t slave_addr);
float SM_DDR5_GetTemperature(smbus_t* ctx, uint8_t slave_addr);

int SM_GetSpd(smbus_t* ctx, uint8_t dimm_index, uint8_t data[SPD_MAX_SIZE]);
