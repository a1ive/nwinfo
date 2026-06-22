// SPDX-License-Identifier: Unlicense

#include "rdmsr.h"

#define MSR_IA32_PERF_STATUS  0x198

#define MSR_IA32_BIOS_SIGN_ID 0x8B

#define MSR_ZX_TEMP          0x1423
#define MSR_ZX_TEMP_CRIT     0x1416
#define MSR_ZX_TEMP_MAX      0x1415
#define MSR_ZX_TEMP_CRIT_6B  0x175B
#define MSR_ZX_TEMP_MAX_6B   0x175A

#define MSR_VIA_TEMP_0A_0D   0x1169
#define MSR_VIA_TEMP_0F      0x1423

static inline int
read_centaur_msr(struct msr_info_t* info, uint32_t msr_index, uint8_t highbit, uint8_t lowbit, uint64_t* result)
{
	int err = ERR_NOT_IMP;
	const uint8_t bits = highbit - lowbit + 1;
	ULONG64 in = msr_index;
	ULONG64 out = 0;

	if (highbit > 63 || lowbit > highbit)
		return ERR_INVRANGE;

	if (info->handle->type == WR0_DRIVER_PAWNIO)
	{
		if (info->id->x86.ext_family == 0x07)
			err = WR0_ExecPawn(info->handle, &info->handle->pio_zhaoxin, "ioctl_read_msr", &in, 1, &out, 1, NULL);
	}
	else
		err = WR0_RdMsr(info->handle, msr_index, &out);

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
	return 0.0;
}

static double get_cur_multiplier(struct msr_info_t* info)
{
	return 0.0;
}

static double get_max_multiplier(struct msr_info_t* info)
{
	return 0.0;
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
			if (read_centaur_msr(info, MSR_VIA_TEMP_0A_0D, 23, 0, &reg))
				return 0;
			break;
		case 0x0F:
			if (read_centaur_msr(info, MSR_VIA_TEMP_0F, 23, 0, &reg))
				return 0;
			break;
		default:
			return 0;
		}
	}
		break;
	case 0x07: // Zhaoxin
		if (read_centaur_msr(info, MSR_ZX_TEMP, 23, 0, &reg))
			return 0;
		break;
	default:
		return 0;
	}

	return (int)reg;
}

static int get_pkg_temperature(struct msr_info_t* info)
{
	return 0;
}

static double get_pkg_energy(struct msr_info_t* info)
{
	return 0.0;
}

static double get_pkg_pl1(struct msr_info_t* info)
{
	return 0.0;
}

static double get_pkg_pl2(struct msr_info_t* info)
{
	return 0.0;
}

static double get_voltage(struct msr_info_t* info)
{
	uint64_t reg, vid;
	if (info->id->x86.ext_family < 6)
		goto fail;
	if (read_centaur_msr(info, MSR_IA32_PERF_STATUS, 63, 0, &reg))
		goto fail;
	vid = (reg >> 32) & 0xFFFF;
	if (vid == 0)
		vid = reg & 0xFFFF;
	if (vid > 0)
		return (double)vid / (1ULL << 13ULL);
fail:
	return 0.0;
}

static double get_bus_clock(struct msr_info_t* info)
{
	return 100.0;
}

static int get_microcode_ver(struct msr_info_t* info)
{
	uint64_t rev;
	if (read_centaur_msr(info, MSR_IA32_BIOS_SIGN_ID, 63, 32, &rev))
		goto fail;
	return (int)rev;
fail:
	return 0;
}

static int get_tdp_nominal(struct msr_info_t* info)
{
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
	.get_microcode_ver = get_microcode_ver,
	.get_tdp_nominal = get_tdp_nominal,
};
