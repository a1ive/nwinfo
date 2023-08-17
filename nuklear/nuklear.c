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

GdipFont*
nk_gdip_load_font(LPCWSTR name, int size, WORD fallback)
{
	GdipFont* font = (GdipFont*)calloc(1, sizeof(GdipFont));
	GpFontFamily* family;

	if (!font)
		goto fail;

	if (GdipCreateFontFamilyFromName(name, NULL, &family))
	{
		free(font);
		HRSRC resinfo = FindResourceW(NULL, MAKEINTRESOURCEW(fallback), RT_FONT);
		if (!resinfo)
			goto fail;
		HGLOBAL res = LoadResource(NULL, resinfo);
		if (!res)
			goto fail;
		return nk_gdipfont_create_from_memory(LockResource(res), SizeofResource(NULL, resinfo), size);
	}
	else
	{
		GdipCreateFont(family, (REAL)size, FontStyleRegular, UnitPixel, &font->handle);
		GdipDeleteFontFamily(family);
	}

	return font;
fail:
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
