// SPDX-License-Identifier: Unlicense

#include <stdlib.h>
#include <string.h>
#include <intrin.h>

#include "libnw.h"
#include <libcpuid.h>
#include "utils.h"

static const char* kb_human_sizes[6] =
{ "KB", "MB", "GB", "TB", "PB", "EB", };

static void
PrintHypervisor(PNODE node)
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
		NWL_NodeAttrSet(node, "Hypervisor", "VMware", 0);
	else if (strcmp(VmSign, "Microsoft Hv") == 0)
		NWL_NodeAttrSet(node, "Hypervisor", "Microsoft Hyper-V", 0);
	else if (strcmp(VmSign, "KVMKVMKVM") == 0)
		NWL_NodeAttrSet(node, "Hypervisor", "KVM", 0);
	else if (strcmp(VmSign, "VBoxVBoxVBox") == 0)
		NWL_NodeAttrSet(node, "Hypervisor", "VirtualBox", 0);
	else if (strcmp(VmSign, "XenVMMXenVMM") == 0)
		NWL_NodeAttrSet(node, "Hypervisor", "Xen", 0);
	else if (strcmp(VmSign, "prl hyperv") == 0)
		NWL_NodeAttrSet(node, "Hypervisor", "Parallels", 0);
	else if (strcmp(VmSign, "TCGTCGTCGTCG") == 0)
		NWL_NodeAttrSet(node, "Hypervisor", "QEMU", 0);
	else if (strcmp(VmSign, "bhyve bhyve") == 0)
		NWL_NodeAttrSet(node, "Hypervisor", "FreeBSD bhyve", 0);
	else
		NWL_NodeAttrSet(node, "Hypervisor", VmSign, 0);
}

static void
PrintSgx(PNODE node, const struct cpu_raw_data_t* raw, const struct cpu_id_t* data)
{
	int i;
	PNODE nsgx, nepc;
	if (!data->sgx.present)
		return;
	nsgx = NWL_NodeAppendNew(node, "SGX", NFLG_ATTGROUP);

	NWL_NodeAttrSetf(nsgx, "Max Enclave Size (32-bit)", 0, "2^%d", data->sgx.max_enclave_32bit);
	NWL_NodeAttrSetf(nsgx, "Max Enclave Size (64-bit)", 0, "2^%d", data->sgx.max_enclave_64bit);
	NWL_NodeAttrSetBool(nsgx, "SGX1 Extensions", data->sgx.flags[INTEL_SGX1], 0);
	NWL_NodeAttrSetBool(nsgx, "SGX2 Extensions", data->sgx.flags[INTEL_SGX2], 0);
	NWL_NodeAttrSetf(nsgx, "MISCSELECT", 0, "%08x", data->sgx.misc_select);
	NWL_NodeAttrSetf(nsgx, "SECS.ATTRIBUTES Mask", 0, "%016llx", (unsigned long long) data->sgx.secs_attributes);
	NWL_NodeAttrSetf(nsgx, "SECS.XSAVE Feature Mask", 0, "%016llx", (unsigned long long) data->sgx.secs_xfrm);
	nepc = NWL_NodeAppendNew(nsgx, "EPC Sections", NFLG_TABLE);
	for (i = 0; i < data->sgx.num_epc_sections; i++)
	{
		struct cpu_epc_t epc = cpuid_get_epc(i, raw);
		PNODE p = NWL_NodeAppendNew(nepc, "Section", NFLG_TABLE_ROW);
		NWL_NodeAttrSetf(p, "Start", 0, "0x%llx", (unsigned long long) epc.start_addr);
		NWL_NodeAttrSetf(p, "Size", 0, "0x%llx", (unsigned long long) epc.length);
	}
}

struct cpu_id_t* get_cached_cpuid(void);

static int rdmsr_supported(void)
{
	struct cpu_id_t* id = get_cached_cpuid();
	return id->flags[CPU_FEATURE_MSR];
}

