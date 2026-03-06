// SPDX-License-Identifier: Unlicense

#include <lpcio.h>
#include <ioctl.h>
#include <superio.h>
#include <libnw.h>
#include <chip_ids.h>
#include <mb_vendor.h>

#include "lpc.h"

static bool detect_fintek(plpcio io, NWLIB_MAINBOARD_INFO* board, NWLIB_LPC_SLOT* slot)
{
	// Enter
	lpcio_pio_outb(io, io->reg_port, 0x87);
	lpcio_pio_outb(io, io->reg_port, 0x87);

	uint8_t id = 0;
	uint8_t revision = 0;
	enum CHIP_ID chip = CHIP_UNKNOWN;
	uint8_t ldn = FINTEK_HARDWARE_MONITOR_LDN;
	const char* name = "Fintek";

	lpcio_sio_inb(io, LPCIO_CHIP_ID_REG, &id);
	lpcio_sio_inb(io, LPCIO_CHIP_REV_REG, &revision);
	NWL_Debug("LPC", "Chip id: %02X, rev: %02X", id, revision);

	switch (id)
	{
	case 0x05:
		switch (revision)
		{
		case 0x07:
			chip = CHIP_F71858;
			ldn = F71858_HARDWARE_MONITOR_LDN;
			break;
		case 0x41:
			chip = CHIP_F71882;
			break;
		}
		break;
	case 0x06:
		switch (revision)
		{
		case 0x01:
			chip = CHIP_F71862;
			break;
		}
		break;
	case 0x07:
		switch (revision)
		{
		case 0x23:
			chip = CHIP_F71889F;
			break;
		}
		break;
	case 0x08:
		switch (revision)
		{
		case 0x14:
			chip = CHIP_F71869;
			break;
		}
		break;
	case 0x09:
		switch (revision)
		{
		case 0x01:
			chip = CHIP_F71808E;
			break;
		case 0x09:
			chip = CHIP_F71889ED;
			break;
		}
		break;
	case 0x10:
		switch (revision)
		{
		case 0x05:
			chip = CHIP_F71889AD;
			break;
		case 0x07:
			chip = CHIP_F71869A;
			break;
		}
		break;
	case 0x11:
		switch (revision)
		{
		case 0x06:
			chip = CHIP_F71878AD;
			break;
		case 0x18:
			chip = CHIP_F71811;
			break;
		}
		break;
	}

	name = CHIP_ID_TO_NAME(chip);
	NWL_Debug("LPC", "Identified %s chip", name);

	if (chip == CHIP_UNKNOWN)
	{
		// Exit
		lpcio_pio_outb(io, io->reg_port, 0xAA);
		return false;
	}

	lpcio_find_bars(io);
	lpcio_sio_outb(io, LPCIO_DEV_SELECT_REG, ldn);
	uint16_t address = 0;
	lpcio_sio_inw(io, LPCIO_BASE_ADDR_REG1, &address);
	NWL_Debug("LPC", "Address: 0x%04X", address);
	Sleep(100);
	uint16_t verify = 0xFFFF;
	lpcio_sio_inw(io, LPCIO_BASE_ADDR_REG1, &verify);
	NWL_Debug("LPC", "Verify: 0x%04X", verify);

	// Exit
	lpcio_pio_outb(io, io->reg_port, 0xAA);

	if (address != verify)
	{
		NWL_Debug("LPC", "Address verification failed");
		return false;
	}

	// some Fintek chips have address register offset 0x05 added already
	if ((address & 0x07) == 0x05)
		address &= 0xFFF8;

	NWL_Debug("LPC", "Detected at 0x%04X", address);
	if (address < 0x100 || (address & 0xF007) != 0)
	{
		NWL_Debug("LPC", "Invalid address");
		return false;
	}

	slot->chip = chip;
	slot->ldn = ldn;
	slot->name = name;
	slot->id = id;
	slot->revision = revision;
	slot->address = address;
	return true;
}

NWLIB_LPC_DRV lpc_fintek_drv =
{
	.name = "Fintek",
	.detect = detect_fintek,
};
