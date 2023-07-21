// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

#ifdef USE_PDH
#pragma comment(lib, "pdh.lib")
#endif

#include "../libcpuid/libcpuid.h"
#include "../libcpuid/libcpuid_util.h"

GNW_CONTEXT g_ctx;

static struct nk_image
load_png(WORD id)
{
	HRSRC res = FindResourceW(NULL, MAKEINTRESOURCEW(id), RT_RCDATA);
	if (!res)
		goto fail;
	HGLOBAL mem = LoadResource(NULL, res);
	if (!mem)
		goto fail;
	DWORD size = SizeofResource(NULL, res);
	if (!size)
		goto fail;
	void* data = LockResource(mem);
	if (!data)
		goto fail;
	return nk_gdip_load_image_from_memory(data, size);
fail:
	return nk_image_id(0);
}

static void
gnwinfo_ctx_error_callback(LPCSTR lpszText)
{
	MessageBoxA(g_ctx.wnd, lpszText, "Error", MB_ICONERROR);
}

LPCSTR NWL_GetHumanSize(UINT64 size, LPCSTR human_sizes[6], UINT64 base);
static const char* human_sizes[6] =
{ "B", "K", "M", "G", "T", "P", };

#ifndef USE_PDH
__int64 compare_file_time(const FILETIME* time1, const FILETIME* time2)
{
	__int64 a = ((__int64)time1->dwHighDateTime) << 32 | time1->dwLowDateTime;
	__int64 b = ((__int64)time2->dwHighDateTime) << 32 | time2->dwLowDateTime;
	return b - a;
}

static void
get_cpu_usage_by_api(void)
{
	static FILETIME old_idle = { 0 };
	static FILETIME old_krnl = { 0 };
	static FILETIME old_user = { 0 };
	FILETIME idle = { 0 };
	FILETIME krnl = { 0 };
	FILETIME user = { 0 };
	__int64 diff_idle, diff_krnl, diff_user, total;
	g_ctx.cpu_usage = 0.0;
	GetSystemTimes(&idle, &krnl, &user);
	diff_idle = compare_file_time(&idle, &old_idle);
	diff_krnl = compare_file_time(&krnl, &old_krnl);
	diff_user = compare_file_time(&user, &old_user);
	total = diff_krnl + diff_user;
	if (total != 0)
		g_ctx.cpu_usage = (100.0 * _abs64(total - diff_idle)) / _abs64(total);
	old_idle = idle;
	old_krnl = krnl;
	old_user = user;
}

static void
get_cpu_msr(void)
{
	logical_cpu_t i;
	struct cpu_id_t* data;
	bool affinity_saved = FALSE;
	static int old_energy = 0;
	int value = CPU_INVALID_VALUE;
	if (g_ctx.lib.NwDrv == NULL ||
		g_ctx.lib.NwCpuid->num_cpu_types <= g_ctx.cpu_index)
		return;
	data = &g_ctx.lib.NwCpuid->cpu_types[g_ctx.cpu_index];
	if (!data->flags[CPU_FEATURE_MSR])
		return;
	affinity_saved = save_cpu_affinity();
	for (i = 0; i < data->num_logical_cpus; i++)
	{
		if (!get_affinity_mask_bit(i, &data->affinity_mask))
			continue;
		if (!set_cpu_affinity(i))
			continue;
		int min_multi = cpu_msrinfo(g_ctx.lib.NwDrv, INFO_MIN_MULTIPLIER);
		int max_multi = cpu_msrinfo(g_ctx.lib.NwDrv, INFO_MAX_MULTIPLIER);
		int cur_multi = cpu_msrinfo(g_ctx.lib.NwDrv, INFO_CUR_MULTIPLIER);
		if (min_multi == CPU_INVALID_VALUE)
			min_multi = 0;
		if (max_multi == CPU_INVALID_VALUE)
			max_multi = 0;
		if (cur_multi == CPU_INVALID_VALUE)
			cur_multi = 0;
		snprintf(g_ctx.cpu_msr_multi, GNWC_STR_SIZE, "%.1lf (%d - %d)", cur_multi / 100.0, min_multi / 100, max_multi / 100);
		value = cpu_msrinfo(g_ctx.lib.NwDrv, INFO_PKG_TEMPERATURE);
		if (value != CPU_INVALID_VALUE && value > 0)
			g_ctx.cpu_msr_temp = value;
		value = cpu_msrinfo(g_ctx.lib.NwDrv, INFO_VOLTAGE);
		if (value != CPU_INVALID_VALUE && value > 0)
			g_ctx.cpu_msr_volt = value / 100.0;
		value = cpu_msrinfo(g_ctx.lib.NwDrv, INFO_PKG_ENERGY);
		if (value != CPU_INVALID_VALUE && value > old_energy)
		{
			g_ctx.cpu_msr_power = (value - old_energy) / 100.0;
			old_energy = value;
		}
		value = cpu_msrinfo(g_ctx.lib.NwDrv, INFO_BUS_CLOCK);
		if (value != CPU_INVALID_VALUE && value > 0)
			g_ctx.cpu_msr_bus = value / 100.0;
		break;
	}
	if (affinity_saved)
		restore_cpu_affinity();
}

