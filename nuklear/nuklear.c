// SPDX-License-Identifier: Unlicense

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR

#pragma warning(disable:4996)
#pragma warning(disable:4116)
#pragma warning(disable:4244)

#include <assert.h>
#include <math.h>
#define NK_ASSERT(expr) assert(expr)

#define NK_MEMSET memset
#define NK_MEMCPY memcpy
#define NK_SIN sinf
#define NK_COS cosf
#define NK_STRTOD strtod

#define NK_IMPLEMENTATION
#include <nuklear.h>

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

static inline nk_bool
nk_hover_begin(struct nk_context* ctx, float width)
{
	int x, y, w, h;
	struct nk_window* win;
	const struct nk_input* in;
	struct nk_rect bounds;
	int ret;

	/* make sure that no nonblocking popup is currently active */
	win = ctx->current;
	in = &ctx->input;

	w = nk_iceilf(width);
	h = nk_iceilf(nk_null_rect.h);
	x = nk_ifloorf(in->mouse.pos.x + 1) - (int)win->layout->clip.x;
	if (w + x > (int)win->bounds.w) x = (int)win->bounds.w - w;
	y = nk_ifloorf(in->mouse.pos.y + 1) - (int)win->layout->clip.y;

	bounds.x = (float)x;
	bounds.y = (float)y;
	bounds.w = (float)w;
	bounds.h = (float)h;

	ret = nk_popup_begin(ctx, NK_POPUP_DYNAMIC,
		"__##Tooltip##__", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER, bounds);
	if (ret) win->layout->flags &= ~(nk_flags)NK_WINDOW_ROM;
	win->popup.type = NK_PANEL_TOOLTIP;
	ctx->current->layout->type = NK_PANEL_TOOLTIP;
	return ret;
}

static inline void
nk_hover_end(struct nk_context* ctx)
{
	ctx->current->seq--;
	nk_popup_close(ctx);
	nk_popup_end(ctx);
}

static void
nk_hover_colored(struct nk_context* ctx, const char* text, int text_len, struct nk_color color)
{
	if (ctx->current == ctx->active // only show tooltip if the window is active
		// make sure that no nonblocking popup is currently active
		&& !(ctx->current->popup.win && ((int)ctx->current->popup.type & (int)NK_PANEL_SET_NONBLOCK)))
	{
		/* calculate size of the text and tooltip */
		float text_width = ctx->style.font->width(ctx->style.font->userdata, ctx->style.font->height, text, text_len)
			+ (4 * ctx->style.window.padding.x);
		float text_height = (ctx->style.font->height + 2 * ctx->style.window.padding.y);

		/* execute tooltip and fill with text */
		if (nk_hover_begin(ctx, text_width))
		{
			nk_layout_row_dynamic(ctx, text_height, 1);
			nk_text_colored(ctx, text, text_len, NK_TEXT_LEFT, color);
			nk_hover_end(ctx);
		}
	}
}

void
nk_lhsc(struct nk_context* ctx, const char* str,
	nk_flags alignment, struct nk_color color, nk_bool hover, nk_bool space)
{
	struct nk_window* win;
	const struct nk_style* style;
	int text_len;
	struct nk_rect bounds;
	struct nk_text text;

	NK_ASSERT(ctx);
	NK_ASSERT(ctx->current);
	NK_ASSERT(ctx->current->layout);
	if (!ctx || !ctx->current || !ctx->current->layout) return;

	win = ctx->current;
	style = &ctx->style;
	if (space)
	{
		if (!nk_widget(&bounds, ctx))
			return;
		bounds.x += bounds.h + style->window.padding.x + style->window.border;
		bounds.w -= bounds.h + style->window.padding.x + style->window.border;
	}
	else
	{
		nk_panel_alloc_space(&bounds, ctx);
	}

	text_len = nk_strlen(str);
	text.padding.x = style->text.padding.x;
	text.padding.y = style->text.padding.y;
	text.background = style->window.background;
	text.text = color;
	nk_widget_text(&win->buffer, bounds, str, text_len, &text, alignment, style->font);

	if (hover && nk_input_is_mouse_hovering_rect(&ctx->input, bounds))
		nk_hover_colored(ctx, str, text_len, color);
}

