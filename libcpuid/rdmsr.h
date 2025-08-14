// SPDX-License-Identifier: Unlicense
#pragma once

#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcpuid.h"
#include "asm-bits.h"
#include "libcpuid_util.h"
#include "libcpuid_internal.h"
#include "rdtsc.h"
#include <winring0.h>

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
};

extern struct msr_fn_t msr_fn_intel;
extern struct msr_fn_t msr_fn_amd;
extern struct msr_fn_t msr_fn_centaur;

struct msr_info_t
{
	int cpu_clock;
	struct wr0_drv_t* handle;
	struct cpu_id_t* id;
	struct internal_id_info_t* internal;
};
