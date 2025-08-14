/*
 * Copyright 2009  Veselin Georgiev,
 * anrieffNOSPAM @ mgail_DOT.com (convert to gmail)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "rdmsr.h"

#include <windows.h>
#include <winioctl.h>
#include <winerror.h>
#include <pathcch.h>

#define IA32_MPERF             0xE7
#define IA32_APERF             0xE8

static int perfmsr_measure(struct wr0_drv_t* handle, int msr)
{
	int err;
	uint64_t a, b;
	uint64_t x, y;
	err = WR0_RdMsr(handle, msr, &x);
	if (err)
		return CPU_INVALID_VALUE;
	sys_precise_clock(&a);
	busy_loop_delay(10);
	WR0_RdMsr(handle, msr, &y);
	sys_precise_clock(&b);
	if (a >= b || x > y)
		return CPU_INVALID_VALUE;
	return (int) ((y - x) / (b - a));
}

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

int cpu_rdmsr_range(struct wr0_drv_t* handle, uint32_t msr_index, uint8_t highbit, uint8_t lowbit, uint64_t* result)
{
	int err;
	const uint8_t bits = highbit - lowbit + 1;

	if(highbit > 63 || lowbit > highbit)
		return cpuid_set_error(ERR_INVRANGE);

	err = WR0_RdMsr(handle, msr_index, result);

	if(!err && bits < 64)
	{
		/* Show only part of register */
		*result >>= lowbit;
		*result &= (1ULL << bits) - 1;
	}

	return err;
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
		case INFO_MPERF:
			ret = perfmsr_measure(handle, IA32_MPERF);
			break;
		case INFO_APERF:
			ret = perfmsr_measure(handle, IA32_APERF);
			break;
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
	}
	// Restore AffinityMask
	SetThreadGroupAffinity(thread, &saved_aff, NULL);
	return ret;
}
