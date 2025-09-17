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

static nk_bool
nk_panel_begin_ex(struct nk_context* ctx, const char* title, enum nk_panel_type panel_type,
	struct nk_image img_icon, struct nk_image img_close)
{
	struct nk_input* in;
	struct nk_window* win;
	struct nk_panel* layout;
	struct nk_command_buffer* out;
	const struct nk_style* style;
	const struct nk_user_font* font;

	struct nk_vec2 scrollbar_size;
	struct nk_vec2 panel_padding;

	NK_ASSERT(ctx);
	NK_ASSERT(ctx->current);
	NK_ASSERT(ctx->current->layout);
	if (!ctx || !ctx->current || !ctx->current->layout) return 0;
	nk_zero(ctx->current->layout, sizeof(*ctx->current->layout));
	if ((ctx->current->flags & NK_WINDOW_HIDDEN) || (ctx->current->flags & NK_WINDOW_CLOSED)) {
		nk_zero(ctx->current->layout, sizeof(struct nk_panel));
		ctx->current->layout->type = panel_type;
		return 0;
	}
	/* pull state into local stack */
	style = &ctx->style;
	font = style->font;
	win = ctx->current;
	layout = win->layout;
	out = &win->buffer;
	in = (win->flags & NK_WINDOW_NO_INPUT) ? 0 : &ctx->input;
#ifdef NK_INCLUDE_COMMAND_USERDATA
	win->buffer.userdata = ctx->userdata;
#endif
	/* pull style configuration into local stack */
	scrollbar_size = style->window.scrollbar_size;
	panel_padding = nk_panel_get_padding(style, panel_type);

	/* window movement */
	if ((win->flags & NK_WINDOW_MOVABLE) && !(win->flags & NK_WINDOW_ROM)) {
		nk_bool left_mouse_down;
		unsigned int left_mouse_clicked;
		int left_mouse_click_in_cursor;

		/* calculate draggable window space */
		struct nk_rect header;
		header.x = win->bounds.x;
		header.y = win->bounds.y;
		header.w = win->bounds.w;
		if (nk_panel_has_header(win->flags, title)) {
			header.h = font->height + 2.0f * style->window.header.padding.y;
			header.h += 2.0f * style->window.header.label_padding.y;
		}
		else header.h = panel_padding.y;

		/* window movement by dragging */
		left_mouse_down = in->mouse.buttons[NK_BUTTON_LEFT].down;
		left_mouse_clicked = in->mouse.buttons[NK_BUTTON_LEFT].clicked;
		left_mouse_click_in_cursor = nk_input_has_mouse_click_down_in_rect(in,
			NK_BUTTON_LEFT, header, nk_true);
		if (left_mouse_down && left_mouse_click_in_cursor && !left_mouse_clicked) {
			win->bounds.x = win->bounds.x + in->mouse.delta.x;
			win->bounds.y = win->bounds.y + in->mouse.delta.y;
			in->mouse.buttons[NK_BUTTON_LEFT].clicked_pos.x += in->mouse.delta.x;
			in->mouse.buttons[NK_BUTTON_LEFT].clicked_pos.y += in->mouse.delta.y;
			ctx->style.cursor_active = ctx->style.cursors[NK_CURSOR_MOVE];
		}
	}

	/* setup panel */
	layout->type = panel_type;
	layout->flags = win->flags;
	layout->bounds = win->bounds;
	layout->bounds.x += panel_padding.x;
	layout->bounds.w -= 2 * panel_padding.x;
	if (win->flags & NK_WINDOW_BORDER) {
		layout->border = nk_panel_get_border(style, win->flags, panel_type);
		layout->bounds = nk_shrink_rect(layout->bounds, layout->border);
	}
	else layout->border = 0;
	layout->at_y = layout->bounds.y;
	layout->at_x = layout->bounds.x;
	layout->max_x = 0;
	layout->header_height = 0;
	layout->footer_height = 0;
	nk_layout_reset_min_row_height(ctx);
	layout->row.index = 0;
	layout->row.columns = 0;
	layout->row.ratio = 0;
	layout->row.item_width = 0;
	layout->row.tree_depth = 0;
	layout->row.height = panel_padding.y;
	layout->has_scrolling = nk_true;
	if (!(win->flags & NK_WINDOW_NO_SCROLLBAR))
		layout->bounds.w -= scrollbar_size.x;
	if (!nk_panel_is_nonblock(panel_type)) {
		layout->footer_height = 0;
		if (!(win->flags & NK_WINDOW_NO_SCROLLBAR) || win->flags & NK_WINDOW_SCALABLE)
			layout->footer_height = scrollbar_size.y;
		layout->bounds.h -= layout->footer_height;
	}

