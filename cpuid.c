// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <intrin.h>
#include "nwinfo.h"
#include "libcpuid.h"

static const char* kb_human_sizes[6] =
{ "KB", "MB", "GB", "TB", "PB", "EB", };

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

static void
PrintSgx(const struct cpu_raw_data_t* raw, const struct cpu_id_t* data)
{
	int i;
	printf("SGX: %d (%s)\n", data->sgx.present, data->sgx.present ? "present" : "absent");
	if (data->sgx.present) {
		printf("SGX max enclave size (32-bit): 2^%d\n", data->sgx.max_enclave_32bit);
		printf("SGX max enclave size (64-bit): 2^%d\n", data->sgx.max_enclave_64bit);
		printf("SGX1 extensions              : %d (%s)\n", data->sgx.flags[INTEL_SGX1], data->sgx.flags[INTEL_SGX1] ? "present" : "absent");
		printf("SGX2 extensions              : %d (%s)\n", data->sgx.flags[INTEL_SGX2], data->sgx.flags[INTEL_SGX2] ? "present" : "absent");
		printf("SGX MISCSELECT               : %08x\n", data->sgx.misc_select);
		printf("SGX SECS.ATTRIBUTES mask     : %016llx\n", (unsigned long long) data->sgx.secs_attributes);
		printf("SGX SECS.XSAVE feature mask  : %016llx\n", (unsigned long long) data->sgx.secs_xfrm);
		printf("SGX EPC sections count       : %d\n", data->sgx.num_epc_sections);
		for (i = 0; i < data->sgx.num_epc_sections; i++) {
			struct cpu_epc_t epc = cpuid_get_epc(i, raw);
			printf("    SGX EPC section #%-8d: start = %llx, size = %llu\n", i,
				(unsigned long long) epc.start_addr, (unsigned long long) epc.length);
		}
	}
}

struct cpu_id_t* get_cached_cpuid(void);

static int rdmsr_supported(void)
{
	struct cpu_id_t* id = get_cached_cpuid();
	return id->flags[CPU_FEATURE_MSR];
}

static void
PrintMsr(void)
{
	int value = CPU_INVALID_VALUE;
	struct msr_driver_t* handle = NULL;
	if (!rdmsr_supported()) {
		return;
	}
	if ((handle = cpu_msr_driver_open()) == NULL) {
		return;
	}

	int min_multi = cpu_msrinfo(handle, INFO_MIN_MULTIPLIER);
	int max_multi = cpu_msrinfo(handle, INFO_MAX_MULTIPLIER);
	int cur_multi = cpu_msrinfo(handle, INFO_CUR_MULTIPLIER);
	if (min_multi == CPU_INVALID_VALUE)
		min_multi = 0;
	if (max_multi == CPU_INVALID_VALUE)
		max_multi = 0;
	if (cur_multi == CPU_INVALID_VALUE)
		cur_multi = 0;
	printf("Multiplier: x%.1lf (%d-%d)\n", cur_multi / 100.0, min_multi / 100, max_multi / 100);
	if ((value = cpu_msrinfo(handle, INFO_TEMPERATURE)) != CPU_INVALID_VALUE)
		printf("Temperature: %d (C)\n", value);
	if ((value = cpu_msrinfo(handle, INFO_THROTTLING)) != CPU_INVALID_VALUE)
		printf("Throttling: %s\n", value ? "yes" : "no");
	if ((value = cpu_msrinfo(handle, INFO_VOLTAGE)) != CPU_INVALID_VALUE)
		printf("Core voltage: %.2lf V\n", value / 100.0);
	if ((value = cpu_msrinfo(handle, INFO_BUS_CLOCK)) != CPU_INVALID_VALUE)
		printf("Bus clock: %.2lf MHz\n", value / 100.0);
	cpu_msr_driver_close(handle);
}

void nwinfo_cpuid(int debug_level)
{
	struct cpu_raw_data_t raw = { 0 };
	struct cpu_id_t data = { 0 };
	int i = 0;

	cpuid_set_verbosiness_level(debug_level);

	if (cpuid_get_raw_data(&raw) < 0) {
		printf("Cannot obtain raw CPU data!\n");
		return;
	}

	if (cpu_identify(&raw, &data) < 0)
		printf("Error identifying the CPU: %s\n", cpuid_error());
	PrintHypervisor();
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

	printf("CPU clock: %d MHz\n", cpu_clock_measure(200, 1));
	//printf("CPU clock: %d MHz\n", cpu_clock());
	PrintSgx(&raw, &data);
	PrintMsr();
}
