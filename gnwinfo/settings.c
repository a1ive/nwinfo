// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "../libcdi/libcdi.h"

LPCSTR NWL_Ucs2ToUtf8(LPCWSTR src);
LPCWSTR NWL_Utf8ToUcs2(LPCSTR src);

WCHAR g_ini_path[MAX_PATH] = { 0 };

LPCSTR
gnwinfo_get_ini_value(LPCWSTR section, LPCWSTR key, LPCWSTR fallback)
{
	WCHAR wvalue[MAX_PATH];
	GetPrivateProfileStringW(section, key, fallback, wvalue, MAX_PATH, g_ini_path);
	return NWL_Ucs2ToUtf8(wvalue);
}

LPCSTR
gnwinfo_get_text(LPCWSTR text)
{
	WCHAR wvalue[MAX_PATH];
	GetPrivateProfileStringW(g_ctx.lang, text, text, wvalue, MAX_PATH, g_ini_path);
	return NWL_Ucs2ToUtf8(wvalue);
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

static void
set_ini_color(LPCWSTR key, struct nk_color color)
{
	gnwinfo_set_ini_value(L"Color", key, L"%02X%02X%02X", color.r, color.g, color.b);
}

static void
draw_color_picker(struct nk_context* ctx, struct nk_color* color)
{
	if (nk_combo_begin_color(ctx, *color, nk_vec2(200, 200)))
	{
		float ratios[] = { 0.15f, 0.85f };
		nk_layout_row(ctx, NK_DYNAMIC, 30, 2, ratios);
		nk_label(ctx, "R:", NK_TEXT_LEFT);
		color->r = (nk_byte)nk_slide_int(ctx, 0, color->r, 255, 5);
		nk_label(ctx, "G:", NK_TEXT_LEFT);
		color->g = (nk_byte)nk_slide_int(ctx, 0, color->g, 255, 5);
		nk_label(ctx, "B:", NK_TEXT_LEFT);
		color->b = (nk_byte)nk_slide_int(ctx, 0, color->b, 255, 5);
		nk_combo_end(ctx);
	}
}

VOID
gnwinfo_draw_settings_window(struct nk_context* ctx, float width, float height)
{
	if (g_ctx.gui_settings == FALSE)
		return;
	if (!nk_begin(ctx, gnwinfo_get_text(L"Settings"),
		nk_rect(width / 4.0f, height / 3.0f, width / 2.0f, height / 3.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE))
	{
		g_ctx.gui_settings = FALSE;
		goto out;
	}

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, gnwinfo_get_text(L"Hide components"), NK_TEXT_LEFT);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.1f, 0.45f, 0.45f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Operating System"), &g_ctx.main_flag, MAIN_INFO_OS);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"BIOS"), &g_ctx.main_flag, MAIN_INFO_BIOS);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Computer"), &g_ctx.main_flag, MAIN_INFO_BOARD);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Processor"), &g_ctx.main_flag, MAIN_INFO_CPU);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Memory"), &g_ctx.main_flag, MAIN_INFO_MEMORY);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Display Devices"), &g_ctx.main_flag, MAIN_INFO_MONITOR);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Storage"), &g_ctx.main_flag, MAIN_INFO_STORAGE);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Network"), &g_ctx.main_flag, MAIN_INFO_NETWORK);

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, gnwinfo_get_text(L"Operating System"), NK_TEXT_LEFT);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Hide uptime"), &g_ctx.main_flag, MAIN_OS_UPTIME);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Hide login status"), &g_ctx.main_flag, MAIN_OS_DETAIL);

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, gnwinfo_get_text(L"BIOS"), NK_TEXT_LEFT);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Hide vendor"), &g_ctx.main_flag, MAIN_B_VENDOR);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Hide version"), &g_ctx.main_flag, MAIN_B_VERSION);

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, gnwinfo_get_text(L"Processor"), NK_TEXT_LEFT);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Hide details"), &g_ctx.main_flag, MAIN_CPU_DETAIL);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Hide cache"), &g_ctx.main_flag, MAIN_CPU_CACHE);

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, gnwinfo_get_text(L"Memory"), NK_TEXT_LEFT);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Hide details"), &g_ctx.main_flag, MAIN_MEM_DETAIL);

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, gnwinfo_get_text(L"Network"), NK_TEXT_LEFT);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Hide inactive network"), &g_ctx.main_flag, MAIN_NET_INACTIVE);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Hide details"), &g_ctx.main_flag, MAIN_NET_DETAIL);

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, gnwinfo_get_text(L"Storage"), NK_TEXT_LEFT);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Compact view"), &g_ctx.main_flag, MAIN_DISK_COMPACT);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Hide SMART"), &g_ctx.main_flag, MAIN_DISK_SMART);
	nk_spacer(ctx);
	g_ctx.smart_hex = !nk_check_label(ctx, gnwinfo_get_text(L"Display SMART in HEX format"), !g_ctx.smart_hex);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Advanced disk search"), &g_ctx.smart_flag, CDI_FLAG_ADVANCED_SEARCH);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"HD204UI workaround"), &g_ctx.smart_flag, CDI_FLAG_WORKAROUND_HD204UI);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"ADATA SSD workaround"), &g_ctx.smart_flag, CDI_FLAG_WORKAROUND_ADATA);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"ATA pass through"), &g_ctx.smart_flag, CDI_FLAG_ATA_PASS_THROUGH);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable Nvidia controller"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_NVIDIA);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable Marvell controller"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_MARVELL);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable USB SAT"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_SAT);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable USB I-O DATA"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_IODATA);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable USB Sunplus"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_SUNPLUS);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable USB Logitec"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_LOGITEC);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable USB Prolific"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_PROLIFIC);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable USB JMicron"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_JMICRON);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable USB Cypress"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_CYPRESS);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable ASMedia ASM1352R"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_ASM1352R);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable USB Memory"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_MEMORY);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable NVMe JMicron3"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_NVME_JMICRON3);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable NVMe JMicron"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_NVME_JMICRON);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable NVMe ASMedia"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_NVME_ASMEDIA);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable NVMe Realtek"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_NVME_REALTEK);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable MegaRAID"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_MEGA_RAID);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable Intel VROC"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_INTEL_VROC);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable AMD RC2"), &g_ctx.smart_flag, CDI_FLAG_ENABLE_AMD_RC2);

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, gnwinfo_get_text(L"Window (Restart required)"), NK_TEXT_LEFT);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_label(ctx, gnwinfo_get_text(L"Background Info"), &g_ctx.gui_bginfo);
#ifdef GNWINFO_ENABLE_PDH
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, gnwinfo_get_text(L"Enable PDH"), &g_ctx.main_flag, MAIN_NO_PDH);
#endif
	nk_spacer(ctx);
	nk_property_int(ctx, gnwinfo_get_text(L"#Width"), 60, &g_init_width, 1920, 10, 10);
	nk_spacer(ctx);
	nk_property_int(ctx, gnwinfo_get_text(L"#Height"), 80, &g_init_height, 1080, 10, 10);
	nk_spacer(ctx);
	nk_property_int(ctx, gnwinfo_get_text(L"#Alpha"), 10, &g_init_alpha, 255, 5, 10);

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_label(ctx, gnwinfo_get_text(L"Color"), NK_TEXT_LEFT);
	nk_layout_row(ctx, NK_DYNAMIC, 25, 3, (float[3]) { 0.1f, 0.4f, 0.5f });
	nk_spacer(ctx);
	nk_label(ctx, "BACKGROUND", NK_TEXT_LEFT);
	draw_color_picker(ctx, &g_color_back);
	nk_spacer(ctx);
	nk_label(ctx, "HIGHLIGHT", NK_TEXT_LEFT);
	draw_color_picker(ctx, &g_color_text_l);
	nk_spacer(ctx);
	nk_label(ctx, "DEFAULT", NK_TEXT_LEFT);
	draw_color_picker(ctx, &g_color_text_d);
	nk_spacer(ctx);
	nk_label(ctx, "STATE GOOD", NK_TEXT_LEFT);
	draw_color_picker(ctx, &g_color_good);
	nk_spacer(ctx);
	nk_label(ctx, "STATE WARN", NK_TEXT_LEFT);
	draw_color_picker(ctx, &g_color_warning);
	nk_spacer(ctx);
	nk_label(ctx, "STATE ERROR", NK_TEXT_LEFT);
	draw_color_picker(ctx, &g_color_error);
	nk_spacer(ctx);
	nk_label(ctx, "STATE UNKNOWN", NK_TEXT_LEFT);
	draw_color_picker(ctx, &g_color_unknown);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.8f, 0.2f });
	nk_spacer(ctx);
	if (nk_button_label(ctx, gnwinfo_get_text(L"Save")))
	{
		gnwinfo_set_ini_value(L"Widgets", L"MainFlags", L"0x%08X", g_ctx.main_flag);
		gnwinfo_set_ini_value(L"Widgets", L"SmartFormat", L"%d", g_ctx.smart_hex);
		gnwinfo_set_ini_value(L"Widgets", L"SmartFlags", L"0x%08X", g_ctx.smart_flag);
		gnwinfo_set_ini_value(L"Window", L"Width", L"%u", g_init_width);
		gnwinfo_set_ini_value(L"Window", L"Height", L"%u", g_init_height);
		gnwinfo_set_ini_value(L"Window", L"Alpha", L"%u", g_init_alpha);
		gnwinfo_set_ini_value(L"Window", L"BGInfo", L"%d", !g_ctx.gui_bginfo);
		set_ini_color(L"Background", g_color_back);
		set_ini_color(L"Highlight", g_color_text_l);
		set_ini_color(L"Default", g_color_text_d);
		set_ini_color(L"StateGood", g_color_good);
		set_ini_color(L"StateWarn", g_color_warning);
		set_ini_color(L"StateError", g_color_error);
		set_ini_color(L"StateUnknown", g_color_unknown);
	}
out:
	nk_end(ctx);
}
