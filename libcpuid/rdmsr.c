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

#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcpuid.h"
#include "asm-bits.h"
#include "libcpuid_util.h"
#include "libcpuid_internal.h"
#include "rdtsc.h"

#include <winring0.h>

#include <windows.h>
#include <winioctl.h>
#include <winerror.h>
#include <pathcch.h>

#include "../ryzenadj/ryzenadj.h"

#ifdef _WIN64
#define RY_DLL L"ryzenadjx64.dll"
#else
#define RY_DLL L"ryzenadj.dll"
#endif

#ifndef RDMSR_UNSUPPORTED_OS

/* Useful links for hackers:
- AMD MSRs:
  AMD BIOS and Kernel Developer's Guide (BKDG)
  * AMD Family 10h Processors
  http://support.amd.com/TechDocs/31116.pdf
  * AMD Family 11h Processors
  http://support.amd.com/TechDocs/41256.pdf
  * AMD Family 12h Processors
  http://support.amd.com/TechDocs/41131.pdf
  * AMD Family 14h Processors
  http://support.amd.com/TechDocs/43170_14h_Mod_00h-0Fh_BKDG.pdf
  * AMD Family 15h Processors
  http://support.amd.com/TechDocs/42301_15h_Mod_00h-0Fh_BKDG.pdf
  http://support.amd.com/TechDocs/42300_15h_Mod_10h-1Fh_BKDG.pdf
  http://support.amd.com/TechDocs/49125_15h_Models_30h-3Fh_BKDG.pdf
  http://support.amd.com/TechDocs/50742_15h_Models_60h-6Fh_BKDG.pdf
  http://support.amd.com/TechDocs/55072_AMD_Family_15h_Models_70h-7Fh_BKDG.pdf
  * AMD Family 16h Processors
  http://support.amd.com/TechDocs/48751_16h_bkdg.pdf
  http://support.amd.com/TechDocs/52740_16h_Models_30h-3Fh_BKDG.pdf

  AMD Processor Programming Reference (PPR)
  * AMD Family 17h Processors
  Models 00h-0Fh: https://www.amd.com/system/files/TechDocs/54945-ppr-family-17h-models-00h-0fh-processors.zip
  Models 01h, 08h, Revision B2: https://www.amd.com/system/files/TechDocs/54945_3.03_ppr_ZP_B2_pub.zip
  Model 71h, Revision B0: https://www.amd.com/system/files/TechDocs/56176_ppr_Family_17h_Model_71h_B0_pub_Rev_3.06.zip
  Model 60h, Revision A1: https://www.amd.com/system/files/TechDocs/55922-A1-PUB.zip
  Model 18h, Revision B1: https://www.amd.com/system/files/TechDocs/55570-B1-PUB.zip
  Model 20h, Revision A1: https://www.amd.com/system/files/TechDocs/55772-A1-PUB.zip
  Model 31h, Revision B0 Processors https://www.amd.com/system/files/TechDocs/55803-ppr-family-17h-model-31h-b0-processors.pdf
  Models A0h-AFh, Revision A0: https://www.amd.com/system/files/TechDocs/57243-A0-PUB.zip
  * AMD Family 19h Processors
  Model 01h, Revision B1: https://www.amd.com/system/files/TechDocs/55898_B1_pub_0.50.zip
  Model 21h, Revision B0: https://www.amd.com/system/files/TechDocs/56214-B0-PUB.zip
  Model 51h, Revision A1: https://www.amd.com/system/files/TechDocs/56569-A1-PUB.zip
  Model 11h, Revision B1: https://www.amd.com/system/files/TechDocs/55901_0.25.zip
  Model 61h, Revision B1: https://www.amd.com/system/files/TechDocs/56713-B1-PUB_3.04.zip
  Model 70h, Revision A0: https://www.amd.com/system/files/TechDocs/57019-A0-PUB_3.00.zip

- Intel MSRs:
  Intel 64 and IA-32 Architectures Software Developer's Manual
  * Volume 3 (3A, 3B, 3C & 3D): System Programming Guide
  http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-system-programming-manual-325384.pdf
*/

/* AMD MSRs addresses */
#define MSR_PSTATE_L           0xC0010061
#define MSR_PSTATE_S           0xC0010063
#define MSR_PSTATE_0           0xC0010064
#define MSR_PSTATE_1           0xC0010065
#define MSR_PSTATE_2           0xC0010066
#define MSR_PSTATE_3           0xC0010067
#define MSR_PSTATE_4           0xC0010068
#define MSR_PSTATE_5           0xC0010069
#define MSR_PSTATE_6           0xC001006A
#define MSR_PSTATE_7           0xC001006B
#define MSR_PWR_UNIT           0xC0010299
#define MSR_PKG_ENERGY_STAT    0xC001029B
static const uint32_t amd_msr[] = {
	MSR_PSTATE_L,
	MSR_PSTATE_S,
	MSR_PSTATE_0,
	MSR_PSTATE_1,
	MSR_PSTATE_2,
	MSR_PSTATE_3,
	MSR_PSTATE_4,
	MSR_PSTATE_5,
	MSR_PSTATE_6,
	MSR_PSTATE_7,
	MSR_PWR_UNIT,
	MSR_PKG_ENERGY_STAT,
	CPU_INVALID_VALUE
};

/* Intel MSRs addresses */
#define IA32_MPERF             0xE7
#define IA32_APERF             0xE8
#define IA32_PERF_STATUS       0x198
#define IA32_THERM_STATUS      0x19C
#define IA32_PKG_THERM_STATUS  0x1B1
#define MSR_EBL_CR_POWERON     0x2A
#define MSR_TURBO_RATIO_LIMIT  0x1AD
#define MSR_TEMPERATURE_TARGET 0x1A2
#define MSR_PERF_STATUS        0x198
#define MSR_RAPL_POWER_UNIT    0x606
#define MSR_PKG_POWER_LIMIT    0x610
#define MSR_PKG_ENERGY_STATUS  0x611
#define MSR_PLATFORM_INFO      0xCE
static const uint32_t intel_msr[] = {
	IA32_MPERF,
	IA32_APERF,
	IA32_PERF_STATUS,
	IA32_THERM_STATUS,
	IA32_PKG_THERM_STATUS,
	MSR_EBL_CR_POWERON,
	MSR_TURBO_RATIO_LIMIT,
	MSR_TEMPERATURE_TARGET,
	MSR_PERF_STATUS,
	MSR_RAPL_POWER_UNIT,
	MSR_PKG_POWER_LIMIT,
	MSR_PKG_ENERGY_STATUS,
	MSR_PLATFORM_INFO,
	CPU_INVALID_VALUE
};

