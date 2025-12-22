// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdbool.h>

#include "../node.h"

typedef struct
{
	const char* name;
	bool enabled;
	bool (*init)(void);
	void (*get)(PNODE node);
	void (*fini)(void);
} sensor_t;

void NWL_InitSensors(void);
void NWL_FreeSensors(void);
PNODE NWL_GetSensors(PNODE parent);
