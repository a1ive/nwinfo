// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include "gnwinfo.h"
#include "utils.h"

struct nk_color
gnwinfo_get_color(double value, double warn, double err)
{
	if (value > err)
		return g_color_error;
	if (value > warn)
		return g_color_warning;
	if (value <= 0.0)
		return g_color_unknown;
	return g_color_good;
}

LPCSTR
gnwinfo_get_smbios_attr(LPCSTR type, LPCSTR key, PVOID ctx, BOOL(*cond)(PNODE node, PVOID ctx))
{
	INT i;
	for (i = 0; g_ctx.smbios->Children[i].LinkedNode; i++)
	{
		PNODE tab = g_ctx.smbios->Children[i].LinkedNode;
		LPCSTR attr = NWL_NodeAttrGet(tab, "Table Type");
		if (strcmp(attr, type) != 0)
			continue;
		if (!cond || cond(tab, ctx) == TRUE)
			return NWL_NodeAttrGet(tab, key);
	}
	return "-";
}

void
gnwinfo_draw_percent_prog(struct nk_context* ctx, double percent)
{
	nk_size size = (nk_size)percent;
	struct nk_color color = gnwinfo_get_color(percent, 70.0, 90.0);
	if (size == 0)
		size = 1;
	else if (size > 100)
		size = 100;
	ctx->style.progress.cursor_normal = nk_style_item_color(color);
	ctx->style.progress.cursor_hover = nk_style_item_color(color);
	ctx->style.progress.cursor_active = nk_style_item_color(color);
	nk_progress(ctx, &size, 100, 0);
}
