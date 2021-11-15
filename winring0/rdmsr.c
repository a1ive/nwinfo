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

#include "winring0.h"

#define MSR_PATH_LEN 32

#include <windows.h>
#include <winioctl.h>
#include <winerror.h>

extern uint8_t cc_x86driver_code[];
extern int cc_x86driver_code_size;
extern uint8_t cc_x64driver_code[];
extern int cc_x64driver_code_size;

struct msr_driver_t {
	char driver_path[MAX_PATH + 1];
	SC_HANDLE scManager;
	SC_HANDLE scDriver;
	HANDLE hhDriver;
	int errorcode;
};

static BOOL MsrLoadDriver(struct msr_driver_t* drv)
{
	BOOL Ret = FALSE;
	DWORD Status = 0;

	debugf(1, "Load driver: %s\n", drv->driver_path);

	drv->scManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (drv->scManager == NULL) {
		debugf(1, "OpenSCManager() failed %d.\n", GetLastError());
		return FALSE;
	}

	debugf(1, "OpenSCManager() ok.\n");

	drv->scDriver = CreateServiceA(drv->scManager, OLS_DRIVER_ID, OLS_DRIVER_ID,
		SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
		drv->driver_path, NULL, NULL, NULL, NULL, NULL);
	if (drv->scDriver == NULL) {
		Status = GetLastError();
		if (Status != ERROR_IO_PENDING && Status != ERROR_SERVICE_EXISTS) {
			debugf(1, "CreateService() failed %d.\n", Status);
			CloseServiceHandle(drv->scManager);
			return FALSE;
		}

		drv->scDriver = OpenServiceA(drv->scManager, OLS_DRIVER_ID, SERVICE_ALL_ACCESS);
		if (drv->scDriver == NULL) {
			debugf(1, "OpenService() failed %d.\n", Status);
			CloseServiceHandle(drv->scManager);
			return FALSE;
		}
	}

	debugf(1, "CreateService() ok.\n");

	Ret = StartServiceA(drv->scDriver, 0, NULL);
	if (Ret) {
		debugf(1, "StartService() ok.\n");
	} else {
		Status = GetLastError();
		if (Status == ERROR_SERVICE_ALREADY_RUNNING) {
			Ret = TRUE;
		} else {
			debugf(1, "StartService() error %d.\n", Status);
			Ret = FALSE;
		}
	}

	CloseServiceHandle(drv->scDriver);
	CloseServiceHandle(drv->scManager);

	debugf(1, "Load driver %s\n", Ret ? "success" : "failed");

	return Ret;
}

static int rdmsr_supported(void);

#ifndef _WIN64
typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
static BOOL is_running_x64(void)
{
	BOOL bIsWow64 = FALSE;

	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process");
	if (NULL != fnIsWow64Process)
		fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
	return bIsWow64;
}
#endif

static int extract_driver(struct msr_driver_t* driver)
{

	FILE* f;
	if (!GetTempPathA(sizeof(driver->driver_path), driver->driver_path))
		return 0;
	strcat(driver->driver_path, "WinRing0.sys");

	f = fopen(driver->driver_path, "wb");
	if (!f) return 0;
#ifndef _WIN64
	if (is_running_x64())
#endif
		fwrite(cc_x64driver_code, 1, cc_x64driver_code_size, f);
#ifndef _WIN64
	else
		fwrite(cc_x86driver_code, 1, cc_x86driver_code_size, f);
#endif
	fclose(f);
	return 1;
}

struct msr_driver_t* cpu_msr_driver_open(void)
{
	struct msr_driver_t* drv;
	BOOL status = FALSE;
	if (!rdmsr_supported()) {
		set_error(ERR_NO_RDMSR);
		return NULL;
	}

	drv = (struct msr_driver_t*) malloc(sizeof(struct msr_driver_t));
	if (!drv) {
		set_error(ERR_NO_MEM);
		return NULL;
	}
	memset(drv, 0, sizeof(struct msr_driver_t));

	if (!extract_driver(drv)) {
		free(drv);
		set_error(ERR_EXTRACT);
		return NULL;
	}
	status = MsrLoadDriver(drv);
	if (status) {
		drv->hhDriver = CreateFileA("\\\\.\\"OLS_DRIVER_ID,
			GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);
		if (drv->hhDriver == INVALID_HANDLE_VALUE) {
			debugf(1, "Create Device failed %d.\n", GetLastError());
			status = FALSE;
		}
	}

	if (!DeleteFileA(drv->driver_path))
		debugf(1, "Deleting temporary driver file failed.\n");
	if (!status) {
		debugf(1, "driver open failed.\n");
		set_error(drv->errorcode ? drv->errorcode : ERR_NO_DRIVER);
		free(drv);
		return NULL;
	}
	debugf(1, "driver open success.\n");
	return drv;
}

struct msr_driver_t* cpu_msr_driver_open_core(unsigned core_num)
{
	warnf("cpu_msr_driver_open_core(): parameter ignored (function is the same as cpu_msr_driver_open)\n");
	return cpu_msr_driver_open();
}

