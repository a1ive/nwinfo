// smbus_fch.c
// SPDX-License-Identifier: Unlicense

#include "smbus.h"
#include <windows.h>
#include <stdio.h>

#define SMBHSTSTS_HOST_BUSY     0x01
#define SMBHSTSTS_INTR          0x02
#define SMBHSTSTS_DEV_ERR       0x04
#define SMBHSTSTS_BUS_ERR       0x08
#define SMBHSTSTS_FAILED        0x10
#define SMBHSTSTS_SMBALERT_STS  0x20
#define SMBHSTSTS_INUSE_STS     0x40
#define SMBHSTSTS_BYTE_DONE     0x80

#define SMBHSTSTS_ERROR_MASK    (SMBHSTSTS_DEV_ERR | SMBHSTSTS_BUS_ERR | SMBHSTSTS_FAILED)
#define SMBHSTSTS_CLEAR_MASK    0xFF

#define SMBHSTCNT_QUICK             0x00
#define SMBHSTCNT_BYTE              0x04
#define SMBHSTCNT_BYTE_DATA         0x08
#define SMBHSTCNT_WORD_DATA         0x0C
#define SMBHSTCNT_PROC_CALL         0x10
#define SMBHSTCNT_BLOCK_DATA        0x14
#define SMBHSTCNT_I2C_BLOCK_DATA    0x18
#define SMBHSTCNT_LAST_BYTE         0x20
#define SMBHSTCNT_START             0x40

#define PCI_VID_AMD         0x1022
#define PCI_VID_ATI         0x1002
#define PCI_VID_HYGON       0x1D94

#define AMD_INDEX_IO_PORT   0xCD6
#define AMD_DATA_IO_PORT    0xCD7
#define AMD_SMBUS_BASE_REG  0x2C
#define AMD_PM_INDEX        0x00

static int FchDetect(smbus_t* ctx)
{
	uint16_t vid = ctx->pci_id & 0xFFFF;
	uint16_t did = (ctx->pci_id >> 16) & 0xFFFF;
	switch (vid)
	{
	case PCI_VID_AMD:
	case PCI_VID_HYGON:
		if (did == 0x790B || did == 0x780B)
			return SM_OK;
	case PCI_VID_ATI:
		if (did == 0x4385 && ctx->rev_id > 0x3D)
			return SM_OK;
	}
	return SM_ERR_NO_DEVICE;
}

static int ZenInit(smbus_t* ctx)
{
	SMBUS_DBG("Using Zen FCH SMBus initialization");
	WR0_WrIo8(ctx->drv, AMD_INDEX_IO_PORT, AMD_PM_INDEX + 1);
	uint16_t pm_reg = WR0_RdIo8(ctx->drv, AMD_DATA_IO_PORT) << 8;
	WR0_WrIo8(ctx->drv, AMD_INDEX_IO_PORT, AMD_PM_INDEX);
	pm_reg |= WR0_RdIo8(ctx->drv, AMD_DATA_IO_PORT);
	if (pm_reg == 0xFFFF)
	{
		if (WR0_RdMem(ctx->drv, 0xFED80300, (uint8_t*)&pm_reg, 1, sizeof(uint16_t)) != sizeof(uint16_t))
			return SM_ERR_GENERIC;
		ctx->base_addr = pm_reg & 0xFF00;
	}
	else
	{
		// Check if I/O is enabled
		if ((pm_reg & 0x10) == 0)
			return SM_ERR_GENERIC;
		ctx->base_addr = pm_reg & 0xFF00;
	}
	if (ctx->base_addr == 0)
		return SM_ERR_GENERIC;
	return SM_OK;
}