static void
get_network_traffic_by_api(void)
{
	static UINT64 old_recv = 0;
	static UINT64 old_send = 0;
	UINT64 recv = 0;
	UINT64 send = 0;
	UINT64 diff_recv, diff_send, i;
	for (i = 0; g_ctx.network->Children[i].LinkedNode; i++)
	{
		PNODE nw = g_ctx.network->Children[i].LinkedNode;
		if (!nw)
			continue;
		if (strcmp(gnwinfo_get_node_attr(nw, "Type"), "Software Loopback") == 0)
			continue;
		recv += strtoull(gnwinfo_get_node_attr(nw, "Received (Octets)"), NULL, 10);
		send += strtoull(gnwinfo_get_node_attr(nw, "Sent (Octets)"), NULL, 10);
	}
	diff_recv = (recv >= old_recv) ? recv - old_recv : 0;
	diff_send = (send >= old_send) ? send - old_send : 0;
	old_recv = recv;
	old_send = send;
	memcpy(g_ctx.net_recv, NWL_GetHumanSize(diff_recv, human_sizes, 1024), GNWC_STR_SIZE);
	memcpy(g_ctx.net_send, NWL_GetHumanSize(diff_send, human_sizes, 1024), GNWC_STR_SIZE);
}
#endif

void
gnwinfo_ctx_update(WPARAM wparam)
{
	switch (wparam)
	{
	case IDT_TIMER_1S:
		if (g_ctx.network)
			NWL_NodeFree(g_ctx.network, 1);
		g_ctx.network = NW_Network();
		NWL_GetUptime(g_ctx.sys_uptime, GNWC_STR_SIZE);
		g_ctx.mem_status.dwLength = sizeof(g_ctx.mem_status);
		GlobalMemoryStatusEx(&g_ctx.mem_status);
		memcpy(g_ctx.mem_avail, NWL_GetHumanSize(g_ctx.mem_status.ullAvailPhys, human_sizes, 1024), GNWC_STR_SIZE);
		memcpy(g_ctx.mem_total, NWL_GetHumanSize(g_ctx.mem_status.ullTotalPhys, human_sizes, 1024), GNWC_STR_SIZE);
#ifdef USE_PDH
		if (g_ctx.pdh && PdhCollectQueryData(g_ctx.pdh) == ERROR_SUCCESS)
		{
			PDH_FMT_COUNTERVALUE value = { 0 };
			if (g_ctx.pdh_cpu && PdhGetFormattedCounterValue(g_ctx.pdh_cpu, PDH_FMT_DOUBLE, NULL, &value) == ERROR_SUCCESS)
				g_ctx.cpu_usage = value.doubleValue;
			if (g_ctx.pdh_net_recv && PdhGetFormattedCounterValue(g_ctx.pdh_net_recv, PDH_FMT_LARGE, NULL, &value) == ERROR_SUCCESS)
				memcpy(g_ctx.net_recv, NWL_GetHumanSize(value.largeValue, human_sizes, 1024), GNWC_STR_SIZE);
			if (g_ctx.pdh_net_send && PdhGetFormattedCounterValue(g_ctx.pdh_net_send, PDH_FMT_LARGE, NULL, &value) == ERROR_SUCCESS)
				memcpy(g_ctx.net_send, NWL_GetHumanSize(value.largeValue, human_sizes, 1024), GNWC_STR_SIZE);
		}
#else
		get_network_traffic_by_api();
		get_cpu_usage_by_api();
#endif
		get_cpu_msr();
		break;
	case IDT_TIMER_1M:
		if (g_ctx.disk)
			NWL_NodeFree(g_ctx.disk, 1);
		g_ctx.lib.NwSmbiosInit = FALSE;
		g_ctx.disk = NW_Disk();
		if (g_ctx.battery)
			NWL_NodeFree(g_ctx.battery, 1);
		g_ctx.battery = NW_Battery();
		if (g_ctx.edid)
			NWL_NodeFree(g_ctx.edid, 1);
		g_ctx.edid = NW_Edid();
		break;
	}
}

