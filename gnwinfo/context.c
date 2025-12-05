// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winioctl.h>
#include <objbase.h>
#include "gnwinfo.h"
#include "utils.h"
#include "gettext.h"

#include "../libcpuid/libcpuid.h"
#include "../libcpuid/libcpuid_util.h"

GNW_CONTEXT g_ctx;

#define GNW_UPDATE_FLAG_1S      (1u << 0)
#define GNW_UPDATE_FLAG_1M      (1u << 1)
#define GNW_UPDATE_FLAG_DISK    (1u << 2)
#define GNW_UPDATE_FLAG_DISPLAY (1u << 3)
#define GNW_UPDATE_FLAG_POWER   (1u << 4)
#define GNW_UPDATE_FLAG_SMB     (1u << 5)
#define GNW_UPDATE_FLAG_SPD     (1u << 6)

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

static DWORD
gnwinfo_ctx_update_flag(WPARAM wparam)
{
	switch (wparam)
	{
	case IDT_TIMER_1S: return GNW_UPDATE_FLAG_1S;
	case IDT_TIMER_1M: return GNW_UPDATE_FLAG_1M;
	case IDT_TIMER_DISK: return GNW_UPDATE_FLAG_DISK;
	case IDT_TIMER_DISPLAY: return GNW_UPDATE_FLAG_DISPLAY;
	case IDT_TIMER_POWER: return GNW_UPDATE_FLAG_POWER;
	case IDT_TIMER_SMB: return GNW_UPDATE_FLAG_SMB;
	case IDT_TIMER_SPD: return GNW_UPDATE_FLAG_SPD;
	default: return 0;
	}
}

static void
gnwinfo_ctx_update_1s(void)
{
	PNODE network = NULL;
	NWLIB_MEM_INFO mem_status = { 0 };
	NWLIB_NET_TRAFFIC net_traffic = { 0 };
	double cpu_usage = 0.0;
	DWORD cpu_freq = 0;
	NWLIB_CPU_INFO* cpu_info = NULL;
	NWLIB_CUR_DISPLAY cur_display = { 0 };
	NWLIB_GPU_INFO gpu_info;
	UINT audio_count = 0;
	NWLIB_AUDIO_DEV* audio = NULL;
	CHAR sys_uptime[NWL_STR_SIZE] = { 0 };
	DWORD main_flag = 0;
	int cpu_count = 0;
	BOOL enable_audio = FALSE;
	NWLIB_MEM_SENSORS mem_sensors = { 0 };

	AcquireSRWLockShared(&g_ctx.lock);
	main_flag = g_ctx.main_flag;
	cpu_count = g_ctx.cpu_count;
	enable_audio = (g_ctx.lib.NwOsInfo.dwMajorVersion >= 6);
	if (cpu_count > 0 && g_ctx.cpu_info)
	{
		cpu_info = malloc((size_t)cpu_count * sizeof(NWLIB_CPU_INFO));
		if (cpu_info)
			memcpy(cpu_info, g_ctx.cpu_info, (size_t)cpu_count * sizeof(NWLIB_CPU_INFO));
	}
	gpu_info = g_ctx.gpu_info;
	mem_sensors = g_ctx.mem_sensors;
	ReleaseSRWLockShared(&g_ctx.lock);

	g_ctx.lib.NetFlags = NW_NET_PHYS | ((main_flag & MAIN_NET_INACTIVE) ? 0 : NW_NET_ACTIVE);
	network = NW_Network();
	NWL_GetUptime(sys_uptime, NWL_STR_SIZE);
	NWL_GetMemInfo(&mem_status);
	NWL_GetNetTraffic(&net_traffic, !(main_flag & MAIN_NET_UNIT_B));
	cpu_usage = NWL_GetCpuUsage();
	cpu_freq = NWL_GetCpuFreq();
	if (cpu_info && cpu_count > 0)
		NWL_GetCpuMsr(cpu_count, cpu_info);
	NWL_GetCurDisplay(g_ctx.wnd, &cur_display);
	NWL_GetGpuInfo(&gpu_info);
	if ((main_flag & MAIN_INFO_AUDIO) && enable_audio)
		audio = NWL_GetAudio(&audio_count);
	NWL_GetMemSensors(g_ctx.lib.NwSmbus, &mem_sensors);

	AcquireSRWLockExclusive(&g_ctx.lock);
	PNODE old_network = g_ctx.network;
	g_ctx.network = network;
	memcpy(g_ctx.sys_uptime, sys_uptime, sizeof(g_ctx.sys_uptime));
	g_ctx.mem_status = mem_status;
	g_ctx.net_traffic = net_traffic;
	g_ctx.cpu_usage = cpu_usage;
	g_ctx.cpu_freq = cpu_freq;
	if (cpu_info && g_ctx.cpu_info && cpu_count == g_ctx.cpu_count)
		memcpy(g_ctx.cpu_info, cpu_info, (size_t)cpu_count * sizeof(NWLIB_CPU_INFO));
	g_ctx.cur_display = cur_display;
	g_ctx.gpu_info = gpu_info;
	NWLIB_AUDIO_DEV* old_audio = g_ctx.audio;
	g_ctx.audio = audio;
	g_ctx.audio_count = audio_count;
	g_ctx.mem_sensors = mem_sensors;
	ReleaseSRWLockExclusive(&g_ctx.lock);

	if (old_network)
		NWL_NodeFree(old_network, 1);
	if (old_audio)
		free(old_audio);
	if (cpu_info)
		free(cpu_info);
}

