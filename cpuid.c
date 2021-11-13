// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nwinfo.h"
#include "libcpuid/libcpuid.h"

#if 0
#include <intrin.h>

void exec_cpuid(uint32_t* regs)
{
	int Eax = regs[0];
	int Ecx = regs[2];
	int cpuInfo[4] = { 0 };
	__cpuidex(cpuInfo, Eax, Ecx);
	memcpy(regs, cpuInfo, sizeof(cpuInfo));
}

int cpu_clock(void)
{
	HKEY key;
	DWORD result;
	DWORD size = 4;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), 0, KEY_READ, &key) != ERROR_SUCCESS)
		return -1;

	if (RegQueryValueEx(key, TEXT("~MHz"), NULL, NULL, (LPBYTE)&result, (LPDWORD)&size) != ERROR_SUCCESS) {
		RegCloseKey(key);
		return -1;
	}
	RegCloseKey(key);

	return (int)result;
}
#endif

void nwinfo_cpuid(void)
{
	struct cpu_raw_data_t raw = { 0 };
	struct cpu_id_t data = { 0 };
	int i = 0;

	cpuid_set_verbosiness_level(0);

	if (cpuid_get_raw_data(&raw) < 0) {
		printf("Cannot obtain raw CPU data!\n");
		return;
	}

	if (cpu_identify(&raw, &data) < 0)
		printf("Error identifying the CPU: %s\n", cpuid_error());

	printf("Vendor: %s\n", data.vendor_str);
	printf("Brand: %s\n", data.brand_str);
	printf("Code Name: %s\n", data.cpu_codename);
	printf("Family:     %02Xh Module:     %02Xh Stepping: %02Xh\n", data.family, data.model, data.stepping);
	printf("Ext.Family: %02Xh Ext.Module: %02Xh\n", data.ext_family, data.ext_model);

	printf("Cores: %d\n", data.num_cores);
	printf("Logical CPUs: %d\n", data.num_logical_cpus);
	printf("Total CPUs: %d\n", cpuid_get_total_cpus());
	printf("Cache:\n");
	if (data.l1_data_cache > 0)
		printf("  L1 D: %d KB, %d-way\n", data.l1_data_cache, data.l1_data_assoc);
	if (data.l1_instruction_cache > 0)
		printf("  L1 I: %d KB, %d-way\n", data.l1_instruction_cache, data.l1_instruction_assoc);
	if (data.l2_cache > 0)
		printf("  L2:   %d KB, %d-way\n", data.l2_cache, data.l2_assoc);
	if (data.l3_cache > 0)
		printf("  L3:   %d KB, %d-way\n", data.l3_cache, data.l3_assoc);
	if (data.l4_cache > 0)
		printf("  L4:   %d KB, %d-way\n", data.l4_cache, data.l4_assoc);

	printf("SSE units: %d bits (%s)\n", data.sse_size, data.detection_hints[CPU_HINT_SSE_SIZE_AUTH] ? "authoritative" : "non-authoritative");
	printf("Features:");
	/*
	 * Here we enumerate all CPU feature bits, and when a feature
	 * is present output its name:
	 */
	for (i = 0; i < NUM_CPU_FEATURES; i++)
		if (data.flags[i])
			printf(" %s", cpu_feature_str(i));
	printf("\n");

	//printf("CPU clock: %d MHz\n", cpu_clock_measure(400, 1));
	printf("CPU clock: %d MHz\n", cpu_clock());

}
