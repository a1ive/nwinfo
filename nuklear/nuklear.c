// SPDX-License-Identifier: Unlicense

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR

#pragma warning(disable:4116)
#pragma warning(disable:4244)

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")

#define NUKLEAR_C_INCLUDE
#define NK_IMPLEMENTATION
#define NK_GDIP_IMPLEMENTATION

#define _USE_MATH_DEFINES
#include <math.h>
#define NK_VSNPRINTF(s,n,f,a) vsnprintf(s,f,a)
#define NK_MEMSET memset
#define NK_MEMCPY memcpy
#define NK_SIN sinf
#define NK_COS cosf
#define NK_STRTOD strtod
#include "../gnwinfo/gnwinfo.h"

#include <VersionHelpers.h>

#include <stdlib.h>
#include <malloc.h>

/* manually declare everything GDI+ needs, because
   GDI+ headers are not usable from C */
#define WINGDIPAPI __stdcall
#define GDIPCONST const

typedef struct GpGraphics GpGraphics;
typedef struct GpImage GpImage;
typedef struct GpPen GpPen;
typedef struct GpBrush GpBrush;
typedef struct GpStringFormat GpStringFormat;
typedef struct GpFont GpFont;
typedef struct GpFontFamily GpFontFamily;
typedef struct GpFontCollection GpFontCollection;

typedef GpImage GpBitmap;
typedef GpBrush GpSolidFill;

typedef int Status;
typedef Status GpStatus;

typedef float REAL;
typedef DWORD ARGB;
typedef POINT GpPoint;

typedef enum
{
	TextRenderingHintSystemDefault = 0,
	TextRenderingHintSingleBitPerPixelGridFit = 1,
	TextRenderingHintSingleBitPerPixel = 2,
	TextRenderingHintAntiAliasGridFit = 3,
	TextRenderingHintAntiAlias = 4,
	TextRenderingHintClearTypeGridFit = 5
} TextRenderingHint;

typedef enum
{
	StringFormatFlagsDirectionRightToLeft = 0x00000001,
	StringFormatFlagsDirectionVertical = 0x00000002,
	StringFormatFlagsNoFitBlackBox = 0x00000004,
	StringFormatFlagsDisplayFormatControl = 0x00000020,
	StringFormatFlagsNoFontFallback = 0x00000400,
	StringFormatFlagsMeasureTrailingSpaces = 0x00000800,
	StringFormatFlagsNoWrap = 0x00001000,
	StringFormatFlagsLineLimit = 0x00002000,
	StringFormatFlagsNoClip = 0x00004000
} StringFormatFlags;

typedef enum
{
	QualityModeInvalid = -1,
	QualityModeDefault = 0,
	QualityModeLow = 1,
	QualityModeHigh = 2
} QualityMode;

typedef enum
{
	SmoothingModeInvalid = QualityModeInvalid,
	SmoothingModeDefault = QualityModeDefault,
	SmoothingModeHighSpeed = QualityModeLow,
	SmoothingModeHighQuality = QualityModeHigh,
	SmoothingModeNone,
	SmoothingModeAntiAlias,
	SmoothingModeAntiAlias8x4 = SmoothingModeAntiAlias,
	SmoothingModeAntiAlias8x8
} SmoothingMode;

typedef enum
{
	FontStyleRegular = 0,
	FontStyleBold = 1,
	FontStyleItalic = 2,
	FontStyleBoldItalic = 3,
	FontStyleUnderline = 4,
	FontStyleStrikeout = 8
} FontStyle;

typedef enum
{
	FillModeAlternate,
	FillModeWinding
} FillMode;

typedef enum
{
	CombineModeReplace,
	CombineModeIntersect,
	CombineModeUnion,
	CombineModeXor,
	CombineModeExclude,
	CombineModeComplement
} CombineMode;

typedef enum
{
	UnitWorld,
	UnitDisplay,
	UnitPixel,
	UnitPoint,
	UnitInch,
	UnitDocument,
	UnitMillimeter
} Unit;

typedef struct
{
	FLOAT X;
	FLOAT Y;
	FLOAT Width;
	FLOAT Height;
} RectF;

typedef enum
{
	DebugEventLevelFatal,
	DebugEventLevelWarning
} DebugEventLevel;

typedef VOID(WINAPI* DebugEventProc)(DebugEventLevel level, CHAR* message);

typedef struct
{
	UINT32 GdiplusVersion;
	DebugEventProc DebugEventCallback;
	BOOL SuppressBackgroundThread;
	BOOL SuppressExternalCodecs;
} GdiplusStartupInput;

typedef Status(WINAPI* NotificationHookProc)(OUT ULONG_PTR* token);
typedef VOID(WINAPI* NotificationUnhookProc)(ULONG_PTR token);

typedef struct
{
	NotificationHookProc NotificationHook;
	NotificationUnhookProc NotificationUnhook;
} GdiplusStartupOutput;

/* startup & shutdown */

Status WINAPI GdiplusStartup(
	OUT ULONG_PTR* token,
	const GdiplusStartupInput* input,
	OUT GdiplusStartupOutput* output);

VOID WINAPI GdiplusShutdown(ULONG_PTR token);

/* image */

GpStatus WINGDIPAPI
GdipCreateBitmapFromGraphics(INT width,
	INT height,
	GpGraphics* target,
	GpBitmap** bitmap);

GpStatus WINGDIPAPI
GdipDisposeImage(GpImage* image);

GpStatus WINGDIPAPI
GdipGetImageGraphicsContext(GpImage* image, GpGraphics** graphics);

GpStatus WINGDIPAPI
GdipGetImageWidth(GpImage* image, UINT* width);

GpStatus WINGDIPAPI
GdipGetImageHeight(GpImage* image, UINT* height);

GpStatus WINGDIPAPI
GdipLoadImageFromFile(GDIPCONST WCHAR* filename, GpImage** image);

GpStatus WINGDIPAPI
GdipLoadImageFromStream(IStream* stream, GpImage** image);

/* pen */

GpStatus WINGDIPAPI
GdipCreatePen1(ARGB color, REAL width, Unit unit, GpPen** pen);

GpStatus WINGDIPAPI
GdipDeletePen(GpPen* pen);

GpStatus WINGDIPAPI
GdipSetPenWidth(GpPen* pen, REAL width);

GpStatus WINGDIPAPI
GdipSetPenColor(GpPen* pen, ARGB argb);

/* brush */

GpStatus WINGDIPAPI
GdipCreateSolidFill(ARGB color, GpSolidFill** brush);

GpStatus WINGDIPAPI
GdipDeleteBrush(GpBrush* brush);

GpStatus WINGDIPAPI
GdipSetSolidFillColor(GpSolidFill* brush, ARGB color);

/* font */