static void
gnwinfo_ctx_update_battery(void)
{
	PNODE battery = NW_Battery();

	AcquireSRWLockExclusive(&g_ctx.lock);
	PNODE old_battery = g_ctx.battery;
	g_ctx.battery = battery;
	ReleaseSRWLockExclusive(&g_ctx.lock);

	if (old_battery)
		NWL_NodeFree(old_battery, 1);
}

static void
gnwinfo_ctx_update_disk(void)
{
	DWORD main_flag = 0;

	AcquireSRWLockShared(&g_ctx.lock);
	main_flag = g_ctx.main_flag;
	ReleaseSRWLockShared(&g_ctx.lock);

	g_ctx.lib.NwSmartInit = FALSE;
	g_ctx.lib.DiskFlags = (main_flag & MAIN_DISK_SMART) ? 0 : NW_DISK_NO_SMART;

	PNODE disk = NW_Disk();

	AcquireSRWLockExclusive(&g_ctx.lock);
	PNODE old_disk = g_ctx.disk;
	g_ctx.disk = disk;
	ReleaseSRWLockExclusive(&g_ctx.lock);

	if (old_disk)
		NWL_NodeFree(old_disk, 1);
}

static void
gnwinfo_ctx_update_smb(void)
{
	PNODE smb = NW_NetShare();

	AcquireSRWLockExclusive(&g_ctx.lock);
	PNODE old_smb = g_ctx.smb;
	g_ctx.smb = smb;
	ReleaseSRWLockExclusive(&g_ctx.lock);

	if (old_smb)
		NWL_NodeFree(old_smb, 1);
}

static void
gnwinfo_ctx_update_display(void)
{
	AcquireSRWLockExclusive(&g_ctx.lock);
	NWL_FreeGpu(&g_ctx.gpu_info);
	ReleaseSRWLockExclusive(&g_ctx.lock);

	NWLIB_GPU_INFO gpu_info;
	NWL_InitGpu(&gpu_info);
	PNODE edid = NW_Edid();

	AcquireSRWLockExclusive(&g_ctx.lock);
	g_ctx.gpu_info = gpu_info;
	PNODE old_edid = g_ctx.edid;
	g_ctx.edid = edid;
	ReleaseSRWLockExclusive(&g_ctx.lock);

	if (old_edid)
		NWL_NodeFree(old_edid, 1);
}

static void
gnwinfo_ctx_update_spd(void)
{
	DWORD main_flag = 0;
	AcquireSRWLockShared(&g_ctx.lock);
	main_flag = g_ctx.main_flag;
	ReleaseSRWLockShared(&g_ctx.lock);

	PNODE spd = (main_flag & MAIN_SMBUS_SPD) ? NULL : NW_Spd();

	AcquireSRWLockExclusive(&g_ctx.lock);
	PNODE old_spd = g_ctx.spd;
	g_ctx.spd = spd;
	ReleaseSRWLockExclusive(&g_ctx.lock);

	if (old_spd)
		NWL_NodeFree(old_spd, 1);
}

static void
gnwinfo_ctx_update_internal(WPARAM wparam)
{
	switch (wparam)
	{
	case IDT_TIMER_1S:
		gnwinfo_ctx_update_1s();
		break;
	case IDT_TIMER_1M:
	case IDT_TIMER_POWER:
		gnwinfo_ctx_update_battery();
		break;
	case IDT_TIMER_DISK:
		gnwinfo_ctx_update_disk();
		break;
	case IDT_TIMER_SMB:
		gnwinfo_ctx_update_smb();
		break;
	case IDT_TIMER_DISPLAY:
		gnwinfo_ctx_update_display();
		break;
	case IDT_TIMER_SPD:
		gnwinfo_ctx_update_spd();
		break;
	default:
		break;
	}
}

