// SPDX-License-Identifier: Unlicense

#include <stdlib.h>
#include <string.h>
#include <intrin.h>

#include "libnw.h"
#include <libcpuid.h>
#include <libcpuid_util.h>
#include "cpu/rdmsr.h"

#include "utils.h"
#include "smbios.h"

static __int64
CpuCompareFileTime(const FILETIME* time1, const FILETIME* time2)
{
	__int64 a = ((__int64)time1->dwHighDateTime) << 32 | time1->dwLowDateTime;
	__int64 b = ((__int64)time2->dwHighDateTime) << 32 | time2->dwLowDateTime;
	return b - a;
}

double
NWL_GetCpuUsage(VOID)
{
	double ret = 0.0;
	static FILETIME old_idle = { 0 };
	static FILETIME old_krnl = { 0 };
	static FILETIME old_user = { 0 };
	FILETIME idle = { 0 };
	FILETIME krnl = { 0 };
	FILETIME user = { 0 };
	__int64 diff_idle, diff_krnl, diff_user, total;
	GetSystemTimes(&idle, &krnl, &user);
	diff_idle = CpuCompareFileTime(&idle, &old_idle);
	diff_krnl = CpuCompareFileTime(&krnl, &old_krnl);
	diff_user = CpuCompareFileTime(&user, &old_user);
	total = diff_krnl + diff_user;
	if (total != 0)
		ret = (100.0 * _abs64(total - diff_idle)) / _abs64(total);
	if (ret > 100.0)
		ret = 100.0;
	old_idle = idle;
	old_krnl = krnl;
	old_user = user;
	return ret;
}

static PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION
GetProcessorPerfDist(size_t* pCount)
{
	ULONG len = 0;
	NTSTATUS rc = NWL_NtQuerySystemInformation(SystemProcessorPerformanceDistribution, NULL, 0, &len);
	if (len == 0)
		return NULL;
	PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION ppd = (PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION)malloc(len);
	if (!ppd)
		return NULL;
	rc = NWL_NtQuerySystemInformation(SystemProcessorPerformanceDistribution, ppd, len, &len);
	if (!NT_SUCCESS(rc))
	{
		free(ppd);
		return NULL;
	}
	if (pCount)
		*pCount = ppd->ProcessorCount;
	return ppd;
}

