// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include "gnwinfo.h"
#include "utils.h"
#include "gettext.h"

static inline VOID
draw_key_value(struct nk_context* ctx, PNODE node, LPCSTR key)
{
	nk_lhsc(ctx, key, NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
	nk_lhc(ctx, NWL_NodeAttrGet(node, key), NK_TEXT_RIGHT, g_color_text_l);
}

static VOID
draw_monitors(struct nk_context* ctx)
{
	INT count = NWL_NodeChildCount(g_ctx.edid);
	for (INT i = 0; i < count; i++)
	{
		PNODE mon = NWL_NodeEnumChild(g_ctx.edid, i);
		LPCSTR id = NWL_NodeAttrGet(mon, "ID");
		if (id[0] == '-')
			continue;

		nk_layout_row_dynamic(ctx, 0, 1);
		nk_image_label(ctx, GET_PNG(IDR_PNG_MONITOR),
			NWL_NodeAttrGet(mon, "HWID"), NK_TEXT_LEFT, g_color_text_l);

		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });

		draw_key_value(ctx, mon, "Manufacturer");
		draw_key_value(ctx, mon, "Serial Number");
		draw_key_value(ctx, mon, "Date");
		draw_key_value(ctx, mon, "EDID Version");
		draw_key_value(ctx, mon, "Display Name");
		draw_key_value(ctx, mon, "Max Resolution");
		draw_key_value(ctx, mon, "Max Refresh Rate (Hz)");
		draw_key_value(ctx, mon, "Gamma");
		draw_key_value(ctx, mon, "Aspect Ratio");
		draw_key_value(ctx, mon, "Width (cm)");
		draw_key_value(ctx, mon, "Height (cm)");
		draw_key_value(ctx, mon, "Diagonal (in)");
	}
}

static VOID
draw_gpu(struct nk_context* ctx)
{
	PNODE pci03 = g_ctx.gpu_info.PciList;
	INT pci03_count = NWL_NodeChildCount(pci03);
	for (INT i = 0; i < pci03_count; i++)
	{
		PNODE gpu = NWL_NodeEnumChild(pci03, i);
		if (!gpu)
			continue;
		nk_layout_row_dynamic(ctx, 0, 1);
		nk_image_label(ctx, GET_PNG(IDR_PNG_GPU), NWL_NodeAttrGet(gpu, "HWID"), NK_TEXT_LEFT, g_color_text_l);

		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });

		INT attr_count = NWL_NodeAttrCount(gpu);
		for (INT j = 0; j < attr_count; j++)
		{
			NODE_ATT* att = NWL_NodeAttrEnum(gpu, j);
			if (!att || strcmp(att->key, "HWID") == 0)
				continue;
			nk_lhsc(ctx, att->key, NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
			nk_lhc(ctx, att->value, NK_TEXT_RIGHT, g_color_text_l);
		}
	}
}

VOID
gnwinfo_draw_display_window(struct nk_context* ctx, float width, float height)
{
	if (!(g_ctx.window_flag & GUI_WINDOW_DISPLAY))
		return;
	struct nk_rect window = { .x = 0, .y = height / 4.0f, .w = width * 0.98f, .h = height / 2.0f };
	if (!nk_begin_ex(ctx, N_(N__DISPLAY),
		window,
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE,
		GET_PNG(IDR_PNG_DISPLAY), GET_PNG(IDR_PNG_CLOSE)))
	{
		g_ctx.window_flag &= ~GUI_WINDOW_DISPLAY;
		goto out;
	}

	draw_gpu(ctx);

	draw_monitors(ctx);

out:
	nk_end(ctx);
}
