// SPDX-License-Identifier: Unlicense

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <pdh.h>

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
#define MAIN_NET_PUB_IP     (1U << 17)

typedef struct _GNW_CONTEXT
{
	HINSTANCE inst;
	HWND wnd;
	HANDLE mutex;
	NWLIB_CONTEXT lib;
	struct nk_context* nk;
	float gui_width;
	float gui_height;
	UINT32 main_flag;
	BOOL gui_cpuid;
	BOOL gui_smart;
	BOOL gui_about;
	BOOL gui_settings;
	nk_bool smart_hex;

	struct nk_image image_os;
	struct nk_image image_bios;
	struct nk_image image_board;
	struct nk_image image_cpu;
	struct nk_image image_ram;
	struct nk_image image_edid;
	struct nk_image image_disk;
	struct nk_image image_net;
	struct nk_image image_close;
	struct nk_image image_dir;
	struct nk_image image_info;
	struct nk_image image_refresh;
	struct nk_image image_set;

	PNODE system;
	PNODE cpuid;
	PNODE smbios;
	PNODE network;
	PNODE disk;
	PNODE edid;
	PNODE pci;
	PNODE uefi;

	PDH_HQUERY pdh;
	PDH_HCOUNTER pdh_cpu;
	PDH_HCOUNTER pdh_net_recv;
	PDH_HCOUNTER pdh_net_send;
	double pdh_val_cpu;
	CHAR net_recv[48];
	CHAR net_send[48];

	LPCSTR sys_uptime;
	MEMORYSTATUSEX mem_status;
	CHAR mem_total[48];
	CHAR mem_avail[48];
	CHAR pub_ip[128];
} GNW_CONTEXT;
extern GNW_CONTEXT g_ctx;

extern WCHAR g_ini_path[MAX_PATH];
extern unsigned int g_init_width;
extern unsigned int g_init_height;
extern unsigned int g_init_alpha;
extern nk_bool g_bginfo;

#define NK_COLOR_YELLOW     {0xFF, 0xEA, 0x00, 0xff}
#define NK_COLOR_RED        {0xFF, 0x17, 0x44, 0xff}
#define NK_COLOR_GREEN      {0x00, 0xE6, 0x76, 0xff}
#define NK_COLOR_CYAN       {0x03, 0xDA, 0xC6, 0xff}
#define NK_COLOR_BLUE       {0x29, 0x79, 0xFF, 0xff}
#define NK_COLOR_WHITE      {0xFF, 0xFF, 0xFF, 0xff}
#define NK_COLOR_BLACK      {0x00, 0x00, 0x00, 0xff}
#define NK_COLOR_GRAY       {0x1E, 0x1E, 0x1E, 0xff}
#define NK_COLOR_LIGHT      {0xBF, 0xBF, 0xBF, 0xff}
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

char* gnwinfo_get_ini_value(LPCWSTR section, LPCWSTR key, LPCWSTR fallback);
void gnwinfo_set_ini_value(LPCWSTR section, LPCWSTR key, LPCWSTR _Printf_format_string_ format, ...);

LPCSTR gnwinfo_get_node_attr(PNODE node, LPCSTR key);
