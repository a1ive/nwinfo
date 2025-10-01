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

#define PCI_VID_ATI         0x1002
#define PCI_VID_VIA         0x1106
#define PCI_VID_INTEL       0x8086
#define PCI_VID_SERVERWORKS 0x1166

#define PIIX4_SMB_BASE_ADR_DEFAULT  0x90
#define PIIX4_SMB_BASE_ADR_VIAPRO   0xD0

static int Piix4Detect(smbus_t* ctx)
{
	uint16_t vid = ctx->pci_id & 0xFFFF;
	uint16_t did = (ctx->pci_id >> 16) & 0xFFFF;
	switch (vid)
	{
	case PCI_VID_ATI:
		if (did == 0x4372)
			return SM_OK;
		if (did == 0x4385 && ctx->rev_id <= 0x3D)
			return SM_OK;
	case PCI_VID_VIA:
		switch (vid)
		{
		case 0x3057:
		case 0x3074:
		case 0x3147:
		case 0x3177:
		case 0x3227:
		case 0x3372:
			return SM_OK;
		}
		break;
	case PCI_VID_INTEL:
		if (did == 0x7113)
			return SM_OK;
	case PCI_VID_SERVERWORKS:
		if (did == 0x0201)
			return SM_OK;
	}
	return SM_ERR_NO_DEVICE;
}

static int Piix4Init(smbus_t* ctx)
{
	uint16_t vid = ctx->pci_id & 0xFFFF;
	uint16_t did = (ctx->pci_id >> 16) & 0xFFFF;
	uint8_t piix4_addr = PIIX4_SMB_BASE_ADR_DEFAULT;
	if (vid == PCI_VID_VIA)
	{
		switch (did)
		{
		case 0x3074:
		case 0x3147:
		case 0x3177:
		case 0x3227:
		case 0x3372:
			piix4_addr = PIIX4_SMB_BASE_ADR_VIAPRO;
			break;
		}
	}

	ctx->base_addr = WR0_RdPciConf16(ctx->drv, ctx->pci_addr, piix4_addr) & 0xFFF0;

	if (ctx->base_addr == 0)
		return SM_ERR_GENERIC;
	return SM_OK;
}

static int Piix4Wait(smbus_t* ctx)
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

static int Piix4Transaction(smbus_t* ctx)
{
	// Clear status bits by writing them back
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTSTS, SMBHSTSTS_CLEAR_MASK);

	// Start transaction
	uint8_t ctrl = WR0_RdIo8(ctx->drv, ctx->base_addr + SMBHSTCNT);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTCNT, ctrl | SMBHSTCNT_START);

	int result = Piix4Wait(ctx);

	// Clear status again after transaction
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTSTS, SMBHSTSTS_CLEAR_MASK);

	return result;
}

static int Piix4ReadByte(smbus_t* ctx, uint8_t slave_addr, uint8_t command, uint8_t* value)
{
	if (!value)
		return SM_ERR_PARAM;
	*value = 0xFF;

	if (WR0_RdIo8(ctx->drv, ctx->base_addr + SMBHSTSTS) & SMBHSTSTS_HOST_BUSY)
		return SM_ERR_BUS_ERROR;

	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTADD, (slave_addr << 1) | I2C_READ);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTCMD, command);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTCNT, SMBHSTCNT_BYTE_DATA);

	int result = Piix4Transaction(ctx);
	if (result != SM_OK)
		return result;

	*value = WR0_RdIo8(ctx->drv, ctx->base_addr + SMBHSTDAT0);

	return SM_OK;
}

static int Piix4WriteByte(smbus_t* ctx, uint8_t slave_addr, uint8_t command, uint8_t value)
{
	if (!ctx)
		return SM_ERR_PARAM;

	if (WR0_RdIo8(ctx->drv, ctx->base_addr + SMBHSTSTS) & SMBHSTSTS_HOST_BUSY)
		return SM_ERR_BUS_ERROR;

	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTADD, (slave_addr << 1) | I2C_WRITE); // Set address and write bit
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTCMD, command);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTDAT0, value);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTCNT, SMBHSTCNT_BYTE_DATA);

	return Piix4Transaction(ctx);
}

const smctrl_t piix4_controller =
{
	.name = "PIIX4",
	.detect = Piix4Detect,
	.init = Piix4Init,
	.read_byte = Piix4ReadByte,
	.write_byte = Piix4WriteByte,
};
