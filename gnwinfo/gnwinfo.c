// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

#include <pathcch.h>

unsigned int g_init_width = 600;
unsigned int g_init_height = 800;

GdipFont*
nk_gdip_load_font(LPCWSTR name, int size, WORD fallback);

static LRESULT CALLBACK
WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
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

int APIENTRY
wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	GdipFont* font;
	struct nk_context* ctx;
	char* str;

	WNDCLASSW wc;
	RECT rect = { 0 };
	DWORD style = WS_THICKFRAME;
	DWORD exstyle = WS_EX_APPWINDOW;
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

	/* Win32 */
	memset(&wc, 0, sizeof(wc));
	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON1));
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wc.lpszClassName = L"NwinfoWindowClass";
	RegisterClassW(&wc);

	rect.right = g_init_width;
	rect.bottom = g_init_height;
	AdjustWindowRectEx(&rect, style, FALSE, exstyle);

	wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"NWinfo GUI",
		style, CW_USEDEFAULT, CW_USEDEFAULT,
		rect.right - rect.left, rect.bottom - rect.top,
		NULL, NULL, wc.hInstance, NULL);
	ShowWindow(wnd, SW_SHOW);

	/* GUI */
	ctx = nk_gdip_init(wnd, g_init_width, g_init_height);
	font = nk_gdip_load_font(L"Courier New", GNWINFO_FONT_SIZE, IDR_FONT1);
	nk_gdip_set_font(font);

	(void)CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	gnwinfo_ctx_init(hInstance, wnd, ctx, (float)g_init_width, (float)g_init_height);
	ctx->style.button.rounding = 0;
	ctx->style.window.min_row_height_padding = 2;

	str = gnwinfo_get_ini_value(L"Summary", L"HideComponents", L"0xFFFFFFFF"); // ~0U
	g_ctx.main_flag = strtoul(str, NULL, 16);
	free(str);
	str = gnwinfo_get_ini_value(L"Summary", L"SmartFormat", L"0");
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
		gnwinfo_draw_main_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_cpuid_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_about_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_smart_window(ctx, g_ctx.gui_width, g_ctx.gui_height);
		gnwinfo_draw_settings_window(ctx, g_ctx.gui_width, g_ctx.gui_height);

		/* Draw */
		nk_gdip_render(NK_ANTI_ALIASING_ON, NK_COLOR_GRAY);
	}

	CoUninitialize();
	nk_gdipfont_del(font);
	nk_gdip_shutdown();
	UnregisterClassW(wc.lpszClassName, wc.hInstance);
	gnwinfo_ctx_exit();
	return 0;
}
