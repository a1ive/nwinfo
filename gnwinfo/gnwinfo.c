// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

#include <pathcch.h>

unsigned int g_init_width = 600;
unsigned int g_init_height = 800;
unsigned int g_init_alpha = 255;
nk_bool g_bginfo = 0;
struct nk_color g_color_warning = NK_COLOR_YELLOW;
struct nk_color g_color_error = NK_COLOR_RED;
struct nk_color g_color_good = NK_COLOR_GREEN;
struct nk_color g_color_unknown = NK_COLOR_BLUE;
struct nk_color g_color_text_l = NK_COLOR_WHITE;
struct nk_color g_color_text_d = NK_COLOR_LIGHT;
struct nk_color g_color_back = NK_COLOR_GRAY;

GdipFont*
nk_gdip_load_font(LPCWSTR name, int size, WORD fallback);

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
	char* str;
	WCHAR fallback[7];
	UINT32 hex;
	swprintf(fallback, 7, L"%02X%02X%02X", color->r, color->g, color->b);
	str = gnwinfo_get_ini_value(L"Color", key, fallback);
	hex = strtoul(str, NULL, 16);
	color->r = (hex >> 16) & 0xFF;
	color->g = (hex >> 8) & 0xFF;
	color->b = hex & 0xFF;
	free(str);
}

int APIENTRY
wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	GdipFont* font;
	struct nk_context* ctx;
	char* str;
	int x_pos = CW_USEDEFAULT, y_pos = CW_USEDEFAULT;
	WNDCLASSW wc;
	DWORD style = WS_SIZEBOX | WS_VISIBLE;
	DWORD exstyle = WS_EX_LAYERED;
	HWND wnd;
	int running = 1;
	int needs_refresh = 1;

	GetModuleFileNameW(NULL, g_ini_path, MAX_PATH);
	PathCchRemoveFileSpec(g_ini_path, MAX_PATH);
	PathCchAppend(g_ini_path, MAX_PATH, L"gnwinfo.ini");

	str = gnwinfo_get_ini_value(L"Window", L"Width", L"600");
	g_init_width = strtoul(str, NULL, 10);
	free(str);
	str = gnwinfo_get_ini_value(L"Window", L"Height", L"800");
	g_init_height = strtoul(str, NULL, 10);
	free(str);
	str = gnwinfo_get_ini_value(L"Window", L"Alpha", L"255");
	g_init_alpha = strtoul(str, NULL, 10);
	free(str);
	str = gnwinfo_get_ini_value(L"Window", L"BGInfo", L"0");
	if (str[0] != '0')
	{
		RECT desktop = {0, 0, 1024, 768};
		g_bginfo = 1;
		exstyle |= WS_EX_NOACTIVATE;
		GetWindowRect(GetDesktopWindow(), &desktop);
		x_pos = desktop.right > (LONG)g_init_width ? (desktop.right - (LONG)g_init_width) : 0;
		y_pos = 0;
	}
	free(str);
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

	wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"NWinfo GUI",
		style, x_pos, y_pos,
		g_init_width, g_init_height,
		NULL, NULL, wc.hInstance, NULL);

	style = (DWORD)GetWindowLongPtrW(wnd, GWL_STYLE) & ~WS_CAPTION;
	if (g_bginfo)
		style &= ~WS_SIZEBOX;
	SetWindowLongPtrW(wnd, GWL_STYLE, style);
	SetLayeredWindowAttributes(wnd, 0, (BYTE)g_init_alpha, LWA_ALPHA);

	/* GUI */
	ctx = nk_gdip_init(wnd, g_init_width, g_init_height);
	font = nk_gdip_load_font(L"Courier New", GNWINFO_FONT_SIZE, IDR_FONT1);
	nk_gdip_set_font(font);

	(void)CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	gnwinfo_ctx_init(hInstance, wnd, ctx, (float)g_init_width, (float)g_init_height);
	ctx->style.button.rounding = 0;
	ctx->style.window.min_row_height_padding = 2;

	str = gnwinfo_get_ini_value(L"Widgets", L"HideComponents", L"0xFFFFFFFF"); // ~0U
	g_ctx.main_flag = strtoul(str, NULL, 16);
	free(str);
	str = gnwinfo_get_ini_value(L"Widgets", L"SmartFormat", L"0");
	g_ctx.smart_hex = strtoul(str, NULL, 10);
	free(str);

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
		ctx->style.text.color = g_color_text_d;
		ctx->style.window.fixed_background.data.color = g_color_back;
		gnwinfo_draw_main_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_cpuid_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_about_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_smart_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_settings_window(ctx, g_ctx.gui_width, g_ctx.gui_height);

		/* Draw */
		nk_gdip_render(NK_ANTI_ALIASING_ON, (struct nk_color)NK_COLOR_BLACK);
	}

	CoUninitialize();
	nk_gdipfont_del(font);
	nk_gdip_shutdown();
	UnregisterClassW(wc.lpszClassName, wc.hInstance);
	gnwinfo_ctx_exit();
	return 0;
}
