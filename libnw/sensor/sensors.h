// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "node.h"

typedef struct
{
	const char* name;
	uint64_t flag;
	bool enabled;
	bool (*init)(void);
	void (*get)(PNODE node);
	void (*fini)(void);
} sensor_t;

#define NWL_SENSOR_LHM      (1 << 0)
#define NWL_SENSOR_HWINFO   (1 << 1)
#define NWL_SENSOR_GPUZ     (1 << 2)
#define NWL_SENSOR_CPU      (1 << 3)
#define NWL_SENSOR_DIMM     (1 << 4)
#define NWL_SENSOR_GPU      (1 << 5)
#define NWL_SENSOR_SMART    (1 << 6)
#define NWL_SENSOR_NET      (1 << 7)
#define NWL_SENSOR_IMC      (1 << 8)

void NWL_InitSensors(uint64_t flags);
void NWL_FreeSensors(void);
PNODE NWL_GetSensors(PNODE parent);
