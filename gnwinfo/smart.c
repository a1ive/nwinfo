// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "../libcdi/libcdi.h"

LPCSTR NWL_GetHumanSize(UINT64 size, LPCSTR human_sizes[6], UINT64 base);
static const char* disk_human_sizes[6] =
{ "MB", "GB", "TB", "PB", "EB", "ZB", };

static void
draw_rect(struct nk_context* ctx, struct nk_color bg, const char* str)
{
	static struct nk_style_button style;
	memcpy(&style, &ctx->style.button, sizeof(struct nk_style_button));
	style.normal.type = NK_STYLE_ITEM_COLOR;
	style.normal.data.color = bg;
	style.hover.type = NK_STYLE_ITEM_COLOR;
	style.hover.data.color = bg;
	style.active.type = NK_STYLE_ITEM_COLOR;
	style.active.data.color = bg;
	style.text_normal = NK_COLOR_BLACK;
	style.text_hover = NK_COLOR_BLACK;
	style.text_active = NK_COLOR_BLACK;
	nk_button_label_styled(ctx, &style, str);
}

static void
draw_health(struct nk_context* ctx, CDI_SMART* smart, int disk, float height)
{
	int n;
	char* str;
	char tmp[32];

	if (nk_group_begin(ctx, "Health", NK_WINDOW_NO_INPUT))
	{
		struct nk_color color = NK_COLOR_YELLOW;
		nk_layout_row_dynamic(ctx, height / 5.0f, 1);
		nk_label(ctx, "Health Status", NK_TEXT_CENTERED);
		n = cdi_get_int(smart, disk, CDI_INT_LIFE);
		str = cdi_get_string(smart, disk, CDI_STRING_DISK_STATUS);
		if (strncmp(str, "Good", 4) == 0)
			color = NK_COLOR_GREEN;
		else if (strncmp(str, "Bad", 3) == 0)
			color = NK_COLOR_RED;
		if (n >= 0)
			snprintf(tmp, sizeof(tmp), "%s\n%d%%", str, n);
		else
			snprintf(tmp, sizeof(tmp), "%s", str);
		draw_rect(ctx, color, tmp);
		cdi_free_string(str);
		nk_label(ctx, "Temperature", NK_TEXT_CENTERED);
		color = NK_COLOR_YELLOW;
		int alarm = cdi_get_int(smart, disk, CDI_INT_TEMPERATURE_ALARM);
		if (alarm <= 0)
			alarm = 60;
		n = cdi_get_int(smart, disk, CDI_INT_TEMPERATURE);
		if (n > 0 && n < alarm)
			color = NK_COLOR_GREEN;
		snprintf(tmp, sizeof(tmp), u8"%d ¡ãC", n);
		draw_rect(ctx, color, tmp);
		nk_group_end(ctx);
	}
}

VOID
gnwinfo_draw_smart_window(struct nk_context* ctx, float width, float height)
{
	INT count;
	char* str;
	static int cur_disk = 0;

	if (g_ctx.gui_smart == FALSE)
		return;
	if (!nk_begin(ctx, "S.M.A.R.T.",
		nk_rect(0, height / 4.0f, width, height / 2.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE))
	{
		g_ctx.gui_smart = FALSE;
		goto out;
	}

	count = cdi_get_disk_count(NWLC->NwSmart);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.2f, 0.8f });
	nk_property_int(ctx, "DISK", 0, &cur_disk, count - 1, 1, 1);
	str = cdi_get_string(NWLC->NwSmart, cur_disk, CDI_STRING_MODEL);
	nk_labelf_colored(ctx, NK_TEXT_CENTERED, NK_COLOR_WHITE,
		"%s %s",
		str,
		NWL_GetHumanSize(cdi_get_dword(NWLC->NwSmart, cur_disk, CDI_DWORD_DISK_SIZE), disk_human_sizes, 1000));
	cdi_free_string(str);

	
	nk_layout_row(ctx, NK_DYNAMIC, height / 4.0f, 2, (float[2]) {0.4f, 0.6f});
	draw_health(ctx, NWLC->NwSmart, cur_disk, height / 4.0f);

out:
	nk_end(ctx);
}
