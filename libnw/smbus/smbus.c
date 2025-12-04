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

static inline int SetDDR4Page(smbus_t* ctx, uint8_t slave_addr, uint8_t page)
{
	if (page > SPD_DDR4_PAGE_MAX)
		return SM_ERR_PARAM;
	if (page == ctx->spd_page && slave_addr == ctx->last_dimm_addr)
		return SM_OK;
	return SM_WriteByteData(ctx, SPD_DDR4_ADDR_PAGE + page, 0, SPD_DDR4_PAGE_MASK);
}

bool SM_DDR4_IsAvailable(smbus_t* ctx, uint8_t slave_addr)
{
	if (SM_WriteQuick(ctx, SPD_DDR4_ADDR_PAGE, 0x00) != SM_OK)
	{
		SMBUS_DBG("DDR4 WriteQuick failed");
		return false;
	}
	if (SetDDR4Page(ctx, slave_addr, 0) != SM_OK)
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

int
SM_DDR4_ReadByteAt(smbus_t* ctx, uint8_t slave_addr, uint16_t address, uint8_t* value)
{
	if (address >= ((SPD_DDR4_PAGE_MAX + 1) << SPD_DDR4_PAGE_SHIFT))
		return SM_ERR_PARAM;

	uint8_t page = (uint8_t)(address >> SPD_DDR4_PAGE_SHIFT);
	uint8_t offset = (uint8_t)(address & SPD_DDR4_PAGE_MASK);

	int status = SetDDR4Page(ctx, slave_addr, page);
	if (status != SM_OK)
		return status;

	status = SM_ReadByteData(ctx, slave_addr, offset, value);
	return status;
}

static inline float ConvertTemperature(uint16_t temp_raw)
{
	float temp = 0.0f;
	if ((temp_raw & 0x1000) != 0) // Negative temperature
	{
		temp_raw = (uint16_t)(temp_raw & ~0x1000);
		temp = temp_raw * 0.0625f - 256;
	}
	else // Positive temperature
	{
		temp = temp_raw* 0.0625f;
	}
	return temp;
}

bool SM_DDR4_IsThermalSensorPresent(smbus_t* ctx, uint8_t slave_addr)
{
	uint8_t spd_byte = 0;
	if (SM_DDR4_ReadByteAt(ctx, slave_addr, 14, &spd_byte) != SM_OK)
		return false;
	if (!(spd_byte & 0x80)) // Bit 7
		return false;
	uint8_t ts_addr = 0x18 | (slave_addr & 0x07);
	if (SM_WriteQuick(ctx, ts_addr, 0x00) != SM_OK)
		return false;
	return true;
}

float SM_DDR4_GetTemperature(smbus_t* ctx, uint8_t slave_addr)
{
	uint8_t ts_addr = 0x18 | (slave_addr & 0x07);
	uint16_t temp_raw = 0;
	if (SetDDR4Page(ctx, slave_addr, 0) != SM_OK)
		return 0.0f;
	if (SM_ReadWordData(ctx, ts_addr, 0x05, &temp_raw) != SM_OK)
		return 0.0f;
	// Swap bytes
	temp_raw = (uint16_t)(((temp_raw & 0xFF00) >> 8) | ((temp_raw & 0x00FF) << 8));
	// Strip away not required bits
	temp_raw &= 0x0FFF;
	return ConvertTemperature(temp_raw);
}

static inline int SetDDR5Page(smbus_t* ctx, uint8_t slave_addr, uint8_t page)
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

bool SM_DDR5_IsAvailable(smbus_t* ctx, uint8_t slave_addr)
{
	uint8_t current_page;
	if (SM_ReadByteData(ctx, slave_addr, SPD5_HUB_I2C_CONF, &current_page) != SM_OK)
	{
		SMBUS_DBG("DDR5 GetPage failed");
		return false;
	}
	ctx->spd_page = current_page & 0x07;
	ctx->last_dimm_addr = slave_addr;
	if (SetDDR5Page(ctx, slave_addr, 0) != SM_OK)
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

int
SM_DDR5_ReadByteAt(smbus_t* ctx, uint8_t slave_addr, uint16_t address, uint8_t* value)
{
	if (address >= ((SPD_DDR5_PAGE_MAX + 1) << SPD_DDR5_PAGE_SHIFT))
		return SM_ERR_PARAM;

	uint8_t page = (uint8_t)(address >> SPD_DDR5_PAGE_SHIFT);
	uint8_t offset = (uint8_t)((address & SPD_DDR5_PAGE_MASK) | 0x80);

	int status = SetDDR5Page(ctx, slave_addr, page);
	if (status != SM_OK)
		return status;

	return SM_ReadByteData(ctx, slave_addr, offset, value);
}

bool SM_DDR5_IsThermalSensorPresent(smbus_t* ctx, uint8_t slave_addr)
{
	uint8_t mr5 = 0; // MR5 - Device Capability
	if (SetDDR5Page(ctx, slave_addr, 0) != SM_OK)
		return false;
	if (SM_ReadByteData(ctx, slave_addr, SPD5_HUB_CAP, &mr5) != SM_OK)
		return false;
	if (!(mr5 & 0x02)) // Bit 1 = Temperature Sensor Support
		return false;
	return true;
}

float SM_DDR5_GetTemperature(smbus_t* ctx, uint8_t slave_addr)
{
	uint16_t temp_raw = 0;
	if (SetDDR5Page(ctx, slave_addr, 0) != SM_OK)
		return 0.0f;
	if (SM_ReadWordData(ctx, slave_addr, SPD5_HUB_TS_LSB, &temp_raw) != SM_OK)
		return 0.0f;
	return ConvertTemperature(temp_raw);
}

VOID NWL_GetMemSensors(NWLIB_MEM_SENSORS* info)
{
	if (NWLC->NwSmbus == NULL)
		return;
	WR0_WaitSmBus(10);
	if (!info->Initialized)
	{
		info->Count = 0;
		for (uint8_t i = 0; i < SPD_MAX_SLOT; i++)
		{
			uint8_t slave_addr = SPD_SLABE_ADDR_BASE + i;
			info->Sensor[info->Count].Addr = slave_addr;
			if (SM_DDR4_IsAvailable(NWLC->NwSmbus, slave_addr))
			{
				if (SM_DDR4_IsThermalSensorPresent(NWLC->NwSmbus, slave_addr))
					info->Sensor[info->Count].Type = MEM_TYPE_DDR4;
				info->Count++;
			}
			else if (SM_DDR5_IsAvailable(NWLC->NwSmbus, slave_addr))
			{
				if (SM_DDR5_IsThermalSensorPresent(NWLC->NwSmbus, slave_addr))
					info->Sensor[info->Count].Type = MEM_TYPE_DDR5;
				info->Count++;
			}
		}
		info->Initialized = TRUE;
	}
	for (uint8_t i = 0; i < info->Count; i++)
	{
		switch (info->Sensor[i].Type)
		{
		case MEM_TYPE_DDR4:
			info->Sensor[i].Temp = SM_DDR4_GetTemperature(NWLC->NwSmbus, info->Sensor[i].Addr);
			break;
		case MEM_TYPE_DDR5:
			info->Sensor[i].Temp = SM_DDR5_GetTemperature(NWLC->NwSmbus, info->Sensor[i].Addr);
			break;
		}
	}
	WR0_ReleaseSmBus();
}

static const uint16_t
SPD_INDEX_DDR3[] =
{
	0,1,2,3,4,6,7,8,12,16,18,20,21,22,23,32,34,35,36,37,
	117,118,120,121,122,123,124,125,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,
	176,177,180,181,186,187,191,192,194,195,196,221,
};

static const uint16_t
SPD_INDEX_DDR4[] =
{
	0,1,2,3,4,6,12,13,14,18,24,25,26,27,28,29,
	121,122,123,125,
	320,321,323,324,325,326,327,328,329,330,331,332,333,334,335,336,337,338,339,340,341,342,343,344,345,346,347,348,
	384,385,393,396,401,402,403,404,405,406,427,428,429,430,431,
};

static const uint16_t
SPD_INDEX_DDR5[] =
{
	0,1,2,3,4,6,8,10,15,20,21,30,31,32,33,34,35,36,37,38,39,
	234,235,
	512,513,515,516,517,518,519,520,521,522,523,524,525,526,527,528,529,530,531,532,533,534,535,536,537,538,539,540,
	640,641,
	709,710,717,718,719,720,721,722,723,724,725,773,774,781,782,783,784,785,786,787,788,789,790,
};

int SM_GetSpd(smbus_t* ctx, uint8_t dimm_index, uint8_t data[SPD_MAX_SIZE])
{
	int result = SM_OK;
	if (!ctx || dimm_index >= SPD_MAX_SLOT || !data)
		return SM_ERR_PARAM;

	// SPD slave address for DIMMs are typically 0x50 to 0x57.
	uint8_t slave_addr = SPD_SLABE_ADDR_BASE + dimm_index;
	memset(data, 0xFF, SPD_MAX_SIZE);

	WR0_WaitSmBus(100);

	if (SM_DDR4_IsAvailable(ctx, slave_addr))
		SMBUS_DBG("Detected DDR4 SPD");
	else if (SM_DDR5_IsAvailable(ctx, slave_addr))
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
		if (NWLC->BinaryFormat == BIN_FMT_NONE)
		{
			for (uint16_t i = 0; i < ARRAYSIZE(SPD_INDEX_DDR4); i++)
			{
				uint16_t off = SPD_INDEX_DDR4[i];
				result = SM_DDR4_ReadByteAt(ctx, slave_addr, off, &data[off]);
				if (result != SM_OK)
				{
					SMBUS_DBG("Failed to read addr %u for DDR4 SPD on DIMM %u", off, dimm_index);
					goto fail;
				}
			}
		}
		else
		{
			for (uint16_t i = 0; i < 512; i++)
			{
				result = SM_DDR4_ReadByteAt(ctx, slave_addr, i, &data[i]);
				if (result != SM_OK)
				{
					SMBUS_DBG("Failed to read addr %u for DDR4 SPD on DIMM %u", i, dimm_index);
					goto fail;
				}
			}
		}
		SetDDR4Page(ctx, slave_addr, 0);
		break;
	case MEM_TYPE_DDR5:
	case MEM_TYPE_LPDDR5:
	case MEM_TYPE_LPDDR5X:
		if (NWLC->BinaryFormat == BIN_FMT_NONE)
		{
			for (uint16_t i = 0; i < ARRAYSIZE(SPD_INDEX_DDR5); i++)
			{
				uint16_t off = SPD_INDEX_DDR5[i];
				result = SM_DDR5_ReadByteAt(ctx, slave_addr, off, &data[off]);
				if (result != SM_OK)
				{
					SMBUS_DBG("Failed to read addr %u for DDR5 SPD on DIMM %u", off, dimm_index);
					goto fail;
				}
			}
		}
		else
		{
			for (uint16_t i = 0; i < 1024; i++)
			{
				result = SM_DDR5_ReadByteAt(ctx, slave_addr, i, &data[i]);
				if (result != SM_OK)
				{
					SMBUS_DBG("Failed to read addr %u for DDR5 SPD on DIMM %u", i, dimm_index);
					goto fail;
				}
			}
		}
		SetDDR5Page(ctx, slave_addr, 0);
		break;
	case MEM_TYPE_DDR3:
	case MEM_TYPE_LPDDR3:
		if (NWLC->BinaryFormat == BIN_FMT_NONE)
		{
			for (uint16_t i = 0; i < ARRAYSIZE(SPD_INDEX_DDR3); i++)
			{
				uint16_t off = SPD_INDEX_DDR3[i];
				result = SM_ReadByteData(ctx, slave_addr, (uint8_t)off, &data[off]);
				if (result != SM_OK)
				{
					SMBUS_DBG("Failed to read addr %u for DDR3 SPD on DIMM %u", off, dimm_index);
					goto fail;
				}
			}
		}
		else
		{
			for (uint16_t i = 0; i < 256; i++)
			{
				result = SM_ReadByteData(ctx, slave_addr, (uint8_t)i, &data[i]);
				if (result != SM_OK)
				{
					SMBUS_DBG("Failed to read addr %u for DDR3 SPD on DIMM %u", i, dimm_index);
					goto fail;
				}
			}
		}
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
				SMBUS_DBG("Failed to read addr %u for Legacy RAM SPD on DIMM %u", i, dimm_index);
				goto fail;
			}
		}
		break;
	case MEM_TYPE_DDR2:
	case MEM_TYPE_DDR2_FB:
	case MEM_TYPE_DDR2_FB_P:
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