struct msr_info_t {
	int cpu_clock;
	bool ryzenadj;
	struct wr0_drv_t *handle;
	struct cpu_id_t *id;
	struct internal_id_info_t *internal;
};

static int perfmsr_measure(struct wr0_drv_t* handle, int msr)
{
	int err;
	uint64_t a, b;
	uint64_t x, y;
	err = cpu_rdmsr(handle, msr, &x);
	if (err) return CPU_INVALID_VALUE;
	sys_precise_clock(&a);
	busy_loop_delay(10);
	cpu_rdmsr(handle, msr, &y);
	sys_precise_clock(&b);
	if (a >= b || x > y) return CPU_INVALID_VALUE;
	return (int) ((y - x) / (b - a));
}

static int msr_platform_info_supported(struct msr_info_t *info)
{
	int i;
	static int supported = -1;

	/* Return cached result */
	if(supported >= 0)
		return supported;

	/* List of microarchitectures that provide both "Maximum Non-Turbo Ratio" and "Maximum Efficiency Ratio" values
	Please note Silvermont does not report "Maximum Efficiency Ratio" */
	const struct { int32_t ext_family; int32_t ext_model; } msr_platform_info[] = {
		/* Table 2-12. MSRs in Intel Atom Processors Based on Goldmont Microarchitecture */
		{ 6, 92 },
		{ 6, 122 },
		/* Table 2-15. MSRs in Processors Based on Nehalem Microarchitecture */
		{ 6, 26 },
		{ 6, 30 },
		{ 6, 37 },
		{ 6, 44 },
		/* Table 2-20. MSRs Supported by Intel Processors Based on Sandy Bridge Microarchitecture */
		{ 6, 42 },
		{ 6, 45 },
		/* Table 2-25. Additional MSRs Supported by 3rd Generation Intel CoreTM Processors Based on Ivy Bridge Microarchitecture */
		{ 6, 58 },
		/* Table 2-26. MSRs Supported by the Intel Xeon Processor E5 v2 Product Family (Ivy Bridge-E Microarchitecture) */
		{ 6, 62 },
		/* Table 2-29. Additional MSRs Supported by Processors Based on the Haswell and Haswell-E Microarchitectures */
		{ 6, 60 },
		{ 6, 63 },
		{ 6, 69 },
		{ 6, 70 },
		/* Table 2-36. Additional MSRs Common to the Intel Xeon Processor D and the Intel Xeon Processor E5 v4 Family Based on Broadwell Microarchitecture */
		{ 6, 61 },
		{ 6, 71 },
		{ 6, 79 },
		/* Table 2-39. Additional MSRs Supported by the 6th-13th Generation Intel CoreTM Processors, 1st-5th Generation Intel Xeon Scalable Processor Families, Intel CoreTM Ultra 7 Processors, 8th Generation Intel CoreTM i3 Processors, and Intel Xeon E Processors */
		/* ==> Skylake */
		{ 6, 78 },
		{ 6, 85 },
		{ 6, 94 },
		/* ==> Kaby Lake */
		{ 6, 142 },
		{ 6, 158 },
		/* ==> Coffee Lake */
		{ 6, 102 },
		{ 6, 142 },
		{ 6, 158 },
		/* ==> Cascade Lake */
		{ 6, 85 },
		/* ==> Comet Lake */
		{ 6, 142 },
		{ 6, 165 },
		/* ==> Ice Lake */
		{ 6, 106 },
		{ 6, 108 },
		{ 6, 126 },
		/* ==> Rocket Lake */
		{ 6, 167 },
		/* ==> Tremont */
		{ 6, 138 },
		{ 6, 150 },
		{ 6, 156 },
		/* ==> Tiger Lake */
		{ 6, 140 },
		/* ==> Alder Lake */
		{ 6, 151 },
		{ 6, 154 },
		{ 6, 190 },
		/* ==> Raptor Lake */
		{ 6, 183 },
		{ 6, 186 },
		{ 6, 191 },
		/* ==> Sapphire Rapids */
		{ 6, 143 },
		/* ==> Emerald Rapids */
		{ 6, 207 },
		/* ==> Meteor Lake */
		{ 6, 170 },
		/* ==> Arrow Lake */
		{ 6, 198 },
		/* Table 2-50. MSRs Supported by the Intel Xeon Scalable Processor Family with a CPUID Signature DisplayFamily_DisplayModel Value of 06_55H */
		{ 0x6, 0x55 },
		/* Table 2-56. Selected MSRs Supported by Intel Xeon PhiTM Processors with a CPUID Signature DisplayFamily_DisplayModel Value of 06_57H or 06_85H */
		{ 0x6, 0x57 },
		{ 0x6, 0x85 },
	};

	if(info->id->vendor == VENDOR_INTEL) {
		for(i = 0; i < COUNT_OF(msr_platform_info); i++) {
			if((info->id->x86.ext_family == msr_platform_info[i].ext_family) && (info->id->x86.ext_model == msr_platform_info[i].ext_model)) {
				debugf(2, "Intel CPU with CPUID signature %02X_%02XH supports MSR_PLATFORM_INFO.\n", info->id->x86.ext_family, info->id->x86.ext_model);
				supported = 1;
				return supported;
			}
		}
		debugf(2, "Intel CPU with CPUID signature %02X_%02XH does not support MSR_PLATFORM_INFO.\n", info->id->x86.ext_family, info->id->x86.ext_model);
	}

	supported = 0;
	return supported;
}

