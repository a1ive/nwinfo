// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winioctl.h>
#include "gnwinfo.h"
#include "utils.h"

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

static __int64 compare_file_time(const FILETIME* time1, const FILETIME* time2)
{
	__int64 a = ((__int64)time1->dwHighDateTime) << 32 | time1->dwLowDateTime;
	__int64 b = ((__int64)time2->dwHighDateTime) << 32 | time2->dwLowDateTime;
	return b - a;
}

static void
get_cpu_usage(void)
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
get_cpu_info(int index)
{
	logical_cpu_t i;
	struct cpu_id_t* data;
	int value = CPU_INVALID_VALUE;
	data = &g_ctx.lib.NwCpuid->cpu_types[index];
	if (!data->flags[CPU_FEATURE_MSR])
		return;
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
		snprintf(g_ctx.cpu_info[index].cpu_msr_multi, GNWC_STR_SIZE, "%.1lf (%d - %d)", cur_multi / 100.0, min_multi / 100, max_multi / 100);
		value = cpu_msrinfo(g_ctx.lib.NwDrv, INFO_PKG_TEMPERATURE);
		if (value != CPU_INVALID_VALUE && value > 0)
			g_ctx.cpu_info[index].cpu_msr_temp = value;
		else
		{
			value = cpu_msrinfo(g_ctx.lib.NwDrv, INFO_TEMPERATURE);
			if (value != CPU_INVALID_VALUE && value > 0)
				g_ctx.cpu_info[index].cpu_msr_temp = value;
		}
		value = cpu_msrinfo(g_ctx.lib.NwDrv, INFO_VOLTAGE);
		if (value != CPU_INVALID_VALUE && value > 0)
			g_ctx.cpu_info[index].cpu_msr_volt = value / 100.0;
		value = cpu_msrinfo(g_ctx.lib.NwDrv, INFO_PKG_ENERGY);
		if (value != CPU_INVALID_VALUE && value > g_ctx.cpu_info[index].cpu_energy)
		{
			g_ctx.cpu_info[index].cpu_msr_power = (value - g_ctx.cpu_info[index].cpu_energy) / 100.0;
			g_ctx.cpu_info[index].cpu_energy = value;
		}
		value = cpu_msrinfo(g_ctx.lib.NwDrv, INFO_BUS_CLOCK);
		if (value != CPU_INVALID_VALUE && value > 0)
			g_ctx.cpu_info[index].cpu_msr_bus = value / 100.0;
		break;
	}
}

static void
get_cpu_msr(void)
{
	int i;
	bool affinity_saved = FALSE;
	static int old_energy = 0;
	int value = CPU_INVALID_VALUE;
	if (g_ctx.lib.NwDrv == NULL)
		return;
	affinity_saved = save_cpu_affinity();
	for (i = 0; i < g_ctx.cpu_count; i++)
		get_cpu_info(i);
	if (affinity_saved)
		restore_cpu_affinity();
}

static void
get_network_traffic(void)
{
	static UINT64 old_recv = 0;
	static UINT64 old_send = 0;
	const char* bit_units[6] =
	{ "b", "kb", "Mb", "Gb", "Tb", "Pb", };
	UINT64 recv = 0;
	UINT64 send = 0;
	UINT64 diff_recv, diff_send, i;
	for (i = 0; g_ctx.network->Children[i].LinkedNode; i++)
	{
		PNODE nw = g_ctx.network->Children[i].LinkedNode;
		if (!nw)
			continue;
		recv += strtoull(gnwinfo_get_node_attr(nw, "Received (Octets)"), NULL, 10);
		send += strtoull(gnwinfo_get_node_attr(nw, "Sent (Octets)"), NULL, 10);
	}
	diff_recv = (recv >= old_recv) ? recv - old_recv : 0;
	diff_send = (send >= old_send) ? send - old_send : 0;
	old_recv = recv;
	old_send = send;
	if (g_ctx.main_flag & MAIN_NET_UNIT_B)
	{
		memcpy(g_ctx.net_recv, NWL_GetHumanSize(diff_recv, NWLC->NwUnits, 1024), GNWC_STR_SIZE);
		memcpy(g_ctx.net_send, NWL_GetHumanSize(diff_send, NWLC->NwUnits, 1024), GNWC_STR_SIZE);
	}
	else
	{
		memcpy(g_ctx.net_recv, NWL_GetHumanSize(diff_recv * 8, bit_units, 1000), GNWC_STR_SIZE);
		memcpy(g_ctx.net_send, NWL_GetHumanSize(diff_send * 8, bit_units, 1000), GNWC_STR_SIZE);
	}
}

