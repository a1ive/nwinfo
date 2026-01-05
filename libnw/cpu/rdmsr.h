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
#include <winring0.h>

typedef enum
{
	INFO_MIN_MULTIPLIER,       /*!< Minimum CPU:FSB ratio for this CPU,
									multiplied by 100. */
	INFO_CUR_MULTIPLIER,       /*!< Current CPU:FSB ratio, multiplied by 100.
									e.g., a CPU:FSB value of 18.5 reads as
									"1850". */
	INFO_MAX_MULTIPLIER,       /*!< Maximum CPU:FSB ratio for this CPU,
									multiplied by 100. */
	INFO_TEMPERATURE,          /*!< The current core temperature in Celsius. */
	INFO_PKG_TEMPERATURE,      /*!< The current package temperature in Celsius. */
	INFO_THROTTLING,           /*!< 1 if the current logical processor is
									throttling. 0 if it is running normally. */
	INFO_VOLTAGE,              /*!< The current core voltage in Volt,
									multiplied by 100. */
	INFO_PKG_ENERGY,           /*!< The current package energy consumption in Joules,
									multiplied by 100. */
	INFO_PKG_PL1,              /*!< The current package power limit #1 in Watts,
									multiplied by 100. */
	INFO_PKG_PL2,              /*!< The current package power limit #2 in Watts,
									multiplied by 100. */
	INFO_BCLK,                 /*!< See \ref INFO_BUS_CLOCK. */
	INFO_BUS_CLOCK,            /*!< The main bus clock in MHz,
									e.g., FSB/QPI/DMI/HT base clock,
									multiplied by 100. */
	INFO_IGPU_TEMPERATURE,     /*!< The current integrated GPU temperature in Celsius. */
	INFO_IGPU_ENERGY,          /*!< The current integrated GPU energy consumption in Joules,
									multiplied by 100. */
	INFO_MICROCODE_VER,        /*!< The microcode revision number */
	INFO_TDP_NOMINAL,          /*!< The TDP in Watts. */
} cpu_msrinfo_request_t;

/**
 * @brief Reads extended CPU information from Model-Specific Registers.
 * @param handle - a handle to an open MSR driver, @see WR0_OpenDriver
 * @param cpu - the logical CPU number, where the MSR is read.
 * @param which - which info field should be returned. A list of
 *                available information entities is listed in the
 *                cpu_msrinfo_request_t enum.
 * @retval - if the requested information is available for the current
 *           processor model, the respective value is returned.
 *           if no information is available, or the CPU doesn't support
 *           the query, the special value CPU_INVALID_VALUE is returned
 * @note This function is not MT-safe. If you intend to call it from multiple
 *       threads, guard it through a mutex or a similar primitive.
 */
int cpu_msrinfo(struct wr0_drv_t* handle, logical_cpu_t cpu, cpu_msrinfo_request_t which);
#define CPU_INVALID_VALUE 0x3fffffff

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
	int cpu_clock;
	struct wr0_drv_t* handle;
	struct cpu_id_t* id;
	struct internal_id_info_t* internal;
};