static int msr_perf_status_supported(struct msr_info_t *info)
{
	int i;
	static int supported = -1;

	/* Return cached result */
	if(supported >= 0)
		return supported;

	/* List of microarchitectures that provide "Core Voltage" values */
	const struct { int32_t ext_family; int32_t ext_model; } msr_perf_status[] = {
		/* Table 2-20. MSRs Supported by Intel Processors Based on Sandy Bridge Microarchitecture */
		{ 6, 42 },
		{ 6, 45 },
		/* ==> Ivy Bridge */
		{ 6, 58 },
		{ 6, 62 },
		/* ==> Haswell */
		{ 6, 60 },
		{ 6, 63 },
		{ 6, 69 },
		{ 6, 70 },
		/* ==> Broadwell */
		{ 6, 61 },
		{ 6, 71 },
		{ 6, 79 },
		/* ==> Skylake */
		{ 6, 78 },
		{ 6, 85 },
		{ 6, 94 },
		/* ==> Kaby Lake */
		{ 6, 142 },
		{ 6, 158 },
		/* ==> Coffee Lake */
		{ 6, 102 },
		{ 6, 142 },
		{ 6, 158 },
		/* ==> Cascade Lake */
		{ 6, 85 },
		/* ==> Comet Lake */
		{ 6, 142 },
		{ 6, 165 },
		/* ==> Ice Lake */
		{ 6, 106 },
		{ 6, 108 },
		{ 6, 126 },
		/* ==> Rocket Lake */
		{ 6, 167 },
		/* ==> Tremont */
		{ 6, 138 },
		{ 6, 150 },
		{ 6, 156 },
		/* ==> Tiger Lake */
		{ 6, 140 },
		/* ==> Alder Lake */
		{ 6, 151 },
		{ 6, 154 },
		{ 6, 190 },
		/* ==> Raptor Lake */
		{ 6, 183 },
		{ 6, 186 },
		{ 6, 191 },
		/* ==> Sapphire Rapids */
		{ 6, 143 },
		/* ==> Emerald Rapids */
		{ 6, 207 },
		/* ==> Meteor Lake */
		{ 6, 170 },
		/* Table 2-50. MSRs Supported by the Intel Xeon Scalable Processor Family with a CPUID Signature DisplayFamily_DisplayModel Value of 06_55H */
		{ 0x6, 0x55 },
		/* Table 2-56. Selected MSRs Supported by Intel Xeon PhiTM Processors with a CPUID Signature DisplayFamily_DisplayModel Value of 06_57H or 06_85H */
		{ 0x6, 0x57 },
		{ 0x6, 0x85 },
	};

	if(info->id->vendor == VENDOR_INTEL) {
		for(i = 0; i < COUNT_OF(msr_perf_status); i++) {
			if((info->id->x86.ext_family == msr_perf_status[i].ext_family) && (info->id->x86.ext_model == msr_perf_status[i].ext_model)) {
				debugf(2, "Intel CPU with CPUID signature %02X_%02XH supports MSR_PERF_STATUS.\n", info->id->x86.ext_family, info->id->x86.ext_model);
				supported = 1;
				return supported;
			}
		}
		debugf(2, "Intel CPU with CPUID signature %02X_%02XH does not support MSR_PERF_STATUS.\n", info->id->x86.ext_family, info->id->x86.ext_model);
	}

	supported = 0;
	return supported;
}

static int get_amd_multipliers(struct msr_info_t *info, uint32_t pstate, double *multiplier)
{
	int i, err;
	uint64_t CpuFid, CpuDid, CpuDidLSD;

	/* Constant values needed for 12h family */
	const struct { uint64_t did; double divisor; } divisor_t[] = {
		{ 0x0,    1   },
		{ 0x1,    1.5 },
		{ 0x2,    2   },
		{ 0x3,    3   },
		{ 0x4,    4   },
		{ 0x5,    6   },
		{ 0x6,    8   },
		{ 0x7,    12  },
		{ 0x8,    16  },
	};
	const int num_dids = (int) COUNT_OF(divisor_t);

	/* Constant values for common families */
	const int magic_constant = (info->id->x86.ext_family == 0x11) ? 0x8 : 0x10;
	const int is_apu = ((FUSION_C <= info->internal->code.amd) && (info->internal->code.amd <= FUSION_A)) || (info->internal->bits & _APU_);
	const double divisor = is_apu ? 1.0 : 2.0;

	/* Check if P-state is valid */
	if (pstate < MSR_PSTATE_0 || MSR_PSTATE_7 < pstate)
		return 1;

	/* Overview of AMD CPU microarchitectures: https://en.wikipedia.org/wiki/List_of_AMD_CPU_microarchitectures#Nomenclature */
	switch (info->id->x86.ext_family) {
		case 0x12: /* K10 (Llano) / K12 */
			/* BKDG 12h, page 469
			MSRC001_00[6B:64][8:4] is CpuFid
			MSRC001_00[6B:64][3:0] is CpuDid
			CPU COF is (100MHz * (CpuFid + 10h) / (divisor specified by CpuDid))
			Note: This family contains only APUs */
			err  = cpu_rdmsr_range(info->handle, pstate, 8, 4, &CpuFid);
			err += cpu_rdmsr_range(info->handle, pstate, 3, 0, &CpuDid);
			i = 0;
			while (i < num_dids && divisor_t[i].did != CpuDid)
				i++;
			if (i < num_dids)
				*multiplier = (double) ((CpuFid + magic_constant) / divisor_t[i].divisor);
			else
				err++;
			break;
		case 0x14: /* Bobcat */
			/* BKDG 14h, page 430
			MSRC001_00[6B:64][8:4] is CpuDidMSD
			MSRC001_00[6B:64][3:0] is CpuDidLSD
			PLL COF is (100 MHz * (D18F3xD4[MainPllOpFreqId] + 10h))
			Divisor is (CpuDidMSD + (CpuDidLSD * 0.25) + 1)
			CPU COF is (main PLL frequency specified by D18F3xD4[MainPllOpFreqId]) / (core clock divisor specified by CpuDidMSD and CpuDidLSD)
			Note: This family contains only APUs */
			err  = cpu_rdmsr_range(info->handle, pstate, 8, 4, &CpuDid);
			err += cpu_rdmsr_range(info->handle, pstate, 3, 0, &CpuDidLSD);
			*multiplier = (double) (((info->cpu_clock + 5LL) / 100 + magic_constant) / (CpuDid + CpuDidLSD * 0.25 + 1));
			break;
		case 0x10: /* K10 */
			/* BKDG 10h, page 429
			MSRC001_00[6B:64][8:6] is CpuDid
			MSRC001_00[6B:64][5:0] is CpuFid
			CPU COF is (100 MHz * (CpuFid + 10h) / (2^CpuDid))
			Note: This family contains only CPUs */
			/* Fall through */
		case 0x11: /* K8 & K10 "hybrid" */
			/* BKDG 11h, page 236
			MSRC001_00[6B:64][8:6] is CpuDid
			MSRC001_00[6B:64][5:0] is CpuFid
			CPU COF is ((100 MHz * (CpuFid + 08h)) / (2^CpuDid))
			Note: This family contains only CPUs */
			/* Fall through */
		case 0x15: /* Bulldozer / Piledriver / Steamroller / Excavator */
			/* BKDG 15h, page 570/580/635/692 (00h-0Fh/10h-1Fh/30h-3Fh/60h-6Fh)
			MSRC001_00[6B:64][8:6] is CpuDid
			MSRC001_00[6B:64][5:0] is CpuFid
			CoreCOF is (100 * (MSRC001_00[6B:64][CpuFid] + 10h) / (2^MSRC001_00[6B:64][CpuDid]))
			Note: This family contains BOTH CPUs and APUs */
			/* Fall through */
		case 0x16: /* Jaguar / Puma */
			/* BKDG 16h, page 549/611 (00h-0Fh/30h-3Fh)
			MSRC001_00[6B:64][8:6] is CpuDid
			MSRC001_00[6B:64][5:0] is CpuFid
			CoreCOF is (100 * (MSRC001_00[6B:64][CpuFid] + 10h) / (2^MSRC001_00[6B:64][CpuDid]))
			Note: This family contains only APUs */
			err  = cpu_rdmsr_range(info->handle, pstate, 8, 6, &CpuDid);
			err += cpu_rdmsr_range(info->handle, pstate, 5, 0, &CpuFid);
			*multiplier = ((double) (CpuFid + magic_constant) / (1ull << CpuDid)) / divisor;
			break;
		case 0x17: /* Zen / Zen+ / Zen 2 */
			/* PPR 17h, pages 30 and 138-139
			MSRC001_00[6B:64][13:8] is CpuDfsId
			MSRC001_00[6B:64][7:0]  is CpuFid
			CoreCOF is (Core::X86::Msr::PStateDef[CpuFid[7:0]] / Core::X86::Msr::PStateDef[CpuDfsId]) * 200 */
			/* Fall through */
		case 0x18: /* Hygon Dhyana */
			/* Note: Dhyana is "mostly a re-branded Zen CPU for the Chinese server market"
			https://www.phoronix.com/news/Hygon-Dhyana-AMD-China-CPUs */
			/* Fall through */
		case 0x19: /* Zen 3 / Zen 3+ / Zen 4 */
			/* PPR for AMD Family 19h Model 70h A0, pages 37 and 206-207
			MSRC001_006[4...B][13:8] is CpuDfsId
			MSRC001_006[4...B][7:0]  is CpuFid
			CoreCOF is (Core::X86::Msr::PStateDef[CpuFid[7:0]]/Core::X86::Msr::PStateDef[CpuDfsId]) *200 */
			err  = cpu_rdmsr_range(info->handle, pstate, 13, 8, &CpuDid);
			err += cpu_rdmsr_range(info->handle, pstate,  7, 0, &CpuFid);
			*multiplier = ((double) CpuFid / CpuDid) * 2;
			break;
		default:
			warnf("get_amd_multipliers(): unsupported CPU extended family: %xh\n", info->id->x86.ext_family);
			err = 1;
			break;
	}

	return err;
}

