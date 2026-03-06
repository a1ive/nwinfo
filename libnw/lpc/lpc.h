// SPDX-License-Identifier: Unlicense
#pragma once

#include <lpcio.h>
#include "../nwapi.h"

struct _NWLIB_MAINBOARD_INFO;

typedef struct _NWLIB_LPC_DRV
{
	const char* name;
	bool (*detect)(plpcio io, struct _NWLIB_MAINBOARD_INFO* board, struct _NWLIB_LPC_SLOT* slot);
} NWLIB_LPC_DRV;

typedef struct _NWLIB_LPC_SLOT
{
	NWLIB_LPC_DRV* drv;
	uint16_t id;
	uint8_t revision;
	uint8_t ldn;
	uint16_t address;
	uint8_t gpio_ldn;
	uint16_t gpio_address;
	enum CHIP_ID chip;
	const char* name;
} NWLIB_LPC_SLOT;

typedef struct _NWLIB_LPC
{
	struct _NWLIB_MAINBOARD_INFO* board;
	NWLIB_LPC_SLOT slots[LPCIO_SLOT_MAX];
	lpcio io;
} NWLIB_LPC, * PNWLIB_LPC;

#define WINBOND_NUVOTON_HARDWARE_MONITOR_LDN 0x0B
#define F71858_HARDWARE_MONITOR_LDN 0x02
#define FINTEK_HARDWARE_MONITOR_LDN 0x04
#define IT87_ENVIRONMENT_CONTROLLER_LDN 0x04
#define IT8705_GPIO_LDN 0x05
#define IT87XX_GPIO_LDN 0x07

LIBNW_API PNWLIB_LPC NWL_InitLpc(struct _NWLIB_MAINBOARD_INFO* board);
LIBNW_API VOID NWL_FreeLpc(PNWLIB_LPC lpc);
