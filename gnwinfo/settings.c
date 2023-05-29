// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

VOID
gnwinfo_draw_settings_window(struct nk_context* ctx, float width, float height)
{
	if (g_ctx.gui_settings == FALSE)
		return;
	if (!nk_begin(ctx, "Settings",
		nk_rect(width / 4.0f, height / 3.0f, width / 2.0f, height / 3.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE))
	{
		g_ctx.gui_settings = FALSE;
		goto out;
	}

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, "Hide components", NK_TEXT_LEFT);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.1f, 0.45f, 0.45f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, "OS", &g_ctx.main_flag, MAIN_INFO_OS);
	nk_checkbox_flags_label(ctx, "BIOS", &g_ctx.main_flag, MAIN_INFO_BIOS);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, "Computer", &g_ctx.main_flag, MAIN_INFO_BOARD);
	nk_checkbox_flags_label(ctx, "CPU", &g_ctx.main_flag, MAIN_INFO_CPU);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, "Memory", &g_ctx.main_flag, MAIN_INFO_MEMORY);
	nk_checkbox_flags_label(ctx, "Display Devices", &g_ctx.main_flag, MAIN_INFO_MONITOR);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, "Storage", &g_ctx.main_flag, MAIN_INFO_STORAGE);
	nk_checkbox_flags_label(ctx, "Network", &g_ctx.main_flag, MAIN_INFO_NETWORK);

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, "Network", NK_TEXT_LEFT);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, "Hide inactive network", &g_ctx.main_flag, MAIN_NET_ACTIVE);

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, "Storage", NK_TEXT_LEFT);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	g_ctx.smart_hex = !nk_check_label(ctx, "Display SMART in HEX format", !g_ctx.smart_hex);

out:
	nk_end(ctx);
}
