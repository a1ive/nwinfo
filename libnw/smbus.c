// SPDX-License-Identifier: Unlicense

#include "smbus.h"
#include <stdlib.h>
#include <stdio.h>

extern const smctrl_t pch_controller;
extern const smctrl_t piix4_controller;
extern const smctrl_t fch_controller;

static const smctrl_t* ctrl_list[] =
{
	&pch_controller,
	&piix4_controller,
	&fch_controller,
	NULL
};

smbus_t* SM_Init(struct wr0_drv_t* drv)
{
	if (!drv)
		return NULL;

	smbus_t* ctx = (smbus_t*)calloc(1, sizeof(smbus_t));
	if (!ctx)
		return NULL;

	ctx->drv = drv;
	ctx->spd_page = -1;
	ctx->last_dimm_addr = 0xFF;
	ctx->base_addr = 0;
	ctx->pci_addr = WR0_FindPciByClass(drv, SMBUS_BASE_CLASS, SMBUS_SUB_CLASS, SMBUS_PROG_IF, 0);
	if (ctx->pci_addr == 0xFFFFFFFF)
		goto fail;
	ctx->pci_id = WR0_RdPciConf32(drv, ctx->pci_addr, 0x00);
	if (ctx->pci_id == 0xFFFFFFFF)
		goto fail;
	ctx->rev_id = WR0_RdPciConf8(drv, ctx->pci_addr, 0x08);

	SMBUS_DBG("Found SMBus device at PCI %02X:%02X.%X, ID %04X:%04X REV %02X",
		(ctx->pci_addr >> 16) & 0xFF,
		(ctx->pci_addr >> 11) & 0x1F,
		(ctx->pci_addr >> 8) & 0x07,
		ctx->pci_id & 0xFFFF,
		(ctx->pci_id >> 16) & 0xFFFF,
		ctx->rev_id);

	for (int i = 0; ctrl_list[i] != NULL; i++)
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
			}
		}
	}

fail:
	SMBUS_DBG("No supported SMBus controller found");
	free(ctx);
	return NULL;
}

void SM_Free(smbus_t* ctx)
{
	if (ctx)
		free(ctx);
}

int SM_ReadByte(smbus_t* ctx, uint8_t slave_addr, uint8_t command, uint8_t* value)
{
	if (!ctx || !ctx->ctrl || !ctx->ctrl->read_byte)
		return SM_ERR_GENERIC;
	return ctx->ctrl->read_byte(ctx, slave_addr, command, value);
}

int SM_WriteByte(smbus_t* ctx, uint8_t slave_addr, uint8_t command, uint8_t value)
{
	if (!ctx || !ctx->ctrl || !ctx->ctrl->write_byte)
		return SM_ERR_GENERIC;
	return ctx->ctrl->write_byte(ctx, slave_addr, command, value);
}

static int SetSpdPage(smbus_t* ctx, uint8_t dimm_addr, uint8_t mem_type, uint8_t page)
{
	if (ctx->spd_page == page && ctx->last_dimm_addr == dimm_addr)
	{
		// Already on the correct page for this DIMM
		return SM_OK;
	}

	int result = SM_ERR_GENERIC;

	switch (mem_type)
	{
	case MEM_TYPE_DDR4:
	case MEM_TYPE_DDR4E:
	case MEM_TYPE_LPDDR4:
	{
		result = SM_WriteByte(ctx, (page == 0) ? SPD_PAGE_SELECT_DDR4_0 : SPD_PAGE_SELECT_DDR4_1, 0x00, 0x00);
		break;
	}
	case MEM_TYPE_DDR5:
	case MEM_TYPE_LPDDR5:
	{
		result = SM_WriteByte(ctx, dimm_addr, SPD5_MR11 &0x7F, page & 0x07);
		break;
	}
	default:
		return SM_OK;
	}

	if (result == SM_OK)
	{
		ctx->spd_page = page;
		ctx->last_dimm_addr = dimm_addr;
	}
	else
	{
		ctx->spd_page = -1;
		ctx->last_dimm_addr = 0xFF;
	}

	return result;
}

