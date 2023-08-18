// SPDX-License-Identifier: Unlicense

#pragma once

#define GNWINFO_ENABLE_PDH

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#ifdef GNWINFO_ENABLE_PDH
#include <pdh.h>
#endif

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR

#include <assert.h>
#define NK_ASSERT(expr) assert(expr)

#include <nuklear.h>
#include <nuklear_gdip.h>

#include <libnw.h>

#include "resource.h"

GdipFont*
nk_gdip_load_font(LPCWSTR name, int size, WORD fallback);

void
nk_image_label(struct nk_context* ctx, struct nk_image img,
	const char* str, nk_flags align, struct nk_color color);

void
nk_space_label(struct nk_context* ctx, const char* str, nk_flags align);

#define GNWINFO_FONT_SIZE 12

#define MAIN_INFO_OS        (1U << 0)
#define MAIN_INFO_BIOS      (1U << 1)
#define MAIN_INFO_BOARD     (1U << 2)
#define MAIN_INFO_CPU       (1U << 3)
#define MAIN_INFO_MEMORY    (1U << 4)
#define MAIN_INFO_MONITOR   (1U << 5)
#define MAIN_INFO_STORAGE   (1U << 6)
#define MAIN_INFO_NETWORK   (1U << 7)

#define MAIN_NET_INACTIVE   (1U << 16)
#define MAIN_NO_PDH         (1U << 17)
#define MAIN_NET_DETAIL     (1U << 18)
#define MAIN_DISK_SMART     (1U << 19)
#define MAIN_OS_DETAIL      (1U << 20)
#define MAIN_OS_UPTIME      (1U << 21)
#define MAIN_B_VERSION      (1U << 22)
#define MAIN_B_VENDOR       (1U << 23)
#define MAIN_CPU_DETAIL     (1U << 24)
#define MAIN_CPU_CACHE      (1U << 25)
#define MAIN_DISK_COMPACT   (1U << 26)
#define MAIN_MEM_DETAIL     (1U << 27)

#define GNWC_STR_SIZE       64

typedef struct _GNW_CPU_INFO
{
	CHAR cpu_msr_multi[GNWC_STR_SIZE];
	int cpu_msr_temp;
	double cpu_msr_volt;
	double cpu_msr_bus;
	int cpu_energy;
	double cpu_msr_power;
}GNW_CPU_INFO;

typedef struct _GNW_CONTEXT
{
	HINSTANCE inst;
	HWND wnd;
	HANDLE mutex;
	WCHAR lang[10];
	BOOL mouse;
	NWLIB_CONTEXT lib;
	struct nk_context* nk;
	float gui_title;
	float gui_width;
	float gui_height;
	float gui_ratio;
	UINT32 main_flag;
	BOOL gui_cpuid;
	BOOL gui_smart;
	BOOL gui_about;
	BOOL gui_settings;
	BOOL gui_pci;
	BOOL gui_dmi;
	BOOL gui_mm;
	nk_bool smart_hex;
	UINT32 smart_flag;
	nk_bool gui_bginfo;

	struct nk_image image_os;
	struct nk_image image_bios;
	struct nk_image image_board;
	struct nk_image image_cpu;
	struct nk_image image_ram;
	struct nk_image image_edid;
	struct nk_image image_disk;
	struct nk_image image_net;
	struct nk_image image_close;
	struct nk_image image_hdd;
	struct nk_image image_info;
	struct nk_image image_refresh;
	struct nk_image image_set;
	struct nk_image image_sysdisk;
	struct nk_image image_smart;
	struct nk_image image_cpuid;
	struct nk_image image_cd;
	struct nk_image image_pci;
	struct nk_image image_mm;
	struct nk_image image_dmi;

	PNODE system;
	PNODE cpuid;
	PNODE smbios;
	PNODE network;
	PNODE disk;
	PNODE edid;
	PNODE pci;
	PNODE uefi;
	PNODE battery;

	LPCSTR sys_boot;
	LPCSTR sys_disk;

#ifdef GNWINFO_ENABLE_PDH
	PDH_HQUERY pdh;
	PDH_HCOUNTER pdh_cpu;
	PDH_HCOUNTER pdh_net_recv;
	PDH_HCOUNTER pdh_net_send;
#endif

	CHAR net_recv[GNWC_STR_SIZE];
	CHAR net_send[GNWC_STR_SIZE];
	double cpu_usage;
	int cpu_count;
	GNW_CPU_INFO* cpu_info;
	
	CHAR sys_uptime[GNWC_STR_SIZE];
	NWLIB_MEM_INFO mem_status;
	CHAR mem_total[GNWC_STR_SIZE];
	CHAR mem_avail[GNWC_STR_SIZE];
	CHAR page_total[GNWC_STR_SIZE];
	CHAR page_avail[GNWC_STR_SIZE];
	CHAR sfci_total[GNWC_STR_SIZE];
	CHAR sfci_avail[GNWC_STR_SIZE];

} GNW_CONTEXT;
extern GNW_CONTEXT g_ctx;

