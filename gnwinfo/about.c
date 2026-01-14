// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "gettext.h"

#include "ioctl.h"
#include "version.h"

LPCSTR NWL_Ucs2ToUtf8(LPCWSTR src);

static void
draw_install_pawnio_button(struct nk_context* ctx)
{
	if (!nk_button_label(ctx, N_(N__INSTALL_PAWNIO)))
		return;
	if (!WR0_InstallPawnIO())
	{
		MessageBoxW(g_ctx.wnd, L"Failed to install PawnIO", L"Error", MB_ICONERROR | MB_OK);
		return;
	}
	NWLC->NwDrv = WR0_OpenDriver();
}

VOID
gnwinfo_draw_about_window(struct nk_context* ctx, float width, float height)
{
	if (!(g_ctx.window_flag & GUI_WINDOW_ABOUT))
		return;
	if (!nk_begin_ex(ctx, N_(N__ABOUT),
		nk_rect(width / 4.0f, height / 3.0f, width / 2.0f, height / 4.0f),
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
	if (NWLC->NwDrv)
		nk_l(ctx, NWL_Ucs2ToUtf8(NWLC->NwDrv->id), NK_TEXT_CENTERED);
	else
	{
		nk_layout_row_dynamic(ctx, 0, 3);
		nk_spacer(ctx);
		draw_install_pawnio_button(ctx);
		nk_spacer(ctx);
	}
out:
	nk_end(ctx);
}
