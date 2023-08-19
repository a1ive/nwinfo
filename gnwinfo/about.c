// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

#include <version.h>

VOID
gnwinfo_draw_about_window(struct nk_context* ctx, float width, float height)
{
	if (g_ctx.gui_about == FALSE)
		return;
	if (!nk_begin(ctx, gnwinfo_get_text(L"About"),
		nk_rect(width / 4.0f, height / 3.0f, width / 2.0f, height / 5.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE))
	{
		g_ctx.gui_about = FALSE;
		goto out;
	}

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_spacer(ctx);
	nk_label(ctx, NWINFO_GUI, NK_TEXT_CENTERED);
	nk_label(ctx, NWINFO_COPYRIGHT, NK_TEXT_CENTERED);
	nk_label(ctx, "v" NWINFO_VERSION_STR, NK_TEXT_CENTERED);
	nk_label(ctx, "Build. " __DATE__ " " __TIME__, NK_TEXT_CENTERED);

out:
	nk_end(ctx);
}
