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
	for (INT i = 0; g_ctx.edid->Children[i].LinkedNode; i++)
	{
		PNODE mon = g_ctx.edid->Children[i].LinkedNode;
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
	for (INT i = 0; pci03->Children[i].LinkedNode; i++)
	{
		PNODE gpu = pci03->Children[i].LinkedNode;
		nk_layout_row_dynamic(ctx, 0, 1);
		nk_image_label(ctx, GET_PNG(IDR_PNG_GPU), NWL_NodeAttrGet(gpu, "HWID"), NK_TEXT_LEFT, g_color_text_l);

		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });

		for (INT j = 0; gpu->Attributes[j].LinkedAttribute; j++)
		{
			NODE_ATT* att = gpu->Attributes[j].LinkedAttribute;
			if (strcmp(att->Key, "HWID") == 0)
				continue;
			nk_lhsc(ctx, att->Key, NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
			nk_lhc(ctx, att->Value, NK_TEXT_RIGHT, g_color_text_l);
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
