// SPDX-License-Identifier: Unlicense

#include "rdmsr.h"

/*
  Intel 64 and IA-32 Architectures Software Developer's Manual
  * Volume 3 (3A, 3B, 3C & 3D): System Programming Guide
  http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-system-programming-manual-325384.pdf
*/

#define MSR_IA32_PERF_STATUS 0x198
#define MSR_IA32_THERM_STATUS 0x19C
#define MSR_IA32_PACKAGE_THERM_STATUS 0x1B1
#define MSR_IA32_EBL_CR_POWERON 0x2A
#define MSR_TURBO_RATIO_LIMIT 0x1AD
#define MSR_IA32_TEMPERATURE_TARGET 0x1A2
#define MSR_IA32_PERF_STATUS        0x198
#define MSR_RAPL_POWER_UNIT    0x606
#define MSR_PKG_POWER_LIMIT    0x610
#define MSR_PKG_ENERGY_STATUS  0x611
#define MSR_PP0_ENERGY_STATUS  0x639
#define MSR_PP1_ENERGY_STATUS  0x641
#define MSR_PLATFORM_INFO      0xCE
#define MSR_IA32_BIOS_SIGN_ID  0x8B

static inline int
read_intel_msr(struct wr0_drv_t* handle, uint32_t msr_index, uint8_t highbit, uint8_t lowbit, uint64_t* result)
{
	int err;
	const uint8_t bits = highbit - lowbit + 1;
	ULONG64 in = msr_index;
	ULONG64 out = 0;

	if (highbit > 63 || lowbit > highbit)
		return ERR_INVRANGE;

	if (handle->type == WR0_DRIVER_PAWNIO)
		err = WR0_ExecPawn(handle, &handle->pio_intel, "ioctl_read_msr", &in, 1, &out, 1, NULL);
	else
		err = WR0_RdMsr(handle, msr_index, &out);

	if (!err && bits < 64)
	{
		/* Show only part of register */
		out >>= lowbit;
		out &= (1ULL << bits) - 1;
		*result = out;
	}

	return err;
}