static void
get_memory_usage(void)
{
	NWL_GetMemInfo(&g_ctx.mem_status);
	memcpy(g_ctx.mem_avail, NWL_GetHumanSize(g_ctx.mem_status.PhysAvail, NWLC->NwUnits, 1024), GNWC_STR_SIZE);
	memcpy(g_ctx.mem_total, NWL_GetHumanSize(g_ctx.mem_status.PhysTotal, NWLC->NwUnits, 1024), GNWC_STR_SIZE);
	memcpy(g_ctx.page_avail, NWL_GetHumanSize(g_ctx.mem_status.PageAvail, NWLC->NwUnits, 1024), GNWC_STR_SIZE);
	memcpy(g_ctx.page_total, NWL_GetHumanSize(g_ctx.mem_status.PageTotal, NWLC->NwUnits, 1024), GNWC_STR_SIZE);
	memcpy(g_ctx.sfci_avail, NWL_GetHumanSize(g_ctx.mem_status.SfciAvail, NWLC->NwUnits, 1024), GNWC_STR_SIZE);
	memcpy(g_ctx.sfci_total, NWL_GetHumanSize(g_ctx.mem_status.SfciTotal, NWLC->NwUnits, 1024), GNWC_STR_SIZE);
}

#ifdef GNWINFO_ENABLE_PDH
static HMODULE hpdh = NULL;
static PDH_STATUS(WINAPI* pdh_open_query)(LPCWSTR, DWORD_PTR, PDH_HQUERY*) = NULL;
static PDH_STATUS(WINAPI* pdh_add_counter)(PDH_HQUERY, LPCWSTR, DWORD_PTR, PDH_HCOUNTER*) = NULL;
static PDH_STATUS(WINAPI* pdh_collect_query_data)(PDH_HQUERY) = NULL;
static PDH_STATUS(WINAPI* pdh_get_formatted_counter_value)(PDH_HCOUNTER, DWORD, LPDWORD, PPDH_FMT_COUNTERVALUE) = NULL;
static PDH_STATUS(WINAPI* pdh_close_query)(PDH_HQUERY) = NULL;

static void
get_pdh_data(void)
{
	PDH_FMT_COUNTERVALUE value = { 0 };
	if (!g_ctx.pdh)
		return;
	if (pdh_collect_query_data(g_ctx.pdh) != ERROR_SUCCESS)
	{
		pdh_close_query(g_ctx.pdh);
		g_ctx.pdh = NULL;
		return;
	}
	if (g_ctx.pdh_cpu_usage && pdh_get_formatted_counter_value(g_ctx.pdh_cpu_usage, PDH_FMT_DOUBLE, NULL, &value) == ERROR_SUCCESS)
		g_ctx.cpu_usage = value.doubleValue;
	if (g_ctx.pdh_cpu_base_freq && pdh_get_formatted_counter_value(g_ctx.pdh_cpu_base_freq, PDH_FMT_LONG, NULL, &value) == ERROR_SUCCESS)
		g_ctx.cpu_base_freq = (DWORD)value.longValue;
	if (g_ctx.pdh_cpu_freq && pdh_get_formatted_counter_value(g_ctx.pdh_cpu_freq, PDH_FMT_DOUBLE, NULL, &value) == ERROR_SUCCESS)
		g_ctx.cpu_freq = (DWORD)(value.doubleValue * 0.01 * g_ctx.cpu_base_freq);
	if (g_ctx.pdh_net_recv && pdh_get_formatted_counter_value(g_ctx.pdh_net_recv, PDH_FMT_LARGE, NULL, &value) == ERROR_SUCCESS)
		memcpy(g_ctx.net_recv, NWL_GetHumanSize(value.largeValue, NWLC->NwUnits, 1024), GNWC_STR_SIZE);
	if (g_ctx.pdh_net_send && pdh_get_formatted_counter_value(g_ctx.pdh_net_send, PDH_FMT_LARGE, NULL, &value) == ERROR_SUCCESS)
		memcpy(g_ctx.net_send, NWL_GetHumanSize(value.largeValue, NWLC->NwUnits, 1024), GNWC_STR_SIZE);
}
#endif

