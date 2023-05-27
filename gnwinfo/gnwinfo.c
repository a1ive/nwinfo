// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 800

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
		if (wparam == IDT_TIMER_1M)
		{
			NWL_NodeFree(g_ctx.disk, 1);
			g_ctx.disk = NW_Disk();
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

int APIENTRY
wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	GdipFont* font;
	struct nk_context* ctx;

	WNDCLASSW wc;
	RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
	DWORD style = WS_THICKFRAME;
	DWORD exstyle = WS_EX_APPWINDOW;
	HWND wnd;
	int running = 1;
	int needs_refresh = 1;

	/* Win32 */
	memset(&wc, 0, sizeof(wc));
	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON1));
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wc.lpszClassName = L"NwinfoWindowClass";
	RegisterClassW(&wc);

	AdjustWindowRectEx(&rect, style, FALSE, exstyle);

	wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"NWinfo GUI",
		style, CW_USEDEFAULT, CW_USEDEFAULT,
		rect.right - rect.left, rect.bottom - rect.top,
		NULL, NULL, wc.hInstance, NULL);
	ShowWindow(wnd, SW_SHOW);

	/* GUI */
	ctx = nk_gdip_init(wnd, WINDOW_WIDTH, WINDOW_HEIGHT);
	font = nk_gdip_load_font(L"Courier New", GNWINFO_FONT_SIZE, IDR_FONT1);
	nk_gdip_set_font(font);

	(void)CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	gnwinfo_ctx_init(hInstance, wnd, ctx, WINDOW_WIDTH, WINDOW_HEIGHT);
	ctx->style.button.rounding = 0;
	ctx->style.window.min_row_height_padding = 2;

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
