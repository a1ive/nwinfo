// SPDX-License-Identifier: Unlicense

#include "rdmsr.h"

#include <windows.h>

extern struct msr_fn_t msr_fn_intel;
extern struct msr_fn_t msr_fn_amd;
extern struct msr_fn_t msr_fn_centaur;

void
NWL_GetCpuIndexStr(struct cpu_id_t* id, char* buf, size_t buf_len)
{
	const char* purpose = "";
	switch (id->purpose)
	{
	case PURPOSE_GENERAL:
		purpose = "-G";
		break;
	case PURPOSE_PERFORMANCE:
		purpose = "-P";
		break;
	case PURPOSE_EFFICIENCY:
		purpose = "-E";
		break;
	case PURPOSE_LP_EFFICIENCY:
		purpose = "-LPE";
		break;
	case PURPOSE_U_PERFORMANCE:
		purpose = "-UP";
		break;
	}
	snprintf(buf, buf_len, "CPU%u%s", id->index, purpose);
}

bool
NWL_MsrInit(struct msr_info_t* info, struct wr0_drv_t* drv, struct cpu_id_t* id)
{
	if (!info || !id || !drv)
		return false;

	struct msr_fn_t* fn = NULL;
	switch (id->vendor)
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
		return false;
	}

	info->ticks = GetTickCount64();
	info->id = id;
	info->handle = drv;
	info->fn = fn;
	info->valid = 1;

	NWL_GetCpuIndexStr(id, info->name, ARRAYSIZE(info->name));

	GROUP_AFFINITY saved_aff;
	HANDLE thread = GetCurrentThread();

	// cpu->affinity_mask to GROUP_AFFINITY
	ZeroMemory(&info->aff, sizeof(info->aff));
	const cpu_affinity_mask_t* affinity_mask = &id->affinity_mask;
	WORD group_count = GetActiveProcessorGroupCount();
	DWORD total_processors = 0;
	for (WORD group = 0; group < group_count; group++)
	{
		DWORD processors = GetActiveProcessorCount(group);
		KAFFINITY mask = 0;
		for (DWORD cpu_index = 0; cpu_index < processors; cpu_index++)
		{
			uint32_t logical_cpu = total_processors + cpu_index;
			uint32_t byte_index = logical_cpu / __MASK_NCPUBITS;
			uint8_t bit = (uint8_t)(1U << (logical_cpu % __MASK_NCPUBITS));
			if (affinity_mask->__bits[byte_index] & bit)
				mask |= ((KAFFINITY)1) << cpu_index;
		}
		if (mask != 0)
		{
			info->aff.Group = group;
			info->aff.Mask = mask;
			break;
		}
		total_processors += processors;
	}
	if (!info->aff.Mask)
	{
		info->aff.Group = 0;
		info->aff.Mask = 1;
	}

	SetThreadGroupAffinity(thread, &info->aff, &saved_aff);
	info->clock = cpu_clock_measure(250, 1);
	SetThreadGroupAffinity(thread, &saved_aff, NULL);
	return true;
}

int
NWL_MsrGet(struct msr_info_t* info, cpu_msrinfo_request_t which)
{
	if (!info || !info->valid)
		return 0;

	GROUP_AFFINITY saved_aff;
	HANDLE thread = GetCurrentThread();
	SetThreadGroupAffinity(thread, &info->aff, &saved_aff);

	int ret = 0;
	struct msr_fn_t* fn = info->fn;

	switch (which)
	{
	case INFO_MIN_MULTIPLIER:
		if (info->cached_min_multiplier <= 0)
			info->cached_min_multiplier = (int)(fn->get_min_multiplier(info) * 100);
		ret = info->cached_min_multiplier;
		break;
	case INFO_CUR_MULTIPLIER:
		ret = (int)(fn->get_cur_multiplier(info) * 100);
		break;
	case INFO_MAX_MULTIPLIER:
		if (info->cached_max_multiplier <= 0)
			info->cached_max_multiplier = (int)(fn->get_max_multiplier(info) * 100);
		ret = info->cached_max_multiplier;
		break;
	case INFO_TEMPERATURE:
		ret = fn->get_temperature(info);
		break;
	case INFO_PKG_TEMPERATURE:
		ret = fn->get_pkg_temperature(info);
		break;
	case INFO_VOLTAGE:
		ret = (int)(fn->get_voltage(info) * 100);
		break;
	case INFO_PKG_ENERGY:
		ret = (int)(fn->get_pkg_energy(info) * 100);
		break;
	case INFO_PKG_POWER:
	{
		ULONGLONG ticks = GetTickCount64();
		ULONGLONG delta_ticks = 0;
		if (ticks > info->ticks)
			delta_ticks = ticks - info->ticks;
		int pkg_energy = (int)(fn->get_pkg_energy(info) * 100);
		if (pkg_energy > info->pkg_energy && delta_ticks > 0)
			ret = (int)((pkg_energy - info->pkg_energy) * 1000 / delta_ticks);
		info->pkg_energy = pkg_energy;
		info->ticks = ticks;
	}
		break;
	case INFO_PKG_PL1:
		if (info->cached_pl1 <= 0)
			info->cached_pl1 = (int)(fn->get_pkg_pl1(info) * 100);
		ret = info->cached_pl1;
		break;
	case INFO_PKG_PL2:
		if (info->cached_pl2 <= 0)
			info->cached_pl2 = (int)(fn->get_pkg_pl2(info) * 100);
		ret = info->cached_pl2;
		break;
	case INFO_BUS_CLOCK:
		ret = (int)(fn->get_bus_clock(info) * 100);
		break;
	case INFO_IGPU_TEMPERATURE:
		ret = fn->get_igpu_temperature(info);
		break;
	case INFO_IGPU_ENERGY:
		ret = (int)(fn->get_igpu_energy(info) * 100);
		break;
	case INFO_MICROCODE_VER:
		if (info->cached_microcode == 0)
			info->cached_microcode = (int)(fn->get_microcode_ver(info));
		ret = info->cached_microcode;
		break;
	case INFO_TDP_NOMINAL:
		if (info->cached_tdp <= 0)
			info->cached_tdp = (int)fn->get_tdp_nominal(info);
		ret = info->cached_tdp;
		break;
	}

	SetThreadGroupAffinity(thread, &saved_aff, NULL);
	return ret;
}

void
NWL_MsrFini(struct msr_info_t* info)
{
	if (!info)
		return;
	ryzen_smu_free(info->ry);
	ZeroMemory(info, sizeof(struct msr_info_t));
}