static void
get_display_info(void)
{
	MONITORINFO mni = { .cbSize = sizeof(MONITORINFO) };
	GetMonitorInfoW(MonitorFromWindow(g_ctx.wnd, MONITOR_DEFAULTTONEAREST), &mni);
	g_ctx.display_width = mni.rcMonitor.right - mni.rcMonitor.left;
	g_ctx.display_height = mni.rcMonitor.bottom - mni.rcMonitor.top;
	g_ctx.display_dpi = GetDpiForWindow(g_ctx.wnd);
	g_ctx.display_scale = 100 * g_ctx.display_dpi / USER_DEFAULT_SCREEN_DPI;
}

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
		get_memory_usage();
#ifdef GNWINFO_ENABLE_PDH
		get_pdh_data();
		if (!g_ctx.pdh)
#endif
		{
			get_network_traffic();
			get_cpu_usage();
		}
		get_cpu_msr();
		get_display_info();
		if (g_ctx.audio)
		{
			free(g_ctx.audio);
			g_ctx.audio = NULL;
		}
		if ((g_ctx.main_flag & MAIN_INFO_AUDIO) && g_ctx.lib.NwOsInfo.dwMajorVersion >= 6)
			g_ctx.audio = NWL_GetAudio(&g_ctx.audio_count);
		break;
	case IDT_TIMER_1M:
	case IDT_TIMER_POWER:
		if (g_ctx.battery)
			NWL_NodeFree(g_ctx.battery, 1);
		g_ctx.battery = NW_Battery();
		break;
	case IDT_TIMER_DISK:
		if (g_ctx.disk)
			NWL_NodeFree(g_ctx.disk, 1);
		g_ctx.lib.NwSmartInit = FALSE;
		g_ctx.lib.DisableSmart = (g_ctx.main_flag & MAIN_DISK_SMART) ? FALSE : TRUE;
		g_ctx.disk = NW_Disk();
		break;
	case IDT_TIMER_SMB:
		if (g_ctx.smb)
			NWL_NodeFree(g_ctx.smb, 1);
		g_ctx.smb = NW_NetShare();
		break;
	case IDT_TIMER_DISPLAY:
		if (g_ctx.edid)
			NWL_NodeFree(g_ctx.edid, 1);
		g_ctx.edid = NW_Edid();
		break;
	}
}

