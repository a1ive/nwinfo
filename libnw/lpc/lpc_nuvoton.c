// SPDX-License-Identifier: Unlicense

#include <lpcio.h>
#include <ioctl.h>
#include <superio.h>
#include <libnw.h>
#include <chip_ids.h>
#include <mb_vendor.h>

#include "lpc.h"

#define NUVOTON_HARDWARE_MONITOR_IO_SPACE_LOCK 0x28

static bool detect_nuvoton(plpcio io, NWLIB_MAINBOARD_INFO* board, NWLIB_LPC_SLOT* slot)
{
	// Enter
	lpcio_pio_outb(io, io->reg_port, 0x87);
	lpcio_pio_outb(io, io->reg_port, 0x87);

	uint8_t id = 0;
	uint8_t revision = 0;
	enum CHIP_ID chip = CHIP_UNKNOWN;
	uint8_t ldn = WINBOND_NUVOTON_HARDWARE_MONITOR_LDN;
	const char* name = "Nuvoton";

	lpcio_sio_inb(io, LPCIO_CHIP_ID_REG, &id);
	lpcio_sio_inb(io, LPCIO_CHIP_REV_REG, &revision);
	NWL_Debug("LPC", "Chip id: %02X, rev: %02X", id, revision);

	switch (id)
	{
	case 0xB4:
		switch (revision & 0xF0)
		{
		case 0x70:
			chip = CHIP_NCT6771F;
			break;
		}
		break;
	case 0xC3:
		switch (revision & 0xF0)
		{
		case 0x30:
			chip = CHIP_NCT6776F;
			break;
		}
		break;
	case 0xC4:
		switch (revision & 0xF0)
		{
		case 0x50:
			chip = CHIP_NCT610XD;
			break;
		}
		break;
	case 0xC5:
		switch (revision & 0xF0)
		{
		case 0x60:
			chip = CHIP_NCT6779D;
			break;
		}
		break;
	case 0xC7:
		switch (revision)
		{
		case 0x32:
			chip = CHIP_NCT6683D;
			break;
		}
		break;
	case 0xC8:
		switch (revision)
		{
		case 0x03:
			chip = CHIP_NCT6791D;
			break;
		}
		break;
	case 0xC9:
		switch (revision)
		{
		case 0x11:
			chip = CHIP_NCT6792D;
			break;
		case 0x13:
			chip = CHIP_NCT6792DA;
			break;
		}
		break;
	case 0xD1:
		switch (revision)
		{
		case 0x21:
			chip = CHIP_NCT6793D;
			break;
		}
		break;
	case 0xD3:
		switch (revision)
		{
		case 0x52:
			chip = CHIP_NCT6795D;
			break;
		}
		break;
	case 0xD4:
		switch (revision)
		{
		case 0x23:
			chip = CHIP_NCT6796D;
			break;
		case 0x2A:
			if (_stricmp(board->ProductStr, "X870E Nova WiFi") == 0)
				chip = CHIP_NCT5585D;
			else
				chip = CHIP_NCT6796DR;
			break;
		case 0x51:
			chip = CHIP_NCT6797D;
			break;
		case 0x2B:
			chip = CHIP_NCT6798D;
			break;
		case 0x40:
		case 0x41:
			chip = CHIP_NCT6686D;
			break;
		}
		break;
	case 0xD5:
		switch (revision)
		{
		case 0x92:
			// MSI AM5/LGA1851 800 Series Motherboard Compatibility (Nuvoton NCT6687DR)
			if (board->VendorId == VENDOR_MSI &&
				(strcmp(board->Chipset, "B840") == 0 ||
				 strcmp(board->Chipset, "B850") == 0 ||
				 strcmp(board->Chipset, "B860") == 0 ||
				 strcmp(board->Chipset, "X870") == 0 ||
				 strcmp(board->Chipset, "Z890") == 0))
				chip = CHIP_NCT6687DR;
			else
				chip = CHIP_NCT6687D;
			break;
		}

		break;
	case 0xD8:
		switch (revision)
		{
		case 0x02:
			if (_stricmp(board->ProductStr, "X870E Nova WiFi") == 0)
				chip = CHIP_NCT6796DS;
			else
				chip = CHIP_NCT6799D;
			break;
		case 0x06:
			chip = CHIP_NCT6701D;
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

	// Disable the hardware monitor i/o space lock on NCT679XD chips
	switch (chip)
	{
		case CHIP_NCT6791D:
		case CHIP_NCT6792D:
		case CHIP_NCT6792DA:
		case CHIP_NCT6793D:
		case CHIP_NCT6795D:
		case CHIP_NCT6796D:
		case CHIP_NCT6796DR:
		case CHIP_NCT6796DS:
		case CHIP_NCT6798D:
		case CHIP_NCT6797D:
		case CHIP_NCT6799D:
		case CHIP_NCT6701D:
			if (address == verify)
			{
				uint8_t options = 0;
				lpcio_sio_inb(io, NUVOTON_HARDWARE_MONITOR_IO_SPACE_LOCK, &options);
				if ((options & 0x10) > 0)
					lpcio_sio_outb(io, NUVOTON_HARDWARE_MONITOR_IO_SPACE_LOCK, options & ~0x10);
			}
			break;
	}

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

NWLIB_LPC_DRV lpc_nuvoton_drv =
{
	.name = "Nuvoton",
	.detect = detect_nuvoton,
};
