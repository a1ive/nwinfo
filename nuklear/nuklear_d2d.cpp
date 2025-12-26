// SPDX-License-Identifier: Unlicense

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR

#include "nuklear_d2d.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <dxgiformat.h>
#include <VersionHelpers.h>

#pragma comment(lib, "d2d1.lib")

struct nk_d2d_device
{
	struct nk_context ctx;
	HWND hwnd;
	UINT width;
	UINT height;
	ID2D1Factory* d2d_factory;
	ID2D1HwndRenderTarget* target;
	ID2D1SolidColorBrush* brush;
	IDWriteFactory* dwrite_factory;
	IWICImagingFactory* wic_factory;
};

static struct nk_d2d_device d2d;

static void
nk_d2d_safe_release(IUnknown* obj)
{
	if (obj)
		obj->Release();
}

static D2D1_COLOR_F
nk_d2d_color(struct nk_color col)
{
	return D2D1::ColorF(
		(float)col.r / 255.0f,
		(float)col.g / 255.0f,
		(float)col.b / 255.0f,
		(float)col.a / 255.0f);
}

static HRESULT
nk_d2d_create_render_target(UINT width, UINT height)
{
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
		0,
		0,
		D2D1_RENDER_TARGET_USAGE_NONE,
		D2D1_FEATURE_LEVEL_DEFAULT);
	D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_props = D2D1::HwndRenderTargetProperties(
		d2d.hwnd,
		D2D1::SizeU(width, height));
	HRESULT hr = d2d.d2d_factory->CreateHwndRenderTarget(props, hwnd_props, &d2d.target);
	if (FAILED(hr))
		return hr;
	return d2d.target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 1), &d2d.brush);
}

static void
nk_d2d_discard_render_target(void)
{
	nk_d2d_safe_release(d2d.brush);
	nk_d2d_safe_release(d2d.target);
	d2d.brush = NULL;
	d2d.target = NULL;
}

static WCHAR*
nk_d2d_utf8_to_wchar(const char* text, int len, int* out_len)
{
	int wlen = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
	WCHAR* wstr = NULL;
	if (!wlen)
		return NULL;
	wstr = (WCHAR*)malloc(sizeof(WCHAR) * (size_t)(wlen + 1));
	if (!wstr)
		return NULL;
	MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wlen);
	wstr[wlen] = 0;
	if (out_len)
		*out_len = wlen;
	return wstr;
}

static float
nk_d2d_font_get_text_width(nk_handle handle, float height, const char* text, int len)
{
	nk_d2d_font* font = (nk_d2d_font*)handle.ptr;
	IDWriteTextLayout* layout = NULL;
	DWRITE_TEXT_METRICS metrics = {};
	float width = 0.0f;
	int wlen = 0;
	WCHAR* wstr = NULL;

	(void)height;
	if (!font || !font->format || !text || !len)
		return 0.0f;

	wstr = nk_d2d_utf8_to_wchar(text, len, &wlen);
	if (!wstr)
		return 0.0f;

	if (SUCCEEDED(d2d.dwrite_factory->CreateTextLayout(
		wstr,
		wlen,
		(IDWriteTextFormat*)font->format,
		4096.0f,
		4096.0f,
		&layout)))
	{
		if (SUCCEEDED(layout->GetMetrics(&metrics)))
			width = metrics.widthIncludingTrailingWhitespace;
		layout->Release();
	}

	free(wstr);
	return width;
}

static float
nk_d2d_font_get_line_height(IDWriteTextFormat* format, float fallback_height)
{
	static const WCHAR sample_text[] = L"Mg";
	IDWriteTextLayout* layout = NULL;
	DWRITE_TEXT_METRICS metrics = {};
	float height = fallback_height;

	if (!format || !d2d.dwrite_factory)
		return fallback_height;

	if (SUCCEEDED(d2d.dwrite_factory->CreateTextLayout(
		sample_text,
		(UINT32)(sizeof(sample_text) / sizeof(sample_text[0]) - 1),
		format,
		4096.0f,
		4096.0f,
		&layout)))
	{
		if (SUCCEEDED(layout->GetMetrics(&metrics)) && metrics.height > 0.0f)
			height = metrics.height;
		layout->Release();
	}

	return (float)ceilf(height);
}