static void
PrintMsr(PNODE node)
{
	int value = CPU_INVALID_VALUE;
	if (!rdmsr_supported())
	{
		fprintf(stderr, "rdmsr not supported\n");
		return;
	}
	if (NWLC->NwDrv == NULL)
	{
		fprintf(stderr, "Cannot load driver!\n");
		return;
	}
	int min_multi = cpu_msrinfo(NWLC->NwDrv, INFO_MIN_MULTIPLIER);
	int max_multi = cpu_msrinfo(NWLC->NwDrv, INFO_MAX_MULTIPLIER);
	int cur_multi = cpu_msrinfo(NWLC->NwDrv, INFO_CUR_MULTIPLIER);
	if (min_multi == CPU_INVALID_VALUE)
		min_multi = 0;
	if (max_multi == CPU_INVALID_VALUE)
		max_multi = 0;
	if (cur_multi == CPU_INVALID_VALUE)
		cur_multi = 0;
	PNODE nmulti = NWL_NodeAppendNew(node, "Multiplier", NFLG_ATTGROUP);
	NWL_NodeAttrSetf(nmulti, "Current", NAFLG_FMT_NUMERIC, "%.1lf", cur_multi / 100.0);
	NWL_NodeAttrSetf(nmulti, "Max", NAFLG_FMT_NUMERIC, "%d", max_multi / 100);
	NWL_NodeAttrSetf(nmulti, "Min", NAFLG_FMT_NUMERIC, "%d", min_multi / 100);
	if ((value = cpu_msrinfo(NWLC->NwDrv, INFO_TEMPERATURE)) != CPU_INVALID_VALUE)
		NWL_NodeAttrSetf(node, "Temperature (C)", NAFLG_FMT_NUMERIC, "%d", value);
	if ((value = cpu_msrinfo(NWLC->NwDrv, INFO_THROTTLING)) != CPU_INVALID_VALUE)
		NWL_NodeAttrSetBool(node, "Throttling", value, 0);
	if ((value = cpu_msrinfo(NWLC->NwDrv, INFO_VOLTAGE)) != CPU_INVALID_VALUE)
		NWL_NodeAttrSetf(node, "Core Voltage (V)", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
	if ((value = cpu_msrinfo(NWLC->NwDrv, INFO_BUS_CLOCK)) != CPU_INVALID_VALUE)
		NWL_NodeAttrSetf(node, "Bus Clock (MHz)", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
}

PNODE NW_Cpuid(VOID)
{
	struct cpu_raw_data_t raw = { 0 };
	struct cpu_id_t data = { 0 };
	int i = 0;
	PNODE cache, feature;
	BOOL saved_human_size;
	PNODE node = NWL_NodeAlloc("CPUID", 0);
	if (NWLC->CpuInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	if (cpuid_get_raw_data(&raw) < 0)
	{
		fprintf(stderr, "Cannot obtain raw CPU data!\n");
		return node;
	}

	if (cpu_identify(&raw, &data) < 0)
		fprintf(stderr, "Error identifying the CPU: %s\n", cpuid_error());
	PrintHypervisor(node);
	NWL_NodeAttrSet(node, "Vendor", data.vendor_str, 0);
	NWL_NodeAttrSet(node, "Brand", data.brand_str, 0);
	NWL_NodeAttrSet(node, "Code Name", data.cpu_codename, 0);
	NWL_NodeAttrSetf(node, "Family", 0, "%02Xh", data.family);
	NWL_NodeAttrSetf(node, "Model", 0, "%02Xh", data.model);
	NWL_NodeAttrSetf(node, "Stepping", 0, "%02Xh", data.stepping);
	NWL_NodeAttrSetf(node, "Ext.Family", 0, "%02Xh", data.ext_family);
	NWL_NodeAttrSetf(node, "Ext.Model", 0, "%02Xh", data.ext_model);

	NWL_NodeAttrSetf(node, "Cores", NAFLG_FMT_NUMERIC, "%d", data.num_cores);
	NWL_NodeAttrSetf(node, "Logical CPUs", NAFLG_FMT_NUMERIC, "%d", data.num_logical_cpus);
	NWL_NodeAttrSetf(node, "Total CPUs", NAFLG_FMT_NUMERIC, "%d", cpuid_get_total_cpus());
	cache = NWL_NodeAppendNew(node, "Cache", NFLG_ATTGROUP);
	saved_human_size = NWLC->HumanSize;
	NWLC->HumanSize = TRUE;
	if (data.l1_data_cache > 0)
		NWL_NodeAttrSetf(cache, "L1 D", 0, "%d * %s, %d-way",
			data.num_cores, NWL_GetHumanSize(data.l1_data_cache, kb_human_sizes, 1024), data.l1_data_assoc);
	if (data.l1_instruction_cache > 0)
		NWL_NodeAttrSetf(cache, "L1 I", 0, "%d * %s, %d-way",
			data.num_cores, NWL_GetHumanSize(data.l1_instruction_cache, kb_human_sizes, 1024), data.l1_instruction_assoc);
	if (data.l2_cache > 0)
		NWL_NodeAttrSetf(cache, "L2", 0, "%d * %s, %d-way",
			data.num_cores, NWL_GetHumanSize(data.l2_cache, kb_human_sizes, 1024), data.l2_assoc);
	if (data.l3_cache > 0)
		NWL_NodeAttrSetf(cache, "L3", 0, "%s, %d-way", NWL_GetHumanSize(data.l3_cache, kb_human_sizes, 1024), data.l3_assoc);
	if (data.l4_cache > 0)
		NWL_NodeAttrSetf(cache, "L4", 0, "%s, %d-way", NWL_GetHumanSize(data.l4_cache, kb_human_sizes, 1024), data.l4_assoc);
	NWLC->HumanSize = saved_human_size;
	NWL_NodeAttrSetf(node, "SSE Units", 0, "%d bits (%s)",
		data.sse_size, data.detection_hints[CPU_HINT_SSE_SIZE_AUTH] ? "authoritative" : "non-authoritative");
	feature = NWL_NodeAppendNew(node, "Features", NFLG_ATTGROUP);
	for (i = 0; i < NUM_CPU_FEATURES; i++)
	{
		NWL_NodeAttrSetBool(feature, cpu_feature_str(i), data.flags[i], 0);
	}

	NWL_NodeAttrSetf(node, "CPU Clock (MHz)", NAFLG_FMT_NUMERIC, "%d", cpu_clock_measure(200, 1));
	PrintSgx(node, &raw, &data);
	PrintMsr(node);
	return node;
}
