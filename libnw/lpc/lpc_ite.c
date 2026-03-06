// SPDX-License-Identifier: Unlicense

#include <lpcio.h>
#include <ioctl.h>
#include <superio.h>
#include <libnw.h>
#include <chip_ids.h>
#include <mb_vendor.h>

#include "lpc.h"

#define CONFIGURATION_CONTROL_REGISTER 0x02
#define IT87_CHIP_VERSION_REGISTER 0x22

static inline void it87_enter(plpcio io)
{
	lpcio_pio_outb(io, io->reg_port, 0x87);
	lpcio_pio_outb(io, io->reg_port, 0x01);
	lpcio_pio_outb(io, io->reg_port, 0x55);
	if (io->reg_port == 0x4E)
		lpcio_pio_outb(io, io->reg_port, 0xAA);
	else
		lpcio_pio_outb(io, io->reg_port, 0x55);
}

static inline void it87_exit(plpcio io)
{
	if (io->reg_port != 0x4E)
		lpcio_sio_outb(io, CONFIGURATION_CONTROL_REGISTER, 0x02);
}

static bool detect_ite(plpcio io, NWLIB_MAINBOARD_INFO* board, NWLIB_LPC_SLOT* slot)
{
	uint16_t id = 0xFFFF;
	uint8_t version = 0;
	enum CHIP_ID chip = CHIP_UNKNOWN;
	uint8_t ldn = IT87_ENVIRONMENT_CONTROLLER_LDN;
	uint8_t gpio_ldn = IT87XX_GPIO_LDN;
	const char* name = "ITE";

	// IT87XX can enter only on port 0x2E
	// IT8792 using 0x4E
	if (io->reg_port != 0x2E && io->reg_port != 0x4E)
		return false;

	// Read the chip ID before entering.
	// If already entered (not 0xFFFF) and the register port is 0x4E, it is most likely bugged and should be left alone.
	// Entering IT8792 in this state will result in IT8792 reporting with chip ID of 0x8883.
	
	if (io->reg_port != 0x4E)
	{
		it87_enter(io);
		lpcio_sio_inw(io, LPCIO_CHIP_ID_REG, &id);
	}
	else
	{
		lpcio_sio_inw(io, LPCIO_CHIP_ID_REG, &id);
		if (id != 0xFFFF)
		{
			it87_enter(io);
			lpcio_sio_inw(io, LPCIO_CHIP_ID_REG, &id);
		}
	}

	NWL_Debug("LPC", "Chip id: %04X", id);

	switch (id)
	{
		case 0x8613: chip = CHIP_IT8613E; break;
		case 0x8620: chip = CHIP_IT8620E; break;
		case 0x8625: chip = CHIP_IT8625E; break;
		case 0x8628: chip = CHIP_IT8628E; break;
		case 0x8631: chip = CHIP_IT8631E; break;
		case 0x8638: chip = CHIP_IT8638E; break;
		case 0x8665: chip = CHIP_IT8665E; break;
		case 0x8655: chip = CHIP_IT8655E; break;
		case 0x8686: chip = CHIP_IT8686E; break;
		case 0x8688: chip = CHIP_IT8688E; break;
		case 0x8689: chip = CHIP_IT8689E; break;
		case 0x8696: chip = CHIP_IT8696E; break;
		case 0x8705:
			chip = CHIP_IT8705F;
			gpio_ldn = IT8705_GPIO_LDN;
			break;
		case 0x8712: chip = CHIP_IT8712F; break;
		case 0x8716: chip = CHIP_IT8716F; break;
		case 0x8718: chip = CHIP_IT8718F; break;
		case 0x8720: chip = CHIP_IT8720F; break;
		case 0x8721: chip = CHIP_IT8721F; break;
		case 0x8726: chip = CHIP_IT8726F; break;
		case 0x8728: chip = CHIP_IT8728F; break;
		case 0x8771: chip = CHIP_IT8771E; break;
		case 0x8772: chip = CHIP_IT8772E; break;
		case 0x8790: chip = CHIP_IT8790E; break;
		case 0x8733: chip = CHIP_IT8792E; break;
		case 0x8695: chip = CHIP_IT87952E; break;
	}

	name = CHIP_ID_TO_NAME(chip);
	NWL_Debug("LPC", "Identified %s chip", name);

	if (chip == CHIP_UNKNOWN)
	{
		it87_exit(io);
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

	lpcio_pio_inb(io, IT87_CHIP_VERSION_REGISTER, &version);
	version &= 0x0F;
	NWL_Debug("LPC", "Version: 0x%02X", version);

	lpcio_sio_outb(io, LPCIO_DEV_SELECT_REG, gpio_ldn);
	uint16_t gpio_address = 0;
	lpcio_sio_inw(io, LPCIO_BASE_ADDR_REG1, &gpio_address);
	NWL_Debug("LPC", "GPIOAddress: 0x%04X", gpio_address);
	Sleep(100);
	uint16_t gpio_verify = 0xFFFF;
	lpcio_sio_inw(io, LPCIO_BASE_ADDR_REG1, &gpio_verify);
	NWL_Debug("LPC", "GPIO Verify: 0x%04X", gpio_verify);

	it87_exit(io);

	if (address != verify || address < 0x100 || (address & 0xF007) != 0)
	{
		NWL_Debug("LPC", "Address verification failed");
		return false;
	}

	if (gpio_address != gpio_verify || gpio_address < 0x100 || (gpio_address & 0xF007) != 0)
	{
		NWL_Debug("LPC", "GPIO address verification failed");
		return false;
	}

	slot->chip = chip;
	slot->ldn = ldn;
	slot->gpio_ldn = gpio_ldn;
	slot->name = name;
	slot->id = id;
	slot->revision = version;
	slot->address = address;
	slot->gpio_address = gpio_address;
	return true;
}

NWLIB_LPC_DRV lpc_ite_drv =
{
	.name = "ITE",
	.detect = detect_ite,
};