static void
nk_d2d_clipboard_paste(nk_handle usr, struct nk_text_edit* edit)
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
					int utf8size = WideCharToMultiByte(CP_UTF8, 0, wstr,
						(int)(size / sizeof(wchar_t)), NULL, 0, NULL, NULL);
					if (utf8size)
					{
						char* utf8 = (char*)malloc(utf8size);
						if (utf8)
						{
							WideCharToMultiByte(CP_UTF8, 0, wstr,
								(int)(size / sizeof(wchar_t)), utf8, utf8size, NULL, NULL);
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
nk_d2d_clipboard_copy(nk_handle usr, const char* text, int len)
{
	(void)usr;
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

static void
nk_d2d_scissor(float x, float y, float w, float h)
{
	D2D1_RECT_F rect = D2D1::RectF(x, y, x + w, y + h);
	d2d.target->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_ALIASED);
}

static void
nk_d2d_draw_line(short x0, short y0, short x1, short y1, unsigned short line_thickness, struct nk_color col)
{
	d2d.brush->SetColor(nk_d2d_color(col));
	d2d.target->DrawLine(D2D1::Point2F((FLOAT)x0, (FLOAT)y0), D2D1::Point2F((FLOAT)x1, (FLOAT)y1),
		d2d.brush, (FLOAT)line_thickness);
}

static void
nk_d2d_stroke_rect(short x, short y, unsigned short w, unsigned short h, unsigned short r,
	unsigned short line_thickness, struct nk_color col)
{
	D2D1_RECT_F rect = D2D1::RectF((FLOAT)x, (FLOAT)y, (FLOAT)(x + w), (FLOAT)(y + h));
	D2D1_ROUNDED_RECT round = D2D1::RoundedRect(rect, (FLOAT)r, (FLOAT)r);

	d2d.brush->SetColor(nk_d2d_color(col));
	if (r)
		d2d.target->DrawRoundedRectangle(round, d2d.brush, (FLOAT)line_thickness);
	else
		d2d.target->DrawRectangle(rect, d2d.brush, (FLOAT)line_thickness);
}

static void
nk_d2d_fill_rect(short x, short y, unsigned short w, unsigned short h, unsigned short r, struct nk_color col)
{
	D2D1_RECT_F rect = D2D1::RectF((FLOAT)x, (FLOAT)y, (FLOAT)(x + w), (FLOAT)(y + h));
	D2D1_ROUNDED_RECT round = D2D1::RoundedRect(rect, (FLOAT)r, (FLOAT)r);

	d2d.brush->SetColor(nk_d2d_color(col));
	if (r)
		d2d.target->FillRoundedRectangle(round, d2d.brush);
	else
		d2d.target->FillRectangle(rect, d2d.brush);
}

static void
nk_d2d_fill_triangle(short x0, short y0, short x1, short y1, short x2, short y2, struct nk_color col)
{
	ID2D1PathGeometry* geo = NULL;
	ID2D1GeometrySink* sink = NULL;

	if (FAILED(d2d.d2d_factory->CreatePathGeometry(&geo)))
		return;
	if (FAILED(geo->Open(&sink)))
	{
		geo->Release();
		return;
	}

	sink->BeginFigure(D2D1::Point2F((FLOAT)x0, (FLOAT)y0), D2D1_FIGURE_BEGIN_FILLED);
	sink->AddLine(D2D1::Point2F((FLOAT)x1, (FLOAT)y1));
	sink->AddLine(D2D1::Point2F((FLOAT)x2, (FLOAT)y2));
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	sink->Close();
	sink->Release();

	d2d.brush->SetColor(nk_d2d_color(col));
	d2d.target->FillGeometry(geo, d2d.brush);
	geo->Release();
}

static void
nk_d2d_stroke_triangle(short x0, short y0, short x1, short y1, short x2, short y2,
	unsigned short line_thickness, struct nk_color col)
{
	ID2D1PathGeometry* geo = NULL;
	ID2D1GeometrySink* sink = NULL;

	if (FAILED(d2d.d2d_factory->CreatePathGeometry(&geo)))
		return;
	if (FAILED(geo->Open(&sink)))
	{
		geo->Release();
		return;
	}

	sink->BeginFigure(D2D1::Point2F((FLOAT)x0, (FLOAT)y0), D2D1_FIGURE_BEGIN_HOLLOW);
	sink->AddLine(D2D1::Point2F((FLOAT)x1, (FLOAT)y1));
	sink->AddLine(D2D1::Point2F((FLOAT)x2, (FLOAT)y2));
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	sink->Close();
	sink->Release();

	d2d.brush->SetColor(nk_d2d_color(col));
	d2d.target->DrawGeometry(geo, d2d.brush, (FLOAT)line_thickness);
	geo->Release();
}

static void
nk_d2d_fill_polygon(const struct nk_vec2i* pnts, int count, struct nk_color col)
{
	ID2D1PathGeometry* geo = NULL;
	ID2D1GeometrySink* sink = NULL;

	if (count < 3)
		return;
	if (FAILED(d2d.d2d_factory->CreatePathGeometry(&geo)))
		return;
	if (FAILED(geo->Open(&sink)))
	{
		geo->Release();
		return;
	}

	sink->BeginFigure(D2D1::Point2F((FLOAT)pnts[0].x, (FLOAT)pnts[0].y), D2D1_FIGURE_BEGIN_FILLED);
	for (int i = 1; i < count; i++)
		sink->AddLine(D2D1::Point2F((FLOAT)pnts[i].x, (FLOAT)pnts[i].y));
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	sink->Close();
	sink->Release();

	d2d.brush->SetColor(nk_d2d_color(col));
	d2d.target->FillGeometry(geo, d2d.brush);
	geo->Release();
}

static void
nk_d2d_stroke_polygon(const struct nk_vec2i* pnts, int count, unsigned short line_thickness, struct nk_color col)
{
	ID2D1PathGeometry* geo = NULL;
	ID2D1GeometrySink* sink = NULL;

	if (count < 2)
		return;
	if (FAILED(d2d.d2d_factory->CreatePathGeometry(&geo)))
		return;
	if (FAILED(geo->Open(&sink)))
	{
		geo->Release();
		return;
	}

	sink->BeginFigure(D2D1::Point2F((FLOAT)pnts[0].x, (FLOAT)pnts[0].y), D2D1_FIGURE_BEGIN_HOLLOW);
	for (int i = 1; i < count; i++)
		sink->AddLine(D2D1::Point2F((FLOAT)pnts[i].x, (FLOAT)pnts[i].y));
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	sink->Close();
	sink->Release();

	d2d.brush->SetColor(nk_d2d_color(col));
	d2d.target->DrawGeometry(geo, d2d.brush, (FLOAT)line_thickness);
	geo->Release();
}

static void
nk_d2d_stroke_polyline(const struct nk_vec2i* pnts, int count, unsigned short line_thickness, struct nk_color col)
{
	ID2D1PathGeometry* geo = NULL;
	ID2D1GeometrySink* sink = NULL;

	if (count < 2)
		return;
	if (FAILED(d2d.d2d_factory->CreatePathGeometry(&geo)))
		return;
	if (FAILED(geo->Open(&sink)))
	{
		geo->Release();
		return;
	}

	sink->BeginFigure(D2D1::Point2F((FLOAT)pnts[0].x, (FLOAT)pnts[0].y), D2D1_FIGURE_BEGIN_HOLLOW);
	for (int i = 1; i < count; i++)
		sink->AddLine(D2D1::Point2F((FLOAT)pnts[i].x, (FLOAT)pnts[i].y));
	sink->EndFigure(D2D1_FIGURE_END_OPEN);
	sink->Close();
	sink->Release();

	d2d.brush->SetColor(nk_d2d_color(col));
	d2d.target->DrawGeometry(geo, d2d.brush, (FLOAT)line_thickness);
	geo->Release();
}

static void
nk_d2d_fill_circle(short x, short y, unsigned short w, unsigned short h, struct nk_color col)
{
	D2D1_ELLIPSE ellipse = D2D1::Ellipse(
		D2D1::Point2F((FLOAT)x + (FLOAT)w * 0.5f, (FLOAT)y + (FLOAT)h * 0.5f),
		(FLOAT)w * 0.5f,
		(FLOAT)h * 0.5f);
	d2d.brush->SetColor(nk_d2d_color(col));
	d2d.target->FillEllipse(ellipse, d2d.brush);
}

static void
nk_d2d_stroke_circle(short x, short y, unsigned short w, unsigned short h, unsigned short line_thickness, struct nk_color col)
{
	D2D1_ELLIPSE ellipse = D2D1::Ellipse(
		D2D1::Point2F((FLOAT)x + (FLOAT)w * 0.5f, (FLOAT)y + (FLOAT)h * 0.5f),
		(FLOAT)w * 0.5f,
		(FLOAT)h * 0.5f);
	d2d.brush->SetColor(nk_d2d_color(col));
	d2d.target->DrawEllipse(ellipse, d2d.brush, (FLOAT)line_thickness);
}

static void
nk_d2d_stroke_curve(struct nk_vec2i p1, struct nk_vec2i p2, struct nk_vec2i p3, struct nk_vec2i p4,
	unsigned short line_thickness, struct nk_color col)
{
	D2D1_POINT_2F start = D2D1::Point2F((FLOAT)p1.x, (FLOAT)p1.y);
	D2D1_BEZIER_SEGMENT seg = {
		D2D1::Point2F((FLOAT)p2.x, (FLOAT)p2.y),
		D2D1::Point2F((FLOAT)p3.x, (FLOAT)p3.y),
		D2D1::Point2F((FLOAT)p4.x, (FLOAT)p4.y)
	};
	ID2D1PathGeometry* geo = NULL;
	ID2D1GeometrySink* sink = NULL;

	if (FAILED(d2d.d2d_factory->CreatePathGeometry(&geo)))
		return;
	if (FAILED(geo->Open(&sink)))
	{
		geo->Release();
		return;
	}

	sink->BeginFigure(start, D2D1_FIGURE_BEGIN_HOLLOW);
	sink->AddBezier(seg);
	sink->EndFigure(D2D1_FIGURE_END_OPEN);
	sink->Close();
	sink->Release();

	d2d.brush->SetColor(nk_d2d_color(col));
	d2d.target->DrawGeometry(geo, d2d.brush, (FLOAT)line_thickness);
	geo->Release();
}

static void
nk_d2d_fill_arc(short cx, short cy, unsigned short r, float amin, float adelta, struct nk_color col)
{
	float angle_end = amin + adelta;
	D2D1_POINT_2F center = D2D1::Point2F((FLOAT)cx, (FLOAT)cy);
	D2D1_POINT_2F start = D2D1::Point2F(
		(FLOAT)(cx + cosf(amin) * r),
		(FLOAT)(cy + sinf(amin) * r));
	D2D1_POINT_2F end = D2D1::Point2F(
		(FLOAT)(cx + cosf(angle_end) * r),
		(FLOAT)(cy + sinf(angle_end) * r));
	D2D1_ARC_SIZE arc_size = (fabsf(adelta) > (float)NK_PI) ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL;
	D2D1_SWEEP_DIRECTION sweep = adelta >= 0.0f ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;
	D2D1_ARC_SEGMENT arc = { end, D2D1::SizeF((FLOAT)r, (FLOAT)r), 0.0f, sweep, arc_size };
	ID2D1PathGeometry* geo = NULL;
	ID2D1GeometrySink* sink = NULL;

	if (FAILED(d2d.d2d_factory->CreatePathGeometry(&geo)))
		return;
	if (FAILED(geo->Open(&sink)))
	{
		geo->Release();
		return;
	}

	sink->BeginFigure(center, D2D1_FIGURE_BEGIN_FILLED);
	sink->AddLine(start);
	sink->AddArc(arc);
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	sink->Close();
	sink->Release();

	d2d.brush->SetColor(nk_d2d_color(col));
	d2d.target->FillGeometry(geo, d2d.brush);
	geo->Release();
}

static void
nk_d2d_stroke_arc(short cx, short cy, unsigned short r, float amin, float adelta, unsigned short line_thickness,
	struct nk_color col)
{
	float angle_end = amin + adelta;
	D2D1_POINT_2F start = D2D1::Point2F(
		(FLOAT)(cx + cosf(amin) * r),
		(FLOAT)(cy + sinf(amin) * r));
	D2D1_POINT_2F end = D2D1::Point2F(
		(FLOAT)(cx + cosf(angle_end) * r),
		(FLOAT)(cy + sinf(angle_end) * r));
	D2D1_ARC_SIZE arc_size = (fabsf(adelta) > (float)NK_PI) ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL;
	D2D1_SWEEP_DIRECTION sweep = adelta >= 0.0f ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;
	D2D1_ARC_SEGMENT arc = { end, D2D1::SizeF((FLOAT)r, (FLOAT)r), 0.0f, sweep, arc_size };
	ID2D1PathGeometry* geo = NULL;
	ID2D1GeometrySink* sink = NULL;

	if (FAILED(d2d.d2d_factory->CreatePathGeometry(&geo)))
		return;
	if (FAILED(geo->Open(&sink)))
	{
		geo->Release();
		return;
	}

	sink->BeginFigure(start, D2D1_FIGURE_BEGIN_HOLLOW);
	sink->AddArc(arc);
	sink->EndFigure(D2D1_FIGURE_END_OPEN);
	sink->Close();
	sink->Release();

	d2d.brush->SetColor(nk_d2d_color(col));
	d2d.target->DrawGeometry(geo, d2d.brush, (FLOAT)line_thickness);
	geo->Release();
}

static void
nk_d2d_draw_text(short x, short y, unsigned short w, unsigned short h, const char* text, int len,
	const nk_d2d_font* font, struct nk_color col)
{
	IDWriteTextLayout* layout = NULL;
	int wlen = 0;
	WCHAR* wstr = nk_d2d_utf8_to_wchar(text, len, &wlen);
	if (!font || !font->format)
	{
		free(wstr);
		return;
	}
	if (!wstr)
		return;

	if (SUCCEEDED(d2d.dwrite_factory->CreateTextLayout(
		wstr,
		wlen,
		(IDWriteTextFormat*)font->format,
		(FLOAT)w,
		(FLOAT)h,
		&layout)))
	{
		d2d.brush->SetColor(nk_d2d_color(col));
		d2d.target->DrawTextLayout(D2D1::Point2F((FLOAT)x, (FLOAT)y), layout, d2d.brush,
			D2D1_DRAW_TEXT_OPTIONS_NONE);
		layout->Release();
	}

	free(wstr);
}

static void
nk_d2d_draw_image(short x, short y, unsigned short w, unsigned short h, struct nk_image img, struct nk_color col)
{
	ID2D1Bitmap* bitmap = (ID2D1Bitmap*)img.handle.ptr;
	if (!bitmap)
		return;
	D2D1_RECT_F rect = D2D1::RectF((FLOAT)x, (FLOAT)y, (FLOAT)(x + w), (FLOAT)(y + h));
	d2d.target->DrawBitmap(bitmap, rect, (FLOAT)col.a / 255.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
}

NK_API nk_d2d_font*
nk_d2d_load_font(LPCWSTR name, int size)
{
	HRESULT hr;
	IDWriteTextFormat* format = NULL;
	nk_d2d_font* font = (nk_d2d_font*)calloc(1, sizeof(nk_d2d_font));

	if (!font)
		goto fail;

	hr = d2d.dwrite_factory->CreateTextFormat(
		name,
		NULL,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		(FLOAT)size,
		L"",
		&format);
	if (FAILED(hr))
	{
		UINT len = IsWindowsVistaOrGreater() ? sizeof(NONCLIENTMETRICSW) : sizeof(NONCLIENTMETRICSW) - sizeof(int);
		NONCLIENTMETRICSW metrics = { 0 };
		metrics.cbSize = len;
		if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, len, &metrics, 0))
		{
			hr = d2d.dwrite_factory->CreateTextFormat(
				metrics.lfMessageFont.lfFaceName,
				NULL,
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				(FLOAT)size,
				L"",
				&format);
		}
	}
	if (FAILED(hr) || !format)
		goto fail;

	format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

	font->format = format;
	font->nk.userdata = nk_handle_ptr(font);
	font->nk.height = nk_d2d_font_get_line_height(format, (float)size);
	font->nk.width = nk_d2d_font_get_text_width;
	return font;

fail:
	MessageBoxW(NULL, L"Failed to load font", L"Error", MB_OK);
	if (format)
		format->Release();
	free(font);
	exit(1);
}

NK_API void
nk_d2d_font_del(nk_d2d_font* font)
{
	if (!font)
		return;
	if (font->format)
		((IDWriteTextFormat*)font->format)->Release();
	free(font);
}

NK_API struct nk_context*
nk_d2d_init(HWND hwnd, unsigned int width, unsigned int height)
{
	HRESULT hr;
	d2d.hwnd = hwnd;
	d2d.width = width;
	d2d.height = height;

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d.d2d_factory);
	if (FAILED(hr))
		goto fail;
	
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(&d2d.dwrite_factory));
	if (FAILED(hr))
		goto fail;

	hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&d2d.wic_factory));
	if (FAILED(hr))
		goto fail;

	hr = nk_d2d_create_render_target(width, height);
	if (FAILED(hr))
		goto fail;

	nk_init_default(&d2d.ctx, NULL);
	d2d.ctx.clip.copy = nk_d2d_clipboard_copy;
	d2d.ctx.clip.paste = nk_d2d_clipboard_paste;
	return &d2d.ctx;

