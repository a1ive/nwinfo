// SPDX-License-Identifier: Unlicense

#include "rdmsr.h"

#define MSR_IA32_PERF_STATUS  0x198

#define MSR_IA32_BIOS_SIGN_ID 0x8B
#define MSR_FSB_FREQ          0xCD

#define MSR_ZX_TEMP          0x1423
#define MSR_ZX_TEMP_CRIT     0x1416
#define MSR_ZX_TEMP_MAX      0x1415
#define MSR_ZX_TEMP_CRIT_6B  0x175B
#define MSR_ZX_TEMP_MAX_6B   0x175A

#define MSR_VIA_TEMP_0A_0D   0x1169
#define MSR_VIA_TEMP_0F      0x1423

static inline int
read_centaur_msr(struct wr0_drv_t* handle, uint32_t msr_index, uint8_t highbit, uint8_t lowbit, uint64_t* result)
{
	int err;
	const uint8_t bits = highbit - lowbit + 1;
	ULONG64 in = msr_index;
	ULONG64 out = 0;

	if (highbit > 63 || lowbit > highbit)
		return ERR_INVRANGE;

	if (handle->type == WR0_DRIVER_PAWNIO)
		err = ERR_NOT_IMP;
	else
		err = WR0_RdMsr(handle, msr_index, &out);

	if (err)
		return err;

	/* Show only part of register when a subrange is requested */
	if (bits < 64)
	{
		out >>= lowbit;
		out &= (1ULL << bits) - 1;
	}
	*result = out;

	return 0;
}

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

// https://github.com/deepin-community/kernel/blob/linux-6.6.y/drivers/hwmon/zhaoxin-cputemp.c
static int get_temperature(struct msr_info_t* info)
{
	uint64_t reg = 0;
	switch (info->id->x86.ext_family)
	{
	case 0x06: // VIA
	{
		switch (info->id->x86.ext_model)
		{
		case 0x0A:
		case 0x0D:
			if (read_centaur_msr(info->handle, MSR_VIA_TEMP_0A_0D, 23, 0, &reg))
				return CPU_INVALID_VALUE;
			break;
		case 0x0F:
			if (read_centaur_msr(info->handle, MSR_VIA_TEMP_0F, 23, 0, &reg))
				return CPU_INVALID_VALUE;
			break;
		default:
			return CPU_INVALID_VALUE;
		}
	}
		break;
	case 0x07: // Zhaoxin
		if (read_centaur_msr(info->handle, MSR_ZX_TEMP, 23, 0, &reg))
			return CPU_INVALID_VALUE;
		break;
	default:
		return CPU_INVALID_VALUE;
	}

	return (int)reg;
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
	uint64_t reg;
	if (read_centaur_msr(info->handle, MSR_FSB_FREQ, 2, 0, &reg))
		goto fail;
	switch (reg)
	{
	case 0: return 266.67;
	case 1: return 133.33;
	case 2: return 200.00;
	case 3: return 166.67;
	case 4: return 333.33;
	case 5: return 100.00;
	case 6: return 400.00;
	}
fail:
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

static int get_microcode_ver(struct msr_info_t* info)
{
	uint64_t rev;
	if (read_centaur_msr(info->handle, MSR_IA32_BIOS_SIGN_ID, 63, 32, &rev))
		goto fail;
	return (int)rev;
fail:
	return 0;
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
	.get_microcode_ver = get_microcode_ver,
};
