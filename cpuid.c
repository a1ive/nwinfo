// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <intrin.h>
#include "nwinfo.h"
#include "libcpuid.h"

static const char* kb_human_sizes[6] =
{ "KB", "MB", "GB", "TB", "PB", "EB", };

static PNODE node;

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
		node_att_set(node, "Hypervisor", "VMware", 0);
	else if (strcmp(VmSign, "Microsoft Hv") == 0)
		node_att_set(node, "Hypervisor", "Microsoft Hyper-V", 0);
	else if (strcmp(VmSign, "KVMKVMKVM") == 0)
		node_att_set(node, "Hypervisor", "KVM", 0);
	else if (strcmp(VmSign, "VBoxVBoxVBox") == 0)
		node_att_set(node, "Hypervisor", "VirtualBox", 0);
	else if (strcmp(VmSign, "XenVMMXenVMM") == 0)
		node_att_set(node, "Hypervisor", "Xen", 0);
	else if (strcmp(VmSign, "prl hyperv") == 0)
		node_att_set(node, "Hypervisor", "Parallels", 0);
	else if (strcmp(VmSign, "TCGTCGTCGTCG") == 0)
		node_att_set(node, "Hypervisor", "QEMU", 0);
	else if (strcmp(VmSign, "bhyve bhyve") == 0)
		node_att_set(node, "Hypervisor", "FreeBSD bhyve", 0);
	else
		node_att_set(node, "Hypervisor", VmSign, 0);
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
	if (!rdmsr_supported())
	{
		fprintf(stderr, "rdmsr not supported\n");
		return;
	}
	if ((handle = cpu_msr_driver_open()) == NULL)
	{
		fprintf(stderr, "Cannot load driver!\n");
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
	node_setf(node, "Multiplier", 0, "x%.1lf (%d-%d)", cur_multi / 100.0, min_multi / 100, max_multi / 100);
	if ((value = cpu_msrinfo(handle, INFO_TEMPERATURE)) != CPU_INVALID_VALUE)
		node_setf(node, "Temperature", 0, "%d (C)", value);
	if ((value = cpu_msrinfo(handle, INFO_THROTTLING)) != CPU_INVALID_VALUE)
		node_att_set_bool(node, "Throttling", value, 0);
	if ((value = cpu_msrinfo(handle, INFO_VOLTAGE)) != CPU_INVALID_VALUE)
		node_setf(node, "Core voltage", 0, "%.2lf V", value / 100.0);
	if ((value = cpu_msrinfo(handle, INFO_BUS_CLOCK)) != CPU_INVALID_VALUE)
		node_setf(node, "Bus clock", 0, "%.2lf MHz", value / 100.0);
	cpu_msr_driver_close(handle);
}

PNODE nwinfo_cpuid(void)
{
	struct cpu_raw_data_t raw = { 0 };
	struct cpu_id_t data = { 0 };
	int i = 0;
	PNODE cache, feature;

	node = node_alloc("CPUID", 0);
	if (cpuid_get_raw_data(&raw) < 0)
	{
		fprintf(stderr, "Cannot obtain raw CPU data!\n");
		return node;
	}

	if (cpu_identify(&raw, &data) < 0)
		fprintf(stderr, "Error identifying the CPU: %s\n", cpuid_error());
	PrintHypervisor();
	node_att_set(node, "Vendor", data.vendor_str, 0);
	node_att_set(node, "Brand", data.brand_str, 0);
	node_att_set(node, "Code Name", data.cpu_codename, 0);
	node_setf(node, "Family", 0, "%02Xh", data.family);
	node_setf(node, "Model", 0, "%02Xh", data.model);
	node_setf(node, "Stepping", 0, "%02Xh", data.stepping);
	node_setf(node, "Ext.Family", 0, "%02Xh", data.ext_family);
	node_setf(node, "Ext.Model", 0, "%02Xh", data.ext_model);

	node_setf(node, "Cores", NAFLG_FMT_NUMERIC, "%d", data.num_cores);
	node_setf(node, "Logical CPUs", NAFLG_FMT_NUMERIC, "%d", data.num_logical_cpus);
	node_setf(node, "Total CPUs", NAFLG_FMT_NUMERIC, "%d", cpuid_get_total_cpus());
	cache = node_append_new(node, "Cache", NFLG_PLACEHOLDER);
	if (data.l1_data_cache > 0)
		node_setf(cache, "L1 D", 0, "%d * %s, %d-way",
			data.num_cores, GetHumanSize(data.l1_data_cache, kb_human_sizes, 1024), data.l1_data_assoc);
	if (data.l1_instruction_cache > 0)
		node_setf(cache, "L1 I", 0, "%d * %s, %d-way",
			data.num_cores, GetHumanSize(data.l1_instruction_cache, kb_human_sizes, 1024), data.l1_instruction_assoc);
	if (data.l2_cache > 0)
		node_setf(cache, "L2", 0, "%d * %s, %d-way",
			data.num_cores, GetHumanSize(data.l2_cache, kb_human_sizes, 1024), data.l2_assoc);
	if (data.l3_cache > 0)
		node_setf(cache, "L3", 0, "%s, %d-way", GetHumanSize(data.l3_cache, kb_human_sizes, 1024), data.l3_assoc);
	if (data.l4_cache > 0)
		node_setf(cache, "L4", 0, "%s, %d-way", GetHumanSize(data.l4_cache, kb_human_sizes, 1024), data.l4_assoc);
	node_setf(node, "SSE units", 0, "%d bits (%s)",
		data.sse_size, data.detection_hints[CPU_HINT_SSE_SIZE_AUTH] ? "authoritative" : "non-authoritative");
	feature = node_append_new(node, "Features", NFLG_ATTGROUP);
	for (i = 0; i < NUM_CPU_FEATURES; i++)
	{
		node_att_set_bool(feature, cpu_feature_str(i), data.flags[i], 0);
	}

	node_setf(node, "CPU clock", 0, "%d MHz", cpu_clock_measure(200, 1));
	PrintMsr();
	return node;
}
