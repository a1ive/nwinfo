// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

WCHAR g_ini_path[MAX_PATH] = { 0 };

char*
gnwinfo_get_ini_value(LPCWSTR section, LPCWSTR key, LPCWSTR fallback)
{
	CHAR* value = NULL;
	int size;
	WCHAR wvalue[MAX_PATH];

	GetPrivateProfileStringW(section, key, fallback, wvalue, MAX_PATH, g_ini_path);
	size = WideCharToMultiByte(CP_UTF8, 0, wvalue, -1, NULL, 0, NULL, NULL);
	if (size <= 0)
		goto fail;
	value = (char*)calloc(size, sizeof(char));
	if (!value)
		goto fail;
	WideCharToMultiByte(CP_UTF8, 0, wvalue, -1, value, size, NULL, NULL);
	return value;
fail:
	MessageBoxW(NULL, L"Get ini value failed", L"Error", MB_ICONERROR);
	exit(1);
}

void
gnwinfo_set_ini_value(LPCWSTR section, LPCWSTR key, LPCWSTR _Printf_format_string_ format, ...)
{
	int sz;
	WCHAR* buf = NULL;
	va_list ap;
	va_start(ap, format);
	sz = _vscwprintf(format, ap) + 1;
	if (sz <= 0)
	{
		va_end(ap);
		goto fail;
	}
	buf = calloc(sizeof(WCHAR), sz);
	if (!buf)
	{
		va_end(ap);
		goto fail;
	}
	_vsnwprintf_s(buf, sz, _TRUNCATE, format, ap);
	va_end(ap);
	WritePrivateProfileStringW(section, key, buf, g_ini_path);
	free(buf);
	return;
fail:
	MessageBoxW(NULL, L"Set ini value failed", L"Error", MB_ICONERROR);
}

VOID
gnwinfo_draw_settings_window(struct nk_context* ctx, float width, float height)
{
	if (g_ctx.gui_settings == FALSE)
		return;
	if (!nk_begin(ctx, "Settings",
		nk_rect(width / 4.0f, height / 3.0f, width / 2.0f, height / 3.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE))
	{
		g_ctx.gui_settings = FALSE;
		goto out;
	}

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, "Hide components", NK_TEXT_LEFT);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.1f, 0.45f, 0.45f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, "OS", &g_ctx.main_flag, MAIN_INFO_OS);
	nk_checkbox_flags_label(ctx, "BIOS", &g_ctx.main_flag, MAIN_INFO_BIOS);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, "Computer", &g_ctx.main_flag, MAIN_INFO_BOARD);
	nk_checkbox_flags_label(ctx, "CPU", &g_ctx.main_flag, MAIN_INFO_CPU);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, "Memory", &g_ctx.main_flag, MAIN_INFO_MEMORY);
	nk_checkbox_flags_label(ctx, "Display Devices", &g_ctx.main_flag, MAIN_INFO_MONITOR);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, "Storage", &g_ctx.main_flag, MAIN_INFO_STORAGE);
	nk_checkbox_flags_label(ctx, "Network", &g_ctx.main_flag, MAIN_INFO_NETWORK);

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, "Network", NK_TEXT_LEFT);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, "Hide inactive network", &g_ctx.main_flag, MAIN_NET_INACTIVE);

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, "Storage", NK_TEXT_LEFT);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	g_ctx.smart_hex = !nk_check_label(ctx, "Display SMART in HEX format", !g_ctx.smart_hex);

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, "Default window size", NK_TEXT_LEFT);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.5f, 0.5f });
	nk_property_int(ctx, "#Width", 60, &g_init_width, 1920, 10, 10);
	nk_property_int(ctx, "#Height", 80, &g_init_height, 1080, 10, 10);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.8f, 0.2f });
	nk_spacer(ctx);
	if (nk_button_label(ctx, "Save"))
	{
		gnwinfo_set_ini_value(L"Summary", L"HideComponents", L"0x%08X", g_ctx.main_flag);
		gnwinfo_set_ini_value(L"Summary", L"SmartFormat", L"%d", g_ctx.smart_hex);
		gnwinfo_set_ini_value(L"Window", L"Width", L"%u", g_init_width);
		gnwinfo_set_ini_value(L"Window", L"Height", L"%u", g_init_height);
	}
out:
	nk_end(ctx);
}