static int SbInit(smbus_t* ctx)
{
	SMBUS_DBG("Using SB FCH SMBus initialization");
	WR0_WrIo8(ctx->drv, AMD_INDEX_IO_PORT, AMD_SMBUS_BASE_REG + 1);
	uint16_t base = WR0_RdIo8(ctx->drv, AMD_DATA_IO_PORT) << 8;
	WR0_WrIo8(ctx->drv, AMD_INDEX_IO_PORT, AMD_SMBUS_BASE_REG);
	base |= WR0_RdIo8(ctx->drv, AMD_DATA_IO_PORT) & 0xE0;
	if (base == 0xFFE0 || base == 0)
		return SM_ERR_GENERIC;
	ctx->base_addr = base;
	return SM_OK;
}

static int FchInit(smbus_t* ctx)
{
	uint16_t vid = ctx->pci_id & 0xFFFF;
	uint16_t did = (ctx->pci_id >> 16) & 0xFFFF;
	switch (vid)
	{
	case PCI_VID_AMD:
	case PCI_VID_HYGON:
		if (did == 0x790B)
			return ZenInit(ctx);
		else if (did == 0x780B)
		{
			if (ctx->rev_id >= 0x42)
				return ZenInit(ctx);
			return SbInit(ctx);
		}
	case PCI_VID_ATI:
		if (did == 0x4385 && ctx->rev_id > 0x3D)
			return SbInit(ctx);
	}

	return SM_ERR_GENERIC;
}

static int FchWait(smbus_t* ctx)
{
	int timeout = 5;
	uint8_t status;
	do
	{
		Sleep(1);
		status = WR0_RdIo8(ctx->drv, ctx->base_addr + SMBHSTSTS);
		if (!(status & SMBHSTSTS_HOST_BUSY))
			break;
	} while (--timeout > 0);

	if (timeout <= 0)
		return SM_ERR_TIMEOUT;

	if (status & SMBHSTSTS_ERROR_MASK)
		return SM_ERR_BUS_ERROR;

	return SM_OK;
}

static int FchTransaction(smbus_t* ctx)
{
	// Clear status bits by writing them back
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTSTS, SMBHSTSTS_CLEAR_MASK);

	// Start transaction
	uint8_t ctrl = WR0_RdIo8(ctx->drv, ctx->base_addr + SMBHSTCNT);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTCNT, ctrl | SMBHSTCNT_START);

	int result = FchWait(ctx);

	// Clear status again after transaction
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTSTS, SMBHSTSTS_CLEAR_MASK);

	return result;
}

static int FchReadByte(smbus_t* ctx, uint8_t slave_addr, uint8_t command, uint8_t* value)
{
	if (!value)
		return SM_ERR_PARAM;
	*value = 0xFF;

	if (WR0_RdIo8(ctx->drv, ctx->base_addr + SMBHSTSTS) & SMBHSTSTS_HOST_BUSY)
		return SM_ERR_BUS_ERROR;

	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTADD, (slave_addr << 1) | I2C_READ);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTCMD, command);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTCNT, SMBHSTCNT_BYTE_DATA);

	int result = FchTransaction(ctx);
	if (result != SM_OK)
		return result;

	*value = WR0_RdIo8(ctx->drv, ctx->base_addr + SMBHSTDAT0);

	return SM_OK;
}

static int FchWriteByte(smbus_t* ctx, uint8_t slave_addr, uint8_t command, uint8_t value)
{
	if (!ctx)
		return SM_ERR_PARAM;

	if (WR0_RdIo8(ctx->drv, ctx->base_addr + SMBHSTSTS) & SMBHSTSTS_HOST_BUSY)
		return SM_ERR_BUS_ERROR;

	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTADD, (slave_addr << 1) | I2C_WRITE); // Set address and write bit
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTCMD, command);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTDAT0, value);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTCNT, SMBHSTCNT_BYTE_DATA);

	return FchTransaction(ctx);
}

const smctrl_t fch_controller =
{
	.name = "AMD SB / Zen FCH",
	.detect = FchDetect,
	.init = FchInit,
	.read_byte = FchReadByte,
	.write_byte = FchWriteByte,
};
