// SPDX-License-Identifier: Unlicense

#include <stdlib.h>
#include <string.h>
#include <intrin.h>

#include "libnw.h"
#include <libcpuid.h>
#include <libcpuid_util.h>
#include <pathcch.h>

#include "utils.h"

#include "../ryzenadj/ryzenadj.h"

#ifdef _WIN64
#define RY_DLL L"ryzenadjx64.dll"
#else
#define RY_DLL L"ryzenadj.dll"
#endif

static struct
{
	HMODULE dll;
	ryzen_access ry;
	ryzen_access (CALL *init_ryzenadj)(VOID);
	void (CALL* cleanup_ryzenadj)(ryzen_access ry);
	int (CALL* init_table)(ryzen_access ry);
	uint32_t (CALL *get_table_ver)(ryzen_access ry);
	size_t (CALL *get_table_size)(ryzen_access ry);
	float* (CALL *get_table_values)(ryzen_access ry);
	int (CALL *refresh_table)(ryzen_access ry);
	float (CALL *get_stapm_limit)(ryzen_access ry);
	float (CALL *get_fast_limit)(ryzen_access ry);
	float (CALL *get_slow_limit)(ryzen_access ry);
} m_ry;

static void
RyzenAdjCleanup(void)
{
	if (m_ry.ry)
		m_ry.cleanup_ryzenadj(m_ry.ry);
	if (m_ry.dll)
		FreeLibrary(m_ry.dll);
	ZeroMemory(&m_ry, sizeof(m_ry));
}

static bool
RyzenAdjInit(void)
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
	RyzenAdjCleanup();
	return FALSE;
}

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

static void
PrintCacheSize(PNODE node, LPCSTR name, int32_t instances, int32_t size, int32_t assoc)
{
	if (size <= 0)
		return;
	else if (instances <= 0)
		NWL_NodeAttrSetf(node, name, 0,
			"%s, %d-way",
			NWL_GetHumanSize(size, &NWLC->NwUnits[1], 1024), assoc);
	else
		NWL_NodeAttrSetf(node, name, 0,
			"%d * %s, %d-way",
			instances, NWL_GetHumanSize(size, &NWLC->NwUnits[1], 1024), assoc);
}