static double get_min_multiplier(struct msr_info_t* info)
{
	uint64_t reg;
	if (info->id->x86.ext_family < 6)
		goto fail;
	if (read_intel_msr(info->handle, MSR_PLATFORM_INFO, 47, 40, &reg))
		goto fail;
	return (double)reg;
fail:
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_cur_multiplier(struct msr_info_t* info)
{
	uint64_t reg;
	if (info->id->x86.ext_family < 6)
		goto fail;
	if (read_intel_msr(info->handle, MSR_IA32_PERF_STATUS, 15, 8, &reg))
		goto fail;
	return (double)reg;
fail:
	if (!read_intel_msr(info->handle, MSR_IA32_EBL_CR_POWERON, 63, 0, &reg))
		return (double)((reg >> 22) & 0x1f);
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_max_multiplier(struct msr_info_t* info)
{
	/* Refer links above
		Table 35-10.  Specific MSRs Supported by Intel Atom Processor C2000 Series with CPUID Signature 06_4DH
		Table 35-12.  MSRs in Next Generation Intel Atom Processors Based on the Goldmont Microarchitecture (Contd.)
		Table 35-13.  MSRs in Processors Based on Intel Microarchitecture Code Name Nehalem (Contd.)
		Table 35-14.  Additional MSRs in Intel Xeon Processor 5500 and 3400 Series
		Table 35-16.  Additional MSRs Supported by Intel Processors (Based on Intel Microarchitecture Code Name Westmere)
		Table 35-19.  MSRs Supported by 2nd Generation Intel Core Processors (Intel microarchitecture code name Sandy Bridge)
		Table 35-21.  Selected MSRs Supported by Intel Xeon Processors E5 Family (based on Sandy Bridge microarchitecture)
		Table 35-28.  MSRs Supported by 4th Generation Intel Core Processors (Haswell microarchitecture) (Contd.)
		Table 35-30.  Additional MSRs Supported by Intel Xeon Processor E5 v3 Family
		Table 35-33.  Additional MSRs Supported by Intel Core M Processors and 5th Generation Intel Core Processors
		Table 35-34.  Additional MSRs Common to Intel Xeon Processor D and Intel Xeon Processors E5 v4 Family Based on the Broadwell Microarchitecture
		Table 35-37.  Additional MSRs Supported by 6th Generation Intel Core Processors Based on Skylake Microarchitecture
		Table 35-40.  Selected MSRs Supported by Next Generation Intel Xeon Phi Processors with DisplayFamily_DisplayModel Signature 06_57H
		MSR_TURBO_RATIO_LIMIT[7:0] is Maximum Ratio Limit for 1C
	*/
	uint64_t reg;
	if (info->id->x86.ext_family < 6)
		goto fail;
	if (read_intel_msr(info->handle, MSR_TURBO_RATIO_LIMIT, 7, 0, &reg))
		goto fail;
	return (double)reg;

fail:
	if (!read_intel_msr(info->handle, MSR_IA32_PERF_STATUS, 63, 0, &reg))
		return (double)((reg >> 40) & 0x1f);
	return (double)CPU_INVALID_VALUE / 100;
}

static int get_temperature(struct msr_info_t* info)
{
	uint64_t delta, read_valid, tj;

	if (!info->id->flags[CPU_FEATURE_INTEL_DTS])
		goto fail;
	/* Refer links above
		Table 35-2.   IA-32 Architectural MSRs
		MSR_IA32_THERM_STATUS[22:16] is Digital Readout
		MSR_IA32_THERM_STATUS[31]    is Reading Valid

		Table 35-6.   MSRs Common to the Silvermont Microarchitecture and Newer Microarchitectures for Intel Atom
		Table 35-13.  MSRs in Processors Based on Intel Microarchitecture Code Name Nehalem (Contd.)
		Table 35-18.  MSRs Supported by Intel Processors based on Intel microarchitecture code name Sandy Bridge (Contd.)
		Table 35-24.  MSRs Supported by Intel Xeon Processors E5 v2 Product Family (based on Ivy Bridge-E microarchitecture) (Contd.)
		Table 35-34.  Additional MSRs Common to Intel Xeon Processor D and Intel Xeon Processors E5 v4 Family Based on the Broadwell Microarchitecture
		Table 35-40.  Selected MSRs Supported by Next Generation Intel Xeon Phi Processors with DisplayFamily_DisplayModel Signature 06_57H
		MSR_IA32_TEMPERATURE_TARGET[23:16] is Temperature Target
	*/
	if (read_intel_msr(info->handle, MSR_IA32_THERM_STATUS, 22, 16, &delta))
		goto fail;
	if (read_intel_msr(info->handle, MSR_IA32_THERM_STATUS, 31, 31, &read_valid))
		goto fail;
	if (read_intel_msr(info->handle, MSR_IA32_TEMPERATURE_TARGET, 23, 16, &tj))
		goto fail;
	if (read_valid)
		return (int)(tj - delta);

fail:
	return CPU_INVALID_VALUE;
}

static int get_pkg_temperature(struct msr_info_t* info)
{
	uint64_t delta, tj;
	if (!info->id->flags[CPU_FEATURE_INTEL_PTM])
		goto fail;
	if (read_intel_msr(info->handle, MSR_IA32_PACKAGE_THERM_STATUS, 22, 16, &delta))
		goto fail;
	if (read_intel_msr(info->handle, MSR_IA32_TEMPERATURE_TARGET, 23, 16, &tj))
		goto fail;
	return (int)(tj - delta);
fail:
	return CPU_INVALID_VALUE;
}

static double get_pkg_energy(struct msr_info_t* info)
{
	uint64_t total_energy, energy_units;
	if (read_intel_msr(info->handle, MSR_PKG_ENERGY_STATUS, 31, 0, &total_energy))
		goto fail;
	if (read_intel_msr(info->handle, MSR_RAPL_POWER_UNIT, 12, 8, &energy_units))
		goto fail;
	return (double)total_energy / (1ULL << energy_units);
fail:
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_pkg_pl1(struct msr_info_t* info)
{
	uint64_t pl, pu;
	if (read_intel_msr(info->handle, MSR_PKG_POWER_LIMIT, 14, 0, &pl))
		goto fail;
	if (read_intel_msr(info->handle, MSR_RAPL_POWER_UNIT, 3, 0, &pu))
		goto fail;
	return (double)pl / (1ULL << pu);
fail:
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_pkg_pl2(struct msr_info_t* info)
{
	uint64_t pl, pu;
	if (read_intel_msr(info->handle, MSR_PKG_POWER_LIMIT, 46, 32, &pl))
		goto fail;
	if (read_intel_msr(info->handle, MSR_RAPL_POWER_UNIT, 3, 0, &pu))
		goto fail;
	return (double)pl / (1ULL << pu);
fail:
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_voltage(struct msr_info_t* info)
{
	/* Refer links above
		Table 35-18.  MSRs Supported by Intel Processors based on Intel microarchitecture code name Sandy Bridge (Contd.)
		MSR_IA32_PERF_STATUS[47:32] is Core Voltage
		P-state core voltage can be computed by MSR_IA32_PERF_STATUS[37:32] * (float) 1/(2^13).
	*/
	uint64_t reg;
	if (info->id->x86.ext_family < 6)
		goto fail;
	if (read_intel_msr(info->handle, MSR_IA32_PERF_STATUS, 47, 32, &reg))
		goto fail;
	return (double)reg / (1ULL << 13ULL);
fail:
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_bus_clock(struct msr_info_t* info)
{
	/* Refer links above
		Table 35-12.  MSRs in Next Generation Intel Atom Processors Based on the Goldmont Microarchitecture
		Table 35-13.  MSRs in Processors Based on Intel Microarchitecture Code Name Nehalem
		Table 35-18.  MSRs Supported by Intel Processors based on Intel microarchitecture code name Sandy Bridge (Contd.)
		Table 35-23.  Additional MSRs Supported by 3rd Generation Intel Core Processors (based on Intel microarchitecture code name Ivy Bridge)
		Table 35-24.  MSRs Supported by Intel Xeon Processors E5 v2 Product Family (based on Ivy Bridge-E microarchitecture)
		Table 35-27.  Additional MSRs Supported by Processors based on the Haswell or Haswell-E microarchitectures
		Table 35-40.  Selected MSRs Supported by Next Generation Intel Xeon Phi Processors with DisplayFamily_DisplayModel Signature 06_57H
		MSR_PLATFORM_INFO[15:8] is Maximum Non-Turbo Ratio
	*/
	uint64_t reg;
	if (info->id->x86.ext_family < 6)
		goto fail;
	if (read_intel_msr(info->handle, MSR_PLATFORM_INFO, 15, 8, &reg))
		goto fail;
	return (double)info->cpu_clock / reg;
fail:
	return (double)CPU_INVALID_VALUE / 100;
}

static int get_igpu_temperature(struct msr_info_t* info)
{
	return CPU_INVALID_VALUE;
}

static double get_igpu_energy(struct msr_info_t* info)
{
	uint64_t total_energy, energy_units;
	if (read_intel_msr(info->handle, MSR_PP1_ENERGY_STATUS, 31, 0, &total_energy))
		goto fail;
	if (read_intel_msr(info->handle, MSR_RAPL_POWER_UNIT, 12, 8, &energy_units))
		goto fail;
	return (double)total_energy / (1ULL << energy_units);
fail:
	return (double)CPU_INVALID_VALUE / 100;
}

static int get_microcode_ver(struct msr_info_t* info)
{
	uint64_t rev;
	if (read_intel_msr(info->handle, MSR_IA32_BIOS_SIGN_ID, 63, 32, &rev))
		goto fail;
	return (int)rev;
fail:
	return 0;
}

struct msr_fn_t msr_fn_intel =
{
	.get_min_multiplier = get_min_multiplier,
	.get_cur_multiplier = get_cur_multiplier,
	.get_max_multiplier = get_max_multiplier,
	.get_temperature = get_temperature,
	.get_pkg_temperature = get_pkg_temperature,
	.get_pkg_energy = get_pkg_energy,
	.get_pkg_pl1 = get_pkg_pl1,
	.get_pkg_pl2 = get_pkg_pl2,
	.get_voltage = get_voltage,
	.get_bus_clock = get_bus_clock,
	.get_igpu_temperature = get_igpu_temperature,
	.get_igpu_energy = get_igpu_energy,
	.get_microcode_ver = get_microcode_ver,
};