DWORD
NWL_GetCpuFreq(VOID)
{
	static PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION saved_ppd = NULL;
	PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION cur_ppd = NULL;

	size_t cpu_count = 0;
	PPROCESSOR_POWER_INFORMATION ppi = NWL_NtPowerInformation(&cpu_count);
	if (cpu_count == 0 || ppi == NULL)
		goto fail;

	if (NWLC->NwOsInfo.dwMajorVersion < 10)
	{
		ULONG sum = 0;
		for (size_t i = 0; i < cpu_count; i++)
			sum += ppi[i].CurrentMhz;
		free(ppi);
		return (DWORD) (sum / cpu_count);
	}

	if (!saved_ppd)
		saved_ppd = GetProcessorPerfDist(NULL);
	if (!saved_ppd)
		goto fail;
	cur_ppd = GetProcessorPerfDist(NULL);
	if (!cur_ppd)
		goto fail;
	if (cur_ppd->ProcessorCount != saved_ppd->ProcessorCount ||
		cur_ppd->ProcessorCount < cpu_count)
		goto fail;

	ULONGLONG total_hits_delta = 0;
	ULONGLONG total_freq_contrib = 0;
	for (size_t i = 0; i < cur_ppd->ProcessorCount; i++)
	{
		PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION cur_state =
			(PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION)((BYTE*)cur_ppd + cur_ppd->Offsets[i]);
		PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION saved_state =
			(PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION)((BYTE*)saved_ppd + saved_ppd->Offsets[i]);
		if (cur_state->StateCount != saved_state->StateCount)
			continue;
		ULONG max_mhz = ppi[i].MaxMhz;
		if (max_mhz == 0)
			max_mhz = ppi[0].MaxMhz;
		for (ULONG j = 0; j < cur_state->StateCount; j++)
		{
			ULONGLONG hits_delta = cur_state->States[j].Hits - saved_state->States[j].Hits;
			total_hits_delta += hits_delta;
			total_freq_contrib += hits_delta * cur_state->States[j].PercentFrequency * max_mhz;
		}
	}

	free(ppi);
	free(saved_ppd);
	saved_ppd = cur_ppd;

	if (total_hits_delta == 0)
		return 0;
	return (DWORD)(total_freq_contrib / total_hits_delta / 100);

fail:
	if (saved_ppd)
	{
		free(saved_ppd);
		saved_ppd = NULL;
	}
	if (cur_ppd)
		free(cur_ppd);

	DWORD reg_freq = 0;
	NWL_GetRegDwordValue(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"~MHz", &reg_freq);
	return reg_freq;
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
	case VENDOR_VIRTCPU: return "Microsoft";
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
PrintCacheSize(PNODE node, LPCSTR name, int32_t instances, int32_t size, int32_t assoc, UINT64* sum)
{
	if (size <= 0)
		return;
	else if (instances <= 0)
	{
		NWL_NodeAttrSetf(node, name, 0,
			"%s, %d-way",
			NWL_GetHumanSize(size, &NWLC->NwUnits[1], 1024), assoc);
		*sum += 1024ULL * size;
	}
	else
	{
		NWL_NodeAttrSetf(node, name, 0,
			"%d * %s, %d-way",
			instances, NWL_GetHumanSize(size, &NWLC->NwUnits[1], 1024), assoc);
		*sum += 1024ULL * size * instances;
	}
}

static void
PrintCache(PNODE node, const struct cpu_id_t* data)
{
	PNODE cache;
	BOOL saved_human_size;
	UINT64 l1 = 0, l2 = 0, l3 = 0, l4 = 0;
	cache = NWL_NodeAppendNew(node, "Cache", NFLG_ATTGROUP);
	saved_human_size = NWLC->HumanSize;
	NWLC->HumanSize = TRUE;
	PrintCacheSize(cache, "L1 D",
		data->l1_data_instances, data->l1_data_cache, data->l1_data_assoc, &l1);
	PrintCacheSize(cache, "L1 I",
		data->l1_instruction_instances, data->l1_instruction_cache, data->l1_instruction_assoc, &l1);
	PrintCacheSize(cache, "L2",
		data->l2_instances, data->l2_cache, data->l2_assoc, &l2);
	PrintCacheSize(cache, "L3",
		data->l3_instances, data->l3_cache, data->l3_assoc, &l3);
	PrintCacheSize(cache, "L4",
		data->l4_instances, data->l4_cache, data->l4_assoc, &l4);
	NWLC->HumanSize = saved_human_size;

	if (l1 > 0)
		NWL_NodeAttrSet(cache, "L1 Cache Size", NWL_GetHumanSize(l1, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
	if (l2 > 0)
		NWL_NodeAttrSet(cache, "L2 Cache Size", NWL_GetHumanSize(l2, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
	if (l3 > 0)
		NWL_NodeAttrSet(cache, "L3 Cache Size", NWL_GetHumanSize(l3, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
	if (l4 > 0)
		NWL_NodeAttrSet(cache, "L4 Cache Size", NWL_GetHumanSize(l4, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
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

NWLIB_CPU_INFO*
NWL_GetCpuMsr(VOID)
{
	NWLIB_CPU_INFO* info = NULL;
	if (!NWLC->NwCpuid || NWLC->NwCpuid->num_cpu_types <= 0)
		return NULL;
	info = calloc(NWLC->NwCpuid->num_cpu_types, sizeof(NWLIB_CPU_INFO));
	if (info == NULL)
		return NULL;
	for (uint8_t i = 0; i < NWLC->NwCpuid->num_cpu_types; i++)
	{
		struct msr_info_t* msr = &NWLC->NwMsr[i];
		NWLIB_CPU_INFO* p = &info[i];
		double multiplier = NWL_MsrGet(msr, INFO_CUR_MULTIPLIER) / 100.0;
		snprintf(p->MsrMulti, NWL_STR_SIZE, "%.1lf (%d - %d)",
			multiplier,
			NWL_MsrGet(msr, INFO_MIN_MULTIPLIER) / 100,
			NWL_MsrGet(msr, INFO_MAX_MULTIPLIER) / 100);
		p->MsrTemp = NWL_MsrGet(msr, INFO_PKG_TEMPERATURE);
		if (p->MsrTemp <= 0)
			p->MsrTemp = NWL_MsrGet(msr, INFO_TEMPERATURE);
		p->MsrVolt = NWL_MsrGet(msr, INFO_VOLTAGE) / 100.0;
		p->MsrPower = NWL_MsrGet(msr, INFO_PKG_POWER) / 100.0;
		p->MsrPl1 = NWL_MsrGet(msr, INFO_PKG_PL1) / 100.0;
		p->MsrPl2 = NWL_MsrGet(msr, INFO_PKG_PL2) / 100.0;
		p->MsrBus = NWL_MsrGet(msr, INFO_BUS_CLOCK) / 100.0;
		p->BiosRev = (UINT32)NWL_MsrGet(msr, INFO_MICROCODE_VER);
		p->MsrFreq = multiplier * p->MsrBus;
	}
	return info;
}

static void
PrintCpuMsr(PNODE node, uint8_t index)
{
	if (NWLC->NwMsr == NULL)
		return;
	struct msr_info_t* msr = &NWLC->NwMsr[index];

	NWL_NodeAttrSetf(node, "Multiplier", 0, "%.1lf (%d - %d)",
		NWL_MsrGet(msr, INFO_CUR_MULTIPLIER) / 100.0,
		NWL_MsrGet(msr, INFO_MIN_MULTIPLIER) / 100,
		NWL_MsrGet(msr, INFO_MAX_MULTIPLIER) / 100);
	NWL_NodeAttrSetf(node, "Temperature (C)", NAFLG_FMT_NUMERIC, "%d", NWL_MsrGet(msr, INFO_PKG_TEMPERATURE));
	NWL_NodeAttrSetf(node, "Core Voltage (V)", NAFLG_FMT_NUMERIC, "%.2lf", NWL_MsrGet(msr, INFO_VOLTAGE) / 100.0);
	NWL_NodeAttrSetf(node, "Bus Clock (MHz)", NAFLG_FMT_NUMERIC, "%.2lf", NWL_MsrGet(msr, INFO_BUS_CLOCK) / 100.0);
	NWL_NodeAttrSetf(node, "PL1 (W)", NAFLG_FMT_NUMERIC, "%.2lf", NWL_MsrGet(msr, INFO_PKG_PL1) / 100.0);
	NWL_NodeAttrSetf(node, "PL2 (W)", NAFLG_FMT_NUMERIC, "%.2lf", NWL_MsrGet(msr, INFO_PKG_PL2) / 100.0);
	NWL_NodeAttrSetf(node, "Microcode Rev", 0, "0x%X", (UINT32)NWL_MsrGet(msr, INFO_MICROCODE_VER));
}

// Get DMI Processor Information (Type 4) Table
static void
PrintCpuDmi(PNODE node, const char* name)
{
	LPBYTE p = (LPBYTE)NWLC->NwSmbios->Data;
	const LPBYTE lastAddress = p + NWLC->NwSmbios->Length;
	PProcessorInfo pInfo;

	while ((pInfo = (PProcessorInfo)NWL_GetNextDmiTable(&p, lastAddress, 4)) != NULL)
	{
		if (strcmp(name, NWL_GetDmiString((UINT8*)pInfo, pInfo->Version)) != 0)
			continue;
		NWL_NodeAttrSetf(node, "Base Clock (MHz)", NAFLG_FMT_NUMERIC, "%u", pInfo->CurrentSpeed);
		NWL_NodeAttrSet(node, "Socket Designation", NWL_GetDmiString((UINT8*)pInfo, pInfo->SocketDesignation), 0);
		NWL_NodeAttrSet(node, "Socket Type", NWL_GetDmiProcessorSocket(pInfo), 0);
	}
}

static void
PrintCpuInfo(PNODE node, struct cpu_id_t* data, uint8_t index)
{
	NWL_NodeAttrSet(node, "Purpose", cpu_purpose_str(data->purpose), 0);
	NWL_NodeAttrSet(node, "Vendor", data->vendor_str, 0);
	NWL_NodeAttrSet(node, "Vendor Name", CpuVendorToStr(data->vendor), 0);
	NWL_NodeAttrSet(node, "Brand", data->brand_str, 0);
	NWL_NodeAttrSet(node, "Code Name", data->cpu_codename, 0);
	NWL_NodeAttrSet(node, "Technology", data->technology_node, 0);
	NWL_NodeAttrSetf(node, "Family", 0, "%02Xh", data->x86.family);
	NWL_NodeAttrSetf(node, "Model", 0, "%02Xh", data->x86.model);
	NWL_NodeAttrSetf(node, "Stepping", 0, "%02Xh", data->x86.stepping);
	NWL_NodeAttrSetf(node, "Ext.Family", 0, "%02Xh", data->x86.ext_family);
	NWL_NodeAttrSetf(node, "Ext.Model", 0, "%02Xh", data->x86.ext_model);
	NWL_NodeAttrSetf(node, "Package Type", 0, "%Xh", data->x86.pkg_type);

	NWL_NodeAttrSetf(node, "Cores", NAFLG_FMT_NUMERIC, "%d", data->num_cores);
	NWL_NodeAttrSetf(node, "Logical CPUs", NAFLG_FMT_NUMERIC, "%d", data->num_logical_cpus);
	NWL_NodeAttrSet(node, "Affinity Mask", affinity_mask_str(&data->affinity_mask), 0);
	NWL_NodeAttrSetf(node, "SSE Units", 0, "%d bits (%s)",
		data->x86.sse_size, data->detection_hints[CPU_HINT_SSE_SIZE_AUTH] ? "authoritative" : "non-authoritative");
	PrintCache(node, data);
	PrintFeatures(node, data);
	if (!NWLC->CpuDump)
	{
		PrintCpuDmi(node, data->brand_str);
		PrintCpuMsr(node, index);
	}
}

static inline void
PrintAllCpuInfo(PNODE node, struct system_id_t* id)
{
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
	for (uint8_t i = 0; i < id->num_cpu_types; i++)
	{
		CHAR name[32];
		PNODE cpu = NULL;
		NWL_GetCpuIndexStr(&id->cpu_types[i], name, ARRAYSIZE(name));
		cpu = NWL_NodeAppendNew(node, name, 0);
		PrintCpuInfo(cpu, &id->cpu_types[i], i);
	}
}

static inline void
PrintCpuidDump(PNODE node)
{
	struct cpu_raw_data_array_t* raw = calloc(1, sizeof(struct cpu_raw_data_array_t));
	struct system_id_t* id = calloc(1, sizeof(struct system_id_t));
	if (!raw || !id)
		goto out;

	if (cpuid_deserialize_all_raw_data(raw, NWLC->CpuDump) != 0)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Cannot deserialize raw CPU data");
		goto out;
	}

	if (cpu_identify_all(raw, id) != 0)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, cpuid_error());
		goto out;
	}

	PrintAllCpuInfo(node, id);

out:
	if (id)
	{
		cpuid_free_system_id(id);
		free(id);
	}
	if (raw)
	{
		cpuid_free_raw_data_array(raw);
		free(raw);
	}
}

static inline void
PrintCpuidMachine(PNODE node)
{
	struct cpu_raw_data_array_t* raw = NWLC->NwCpuRaw;
	struct system_id_t* id = NWLC->NwCpuid;

	NWL_NodeAttrSetf(node, "Total CPUs", NAFLG_FMT_NUMERIC, "%d", cpuid_get_total_cpus());
	NWL_NodeAttrSetf(node, "CPU Clock (MHz)", NAFLG_FMT_NUMERIC, "%lu", NWL_GetCpuFreq());

	if (raw->num_raw <= 0)
	{
		// no cached raw data
		if (cpuid_get_all_raw_data(raw) != 0)
		{
			NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Cannot obtain raw CPU data");
			return;
		}
	}

	if (id->num_cpu_types <= 0)
	{
		// no cached id data
		if (cpu_identify_all(raw, id) != 0)
		{
			NWL_NodeAppendMultiSz(&NWLC->ErrLog, cpuid_error());
			return;
		}
	}

	if (NWLC->NwMsr == NULL && id->num_cpu_types > 0)
	{
		NWLC->NwMsr = calloc(id->num_cpu_types, sizeof(struct msr_info_t));
		if (NWLC->NwMsr)
		{
			for (uint8_t i = 0; i < id->num_cpu_types; i++)
				NWL_MsrInit(&NWLC->NwMsr[i], NWLC->NwDrv, &id->cpu_types[i]);
		}
	}

	PrintAllCpuInfo(node, id);
}

PNODE NW_Cpuid(BOOL bAppend)
{
	PNODE node = NWL_NodeAlloc("CPUID", 0);
	if (bAppend)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	if (NWLC->CpuDump)
		PrintCpuidDump(node);
	else
		PrintCpuidMachine(node);

	return node;
}
