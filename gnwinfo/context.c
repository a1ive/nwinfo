// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

#include "icons.h"

GNW_CONTEXT g_ctx;

static void
gnwinfo_ctx_error_callback(LPCSTR lpszText)
{
	MessageBoxA(g_ctx.wnd, lpszText, "Error", MB_ICONERROR);
}

#define GDIP_LOAD_IMG(img, x) \
   img = nk_gdip_load_image_from_memory(x, sizeof(x))

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
	nk_gdip_render(NK_ANTI_ALIASING_ON, nk_rgb(30, 30, 30));

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
	//g_ctx.cpuid = NW_Cpuid();
	g_ctx.smbios = NW_Smbios();
	//g_ctx.network = NW_Network(); // Update: 1S
	g_ctx.disk = NW_Disk(); // Update: 1MIN
	g_ctx.edid = NW_Edid();
	g_ctx.uefi = NW_Uefi();
	g_ctx.pci = NW_Pci();

	GDIP_LOAD_IMG(g_ctx.image_os, ICON_OS);
	GDIP_LOAD_IMG(g_ctx.image_bios, ICON_BIOS);
	GDIP_LOAD_IMG(g_ctx.image_board, ICON_BOARD);
	GDIP_LOAD_IMG(g_ctx.image_cpu, ICON_CPU);
	GDIP_LOAD_IMG(g_ctx.image_ram, ICON_RAM);
	GDIP_LOAD_IMG(g_ctx.image_edid, ICON_EDID);
	GDIP_LOAD_IMG(g_ctx.image_disk, ICON_DISK);
	GDIP_LOAD_IMG(g_ctx.image_net, ICON_NET);
	GDIP_LOAD_IMG(g_ctx.image_close, ICON_CLOSE);
	GDIP_LOAD_IMG(g_ctx.image_smart, ICON_SMART);
	GDIP_LOAD_IMG(g_ctx.image_cpuid, ICON_CPUID);
	GDIP_LOAD_IMG(g_ctx.image_dir, ICON_DIR);
	GDIP_LOAD_IMG(g_ctx.image_info, ICON_INFO);

	SetTimer(g_ctx.wnd, IDT_TIMER_1S, 1000, (TIMERPROC)NULL);
	SetTimer(g_ctx.wnd, IDT_TIMER_1M, 60 * 1000, (TIMERPROC)NULL);
}

noreturn void
gnwinfo_ctx_exit()
{
	KillTimer(g_ctx.wnd, IDT_TIMER_1S);
	KillTimer(g_ctx.wnd, IDT_TIMER_1M);
	ReleaseMutex(g_ctx.mutex);
	CloseHandle(g_ctx.mutex);
	if (g_ctx.network)
		NWL_NodeFree(g_ctx.network, 1);
	if (g_ctx.disk)
		NWL_NodeFree(g_ctx.disk, 1);
	if (g_ctx.cpuid)
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
	nk_gdip_image_free(g_ctx.image_smart);
	nk_gdip_image_free(g_ctx.image_cpuid);
	nk_gdip_image_free(g_ctx.image_dir);
	nk_gdip_image_free(g_ctx.image_info);
	exit(0);
}