static uint32_t get_amd_last_pstate_addr(struct msr_info_t *info)
{
	static uint32_t last_addr = 0x0;
	uint64_t reg = 0x0;

	/* The result is cached, need to be computed once */
	if(last_addr != 0x0)
		return last_addr;

	/* Refer links above
	MSRC001_00[6B:64][63] is PstateEn
	PstateEn indicates if the rest of the P-state information in the register is valid after a reset */
	last_addr = MSR_PSTATE_7 + 1;
	while((reg == 0x0) && (last_addr > MSR_PSTATE_0)) {
		last_addr--;
		cpu_rdmsr_range(info->handle, last_addr, 63, 63, &reg);
	}
	return last_addr;
}

static double get_info_min_multiplier(struct msr_info_t *info)
{
	int err;
	double mult;
	uint32_t addr;
	uint64_t reg;

	if(msr_platform_info_supported(info)) {
		/* Refer links above
		Table 35-12.  MSRs in Next Generation Intel Atom Processors Based on the Goldmont Microarchitecture
		Table 35-13.  MSRs in Processors Based on Intel Microarchitecture Code Name Nehalem
		Table 35-18.  MSRs Supported by Intel Processors based on Intel microarchitecture code name Sandy Bridge (Contd.)
		Table 35-23.  Additional MSRs Supported by 3rd Generation Intel Core Processors (based on Intel microarchitecture code name Ivy Bridge)
		Table 35-24.  MSRs Supported by Intel Xeon Processors E5 v2 Product Family (based on Ivy Bridge-E microarchitecture)
		Table 35-27.  Additional MSRs Supported by Processors based on the Haswell or Haswell-E microarchitectures
		Table 35-34.  Additional MSRs Common to Intel Xeon Processor D and Intel Xeon Processors E5 v4 Family Based on the Broadwell Microarchitecture
		Table 35-40.  Selected MSRs Supported by Next Generation Intel Xeon Phi Processors with DisplayFamily_DisplayModel Signature 06_57H
		MSR_PLATFORM_INFO[47:40] is Maximum Efficiency Ratio
		Maximum Efficiency Ratio is the minimum ratio that the processor can operates */
		err = cpu_rdmsr_range(info->handle, MSR_PLATFORM_INFO, 47, 40, &reg);
		if (!err) return (double) reg;
	}
	else if(info->id->vendor == VENDOR_AMD || info->id->vendor == VENDOR_HYGON) {
		/* N.B.: Find the last P-state
		get_amd_last_pstate_addr() returns the last P-state, MSR_PSTATE_0 <= addr <= MSR_PSTATE_7 */
		addr = get_amd_last_pstate_addr(info);
		err  = get_amd_multipliers(info, addr, &mult);
		if (!err) return mult;
	}

	return (double) CPU_INVALID_VALUE / 100;
}

