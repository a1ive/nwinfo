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