	/* panel header */
	if (nk_panel_has_header(win->flags, title))
	{
		struct nk_text text;
		struct nk_rect header;
		const struct nk_style_item* background = 0;

		/* calculate header bounds */
		header.x = win->bounds.x;
		header.y = win->bounds.y;
		header.w = win->bounds.w;
		header.h = font->height + 2.0f * style->window.header.padding.y;
		header.h += (2.0f * style->window.header.label_padding.y);

		/* shrink panel by header */
		layout->header_height = header.h;
		layout->bounds.y += header.h;
		layout->bounds.h -= header.h;
		layout->at_y += header.h;

		/* select correct header background and text color */
		if (ctx->active == win) {
			background = &style->window.header.active;
			text.text = style->window.header.label_active;
		}
		else if (nk_input_is_mouse_hovering_rect(&ctx->input, header)) {
			background = &style->window.header.hover;
			text.text = style->window.header.label_hover;
		}
		else {
			background = &style->window.header.normal;
			text.text = style->window.header.label_normal;
		}

		/* draw header background */
		header.h += 1.0f;

		switch (background->type) {
		case NK_STYLE_ITEM_IMAGE:
			text.background = nk_rgba(0, 0, 0, 0);
			nk_draw_image(&win->buffer, header, &background->data.image, nk_white);
			break;
		case NK_STYLE_ITEM_NINE_SLICE:
			text.background = nk_rgba(0, 0, 0, 0);
			nk_draw_nine_slice(&win->buffer, header, &background->data.slice, nk_white);
			break;
		case NK_STYLE_ITEM_COLOR:
			text.background = background->data.color;
			nk_fill_rect(out, header, 0, background->data.color);
			break;
		}

		/* window close button */
		{
			struct nk_rect button;
			button.y = header.y + style->window.header.padding.y;
			button.h = header.h - 2 * style->window.header.padding.y;
			button.w = button.h;
			if (win->flags & NK_WINDOW_CLOSABLE) {
				nk_flags ws = 0;
				if (style->window.header.align == NK_HEADER_RIGHT) {
					button.x = (header.w + header.x) - (button.w + style->window.header.padding.x);
					header.w -= button.w + style->window.header.spacing.x + style->window.header.padding.x;
				}
				else {
					button.x = header.x + style->window.header.padding.x;
					header.x += button.w + style->window.header.spacing.x + style->window.header.padding.x;
				}

				if (nk_do_button_image(&ws, &win->buffer, button,
					img_close, NK_BUTTON_DEFAULT,
					&style->window.header.close_button, in) && !(win->flags & NK_WINDOW_ROM))
				{
					layout->flags |= NK_WINDOW_HIDDEN;
					layout->flags &= (nk_flags)~NK_WINDOW_MINIMIZED;
				}
			}

			/* window minimize button */
			if (win->flags & NK_WINDOW_MINIMIZABLE) {
				nk_flags ws = 0;
				if (style->window.header.align == NK_HEADER_RIGHT) {
					button.x = (header.w + header.x) - button.w;
					if (!(win->flags & NK_WINDOW_CLOSABLE)) {
						button.x -= style->window.header.padding.x;
						header.w -= style->window.header.padding.x;
					}
					header.w -= button.w + style->window.header.spacing.x;
				}
				else {
					button.x = header.x;
					header.x += button.w + style->window.header.spacing.x + style->window.header.padding.x;
				}
				if (nk_do_button_image(&ws, &win->buffer, button, (layout->flags & NK_WINDOW_MINIMIZED) ?
					img_close : img_close, // TODO: use minimize icon
					NK_BUTTON_DEFAULT, &style->window.header.minimize_button, in) && !(win->flags & NK_WINDOW_ROM))
					layout->flags = (layout->flags & NK_WINDOW_MINIMIZED) ?
					layout->flags & (nk_flags)~NK_WINDOW_MINIMIZED :
					layout->flags | NK_WINDOW_MINIMIZED;
			}
		}

		{
			/* window header title and icon */
			int text_len = nk_strlen(title);
			struct nk_rect label = { 0,0,0,0 };
			struct nk_rect icon;
			float t = font->width(font->userdata, font->height, title, text_len);
			text.padding = nk_vec2(0, 0);

			label.x = header.x + style->window.header.padding.x;
			label.x += style->window.header.label_padding.x;
			label.y = header.y + style->window.header.label_padding.y;
			label.h = font->height + 2 * style->window.header.label_padding.y;
			label.w = t + 2 * style->window.header.spacing.x;
			label.w = NK_CLAMP(0, label.w, header.x + header.w - label.x);

			if (img_icon.handle.id != 0)
			{
				icon.w = icon.h = label.h;
				icon.x = label.x;
				icon.y = label.y;
				nk_draw_image(out, icon, &img_icon, nk_white);
				label.x += icon.w + style->window.header.padding.x;
			}
			nk_widget_text(out, label, (const char*)title, text_len, &text, NK_TEXT_LEFT, font);
		}
	}

