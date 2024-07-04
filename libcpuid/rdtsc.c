/*
 * Copyright 2008  Veselin Georgiev,
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
#include <stdio.h>
#include <string.h>
#include "libcpuid.h"
#include "libcpuid_util.h"
#include "asm-bits.h"
#include "rdtsc.h"

#include <windows.h>
void sys_precise_clock(uint64_t *result)
{
	double c, f;
	LARGE_INTEGER freq, counter;
	QueryPerformanceCounter(&counter);
	QueryPerformanceFrequency(&freq);
	c = (double) counter.QuadPart;
	f = (double) freq.QuadPart;
	*result = (uint64_t) ( c * 1000000.0 / f );
}

/* out = a - b */
static void mark_t_subtract(struct cpu_mark_t* a, struct cpu_mark_t* b, struct cpu_mark_t *out)
{
	out->tsc = a->tsc - b->tsc;
	out->sys_clock = a->sys_clock - b->sys_clock;
}

void cpu_tsc_mark(struct cpu_mark_t* mark)
{
	cpu_rdtsc(&mark->tsc);
	sys_precise_clock(&mark->sys_clock);
}

void cpu_tsc_unmark(struct cpu_mark_t* mark)
{
	struct cpu_mark_t temp;
	cpu_tsc_mark(&temp);
	mark_t_subtract(&temp, mark, mark);
}


int cpu_clock_by_mark(struct cpu_mark_t* mark)
{
	uint64_t result;

	/* Check if some subtraction resulted in a negative number: */
	if ((mark->tsc >> 63) != 0 || (mark->sys_clock >> 63) != 0) return -1;

	/* Divide-by-zero check: */
	if (mark->sys_clock == 0) return -1;

	/* Check if the result fits in 32bits */
	result = mark->tsc / mark->sys_clock;
	if (result > (uint64_t) 0x7fffffff) return -1;
	return (int) result;
}

int cpu_clock_by_os(void)
{
	HKEY key;
	DWORD result;
	DWORD size = 4;

	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
		0, KEY_READ, &key) != ERROR_SUCCESS)
		return -1;

	if (RegQueryValueExW(key, L"~MHz",
		NULL, NULL, (LPBYTE) &result, (LPDWORD) &size) != ERROR_SUCCESS)
	{
		RegCloseKey(key);
		return -1;
	}
	RegCloseKey(key);

	return (int)result;
}

/* Emulate doing useful CPU intensive work */
static int busy_loop(int amount)
{
	int i, j, k, s = 0;
	static volatile int data[42] = {32, 12, -1, 5, 23, 0 };
	for (i = 0; i < amount; i++)
		for (j = 0; j < 65536; j++)
			for (k = 0; k < 42; k++)
				s += data[k];
	return s;
}

int busy_loop_delay(int milliseconds)
{
	int cycles = 0, r = 0, first = 1;
	uint64_t a, b, c;
	sys_precise_clock(&a);
	while (1) {
		sys_precise_clock(&c);
		if ((c - a) / 1000 > milliseconds) return r;
		r += busy_loop(cycles);
		if (first) {
			first = 0;
		} else {
			if (c - b < 1000) cycles *= 2;
			if (c - b > 10000) cycles /= 2;
		}
		b = c;
	}
}

int cpu_clock_measure(int millis, int quad_check)
{
	struct cpu_mark_t begin[4], end[4], temp, temp2;
	int results[4], cycles, n, k, i, j, bi, bj, mdiff, diff, _zero = 0;
	uint64_t tl;

	if (millis < 1) return -1;
	tl = millis * (uint64_t) 1000;
	if (quad_check)
		tl /= 4;
	n = quad_check ? 4 : 1;
	cycles = 1;
	for (k = 0; k < n; k++) {
		cpu_tsc_mark(&begin[k]);
		end[k] = begin[k];
		do {
			/* Run busy loop, and fool the compiler that we USE the garbishy
			   value it calculates */
			_zero |= (1 & busy_loop(cycles));
			cpu_tsc_mark(&temp);
			mark_t_subtract(&temp, &end[k], &temp2);
			/* If busy loop is too short, increase it */
			if (temp2.sys_clock < tl / 8)
				cycles *= 2;
			end[k] = temp;
		} while (end[k].sys_clock - begin[k].sys_clock < tl);
		mark_t_subtract(&end[k], &begin[k], &temp);
		results[k] = cpu_clock_by_mark(&temp);
	}
	if (n == 1) return results[0];
	mdiff = 0x7fffffff;
	bi = bj = -1;
	for (i = 0; i < 4; i++) {
		for (j = i + 1; j < 4; j++) {
			diff = results[i] - results[j];
			if (diff < 0) diff = -diff;
			if (diff < mdiff) {
				mdiff = diff;
				bi = i;
				bj = j;
			}
		}
	}
	if (results[bi] == -1) return -1;
	return (results[bi] + results[bj] + _zero) / 2;
}