static void
PrintCache(PNODE node, const struct cpu_id_t* data)
{
	PNODE cache;
	BOOL saved_human_size;
	cache = NWL_NodeAppendNew(node, "Cache", NFLG_ATTGROUP);
	saved_human_size = NWLC->HumanSize;
	NWLC->HumanSize = TRUE;
	PrintCacheSize(cache, "L1 D",
		data->l1_data_instances, data->l1_data_cache, data->l1_data_assoc);
	PrintCacheSize(cache, "L1 I",
		data->l1_instruction_instances, data->l1_instruction_cache, data->l1_instruction_assoc);
	PrintCacheSize(cache, "L2",
		data->l2_instances, data->l2_cache, data->l2_assoc);
	PrintCacheSize(cache, "L3",
		data->l3_instances, data->l3_cache, data->l3_assoc);
	PrintCacheSize(cache, "L4",
		data->l4_instances, data->l4_cache, data->l4_assoc);
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
PrintCpuMsr(PNODE node, struct cpu_id_t* data)
{
	logical_cpu_t i;
	bool affinity_saved = FALSE;
	bool use_ryzenadj = FALSE;
	int value = CPU_INVALID_VALUE;
	if (!data->flags[CPU_FEATURE_MSR] || NWLC->NwDrv == NULL)
		return;
	affinity_saved = save_cpu_affinity();
	if (data->vendor == VENDOR_AMD)
		use_ryzenadj = RyzenAdjInit();
	for (i = 0; i < data->num_logical_cpus; i++)
	{
		if (!get_affinity_mask_bit(i, &data->affinity_mask))
			continue;
		if (!set_cpu_affinity(i))
			continue;
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
		value = cpu_msrinfo(NWLC->NwDrv, INFO_PKG_TEMPERATURE);
		if (value != CPU_INVALID_VALUE && value > 0)
			NWL_NodeAttrSetf(node, "Temperature (C)", NAFLG_FMT_NUMERIC, "%d", value);
		value = cpu_msrinfo(NWLC->NwDrv, INFO_VOLTAGE);
		if (value != CPU_INVALID_VALUE && value > 0)
			NWL_NodeAttrSetf(node, "Core Voltage (V)", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
		value = cpu_msrinfo(NWLC->NwDrv, INFO_BUS_CLOCK);
		if (value != CPU_INVALID_VALUE && value > 0)
			NWL_NodeAttrSetf(node, "Bus Clock (MHz)", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
		value = cpu_msrinfo(NWLC->NwDrv, INFO_PKG_POWER);
		if (value != CPU_INVALID_VALUE && value > 0)
			NWL_NodeAttrSetf(node, "Power (W)", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
		value = cpu_msrinfo(NWLC->NwDrv, INFO_PKG_PL1);
		if (value != CPU_INVALID_VALUE && value > 0)
			NWL_NodeAttrSetf(node, "PL1 (W)", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
		value = cpu_msrinfo(NWLC->NwDrv, INFO_PKG_PL2);
		if (value != CPU_INVALID_VALUE && value > 0)
			NWL_NodeAttrSetf(node, "PL2 (W)", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
		if (use_ryzenadj)
		{
			NWL_NodeAttrSetf(node, "STAPM Limit (W)", NAFLG_FMT_NUMERIC, "%.2lf", m_ry.get_stapm_limit(m_ry.ry));
			NWL_NodeAttrSetf(node, "PL1 (W)", NAFLG_FMT_NUMERIC, "%.2lf", m_ry.get_slow_limit(m_ry.ry));
			NWL_NodeAttrSetf(node, "PL2 (W)", NAFLG_FMT_NUMERIC, "%.2lf", m_ry.get_fast_limit(m_ry.ry));
		}
		break;
	}
	if (use_ryzenadj)
		RyzenAdjCleanup();
	if (affinity_saved)
		restore_cpu_affinity();
}

static void
PrintCpuInfo(PNODE node, struct cpu_id_t* data)
{
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
	PrintCpuMsr(node, data);
	PrintFeatures(node, data);
}

PNODE NW_Cpuid(VOID)
{
	uint8_t i;
	struct cpu_raw_data_array_t* raw = NWLC->NwCpuRaw;
	struct system_id_t* id = NWLC->NwCpuid;
	PNODE node = NWL_NodeAlloc("CPUID", 0);
	if (NWLC->CpuInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	cpuid_free_system_id(id);
	cpuid_free_raw_data_array(raw);
	NWL_NodeAttrSetf(node, "Total CPUs", NAFLG_FMT_NUMERIC, "%d", cpuid_get_total_cpus());
	if (cpuid_get_all_raw_data(raw) < 0)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Cannot obtain raw CPU data");
		goto fail;
	}
	if (cpu_identify_all(raw, id) < 0)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, cpuid_error());
		goto fail;
	}
	PrintHypervisor(node, &id->cpu_types[0]);
	NWL_NodeAttrSetf(node, "Processor Count", NAFLG_FMT_NUMERIC, "%u", id->num_cpu_types);
	if (id->l1_data_total_instances >= 0)
		NWL_NodeAttrSetf(node, "L1 Data Cache Instances", NAFLG_FMT_NUMERIC, "%d", id->l1_data_total_instances);
	if (id->l1_instruction_total_instances >= 0)
		NWL_NodeAttrSetf(node, "L1 Instruction Cache Instances", NAFLG_FMT_NUMERIC, "%d", id->l1_instruction_total_instances);
	if (id->l2_total_instances >= 0)
		NWL_NodeAttrSetf(node, "L2 Cache Instances", NAFLG_FMT_NUMERIC, "%d", id->l2_total_instances);
	if (id->l3_total_instances >= 0)
		NWL_NodeAttrSetf(node, "L3 Cache Instances", NAFLG_FMT_NUMERIC, "%d", id->l3_total_instances);
	if (id->l4_total_instances >= 0)
		NWL_NodeAttrSetf(node, "L4 Cache Instances", NAFLG_FMT_NUMERIC, "%d", id->l4_total_instances);
	NWL_NodeAttrSetf(node, "CPU Clock (MHz)", NAFLG_FMT_NUMERIC, "%d", cpu_clock_by_os());
	for (i = 0; i < id->num_cpu_types; i++)
	{
		CHAR name[32];
		PNODE cpu = NULL;
		snprintf(name, sizeof(name), "CPU%u", i);
		cpu = NWL_NodeAppendNew(node, name, 0);
		PrintCpuInfo(cpu, &id->cpu_types[i]);
	}
fail:
	return node;
}
