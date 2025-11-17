// SPDX-License-Identifier: Unlicense

#include "smbus.h"
#include <stdlib.h>
#include <stdio.h>

extern const smctrl_t i801_controller;
extern const smctrl_t piix4_controller;

static const smctrl_t* ctrl_list[] =
{
	&i801_controller,
	&piix4_controller,
};

smbus_t* SM_Init(struct wr0_drv_t* drv)
{
	if (!drv)
		return NULL;

	smbus_t* ctx = (smbus_t*)calloc(1, sizeof(smbus_t));
	if (!ctx)
		return NULL;

	ctx->drv = drv;
	ctx->spd_page = 0xFF;
	ctx->last_dimm_addr = 0xFF;
	ctx->spd_type = 0xFF;
	ctx->base_addr = 0;
	ctx->pci_addr = WR0_FindPciByClass(drv, SMBUS_BASE_CLASS, SMBUS_SUB_CLASS, SMBUS_PROG_IF, 0);
	if (ctx->pci_addr != 0xFFFFFFFF)
	{
		ctx->pci_id = WR0_RdPciConf32(drv, ctx->pci_addr, 0x00);
		ctx->rev_id = WR0_RdPciConf8(drv, ctx->pci_addr, 0x08);
	}
	else
		ctx->pci_id = 0xFFFFFFFF;
	for (int i = 0; i < ARRAYSIZE(ctrl_list); i++)
	{
		if (ctrl_list[i]->detect(ctx) == SM_OK)
		{
			SMBUS_DBG("Detected SMBus controller: %s", ctrl_list[i]->name);
			ctx->ctrl = ctrl_list[i];
			if (ctx->ctrl->init(ctx) == SM_OK)
			{
				SMBUS_DBG("Initialized SMBus controller at IO Base 0x%X", ctx->base_addr);
				return ctx;
			}
			else
			{
				SMBUS_DBG("Failed to initialize SMBus controller: %s", ctx->ctrl->name);
				free(ctx);
				return NULL;
			}
		}
	}

	SMBUS_DBG("No supported SMBus controller found");
	free(ctx);
	return NULL;
}

void SM_Free(smbus_t* ctx)
{
	if (ctx)
		free(ctx);
}

int SM_WriteQuick(smbus_t* ctx, uint8_t slave_addr, uint8_t value)
{
	if (!ctx || !ctx->ctrl || !ctx->ctrl->xfer)
		return SM_ERR_GENERIC;
	return ctx->ctrl->xfer(ctx, slave_addr, value, 0, I2C_SMBUS_QUICK, NULL);
}

