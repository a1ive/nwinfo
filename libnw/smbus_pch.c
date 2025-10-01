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

#define PCI_VID_INTEL     0x8086
#define PIIX4_DID_INTEL   0x7113

static int PchDetect(smbus_t* ctx)
{
	if ((ctx->pci_id & 0xFFFF) != PCI_VID_INTEL)
		return SM_ERR_NO_DEVICE;

	if (((ctx->pci_id >> 16) & 0xFFFF) == PIIX4_DID_INTEL)
		return SM_ERR_NO_DEVICE;

	return SM_OK;
}

static int PchInit(smbus_t* ctx)
{
	// Enable SMBus I/O Space if disabled
	uint16_t pci_cmd = WR0_RdPciConf16(ctx->drv, ctx->pci_addr, 0x04);
	if (!(pci_cmd & 0x01))
		WR0_WrPciConf16(ctx->drv, ctx->pci_addr, 0x04, pci_cmd | 0x01);

	// Read Base Address
	uint32_t base = WR0_RdPciConf32(ctx->drv, ctx->pci_addr, 0x20);
	ctx->base_addr = base & 0xFFF0;

	if (ctx->base_addr == 0)
		return SM_ERR_GENERIC;

	// Enable Host Controller Interface if disabled
	uint8_t hostc = WR0_RdPciConf8(ctx->drv, ctx->pci_addr, 0x40);
	if (!(hostc & 0x04))
	{
		WR0_WrPciConf8(ctx->drv, ctx->pci_addr, 0x40, hostc | 0x04);
	}

	// Reset SMBus controller
	uint8_t ctrl = WR0_RdIo8(ctx->drv, ctx->base_addr + SMBHSTSTS);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTSTS, ctrl & 0x1F);
	Sleep(1);

	return SM_OK;
}

static int PchWait(smbus_t* ctx)
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

static int PchTransaction(smbus_t* ctx)
{
	// Clear status bits by writing them back
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTSTS, SMBHSTSTS_CLEAR_MASK);

	// Start transaction
	uint8_t ctrl = WR0_RdIo8(ctx->drv, ctx->base_addr + SMBHSTCNT);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTCNT, ctrl | SMBHSTCNT_START);

	int result = PchWait(ctx);

	// Clear status again after transaction
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTSTS, SMBHSTSTS_CLEAR_MASK);

	return result;
}

static int PchReadByte(smbus_t* ctx, uint8_t slave_addr, uint8_t command, uint8_t* value)
{
	if (!value)
		return SM_ERR_PARAM;
	*value = 0xFF;

	if (WR0_RdIo8(ctx->drv, ctx->base_addr + SMBHSTSTS) & SMBHSTSTS_HOST_BUSY)
		return SM_ERR_BUS_ERROR;

	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTADD, (slave_addr << 1) | I2C_READ);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTCMD, command);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTCNT, SMBHSTCNT_BYTE_DATA);

	int result = PchTransaction(ctx);
	if (result != SM_OK)
		return result;

	*value = WR0_RdIo8(ctx->drv, ctx->base_addr + SMBHSTDAT0);

	return SM_OK;
}

static int PchWriteByte(smbus_t* ctx, uint8_t slave_addr, uint8_t command, uint8_t value)
{
	if (!ctx)
		return SM_ERR_PARAM;

	if (WR0_RdIo8(ctx->drv, ctx->base_addr + SMBHSTSTS) & SMBHSTSTS_HOST_BUSY)
		return SM_ERR_BUS_ERROR;

	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTADD, (slave_addr << 1) | I2C_WRITE); // Set address and write bit
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTCMD, command);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTDAT0, value);
	WR0_WrIo8(ctx->drv, ctx->base_addr + SMBHSTCNT, SMBHSTCNT_BYTE_DATA);

	return PchTransaction(ctx);
}

const smctrl_t pch_controller =
{
	.name = "i801 / ICH5",
	.detect = PchDetect,
	.init = PchInit,
	.read_byte = PchReadByte,
	.write_byte = PchWriteByte,
};