GpStatus WINGDIPAPI
GdipCreateFont(
	GDIPCONST GpFontFamily* fontFamily,
	REAL                 emSize,
	INT                  style,
	Unit                 unit,
	GpFont** font
);

GpStatus WINGDIPAPI
GdipDeleteFont(GpFont* font);

GpStatus WINGDIPAPI
GdipGetFontSize(GpFont* font, REAL* size);

GpStatus WINGDIPAPI
GdipCreateFontFamilyFromName(GDIPCONST WCHAR* name,
	GpFontCollection* fontCollection,
	GpFontFamily** fontFamily);

GpStatus WINGDIPAPI
GdipDeleteFontFamily(GpFontFamily* fontFamily);

GpStatus WINGDIPAPI
GdipStringFormatGetGenericTypographic(GpStringFormat** format);

GpStatus WINGDIPAPI
GdipSetStringFormatFlags(GpStringFormat* format, INT flags);

GpStatus WINGDIPAPI
GdipDeleteStringFormat(GpStringFormat* format);

GpStatus WINGDIPAPI
GdipPrivateAddMemoryFont(GpFontCollection* fontCollection,
	GDIPCONST void* memory, INT length);

GpStatus WINGDIPAPI
GdipPrivateAddFontFile(GpFontCollection* fontCollection,
	GDIPCONST WCHAR* filename);

GpStatus WINGDIPAPI
GdipNewPrivateFontCollection(GpFontCollection** fontCollection);

GpStatus WINGDIPAPI
GdipDeletePrivateFontCollection(GpFontCollection** fontCollection);

GpStatus WINGDIPAPI
GdipGetFontCollectionFamilyList(GpFontCollection* fontCollection,
	INT numSought, GpFontFamily* gpfamilies[], INT* numFound);

GpStatus WINGDIPAPI
GdipGetFontCollectionFamilyCount(GpFontCollection* fontCollection, INT* numFound);


/* graphics */


GpStatus WINGDIPAPI
GdipCreateFromHWND(HWND hwnd, GpGraphics** graphics);

GpStatus WINGDIPAPI
GdipCreateFromHDC(HDC hdc, GpGraphics** graphics);

GpStatus WINGDIPAPI
GdipDeleteGraphics(GpGraphics* graphics);

GpStatus WINGDIPAPI
GdipSetSmoothingMode(GpGraphics* graphics, SmoothingMode smoothingMode);

GpStatus WINGDIPAPI
GdipSetClipRectI(GpGraphics* graphics, INT x, INT y,
	INT width, INT height, CombineMode combineMode);

GpStatus WINGDIPAPI
GdipDrawLineI(GpGraphics* graphics, GpPen* pen, INT x1, INT y1,
	INT x2, INT y2);

GpStatus WINGDIPAPI
GdipDrawArcI(GpGraphics* graphics, GpPen* pen, INT x, INT y,
	INT width, INT height, REAL startAngle, REAL sweepAngle);

GpStatus WINGDIPAPI
GdipDrawPieI(GpGraphics* graphics, GpPen* pen, INT x, INT y,
	INT width, INT height, REAL startAngle, REAL sweepAngle);

GpStatus WINGDIPAPI
GdipFillPieI(GpGraphics* graphics, GpBrush* brush, INT x, INT y,
	INT width, INT height, REAL startAngle, REAL sweepAngle);

GpStatus WINGDIPAPI
GdipDrawRectangleI(GpGraphics* graphics, GpPen* pen, INT x, INT y,
	INT width, INT height);

GpStatus WINGDIPAPI
GdipFillRectangleI(GpGraphics* graphics, GpBrush* brush, INT x, INT y,
	INT width, INT height);

GpStatus WINGDIPAPI
GdipFillPolygonI(GpGraphics* graphics, GpBrush* brush,
	GDIPCONST GpPoint* points, INT count, FillMode fillMode);

GpStatus WINGDIPAPI
GdipDrawPolygonI(GpGraphics* graphics, GpPen* pen, GDIPCONST GpPoint* points,
	INT count);

GpStatus WINGDIPAPI
GdipFillEllipseI(GpGraphics* graphics, GpBrush* brush, INT x, INT y,
	INT width, INT height);

GpStatus WINGDIPAPI
GdipDrawEllipseI(GpGraphics* graphics, GpPen* pen, INT x, INT y,
	INT width, INT height);

GpStatus WINGDIPAPI
GdipDrawBezierI(GpGraphics* graphics, GpPen* pen, INT x1, INT y1,
	INT x2, INT y2, INT x3, INT y3, INT x4, INT y4);

GpStatus WINGDIPAPI
GdipDrawString(
	GpGraphics* graphics,
	GDIPCONST WCHAR* string,
	INT                       length,
	GDIPCONST GpFont* font,
	GDIPCONST RectF* layoutRect,
	GDIPCONST GpStringFormat* stringFormat,
	GDIPCONST GpBrush* brush
);

GpStatus WINGDIPAPI
GdipGraphicsClear(GpGraphics* graphics, ARGB color);

GpStatus WINGDIPAPI
GdipDrawImageI(GpGraphics* graphics, GpImage* image, INT x, INT y);

GpStatus WINGDIPAPI
GdipDrawImageRectI(GpGraphics* graphics, GpImage* image, INT x, INT y,
	INT width, INT height);

GpStatus WINGDIPAPI
GdipMeasureString(
	GpGraphics* graphics,
	GDIPCONST WCHAR* string,
	INT                       length,
	GDIPCONST GpFont* font,
	GDIPCONST RectF* layoutRect,
	GDIPCONST GpStringFormat* stringFormat,
	RectF* boundingBox,
	INT* codepointsFitted,
	INT* linesFilled
);

GpStatus WINGDIPAPI
GdipSetTextRenderingHint(GpGraphics* graphics, TextRenderingHint mode);

LWSTDAPI_(IStream*) SHCreateMemStream(const BYTE* pInit, _In_ UINT cbInit);

struct GdipFont
{
	struct nk_user_font nk;
	GpFont* handle;
};

static struct
{
	ULONG_PTR token;

	HWND wnd;
	GpGraphics* window;
	GpGraphics* memory;
	GpImage* bitmap;
	GpPen* pen;
	GpSolidFill* brush;
	GpStringFormat* format;

	struct nk_context ctx;
} gdip;