static double get_info_cur_multiplier(struct msr_info_t *info)
{
	int err;
	double mult;
	uint64_t reg;

	if(info->id->vendor == VENDOR_INTEL && info->internal->code.intel == PENTIUM) {
		err = cpu_rdmsr(info->handle, MSR_EBL_CR_POWERON, &reg);
		if (!err) return (double) ((reg>>22) & 0x1f);
	}
	else if(info->id->vendor == VENDOR_INTEL && info->internal->code.intel != PENTIUM) {
		/* Refer links above
		Table 35-2.  IA-32 Architectural MSRs (Contd.)
		IA32_PERF_STATUS[15:0] is Current performance State Value
		[7:0] is 0x0, [15:8] looks like current ratio */
		err = cpu_rdmsr_range(info->handle, IA32_PERF_STATUS, 15, 8, &reg);
		if (!err) return (double) reg;
	}
	else if(info->id->vendor == VENDOR_AMD || info->id->vendor == VENDOR_HYGON) {
		/* Refer links above
		MSRC001_0063[2:0] is CurPstate */
		err  = cpu_rdmsr_range(info->handle, MSR_PSTATE_S, 2, 0, &reg);
		err += get_amd_multipliers(info, MSR_PSTATE_0 + (uint32_t) reg, &mult);
		if (!err) return mult;
	}

	return (double) CPU_INVALID_VALUE / 100;
}

static double get_info_max_multiplier(struct msr_info_t *info)
{
	int err;
	double mult;
	uint64_t reg;

	if(info->id->vendor == VENDOR_INTEL && info->internal->code.intel == PENTIUM) {
		err = cpu_rdmsr(info->handle, IA32_PERF_STATUS, &reg);
		if (!err) return (double) ((reg >> 40) & 0x1f);
	}
	else if(info->id->vendor == VENDOR_INTEL && info->internal->code.intel != PENTIUM) {
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
		MSR_TURBO_RATIO_LIMIT[7:0] is Maximum Ratio Limit for 1C */
		err = cpu_rdmsr_range(info->handle, MSR_TURBO_RATIO_LIMIT, 7, 0, &reg);
		if (!err) return (double) reg;
	}
	else if(info->id->vendor == VENDOR_AMD || info->id->vendor == VENDOR_HYGON) {
		/* Refer links above
		MSRC001_0064 is Pb0
		Pb0 is the highest-performance boosted P-state */
		err = get_amd_multipliers(info, MSR_PSTATE_0, &mult);
		if (!err) return mult;
	}

	return (double) CPU_INVALID_VALUE / 100;
}

static struct
{
	HMODULE dll;
	ryzen_access ry;
	ryzen_access(CALL* init_ryzenadj)(VOID);
	void (CALL* cleanup_ryzenadj)(ryzen_access ry);
	int (CALL* init_table)(ryzen_access ry);
	uint32_t(CALL* get_table_ver)(ryzen_access ry);
	size_t(CALL* get_table_size)(ryzen_access ry);
	float* (CALL* get_table_values)(ryzen_access ry);
	int (CALL* refresh_table)(ryzen_access ry);
	float (CALL* get_stapm_limit)(ryzen_access ry);
	float (CALL* get_fast_limit)(ryzen_access ry);
	float (CALL* get_slow_limit)(ryzen_access ry);
} m_ry;

static void
ry_fini(void)
{
	if (m_ry.ry)
		m_ry.cleanup_ryzenadj(m_ry.ry);
	if (m_ry.dll)
		FreeLibrary(m_ry.dll);
	ZeroMemory(&m_ry, sizeof(m_ry));
}

static bool
ry_init(void)
{
	WCHAR dll_path[MAX_PATH];
	GetModuleFileNameW(NULL, dll_path, MAX_PATH);
	PathCchRemoveFileSpec(dll_path, MAX_PATH);
	PathCchAppend(dll_path, MAX_PATH, RY_DLL);
	m_ry.dll = LoadLibraryW(dll_path);
	if (!m_ry.dll)
		goto fail;
	*(FARPROC*)&m_ry.init_ryzenadj = GetProcAddress(m_ry.dll, "init_ryzenadj");
	*(FARPROC*)&m_ry.cleanup_ryzenadj = GetProcAddress(m_ry.dll, "cleanup_ryzenadj");
	*(FARPROC*)&m_ry.init_table = GetProcAddress(m_ry.dll, "init_table");
	*(FARPROC*)&m_ry.get_table_ver = GetProcAddress(m_ry.dll, "get_table_ver");
	*(FARPROC*)&m_ry.get_table_size = GetProcAddress(m_ry.dll, "get_table_size");
	*(FARPROC*)&m_ry.get_table_values = GetProcAddress(m_ry.dll, "get_table_values");
	*(FARPROC*)&m_ry.refresh_table = GetProcAddress(m_ry.dll, "refresh_table");
	*(FARPROC*)&m_ry.get_stapm_limit = GetProcAddress(m_ry.dll, "get_stapm_limit");
	*(FARPROC*)&m_ry.get_fast_limit = GetProcAddress(m_ry.dll, "get_fast_limit");
	*(FARPROC*)&m_ry.get_slow_limit = GetProcAddress(m_ry.dll, "get_slow_limit");
	if (m_ry.init_ryzenadj == NULL ||
		m_ry.cleanup_ryzenadj == NULL ||
		m_ry.init_table == NULL ||
		m_ry.get_table_ver == NULL ||
		m_ry.get_table_size == NULL ||
		m_ry.get_table_values == NULL ||
		m_ry.refresh_table == NULL ||
		m_ry.get_stapm_limit == NULL ||
		m_ry.get_fast_limit == NULL ||
		m_ry.get_slow_limit == NULL)
	{
		goto fail;
	}
	m_ry.ry = m_ry.init_ryzenadj();
	if (m_ry.ry == NULL)
		goto fail;
	if (m_ry.init_table(m_ry.ry))
		goto fail;
	return TRUE;
fail:
	ry_fini();
	return FALSE;
}

static int get_info_temperature(struct msr_info_t *info)
{
	int err;
	uint64_t DigitalReadout, ReadingValid, TemperatureTarget;

	if(info->id->vendor == VENDOR_INTEL && info->id->flags[CPU_FEATURE_INTEL_DTS]) {
		/* Refer links above
		Table 35-2.   IA-32 Architectural MSRs
		IA32_THERM_STATUS[22:16] is Digital Readout
		IA32_THERM_STATUS[31]    is Reading Valid

		Table 35-6.   MSRs Common to the Silvermont Microarchitecture and Newer Microarchitectures for Intel Atom
		Table 35-13.  MSRs in Processors Based on Intel Microarchitecture Code Name Nehalem (Contd.)
		Table 35-18.  MSRs Supported by Intel Processors based on Intel microarchitecture code name Sandy Bridge (Contd.)
		Table 35-24.  MSRs Supported by Intel Xeon Processors E5 v2 Product Family (based on Ivy Bridge-E microarchitecture) (Contd.)
		Table 35-34.  Additional MSRs Common to Intel Xeon Processor D and Intel Xeon Processors E5 v4 Family Based on the Broadwell Microarchitecture
		Table 35-40.  Selected MSRs Supported by Next Generation Intel Xeon Phi Processors with DisplayFamily_DisplayModel Signature 06_57H
		MSR_TEMPERATURE_TARGET[23:16] is Temperature Target */
		err  = cpu_rdmsr_range(info->handle, IA32_THERM_STATUS,      22, 16, &DigitalReadout);
		err += cpu_rdmsr_range(info->handle, IA32_THERM_STATUS,      31, 31, &ReadingValid);
		err += cpu_rdmsr_range(info->handle, MSR_TEMPERATURE_TARGET, 23, 16, &TemperatureTarget);
		if(!err && ReadingValid) return (int) (TemperatureTarget - DigitalReadout);
	}

	return CPU_INVALID_VALUE;
}

