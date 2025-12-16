// SPDX-License-Identifier: Unlicense

#include "rdmsr.h"

#include <windows.h>
#include <winioctl.h>
#include <winerror.h>
#include <pathcch.h>

extern struct msr_fn_t msr_fn_intel;
extern struct msr_fn_t msr_fn_amd;
extern struct msr_fn_t msr_fn_centaur;

static bool set_cpu_affinity(logical_cpu_t logical_cpu)
{
	int groups = GetActiveProcessorGroupCount();
	int total_processors = 0;
	int group = 0;
	int number = 0;
	int found = 0;
	HANDLE thread = GetCurrentThread();
	GROUP_AFFINITY group_aff;

	for (int i = 0; i < groups; i++)
	{
		int processors = GetActiveProcessorCount(i);
		if (total_processors + processors > logical_cpu)
		{
			group = i;
			number = logical_cpu - total_processors;
			found = 1;
			break;
		}
		total_processors += processors;
	}
	if (!found)
		return 0; // logical CPU # too large, does not exist

	memset(&group_aff, 0, sizeof(group_aff));
	group_aff.Group = (WORD)group;
	group_aff.Mask = (KAFFINITY)(1ULL << number);
	return SetThreadGroupAffinity(thread, &group_aff, NULL);
}

int cpu_msrinfo(struct wr0_drv_t* handle, logical_cpu_t cpu, cpu_msrinfo_request_t which)
{
	static int err = 0, init = 0;
	struct cpu_raw_data_t raw;
	static struct cpu_id_t id;
	static struct internal_id_info_t internal;
	static struct msr_info_t info = { 0 };
	struct msr_fn_t* fn = NULL;

	if (handle == NULL)
	{
		cpuid_set_error(ERR_HANDLE);
		return CPU_INVALID_VALUE;
	}

	info.handle = handle;
	if (!init)
	{
		err  = cpuid_get_raw_data(&raw);
		err += cpu_ident_internal(&raw, &id, &internal);
		info.cpu_clock = cpu_clock_measure(250, 1);
		info.id = &id;
		info.internal = &internal;
		init = 1;
	}

	if (err)
		return CPU_INVALID_VALUE;
	switch (info.id->vendor)
	{
	case VENDOR_INTEL:
		fn = &msr_fn_intel;
		break;
	case VENDOR_AMD:
	case VENDOR_HYGON:
		fn = &msr_fn_amd;
		break;
	case VENDOR_CENTAUR:
	case VENDOR_VIA:
	case VENDOR_ZHAOXIN:
		fn = &msr_fn_centaur;
		break;
	default:
		return CPU_INVALID_VALUE;
	}

	// Save AffinityMask
	GROUP_AFFINITY saved_aff;
	HANDLE thread = GetCurrentThread();
	if (!GetThreadGroupAffinity(thread, &saved_aff))
		return cpuid_set_error(ERR_INVCNB);
	// Set AffinityMask
	set_cpu_affinity(cpu);

	int ret = CPU_INVALID_VALUE;
	switch (which)
	{
		case INFO_MIN_MULTIPLIER:
			ret = (int) (fn->get_min_multiplier(&info) * 100);
			break;
		case INFO_CUR_MULTIPLIER:
			ret = (int) (fn->get_cur_multiplier(&info) * 100);
			break;
		case INFO_MAX_MULTIPLIER:
			ret = (int) (fn->get_max_multiplier(&info) * 100);
			break;
		case INFO_TEMPERATURE:
			ret = fn->get_temperature(&info);
			break;
		case INFO_PKG_TEMPERATURE:
			ret = fn->get_pkg_temperature(&info);
			break;
		case INFO_VOLTAGE:
			ret = (int) (fn->get_voltage(&info) * 100);
			break;
		case INFO_PKG_ENERGY:
			ret = (int) (fn->get_pkg_energy(&info) * 100);
			break;
		case INFO_PKG_PL1:
			ret = (int)(fn->get_pkg_pl1(&info) * 100);
			break;
		case INFO_PKG_PL2:
			ret = (int)(fn->get_pkg_pl2(&info) * 100);
			break;
		case INFO_BCLK:
		case INFO_BUS_CLOCK:
			ret = (int) (fn->get_bus_clock(&info) * 100);
			break;
		case INFO_IGPU_TEMPERATURE:
			ret = fn->get_igpu_temperature(&info);
			break;
		case INFO_IGPU_ENERGY:
			ret = (int) (fn->get_igpu_energy(&info) * 100);
			break;
		case INFO_MICROCODE_VER:
			ret = (int) (fn->get_microcode_ver(&info));
			break;
	}
	// Restore AffinityMask
	SetThreadGroupAffinity(thread, &saved_aff, NULL);
	return ret;
}