	/* draw window background */
	if (!(layout->flags & NK_WINDOW_MINIMIZED) && !(layout->flags & NK_WINDOW_DYNAMIC)) {
		struct nk_rect body;
		body.x = win->bounds.x;
		body.w = win->bounds.w;
		body.y = (win->bounds.y + layout->header_height);
		body.h = (win->bounds.h - layout->header_height);

		switch (style->window.fixed_background.type) {
		case NK_STYLE_ITEM_IMAGE:
			nk_draw_image(out, body, &style->window.fixed_background.data.image, nk_white);
			break;
		case NK_STYLE_ITEM_NINE_SLICE:
			nk_draw_nine_slice(out, body, &style->window.fixed_background.data.slice, nk_white);
			break;
		case NK_STYLE_ITEM_COLOR:
			nk_fill_rect(out, body, style->window.rounding, style->window.fixed_background.data.color);
			break;
		}
	}

	/* set clipping rectangle */
	{
		struct nk_rect clip;
		layout->clip = layout->bounds;
		nk_unify(&clip, &win->buffer.clip, layout->clip.x, layout->clip.y,
			layout->clip.x + layout->clip.w, layout->clip.y + layout->clip.h);
		nk_push_scissor(out, clip);
		layout->clip = clip;
	}
	return !(layout->flags & NK_WINDOW_HIDDEN) && !(layout->flags & NK_WINDOW_MINIMIZED);
}

nk_bool
nk_begin_ex(struct nk_context* ctx, const char* title,
	struct nk_rect bounds, nk_flags flags, struct nk_image img_icon, struct nk_image img_close)
{
	struct nk_window* win;
	struct nk_style* style;
	nk_hash name_hash;
	int name_len;
	int ret = 0;

	NK_ASSERT(ctx);
	NK_ASSERT(title);
	NK_ASSERT(ctx->style.font && ctx->style.font->width && "if this triggers you forgot to add a font");
	NK_ASSERT(!ctx->current && "if this triggers you missed a `nk_end` call");
	if (!ctx || ctx->current || !title)
		return 0;

	/* find or create window */
	style = &ctx->style;
	name_len = (int)nk_strlen(title);
	name_hash = nk_murmur_hash(title, (int)name_len, NK_WINDOW_TITLE);
	win = nk_find_window(ctx, name_hash, title);
	if (!win) {
		/* create new window */
		nk_size name_length = (nk_size)name_len;
		win = (struct nk_window*)nk_create_window(ctx);
		NK_ASSERT(win);
		if (!win) return 0;

		if (flags & NK_WINDOW_BACKGROUND)
			nk_insert_window(ctx, win, NK_INSERT_FRONT);
		else nk_insert_window(ctx, win, NK_INSERT_BACK);
		nk_command_buffer_init(&win->buffer, &ctx->memory, NK_CLIPPING_ON);

		win->flags = flags;
		win->bounds = bounds;
		win->name = name_hash;
		name_length = NK_MIN(name_length, NK_WINDOW_MAX_NAME - 1);
		NK_MEMCPY(win->name_string, title, name_length);
		win->name_string[name_length] = 0;
		win->popup.win = 0;
		win->widgets_disabled = nk_false;
		if (!ctx->active)
			ctx->active = win;
	}
	else {
		/* update window */
		win->flags &= ~(nk_flags)(NK_WINDOW_PRIVATE - 1);
		win->flags |= flags;
		if (!(win->flags & (NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)))
			win->bounds = bounds;
		/* If this assert triggers you either:
		 *
		 * I.) Have more than one window with the same name or
		 * II.) You forgot to actually draw the window.
		 *      More specific you did not call `nk_clear` (nk_clear will be
		 *      automatically called for you if you are using one of the
		 *      provided demo backends). */
		NK_ASSERT(win->seq != ctx->seq);
		win->seq = ctx->seq;
		if (!ctx->active && !(win->flags & NK_WINDOW_HIDDEN)) {
			ctx->active = win;
			ctx->end = win;
		}
	}
	if (win->flags & NK_WINDOW_HIDDEN) {
		ctx->current = win;
		win->layout = 0;
		return 0;
	}
	else nk_start(ctx, win);