static void adjust_march_ic_multiplier(const struct cpu_id_t* id, int* numerator, int* denom)
{
	/*
	 * for cpu_clock_by_ic: we need to know how many clocks does a typical ADDPS instruction
	 * take, when issued in rapid succesion without dependencies. The whole idea of
	 * cpu_clock_by_ic was that this is easy to determine, at least it was back in 2010. Now
	 * it's getting progressively more hairy, but here are the current measurements:
	 *
	 * 1. For CPUs with  64-bit SSE units, ADDPS issue rate is 0.5 IPC (one insn in 2 clocks)
	 * 2. For CPUs with 128-bit SSE units, issue rate is exactly 1.0 IPC
	 * 3. For Bulldozer and later, it is 1.4 IPC (we multiply by 5/7)
	 * 4. For Skylake and later, it is 1.6 IPC (we multiply by 5/8)
	 */
	//
	if (id->x86.sse_size < 128) {
		debugf(1, "SSE execution path is 64-bit\n");
		// on a CPU with half SSE unit length, SSE instructions execute at 0.5 IPC;
		// the resulting value must be multiplied by 2:
		*numerator = 2;
	} else {
		debugf(1, "SSE execution path is 128-bit\n");
	}
	//
	// Bulldozer or later: assume 1.4 IPC
	if ((id->vendor == VENDOR_AMD && id->x86.ext_family >= 21) || (id->vendor == VENDOR_HYGON)) {
		debugf(1, "cpu_clock_by_ic: Bulldozer (or later) detected, dividing result by 1.4\n");
		*numerator = 5;
		*denom = 7; // multiply by 5/7, to divide by 1.4
	}
	//
	// Skylake or later: assume 1.6 IPC
	if (id->vendor == VENDOR_INTEL && id->x86.ext_model >= 94) {
		debugf(1, "cpu_clock_by_ic: Skylake (or later) detected, dividing result by 1.6\n");
		*numerator = 5;
		*denom = 8; // to divide by 1.6, multiply by 5/8
	}
}

int cpu_clock_by_ic(int millis, int runs)
{
	int max_value = 0, cur_value, i, ri, cycles_inner, cycles_outer, c;
	struct cpu_id_t* id;
	uint64_t t0, t1, tl, hz;
	int multiplier_numerator = 1, multiplier_denom = 1;
	if (millis <= 0 || runs <= 0) return -2;
	id = get_cached_cpuid();
	// if there aren't SSE instructions - we can't run the test at all
	if (!id || !id->flags[CPU_FEATURE_SSE]) return -1;
	//
	adjust_march_ic_multiplier(id, &multiplier_numerator, &multiplier_denom);
	//
	tl = 125ULL * millis; // (*1000 / 8)
	cycles_inner = 128;
	cycles_outer = 1;
	do {
		if (cycles_inner < 1000000000) cycles_inner *= 2;
		else cycles_outer *= 2;
		sys_precise_clock(&t0);
		for (i = 0; i < cycles_outer; i++)
			busy_sse_loop(cycles_inner);
		sys_precise_clock(&t1);
	} while (t1 - t0 < tl);
	debugf(2, "inner: %d, outer: %d\n", cycles_inner, cycles_outer);
	for (ri = 0; ri < runs; ri++) {
		sys_precise_clock(&t0);
		c = 0;
		do {
			c++;
			for (i = 0; i < cycles_outer; i++)
				busy_sse_loop(cycles_inner);
			sys_precise_clock(&t1);
		} while (t1 - t0 < tl * (uint64_t) 8);
		// cpu_Hz = cycles_inner * cycles_outer * 256 / (t1 - t0) * 1000000
		debugf(2, "c = %d, td = %d\n", c, (int) (t1 - t0));
		hz = ((uint64_t) cycles_inner * (uint64_t) 256 + 12) *
		     (uint64_t) cycles_outer * (uint64_t) multiplier_numerator * (uint64_t) c * (uint64_t) 1000000
		     / ((t1 - t0) * (uint64_t) multiplier_denom);
		cur_value = (int) (hz / 1000000);
		if (cur_value > max_value) max_value = cur_value;
	}
	return max_value;
}