int cpu_rdmsr(struct msr_driver_t* driver, uint32_t msr_index, uint64_t* result)
{
	DWORD dwBytesReturned;
	UINT64 MsrData = 0;
	BOOL Res = FALSE;

	if (!driver)
		return set_error(ERR_HANDLE);
	Res = DeviceIoControl(driver->hhDriver, IOCTL_OLS_READ_MSR,
		&msr_index, sizeof(msr_index), &MsrData, sizeof(MsrData), &dwBytesReturned, NULL);
	if (Res == FALSE)
		return -1;
	*result = MsrData;
	return 0;
}

int cpu_msr_driver_close(struct msr_driver_t* drv)
{
	SERVICE_STATUS srvStatus = {0};
	if (drv == NULL)
		return 0;
	if (drv->hhDriver && drv->hhDriver != INVALID_HANDLE_VALUE) {
		CloseHandle(drv->hhDriver);
		drv->hhDriver = NULL;
	}
	if (drv->scDriver) {
		CloseServiceHandle(drv->scDriver);
		drv->scDriver = NULL;
	}
	if (drv->scManager) {
		CloseServiceHandle(drv->scManager);
		drv->scManager = NULL;
	}
	free(drv);
	return 0;
}

#ifndef RDMSR_UNSUPPORTED_OS

/* Useful links for hackers:
- AMD MSRs:
  AMD BIOS and Kernel Developer’s Guide (BKDG)
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
  https://support.amd.com/TechDocs/54945_PPR_Family_17h_Models_00h-0Fh.pdf

- Intel MSRs:
  Intel 64 and IA-32 Architectures Software Developer’s Manual
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
	CPU_INVALID_VALUE
};

/* Intel MSRs addresses */
#define IA32_MPERF             0xE7
#define IA32_APERF             0xE8
#define IA32_PERF_STATUS       0x198
#define IA32_THERM_STATUS      0x19C
#define MSR_EBL_CR_POWERON     0x2A
#define MSR_TURBO_RATIO_LIMIT  0x1AD
#define MSR_TEMPERATURE_TARGET 0x1A2
#define MSR_PERF_STATUS        0x198
#define MSR_PLATFORM_INFO      0xCE
static const uint32_t intel_msr[] = {
	IA32_MPERF,
	IA32_APERF,
	IA32_PERF_STATUS,
	IA32_THERM_STATUS,
	MSR_EBL_CR_POWERON,
	MSR_TURBO_RATIO_LIMIT,
	MSR_TEMPERATURE_TARGET,
	MSR_PERF_STATUS,
	MSR_PLATFORM_INFO,
	CPU_INVALID_VALUE
};

struct msr_info_t {
	int cpu_clock;
	struct msr_driver_t *handle;
	struct cpu_id_t *id;
	struct internal_id_info_t *internal;
};

static int rdmsr_supported(void)
{
	struct cpu_id_t* id = get_cached_cpuid();
	return id->flags[CPU_FEATURE_MSR];
}