extern WCHAR g_ini_path[MAX_PATH];
extern unsigned int g_init_width;
extern unsigned int g_init_height;
extern unsigned int g_init_alpha;
extern nk_bool g_bginfo;

#define NK_COLOR_YELLOW     {0xFF, 0xEA, 0x00, 0xFF}
#define NK_COLOR_RED        {0xFF, 0x17, 0x44, 0xFF}
#define NK_COLOR_GREEN      {0x00, 0xE6, 0x76, 0xFF}
#define NK_COLOR_CYAN       {0x03, 0xDA, 0xC6, 0xFF}
#define NK_COLOR_BLUE       {0x29, 0x79, 0xFF, 0xFF}
#define NK_COLOR_WHITE      {0xFF, 0xFF, 0xFF, 0xFF}
#define NK_COLOR_BLACK      {0x00, 0x00, 0x00, 0xFF}
#define NK_COLOR_GRAY       {0x1E, 0x1E, 0x1E, 0xFF}
#define NK_COLOR_LIGHT      {0xBF, 0xBF, 0xBF, 0xFF}
#define NK_COLOR_DARK       {0x2D, 0x2D, 0x2D, 0xFF}

extern struct nk_color g_color_warning;
extern struct nk_color g_color_error;
extern struct nk_color g_color_good;
extern struct nk_color g_color_unknown;
extern struct nk_color g_color_text_l;
extern struct nk_color g_color_text_d;
extern struct nk_color g_color_back;

void gnwinfo_ctx_update(WPARAM wparam);
void gnwinfo_ctx_init(HINSTANCE inst, HWND wnd, struct nk_context* ctx, float width, float height);
void gnwinfo_ctx_exit();
VOID gnwinfo_draw_main_window(struct nk_context* ctx, float width, float height);
VOID gnwinfo_draw_cpuid_window(struct nk_context* ctx, float width, float height);
VOID gnwinfo_draw_about_window(struct nk_context* ctx, float width, float height);
VOID gnwinfo_draw_smart_window(struct nk_context* ctx, float width, float height);
VOID gnwinfo_draw_settings_window(struct nk_context* ctx, float width, float height);
VOID gnwinfo_draw_pci_window(struct nk_context* ctx, float width, float height);
VOID gnwinfo_draw_dmi_window(struct nk_context* ctx, float width, float height);

VOID gnwinfo_init_mm_window(struct nk_context* ctx);
VOID gnwinfo_draw_mm_window(struct nk_context* ctx, float width, float height);

LPCSTR gnwinfo_get_ini_value(LPCWSTR section, LPCWSTR key, LPCWSTR fallback);
LPCSTR gnwinfo_get_text(LPCWSTR text);
void gnwinfo_set_ini_value(LPCWSTR section, LPCWSTR key, LPCWSTR _Printf_format_string_ format, ...);

LPCSTR gnwinfo_get_node_attr(PNODE node, LPCSTR key);
struct nk_color gnwinfo_get_color(double value, double warn, double err);
void gnwinfo_draw_percent_prog(struct nk_context* ctx, double percent);
