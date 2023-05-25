// SPDX-License-Identifier: Unlicense

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>

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

	struct nk_image image_os;
	struct nk_image image_bios;
	struct nk_image image_board;
	struct nk_image image_cpu;
	struct nk_image image_ram;
	struct nk_image image_edid;
	struct nk_image image_disk;
	struct nk_image image_net;
	struct nk_image image_close;
	struct nk_image image_smart;
	struct nk_image image_cpuid;
	struct nk_image image_dir;

	PNODE system;
	PNODE cpuid;
	PNODE smbios;
	PNODE network;
	PNODE disk;
	PNODE edid;
	PNODE pci;
	PNODE uefi;
} GNW_CONTEXT;
extern GNW_CONTEXT g_ctx;

void gnwinfo_ctx_init(HINSTANCE inst, HWND wnd, struct nk_context* ctx);
void gnwinfo_ctx_exit();
VOID gnwinfo_draw_main_window(struct nk_context* ctx, float width, float height);
