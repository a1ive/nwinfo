// SPDX-License-Identifier: Unlicense

#include "rdmsr.h"

#define MSR_VIA_TEMP_F6_C7   0x1169
#define MSR_VIA_TEMP_F7_NANO 0x1423
#define MSR_VIA_INDEX_F7_6B  0x174c
#define MSR_VIA_TEMP_F7_6B   0x174d
#define MSR_VIA_VID_F6_C7    0x198

static double get_min_multiplier(struct msr_info_t* info)
{
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_cur_multiplier(struct msr_info_t* info)
{
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_max_multiplier(struct msr_info_t* info)
{
	return (double)CPU_INVALID_VALUE / 100;
}

static int get_temperature(struct msr_info_t* info)
{
	uint64_t reg;
	uint32_t addr = 0;
	if (info->id->x86.ext_family == 0x07)
	{
		if (info->id->x86.ext_model == 0x6b)
			addr = MSR_VIA_TEMP_F7_6B;
		else
			addr = MSR_VIA_TEMP_F7_NANO;
	}
	else if (info->id->x86.ext_family == 0x06)
	{
		switch (info->id->x86.ext_model)
		{
		case 0x0a:
		case 0x0d:
			addr = MSR_VIA_TEMP_F6_C7;
			break;
		case 0x0f:
			addr = MSR_VIA_TEMP_F7_NANO;
			break;
		}
	}
	if (addr == 0)
		goto fail;
	if (addr == MSR_VIA_TEMP_F7_6B)
	{
		if (WR0_WrMsr(info->handle, MSR_VIA_INDEX_F7_6B, 0x19, 0))
			goto fail;
	}
	if (cpu_rdmsr_range(info->handle, addr, 23, 0, &reg))
		goto fail;
	return (int)(reg);
fail:
	return CPU_INVALID_VALUE;
}

static int get_pkg_temperature(struct msr_info_t* info)
{
	return CPU_INVALID_VALUE;
}

static double get_pkg_energy(struct msr_info_t* info)
{
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_pkg_pl1(struct msr_info_t* info)
{
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_pkg_pl2(struct msr_info_t* info)
{
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_voltage(struct msr_info_t* info)
{
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_bus_clock(struct msr_info_t* info)
{
	return (double)CPU_INVALID_VALUE / 100;
}

static int get_igpu_temperature(struct msr_info_t* info)
{
	return CPU_INVALID_VALUE;
}

static double get_igpu_energy(struct msr_info_t* info)
{
	return (double)CPU_INVALID_VALUE / 100;
}

struct msr_fn_t msr_fn_centaur =
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
};
