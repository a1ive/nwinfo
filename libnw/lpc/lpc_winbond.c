// SPDX-License-Identifier: Unlicense

#include <lpcio.h>
#include <ioctl.h>
#include <superio.h>
#include <libnw.h>
#include <chip_ids.h>
#include <mb_vendor.h>

#include "lpc.h"

static bool detect_winbond(plpcio io, NWLIB_MAINBOARD_INFO* board, NWLIB_LPC_SLOT* slot)
{
	// Enter
	lpcio_pio_outb(io, io->reg_port, 0x87);
	lpcio_pio_outb(io, io->reg_port, 0x87);

	uint8_t id = 0;
	uint8_t revision = 0;
	enum CHIP_ID chip = CHIP_UNKNOWN;
	uint8_t ldn = WINBOND_NUVOTON_HARDWARE_MONITOR_LDN;
	const char* name = "Winbond";

	lpcio_sio_inb(io, LPCIO_CHIP_ID_REG, &id);
	lpcio_sio_inb(io, LPCIO_CHIP_REV_REG, &revision);
	NWL_Debug("LPC", "Chip id: %02X, rev: %02X", id, revision);

	switch (id)
	{
	case 0x52:
		switch (revision)
		{
		case 0x17:
		case 0x3A:
		case 0x41:
			chip = CHIP_W83627HF;
			break;
		}
		break;
	case 0x82:
		switch (revision & 0xF0)
		{
		case 0x80:
			chip = CHIP_W83627THF;
			break;
		}
		break;
	case 0x85:
		switch (revision)
		{
		case 0x41:
			chip = CHIP_W83687THF;
			break;
		}
		break;
	case 0x88:
		switch (revision & 0xF0)
		{
		case 0x50:
		case 0x60:
			chip = CHIP_W83627EHF;
			break;
		}
		break;
	case 0xA0:
		switch (revision & 0xF0)
		{
		case 0x20:
			chip = CHIP_W83627DHG;
			break;
		}
		break;
	case 0xA5:
		switch (revision & 0xF0)
		{
		case 0x10:
			chip = CHIP_W83667HG;
			break;
		}
		break;
	case 0xB0:
		switch (revision & 0xF0)
		{
		case 0x70:
			chip = CHIP_W83627DHGP;
			break;
		}
		break;
	case 0xB3:
		switch (revision & 0xF0)
		{
		case 0x50:
			chip = CHIP_W83667HGB;
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

NWLIB_LPC_DRV lpc_winbond_drv =
{
	.name = "Winbond",
	.detect = detect_winbond,
};