static int ReadSpdPageData(smbus_t* ctx, uint8_t slave_addr, uint8_t* buffer, int page_size)
{
	SMBUS_DBG("Reading SPD data from slave 0x%02X, %d bytes", slave_addr, page_size);
	for (int offset = 0; offset < page_size; offset++)
	{
		int result = SM_ReadByte(ctx, slave_addr, (uint8_t)offset, &buffer[offset]);
		if (result != SM_OK)
			return result;
	}
	return SM_OK;
}

int SM_GetSpd(smbus_t* ctx, uint8_t dimm_index, uint8_t data[SPD_MAX_SIZE])
{
	if (!ctx || dimm_index > 7 || !data)
		return SM_ERR_PARAM;

	// SPD slave address for DIMMs are typically 0x50 to 0x57.
	uint8_t slave_addr = 0x50 + dimm_index;
	memset(data, 0xFF, SPD_MAX_SIZE);

	// Assume the type is DDR4 initially.
	int result = SetSpdPage(ctx, slave_addr, MEM_TYPE_DDR4, 0);
	if (result != SM_OK)
	{
		// Try setting page for DDR5 as a fallback.
		result = SetSpdPage(ctx, slave_addr, MEM_TYPE_DDR5, 0);
		if (result != SM_OK)
			SMBUS_DBG("Failed to set initial SPD page. Assuming legacy memory.");
	}

	// Read the memory type byte from the first page.
	uint8_t mem_type = 0xFF;
	result = SM_ReadByte(ctx, slave_addr, SPD_MEMORY_TYPE_OFFSET, &mem_type);
	if (result != SM_OK)
	{
		SMBUS_DBG("Failed to read memory type for DIMM %d", dimm_index);
		return result;
	}

	switch (mem_type)
	{
	case MEM_TYPE_DDR4:
	case MEM_TYPE_DDR4E:
	case MEM_TYPE_LPDDR4:
		// DDR4 has 2 pages of 256 bytes each.
		result = ReadSpdPageData(ctx, slave_addr, &data[0], 256);
		if (result != SM_OK)
		{
			SMBUS_DBG("Failed to read page 0 for DDR4 SPD on DIMM %d", dimm_index);
			return result;
		}
		result = SetSpdPage(ctx, slave_addr, mem_type, 1);
		if (result != SM_OK)
		{
			SMBUS_DBG("Failed to set page 1 for DDR4 SPD on DIMM %d", dimm_index);
			return result;
		}
		result = ReadSpdPageData(ctx, slave_addr, &data[256], 256);
		if (result != SM_OK)
		{
			SMBUS_DBG("Failed to read page 1 for DDR4 SPD on DIMM %d", dimm_index);
			return result;
		}
		break;

	case MEM_TYPE_DDR5:
	case MEM_TYPE_LPDDR5:
		// DDR5 has 8 pages of 128 bytes.
		for (int page = 0; page < 8; page++)
		{
			result = SetSpdPage(ctx, slave_addr, mem_type, page);
			if (result != SM_OK)
			{
				SMBUS_DBG("Failed to set page %d for DDR5 SPD on DIMM %d", page, dimm_index);
				return result;
			}
			// For DDR5, the I2C offset is within the 128-byte page.
			result = ReadSpdPageData(ctx, slave_addr, &data[page * 128], 128);
			if (result != SM_OK)
			{
				SMBUS_DBG("Failed to read page %d for DDR5 SPD on DIMM %d", page, dimm_index);
				return result;
			}
		}
		break;

	case MEM_TYPE_DDR:
	case MEM_TYPE_DDR2:
	case MEM_TYPE_DDR3:
	case MEM_TYPE_LPDDR3:
	default:
		// Legacy DDR types have a single 256-byte page.
		SMBUS_DBG("Detected legacy / unknown memory type 0x%02X on DIMM %d", mem_type, dimm_index);
		result = ReadSpdPageData(ctx, slave_addr, &data[0], 256);
		if (result != SM_OK)
		{
			SMBUS_DBG("Failed to read SPD for SPD on DIMM %d", dimm_index);
			return result;
		}
		break;
	}

	return SM_OK;
}
