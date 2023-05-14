// SPDX-License-Identifier: Unlicense

#include <stdlib.h>
#include <string.h>
#include <intrin.h>

#include "libnw.h"
#include <libcpuid.h>
#include <libcpuid_util.h>
#include "utils.h"

static const char* kb_human_sizes[6] =
{ "KB", "MB", "GB", "TB", "PB", "EB", };

static LPCSTR
CpuVendorToStr(cpu_vendor_t vendor)
{
	switch (vendor)
	{
	case VENDOR_INTEL: return "Intel";
	case VENDOR_AMD: return "AMD";
	case VENDOR_CYRIX: return "Cyrix";
	case VENDOR_NEXGEN: return "NexGen";
	case VENDOR_TRANSMETA: return "Transmeta";
	case VENDOR_UMC: return "UMC";
	case VENDOR_CENTAUR: return "IDT/Centaur";
	case VENDOR_RISE: return "Rise Technology";
	case VENDOR_SIS: return "SiS";
	case VENDOR_NSC: return "National Semiconductor";
	case VENDOR_HYGON: return "Hygon";
	case VENDOR_VORTEX86: return "DM&P Vortex86";
	case VENDOR_VIA: return "VIA";
	case VENDOR_ZHAOXIN: return "Zhaoxin";
	default: return "Unknown";
	}
}

static LPCSTR
CacheNumToStr(int32_t num)
{
	static char str[12] = "?";
	if (num > 0)
		snprintf(str, sizeof(str), "%d", num);
	return str;
}

static void
PrintHypervisor(PNODE node, const struct cpu_id_t* data)
{
	LPCSTR name = NULL;
	switch (data->hypervisor_vendor)
	{
	case HYPERVISOR_NONE: return;
	case HYPERVISOR_ACRN: name = "Project ACRN"; break;
	case HYPERVISOR_BHYVE: name = "FreeBSD bhyve"; break;
	case HYPERVISOR_HYPERV: name = "Microsoft Hyper-V"; break;
	case HYPERVISOR_KVM: name = "KVM"; break;
	case HYPERVISOR_PARALLELS: name = "Parallels"; break;
	case HYPERVISOR_QEMU: name = "QEMU"; break;
	case HYPERVISOR_QNX: name = "QNX Hypervisor"; break;
	case HYPERVISOR_VIRTUALBOX: name = "VirtualBox"; break;
	case HYPERVISOR_VMWARE: name = "VMware"; break;
	case HYPERVISOR_XEN: name = "Xen"; break;
	default: name = "Unknown";
	}
	NWL_NodeAttrSet(node, "Hypervisor", name, 0);
	NWL_NodeAttrSet(node, "Hypervisor Signature", data->hypervisor_str, 0);
}
#ifdef ENABLE_SGX
static void
PrintSgx(PNODE node, const struct cpu_id_t* data, struct cpu_raw_data_array_t* raw, logical_cpu_t core)
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
	if (core == 0xffff)
		return;
	nepc = NWL_NodeAppendNew(nsgx, "EPC Sections", NFLG_TABLE);
	for (i = 0; i < data->sgx.num_epc_sections; i++)
	{
		struct cpu_epc_t epc = cpuid_get_epc(i, &raw->raw[core]);
		PNODE p = NWL_NodeAppendNew(nepc, "Section", NFLG_TABLE_ROW);
		NWL_NodeAttrSetf(p, "Start", 0, "0x%llx", (unsigned long long) epc.start_addr);
		NWL_NodeAttrSetf(p, "Size", 0, "0x%llx", (unsigned long long) epc.length);
	}
}
#endif
static void
PrintCache(PNODE node, const struct cpu_id_t* data)
{
	PNODE cache;
	BOOL saved_human_size;
	cache = NWL_NodeAppendNew(node, "Cache", NFLG_ATTGROUP);
	saved_human_size = NWLC->HumanSize;
	NWLC->HumanSize = TRUE;
	if (data->l1_data_cache > 0)
		NWL_NodeAttrSetf(cache, "L1 D", 0, "%s * %s, %d-way",
			CacheNumToStr(data->l1_data_instances),
			NWL_GetHumanSize(data->l1_data_cache, kb_human_sizes, 1024), data->l1_data_assoc);
	if (data->l1_instruction_cache > 0)
		NWL_NodeAttrSetf(cache, "L1 I", 0, "%s * %s, %d-way",
			CacheNumToStr(data->l1_instruction_instances),
			NWL_GetHumanSize(data->l1_instruction_cache, kb_human_sizes, 1024), data->l1_instruction_assoc);
	if (data->l2_cache > 0)
		NWL_NodeAttrSetf(cache, "L2", 0, "%s * %s, %d-way",
			CacheNumToStr(data->l2_instances),
			NWL_GetHumanSize(data->l2_cache, kb_human_sizes, 1024), data->l2_assoc);
	if (data->l3_cache > 0)
		NWL_NodeAttrSetf(cache, "L3", 0, "%s * %s, %d-way",
			CacheNumToStr(data->l3_instances),
			NWL_GetHumanSize(data->l3_cache, kb_human_sizes, 1024), data->l3_assoc);
	if (data->l4_cache > 0)
		NWL_NodeAttrSetf(cache, "L4", 0, "%s * %s, %d-way",
			CacheNumToStr(data->l4_instances),
			NWL_GetHumanSize(data->l4_cache, kb_human_sizes, 1024), data->l4_assoc);
	NWLC->HumanSize = saved_human_size;
}