int SM_ReadByte(smbus_t* ctx, uint8_t slave_addr, uint8_t* value)
{
	if (!ctx || !ctx->ctrl || !ctx->ctrl->xfer)
		return SM_ERR_GENERIC;
	union i2c_smbus_data data = { 0 };
	int rc = ctx->ctrl->xfer(ctx, slave_addr, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
	*value = data.u8data;
	return rc;
}

int SM_WriteByte(smbus_t* ctx, uint8_t slave_addr, uint8_t value)
{
	if (!ctx || !ctx->ctrl || !ctx->ctrl->xfer)
		return SM_ERR_GENERIC;
	union i2c_smbus_data data = { 0 };
	data.u8data = value;
	return ctx->ctrl->xfer(ctx, slave_addr, I2C_SMBUS_WRITE, 0, I2C_SMBUS_BYTE, &data);
}

int SM_ReadByteData(smbus_t* ctx, uint8_t slave_addr, uint8_t offset, uint8_t* value)
{
	if (!ctx || !ctx->ctrl || !ctx->ctrl->xfer)
		return SM_ERR_GENERIC;
	union i2c_smbus_data data = { 0 };
	int rc = ctx->ctrl->xfer(ctx, slave_addr, I2C_SMBUS_READ, offset, I2C_SMBUS_BYTE_DATA, &data);
	*value = data.u8data;
	return rc;
}

int SM_WriteByteData(smbus_t* ctx, uint8_t slave_addr, uint8_t offset, uint8_t value)
{
	if (!ctx || !ctx->ctrl || !ctx->ctrl->xfer)
		return SM_ERR_GENERIC;
	union i2c_smbus_data data = { 0 };
	data.u8data = value;
	return ctx->ctrl->xfer(ctx, slave_addr, I2C_SMBUS_WRITE, offset, I2C_SMBUS_BYTE_DATA, &data);
}

int SM_ReadWordData(smbus_t* ctx, uint8_t slave_addr, uint8_t offset, uint16_t* value)
{
	if (!ctx || !ctx->ctrl || !ctx->ctrl->xfer)
		return SM_ERR_GENERIC;
	union i2c_smbus_data data = { 0 };
	int rc = ctx->ctrl->xfer(ctx, slave_addr, I2C_SMBUS_READ, offset, I2C_SMBUS_WORD_DATA, &data);
	*value = data.u16data;
	return rc;
}

int SM_WriteWordData(smbus_t* ctx, uint8_t slave_addr, uint8_t offset, uint16_t value)
{
	if (!ctx || !ctx->ctrl || !ctx->ctrl->xfer)
		return SM_ERR_GENERIC;
	union i2c_smbus_data data = { 0 };
	data.u16data = value;
	return ctx->ctrl->xfer(ctx, slave_addr, I2C_SMBUS_WRITE, offset, I2C_SMBUS_WORD_DATA, &data);
}

int SM_ProcCall(smbus_t* ctx, uint8_t slave_addr, uint8_t offset, uint16_t* value)
{
	if (!ctx || !ctx->ctrl || !ctx->ctrl->xfer)
		return SM_ERR_GENERIC;
	union i2c_smbus_data data = { 0 };
	data.u16data = *value;
	int rc = ctx->ctrl->xfer(ctx, slave_addr, I2C_SMBUS_READ, offset, I2C_SMBUS_PROC_CALL, &data);
	*value = data.u16data;
	return rc;
}

static inline int DDR4_SetPage(smbus_t* ctx, uint8_t slave_addr, uint8_t page)
{
	if (page > SPD_DDR4_PAGE_MAX)
		return SM_ERR_PARAM;
	if (page == ctx->spd_page && slave_addr == ctx->last_dimm_addr)
		return SM_OK;
	return SM_WriteByteData(ctx, SPD_DDR4_ADDR_PAGE + page, 0, SPD_DDR4_PAGE_MASK);
}

static bool DDR4_IsAvailable(smbus_t* ctx, uint8_t slave_addr)
{
	if (SM_WriteQuick(ctx, SPD_DDR4_ADDR_PAGE, 0x00) != SM_OK)
	{
		SMBUS_DBG("DDR4 WriteQuick failed");
		return false;
	}
	if (DDR4_SetPage(ctx, slave_addr, 0) != SM_OK)
	{
		SMBUS_DBG("DDR4 SetPage 0 failed");
		return false;
	}
	if (SM_ReadByteData(ctx, slave_addr, SPD_MEMORY_TYPE_OFFSET, &ctx->spd_type) != SM_OK)
	{
		SMBUS_DBG("DDR4 ReadByteData 2 failed");
		return false;
	}
	SMBUS_DBG("DDR4 ReadByteData 2 OK (%02X)", ctx->spd_type);
	return true;
}

static int
DDR4_ReadByteAt(smbus_t* ctx, uint8_t slave_addr, uint16_t address, uint8_t* value)
{
	if (address >= ((SPD_DDR4_PAGE_MAX + 1) << SPD_DDR4_PAGE_SHIFT))
		return SM_ERR_PARAM;

	uint8_t page = (uint8_t)(address >> SPD_DDR4_PAGE_SHIFT);
	uint8_t offset = (uint8_t)(address & SPD_DDR4_PAGE_MASK);

	int status = DDR4_SetPage(ctx, slave_addr, page);
	if (status != SM_OK)
		return status;

	status = SM_ReadByteData(ctx, slave_addr, offset, value);
	return status;
}

static inline int DDR5_SetPage(smbus_t* ctx, uint8_t slave_addr, uint8_t page)
{
	if (page > SPD_DDR5_PAGE_MAX)
		return SM_ERR_PARAM;
	if (page == ctx->spd_page && slave_addr == ctx->last_dimm_addr)
		return SM_OK;
	if (ctx->spd_wd)
	{
		uint16_t ddr5_page = (uint16_t)page;
		return SM_ProcCall(ctx, slave_addr, SPD5_HUB_I2C_CONF, &ddr5_page);
	}
	return SM_WriteByteData(ctx, slave_addr, SPD5_HUB_I2C_CONF, page);
}

static bool DDR5_IsAvailable(smbus_t* ctx, uint8_t slave_addr)
{
	uint8_t current_page;
	if (SM_ReadByteData(ctx, slave_addr, SPD5_HUB_I2C_CONF, &current_page) != SM_OK)
	{
		SMBUS_DBG("DDR5 GetPage failed");
		return false;
	}
	ctx->spd_page = current_page & 0x07;
	ctx->last_dimm_addr = slave_addr;
	if (DDR5_SetPage(ctx, slave_addr, 0) != SM_OK)
	{
		SMBUS_DBG("DDR5 SetPage 0 failed");
		return false;
	}

	if (SM_ReadByteData(ctx, slave_addr, SPD_MEMORY_TYPE_OFFSET | 0x80, &ctx->spd_type) != SM_OK)
	{
		SMBUS_DBG("DDR5 ReadByteData 2 failed");
		return false;
	}
	SMBUS_DBG("DDR5 ReadByteData 2 OK (%02X)", ctx->spd_type);
	return true;
}

static int
DDR5_ReadByteAt(smbus_t* ctx, uint8_t slave_addr, uint16_t address, uint8_t* value)
{
	if (address >= ((SPD_DDR5_PAGE_MAX + 1) << SPD_DDR5_PAGE_SHIFT))
		return SM_ERR_PARAM;

	uint8_t page = (uint8_t)(address >> SPD_DDR5_PAGE_SHIFT);
	uint8_t offset = (uint8_t)((address & SPD_DDR5_PAGE_MASK) | 0x80);

	int status = DDR5_SetPage(ctx, slave_addr, page);
	if (status != SM_OK)
		return status;

	return SM_ReadByteData(ctx, slave_addr, offset, value);
}

int SM_GetSpd(smbus_t* ctx, uint8_t dimm_index, uint8_t data[SPD_MAX_SIZE])
{
	int result = SM_OK;
	if (!ctx || dimm_index >= SPD_MAX_SLOT || !data)
		return SM_ERR_PARAM;

	// SPD slave address for DIMMs are typically 0x50 to 0x57.
	uint8_t slave_addr = 0x50 + dimm_index;
	memset(data, 0xFF, SPD_MAX_SIZE);

	WR0_WaitSmBus(100);

	if (DDR4_IsAvailable(ctx, slave_addr))
		SMBUS_DBG("Detected DDR4 SPD");
	else if (DDR5_IsAvailable(ctx, slave_addr))
		SMBUS_DBG("Detected DDR5 SPD");
	else if (SM_ReadByteData(ctx, slave_addr, SPD_MEMORY_TYPE_OFFSET, &ctx->spd_type) != SM_OK)
	{
		SMBUS_DBG("Failed to read memory type for DIMM %d", dimm_index);
		result = SM_ERR_NO_DEVICE;
		goto fail;
	}

	switch (ctx->spd_type)
	{
	case MEM_TYPE_UNKNOWN:
		SMBUS_DBG("Invalid type %02Xh", ctx->spd_type);
		result = SM_ERR_NO_DEVICE;
		break;
	case MEM_TYPE_DDR4:
	case MEM_TYPE_DDR4E:
	case MEM_TYPE_LPDDR4:
	case MEM_TYPE_LPDDR4X:
		for (uint16_t i = 0; i < 512; i++)
		{
			result = DDR4_ReadByteAt(ctx, slave_addr, i, &data[i]);
			if (result != SM_OK)
			{
				SMBUS_DBG("Failed to read addr %u for DDR4 SPD on DIMM %u", i, dimm_index);
				goto fail;
			}
		}
		DDR4_SetPage(ctx, slave_addr, 0);
		break;

	case MEM_TYPE_DDR5:
	case MEM_TYPE_LPDDR5:
	case MEM_TYPE_LPDDR5X:
		for (uint16_t i = 0; i < 1024; i++)
		{
			result = DDR5_ReadByteAt(ctx, slave_addr, i, &data[i]);
			if (result != SM_OK)
			{
				SMBUS_DBG("Failed to read addr %u for DDR5 SPD on DIMM %u", i, dimm_index);
				goto fail;
			}
		}
		DDR5_SetPage(ctx, slave_addr, 0);
		break;
	case MEM_TYPE_FPM_DRAM:
	case MEM_TYPE_EDO:
	case MEM_TYPE_PNEDO:
	case MEM_TYPE_SDRAM:
	case MEM_TYPE_ROM:
	case MEM_TYPE_SGRAM:
	case MEM_TYPE_DDR:
		for (uint16_t i = 0; i < 128; i++)
		{
			result = SM_ReadByteData(ctx, slave_addr, (uint8_t)i, &data[i]);
			if (result != SM_OK)
			{
				SMBUS_DBG("Failed to read addr %u for DDR SPD on DIMM %u", i, dimm_index);
				goto fail;
			}
		}
		break;
	case MEM_TYPE_DDR2:
	case MEM_TYPE_DDR2_FB:
	case MEM_TYPE_DDR2_FB_P:
	case MEM_TYPE_DDR3:
	case MEM_TYPE_LPDDR3:
	default:
		for (uint16_t i = 0; i < 256; i++)
		{
			result = SM_ReadByteData(ctx, slave_addr, (uint8_t)i, &data[i]);
			if (result != SM_OK)
			{
				SMBUS_DBG("Failed to read addr %u for DDR SPD on DIMM %u", i, dimm_index);
				goto fail;
			}
		}
		break;
	}
fail:
	WR0_ReleaseSmBus();
	return result;
}
