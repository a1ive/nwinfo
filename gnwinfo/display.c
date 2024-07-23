// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

static inline VOID
draw_key_value(struct nk_context* ctx, PNODE node, LPCSTR key)
{
	nk_lhsc(ctx, key, NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
	nk_lhc(ctx, gnwinfo_get_node_attr(node, key), NK_TEXT_RIGHT, g_color_text_l);
}

static VOID
draw_monitors(struct nk_context* ctx)
{
	INT i;
	for (i = 0; g_ctx.edid->Children[i].LinkedNode; i++)
	{
		PNODE mon = g_ctx.edid->Children[i].LinkedNode;
		LPCSTR id = gnwinfo_get_node_attr(mon, "ID");
		if (id[0] == '-')
			continue;

		nk_layout_row_dynamic(ctx, 0, 1);
		nk_image_label(ctx, GET_PNG(IDR_PNG_MONITOR),
			gnwinfo_get_node_attr(mon, "HWID"), NK_TEXT_LEFT, g_color_text_l);

		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });

		draw_key_value(ctx, mon, "Manufacturer");
		draw_key_value(ctx, mon, "Serial Number");
		draw_key_value(ctx, mon, "Date");
		draw_key_value(ctx, mon, "Display Name");
		draw_key_value(ctx, mon, "Max Resolution");
		draw_key_value(ctx, mon, "Max Refresh Rate (Hz)");
		draw_key_value(ctx, mon, "Width (cm)");
		draw_key_value(ctx, mon, "Height (cm)");
		draw_key_value(ctx, mon, "Diagonal (in)");
	}
}

static VOID
draw_gpu(struct nk_context* ctx)
{
	INT i, j;
	for (i = 0; g_ctx.pci->Children[i].LinkedNode; i++)
	{
		PNODE pci = g_ctx.pci->Children[i].LinkedNode;
		LPCSTR attr = gnwinfo_get_node_attr(pci, "Class Code");
		if (strncmp("03", attr, 2) != 0)
			continue;
		LPCSTR vendor = gnwinfo_get_node_attr(pci, "Vendor");
		if (strcmp(vendor, "-") == 0)
			continue;
		LPCSTR hwid = gnwinfo_get_node_attr(pci, "HWID");

		nk_layout_row_dynamic(ctx, 0, 1);
		nk_image_label(ctx, GET_PNG(IDR_PNG_GPU), hwid, NK_TEXT_LEFT, g_color_text_l);

		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });

		draw_key_value(ctx, pci, "Vendor");
		draw_key_value(ctx, pci, "Device");

		for (j = 0; g_ctx.gpu->Children[j].LinkedNode; j++)
		{
			PNODE gpu = g_ctx.gpu->Children[j].LinkedNode;
			if (_stricmp(hwid, gnwinfo_get_node_attr(gpu, "HWID")) != 0)
				continue;
			draw_key_value(ctx, gpu, "Description");
			draw_key_value(ctx, gpu, "Driver Date");
			draw_key_value(ctx, gpu, "Driver Version");
			draw_key_value(ctx, gpu, "Location Info");
			draw_key_value(ctx, gpu, "Memory Size");
		}
	}
}

VOID
gnwinfo_draw_display_window(struct nk_context* ctx, float width, float height)
{
	if (!(g_ctx.window_flag & GUI_WINDOW_DISPLAY))
		return;
	struct nk_rect window = { .x = 0, .y = height / 4.0f, .w = width * 0.98f, .h = height / 2.0f };
	if (!nk_begin(ctx, gnwinfo_get_text(L"Display Devices"),
		window,
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE))
	{
		g_ctx.window_flag &= ~GUI_WINDOW_DISPLAY;
		goto out;
	}

	draw_gpu(ctx);

	draw_monitors(ctx);

out:
	nk_end(ctx);
}
