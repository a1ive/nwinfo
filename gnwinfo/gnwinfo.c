// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "gettext.h"

#include <pathcch.h>
#include <windowsx.h>
#include <dbt.h>

LPCSTR NWL_Ucs2ToUtf8(LPCWSTR src);
LPCWSTR NWL_Utf8ToUcs2(LPCSTR src);

unsigned int g_init_width = 600;
unsigned int g_init_height = 800;
unsigned int g_init_alpha = 255;
GdipFont* g_font = NULL;
int g_font_size = 12;
double g_dpi_factor = 1.0;
nk_bool g_dpi_scaling = 1;
nk_bool g_bginfo = 0;

static UINT m_dpi = USER_DEFAULT_SCREEN_DPI;

#define REGION_MASK_LEFT    (1 << 0)
#define REGION_MASK_RIGHT   (1 << 1)
#define REGION_MASK_TOP     (1 << 2)
#define REGION_MASK_BOTTOM  (1 << 3)

static void
set_dpi_scaling(HWND wnd)
{
	WCHAR font_name[64];
	GetPrivateProfileStringW(L"Window", L"Font", L"-", font_name, 64, g_ini_path);
	if (wcscmp(font_name, L"-") == 0)
		wcscpy_s(font_name, 64, NWL_Utf8ToUcs2(N_(N__FONT_)));
	if (g_bginfo)
		g_dpi_scaling = 0;
	else
		g_dpi_scaling = strtol(gnwinfo_get_ini_value(L"Window", L"DpiScaling", L"1"), NULL, 10);
	if (g_font)
	{
		nk_gdipfont_del(g_font);
		g_font = NULL;
	}
	if (g_dpi_scaling)
	{
		RECT rect = { 0 };
		UINT dpi = GetDpiForWindow(wnd);
		g_dpi_factor = 1.0 * dpi / m_dpi;
		m_dpi = dpi;
		g_font_size = (int)(g_font_size * g_dpi_factor);
		// resize window
		GetWindowRect(wnd, &rect);
		SetWindowPos(wnd, NULL, 0, 0,
			(int)((rect.right - rect.left) * g_dpi_factor), (int)((rect.bottom - rect.top) * g_dpi_factor),
			SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
	g_font = nk_gdip_load_font(font_name, g_font_size);
	nk_gdip_set_font(g_font);
}

static LRESULT CALLBACK
window_proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_TIMER:
		gnwinfo_ctx_update(wparam);
		break;
	case WM_DEVICECHANGE:
	{
		switch (wparam)
		{
		case DBT_DEVNODES_CHANGED:
			// TODO: check if this is needed
			break;
		case DBT_DEVICEARRIVAL:
		case DBT_DEVICEREMOVECOMPLETE:
			gnwinfo_ctx_update(IDT_TIMER_DISK);
			break;
		}
	}
		break;
	case WM_DPICHANGED:
		set_dpi_scaling(wnd);
		break;
	case WM_POWERBROADCAST:
		gnwinfo_ctx_update(IDT_TIMER_POWER);
		break;
	case WM_DISPLAYCHANGE:
		gnwinfo_ctx_update(IDT_TIMER_DISPLAY);
		if (g_bginfo)
		{
			int x = 0;
			RECT desktop = { 0, 0, 1024, 768 };
			GetWindowRect(GetDesktopWindow(), &desktop);
			if (desktop.right > (LONG)g_init_width)
				x = desktop.right - (LONG)g_init_width;
			SetWindowPos(wnd, HWND_BOTTOM, x, 0, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
		}
		break;
	case WM_MOUSEMOVE:
		if (g_bginfo && !g_ctx.mouse)
		{
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_HOVER | TME_LEAVE;
			tme.hwndTrack = wnd;
			tme.dwHoverTime = 100;
			TrackMouseEvent(&tme);
			g_ctx.mouse = TRUE;
		}
		break;
	case WM_MOUSEHOVER:
		if (g_bginfo)
		{
			g_ctx.mouse = FALSE;
#ifdef GNWINFO_TRANSPARENT
			SetLayeredWindowAttributes(wnd, 0, (BYTE)g_init_alpha, LWA_ALPHA);
#endif
			SetForegroundWindow(wnd);
		}
		break;
	case WM_MOUSELEAVE:
		if (g_bginfo)
		{
			g_ctx.mouse = FALSE;
#ifdef GNWINFO_TRANSPARENT
			SetLayeredWindowAttributes(wnd, RGB(g_color_back.r, g_color_back.g, g_color_back.b), (BYTE)g_init_alpha, LWA_COLORKEY | LWA_ALPHA);
#endif
			SetWindowPos(wnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
		break;
	case WM_NCHITTEST:
		if (!g_bginfo)
		{
			RECT rect = { 0 };
			LONG result = 0;
			LONG x = GET_X_LPARAM(lparam);
			LONG y = GET_Y_LPARAM(lparam);
			LONG w = GetSystemMetricsForDpi(SM_CXFRAME, USER_DEFAULT_SCREEN_DPI)
				+ GetSystemMetricsForDpi(SM_CXPADDEDBORDER, USER_DEFAULT_SCREEN_DPI);
			LONG h = GetSystemMetricsForDpi(SM_CYFRAME, USER_DEFAULT_SCREEN_DPI)
				+ GetSystemMetricsForDpi(SM_CXPADDEDBORDER, USER_DEFAULT_SCREEN_DPI);
			GetWindowRect(wnd, &rect);
			result = REGION_MASK_LEFT * (x < (rect.left + w)) |
				REGION_MASK_RIGHT * (x >= (rect.right - w)) |
				REGION_MASK_TOP * (y < (rect.top + h)) |
				REGION_MASK_BOTTOM * (y >= (rect.bottom - h));
			switch (result)
			{
			case REGION_MASK_LEFT: return HTLEFT;
			case REGION_MASK_RIGHT: return HTRIGHT;
			case REGION_MASK_TOP: return HTTOP;
			case REGION_MASK_BOTTOM: return HTBOTTOM;
			case REGION_MASK_TOP | REGION_MASK_LEFT: return HTTOPLEFT;
			case REGION_MASK_TOP | REGION_MASK_RIGHT: return HTTOPRIGHT;
			case REGION_MASK_BOTTOM | REGION_MASK_LEFT: return HTBOTTOMLEFT;
			case REGION_MASK_BOTTOM | REGION_MASK_RIGHT: return HTBOTTOMRIGHT;
			}
			if (y <= (LONG)(rect.top + g_ctx.gui_title) &&
				x <= (LONG)(rect.right - 3 * g_ctx.gui_title))
				return HTCAPTION;
		}
		break;
	case WM_SIZE:
		g_ctx.gui_height = HIWORD(lparam);
		g_ctx.gui_width = LOWORD(lparam);
		break;
	}
	if (nk_gdip_handle_event(wnd, msg, wparam, lparam))
		return 0;
	return DefWindowProcW(wnd, msg, wparam, lparam);
}

static void
get_ini_color(LPCWSTR key, struct nk_color* color)
{
	WCHAR fallback[7];
	UINT32 hex;
	swprintf(fallback, 7, L"%02X%02X%02X", color->r, color->g, color->b);
	hex = strtoul(gnwinfo_get_ini_value(L"Color", key, fallback), NULL, 16);
	color->r = (hex >> 16) & 0xFF;
	color->g = (hex >> 8) & 0xFF;
	color->b = hex & 0xFF;
}

int APIENTRY
wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	struct nk_context* ctx;
	const char* str;
	int x_pos, y_pos;
	WNDCLASSW wc;
	DWORD style = WS_POPUP | WS_VISIBLE;
	DWORD exstyle = WS_EX_LAYERED;
	HWND wnd;
	int running = 1;
	int needs_refresh = 1;
	DWORD layered_flag = LWA_ALPHA;

	GetModuleFileNameW(NULL, g_ini_path, MAX_PATH);
	PathCchRemoveFileSpec(g_ini_path, MAX_PATH);
	PathCchAppend(g_ini_path, MAX_PATH, L"gnwinfo.ini");
	x_pos = strtol(gnwinfo_get_ini_value(L"Window", L"X", L"100"), NULL, 10);
	y_pos = strtol(gnwinfo_get_ini_value(L"Window", L"Y", L"100"), NULL, 10);
	g_init_width = strtoul(gnwinfo_get_ini_value(L"Window", L"Width", L"600"), NULL, 10);
	g_init_height = strtoul(gnwinfo_get_ini_value(L"Window", L"Height", L"800"), NULL, 10);
	g_init_alpha = strtoul(gnwinfo_get_ini_value(L"Window", L"Alpha", L"255"), NULL, 10);
	g_font_size = strtol(gnwinfo_get_ini_value(L"Window", L"FontSize", L"12"), NULL, 10);
	str = gnwinfo_get_ini_value(L"Window", L"BGInfo", L"0");
	if (str[0] != '0')
	{
		RECT desktop = {0, 0, 1024, 768};
		g_bginfo = 1;
		exstyle |= WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;
		GetWindowRect(GetDesktopWindow(), &desktop);
		x_pos = desktop.right > (LONG)g_init_width ? (desktop.right - (LONG)g_init_width) : 0;
		y_pos = 0;
	}
	get_ini_color(L"Background", &g_color_back);
	get_ini_color(L"Highlight", &g_color_text_l);
	get_ini_color(L"Default", &g_color_text_d);
	get_ini_color(L"StateGood", &g_color_good);
	get_ini_color(L"StateWarn", &g_color_warning);
	get_ini_color(L"StateError", &g_color_error);
	get_ini_color(L"StateUnknown", &g_color_unknown);

	/* Win32 */
	memset(&wc, 0, sizeof(wc));
	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = window_proc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON1));
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wc.lpszClassName = L"NwinfoWindowClass";
	RegisterClassW(&wc);

	wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"NWinfo GUI", style,
		x_pos, y_pos, (int)g_init_width, (int)g_init_height, NULL, NULL, wc.hInstance, NULL);

	if (g_bginfo)
	{
		SetWindowPos(wnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#ifdef GNWINFO_TRANSPARENT
		layered_flag |= LWA_COLORKEY;
#endif
	}

	SetLayeredWindowAttributes(wnd, RGB(g_color_back.r, g_color_back.g, g_color_back.b), (BYTE)g_init_alpha, layered_flag);

	/* GUI */
	ctx = nk_gdip_init(wnd, g_init_width, g_init_height);
	set_dpi_scaling(wnd);

	(void)CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	gnwinfo_set_style(ctx);
	gnwinfo_ctx_init(hInstance, wnd, ctx, (float)(g_init_width * g_dpi_factor), (float)(g_init_height * g_dpi_factor));

	while (running)
	{
		/* Input */
		MSG msg;
		nk_input_begin(ctx);
		if (needs_refresh == 0)
		{
			if (GetMessageW(&msg, NULL, 0, 0) <= 0)
				running = 0;
			else
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
			needs_refresh = 1;
		}
		else
			needs_refresh = 0;
		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				running = 0;
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
			needs_refresh = 1;
		}
		nk_input_end(ctx);

		/* GUI */
		if (g_ctx.window_flag & GUI_WINDOW_SETTINGS)
			gnwinfo_set_style(ctx);
		gnwinfo_draw_main_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_cpuid_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_about_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_smart_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_settings_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_pci_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_dmi_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_display_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_mm_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_hostname_window(ctx, g_ctx.gui_width, g_ctx.gui_height);

		/* Draw */
		nk_gdip_render(g_ctx.gui_aa, g_color_back);
	}

	CoUninitialize();
	nk_gdipfont_del(g_font);
	nk_gdip_shutdown();
	UnregisterClassW(wc.lpszClassName, wc.hInstance);
	gnwinfo_ctx_exit();
	return 0;
}
