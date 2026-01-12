// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcpuid.h>
#include <winring0.h>
#include "ryzen_smu.h"
#include "../nwapi.h"

typedef enum
{
	INFO_MIN_MULTIPLIER,       // Minimum CPU:FSB ratio * 100
	INFO_CUR_MULTIPLIER,       // Current CPU:FSB ratio * 100
	INFO_MAX_MULTIPLIER,       // Maximum CPU:FSB ratio * 100
	INFO_TEMPERATURE,          // Core temperature in Celsius
	INFO_PKG_TEMPERATURE,      // Package temperature in Celsius
	INFO_VOLTAGE,              // Core voltage in Volt * 100
	INFO_PKG_ENERGY,           // Package energy consumption in Joules * 100.
	INFO_PKG_POWER,            // Package power in Watts * 100.
	INFO_PKG_PL1,              // Package power limit #1 in Watts * 100
	INFO_PKG_PL2,              // Package power limit #2 in Watts * 100
	INFO_BUS_CLOCK,            // Bus clock in MHz * 100
	INFO_IGPU_TEMPERATURE,     // Integrated GPU temperature in Celsius
	INFO_IGPU_ENERGY,          // Integrated GPU energy consumption in Joules * 100
	INFO_MICROCODE_VER,        // Microcode revision number
	INFO_TDP_NOMINAL,          // TDP in Watts
} cpu_msrinfo_request_t;

struct msr_fn_t
{
	double (*get_min_multiplier)(struct msr_info_t* info);
	double (*get_cur_multiplier)(struct msr_info_t* info);
	double (*get_max_multiplier)(struct msr_info_t* info);
	int (*get_temperature)(struct msr_info_t* info);
	int (*get_pkg_temperature)(struct msr_info_t* info);
	double (*get_pkg_energy)(struct msr_info_t* info);
	double (*get_pkg_pl1)(struct msr_info_t* info);
	double (*get_pkg_pl2)(struct msr_info_t* info);
	double (*get_voltage)(struct msr_info_t* info);
	double (*get_bus_clock)(struct msr_info_t* info);
	int (*get_igpu_temperature)(struct msr_info_t* info);
	double (*get_igpu_energy)(struct msr_info_t* info);
	int (*get_microcode_ver)(struct msr_info_t* info);
	int (*get_tdp_nominal)(struct msr_info_t* info);
};

struct msr_info_t
{
	int valid;
	int clock;
	int pkg_energy;
	char name[32];
	ULONGLONG ticks;
	struct wr0_drv_t* handle;
	struct cpu_id_t* id;
	struct msr_fn_t* fn;
	GROUP_AFFINITY aff;
	ry_handle_t* ry;

	int cached_min_multiplier;
	int cached_max_multiplier;
	int cached_pl1;
	int cached_pl2;
	int cached_microcode;
	int cached_tdp;
};

void NWL_GetCpuIndexStr(struct cpu_id_t* id, char* buf, size_t buf_len);
LIBNW_API bool NWL_MsrInit(struct msr_info_t* info, struct wr0_drv_t* drv, struct cpu_id_t* id);
LIBNW_API int NWL_MsrGet(struct msr_info_t* info, cpu_msrinfo_request_t which);
LIBNW_API void NWL_MsrFini(struct msr_info_t* info);
