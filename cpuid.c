// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <intrin.h>
#include "nwinfo.h"
#include "libcpuid/libcpuid.h"

static void
PrintHypervisor(void)
{
	int cpuInfo[4] = { 0 };
	unsigned MaxFunc = 0;
	char VmSign[13] = { 0 };
	__cpuid(cpuInfo, 0);
	MaxFunc = cpuInfo[0];
	if (MaxFunc < 1U)
		return;
	__cpuid(cpuInfo, 1);
	if (((unsigned)cpuInfo[2] & (1U << 31U)) == 0)
		return;
	__cpuid(cpuInfo, 0x40000000U);
	memcpy(VmSign, &cpuInfo[1], 12);
	if (strcmp(VmSign, "VMwareVMware") == 0)
		printf("Hypervisor: VMware\n");
	else if (strcmp(VmSign, "Microsoft Hv") == 0)
		printf("Hypervisor: Microsoft Hyper-V\n");
	else if (strcmp(VmSign, "KVMKVMKVM") == 0)
		printf("Hypervisor: KVM\n");
	else if (strcmp(VmSign, "VBoxVBoxVBox") == 0)
		printf("Hypervisor: VirtualBox\n");
	else if (strcmp(VmSign, "XenVMMXenVMM") == 0)
		printf("Hypervisor: Xen\n");
	else if (strcmp(VmSign, "prl hyperv") == 0)
		printf("Hypervisor: Parallels\n");
	else if (strcmp(VmSign, "TCGTCGTCGTCG") == 0)
		printf("Hypervisor: QEMU\n");
	else if (strcmp(VmSign, "bhyve bhyve") == 0)
		printf("Hypervisor: FreeBSD bhyve\n");
	else
		printf("Hypervisor: %s\n", VmSign);
}

static const char* kb_human_sizes[6] =
{ "KB", "MB", "GB", "TB", "PB", "EB", };

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
		printf("  L1 D: %d * %s, %d-way\n", data.num_cores, GetHumanSize(data.l1_data_cache, kb_human_sizes, 1024), data.l1_data_assoc);
	if (data.l1_instruction_cache > 0)
		printf("  L1 I: %d * %s, %d-way\n", data.num_cores, GetHumanSize(data.l1_instruction_cache, kb_human_sizes, 1024), data.l1_instruction_assoc);
	if (data.l2_cache > 0)
		printf("  L2:   %d * %s, %d-way\n", data.num_cores, GetHumanSize(data.l2_cache, kb_human_sizes, 1024), data.l2_assoc);
	if (data.l3_cache > 0)
		printf("  L3:   %s, %d-way\n", GetHumanSize(data.l3_cache, kb_human_sizes, 1024), data.l3_assoc);
	if (data.l4_cache > 0)
		printf("  L4:   %s, %d-way\n", GetHumanSize(data.l4_cache, kb_human_sizes, 1024), data.l4_assoc);

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
	PrintHypervisor();
}