#define THERMTRIP_STATUS_REGISTER       0xE4
#define AMD_PCI_VENDOR_ID               0x1022
#define AMD_PCI_CONTROL_DEVICE_ID       0x1103

static int amd_k8_temperature(struct msr_info_t* info)
{
	uint32_t value;
	uint32_t addr;
	int offset = -49;
	if (info->id->x86.ext_model >= 0x69 &&
		info->id->x86.ext_model != 0xc1 &&
		info->id->x86.ext_model != 0x6c &&
		info->id->x86.ext_model != 0x7c)
		offset += 21;
	addr = pci_find_by_id(info->handle, AMD_PCI_VENDOR_ID, AMD_PCI_CONTROL_DEVICE_ID, info->id->index);

	if (addr == 0xFFFFFFFF)
		return CPU_INVALID_VALUE;

	pci_conf_write32(info->handle, addr, THERMTRIP_STATUS_REGISTER, 0);
	value = pci_conf_read32(info->handle, addr, THERMTRIP_STATUS_REGISTER);
	return (int)((value >> 16) & 0xFF) + offset;
}


#define SMU_REPORTED_TEMP_CTRL_OFFSET              0xD8200CA4

#define FAMILY_10H_MISCELLANEOUS_CONTROL_DEVICE_ID 0x1203
#define FAMILY_11H_MISCELLANEOUS_CONTROL_DEVICE_ID 0x1303
#define FAMILY_12H_MISCELLANEOUS_CONTROL_DEVICE_ID 0x1703
#define FAMILY_14H_MISCELLANEOUS_CONTROL_DEVICE_ID 0x1703
#define FAMILY_15H_MODEL_00_MISC_CONTROL_DEVICE_ID 0x1603
#define FAMILY_15H_MODEL_10_MISC_CONTROL_DEVICE_ID 0x1403
#define FAMILY_15H_MODEL_30_MISC_CONTROL_DEVICE_ID 0x141D
#define FAMILY_15H_MODEL_60_MISC_CONTROL_DEVICE_ID 0x1573
#define FAMILY_15H_MODEL_70_MISC_CONTROL_DEVICE_ID 0x15B3
#define FAMILY_16H_MODEL_00_MISC_CONTROL_DEVICE_ID 0x1533
#define FAMILY_16H_MODEL_30_MISC_CONTROL_DEVICE_ID 0x1583

#define NB_PCI_REG_ADDR_ADDR 0xB8
#define NB_PCI_REG_DATA_ADDR 0xBC

static int amd_k10_temperature(struct msr_info_t* info)
{
	uint32_t value = 0;
	uint32_t addr;
	uint16_t did = 0;
	bool smu = false;
	switch (info->id->x86.ext_family)
	{
	case 0x10:
		did = FAMILY_10H_MISCELLANEOUS_CONTROL_DEVICE_ID;
		break;
	case 0x11:
		did = FAMILY_11H_MISCELLANEOUS_CONTROL_DEVICE_ID;
		break;
	case 0x12:
		did = FAMILY_12H_MISCELLANEOUS_CONTROL_DEVICE_ID;
		break;
	case 0x14:
		did = FAMILY_14H_MISCELLANEOUS_CONTROL_DEVICE_ID;
		break;
	case 0x15:
		switch (info->id->x86.ext_model & 0xF0)
		{
		case 0x00:
			did = FAMILY_15H_MODEL_00_MISC_CONTROL_DEVICE_ID;
			break;
		case 0x10:
			did = FAMILY_15H_MODEL_10_MISC_CONTROL_DEVICE_ID;
			break;
		case 0x30:
			did = FAMILY_15H_MODEL_30_MISC_CONTROL_DEVICE_ID;
			break;
		case 0x60:
			did = FAMILY_15H_MODEL_60_MISC_CONTROL_DEVICE_ID;
			smu = true;
			break;
		case 0x70:
			did = FAMILY_15H_MODEL_70_MISC_CONTROL_DEVICE_ID;
			smu = true;
			break;
		}
		break;
	case 0x16:
		switch (info->id->x86.ext_model & 0xF0)
		{
		case 0x00:
			did = FAMILY_16H_MODEL_00_MISC_CONTROL_DEVICE_ID;
			break;
		case 0x30:
			did = FAMILY_16H_MODEL_30_MISC_CONTROL_DEVICE_ID;
			break;
		};
		break;
	}

	if (smu)
	{
		pci_conf_write32(info->handle, 0, NB_PCI_REG_ADDR_ADDR, SMU_REPORTED_TEMP_CTRL_OFFSET);
		value = pci_conf_read32(info->handle, 0, NB_PCI_REG_DATA_ADDR);
	}
	else
	{
		addr = pci_find_by_id(info->handle, AMD_PCI_VENDOR_ID, did, info->id->index);
		if (addr == 0xFFFFFFFF)
			return CPU_INVALID_VALUE;
		value = pci_conf_read32(info->handle, addr, 0xA4);
	}
	if ((info->id->x86.ext_family == 0x15 ||
		info->id->x86.ext_family == 0x16)
		&& (value & 0x30000) == 0x3000)
	{
		if (info->id->x86.ext_family == 0x15 && (info->id->x86.ext_model & 0xF0) == 0x00)
			return (int) (((value >> 21) & 0x7FC) / 8.0f) - 49;
		return (int) (((value >> 21) & 0x7FF) / 8.0f) - 49;
	}
	return (int) (((value >> 21) & 0x7FF) / 8.0f);
}

#define F17H_M01H_THM_TCON_CUR_TMP          0x00059800
#define F17H_TEMP_OFFSET_FLAG               0x80000
#define FAMILY_17H_PCI_CONTROL_REGISTER     0x60