static DWORD WINAPI
gnwinfo_ctx_update_thread(LPVOID lpParameter)
{
	(void)lpParameter;
	for (;;)
	{
		if (WaitForSingleObject(g_ctx.update_event, INFINITE) != WAIT_OBJECT_0)
			break;
		if (g_ctx.update_stop)
			break;

		for (;;)
		{
			DWORD mask = (DWORD)InterlockedExchange(&g_ctx.update_mask, 0);
			if (mask == 0)
				break;
			if (mask & GNW_UPDATE_FLAG_1S)
				gnwinfo_ctx_update_internal(IDT_TIMER_1S);
			if (mask & GNW_UPDATE_FLAG_1M)
				gnwinfo_ctx_update_internal(IDT_TIMER_1M);
			if (mask & GNW_UPDATE_FLAG_DISK)
				gnwinfo_ctx_update_internal(IDT_TIMER_DISK);
			if (mask & GNW_UPDATE_FLAG_DISPLAY)
				gnwinfo_ctx_update_internal(IDT_TIMER_DISPLAY);
			if (mask & GNW_UPDATE_FLAG_POWER)
				gnwinfo_ctx_update_internal(IDT_TIMER_POWER);
			if (mask & GNW_UPDATE_FLAG_SMB)
				gnwinfo_ctx_update_internal(IDT_TIMER_SMB);
			if (mask & GNW_UPDATE_FLAG_SPD)
				gnwinfo_ctx_update_internal(IDT_TIMER_SPD);
			if (g_ctx.update_stop)
				break;
		}
	}
	return 0;
}

void
gnwinfo_ctx_update(WPARAM wparam)
{
	DWORD flag = gnwinfo_ctx_update_flag(wparam);

	if (!flag)
		return;

	if (!g_ctx.update_event || !g_ctx.update_thread)
	{
		gnwinfo_ctx_update_internal(wparam);
		return;
	}

	InterlockedOr(&g_ctx.update_mask, (LONG)flag);
	SetEvent(g_ctx.update_event);
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

	InitializeSRWLock(&g_ctx.lock);
	g_ctx.update_mask = 0;
	g_ctx.update_stop = 0;
	g_ctx.exit_pending = 0;
	g_ctx.update_event = CreateEventW(NULL, FALSE, FALSE, NULL);
	if (g_ctx.update_event)
		g_ctx.update_thread = CreateThread(NULL, 0, gnwinfo_ctx_update_thread, NULL, 0, NULL);

	g_ctx.main_flag = strtoul(gnwinfo_get_ini_value(L"Widgets", L"MainFlags", L"0xFFFFFFFF"), NULL, 16);
	g_ctx.smart_hex = strtoul(gnwinfo_get_ini_value(L"Widgets", L"SmartFormat", L"0"), NULL, 10);
	// ~0x01FBFF81
	g_ctx.smart_flag = strtoul(gnwinfo_get_ini_value(L"Widgets", L"SmartFlags", L"0xFE04007E"), NULL, 16);
	g_ctx.gui_aa = strtoul(gnwinfo_get_ini_value(L"Window", L"AntiAliasing", L"1"), NULL, 10);

	nk_begin_ex(ctx, N_(N__LOADING),
		nk_rect(width * 0.2f, height / 3, width * 0.6f, height / 4),
		NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_INPUT, nk_image_id(0), GET_PNG(IDR_PNG_CLOSE));
	nk_layout_row_dynamic(ctx, 0, 1);
	nk_spacer(ctx);
	nk_lhsc(ctx, N_(N__PLS_WAIT), NK_TEXT_CENTERED, g_color_text_d, nk_false, nk_false);
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
	g_ctx.lib.NwFile = stdout;
	g_ctx.lib.AcpiTable = 0;
	g_ctx.lib.SmbiosType = 127;
	g_ctx.lib.DiskPath = NULL;

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

	gnwinfo_ctx_update(IDT_TIMER_DISPLAY);
	gnwinfo_ctx_update(IDT_TIMER_1S);
	gnwinfo_ctx_update(IDT_TIMER_1M);
	gnwinfo_ctx_update(IDT_TIMER_DISK);
	gnwinfo_ctx_update(IDT_TIMER_SMB);
	gnwinfo_ctx_update(IDT_TIMER_SPD);

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

	if (g_ctx.update_event)
	{
		g_ctx.update_stop = 1;
		SetEvent(g_ctx.update_event);
	}
	if (g_ctx.update_thread)
	{
		WaitForSingleObject(g_ctx.update_thread, INFINITE);
		CloseHandle(g_ctx.update_thread);
		g_ctx.update_thread = NULL;
	}
	if (g_ctx.update_event)
	{
		CloseHandle(g_ctx.update_event);
		g_ctx.update_event = NULL;
	}

	if (g_ctx.cpu_info)
		free(g_ctx.cpu_info);

	NWL_FreeGpu(&g_ctx.gpu_info);

	if (g_ctx.audio)
		free(g_ctx.audio);

	ReleaseMutex(g_ctx.mutex);
	CloseHandle(g_ctx.mutex);
	NWL_NodeFree(g_ctx.network, 1);
	NWL_NodeFree(g_ctx.disk, 1);
	NWL_NodeFree(g_ctx.smb, 1);
	NWL_NodeFree(g_ctx.spd, 1);
	NWL_NodeFree(g_ctx.battery, 1);
	NWL_NodeFree(g_ctx.edid, 1);
	NW_Fini();
	for (WORD i = 0; i < sizeof(g_ctx.image) / sizeof(g_ctx.image[0]); i++)
		nk_gdip_image_free(g_ctx.image[i]);
	exit(0);
}