int cpu_clock_by_tsc(struct cpu_raw_data_t* raw)
{
	/* Documentation:
	 * Intel 64 and IA-32 Architectures Software Developer's Manual
	 * Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
	 * 20.7.3 Determining the Processor Base Frequency
	 */
	uint16_t base_freq_mhz;
	uint32_t denominator, numerator, nominal_freq_khz;
	struct cpu_raw_data_t myraw;
	struct cpu_id_t id;

	/* Get CPUID raw data and identy CPU */
	if (!raw) {
		if (cpuid_get_raw_data(&myraw) < 0) {
			warnf("cpu_clock_by_tsc: raw CPUID cannot be obtained\n");
			return -2;
		}
		raw = &myraw;
	}
	if (cpu_identify(raw, &id) != ERR_OK) {
		warnf("cpu_clock_by_tsc: CPU cannot be identified\n");
		return -2;
	}

	/* Check if Time Stamp Counter and Nominal Core Crystal Clock Information Leaf is supported */
	if ((id.vendor != VENDOR_INTEL) || (raw->basic_cpuid[0][EAX] < 0x15)) {
		debugf(1, "cpu_clock_by_tsc: Time Stamp Counter and Nominal Core Crystal Clock Information Leaf is not supported\n");
		return -1;
	}

	denominator      = raw->basic_cpuid[0x15][EAX]; // Bits 31-00: An unsigned integer which is the denominator of the TSC/"core crystal clock" ratio
	numerator        = raw->basic_cpuid[0x15][EBX]; // Bits 31-00: An unsigned integer which is the numerator of the TSC/"core crystal clock" ratio
	nominal_freq_khz = raw->basic_cpuid[0x15][ECX] / 1000; // Bits 31-00: An unsigned integer which is the nominal frequency of the core crystal clock in Hz

	/* If EBX[31:0] (numerator) is 0, the TSC/"core crystal clock" ratio is not enumerated. */
	if ((numerator == 0) || (denominator == 0)) {
		debugf(1, "cpu_clock_by_tsc: TSC/\"core crystal clock\" ratio is not enumerated\n");
		return -1;
	}

	/* If ECX is 0, the nominal core crystal clock frequency is not enumerated.
	For Intel processors in which CPUID.15H.EBX[31:0] / CPUID.0x15.EAX[31:0] is enumerated but CPUID.15H.ECX
	is not enumerated, Table 20-91 can be used to look up the nominal core crystal clock frequency. */
	if ((nominal_freq_khz == 0) && (id.x86.ext_family == 0x6)) {
		debugf(1, "cpu_clock_by_tsc: nominal core crystal clock frequency is not enumerated, looking for CPUID signature %02X_%02XH\n", id.x86.ext_family, id.x86.ext_model);
		switch (id.x86.ext_model) {
			case 0x55:
				/* Intel Xeon Scalable Processor Family with CPUID signature 06_55H */
				nominal_freq_khz = 25000; // 25 MHz
				break;
			case 0x4e:
			case 0x5e:
			case 0x8e:
			case 0x9e:
				/* 6th and 7th generation Intel CoreTM processors and Intel Xeon W Processor Family */
				nominal_freq_khz = 24000; // 24 MHz
				break;
			case 0x5c:
				/* Next Generation Intel Atom processors based on Goldmont Microarchitecture with
				CPUID signature 06_5CH (does not include Intel Xeon processors) */
				nominal_freq_khz = 19200; // 19.2 MHz
				break;
			default:
				break;
		}
	}

	/* From native_calibrate_tsc() in Linux: https://github.com/torvalds/linux/blob/master/arch/x86/kernel/tsc.c#L696-L707
	Some Intel SoCs like Skylake and Kabylake don't report the crystal
	clock, but we can easily calculate it to a high degree of accuracy
	by considering the crystal ratio and the CPU speed. */
	if ((nominal_freq_khz == 0) && (raw->basic_cpuid[0][EAX] >= 0x16)) {
		base_freq_mhz    = EXTRACTS_BITS(raw->basic_cpuid[0x16][EAX], 15, 0);
		nominal_freq_khz = base_freq_mhz * 1000 * denominator / numerator;
		debugf(1, "cpu_clock_by_tsc: no crystal clock frequency detected, using base frequency (%u MHz) to calculate it\n", base_freq_mhz);
	}

	if (nominal_freq_khz == 0) {
		debugf(1, "cpu_clock_by_tsc: no crystal clock frequency detected\n");
		return -1;
	}

	/* For Intel processors in which the nominal core crystal clock frequency is enumerated in CPUID.15H.ECX and the
	core crystal clock ratio is encoded in CPUID.15H (see Table 3-8 "Information Returned by CPUID Instruction"), the
	nominal TSC frequency can be determined by using the following equation:
	Nominal TSC frequency = ( CPUID.15H.ECX[31:0] * CPUID.15H.EBX[31:0] ) / CPUID.15H.EAX[31:0] */
	debugf(1, "cpu_clock_by_tsc: denominator=%u, numerator=%u, nominal_freq_khz=%u\n", denominator, numerator, nominal_freq_khz);

	/* Return TSC frequency in MHz */
	return (nominal_freq_khz * numerator) / denominator / 1000;
}

int cpu_clock(void)
{
	int result;
	result = cpu_clock_by_os();
	if (result <= 0)
		result = cpu_clock_measure(200, 1);
	return result;
}