static ARGB convert_color(struct nk_color c)
{
	return (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
}

static void
nk_gdip_scissor(float x, float y, float w, float h)
{
	GdipSetClipRectI(gdip.memory, (INT)x, (INT)y, (INT)(w + 1), (INT)(h + 1), CombineModeReplace);
}

static void
nk_gdip_stroke_line(short x0, short y0, short x1,
	short y1, unsigned int line_thickness, struct nk_color col)
{
	GdipSetPenWidth(gdip.pen, (REAL)line_thickness);
	GdipSetPenColor(gdip.pen, convert_color(col));
	GdipDrawLineI(gdip.memory, gdip.pen, x0, y0, x1, y1);
}

static void
nk_gdip_stroke_rect(short x, short y, unsigned short w,
	unsigned short h, unsigned short r, unsigned short line_thickness, struct nk_color col)
{
	GdipSetPenWidth(gdip.pen, (REAL)line_thickness);
	GdipSetPenColor(gdip.pen, convert_color(col));
	if (r == 0)
		GdipDrawRectangleI(gdip.memory, gdip.pen, x, y, w, h);
	else
	{
		INT d = 2 * r;
		GdipDrawArcI(gdip.memory, gdip.pen, x, y, d, d, 180, 90);
		GdipDrawLineI(gdip.memory, gdip.pen, x + r, y, x + w - r, y);
		GdipDrawArcI(gdip.memory, gdip.pen, x + w - d, y, d, d, 270, 90);
		GdipDrawLineI(gdip.memory, gdip.pen, x + w, y + r, x + w, y + h - r);
		GdipDrawArcI(gdip.memory, gdip.pen, x + w - d, y + h - d, d, d, 0, 90);
		GdipDrawLineI(gdip.memory, gdip.pen, x, y + r, x, y + h - r);
		GdipDrawArcI(gdip.memory, gdip.pen, x, y + h - d, d, d, 90, 90);
		GdipDrawLineI(gdip.memory, gdip.pen, x + r, y + h, x + w - r, y + h);
	}
}

static void
nk_gdip_fill_rect(short x, short y, unsigned short w,
	unsigned short h, unsigned short r, struct nk_color col)
{
	GdipSetSolidFillColor(gdip.brush, convert_color(col));
	if (r == 0)
		GdipFillRectangleI(gdip.memory, gdip.brush, x, y, w, h);
	else
	{
		INT d = 2 * r;
		GdipFillRectangleI(gdip.memory, gdip.brush, x + r, y, w - d, h);
		GdipFillRectangleI(gdip.memory, gdip.brush, x, y + r, r, h - d);
		GdipFillRectangleI(gdip.memory, gdip.brush, x + w - r, y + r, r, h - d);
		GdipFillPieI(gdip.memory, gdip.brush, x, y, d, d, 180, 90);
		GdipFillPieI(gdip.memory, gdip.brush, x + w - d, y, d, d, 270, 90);
		GdipFillPieI(gdip.memory, gdip.brush, x + w - d, y + h - d, d, d, 0, 90);
		GdipFillPieI(gdip.memory, gdip.brush, x, y + h - d, d, d, 90, 90);
	}
}

static BOOL
SetPoint(POINT* p, LONG x, LONG y)
{
	if (!p)
		return FALSE;
	p->x = x;
	p->y = y;
	return TRUE;
}

static void
nk_gdip_fill_triangle(short x0, short y0, short x1,
	short y1, short x2, short y2, struct nk_color col)
{
	POINT points[3];

	SetPoint(&points[0], x0, y0);
	SetPoint(&points[1], x1, y1);
	SetPoint(&points[2], x2, y2);

	GdipSetSolidFillColor(gdip.brush, convert_color(col));
	GdipFillPolygonI(gdip.memory, gdip.brush, points, 3, FillModeAlternate);
}

static void
nk_gdip_stroke_triangle(short x0, short y0, short x1,
	short y1, short x2, short y2, unsigned short line_thickness, struct nk_color col)
{
	POINT points[4];

	SetPoint(&points[0], x0, y0);
	SetPoint(&points[1], x1, y1);
	SetPoint(&points[2], x2, y2);
	SetPoint(&points[3], x0, y0);

	GdipSetPenWidth(gdip.pen, (REAL)line_thickness);
	GdipSetPenColor(gdip.pen, convert_color(col));
	GdipDrawPolygonI(gdip.memory, gdip.pen, points, 4);
}

static void
nk_gdip_fill_polygon(const struct nk_vec2i* pnts, int count, struct nk_color col)
{
	int i = 0;
#define MAX_POINTS 64
	POINT points[MAX_POINTS];
	GdipSetSolidFillColor(gdip.brush, convert_color(col));
	for (i = 0; i < count && i < MAX_POINTS; ++i)
	{
		points[i].x = pnts[i].x;
		points[i].y = pnts[i].y;
	}
	GdipFillPolygonI(gdip.memory, gdip.brush, points, i, FillModeAlternate);
#undef MAX_POINTS
}

static void
nk_gdip_stroke_polygon(const struct nk_vec2i* pnts, int count,
	unsigned short line_thickness, struct nk_color col)
{
	GdipSetPenWidth(gdip.pen, (REAL)line_thickness);
	GdipSetPenColor(gdip.pen, convert_color(col));
	if (count > 0)
	{
		int i;
		for (i = 1; i < count; ++i)
			GdipDrawLineI(gdip.memory, gdip.pen, pnts[i - 1].x, pnts[i - 1].y, pnts[i].x, pnts[i].y);
		GdipDrawLineI(gdip.memory, gdip.pen, pnts[count - 1].x, pnts[count - 1].y, pnts[0].x, pnts[0].y);
	}
}

static void
nk_gdip_stroke_polyline(const struct nk_vec2i* pnts,
	int count, unsigned short line_thickness, struct nk_color col)
{
	GdipSetPenWidth(gdip.pen, (REAL)line_thickness);
	GdipSetPenColor(gdip.pen, convert_color(col));
	if (count > 0)
	{
		int i;
		for (i = 1; i < count; ++i)
			GdipDrawLineI(gdip.memory, gdip.pen, pnts[i - 1].x, pnts[i - 1].y, pnts[i].x, pnts[i].y);
	}
}

static void
nk_gdip_fill_circle(short x, short y, unsigned short w,
	unsigned short h, struct nk_color col)
{
	GdipSetSolidFillColor(gdip.brush, convert_color(col));
	GdipFillEllipseI(gdip.memory, gdip.brush, x, y, w, h);
}

static void
nk_gdip_stroke_circle(short x, short y, unsigned short w,
	unsigned short h, unsigned short line_thickness, struct nk_color col)
{
	GdipSetPenWidth(gdip.pen, (REAL)line_thickness);
	GdipSetPenColor(gdip.pen, convert_color(col));
	GdipDrawEllipseI(gdip.memory, gdip.pen, x, y, w, h);
}

static void
nk_gdip_stroke_curve(struct nk_vec2i p1,
	struct nk_vec2i p2, struct nk_vec2i p3, struct nk_vec2i p4,
	unsigned short line_thickness, struct nk_color col)
{
	GdipSetPenWidth(gdip.pen, (REAL)line_thickness);
	GdipSetPenColor(gdip.pen, convert_color(col));
	GdipDrawBezierI(gdip.memory, gdip.pen, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y);
}

static void
nk_gdip_fill_arc(short cx, short cy, unsigned short r, float amin, float adelta, struct nk_color col)
{
	GdipSetSolidFillColor(gdip.brush, convert_color(col));
	GdipFillPieI(gdip.memory, gdip.brush, cx - r, cy - r, r * 2, r * 2, amin * (180 / M_PI), adelta * (180 / M_PI));
}

static void
nk_gdip_stroke_arc(short cx, short cy, unsigned short r, float amin, float adelta, unsigned short line_thickness, struct nk_color col)
{
	GdipSetPenWidth(gdip.pen, (REAL)line_thickness);
	GdipSetPenColor(gdip.pen, convert_color(col));
	GdipDrawPieI(gdip.memory, gdip.pen, cx - r, cy - r, r * 2, r * 2, amin * (180 / M_PI), adelta * (180 / M_PI));
}

static void
nk_gdip_draw_text(short x, short y, unsigned short w, unsigned short h,
	const char* text, int len, GdipFont* font, struct nk_color cbg, struct nk_color cfg)
{
	int wsize;
	WCHAR* wstr;
	RectF layout;

	layout.X = x;
	layout.Y = y;
	layout.Width = w;
	layout.Height = h;

	if (!text || !font || !len) return;

	wsize = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
	wstr = (WCHAR*)_alloca(wsize * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wsize);

	GdipSetSolidFillColor(gdip.brush, convert_color(cfg));
	GdipDrawString(gdip.memory, wstr, wsize, font->handle, &layout, gdip.format, gdip.brush);
}

static void
nk_gdip_draw_image(short x, short y, unsigned short w, unsigned short h,
	struct nk_image img, struct nk_color col)
{
	GpImage* image = img.handle.ptr;
	GdipDrawImageRectI(gdip.memory, image, x, y, w, h);
}

static void
nk_gdip_clear(struct nk_color col)
{
	GdipGraphicsClear(gdip.memory, convert_color(col));
}

static void
nk_gdip_blit(GpGraphics* graphics)
{
	GdipDrawImageI(graphics, gdip.bitmap, 0, 0);
}

static struct nk_image
nk_gdip_image_to_nk(GpImage* image)
{
	struct nk_image img;
	UINT uwidth, uheight;
	img = nk_image_ptr((void*)image);
	GdipGetImageHeight(image, &uheight);
	GdipGetImageWidth(image, &uwidth);
	img.h = uheight;
	img.w = uwidth;
	return img;
}

NK_API struct nk_image
nk_gdip_load_image_from_memory(const void* membuf, nk_uint membufSize)
{
	GpImage* image;
	GpStatus status;
	IStream* stream = SHCreateMemStream((const BYTE*)membuf, membufSize);
	if (!stream)
		return nk_image_id(0);

	status = GdipLoadImageFromStream(stream, &image);
	stream->lpVtbl->Release(stream);

	if (status)
		return nk_image_id(0);

	return nk_gdip_image_to_nk(image);
}

NK_API void
nk_gdip_image_free(struct nk_image image)
{
	if (!image.handle.ptr)
		return;
	GdipDisposeImage(image.handle.ptr);
}

static float
nk_gdipfont_get_text_width(nk_handle handle, float height, const char* text, int len)
{
	GdipFont* font = (GdipFont*)handle.ptr;
	RectF layout = { 0.0f, 0.0f, 65536.0f, 65536.0f };
	RectF bbox;
	int wsize;
	WCHAR* wstr;
	if (!font || !text)
		return 0;

	(void)height;
	wsize = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
	wstr = (WCHAR*)_malloca(wsize * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wsize);

	GdipMeasureString(gdip.memory, wstr, wsize, font->handle, &layout, gdip.format, &bbox, NULL, NULL);
	_freea(wstr);
	return bbox.Width;
}

NK_API void
nk_gdipfont_del(GdipFont* font)
{
	if (!font) return;
	GdipDeleteFont(font->handle);
	free(font);
}

static void
nk_gdip_clipboard_paste(nk_handle usr, struct nk_text_edit* edit)
{
	(void)usr;
	if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(NULL))
	{
		HGLOBAL mem = GetClipboardData(CF_UNICODETEXT);
		if (mem)
		{
			SIZE_T size = GlobalSize(mem) - 1;
			if (size)
			{
				LPCWSTR wstr = (LPCWSTR)GlobalLock(mem);
				if (wstr)
				{
					int utf8size = WideCharToMultiByte(CP_UTF8, 0, wstr, (int)(size / sizeof(wchar_t)), NULL, 0, NULL, NULL);
					if (utf8size)
					{
						char* utf8 = (char*)malloc(utf8size);
						if (utf8)
						{
							WideCharToMultiByte(CP_UTF8, 0, wstr, (int)(size / sizeof(wchar_t)), utf8, utf8size, NULL, NULL);
							nk_textedit_paste(edit, utf8, utf8size);
							free(utf8);
						}
					}
					GlobalUnlock(mem);
				}
			}
		}
		CloseClipboard();
	}
}

