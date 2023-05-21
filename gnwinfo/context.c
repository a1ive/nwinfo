// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

#include "icons.h"

GNW_CONTEXT g_ctx;

static void
gnwinfo_ctx_error_callback(LPCSTR lpszText)
{
	MessageBoxA(g_ctx.wnd, lpszText, "Error", MB_ICONERROR);
}

void
gnwinfo_ctx_init(HINSTANCE inst, HWND wnd, struct nk_context* ctx)
{
	ZeroMemory(&g_ctx, sizeof(GNW_CONTEXT));
	g_ctx.mutex = CreateMutexW(NULL, TRUE, L"NWinfo{e25f6e37-d51b-4950-8949-510dfc86d913}");
	if (GetLastError() == ERROR_ALREADY_EXISTS || !g_ctx.mutex)
		exit(1);
	g_ctx.inst = inst;
	g_ctx.wnd = wnd;
	g_ctx.nk = ctx;
	g_ctx.lib.NwFormat = FORMAT_JSON;
	g_ctx.lib.HumanSize = TRUE;
	g_ctx.lib.ErrLogCallback = gnwinfo_ctx_error_callback;

	g_ctx.lib.SysInfo = TRUE;
	g_ctx.lib.DmiInfo = TRUE;
	g_ctx.lib.EdidInfo = TRUE;
	g_ctx.lib.UefiInfo = TRUE;

	NW_Init(&g_ctx.lib);
	g_ctx.lib.NwRoot = NWL_NodeAlloc("NWinfo", 0);
	g_ctx.system = NW_System();
	//g_ctx.cpuid = NW_Cpuid();
	g_ctx.smbios = NW_Smbios();
	//g_ctx.network = NW_Network(); // Update: 1S
	g_ctx.disk = NW_Disk(); // Update: 1MIN
	g_ctx.edid = NW_Edid();
	g_ctx.uefi = NW_Uefi();

	g_ctx.image_os = nk_gdip_load_image_from_memory(ICON_OS, ICON_OS_LEN);
	g_ctx.image_bios = nk_gdip_load_image_from_memory(ICON_BIOS, ICON_BIOS_LEN);
	g_ctx.image_board = nk_gdip_load_image_from_memory(ICON_BOARD, ICON_BOARD_LEN);
	g_ctx.image_cpu = nk_gdip_load_image_from_memory(ICON_CPU, ICON_CPU_LEN);
	g_ctx.image_ram = nk_gdip_load_image_from_memory(ICON_RAM, ICON_RAM_LEN);
	g_ctx.image_edid = nk_gdip_load_image_from_memory(ICON_EDID, ICON_EDID_LEN);
	g_ctx.image_disk = nk_gdip_load_image_from_memory(ICON_DISK, ICON_DISK_LEN);
	g_ctx.image_net = nk_gdip_load_image_from_memory(ICON_NET, ICON_NET_LEN);

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
	exit(0);
}
