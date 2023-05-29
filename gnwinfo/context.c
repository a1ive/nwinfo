// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

#include "icons.h"

#pragma comment(lib, "pdh.lib")

GNW_CONTEXT g_ctx;

static void
gnwinfo_ctx_error_callback(LPCSTR lpszText)
{
	MessageBoxA(g_ctx.wnd, lpszText, "Error", MB_ICONERROR);
}

#define GDIP_LOAD_IMG(img, x) \
   img = nk_gdip_load_image_from_memory(x, sizeof(x))

LPCSTR NWL_GetHumanSize(UINT64 size, LPCSTR human_sizes[6], UINT64 base);
static const char* human_sizes[6] =
{ "B", "K", "M", "G", "T", "P", };

void
gnwinfo_ctx_update(WPARAM wparam)
{
	switch (wparam)
	{
	case IDT_TIMER_1S:
		if (g_ctx.cpuid)
			NWL_NodeFree(g_ctx.cpuid, 1);
		g_ctx.cpuid = NW_Cpuid();
		if (g_ctx.network)
			NWL_NodeFree(g_ctx.network, 1);
		g_ctx.network = NW_Network();
		g_ctx.sys_uptime = NWL_GetUptime();
		g_ctx.mem_status.dwLength = sizeof(g_ctx.mem_status);
		GlobalMemoryStatusEx(&g_ctx.mem_status);
		memcpy(g_ctx.mem_avail, NWL_GetHumanSize(g_ctx.mem_status.ullAvailPhys, human_sizes, 1024), 48);
		memcpy(g_ctx.mem_total, NWL_GetHumanSize(g_ctx.mem_status.ullTotalPhys, human_sizes, 1024), 48);
		if (g_ctx.pdh && PdhCollectQueryData(g_ctx.pdh) == ERROR_SUCCESS)
		{
			PDH_FMT_COUNTERVALUE value = { 0 };
			if (g_ctx.pdh_cpu && PdhGetFormattedCounterValue(g_ctx.pdh_cpu, PDH_FMT_DOUBLE, NULL, &value) == ERROR_SUCCESS)
				g_ctx.pdh_val_cpu = value.doubleValue;
			if (g_ctx.pdh_net_recv && PdhGetFormattedCounterValue(g_ctx.pdh_net_recv, PDH_FMT_LARGE, NULL, &value) == ERROR_SUCCESS)
				memcpy(g_ctx.net_recv, NWL_GetHumanSize(value.largeValue, human_sizes, 1024), 48);
			if (g_ctx.pdh_net_send && PdhGetFormattedCounterValue(g_ctx.pdh_net_send, PDH_FMT_LARGE, NULL, &value) == ERROR_SUCCESS)
				memcpy(g_ctx.net_send, NWL_GetHumanSize(value.largeValue, human_sizes, 1024), 48);
		}

		break;
	case IDT_TIMER_1M:
		if (g_ctx.disk)
			NWL_NodeFree(g_ctx.disk, 1);
		g_ctx.disk = NW_Disk();
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

	nk_begin(ctx, "Loading", nk_rect(0, height / 3, width, height / 4), NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_INPUT);
	nk_layout_row_dynamic(ctx, 0, 1);
	nk_spacer(ctx);
	nk_label (ctx, "Please wait ...", NK_TEXT_CENTERED);
	nk_spacer(ctx);
	nk_end(ctx);
	nk_gdip_render(NK_ANTI_ALIASING_ON, NK_COLOR_GRAY);

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

	g_ctx.gui_height = height;
	g_ctx.gui_width = width;
	g_ctx.inst = inst;
	g_ctx.wnd = wnd;
	g_ctx.nk = ctx;
	g_ctx.main_flag = ~0U;
	g_ctx.lib.NwFormat = FORMAT_JSON;
	g_ctx.lib.HumanSize = TRUE;
	g_ctx.lib.ErrLogCallback = gnwinfo_ctx_error_callback;
	g_ctx.lib.CodePage = CP_UTF8;

	g_ctx.lib.SysInfo = TRUE;
	g_ctx.lib.DmiInfo = TRUE;
	g_ctx.lib.EdidInfo = TRUE;
	g_ctx.lib.UefiInfo = TRUE;
	g_ctx.lib.PciInfo = TRUE;

	NW_Init(&g_ctx.lib);
	g_ctx.lib.NwRoot = NWL_NodeAlloc("NWinfo", 0);
	g_ctx.system = NW_System();
	g_ctx.smbios = NW_Smbios();
	g_ctx.edid = NW_Edid();
	g_ctx.uefi = NW_Uefi();
	g_ctx.pci = NW_Pci();

	gnwinfo_ctx_update(IDT_TIMER_1S);
	gnwinfo_ctx_update(IDT_TIMER_1M);

	GDIP_LOAD_IMG(g_ctx.image_os, ICON_OS);
	GDIP_LOAD_IMG(g_ctx.image_bios, ICON_BIOS);
	GDIP_LOAD_IMG(g_ctx.image_board, ICON_BOARD);
	GDIP_LOAD_IMG(g_ctx.image_cpu, ICON_CPU);
	GDIP_LOAD_IMG(g_ctx.image_ram, ICON_RAM);
	GDIP_LOAD_IMG(g_ctx.image_edid, ICON_EDID);
	GDIP_LOAD_IMG(g_ctx.image_disk, ICON_DISK);
	GDIP_LOAD_IMG(g_ctx.image_net, ICON_NET);
	GDIP_LOAD_IMG(g_ctx.image_close, ICON_CLOSE);
	GDIP_LOAD_IMG(g_ctx.image_dir, ICON_DIR);
	GDIP_LOAD_IMG(g_ctx.image_info, ICON_INFO);
	GDIP_LOAD_IMG(g_ctx.image_refresh, ICON_REFRESH);
	GDIP_LOAD_IMG(g_ctx.image_set, ICON_SET);

	SetTimer(g_ctx.wnd, IDT_TIMER_1S, 1000, (TIMERPROC)NULL);
	SetTimer(g_ctx.wnd, IDT_TIMER_1M, 60 * 1000, (TIMERPROC)NULL);
}

noreturn void
gnwinfo_ctx_exit()
{
	if (g_ctx.pdh)
		PdhCloseQuery(g_ctx.pdh);
	KillTimer(g_ctx.wnd, IDT_TIMER_1S);
	KillTimer(g_ctx.wnd, IDT_TIMER_1M);
	ReleaseMutex(g_ctx.mutex);
	CloseHandle(g_ctx.mutex);
	NWL_NodeFree(g_ctx.network, 1);
	NWL_NodeFree(g_ctx.disk, 1);
	NWL_NodeFree(g_ctx.cpuid, 1);
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
	nk_gdip_image_free(g_ctx.image_dir);
	nk_gdip_image_free(g_ctx.image_info);
	nk_gdip_image_free(g_ctx.image_refresh);
	nk_gdip_image_free(g_ctx.image_set);
	exit(0);
}