static void
nk_gdip_clipboard_copy(nk_handle usr, const char* text, int len)
{
	if (OpenClipboard(NULL))
	{
		int wsize = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
		if (wsize)
		{
			HGLOBAL mem = (HGLOBAL)GlobalAlloc(GMEM_MOVEABLE, (wsize + 1) * sizeof(wchar_t));
			if (mem)
			{
				wchar_t* wstr = (wchar_t*)GlobalLock(mem);
				if (wstr)
				{
					MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wsize);
					wstr[wsize] = 0;
					GlobalUnlock(mem);

					SetClipboardData(CF_UNICODETEXT, mem);
				}
			}
		}
		CloseClipboard();
	}
}

NK_API struct nk_context*
nk_gdip_init(HWND hwnd, unsigned int width, unsigned int height)
{
	GdiplusStartupInput startup = { 1, NULL, FALSE, TRUE };
	GdiplusStartup(&gdip.token, &startup, NULL);

	gdip.wnd = hwnd;
	GdipCreateFromHWND(hwnd, &gdip.window);
	GdipCreateBitmapFromGraphics(width, height, gdip.window, &gdip.bitmap);
	GdipGetImageGraphicsContext(gdip.bitmap, &gdip.memory);
	GdipCreatePen1(0, 1.0f, UnitPixel, &gdip.pen);
	GdipCreateSolidFill(0, &gdip.brush);
	GdipStringFormatGetGenericTypographic(&gdip.format);
	GdipSetStringFormatFlags(gdip.format, StringFormatFlagsNoFitBlackBox |
		StringFormatFlagsMeasureTrailingSpaces | StringFormatFlagsNoWrap |
		StringFormatFlagsNoClip);

	nk_init_default(&gdip.ctx, NULL);
	gdip.ctx.clip.copy = nk_gdip_clipboard_copy;
	gdip.ctx.clip.paste = nk_gdip_clipboard_paste;
	return &gdip.ctx;
}