fail:
	MessageBoxW(NULL, L"Failed to initialize Direct2D", L"Error", MB_OK);
	nk_d2d_discard_render_target();
	nk_d2d_safe_release(d2d.wic_factory);
	nk_d2d_safe_release(d2d.dwrite_factory);
	nk_d2d_safe_release(d2d.d2d_factory);
	return NULL;
}

NK_API void
nk_d2d_set_font(nk_d2d_font* font)
{
	if (!font)
		return;
	nk_style_set_font(&d2d.ctx, &font->nk);
}

NK_API int
nk_d2d_handle_event(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static int insert_toggle = 0;
	switch (msg)
	{
	case WM_SIZE:
		if (d2d.target)
		{
			unsigned int width = LOWORD(lparam);
			unsigned int height = HIWORD(lparam);
			d2d.width = width;
			d2d.height = height;
			d2d.target->Resize(D2D1::SizeU(width, height));
		}
		break;

	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		BeginPaint(wnd, &paint);
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
			nk_input_key(&d2d.ctx, NK_KEY_SHIFT, down);
			return 1;

		case VK_DELETE:
			nk_input_key(&d2d.ctx, NK_KEY_DEL, down);
			return 1;

		case VK_RETURN:
		case VK_SEPARATOR:
			nk_input_key(&d2d.ctx, NK_KEY_ENTER, down);
			return 1;

		case VK_TAB:
			nk_input_key(&d2d.ctx, NK_KEY_TAB, down);
			return 1;

		case VK_LEFT:
			if (ctrl)
				nk_input_key(&d2d.ctx, NK_KEY_TEXT_WORD_LEFT, down);
			else
				nk_input_key(&d2d.ctx, NK_KEY_LEFT, down);
			return 1;

		case VK_RIGHT:
			if (ctrl)
				nk_input_key(&d2d.ctx, NK_KEY_TEXT_WORD_RIGHT, down);
			else
				nk_input_key(&d2d.ctx, NK_KEY_RIGHT, down);
			return 1;

		case VK_BACK:
			nk_input_key(&d2d.ctx, NK_KEY_BACKSPACE, down);
			return 1;

		case VK_HOME:
			nk_input_key(&d2d.ctx, NK_KEY_TEXT_START, down);
			nk_input_key(&d2d.ctx, NK_KEY_SCROLL_START, down);
			return 1;

		case VK_END:
			nk_input_key(&d2d.ctx, NK_KEY_TEXT_END, down);
			nk_input_key(&d2d.ctx, NK_KEY_SCROLL_END, down);
			return 1;

		case VK_NEXT:
			nk_input_key(&d2d.ctx, NK_KEY_SCROLL_DOWN, down);
			return 1;

		case VK_PRIOR:
			nk_input_key(&d2d.ctx, NK_KEY_SCROLL_UP, down);
			return 1;

		case VK_ESCAPE:
			nk_input_key(&d2d.ctx, NK_KEY_TEXT_RESET_MODE, down);
			return 1;

		case VK_INSERT:
			if (!down)
			{
				insert_toggle = !insert_toggle;
				if (insert_toggle)
					nk_input_key(&d2d.ctx, NK_KEY_TEXT_INSERT_MODE, !down);
				else
					nk_input_key(&d2d.ctx, NK_KEY_TEXT_REPLACE_MODE, !down);
			}
			return 1;

		case 'A':
			if (ctrl)
			{
				nk_input_key(&d2d.ctx, NK_KEY_TEXT_SELECT_ALL, down);
				return 1;
			}
			break;

		case 'B':
			if (ctrl)
			{
				nk_input_key(&d2d.ctx, NK_KEY_TEXT_LINE_START, down);
				return 1;
			}
			break;

		case 'E':
			if (ctrl)
			{
				nk_input_key(&d2d.ctx, NK_KEY_TEXT_LINE_END, down);
				return 1;
			}
			break;

		case 'C':
			if (ctrl)
			{
				nk_input_key(&d2d.ctx, NK_KEY_COPY, down);
				return 1;
			}
			break;

		case 'V':
			if (ctrl)
			{
				nk_input_key(&d2d.ctx, NK_KEY_PASTE, down);
				return 1;
			}
			break;

		case 'X':
			if (ctrl)
			{
				nk_input_key(&d2d.ctx, NK_KEY_CUT, down);
				return 1;
			}
			break;

		case 'Z':
			if (ctrl)
			{
				nk_input_key(&d2d.ctx, NK_KEY_TEXT_UNDO, down);
				return 1;
			}
			break;

		case 'R':
			if (ctrl)
			{
				nk_input_key(&d2d.ctx, NK_KEY_TEXT_REDO, down);
				return 1;
			}
			break;
		}
		return 0;
	}

	case WM_CHAR:
		if (wparam >= 32)
		{
			nk_input_unicode(&d2d.ctx, (nk_rune)wparam);
			return 1;
		}
		break;

	case WM_LBUTTONDOWN:
		nk_input_button(&d2d.ctx, NK_BUTTON_LEFT, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
		SetCapture(wnd);
		return 1;

	case WM_LBUTTONUP:
		nk_input_button(&d2d.ctx, NK_BUTTON_DOUBLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
		nk_input_button(&d2d.ctx, NK_BUTTON_LEFT, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
		ReleaseCapture();
		return 1;

	case WM_RBUTTONDOWN:
		nk_input_button(&d2d.ctx, NK_BUTTON_RIGHT, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
		SetCapture(wnd);
		return 1;

	case WM_RBUTTONUP:
		nk_input_button(&d2d.ctx, NK_BUTTON_RIGHT, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
		ReleaseCapture();
		return 1;

	case WM_MBUTTONDOWN:
		nk_input_button(&d2d.ctx, NK_BUTTON_MIDDLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
		SetCapture(wnd);
		return 1;

	case WM_MBUTTONUP:
		nk_input_button(&d2d.ctx, NK_BUTTON_MIDDLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
		ReleaseCapture();
		return 1;

	case WM_MOUSEWHEEL:
		nk_input_scroll(&d2d.ctx, nk_vec2(0, (float)(short)HIWORD(wparam) / WHEEL_DELTA));
		return 1;

	case WM_MOUSEMOVE:
		nk_input_motion(&d2d.ctx, (short)LOWORD(lparam), (short)HIWORD(lparam));
		return 1;

	case WM_LBUTTONDBLCLK:
		nk_input_button(&d2d.ctx, NK_BUTTON_DOUBLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
		return 1;
	}

	return 0;
}

NK_API struct nk_image
nk_d2d_load_image_from_file(const WCHAR* filename)
{
	IWICBitmapDecoder* decoder = NULL;
	IWICBitmapFrameDecode* frame = NULL;
	IWICFormatConverter* converter = NULL;
	ID2D1Bitmap* bitmap = NULL;
	HRESULT hr;

	if (!d2d.wic_factory || !d2d.target)
		return nk_image_id(0);

	hr = d2d.wic_factory->CreateDecoderFromFilename(filename, NULL, GENERIC_READ,
		WICDecodeMetadataCacheOnLoad, &decoder);
	if (FAILED(hr))
		goto cleanup;
	
	hr = decoder->GetFrame(0, &frame);
	if (FAILED(hr))
		goto cleanup;

	hr = d2d.wic_factory->CreateFormatConverter(&converter);
	if (FAILED(hr))
		goto cleanup;

	hr = converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeMedianCut);
	if (FAILED(hr))
		goto cleanup;

	hr = d2d.target->CreateBitmapFromWicBitmap(converter, NULL, &bitmap);
	if (FAILED(hr))
		goto cleanup;

	converter->Release();
	frame->Release();
	decoder->Release();
	return nk_image_ptr(bitmap);

cleanup:
	if (bitmap)
		bitmap->Release();
	if (converter)
		converter->Release();
	if (frame)
		frame->Release();
	if (decoder)
		decoder->Release();
	return nk_image_id(0);
}

NK_API struct nk_image
nk_d2d_load_image_from_memory(const void* membuf, nk_uint membufSize)
{
	IWICStream* stream = NULL;
	IWICBitmapDecoder* decoder = NULL;
	IWICBitmapFrameDecode* frame = NULL;
	IWICFormatConverter* converter = NULL;
	ID2D1Bitmap* bitmap = NULL;
	HRESULT hr;

	if (!d2d.wic_factory || !d2d.target)
		return nk_image_id(0);

	hr = d2d.wic_factory->CreateStream(&stream);
	if (FAILED(hr))
		goto cleanup;

	hr = stream->InitializeFromMemory((WICInProcPointer)membuf, (DWORD)membufSize);
	if (FAILED(hr))
		goto cleanup;

	hr = d2d.wic_factory->CreateDecoderFromStream(stream, NULL, WICDecodeMetadataCacheOnLoad, &decoder);
	if (FAILED(hr))
		goto cleanup;

	hr = decoder->GetFrame(0, &frame);
	if (FAILED(hr))
		goto cleanup;

	hr = d2d.wic_factory->CreateFormatConverter(&converter);
	if (FAILED(hr))
		goto cleanup;

	hr = converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeMedianCut);
	if (FAILED(hr))
		goto cleanup;

	hr = d2d.target->CreateBitmapFromWicBitmap(converter, NULL, &bitmap);
	if (FAILED(hr))
		goto cleanup;

	nk_d2d_safe_release(converter);
	nk_d2d_safe_release(frame);
	nk_d2d_safe_release(decoder);
	nk_d2d_safe_release(stream);
	return nk_image_ptr(bitmap);

cleanup:
	nk_d2d_safe_release(bitmap);
	nk_d2d_safe_release(converter);
	nk_d2d_safe_release(frame);
	nk_d2d_safe_release(decoder);
	nk_d2d_safe_release(stream);
	return nk_image_id(0);
}

NK_API void
nk_d2d_image_free(struct nk_image image)
{
	ID2D1Bitmap* bitmap = (ID2D1Bitmap*)image.handle.ptr;
	if (bitmap)
		bitmap->Release();
}

NK_API void
nk_d2d_render(enum nk_anti_aliasing AA, struct nk_color clear)
{
	const struct nk_command* cmd;
	bool has_clip = false;
	HRESULT hr;

	if (!d2d.target)
		return;

	d2d.target->BeginDraw();
	d2d.target->SetAntialiasMode(AA != NK_ANTI_ALIASING_OFF ?
		D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
	d2d.target->SetTextAntialiasMode(AA != NK_ANTI_ALIASING_OFF ?
		D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE : D2D1_TEXT_ANTIALIAS_MODE_ALIASED);
	d2d.target->Clear(nk_d2d_color(clear));

	nk_foreach(cmd, &d2d.ctx)
	{
		switch (cmd->type)
		{
		case NK_COMMAND_NOP:
			break;
		case NK_COMMAND_SCISSOR:
		{
			const struct nk_command_scissor* s = (const struct nk_command_scissor*)cmd;
			if (has_clip)
				d2d.target->PopAxisAlignedClip();
			nk_d2d_scissor(s->x, s->y, s->w, s->h);
			has_clip = true;
		} break;
		case NK_COMMAND_LINE:
		{
			const struct nk_command_line* l = (const struct nk_command_line*)cmd;
			nk_d2d_draw_line(l->begin.x, l->begin.y, l->end.x, l->end.y,
				l->line_thickness, l->color);
		} break;
		case NK_COMMAND_RECT:
		{
			const struct nk_command_rect* r = (const struct nk_command_rect*)cmd;
			nk_d2d_stroke_rect(r->x, r->y, r->w, r->h,
				(unsigned short)r->rounding, r->line_thickness, r->color);
		} break;
		case NK_COMMAND_RECT_FILLED:
		{
			const struct nk_command_rect_filled* r = (const struct nk_command_rect_filled*)cmd;
			nk_d2d_fill_rect(r->x, r->y, r->w, r->h,
				(unsigned short)r->rounding, r->color);
		} break;
		case NK_COMMAND_CIRCLE:
		{
			const struct nk_command_circle* c = (const struct nk_command_circle*)cmd;
			nk_d2d_stroke_circle(c->x, c->y, c->w, c->h, c->line_thickness, c->color);
		} break;
		case NK_COMMAND_CIRCLE_FILLED:
		{
			const struct nk_command_circle_filled* c = (const struct nk_command_circle_filled*)cmd;
			nk_d2d_fill_circle(c->x, c->y, c->w, c->h, c->color);
		} break;
		case NK_COMMAND_TRIANGLE:
		{
			const struct nk_command_triangle* t = (const struct nk_command_triangle*)cmd;
			nk_d2d_stroke_triangle(t->a.x, t->a.y, t->b.x, t->b.y,
				t->c.x, t->c.y, t->line_thickness, t->color);
		} break;
		case NK_COMMAND_TRIANGLE_FILLED:
		{
			const struct nk_command_triangle_filled* t = (const struct nk_command_triangle_filled*)cmd;
			nk_d2d_fill_triangle(t->a.x, t->a.y, t->b.x, t->b.y, t->c.x, t->c.y, t->color);
		} break;
		case NK_COMMAND_POLYGON:
		{
			const struct nk_command_polygon* p = (const struct nk_command_polygon*)cmd;
			nk_d2d_stroke_polygon(p->points, p->point_count, p->line_thickness, p->color);
		} break;
		case NK_COMMAND_POLYGON_FILLED:
		{
			const struct nk_command_polygon_filled* p = (const struct nk_command_polygon_filled*)cmd;
			nk_d2d_fill_polygon(p->points, p->point_count, p->color);
		} break;
		case NK_COMMAND_POLYLINE:
		{
			const struct nk_command_polyline* p = (const struct nk_command_polyline*)cmd;
			nk_d2d_stroke_polyline(p->points, p->point_count, p->line_thickness, p->color);
		} break;
		case NK_COMMAND_TEXT:
		{
			const struct nk_command_text* t = (const struct nk_command_text*)cmd;
			nk_d2d_draw_text(t->x, t->y, t->w, t->h, (const char*)t->string, t->length,
				(const nk_d2d_font*)t->font->userdata.ptr, t->foreground);
		} break;
		case NK_COMMAND_CURVE:
		{
			const struct nk_command_curve* q = (const struct nk_command_curve*)cmd;
			nk_d2d_stroke_curve(q->begin, q->ctrl[0], q->ctrl[1], q->end, q->line_thickness, q->color);
		} break;
		case NK_COMMAND_IMAGE:
		{
			const struct nk_command_image* i = (const struct nk_command_image*)cmd;
			nk_d2d_draw_image(i->x, i->y, i->w, i->h, i->img, i->col);
		} break;
		case NK_COMMAND_ARC:
		{
			const struct nk_command_arc* i = (const struct nk_command_arc*)cmd;
			nk_d2d_stroke_arc(i->cx, i->cy, i->r, i->a[0], i->a[1], i->line_thickness, i->color);
		} break;
		case NK_COMMAND_ARC_FILLED:
		{
			const struct nk_command_arc_filled* i = (const struct nk_command_arc_filled*)cmd;
			nk_d2d_fill_arc(i->cx, i->cy, i->r, i->a[0], i->a[1], i->color);
		} break;
		default:
			break;
		}
	}

	if (has_clip)
		d2d.target->PopAxisAlignedClip();

	hr = d2d.target->EndDraw();
	if (hr == D2DERR_RECREATE_TARGET)
	{
		nk_d2d_discard_render_target();
		if (SUCCEEDED(nk_d2d_create_render_target(d2d.width, d2d.height)))
			return;
	}

	nk_clear(&d2d.ctx);
}

NK_API void
nk_d2d_shutdown(void)
{
	nk_d2d_discard_render_target();
	nk_d2d_safe_release(d2d.wic_factory);
	nk_d2d_safe_release(d2d.dwrite_factory);
	nk_d2d_safe_release(d2d.d2d_factory);
	nk_free(&d2d.ctx);
}