static void
PrintFeatures(PNODE node, const struct cpu_id_t* data)
{
	int i = 0;
	char* str = NULL;
	for (i = 0; i < NUM_CPU_FEATURES; i++)
	{
		if (data->flags[i])
			NWL_NodeAppendMultiSz(&str, cpu_feature_str(i));
	}
	NWL_NodeAttrSetMulti(node, "Features", str, 0);
	free(str);
}

static void
PrintCoreMsr(PNODE node, const struct cpu_id_t* data, logical_cpu_t core)
{
	int value = CPU_INVALID_VALUE;
	if (!data->flags[CPU_FEATURE_MSR] || NWLC->NwDrv == NULL || !set_cpu_affinity(core))
		return;
	int min_multi = cpu_msrinfo(NWLC->NwDrv, INFO_MIN_MULTIPLIER);
	int max_multi = cpu_msrinfo(NWLC->NwDrv, INFO_MAX_MULTIPLIER);
	int cur_multi = cpu_msrinfo(NWLC->NwDrv, INFO_CUR_MULTIPLIER);
	if (min_multi == CPU_INVALID_VALUE)
		min_multi = 0;
	if (max_multi == CPU_INVALID_VALUE)
		max_multi = 0;
	if (cur_multi == CPU_INVALID_VALUE)
		cur_multi = 0;
	NWL_NodeAttrSetf(node, "Multiplier", 0, "%.1lf (%d - %d)",
		cur_multi / 100.0, min_multi / 100, max_multi / 100);
	value = cpu_msrinfo(NWLC->NwDrv, INFO_TEMPERATURE);
	if (value != CPU_INVALID_VALUE && value > 0)
		NWL_NodeAttrSetf(node, "Temperature (C)", NAFLG_FMT_NUMERIC, "%d", value);
	value = cpu_msrinfo(NWLC->NwDrv, INFO_THROTTLING);
	if (value != CPU_INVALID_VALUE && value > 0)
		NWL_NodeAttrSetBool(node, "Throttling", value, 0);
	value = cpu_msrinfo(NWLC->NwDrv, INFO_VOLTAGE);
	if (value != CPU_INVALID_VALUE && value > 0)
		NWL_NodeAttrSetf(node, "Core Voltage (V)", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
	value = cpu_msrinfo(NWLC->NwDrv, INFO_BUS_CLOCK);
	if (value != CPU_INVALID_VALUE && value > 0)
		NWL_NodeAttrSetf(node, "Bus Clock (MHz)", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
}

static logical_cpu_t
PrintCpuMsr(PNODE node, struct cpu_id_t* data, struct cpu_raw_data_array_t* raw)
{
	logical_cpu_t i, first_core = 0xffff;
	CHAR name[] = "CORE65536";
	bool affinity_saved = FALSE;
	affinity_saved = save_cpu_affinity();
	for (i = 0; i < raw->num_raw; i++)
	{
		PNODE core = NULL;
		if (!get_affinity_mask_bit(i, &data->affinity_mask))
			continue;
		if (first_core == 0xffff)
			first_core = i;
		snprintf(name, sizeof(name), "CORE%u", i);
		core = NWL_NodeGetChild(node, name);
		if (core == NULL)
			core = NWL_NodeAppendNew(node, name, 0);
		PrintCoreMsr(core, data, i);
	}
	if (affinity_saved)
		restore_cpu_affinity();
	return first_core;
}

static void
PrintCpuInfo(PNODE node, struct cpu_id_t* data, struct cpu_raw_data_array_t* raw)
{
	logical_cpu_t first_core;
	NWL_NodeAttrSet(node, "Purpose", cpu_purpose_str(data->purpose), 0);
	NWL_NodeAttrSet(node, "Vendor", data->vendor_str, 0);
	NWL_NodeAttrSet(node, "Vendor Name", CpuVendorToStr(data->vendor), 0);
	NWL_NodeAttrSet(node, "Brand", data->brand_str, 0);
	NWL_NodeAttrSet(node, "Code Name", data->cpu_codename, 0);
	NWL_NodeAttrSetf(node, "Family", 0, "%02Xh", data->family);
	NWL_NodeAttrSetf(node, "Model", 0, "%02Xh", data->model);
	NWL_NodeAttrSetf(node, "Stepping", 0, "%02Xh", data->stepping);
	NWL_NodeAttrSetf(node, "Ext.Family", 0, "%02Xh", data->ext_family);
	NWL_NodeAttrSetf(node, "Ext.Model", 0, "%02Xh", data->ext_model);

	NWL_NodeAttrSetf(node, "Cores", NAFLG_FMT_NUMERIC, "%d", data->num_cores);
	NWL_NodeAttrSetf(node, "Logical CPUs", NAFLG_FMT_NUMERIC, "%d", data->num_logical_cpus);
	NWL_NodeAttrSet(node, "Affinity Mask", affinity_mask_str(&data->affinity_mask), 0);
	NWL_NodeAttrSetf(node, "SSE Units", 0, "%d bits (%s)",
		data->sse_size, data->detection_hints[CPU_HINT_SSE_SIZE_AUTH] ? "authoritative" : "non-authoritative");
	PrintCache(node, data);
	first_core = PrintCpuMsr(node, data, raw);
	PrintFeatures(node, data);
#ifdef ENABLE_SGX
	PrintSgx(node, data, raw, first_core);
#endif
}

PNODE NW_UpdateCpuid(PNODE node)
{
	uint8_t i;
	struct cpu_raw_data_array_t raw = { 0 };
	struct system_id_t id = { 0 };
	if (cpuid_get_all_raw_data(&raw) < 0)
		goto fail;
	if (cpu_identify_all(&raw, &id) < 0)
		goto fail;
	for (i = 0; i < id.num_cpu_types; i++)
	{
		CHAR name[32];
		PNODE cpu = NULL;
		snprintf(name, sizeof(name), "CPU%u", i);
		cpu = NWL_NodeGetChild(node, name);
		PrintCpuMsr(cpu, &id.cpu_types[i], &raw);
	}
fail:
	cpuid_free_system_id(&id);
	cpuid_free_raw_data_array(&raw);
	return node;
}

PNODE NW_Cpuid(VOID)
{
	uint8_t i;
	struct cpu_raw_data_array_t raw = { 0 };
	struct system_id_t id = { 0 };
	
	PNODE node = NWL_NodeAlloc("CPUID", 0);
	if (NWLC->CpuInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	NWL_NodeAttrSetf(node, "Total CPUs", NAFLG_FMT_NUMERIC, "%d", cpuid_get_total_cpus());
	if (cpuid_get_all_raw_data(&raw) < 0)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Cannot obtain raw CPU data");
		goto fail;
	}

	if (cpu_identify_all(&raw, &id) < 0)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, cpuid_error());
		goto fail;
	}
	PrintHypervisor(node, &id.cpu_types[0]);
	NWL_NodeAttrSetf(node, "Processor Count", NAFLG_FMT_NUMERIC, "%u", id.num_cpu_types);
	if (id.l1_data_total_instances >= 0)
		NWL_NodeAttrSetf(node, "L1 Data Cache Instances", NAFLG_FMT_NUMERIC, "%d", id.l1_data_total_instances);
	if (id.l1_instruction_total_instances >= 0)
		NWL_NodeAttrSetf(node, "L1 Instruction Cache Instances", NAFLG_FMT_NUMERIC, "%d", id.l1_instruction_total_instances);
	if (id.l2_total_instances >= 0)
		NWL_NodeAttrSetf(node, "L2 Cache Instances", NAFLG_FMT_NUMERIC, "%d", id.l2_total_instances);
	if (id.l3_total_instances >= 0)
		NWL_NodeAttrSetf(node, "L3 Cache Instances", NAFLG_FMT_NUMERIC, "%d", id.l3_total_instances);
	if (id.l4_total_instances >= 0)
		NWL_NodeAttrSetf(node, "L4 Cache Instances", NAFLG_FMT_NUMERIC, "%d", id.l4_total_instances);
	NWL_NodeAttrSetf(node, "CPU Clock (MHz)", NAFLG_FMT_NUMERIC, "%d", cpu_clock());
	for (i = 0; i < id.num_cpu_types; i++)
	{
		CHAR name[32];
		PNODE cpu = NULL;
		snprintf(name, sizeof(name), "CPU%u", i);
		cpu = NWL_NodeAppendNew(node, name, 0);
		PrintCpuInfo(cpu, &id.cpu_types[i], &raw);
	}
fail:
	cpuid_free_system_id(&id);
	cpuid_free_raw_data_array(&raw);
	return node;
}