void
gnwinfo_ctx_init(HINSTANCE inst, HWND wnd, struct nk_context* ctx, float width, float height)
{
	ZeroMemory(&g_ctx, sizeof(GNW_CONTEXT));
	g_ctx.mutex = CreateMutexW(NULL, TRUE, L"NWinfo{e25f6e37-d51b-4950-8949-510dfc86d913}");
	if (GetLastError() == ERROR_ALREADY_EXISTS || !g_ctx.mutex)
		exit(1);

	nk_begin(ctx, "Loading", nk_rect(width * 0.2f, height / 3, width * 0.6f, height / 4), NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_INPUT);
	nk_layout_row_dynamic(ctx, 0, 1);
	nk_spacer(ctx);
	nk_label (ctx, "Please wait ...", NK_TEXT_CENTERED);
	nk_spacer(ctx);
	nk_end(ctx);
	nk_gdip_render(NK_ANTI_ALIASING_ON, g_color_back);
#ifdef USE_PDH
	if (PdhOpenQueryW(NULL, 0, &g_ctx.pdh) != ERROR_SUCCESS)
		g_ctx.pdh = NULL;
	else
	{
		if (PdhAddCounterW(g_ctx.pdh, L"\\Processor Information(_Total)\\% Processor Time", 0, &g_ctx.pdh_cpu) != ERROR_SUCCESS)
			g_ctx.pdh_cpu = NULL;
		if (PdhAddCounterW(g_ctx.pdh, L"\\Network Interface(*)\\Bytes Sent/sec", 0, &g_ctx.pdh_net_send) != ERROR_SUCCESS)
			g_ctx.pdh_net_send = NULL;
		if (PdhAddCounterW(g_ctx.pdh, L"\\Network Interface(*)\\Bytes Received/sec", 0, &g_ctx.pdh_net_recv) != ERROR_SUCCESS)
			g_ctx.pdh_net_recv = NULL;
	}

	if (g_ctx.pdh)
		PdhCollectQueryData(g_ctx.pdh);
#endif
	g_ctx.gui_height = height;
	g_ctx.gui_width = width;
	g_ctx.inst = inst;
	g_ctx.wnd = wnd;
	g_ctx.nk = ctx;
	g_ctx.lib.NwFormat = FORMAT_JSON;
	g_ctx.lib.HumanSize = TRUE;
	g_ctx.lib.ErrLogCallback = gnwinfo_ctx_error_callback;
	g_ctx.lib.CodePage = CP_UTF8;

	g_ctx.lib.CpuInfo = TRUE;
	g_ctx.lib.SysInfo = TRUE;
	g_ctx.lib.DmiInfo = TRUE;
	g_ctx.lib.UefiInfo = TRUE;
	g_ctx.lib.PciInfo = TRUE;

	NW_Init(&g_ctx.lib);
	g_ctx.lib.NwSmartFlags = ~g_smart_flag;
	g_ctx.lib.NwRoot = NWL_NodeAlloc("NWinfo", 0);
	g_ctx.cpuid = NW_Cpuid();
	g_ctx.system = NW_System();
	g_ctx.smbios = NW_Smbios();
	g_ctx.uefi = NW_Uefi();
	g_ctx.pci = NW_Pci();

	g_ctx.sys_boot = gnwinfo_get_node_attr(g_ctx.system, "Boot Device");
	g_ctx.sys_disk = gnwinfo_get_node_attr(g_ctx.system, "System Device");

	gnwinfo_ctx_update(IDT_TIMER_1S);
	gnwinfo_ctx_update(IDT_TIMER_1M);

	g_ctx.image_os = load_png(IDR_PNG_OS);
	g_ctx.image_bios = load_png(IDR_PNG_FIRMWARE);
	g_ctx.image_board = load_png(IDR_PNG_PC);
	g_ctx.image_cpu = load_png(IDR_PNG_CPU);
	g_ctx.image_ram = load_png(IDR_PNG_MEMORY);
	g_ctx.image_edid = load_png(IDR_PNG_DISPLAY);
	g_ctx.image_disk = load_png(IDR_PNG_DISK);
	g_ctx.image_net = load_png(IDR_PNG_NETWORK);
	g_ctx.image_close = load_png(IDR_PNG_CLOSE);
	g_ctx.image_hdd = load_png(IDR_PNG_HDD);
	g_ctx.image_info = load_png(IDR_PNG_INFO);
	g_ctx.image_refresh = load_png(IDR_PNG_REFRESH);
	g_ctx.image_set = load_png(IDR_PNG_SETTINGS);
	g_ctx.image_sysdisk = load_png(IDR_PNG_SYSDISK);
	g_ctx.image_smart = load_png(IDR_PNG_SMART);
	g_ctx.image_cd = load_png(IDR_PNG_CD);
	g_ctx.image_cpuid = load_png(IDR_PNG_CPUID);
	g_ctx.image_pci = load_png(IDR_PNG_PCI);
	g_ctx.image_mm = load_png(IDR_PNG_MM);
	g_ctx.image_dmi = load_png(IDR_PNG_DMI);

	SetTimer(g_ctx.wnd, IDT_TIMER_1S, 1000, (TIMERPROC)NULL);
	SetTimer(g_ctx.wnd, IDT_TIMER_1M, 60 * 1000, (TIMERPROC)NULL);
}