void
gnwinfo_ctx_init(HINSTANCE inst, HWND wnd, struct nk_context* ctx, float width, float height)
{
	g_ctx.mutex = CreateMutexW(NULL, TRUE, L"NWinfo{e25f6e37-d51b-4950-8949-510dfc86d913}");
	if (GetLastError() == ERROR_ALREADY_EXISTS || !g_ctx.mutex)
	{
		MessageBoxW(NULL, L"ALREADY RUNNING", L"ERROR", MB_ICONERROR | MB_OK);
		exit(1);
	}

	g_ctx.main_flag = strtoul(gnwinfo_get_ini_value(L"Widgets", L"MainFlags", L"0xFFFFFFFF"), NULL, 16);
	g_ctx.smart_hex = strtoul(gnwinfo_get_ini_value(L"Widgets", L"SmartFormat", L"0"), NULL, 10);
	// ~0x01FBFF81
	g_ctx.smart_flag = strtoul(gnwinfo_get_ini_value(L"Widgets", L"SmartFlags", L"0xFE04007E"), NULL, 16);
	g_ctx.gui_aa = strtoul(gnwinfo_get_ini_value(L"Window", L"AntiAliasing", L"1"), NULL, 10);

	nk_begin(ctx, gnwinfo_get_text(L"Loading"),
		nk_rect(width * 0.2f, height / 3, width * 0.6f, height / 4),
		NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_INPUT);
	nk_layout_row_dynamic(ctx, 0, 1);
	nk_spacer(ctx);
	nk_lhsc(ctx, gnwinfo_get_text(L"Please wait ..."), NK_TEXT_CENTERED, g_color_text_d, nk_false, nk_false);
	nk_spacer(ctx);
	nk_end(ctx);
	nk_gdip_render(g_ctx.gui_aa, g_color_back);

	g_ctx.gui_height = height;
	g_ctx.gui_width = width;
	g_ctx.gui_bginfo = !g_bginfo;
	g_ctx.gui_title = g_font_size + ctx->style.window.header.padding.y + ctx->style.window.header.label_padding.y;
	g_ctx.inst = inst;
	g_ctx.wnd = wnd;
	g_ctx.nk = ctx;
	g_ctx.lib.NwFormat = FORMAT_JSON;
	g_ctx.lib.HumanSize = TRUE;
	g_ctx.lib.ErrLogCallback = gnwinfo_ctx_error_callback;
	g_ctx.lib.CodePage = CP_UTF8;
	g_ctx.lib.SkipVirtualNet = TRUE;

	g_ctx.lib.CpuInfo = TRUE;
	g_ctx.lib.SysInfo = TRUE;
	g_ctx.lib.DmiInfo = TRUE;
	g_ctx.lib.UefiInfo = TRUE;
	g_ctx.lib.PciInfo = TRUE;

	NW_Init(&g_ctx.lib);
	g_ctx.lib.NwSmartFlags = ~g_ctx.smart_flag;
	g_ctx.lib.HideSensitive = strtoul(gnwinfo_get_ini_value(L"Window", L"HideSensitive", L"0"), NULL, 10);
	g_ctx.lib.NwRoot = NWL_NodeAlloc("NWinfo", 0);
	g_ctx.cpuid = NW_Cpuid();
	g_ctx.system = NW_System();
	g_ctx.smbios = NW_Smbios();
	g_ctx.uefi = NW_Uefi();
	g_ctx.pci = NW_Pci();

	g_ctx.sys_boot = gnwinfo_get_node_attr(g_ctx.system, "Boot Device");
	g_ctx.sys_disk = gnwinfo_get_node_attr(g_ctx.system, "System Device");

#ifdef GNWINFO_ENABLE_PDH
	hpdh = LoadLibraryW(L"pdh.dll");
	if (hpdh)
	{
		*(FARPROC*)&pdh_open_query = GetProcAddress(hpdh, "PdhOpenQueryW");
		*(FARPROC*)&pdh_add_counter = GetProcAddress(hpdh, "PdhAddCounterW");
		*(FARPROC*)&pdh_collect_query_data = GetProcAddress(hpdh, "PdhCollectQueryData");
		*(FARPROC*)&pdh_get_formatted_counter_value = GetProcAddress(hpdh, "PdhGetFormattedCounterValue");
		*(FARPROC*)&pdh_close_query = GetProcAddress(hpdh, "PdhCloseQuery");
	}
	if (pdh_open_query && pdh_add_counter && pdh_collect_query_data && pdh_get_formatted_counter_value && pdh_close_query
		&& !(g_ctx.main_flag & MAIN_NO_PDH)
		&& pdh_open_query(NULL, 0, &g_ctx.pdh) == ERROR_SUCCESS)
	{
		LPCWSTR cpu_str = L"\\Processor Information(_Total)\\% Processor Time";
		if (g_ctx.lib.NwOsInfo.dwMajorVersion >= 10)
			cpu_str = L"\\Processor Information(_Total)\\% Processor Utility";
		if (pdh_add_counter(g_ctx.pdh, cpu_str, 0, &g_ctx.pdh_cpu_usage) != ERROR_SUCCESS)
			g_ctx.pdh_cpu_usage = NULL;
		if (pdh_add_counter(g_ctx.pdh, L"\\Processor Information(_Total)\\% Processor Frequency", 0, &g_ctx.pdh_cpu_base_freq) != ERROR_SUCCESS)
			g_ctx.pdh_cpu_base_freq = NULL;
		if (pdh_add_counter(g_ctx.pdh, L"\\Processor Information(_Total)\\% Processor Performance", 0, &g_ctx.pdh_cpu_freq) != ERROR_SUCCESS)
			g_ctx.pdh_cpu_freq = NULL;
		if (pdh_add_counter(g_ctx.pdh, L"\\Network Interface(*)\\Bytes Sent/sec", 0, &g_ctx.pdh_net_send) != ERROR_SUCCESS)
			g_ctx.pdh_net_send = NULL;
		if (pdh_add_counter(g_ctx.pdh, L"\\Network Interface(*)\\Bytes Received/sec", 0, &g_ctx.pdh_net_recv) != ERROR_SUCCESS)
			g_ctx.pdh_net_recv = NULL;
		pdh_collect_query_data(g_ctx.pdh);
	}
	else
		g_ctx.pdh = NULL;
#endif

	g_ctx.cpu_count = (int)g_ctx.lib.NwCpuid->num_cpu_types;
	if (g_ctx.cpu_count > 0)
		g_ctx.cpu_info = calloc((size_t)g_ctx.cpu_count, sizeof(GNW_CPU_INFO));
	else
		g_ctx.cpu_info = NULL;
	NWL_GetRegDwordValue(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"~MHz", &g_ctx.cpu_base_freq);
	g_ctx.cpu_freq = g_ctx.cpu_base_freq;

	NWL_GetHostname(g_ctx.sys_hostname);

	gnwinfo_ctx_update(IDT_TIMER_1S);
	gnwinfo_ctx_update(IDT_TIMER_1M);
	gnwinfo_ctx_update(IDT_TIMER_DISK);
	gnwinfo_ctx_update(IDT_TIMER_DISPLAY);
	gnwinfo_ctx_update(IDT_TIMER_SMB);

	for (WORD i = 0; i < sizeof(g_ctx.image) / sizeof(g_ctx.image[0]); i++)
		g_ctx.image[i] = load_png(i + IDR_PNG_MIN);

	SetTimer(g_ctx.wnd, IDT_TIMER_1S, 1000, (TIMERPROC)NULL);
	SetTimer(g_ctx.wnd, IDT_TIMER_1M, 60 * 1000, (TIMERPROC)NULL);
}

noreturn void
gnwinfo_ctx_exit()
{
	KillTimer(g_ctx.wnd, IDT_TIMER_1S);
	KillTimer(g_ctx.wnd, IDT_TIMER_1M);

#ifdef GNWINFO_ENABLE_PDH
	if (g_ctx.pdh)
		pdh_close_query(g_ctx.pdh);
	if (hpdh)
		FreeLibrary(hpdh);
#endif

	if (g_ctx.cpu_info)
		free(g_ctx.cpu_info);

	if (g_ctx.audio)
		free(g_ctx.audio);

	ReleaseMutex(g_ctx.mutex);
	CloseHandle(g_ctx.mutex);
	NWL_NodeFree(g_ctx.network, 1);
	NWL_NodeFree(g_ctx.disk, 1);
	NWL_NodeFree(g_ctx.smb, 1);
	NWL_NodeFree(g_ctx.battery, 1);
	NWL_NodeFree(g_ctx.edid, 1);
	NW_Fini();
	for (WORD i = 0; i < sizeof(g_ctx.image) / sizeof(g_ctx.image[0]); i++)
		nk_gdip_image_free(g_ctx.image[i]);
	exit(0);
}
