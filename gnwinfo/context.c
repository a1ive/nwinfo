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

void
gnwinfo_ctx_update(WPARAM wparam)
{
	switch (wparam)
	{
	case IDT_TIMER_1S:
		NWL_PdhUpdate();
		if (g_ctx.network)
			NWL_NodeFree(g_ctx.network, 1);
		g_ctx.lib.NetFlags = NW_NET_PHYS | ((g_ctx.main_flag & MAIN_NET_INACTIVE) ? 0 : NW_NET_ACTIVE);
		g_ctx.network = NW_Network();
		NWL_GetUptime(g_ctx.sys_uptime, NWL_STR_SIZE);
		NWL_GetMemInfo(&g_ctx.mem_status);
		NWL_GetNetTraffic(&g_ctx.net_traffic, !(g_ctx.main_flag & MAIN_NET_UNIT_B));
		g_ctx.cpu_usage = NWL_GetCpuUsage();
		g_ctx.cpu_freq = NWL_GetCpuFreq();
		NWL_GetCpuMsr(g_ctx.cpu_count, g_ctx.cpu_info);
		NWL_GetCurDisplay(g_ctx.wnd, &g_ctx.cur_display);
		NWL_GetGpuInfo(&g_ctx.gpu_info);
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
		g_ctx.lib.DiskFlags = (g_ctx.main_flag & MAIN_DISK_SMART) ? 0 : NW_DISK_NO_SMART;
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
	g_ctx.lib.NetFlags = NW_NET_PHYS | ((g_ctx.main_flag & MAIN_NET_INACTIVE) ? 0 : NW_NET_ACTIVE);
	g_ctx.lib.EnablePdh = !(g_ctx.main_flag & MAIN_NO_PDH);

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

	g_ctx.sys_boot = NWL_NodeAttrGet(g_ctx.system, "Boot Device");
	g_ctx.sys_disk = NWL_NodeAttrGet(g_ctx.system, "System Device");

	g_ctx.cpu_count = (int)g_ctx.lib.NwCpuid->num_cpu_types;
	if (g_ctx.cpu_count > 0)
		g_ctx.cpu_info = calloc((size_t)g_ctx.cpu_count, sizeof(NWLIB_CPU_INFO));
	else
		g_ctx.cpu_info = NULL;

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
gnwinfo_ctx_exit(void)
{
	KillTimer(g_ctx.wnd, IDT_TIMER_1S);
	KillTimer(g_ctx.wnd, IDT_TIMER_1M);

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