noreturn void
gnwinfo_ctx_exit()
{
#ifdef USE_PDH
	if (g_ctx.pdh)
		PdhCloseQuery(g_ctx.pdh);
#endif
	KillTimer(g_ctx.wnd, IDT_TIMER_1S);
	KillTimer(g_ctx.wnd, IDT_TIMER_1M);
	ReleaseMutex(g_ctx.mutex);
	CloseHandle(g_ctx.mutex);
	NWL_NodeFree(g_ctx.network, 1);
	NWL_NodeFree(g_ctx.disk, 1);
	NWL_NodeFree(g_ctx.battery, 1);
	NWL_NodeFree(g_ctx.edid, 1);
	NW_Fini();
	nk_gdip_image_free(g_ctx.image_os);
	nk_gdip_image_free(g_ctx.image_bios);
	nk_gdip_image_free(g_ctx.image_board);
	nk_gdip_image_free(g_ctx.image_cpu);
	nk_gdip_image_free(g_ctx.image_ram);
	nk_gdip_image_free(g_ctx.image_edid);
	nk_gdip_image_free(g_ctx.image_disk);
	nk_gdip_image_free(g_ctx.image_net);
	nk_gdip_image_free(g_ctx.image_close);
	nk_gdip_image_free(g_ctx.image_hdd);
	nk_gdip_image_free(g_ctx.image_info);
	nk_gdip_image_free(g_ctx.image_refresh);
	nk_gdip_image_free(g_ctx.image_set);
	nk_gdip_image_free(g_ctx.image_sysdisk);
	nk_gdip_image_free(g_ctx.image_smart);
	nk_gdip_image_free(g_ctx.image_cd);
	nk_gdip_image_free(g_ctx.image_cpuid);
	nk_gdip_image_free(g_ctx.image_pci);
	nk_gdip_image_free(g_ctx.image_mm);
	nk_gdip_image_free(g_ctx.image_dmi);
	exit(0);
}