	/* window overlapping */
	if (!(win->flags & NK_WINDOW_HIDDEN) && !(win->flags & NK_WINDOW_NO_INPUT))
	{
		int inpanel, ishovered;
		struct nk_window* iter = win;
		float h = ctx->style.font->height + 2.0f * style->window.header.padding.y +
			(2.0f * style->window.header.label_padding.y);
		struct nk_rect win_bounds = (!(win->flags & NK_WINDOW_MINIMIZED)) ?
			win->bounds : nk_rect(win->bounds.x, win->bounds.y, win->bounds.w, h);

		/* activate window if hovered and no other window is overlapping this window */
		inpanel = nk_input_has_mouse_click_down_in_rect(&ctx->input, NK_BUTTON_LEFT, win_bounds, nk_true);
		inpanel = inpanel && ctx->input.mouse.buttons[NK_BUTTON_LEFT].clicked;
		ishovered = nk_input_is_mouse_hovering_rect(&ctx->input, win_bounds);
		if ((win != ctx->active) && ishovered && !ctx->input.mouse.buttons[NK_BUTTON_LEFT].down) {
			iter = win->next;
			while (iter) {
				struct nk_rect iter_bounds = (!(iter->flags & NK_WINDOW_MINIMIZED)) ?
					iter->bounds : nk_rect(iter->bounds.x, iter->bounds.y, iter->bounds.w, h);
				if (NK_INTERSECT(win_bounds.x, win_bounds.y, win_bounds.w, win_bounds.h,
					iter_bounds.x, iter_bounds.y, iter_bounds.w, iter_bounds.h) &&
					(!(iter->flags & NK_WINDOW_HIDDEN)))
					break;

				if (iter->popup.win && iter->popup.active && !(iter->flags & NK_WINDOW_HIDDEN) &&
					NK_INTERSECT(win->bounds.x, win_bounds.y, win_bounds.w, win_bounds.h,
						iter->popup.win->bounds.x, iter->popup.win->bounds.y,
						iter->popup.win->bounds.w, iter->popup.win->bounds.h))
					break;
				iter = iter->next;
			}
		}

		/* activate window if clicked */
		if (iter && inpanel && (win != ctx->end)) {
			iter = win->next;
			while (iter) {
				/* try to find a panel with higher priority in the same position */
				struct nk_rect iter_bounds = (!(iter->flags & NK_WINDOW_MINIMIZED)) ?
					iter->bounds : nk_rect(iter->bounds.x, iter->bounds.y, iter->bounds.w, h);
				if (NK_INBOX(ctx->input.mouse.pos.x, ctx->input.mouse.pos.y,
					iter_bounds.x, iter_bounds.y, iter_bounds.w, iter_bounds.h) &&
					!(iter->flags & NK_WINDOW_HIDDEN))
					break;
				if (iter->popup.win && iter->popup.active && !(iter->flags & NK_WINDOW_HIDDEN) &&
					NK_INTERSECT(win_bounds.x, win_bounds.y, win_bounds.w, win_bounds.h,
						iter->popup.win->bounds.x, iter->popup.win->bounds.y,
						iter->popup.win->bounds.w, iter->popup.win->bounds.h))
					break;
				iter = iter->next;
			}
		}
		if (iter && !(win->flags & NK_WINDOW_ROM) && (win->flags & NK_WINDOW_BACKGROUND)) {
			win->flags |= (nk_flags)NK_WINDOW_ROM;
			iter->flags &= ~(nk_flags)NK_WINDOW_ROM;
			ctx->active = iter;
			if (!(iter->flags & NK_WINDOW_BACKGROUND)) {
				/* current window is active in that position so transfer to top
				 * at the highest priority in stack */
				nk_remove_window(ctx, iter);
				nk_insert_window(ctx, iter, NK_INSERT_BACK);
			}
		}
		else {
			if (!iter && ctx->end != win) {
				if (!(win->flags & NK_WINDOW_BACKGROUND)) {
					/* current window is active in that position so transfer to top
					 * at the highest priority in stack */
					nk_remove_window(ctx, win);
					nk_insert_window(ctx, win, NK_INSERT_BACK);
				}
				win->flags &= ~(nk_flags)NK_WINDOW_ROM;
				ctx->active = win;
			}
			if (ctx->end != win && !(win->flags & NK_WINDOW_BACKGROUND))
				win->flags |= NK_WINDOW_ROM;
		}
	}
	win->layout = (struct nk_panel*)nk_create_panel(ctx);
	ctx->current = win;
	ret = nk_panel_begin_ex(ctx, title, NK_PANEL_WINDOW, img_icon, img_close);
	win->layout->offset_x = &win->scrollbar.x;
	win->layout->offset_y = &win->scrollbar.y;
	return ret;
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

	if (nk_input_is_mouse_hovering_rect(&ctx->input, bounds))
	{
		if (hover)
			nk_hover_colored(ctx, str, text_len, color);

		if (nk_input_is_key_pressed(&ctx->input, NK_KEY_COPY))
			ctx->clip.copy(ctx->clip.userdata, str, text_len);
	}
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