static float amd_17h_temperature(struct msr_info_t* info)
{
	uint32_t temperature;
	float offset = 0.0f;

	pci_conf_write32(info->handle, 0, FAMILY_17H_PCI_CONTROL_REGISTER, F17H_M01H_THM_TCON_CUR_TMP);
	temperature = pci_conf_read32(info->handle, 0, FAMILY_17H_PCI_CONTROL_REGISTER + 4);

	if (strstr(info->id->brand_str, "1600X") ||
		strstr(info->id->brand_str, "1700X") ||
		strstr(info->id->brand_str, "1800X"))
		offset = -20.0f;
	else if (strstr(info->id->brand_str, "2700X"))
		offset = -10.0f;
	else if (strstr(info->id->brand_str, "Threadripper 19") ||
		strstr(info->id->brand_str, "Threadripper 29"))
		offset = -27.0f;

	if ((temperature & F17H_TEMP_OFFSET_FLAG))
		offset += -49.0f;

	return 0.001f * ((temperature >> 21) * 125) + offset;
}

static int get_info_pkg_temperature(struct msr_info_t* info)
{
	int err;
	uint64_t DigitalReadout, TemperatureTarget;

	if (info->id->vendor == VENDOR_INTEL && info->id->flags[CPU_FEATURE_INTEL_PTM]) {
		err = cpu_rdmsr_range(info->handle, IA32_PKG_THERM_STATUS, 22, 16, &DigitalReadout);
		err += cpu_rdmsr_range(info->handle, MSR_TEMPERATURE_TARGET, 23, 16, &TemperatureTarget);
		if (!err) return (int)(TemperatureTarget - DigitalReadout);
	}
	else if (info->id->vendor == VENDOR_AMD || info->id->vendor == VENDOR_HYGON)
	{
		if (info->id->x86.ext_family >= 0x17)
		{
			return (int)amd_17h_temperature(info);
		}
		else if (info->id->x86.ext_family > 0x0F)
		{
			return amd_k10_temperature(info);
		}
		else if (info->id->x86.ext_family == 0x0F)
		{
			return amd_k8_temperature(info);
		}
	}

	return CPU_INVALID_VALUE;
}

