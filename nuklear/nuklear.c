// SPDX-License-Identifier: Unlicense

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR

#pragma warning(disable:4996)
#pragma warning(disable:4116)

#include <assert.h>
#define NK_ASSERT(expr) assert(expr)

#define NK_IMPLEMENTATION
#include <nuklear.h>

#pragma warning(disable:4244)
#pragma comment(lib, "gdiplus.lib")

#define NK_GDIP_IMPLEMENTATION
#include <nuklear_gdip.h>

#include <VersionHelpers.h>

GdipFont*
nk_gdip_load_font(LPCWSTR name, int size)
{
	GdipFont* font = (GdipFont*)calloc(1, sizeof(GdipFont));
	GpFontFamily* family;

	if (!font)
		goto fail;

	if (GdipCreateFontFamilyFromName(name, NULL, &family))
	{
		UINT len = IsWindowsVistaOrGreater() ? sizeof(NONCLIENTMETRICSW) : sizeof(NONCLIENTMETRICSW) - sizeof(int);
		NONCLIENTMETRICSW metrics = { .cbSize = len };
		if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, len, &metrics, 0))
		{
			if (GdipCreateFontFamilyFromName(metrics.lfMessageFont.lfFaceName, NULL, &family))
				goto fail;
		}
		else
			goto fail;
	}

	GdipCreateFont(family, (REAL)size, FontStyleRegular, UnitPixel, &font->handle);
	GdipDeleteFontFamily(family);

	return font;
fail:
	MessageBoxW(NULL, L"Failed to load font", L"Error", MB_OK);
	exit(1);
}

void
nk_image_label(struct nk_context* ctx, struct nk_image img,
	const char* str, nk_flags align, struct nk_color color)
{
	struct nk_window* win;
	const struct nk_style* style;
	struct nk_rect bounds;
	struct nk_rect icon;
	struct nk_text text;
	int len;

	NK_ASSERT(ctx);
	NK_ASSERT(ctx->current);
	NK_ASSERT(ctx->current->layout);
	if (!ctx || !ctx->current || !ctx->current->layout) return;

	win = ctx->current;
	style = &ctx->style;
	len = nk_strlen(str);
	if (!nk_widget(&bounds, ctx))
		return;

	icon.w = icon.h = bounds.h;
	icon.x = bounds.x;
	icon.y = bounds.y;

	nk_draw_image(&win->buffer, icon, &img, nk_white);

	bounds.x = icon.x + icon.w + style->window.padding.x + style->window.border;
	bounds.w -= icon.w + style->window.padding.x + style->window.border;

	text.padding.x = style->text.padding.x;
	text.padding.y = style->text.padding.y;
	text.background = style->window.background;
	text.text = color;
	nk_widget_text(&win->buffer, bounds, str, len, &text, align, style->font);
}

void
nk_space_label(struct nk_context* ctx, const char* str, nk_flags align)
{
	struct nk_window* win;
	const struct nk_style* style;
	struct nk_rect bounds;
	struct nk_text text;
	int len;

	NK_ASSERT(ctx);
	NK_ASSERT(ctx->current);
	NK_ASSERT(ctx->current->layout);
	if (!ctx || !ctx->current || !ctx->current->layout) return;

	win = ctx->current;
	style = &ctx->style;
	len = nk_strlen(str);
	if (!nk_widget(&bounds, ctx))
		return;

	bounds.x += bounds.h + style->window.padding.x + style->window.border;
	bounds.w -= bounds.h + style->window.padding.x + style->window.border;

	text.padding.x = style->text.padding.x;
	text.padding.y = style->text.padding.y;
	text.background = style->window.background;
	text.text = style->text.color;
	nk_widget_text(&win->buffer, bounds, str, len, &text, align, style->font);
}

nk_bool
nk_combo_begin_color_dynamic(struct nk_context* ctx, struct nk_color color)
{
	struct nk_window* win;
	struct nk_style* style;
	const struct nk_input* in;
	struct nk_vec2 size;

	struct nk_rect header;
	int is_clicked = nk_false;
	enum nk_widget_layout_states s;
	const struct nk_style_item* background;

	NK_ASSERT(ctx);
	NK_ASSERT(ctx->current);
	NK_ASSERT(ctx->current->layout);
	if (!ctx || !ctx->current || !ctx->current->layout)
		return 0;

	win = ctx->current;
	style = &ctx->style;
	s = nk_widget(&header, ctx);
	if (s == NK_WIDGET_INVALID)
		return 0;

	size.x = header.w;
	size.y = header.h * 4.0f;

	in = (win->layout->flags & NK_WINDOW_ROM || s == NK_WIDGET_ROM) ? 0 : &ctx->input;
	if (nk_button_behavior(&ctx->last_widget_state, header, in, NK_BUTTON_DEFAULT))
		is_clicked = nk_true;

	/* draw combo box header background and border */
	if (ctx->last_widget_state & NK_WIDGET_STATE_ACTIVED)
		background = &style->combo.active;
	else if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER)
		background = &style->combo.hover;
	else background = &style->combo.normal;

	switch (background->type) {
	case NK_STYLE_ITEM_IMAGE:
		nk_draw_image(&win->buffer, header, &background->data.image, nk_white);
		break;
	case NK_STYLE_ITEM_NINE_SLICE:
		nk_draw_nine_slice(&win->buffer, header, &background->data.slice, nk_white);
		break;
	case NK_STYLE_ITEM_COLOR:
		nk_fill_rect(&win->buffer, header, style->combo.rounding, background->data.color);
		nk_stroke_rect(&win->buffer, header, style->combo.rounding, style->combo.border, style->combo.border_color);
		break;
	}
	{
		struct nk_rect content;
		struct nk_rect button;
		struct nk_rect bounds;
		int draw_button_symbol;

		enum nk_symbol_type sym;
		if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER)
			sym = style->combo.sym_hover;
		else if (is_clicked)
			sym = style->combo.sym_active;
		else sym = style->combo.sym_normal;

		/* represents whether or not the combo's button symbol should be drawn */
		draw_button_symbol = sym != NK_SYMBOL_NONE;

		/* calculate button */
		button.w = header.h - 2 * style->combo.button_padding.y;
		button.x = (header.x + header.w - header.h) - style->combo.button_padding.x;
		button.y = header.y + style->combo.button_padding.y;
		button.h = button.w;

		content.x = button.x + style->combo.button.padding.x;
		content.y = button.y + style->combo.button.padding.y;
		content.w = button.w - 2 * style->combo.button.padding.x;
		content.h = button.h - 2 * style->combo.button.padding.y;

		/* draw color */
		bounds.h = header.h - 4 * style->combo.content_padding.y;
		bounds.y = header.y + 2 * style->combo.content_padding.y;
		bounds.x = header.x + 2 * style->combo.content_padding.x;
		if (draw_button_symbol)
			bounds.w = (button.x - (style->combo.content_padding.x + style->combo.spacing.x)) - bounds.x;
		else
			bounds.w = header.w - 4 * style->combo.content_padding.x;
		nk_fill_rect(&win->buffer, bounds, 0, color);

		/* draw open/close button */
		if (draw_button_symbol)
			nk_draw_button_symbol(&win->buffer, &button, &content, ctx->last_widget_state,
				&ctx->style.combo.button, sym, style->font);
	}
	return nk_combo_begin(ctx, win, size, is_clicked, header);
}
