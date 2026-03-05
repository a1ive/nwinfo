// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdint.h>
#include <ioctl.h>

static inline uint8_t superio_inb(struct wr0_drv_t* drv, uint16_t ioreg, uint8_t reg)
{
	WR0_WrIo8(drv, ioreg, reg);
	return WR0_RdIo8(drv, ioreg + 1);
}

static inline void superio_outb(struct wr0_drv_t* drv, uint16_t ioreg, uint8_t reg, uint8_t val)
{
	WR0_WrIo8(drv, ioreg, reg);
	WR0_WrIo8(drv, ioreg + 1, val);
}

static inline uint16_t superio_inw(struct wr0_drv_t* drv, uint16_t ioreg, uint8_t reg)
{
	uint16_t hi = superio_inb(drv, ioreg, reg);
	uint16_t lo = superio_inb(drv, ioreg, reg + 1);
	return (hi << 8) | lo;
}