static double get_info_pkg_energy(struct msr_info_t* info)
{
	int err;
	uint64_t TotalEnergyConsumed, EnergyStatusUnits;

	if (info->id->vendor == VENDOR_INTEL) {
		err = cpu_rdmsr_range(info->handle, MSR_PKG_ENERGY_STATUS, 31, 0, &TotalEnergyConsumed);
		err += cpu_rdmsr_range(info->handle, MSR_RAPL_POWER_UNIT, 12, 8, &EnergyStatusUnits);
		if (!err) return (double) TotalEnergyConsumed / (1ULL << EnergyStatusUnits);
	}
	else if (info->id->vendor == VENDOR_AMD || info->id->vendor == VENDOR_HYGON)
	{
		// 17h: Zen / Zen+ / Zen 2
		// 18h: Hygon Dhyana
		// 19h: Zen 3 / Zen 3+ / Zen 4
		if (info->id->x86.ext_family >= 0x17)
		{
			err = cpu_rdmsr_range(info->handle, MSR_PKG_ENERGY_STAT, 31, 0, &TotalEnergyConsumed);
			err += cpu_rdmsr_range(info->handle, MSR_PWR_UNIT, 12, 8, &EnergyStatusUnits);
			if (!err) return (double)TotalEnergyConsumed / (1ULL << EnergyStatusUnits);
		}
	}
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_info_pkg_pl1(struct msr_info_t* info)
{
	int err;
	uint64_t PowerLimit1, PowerUnits;

	if (info->id->vendor == VENDOR_INTEL) {
		err = cpu_rdmsr_range(info->handle, MSR_PKG_POWER_LIMIT, 14, 0, &PowerLimit1);
		err += cpu_rdmsr_range(info->handle, MSR_RAPL_POWER_UNIT, 3, 0, &PowerUnits);
		if (!err) return (double)PowerLimit1 / (1ULL << PowerUnits);
	}
	else if (info->id->vendor == VENDOR_AMD)
	{
		if (!info->ryzenadj)
			info->ryzenadj = ry_init();
		if (info->ryzenadj)
		{
			m_ry.refresh_table(m_ry.ry);
			return m_ry.get_slow_limit(m_ry.ry);
		}
	}
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_info_pkg_pl2(struct msr_info_t* info)
{
	int err;
	uint64_t PowerLimit2, PowerUnits;
	if (info->id->vendor == VENDOR_INTEL) {
		err = cpu_rdmsr_range(info->handle, MSR_PKG_POWER_LIMIT, 46, 32, &PowerLimit2);
		err += cpu_rdmsr_range(info->handle, MSR_RAPL_POWER_UNIT, 3, 0, &PowerUnits);
		if (!err) return (double)PowerLimit2 / (1ULL << PowerUnits);
	}
	else if (info->id->vendor == VENDOR_AMD)
	{
		if (!info->ryzenadj)
			info->ryzenadj = ry_init();
		if (info->ryzenadj)
		{
			m_ry.refresh_table(m_ry.ry);
			return m_ry.get_slow_limit(m_ry.ry);
		}
	}
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_info_pkg_power(struct msr_info_t* info)
{
	double x, y;
	x = get_info_pkg_energy(info);
	if (x == (double)CPU_INVALID_VALUE / 100)
		goto fail;
	busy_loop_delay(10);
	y = get_info_pkg_energy(info);
	if (y == (double)CPU_INVALID_VALUE / 100 || x >= y)
		goto fail;
	return (y - x) * 100;
fail:
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_info_voltage(struct msr_info_t *info)
{
	int err;
	double VIDStep;
	uint64_t reg, CpuVid;

	if(msr_perf_status_supported(info)) {
		/* Refer links above
		Table 35-18.  MSRs Supported by Intel Processors based on Intel microarchitecture code name Sandy Bridge (Contd.)
		MSR_PERF_STATUS[47:32] is Core Voltage
		P-state core voltage can be computed by MSR_PERF_STATUS[37:32] * (float) 1/(2^13). */
		err = cpu_rdmsr_range(info->handle, MSR_PERF_STATUS, 47, 32, &reg);
		if (!err) return (double) reg / (1ULL << 13ULL);
	}
	else if(info->id->vendor == VENDOR_AMD || info->id->vendor == VENDOR_HYGON) {
		/* Refer links above
		MSRC001_00[6B:64][15:9]  is CpuVid (Jaguar and before)
		MSRC001_00[6B:64][21:14] is CpuVid (Zen)
		MSRC001_0063[2:0] is P-state Status
		BKDG 10h, page 49: voltage = 1.550V - 0.0125V * SviVid (SVI1)
		BKDG 15h, page 50: Voltage = 1.5500 - 0.00625 * Vid[7:0] (SVI2)
		SVI2 since Piledriver (Family 15h, 2nd-gen): Models 10h-1Fh Processors */
		VIDStep = ((info->id->x86.ext_family < 0x15) || ((info->id->x86.ext_family == 0x15) && (info->id->x86.ext_model < 0x10))) ? 0.0125 : 0.00625;
		err = cpu_rdmsr_range(info->handle, MSR_PSTATE_S, 2, 0, &reg);
		if(info->id->x86.ext_family < 0x17)
			err += cpu_rdmsr_range(info->handle, MSR_PSTATE_0 + (uint32_t) reg, 15, 9, &CpuVid);
		else
			err += cpu_rdmsr_range(info->handle, MSR_PSTATE_0 + (uint32_t) reg, 21, 14, &CpuVid);
		if (!err && MSR_PSTATE_0 + (uint32_t) reg <= MSR_PSTATE_7) return 1.550 - VIDStep * CpuVid;
	}

	return (double) CPU_INVALID_VALUE / 100;
}

static double get_info_bus_clock(struct msr_info_t *info)
{
	int err;
	double mult;
	uint32_t addr;
	uint64_t reg;

	if(msr_platform_info_supported(info)) {
		/* Refer links above
		Table 35-12.  MSRs in Next Generation Intel Atom Processors Based on the Goldmont Microarchitecture
		Table 35-13.  MSRs in Processors Based on Intel Microarchitecture Code Name Nehalem
		Table 35-18.  MSRs Supported by Intel Processors based on Intel microarchitecture code name Sandy Bridge (Contd.)
		Table 35-23.  Additional MSRs Supported by 3rd Generation Intel Core Processors (based on Intel microarchitecture code name Ivy Bridge)
		Table 35-24.  MSRs Supported by Intel Xeon Processors E5 v2 Product Family (based on Ivy Bridge-E microarchitecture)
		Table 35-27.  Additional MSRs Supported by Processors based on the Haswell or Haswell-E microarchitectures
		Table 35-40.  Selected MSRs Supported by Next Generation Intel Xeon Phi Processors with DisplayFamily_DisplayModel Signature 06_57H
		MSR_PLATFORM_INFO[15:8] is Maximum Non-Turbo Ratio */
		err = cpu_rdmsr_range(info->handle, MSR_PLATFORM_INFO, 15, 8, &reg);
		if (!err) return (double) info->cpu_clock / reg;
	}
	else if(info->id->vendor == VENDOR_AMD || info->id->vendor == VENDOR_HYGON) {
		/* Refer links above
		MSRC001_0061[6:4] is PstateMaxVal
		PstateMaxVal is the the lowest-performance non-boosted P-state */
		addr = get_amd_last_pstate_addr(info);
		err  = cpu_rdmsr_range(info->handle, MSR_PSTATE_L, 6, 4, &reg);
		err += get_amd_multipliers(info, addr - (uint32_t) reg, &mult);
		if (!err) return (double) info->cpu_clock / mult;
	}

	return (double) CPU_INVALID_VALUE / 100;
}

int cpu_rdmsr_range(struct wr0_drv_t* handle, uint32_t msr_index, uint8_t highbit,
                    uint8_t lowbit, uint64_t* result)
{
	int err;
	const uint8_t bits = highbit - lowbit + 1;

	if(highbit > 63 || lowbit > highbit)
		return cpuid_set_error(ERR_INVRANGE);

	err = cpu_rdmsr(handle, msr_index, result);

	if(!err && bits < 64) {
		/* Show only part of register */
		*result >>= lowbit;
		*result &= (1ULL << bits) - 1;
	}

	return err;
}

int cpu_msrinfo(struct wr0_drv_t* handle, cpu_msrinfo_request_t which)
{
	static int err = 0, init = 0;
	struct cpu_raw_data_t raw;
	static struct cpu_id_t id;
	static struct internal_id_info_t internal;
	static struct msr_info_t info = { 0 };

	if (handle == NULL) {
		cpuid_set_error(ERR_HANDLE);
		return CPU_INVALID_VALUE;
	}

	info.handle = handle;
	if (!init) {
		err  = cpuid_get_raw_data(&raw);
		err += cpu_ident_internal(&raw, &id, &internal);
		info.cpu_clock = cpu_clock_measure(250, 1);
		info.id = &id;
		info.internal = &internal;
		init = 1;
	}

	if (err)
		return CPU_INVALID_VALUE;

	switch (which) {
		case INFO_MPERF:
			return perfmsr_measure(handle, IA32_MPERF);
		case INFO_APERF:
			return perfmsr_measure(handle, IA32_APERF);
		case INFO_MIN_MULTIPLIER:
			return (int) (get_info_min_multiplier(&info) * 100);
		case INFO_CUR_MULTIPLIER:
			return (int) (get_info_cur_multiplier(&info) * 100);
		case INFO_MAX_MULTIPLIER:
			return (int) (get_info_max_multiplier(&info) * 100);
		case INFO_TEMPERATURE:
			return get_info_temperature(&info);
		case INFO_PKG_TEMPERATURE:
			return get_info_pkg_temperature(&info);
		case INFO_THROTTLING:
			return CPU_INVALID_VALUE;
		case INFO_VOLTAGE:
			return (int) (get_info_voltage(&info) * 100);
		case INFO_PKG_ENERGY:
			return (int) (get_info_pkg_energy(&info) * 100);
		case INFO_PKG_POWER:
			return (int)(get_info_pkg_power(&info) * 100);
		case INFO_PKG_PL1:
			return (int)(get_info_pkg_pl1(&info) * 100);
		case INFO_PKG_PL2:
			return (int)(get_info_pkg_pl2(&info) * 100);
		case INFO_BCLK:
		case INFO_BUS_CLOCK:
			return (int) (get_info_bus_clock(&info) * 100);
		default:
			return CPU_INVALID_VALUE;
	}
}

#endif // RDMSR_UNSUPPORTED_OS
