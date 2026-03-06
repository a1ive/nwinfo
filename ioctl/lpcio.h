// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdint.h>
#include <stdbool.h>

struct wr0_drv_t;

enum LPCIO_CHIP_SLOT
{
	LPCIO_SLOT_0 = 0,
	LPCIO_SLOT_1 = 1,
	LPCIO_SLOT_MAX,
};

#define LPCIO_MAX_BARS 128

typedef struct _lpcio
{
	struct wr0_drv_t* drv;
	uint16_t reg_port;
	uint32_t bars_count;
	uint16_t bars[LPCIO_MAX_BARS];
} lpcio, *plpcio;

#define LPCIO_CHIP_ID_REG 0x20
#define LPCIO_CHIP_REV_REG 0x21
#define LPCIO_BASE_ADDR_REG1 0x60
#define LPCIO_BASE_ADDR_REG2 0x62
#define LPCIO_DEV_SELECT_REG 0x07

bool lpcio_select_slot(plpcio h, enum LPCIO_CHIP_SLOT slot);

bool lpcio_find_bars(plpcio h);

bool lpcio_pio_inb(plpcio h, uint16_t port, uint8_t* value);

bool lpcio_pio_outb(plpcio h, uint16_t port, uint8_t value);

bool lpcio_sio_inb(plpcio h, uint8_t reg, uint8_t* value);

bool lpcio_sio_outb(plpcio h, uint8_t reg, uint8_t value);

bool lpcio_sio_inw(plpcio h, uint8_t reg, uint16_t* value);
