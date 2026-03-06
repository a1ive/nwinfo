// SPDX-License-Identifier: Unlicense

#include "lpcio.h"
#include "ioctl.h"
#include "superio.h"
#include "libnw.h"

// https://matrix207.github.io/2012/12/17/monitor-hardware-status

static void check_and_add_bar(plpcio h, uint16_t val, uint16_t val_v)
{
	if (val == val_v && val != 0 && val != 0xFFFF)
	{
		// in fixed range, probably some garbage
		if (val < 0x100)
			return;

		// some Fintek chips have address register offset 0x05 added already
		if ((val & 0x07) == 0x05)
			val &= 0xFFF8;

		// duplicate
		if (h->bars_count > 0 && h->bars[h->bars_count - 1] == val)
			return;

		// duplicate
		if (h->bars_count > 1 && h->bars[h->bars_count - 2] == val)
			return;

		h->bars[h->bars_count] = val;
		h->bars_count++;
		NWL_Debug("LPCIO", "Added %X as BAR", val);
	}
}

static bool find_bars(plpcio h)
{
	uint8_t chip_id = superio_inb(h->drv, h->reg_port, LPCIO_CHIP_ID_REG);

	if (chip_id == 0x00 || chip_id == 0xFF)
	{
		NWL_Debug("LPCIO", "Invalid chip ID %X", chip_id);
		return false;
	}

	NWL_Debug("LPCIO", "Finding BARs for %X", h->reg_port);
	uint16_t vals[256][2];
	for (uint8_t i = 0; i < 0xFF; i++)
	{
		superio_outb(h->drv, h->reg_port, LPCIO_DEV_SELECT_REG, i);
		if (superio_inb(h->drv, h->reg_port, LPCIO_DEV_SELECT_REG) == i)
		{
			vals[i][0] = superio_inw(h->drv, h->reg_port, LPCIO_BASE_ADDR_REG1);
			vals[i][1] = superio_inw(h->drv, h->reg_port, LPCIO_BASE_ADDR_REG2);
		}
	}
	WR0_MicroSleep(1000);
	for (uint8_t i = 0; i < 0xFF; i++)
	{
		superio_outb(h->drv, h->reg_port, LPCIO_DEV_SELECT_REG, i);
		if (superio_inb(h->drv, h->reg_port, LPCIO_DEV_SELECT_REG) == i)
		{
			uint16_t vals_v[2];
			vals_v[0] = superio_inw(h->drv, h->reg_port, LPCIO_BASE_ADDR_REG1);
			vals_v[1] = superio_inw(h->drv, h->reg_port, LPCIO_BASE_ADDR_REG2);
			check_and_add_bar(h, vals[i][0], vals_v[0]);
			check_and_add_bar(h, vals[i][1], vals_v[1]);
		}
	}

	return true;
}

bool lpcio_select_slot(plpcio h, enum LPCIO_CHIP_SLOT slot)
{
	NWL_Debug("LPCIO", "Select slot %d", slot);

	h->reg_port = 0;
	h->bars_count = 0;

	switch (slot)
	{
	case LPCIO_SLOT_0:
		h->reg_port = 0x2e;
		break;
	case LPCIO_SLOT_1:
		h->reg_port = 0x4e;
		break;
	default:
		return false;
	}

	if (h->drv->type == WR0_DRIVER_PAWNIO)
	{
		ULONG64 in = slot;
		if (WR0_ExecPawn(h->drv, &h->drv->pio_lpcio, "ioctl_select_slot", &in, 1, NULL, 0, NULL))
			return false;
		return true;
	}

	return true;
}

bool lpcio_find_bars(plpcio h)
{
	if (h->drv->type == WR0_DRIVER_PAWNIO)
	{
		if (WR0_ExecPawn(h->drv, &h->drv->pio_lpcio, "ioctl_find_bars", NULL, 0, NULL, 0, NULL))
			return false;
		return true;
	}

	if (h->reg_port == 0)
		return false;
	h->bars_count = 0;
	return find_bars(h);
}

bool lpcio_pio_inb(plpcio h, uint16_t port, uint8_t* value)
{
	if (h->drv->type == WR0_DRIVER_PAWNIO)
	{
		ULONG64 in = port;
		ULONG64 out = 0;
		if (WR0_ExecPawn(h->drv, &h->drv->pio_lpcio, "ioctl_pio_inb", &in, 1, &out, 1, NULL))
			return false;
		*value = (uint8_t)out;
		NWL_Debug("LPCIO", "inb 0x%04x 0x%02x", port, *value);
		return true;
	}

	if (h->reg_port == 0)
		return false;
	*value = WR0_RdIo8(h->drv, port);
	NWL_Debug("LPCIO", "inb 0x%04x 0x%02x", port, *value);
	return true;
}

bool lpcio_pio_outb(plpcio h, uint16_t port, uint8_t value)
{
	NWL_Debug("LPCIO", "outb 0x%04x 0x%02x", port, value);

	if (h->drv->type == WR0_DRIVER_PAWNIO)
	{
		ULONG64 in[2] = { port, value };
		if (WR0_ExecPawn(h->drv, &h->drv->pio_lpcio, "ioctl_pio_outb", in, 2, NULL, 0, NULL))
			return false;
		return true;
	}

	if (h->reg_port == 0)
		return false;
	WR0_WrIo8(h->drv, port, value);
	return true;
}

bool lpcio_sio_inb(plpcio h, uint8_t reg, uint8_t* value)
{
	if (h->drv->type == WR0_DRIVER_PAWNIO)
	{
		ULONG64 in = reg;
		ULONG64 out = 0;
		if (WR0_ExecPawn(h->drv, &h->drv->pio_lpcio, "ioctl_superio_inb", &in, 1, &out, 1, NULL))
			return false;
		*value = (uint8_t)out;
		NWL_Debug("LPCIO", "sio inb 0x%02x 0x%02x", reg, *value);
		return true;
	}

	if (h->reg_port == 0)
		return false;
	*value = superio_inb(h->drv, h->reg_port, reg);
	NWL_Debug("LPCIO", "sio inb 0x%02x 0x%02x", reg, *value);
	return true;
}

bool lpcio_sio_outb(plpcio h, uint8_t reg, uint8_t value)
{
	NWL_Debug("LPCIO", "sio outb 0x%02x 0x%02x", reg, value);

	if (h->drv->type == WR0_DRIVER_PAWNIO)
	{
		ULONG64 in[2] = { reg, value };
		if (WR0_ExecPawn(h->drv, &h->drv->pio_lpcio, "ioctl_superio_outb", in, 2, NULL, 0, NULL))
			return false;
		return true;
	}

	if (h->reg_port == 0)
		return false;
	superio_outb(h->drv, h->reg_port, reg, value);
	return true;
}

bool lpcio_sio_inw(plpcio h, uint8_t reg, uint16_t* value)
{
	if (h->drv->type == WR0_DRIVER_PAWNIO)
	{
		ULONG64 in = reg;
		ULONG64 out = 0;
		if (WR0_ExecPawn(h->drv, &h->drv->pio_lpcio, "ioctl_superio_inw", &in, 1, &out, 1, NULL))
			return false;
		*value = (uint16_t)out;
		NWL_Debug("LPCIO", "sio inw 0x%02x 0x%04x", reg, *value);
		return true;
	}

	if (h->reg_port == 0)
		return false;
	*value = superio_inw(h->drv, h->reg_port, reg);
	NWL_Debug("LPCIO", "sio inw 0x%02x 0x%04x", reg, *value);
	return true;
}