static int perfmsr_measure(struct msr_driver_t* handle, int msr)
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
	const int magic_constant = (info->id->ext_family == 0x11) ? 0x8 : 0x10;
	const int is_apu = ((FUSION_C <= info->internal->code.amd) && (info->internal->code.amd <= FUSION_A)) || (info->internal->bits & _APU_);
	const double divisor = is_apu ? 1.0 : 2.0;

	/* Check if P-state is valid */
	if (pstate < MSR_PSTATE_0 || MSR_PSTATE_7 < pstate)
		return 1;

	switch (info->id->ext_family) {
		case 0x12:
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
		case 0x14:
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
		case 0x10:
			/* BKDG 10h, page 429
			MSRC001_00[6B:64][8:6] is CpuDid
			MSRC001_00[6B:64][5:0] is CpuFid
			CPU COF is (100 MHz * (CpuFid + 10h) / (2^CpuDid))
			Note: This family contains only CPUs */
		case 0x11:
			/* BKDG 11h, page 236
			MSRC001_00[6B:64][8:6] is CpuDid
			MSRC001_00[6B:64][5:0] is CpuFid
			CPU COF is ((100 MHz * (CpuFid + 08h)) / (2^CpuDid))
			Note: This family contains only CPUs */
		case 0x15:
			/* BKDG 15h, page 570/580/635/692 (00h-0Fh/10h-1Fh/30h-3Fh/60h-6Fh)
			MSRC001_00[6B:64][8:6] is CpuDid
			MSRC001_00[6B:64][5:0] is CpuFid
			CoreCOF is (100 * (MSRC001_00[6B:64][CpuFid] + 10h) / (2^MSRC001_00[6B:64][CpuDid]))
			Note: This family contains BOTH CPUs and APUs */
		case 0x16:
			/* BKDG 16h, page 549/611 (00h-0Fh/30h-3Fh)
			MSRC001_00[6B:64][8:6] is CpuDid
			MSRC001_00[6B:64][5:0] is CpuFid
			CoreCOF is (100 * (MSRC001_00[6B:64][CpuFid] + 10h) / (2^MSRC001_00[6B:64][CpuDid]))
			Note: This family contains only APUs */
			err  = cpu_rdmsr_range(info->handle, pstate, 8, 6, &CpuDid);
			err += cpu_rdmsr_range(info->handle, pstate, 5, 0, &CpuFid);
			*multiplier = ((double) (CpuFid + magic_constant) / (1ull << CpuDid)) / divisor;
			break;
		case 0x17:
			/* PPR 17h, pages 30 and 138-139
			MSRC001_00[6B:64][13:8] is CpuDfsId
			MSRC001_00[6B:64][7:0]  is CpuFid
			CoreCOF is (Core::X86::Msr::PStateDef[CpuFid[7:0]] / Core::X86::Msr::PStateDef[CpuDfsId]) * 200 */
			err  = cpu_rdmsr_range(info->handle, pstate, 13, 8, &CpuDid);
			err += cpu_rdmsr_range(info->handle, pstate,  7, 0, &CpuFid);
			*multiplier = ((double) CpuFid / CpuDid) * 2;
			break;
		default:
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

	if(info->id->vendor == VENDOR_INTEL) {
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

static int get_info_temperature(struct msr_info_t *info)
{
	int err;
	uint64_t DigitalReadout, ReadingValid, TemperatureTarget;

	if(info->id->vendor == VENDOR_INTEL) {
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

static double get_info_voltage(struct msr_info_t *info)
{
	int err;
	double VIDStep;
	uint64_t reg, CpuVid;

	if(info->id->vendor == VENDOR_INTEL) {
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
		VIDStep = ((info->id->ext_family < 0x15) || ((info->id->ext_family == 0x15) && (info->id->ext_model < 0x10))) ? 0.0125 : 0.00625;
		err = cpu_rdmsr_range(info->handle, MSR_PSTATE_S, 2, 0, &reg);
		if(info->id->ext_family < 0x17)
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

	if(info->id->vendor == VENDOR_INTEL) {
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

int cpu_rdmsr_range(struct msr_driver_t* handle, uint32_t msr_index, uint8_t highbit,
					uint8_t lowbit, uint64_t* result)
{
	int err;
	const uint8_t bits = highbit - lowbit + 1;

	if(highbit > 63 || lowbit > highbit)
		return set_error(ERR_INVRANGE);

	err = cpu_rdmsr(handle, msr_index, result);

	if(!err && bits < 64) {
		/* Show only part of register */
		*result >>= lowbit;
		*result &= (1ULL << bits) - 1;
	}

	return err;
}

int cpu_msrinfo(struct msr_driver_t* handle, cpu_msrinfo_request_t which)
{
	static int err = 0, init = 0;
	struct cpu_raw_data_t raw;
	static struct cpu_id_t id;
	static struct internal_id_info_t internal;
	static struct msr_info_t info = { 0 };

	if (handle == NULL) {
		set_error(ERR_HANDLE);
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
		case INFO_THROTTLING:
			return CPU_INVALID_VALUE;
		case INFO_VOLTAGE:
			return (int) (get_info_voltage(&info) * 100);
		case INFO_BCLK:
		case INFO_BUS_CLOCK:
			return (int) (get_info_bus_clock(&info) * 100);
		default:
			return CPU_INVALID_VALUE;
	}
}

int msr_serialize_raw_data(struct msr_driver_t* handle, const char* filename)
{
	int i, j;
	FILE *f;
	uint64_t reg;
	const uint32_t *msr;
	struct cpu_raw_data_t raw;
	struct cpu_id_t id;
	struct internal_id_info_t internal;

	if (handle == NULL)
		return set_error(ERR_HANDLE);

	if ((filename == NULL) || !strcmp(filename, ""))
		f = stdout;
	else
		f = fopen(filename, "wt");
	if (!f) return set_error(ERR_OPEN);

	if (cpuid_get_raw_data(&raw) || cpu_ident_internal(&raw, &id, &internal))
		return -1;

	fprintf(f, "CPU is %s %s, stock clock is %dMHz.\n", id.vendor_str, id.brand_str, cpu_clock_measure(250, 1));
	switch (id.vendor) {
		case VENDOR_HYGON:
		case VENDOR_AMD:   msr = amd_msr; break;
		case VENDOR_INTEL: msr = intel_msr; break;
		default: return set_error(ERR_CPU_UNKN);
	}

	for (i = 0; msr[i] != CPU_INVALID_VALUE; i++) {
		cpu_rdmsr(handle, msr[i], &reg);
		fprintf(f, "msr[%#08x]=", msr[i]);
		for (j = 56; j >= 0; j -= 8)
			fprintf(f, "%02x ", (int) (reg >> j) & 0xff);
		fprintf(f, "\n");
	}

	if ((filename != NULL) && strcmp(filename, ""))
		fclose(f);
	return set_error(ERR_OK);
}

#endif // RDMSR_UNSUPPORTED_OS
