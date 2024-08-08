// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include "gnwinfo.h"
#include "utils.h"

static inline VOID
draw_key_value(struct nk_context* ctx, PNODE node, LPCSTR key)
{
	nk_lhsc(ctx, key, NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
	nk_lhc(ctx, NWL_NodeAttrGet(node, key), NK_TEXT_RIGHT, g_color_text_l);
}

static VOID
draw_monitors(struct nk_context* ctx)
{
	INT i;
	for (i = 0; g_ctx.edid->Children[i].LinkedNode; i++)
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
		draw_key_value(ctx, mon, "Display Name");
		draw_key_value(ctx, mon, "Max Resolution");
		draw_key_value(ctx, mon, "Max Refresh Rate (Hz)");
		draw_key_value(ctx, mon, "Width (cm)");
		draw_key_value(ctx, mon, "Height (cm)");
		draw_key_value(ctx, mon, "Diagonal (in)");
	}
}

static inline VOID
draw_gpu_attr(struct nk_context* ctx, LPCSTR str, LPCSTR attr)
{
	nk_lhsc(ctx, str, NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
	nk_lhc(ctx, attr, NK_TEXT_RIGHT, g_color_text_l);
}

static VOID
draw_gpu(struct nk_context* ctx)
{
	int i;
	for (i = 0; i < g_ctx.gpu_info.DeviceCount; i++)
	{
		nk_layout_row_dynamic(ctx, 0, 1);
		nk_image_label(ctx, GET_PNG(IDR_PNG_GPU), g_ctx.gpu_info.Device[i].gpu_hwid, NK_TEXT_LEFT, g_color_text_l);

		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });

		draw_gpu_attr(ctx, "Vendor", g_ctx.gpu_info.Device[i].gpu_vendor);
		draw_gpu_attr(ctx, "Device", g_ctx.gpu_info.Device[i].gpu_device);
		if (g_ctx.gpu_info.Device[i].driver)
		{
			draw_gpu_attr(ctx, "Driver Date", g_ctx.gpu_info.Device[i].gpu_driver_date);
			draw_gpu_attr(ctx, "Driver Version", g_ctx.gpu_info.Device[i].gpu_driver_ver);
			draw_gpu_attr(ctx, "Location Info", g_ctx.gpu_info.Device[i].gpu_location);
			draw_gpu_attr(ctx, "Memory Size",
				NWL_GetHumanSize(g_ctx.gpu_info.Device[i].gpu_mem_size, NWLC->NwUnits, 1024));
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