static CHAR m_lhscfv_buf[MAX_PATH + 1];
static void
nk_lhscfv(struct nk_context* ctx, nk_flags alignment, struct nk_color color, nk_bool hover, nk_bool space, const char* fmt, va_list args)
{
	memset(m_lhscfv_buf, 0, MAX_PATH);
	vsnprintf(m_lhscfv_buf, MAX_PATH, fmt, args);
	nk_lhsc(ctx, m_lhscfv_buf, alignment, color, hover, space);
}

void
nk_lhscf(struct nk_context* ctx, nk_flags alignment, struct nk_color color, nk_bool hover, nk_bool space, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	nk_lhscfv(ctx, alignment, color, hover, space, fmt, args);
	va_end(args);
}

void
nk_l(struct nk_context* ctx, const char* str, nk_flags alignment)
{
	NK_ASSERT(ctx);
	if (!ctx) return;
	nk_lhsc(ctx, str, alignment, ctx->style.text.color, nk_false, nk_false);
}

void
nk_lhc(struct nk_context* ctx, const char* str, nk_flags align,
	struct nk_color color)
{
	nk_lhsc(ctx, str, align, color, nk_true, nk_false);
}

void
nk_lf(struct nk_context* ctx, nk_flags flags, const char* fmt, ...)
{
	NK_ASSERT(ctx);
	if (!ctx) return;
	va_list args;
	va_start(args, fmt);
	nk_lhscfv(ctx, flags, ctx->style.text.color,nk_false, nk_false, fmt, args);
	va_end(args);
}

void
nk_lhcf(struct nk_context* ctx, nk_flags flags,
	struct nk_color color, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	nk_lhscfv(ctx, flags, color, nk_true, nk_false, fmt, args);
	va_end(args);
}

nk_bool
nk_button_image_hover(struct nk_context* ctx, struct nk_image img, const char* str)
{
	struct nk_window* win;
	struct nk_panel* layout;
	const struct nk_input* in;

	struct nk_rect bounds;
	enum nk_widget_layout_states state;

	NK_ASSERT(ctx);
	NK_ASSERT(ctx->current);
	NK_ASSERT(ctx->current->layout);
	if (!ctx || !ctx->current || !ctx->current->layout)
		return 0;

	win = ctx->current;
	layout = win->layout;

	state = nk_widget(&bounds, ctx);
	if (!state) return 0;
	in = (state == NK_WIDGET_ROM || layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;

	if (nk_input_is_mouse_hovering_rect(in, bounds) && str)
		nk_hover_colored(ctx, str, nk_strlen(str), ctx->style.text.color);

	return nk_do_button_image(&ctx->last_widget_state, &win->buffer, bounds,
		img, ctx->button_behavior, &ctx->style.button, in);
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

void
nk_block(struct nk_context* ctx, struct nk_color color, const char* str)
{
	static struct nk_style_button style;
	static struct nk_color inv;
	nk_zero_struct(style);
	style.color_factor_text = 1.0f;
	style.color_factor_background = 1.0f;
	style.normal = nk_style_item_color(color);
	style.hover = nk_style_item_color(color);
	style.active = nk_style_item_color(color);
	style.text_background = color;
	if ((uint32_t)color.r + color.g + color.b >=  382)
		inv = nk_rgb(0, 0, 0);
	else
		inv = nk_rgb(255, 255, 255);
	style.text_normal = inv;
	style.text_hover = inv;
	style.text_active = inv;
	style.text_alignment = NK_TEXT_CENTERED;
	nk_button_label_styled(ctx, &style, str);
}
