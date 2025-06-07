// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "gettext.h"

#include <version.h>

VOID
gnwinfo_draw_about_window(struct nk_context* ctx, float width, float height)
{
	if (!(g_ctx.window_flag & GUI_WINDOW_ABOUT))
		return;
	if (!nk_begin_ex(ctx, N_(N__ABOUT),
		nk_rect(width / 4.0f, height / 3.0f, width / 2.0f, height / 5.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE,
		GET_PNG(IDR_PNG_INFO), GET_PNG(IDR_PNG_CLOSE)))
	{
		g_ctx.window_flag &= ~GUI_WINDOW_ABOUT;
		goto out;
	}

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_spacer(ctx);
	nk_l(ctx, NWINFO_GUI, NK_TEXT_CENTERED);
	nk_l(ctx, NWINFO_COPYRIGHT, NK_TEXT_CENTERED);
	nk_l(ctx, "v" NWINFO_VERSION_STR, NK_TEXT_CENTERED);
	nk_l(ctx, "Build. " __DATE__ " " __TIME__, NK_TEXT_CENTERED);
	nk_lf(ctx, NK_TEXT_CENTERED, "%s (%u)", N_(N__LANG_NAME_), g_lang_id);

out:
	nk_end(ctx);
}