NK_API void
nk_gdip_set_font(GdipFont* gdipfont)
{
	struct nk_user_font* font = &gdipfont->nk;
	font->userdata = nk_handle_ptr(gdipfont);
	GdipGetFontSize(gdipfont->handle, &font->height);
	font->width = nk_gdipfont_get_text_width;
	nk_style_set_font(&gdip.ctx, font);
}

NK_API int
nk_gdip_handle_event(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static int insert_toggle = 0;
	switch (msg)
	{
	case WM_SIZE:
		if (gdip.window)
		{
			unsigned int width = LOWORD(lparam);
			unsigned int height = HIWORD(lparam);
			if (!width || !height)
				break;
			GdipDeleteGraphics(gdip.window);
			GdipDeleteGraphics(gdip.memory);
			GdipDisposeImage(gdip.bitmap);
			GdipCreateFromHWND(wnd, &gdip.window);
			GdipCreateBitmapFromGraphics(width, height, gdip.window, &gdip.bitmap);
			GdipGetImageGraphicsContext(gdip.bitmap, &gdip.memory);
		}
		break;

	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC dc = BeginPaint(wnd, &paint);
		GpGraphics* graphics;
		GdipCreateFromHDC(dc, &graphics);
		nk_gdip_blit(graphics);
		GdipDeleteGraphics(graphics);
		EndPaint(wnd, &paint);
		return 1;
	}

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	{
		int down = !((lparam >> 31) & 1);
		int ctrl = GetKeyState(VK_CONTROL) & (1 << 15);

		switch (wparam)
		{
		case VK_SHIFT:
		case VK_LSHIFT:
		case VK_RSHIFT:
			nk_input_key(&gdip.ctx, NK_KEY_SHIFT, down);
			return 1;

		case VK_DELETE:
			nk_input_key(&gdip.ctx, NK_KEY_DEL, down);
			return 1;

		case VK_RETURN:
		case VK_SEPARATOR:
			nk_input_key(&gdip.ctx, NK_KEY_ENTER, down);
			return 1;

		case VK_TAB:
			nk_input_key(&gdip.ctx, NK_KEY_TAB, down);
			return 1;

		case VK_LEFT:
			if (ctrl)
				nk_input_key(&gdip.ctx, NK_KEY_TEXT_WORD_LEFT, down);
			else
				nk_input_key(&gdip.ctx, NK_KEY_LEFT, down);
			return 1;

		case VK_RIGHT:
			if (ctrl)
				nk_input_key(&gdip.ctx, NK_KEY_TEXT_WORD_RIGHT, down);
			else
				nk_input_key(&gdip.ctx, NK_KEY_RIGHT, down);
			return 1;

		case VK_BACK:
			nk_input_key(&gdip.ctx, NK_KEY_BACKSPACE, down);
			return 1;

		case VK_HOME:
			nk_input_key(&gdip.ctx, NK_KEY_TEXT_START, down);
			nk_input_key(&gdip.ctx, NK_KEY_SCROLL_START, down);
			return 1;

		case VK_END:
			nk_input_key(&gdip.ctx, NK_KEY_TEXT_END, down);
			nk_input_key(&gdip.ctx, NK_KEY_SCROLL_END, down);
			return 1;

		case VK_NEXT:
			nk_input_key(&gdip.ctx, NK_KEY_SCROLL_DOWN, down);
			return 1;

		case VK_PRIOR:
			nk_input_key(&gdip.ctx, NK_KEY_SCROLL_UP, down);
			return 1;

		case VK_ESCAPE:
			nk_input_key(&gdip.ctx, NK_KEY_TEXT_RESET_MODE, down);
			return 1;

		case VK_MENU:
			nk_input_key(&gdip.ctx, NK_KEY_ALT, down);
			return 1;

		case VK_F1:
			nk_input_key(&gdip.ctx, NK_KEY_F1, down);
			return 1;
		case VK_F2:
			nk_input_key(&gdip.ctx, NK_KEY_F2, down);
			return 1;
		case VK_F3:
			nk_input_key(&gdip.ctx, NK_KEY_F3, down);
			return 1;
		case VK_F4:
			nk_input_key(&gdip.ctx, NK_KEY_F4, down);
			return 1;
		case VK_F5:
			nk_input_key(&gdip.ctx, NK_KEY_F5, down);
			return 1;
		case VK_F6:
			nk_input_key(&gdip.ctx, NK_KEY_F6, down);
			return 1;
		case VK_F7:
			nk_input_key(&gdip.ctx, NK_KEY_F7, down);
			return 1;
		case VK_F8:
			nk_input_key(&gdip.ctx, NK_KEY_F8, down);
			return 1;
		case VK_F9:
			nk_input_key(&gdip.ctx, NK_KEY_F9, down);
			return 1;
		case VK_F10:
			nk_input_key(&gdip.ctx, NK_KEY_F10, down);
			return 1;
		case VK_F11:
			nk_input_key(&gdip.ctx, NK_KEY_F11, down);
			return 1;
		case VK_F12:
			nk_input_key(&gdip.ctx, NK_KEY_F12, down);
			return 1;

		case VK_INSERT:
			/* Only switch on release to avoid repeat issues
			 * kind of confusing since we have to negate it but we're already
			 * hacking it since Nuklear treats them as two separate keys rather
			 * than a single toggle state */
			if (!down)
			{
				insert_toggle = !insert_toggle;
				if (insert_toggle)
				{
					nk_input_key(&gdip.ctx, NK_KEY_TEXT_INSERT_MODE, !down);
					/* nk_input_key(&gdip.ctx, NK_KEY_TEXT_REPLACE_MODE, down); */
				}
				else
				{
					nk_input_key(&gdip.ctx, NK_KEY_TEXT_REPLACE_MODE, !down);
					/* nk_input_key(&gdip.ctx, NK_KEY_TEXT_INSERT_MODE, down); */
				}
			}
			return 1;

		case 'A':
			if (ctrl)
			{
				nk_input_key(&gdip.ctx, NK_KEY_TEXT_SELECT_ALL, down);
				return 1;
			}
			break;

		case 'B':
			if (ctrl)
			{
				nk_input_key(&gdip.ctx, NK_KEY_TEXT_LINE_START, down);
				return 1;
			}
			break;

		case 'E':
			if (ctrl)
			{
				nk_input_key(&gdip.ctx, NK_KEY_TEXT_LINE_END, down);
				return 1;
			}
			break;


		case 'C':
			if (ctrl)
			{
				nk_input_key(&gdip.ctx, NK_KEY_COPY, down);
				return 1;
			}
			break;

		case 'V':
			if (ctrl)
			{
				nk_input_key(&gdip.ctx, NK_KEY_PASTE, down);
				return 1;
			}
			break;

		case 'X':
			if (ctrl)
			{
				nk_input_key(&gdip.ctx, NK_KEY_CUT, down);
				return 1;
			}
			break;

		case 'Z':
			if (ctrl)
			{
				nk_input_key(&gdip.ctx, NK_KEY_TEXT_UNDO, down);
				return 1;
			}
			break;

		case 'R':
			if (ctrl)
			{
				nk_input_key(&gdip.ctx, NK_KEY_TEXT_REDO, down);
				return 1;
			}
			break;
		}
		return 0;
	}

	case WM_CHAR:
		if (wparam >= 32)
		{
			nk_input_unicode(&gdip.ctx, (nk_rune)wparam);
			return 1;
		}
		break;

	case WM_LBUTTONDOWN:
		nk_input_button(&gdip.ctx, NK_BUTTON_LEFT, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
		SetCapture(wnd);
		return 1;

	case WM_LBUTTONUP:
		nk_input_button(&gdip.ctx, NK_BUTTON_DOUBLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
		nk_input_button(&gdip.ctx, NK_BUTTON_LEFT, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
		ReleaseCapture();
		return 1;

	case WM_RBUTTONDOWN:
		nk_input_button(&gdip.ctx, NK_BUTTON_RIGHT, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
		SetCapture(wnd);
		return 1;

	case WM_RBUTTONUP:
		nk_input_button(&gdip.ctx, NK_BUTTON_RIGHT, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
		ReleaseCapture();
		return 1;

	case WM_MBUTTONDOWN:
		nk_input_button(&gdip.ctx, NK_BUTTON_MIDDLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
		SetCapture(wnd);
		return 1;

	case WM_MBUTTONUP:
		nk_input_button(&gdip.ctx, NK_BUTTON_MIDDLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
		ReleaseCapture();
		return 1;

	case WM_XBUTTONDOWN:
		switch (GET_XBUTTON_WPARAM(wparam)) {
		case XBUTTON1:
			nk_input_button(&gdip.ctx, NK_BUTTON_X1, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
			break;
		case XBUTTON2:
			nk_input_button(&gdip.ctx, NK_BUTTON_X2, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
			break;
		}
		SetCapture(wnd);
		return 1;

	case WM_XBUTTONUP:
		switch (GET_XBUTTON_WPARAM(wparam))
		{
		case XBUTTON1:
			nk_input_button(&gdip.ctx, NK_BUTTON_X1, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
			break;
		case XBUTTON2:
			nk_input_button(&gdip.ctx, NK_BUTTON_X2, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
			break;
		}
		ReleaseCapture();
		return 1;

	case WM_MOUSEWHEEL:
		nk_input_scroll(&gdip.ctx, nk_vec2(0, (float)(short)HIWORD(wparam) / WHEEL_DELTA));
		return 1;

	case WM_MOUSEMOVE:
		nk_input_motion(&gdip.ctx, (short)LOWORD(lparam), (short)HIWORD(lparam));
		return 1;

	case WM_LBUTTONDBLCLK:
		nk_input_button(&gdip.ctx, NK_BUTTON_DOUBLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
		return 1;
	}

	return 0;
}

NK_API void
nk_gdip_shutdown(void)
{
	GdipDeleteGraphics(gdip.window);
	GdipDeleteGraphics(gdip.memory);
	GdipDisposeImage(gdip.bitmap);
	GdipDeletePen(gdip.pen);
	GdipDeleteBrush(gdip.brush);
	GdipDeleteStringFormat(gdip.format);
	GdiplusShutdown(gdip.token);

	nk_free(&gdip.ctx);
}

NK_API void
nk_gdip_prerender_gui(enum nk_anti_aliasing AA)
{
	const struct nk_command* cmd;

	GdipSetTextRenderingHint(gdip.memory, AA != NK_ANTI_ALIASING_OFF ?
		TextRenderingHintClearTypeGridFit : TextRenderingHintSingleBitPerPixelGridFit);
	GdipSetSmoothingMode(gdip.memory, AA != NK_ANTI_ALIASING_OFF ?
		SmoothingModeHighQuality : SmoothingModeNone);

	nk_foreach(cmd, &gdip.ctx)
	{
		switch (cmd->type)
		{
		case NK_COMMAND_NOP:
			break;
		case NK_COMMAND_SCISSOR:
		{
			const struct nk_command_scissor* s = (const struct nk_command_scissor*)cmd;
			nk_gdip_scissor(s->x, s->y, s->w, s->h);
		}
			break;
		case NK_COMMAND_LINE:
		{
			const struct nk_command_line* l = (const struct nk_command_line*)cmd;
			nk_gdip_stroke_line(l->begin.x, l->begin.y, l->end.x,
				l->end.y, l->line_thickness, l->color);
		}
			break;
		case NK_COMMAND_RECT:
		{
			const struct nk_command_rect* r = (const struct nk_command_rect*)cmd;
			nk_gdip_stroke_rect(r->x, r->y, r->w, r->h,
				(unsigned short)r->rounding, r->line_thickness, r->color);
		}
			break;
		case NK_COMMAND_RECT_FILLED:
		{
			const struct nk_command_rect_filled* r = (const struct nk_command_rect_filled*)cmd;
			nk_gdip_fill_rect(r->x, r->y, r->w, r->h,
				(unsigned short)r->rounding, r->color);
		}
			break;
		case NK_COMMAND_CIRCLE:
		{
			const struct nk_command_circle* c = (const struct nk_command_circle*)cmd;
			nk_gdip_stroke_circle(c->x, c->y, c->w, c->h, c->line_thickness, c->color);
		}
			break;
		case NK_COMMAND_CIRCLE_FILLED:
		{
			const struct nk_command_circle_filled* c = (const struct nk_command_circle_filled*)cmd;
			nk_gdip_fill_circle(c->x, c->y, c->w, c->h, c->color);
		}
			break;
		case NK_COMMAND_TRIANGLE:
		{
			const struct nk_command_triangle* t = (const struct nk_command_triangle*)cmd;
			nk_gdip_stroke_triangle(t->a.x, t->a.y, t->b.x, t->b.y,
				t->c.x, t->c.y, t->line_thickness, t->color);
		}
			break;
		case NK_COMMAND_TRIANGLE_FILLED:
		{
			const struct nk_command_triangle_filled* t = (const struct nk_command_triangle_filled*)cmd;
			nk_gdip_fill_triangle(t->a.x, t->a.y, t->b.x, t->b.y,
				t->c.x, t->c.y, t->color);
		}
			break;
		case NK_COMMAND_POLYGON:
		{
			const struct nk_command_polygon* p = (const struct nk_command_polygon*)cmd;
			nk_gdip_stroke_polygon(p->points, p->point_count, p->line_thickness, p->color);
		}
			break;
		case NK_COMMAND_POLYGON_FILLED:
		{
			const struct nk_command_polygon_filled* p = (const struct nk_command_polygon_filled*)cmd;
			nk_gdip_fill_polygon(p->points, p->point_count, p->color);
		}
			break;
		case NK_COMMAND_POLYLINE:
		{
			const struct nk_command_polyline* p = (const struct nk_command_polyline*)cmd;
			nk_gdip_stroke_polyline(p->points, p->point_count, p->line_thickness, p->color);
		}
			break;
		case NK_COMMAND_TEXT:
		{
			const struct nk_command_text* t = (const struct nk_command_text*)cmd;
			nk_gdip_draw_text(t->x, t->y, t->w, t->h,
				(const char*)t->string, t->length,
				(GdipFont*)t->font->userdata.ptr,
				t->background, t->foreground);
		}
			break;
		case NK_COMMAND_CURVE:
		{
			const struct nk_command_curve* q = (const struct nk_command_curve*)cmd;
			nk_gdip_stroke_curve(q->begin, q->ctrl[0], q->ctrl[1],
				q->end, q->line_thickness, q->color);
		}
			break;
		case NK_COMMAND_IMAGE:
		{
			const struct nk_command_image* i = (const struct nk_command_image*)cmd;
			nk_gdip_draw_image(i->x, i->y, i->w, i->h, i->img, i->col);
		}
			break;
		case NK_COMMAND_ARC:
		{
			const struct nk_command_arc* i = (const struct nk_command_arc*)cmd;
			nk_gdip_stroke_arc(i->cx, i->cy, i->r, i->a[0], i->a[1], i->line_thickness, i->color);
		}
			break;
		case NK_COMMAND_ARC_FILLED:
		{
			const struct nk_command_arc_filled* i = (const struct nk_command_arc_filled*)cmd;
			nk_gdip_fill_arc(i->cx, i->cy, i->r, i->a[0], i->a[1], i->color);
		}
			break;
		case NK_COMMAND_RECT_MULTI_COLOR:
		default:
			break;
		}
	}
}

NK_API void
nk_gdip_render_gui(enum nk_anti_aliasing AA)
{
	nk_gdip_prerender_gui(AA);
	nk_gdip_blit(gdip.window);
	nk_clear(&gdip.ctx);
}

NK_API void
nk_gdip_render(enum nk_anti_aliasing AA, struct nk_color clear)
{
	nk_gdip_clear(clear);
	nk_gdip_render_gui(AA);
}

NK_API GdipFont*
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

static FILE* m_report_file = NULL;
static enum nk_report_state m_report_state = NK_REPORT_STATE_IDLE;
static float m_report_row_y = -1.0f;

static void
nk_report_reset_state(void)
{
	m_report_state = NK_REPORT_STATE_IDLE;
	m_report_row_y = -1.0f;
}

static nk_bool
nk_report_write(const char* text, size_t len)
{
	if (fwrite(text, 1, len, m_report_file) == len)
		return nk_true;
	fclose(m_report_file);
	m_report_file = NULL;
	m_report_state = NK_REPORT_STATE_FAILED;
	m_report_row_y = -1.0f;
	return nk_false;
}

static int
nk_report_get_indent(const struct nk_context* ctx, nk_bool minimum_one)
{
	int indent = ctx->current->layout->row.tree_depth;
	if (minimum_one && indent < 1)
		indent = 1;
	return indent;
}

static void
nk_report_capture_text(float y, int indent, const char* text, int len)
{
	if (m_report_state == NK_REPORT_STATE_IDLE || m_report_state == NK_REPORT_STATE_FAILED || len <= 0)
		return;

	if (m_report_state == NK_REPORT_STATE_ACTIVE_ROW_OPEN && fabsf(y - m_report_row_y) > 0.5f)
	{
		if (!nk_report_write("\r\n", 2))
			return;
		m_report_state = NK_REPORT_STATE_ACTIVE_READY;
	}

	if (m_report_state != NK_REPORT_STATE_ACTIVE_ROW_OPEN)
	{
		if (indent == 0 && m_report_state == NK_REPORT_STATE_ACTIVE_READY && !nk_report_write("\r\n", 2))
			return;
		m_report_row_y = y;
		m_report_state = NK_REPORT_STATE_ACTIVE_ROW_OPEN;
		while (indent-- > 0)
		{
			if (!nk_report_write("\t", 1))
				return;
		}
	}
	else
	{
		if (!nk_report_write("\t", 1))
			return;
	}

	nk_report_write(text, (size_t)len);
}

enum nk_report_state
nk_report_begin_capture(LPCWSTR path)
{
	static const unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };

	nk_report_reset_state();

	if (!path || !path[0])
	{
		m_report_state = NK_REPORT_STATE_FAILED;
		return m_report_state;
	}

	if (m_report_file)
	{
		fclose(m_report_file);
		m_report_file = NULL;
	}

	if (_wfopen_s(&m_report_file, path, L"wb") != 0 || !m_report_file)
	{
		m_report_state = NK_REPORT_STATE_FAILED;
		return m_report_state;
	}

	m_report_state = NK_REPORT_STATE_ACTIVE;
	if (!nk_report_write((const char*)utf8_bom, sizeof(utf8_bom)))
		return m_report_state;

	return m_report_state;
}

enum nk_report_state
nk_report_end_capture(void)
{
	if (!m_report_file)
	{
		m_report_state = NK_REPORT_STATE_FAILED;
		return m_report_state;
	}

	if (m_report_state == NK_REPORT_STATE_ACTIVE_ROW_OPEN && !nk_report_write("\r\n", 2))
		return m_report_state;

	if (m_report_file)
	{
		if (fclose(m_report_file) != 0)
		{
			m_report_file = NULL;
			m_report_state = NK_REPORT_STATE_FAILED;
			m_report_row_y = -1.0f;
			return m_report_state;
		}
		m_report_file = NULL;
	}

	nk_report_reset_state();
	return m_report_state;
}

static nk_bool
nk_panel_begin_ex(struct nk_context* ctx, const char* title, enum nk_panel_type panel_type,
	struct nk_image img_icon, struct nk_image img_minimize, struct nk_image img_close)
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
				if (nk_do_button_image(&ws, &win->buffer, button,
					img_minimize, NK_BUTTON_DEFAULT,
					&style->window.header.minimize_button, in) && !(win->flags & NK_WINDOW_ROM))
					PostMessageW(gdip.wnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
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
	struct nk_rect bounds, nk_flags flags, struct nk_image img_icon, struct nk_image img_minimize, struct nk_image img_close)
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
	ret = nk_panel_begin_ex(ctx, title, NK_PANEL_WINDOW, img_icon, img_minimize, img_close);
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
	nk_layout_peek(&bounds, ctx);
	nk_report_capture_text(bounds.y, nk_report_get_indent(ctx, nk_false), str, len);
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

nk_bool
nk_tree_image_push_ex(struct nk_context* ctx, enum nk_tree_type type,
	struct nk_image img, const char* title, enum nk_collapse_states state, int id)
{
	struct nk_rect bounds;

	NK_ASSERT(ctx);
	NK_ASSERT(ctx->current);
	NK_ASSERT(ctx->current->layout);
	NK_ASSERT(title);
	if (!ctx || !ctx->current || !ctx->current->layout || !title)
		return nk_false;

	nk_layout_peek(&bounds, ctx);
	nk_report_capture_text(bounds.y, nk_report_get_indent(ctx, nk_false), title, nk_strlen(title));
	return nk_tree_image_push_hashed(ctx, type, img, title, state, NK_FILE_LINE, nk_strlen(NK_FILE_LINE), id);
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
	h = NK_MAX(win->layout->row.min_height, ctx->style.font->height + 2 * ctx->style.window.padding.y);
	x = nk_ifloorf(in->mouse.pos.x + 1) - (int)win->layout->clip.x + 2 * ctx->style.window.padding.x;
	if (w + x > (int)win->bounds.w) x = (int)win->bounds.w - w;
	y = nk_ifloorf(in->mouse.pos.y + 1) - (int)win->layout->clip.y;
	if (y > h + (int)ctx->style.font->height)
		y -= h + (int)ctx->style.font->height;

	bounds.x = (float)x;
	bounds.y = (float)y;
	bounds.w = (float)w;
	bounds.h = (float)nk_iceilf(nk_null_rect.h);

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
	text_len = nk_strlen(str);
	nk_layout_peek(&bounds, ctx);
	nk_report_capture_text(bounds.y, nk_report_get_indent(ctx, nk_true), str, text_len);
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
	struct nk_rect bounds;
	int str_len = nk_strlen(str);
	nk_layout_peek(&bounds, ctx);
	nk_report_capture_text(bounds.y, nk_report_get_indent(ctx, nk_true), str, str_len);

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
	nk_button_text_styled(ctx, &style, str, str_len);
}

static nk_bool
nk_group_scrolled_offset_begin_ex(struct nk_context* ctx,
	nk_uint* x_offset, nk_uint* y_offset, const char* title, nk_flags flags, struct nk_image img_icon)
{
	struct nk_rect bounds;
	struct nk_window panel;
	struct nk_window* win;

	win = ctx->current;
	nk_panel_alloc_space(&bounds, ctx);
	{
		const struct nk_rect* c = &win->layout->clip;
		if (!NK_INTERSECT(c->x, c->y, c->w, c->h, bounds.x, bounds.y, bounds.w, bounds.h) &&
			!(flags & NK_WINDOW_MOVABLE))
		{
			return 0;
		}
	}
	if (win->flags & NK_WINDOW_ROM)
		flags |= NK_WINDOW_ROM;

	nk_zero(&panel, sizeof(panel));
	panel.bounds = bounds;
	panel.flags = flags;
	panel.scrollbar.x = *x_offset;
	panel.scrollbar.y = *y_offset;
	panel.buffer = win->buffer;
	panel.layout = (struct nk_panel*)nk_create_panel(ctx);
	ctx->current = &panel;
	nk_panel_begin_ex(ctx, (flags & NK_WINDOW_TITLE) ? title : 0, NK_PANEL_GROUP,
		img_icon, nk_image_id(0), nk_image_id(0));

	win->buffer = panel.buffer;
	win->buffer.clip = panel.layout->clip;
	panel.layout->offset_x = x_offset;
	panel.layout->offset_y = y_offset;
	panel.layout->parent = win->layout;
	win->layout = panel.layout;

	ctx->current = win;
	if ((panel.layout->flags & NK_WINDOW_CLOSED) ||
		(panel.layout->flags & NK_WINDOW_MINIMIZED))
	{
		nk_flags f = panel.layout->flags;
		nk_group_scrolled_end(ctx);
		if (f & NK_WINDOW_CLOSED)
			return NK_WINDOW_CLOSED;
		if (f & NK_WINDOW_MINIMIZED)
			return NK_WINDOW_MINIMIZED;
	}
	return 1;
}

nk_bool
nk_group_begin_ex(struct nk_context* ctx, const char* title, nk_flags flags, struct nk_image img_icon)
{
	int id_len;
	nk_hash id_hash;
	struct nk_window* win;
	nk_uint* x_offset;
	nk_uint* y_offset;

	NK_ASSERT(ctx);
	NK_ASSERT(title);
	NK_ASSERT(ctx->current);
	NK_ASSERT(ctx->current->layout);
	if (!ctx || !ctx->current || !ctx->current->layout || !title)
		return 0;

	/* find persistent group scrollbar value */
	win = ctx->current;
	id_len = (int)nk_strlen(title);
	id_hash = nk_murmur_hash(title, (int)id_len, NK_PANEL_GROUP);
	x_offset = nk_find_value(win, id_hash);
	if (!x_offset)
	{
		x_offset = nk_add_value(ctx, win, id_hash, 0);
		y_offset = nk_add_value(ctx, win, id_hash + 1, 0);

		NK_ASSERT(x_offset);
		NK_ASSERT(y_offset);
		if (!x_offset || !y_offset) return 0;
		*x_offset = *y_offset = 0;
	}
	else if (!(y_offset = nk_find_value(win, id_hash + 1)))
	{
		y_offset = nk_add_value(ctx, win, id_hash + 1, 0);
		NK_ASSERT(y_offset);
		if (!y_offset) return 0;
		*x_offset = *y_offset = 0;
	}

	if (flags & NK_WINDOW_TITLE)
	{
		struct nk_rect bounds;
		nk_layout_peek(&bounds, ctx);
		nk_report_capture_text(bounds.y, nk_report_get_indent(ctx, nk_true), title, id_len);
	}

	return nk_group_scrolled_offset_begin_ex(ctx, x_offset, y_offset, title, flags, img_icon);
}

nk_bool
nk_combo_begin_ex(struct nk_context* ctx, const char* selected, float height, nk_bool capture)
{
	struct nk_rect bounds;
	struct nk_vec2 size;
	int len = nk_strlen(selected);
	size.x = nk_widget_width(ctx);
	size.y = 8 * (height + ctx->style.window.spacing.y);
	size.y += ctx->style.window.spacing.y * 2 + ctx->style.window.combo_padding.y * 2;
	if (capture)
	{
		nk_layout_peek(&bounds, ctx);
		nk_report_capture_text(bounds.y, nk_report_get_indent(ctx, nk_false), selected, len);
	}
	return nk_combo_begin_text(ctx, selected, len, size);
}
