// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "gettext.h"
#include "../libcdi/libcdi.h"

LPCSTR NWL_Ucs2ToUtf8(LPCWSTR src);
LPCWSTR NWL_Utf8ToUcs2(LPCSTR src);

WCHAR g_ini_path[MAX_PATH] = { 0 };

static WCHAR m_buf[MAX_PATH];

LPCSTR
gnwinfo_get_ini_value(LPCWSTR section, LPCWSTR key, LPCWSTR fallback)
{
	GetPrivateProfileStringW(section, key, fallback, m_buf, MAX_PATH, g_ini_path);
	return NWL_Ucs2ToUtf8(m_buf);
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

static inline void
set_ini_color(LPCWSTR key, struct nk_color color)
{
	gnwinfo_set_ini_value(L"Color", key, L"%02X%02X%02X", color.r, color.g, color.b);
}

#define set_label(ctx, text) nk_l(ctx, text, NK_TEXT_LEFT)

static void
draw_color_picker(struct nk_context* ctx, struct nk_color* color)
{
	if (nk_combo_begin_color_dynamic(ctx, *color))
	{
		float ratios[] = { 0.15f, 0.85f };
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratios);
		set_label(ctx, "R:");
		color->r = (nk_byte)nk_slide_int(ctx, 0, color->r, 255, 5);
		set_label(ctx, "G:");
		color->g = (nk_byte)nk_slide_int(ctx, 0, color->g, 255, 5);
		set_label(ctx, "B:");
		color->b = (nk_byte)nk_slide_int(ctx, 0, color->b, 255, 5);
		nk_combo_end(ctx);
	}
}

VOID
gnwinfo_draw_settings_window(struct nk_context* ctx, float width, float height)
{
	if (!(g_ctx.window_flag & GUI_WINDOW_SETTINGS))
		return;
	if (!nk_begin_ex(ctx, N_(N__SETTINGS),
		nk_rect(width / 4.0f, height / 3.0f, width / 2.0f, height / 3.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE,
		GET_PNG(IDR_PNG_SETTINGS), GET_PNG(IDR_PNG_CLOSE)))
	{
		g_ctx.window_flag &= ~GUI_WINDOW_SETTINGS;
		goto out;
	}

	nk_layout_row_dynamic(ctx, 0, 1);
	set_label(ctx, N_(N__HIDE_COMPONENTS));

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.1f, 0.45f, 0.45f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__OS), &g_ctx.main_flag, MAIN_INFO_OS);
	nk_checkbox_flags_label(ctx, N_(N__BIOS), &g_ctx.main_flag, MAIN_INFO_BIOS);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__PC), &g_ctx.main_flag, MAIN_INFO_BOARD);
	nk_checkbox_flags_label(ctx, N_(N__CPU), &g_ctx.main_flag, MAIN_INFO_CPU);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__MEMORY), &g_ctx.main_flag, MAIN_INFO_MEMORY);
	nk_checkbox_flags_label(ctx, N_(N__DISPLAY), &g_ctx.main_flag, MAIN_INFO_MONITOR);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__STORAGE), &g_ctx.main_flag, MAIN_INFO_STORAGE);
	nk_checkbox_flags_label(ctx, N_(N__NETWORK), &g_ctx.main_flag, MAIN_INFO_NETWORK);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__AUDIO), &g_ctx.main_flag, MAIN_INFO_AUDIO);
	nk_spacer(ctx);

	nk_layout_row_dynamic(ctx, 0, 1);
	set_label(ctx, N_(N__OS));
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__HIDE_EDITION_ID), &g_ctx.main_flag, MAIN_OS_EDITIONID);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__HIDE_BUILD_NUMBER), &g_ctx.main_flag, MAIN_OS_BUILD);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__HIDE_UPTIME), &g_ctx.main_flag, MAIN_OS_UPTIME);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__HIDE_LOGIN), &g_ctx.main_flag, MAIN_OS_DETAIL);

	nk_layout_row_dynamic(ctx, 0, 1);
	set_label(ctx, N_(N__BIOS));
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__HIDE_VENDOR), &g_ctx.main_flag, MAIN_B_VENDOR);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__HIDE_VERSION), &g_ctx.main_flag, MAIN_B_VERSION);

	nk_layout_row_dynamic(ctx, 0, 1);
	set_label(ctx, N_(N__CPU));
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__HIDE_DETAILS), &g_ctx.main_flag, MAIN_CPU_DETAIL);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__HIDE_CACHE), &g_ctx.main_flag, MAIN_CPU_CACHE);

	nk_layout_row_dynamic(ctx, 0, 1);
	set_label(ctx, N_(N__MEMORY));
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__HIDE_DETAILS), &g_ctx.main_flag, MAIN_MEM_DETAIL);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__USE_SMBUS_SPD), &g_ctx.main_flag, MAIN_SMBUS_SPD);

	nk_layout_row_dynamic(ctx, 0, 1);
	set_label(ctx, N_(N__NETWORK));
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__USE_BIT_UNITS), &g_ctx.main_flag, MAIN_NET_UNIT_B);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__HIDE_INACTIVE_NETWORK), &g_ctx.main_flag, MAIN_NET_INACTIVE);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__HIDE_DETAILS), &g_ctx.main_flag, MAIN_NET_DETAIL);

	nk_layout_row_dynamic(ctx, 0, 1);
	set_label(ctx, N_(N__STORAGE));
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__COMPACT_VIEW), &g_ctx.main_flag, MAIN_DISK_COMPACT);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__HIDE_DISK_SPACE_BAR), &g_ctx.main_flag, MAIN_VOLUME_PROG);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__HIDE_SMART), &g_ctx.main_flag, MAIN_DISK_SMART);
	nk_spacer(ctx);
	g_ctx.smart_hex = !nk_check_label(ctx, N_(N__DISPLAY_SMART_HEX), !g_ctx.smart_hex);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__DISK_USE_WMI), &g_ctx.smart_flag, CDI_FLAG_USE_WMI);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ADV_DISK_SEARCH), &g_ctx.smart_flag, CDI_FLAG_ADVANCED_SEARCH);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__HD204UI_WORKAROUND), &g_ctx.smart_flag, CDI_FLAG_WORKAROUND_HD204UI);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ADATA_SSD_WORKAROUND), &g_ctx.smart_flag, CDI_FLAG_WORKAROUND_ADATA);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ATA_PASS_THROUGH), &g_ctx.smart_flag, CDI_FLAG_ATA_PASS_THROUGH);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_NVIDIA_CTRL), &g_ctx.smart_flag, CDI_FLAG_ENABLE_NVIDIA);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_MARVELL_CTRL), &g_ctx.smart_flag, CDI_FLAG_ENABLE_MARVELL);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_USB_SAT), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_SAT);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_USB_IO_DATA), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_IODATA);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_USB_SUNPLUS), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_SUNPLUS);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_USB_LOGITEC), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_LOGITEC);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_USB_PROLIFIC), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_PROLIFIC);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_USB_JMICRON), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_JMICRON);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_USB_CYPRESS), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_CYPRESS);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_ASMEDIA_ASM1352R), &g_ctx.smart_flag, CDI_FLAG_ENABLE_ASM1352R);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_USB_MEMORY), &g_ctx.smart_flag, CDI_FLAG_ENABLE_USB_MEMORY);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_NVME_JMICRON), &g_ctx.smart_flag, CDI_FLAG_ENABLE_NVME_JMICRON);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_NVME_ASMEDIA), &g_ctx.smart_flag, CDI_FLAG_ENABLE_NVME_ASMEDIA);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_NVME_REALTEK), &g_ctx.smart_flag, CDI_FLAG_ENABLE_NVME_REALTEK);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_MEGARAID), &g_ctx.smart_flag, CDI_FLAG_ENABLE_MEGA_RAID);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_INTEL_VROC), &g_ctx.smart_flag, CDI_FLAG_ENABLE_INTEL_VROC);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_AMD_RC2), &g_ctx.smart_flag, CDI_FLAG_ENABLE_AMD_RC2);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__ENABLE_REALTEK_9220DP), &g_ctx.smart_flag, CDI_FLAG_ENABLE_REALTEK_9220DP);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__HIDE_RAID_VOLUME), &g_ctx.smart_flag, CDI_FLAG_HIDE_RAID_VOLUME);

	nk_layout_row_dynamic(ctx, 0, 1);
	set_label(ctx, N_(N__WINDOW_RESTART));
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.1f, 0.9f });
	nk_spacer(ctx);
	nk_checkbox_label(ctx, N_(N__BACKGROUND_INFO), &g_ctx.gui_bginfo);
	nk_spacer(ctx);
	nk_checkbox_flags_label(ctx, N_(N__NO_QUICK_ACCESS), &g_ctx.main_flag, MAIN_NO_QUICK);

	nk_spacer(ctx);
	nk_checkbox_label(ctx, N_(N__SHOW_SENSITIVE_DATA), &g_ctx.lib.HideSensitive);
	nk_spacer(ctx);
	nk_checkbox_label(ctx, N_(N__DISABLE_DPI_SCALING), &g_dpi_scaling);
	nk_spacer(ctx);
	nk_checkbox_label(ctx, N_(N__DISABLE_ANTIALIASING), &g_ctx.gui_aa);
	nk_spacer(ctx);
	nk_property_int(ctx, N_(N__WIDTH), 60, &g_init_width, 1920, 10, 10);
	nk_spacer(ctx);
	nk_property_int(ctx, N_(N__HEIGHT), 80, &g_init_height, 1080, 10, 10);
	nk_spacer(ctx);
	nk_property_int(ctx, N_(N__ALPHA), 10, &g_init_alpha, 255, 5, 10);

	nk_layout_row_dynamic(ctx, 0, 1);
	set_label(ctx, N_(N__COLOR));
	nk_layout_row(ctx, NK_DYNAMIC, 25, 3, (float[3]) { 0.1f, 0.4f, 0.5f });
	nk_spacer(ctx);
	set_label(ctx, "BACKGROUND");
	draw_color_picker(ctx, &g_color_back);
	nk_spacer(ctx);
	set_label(ctx, "HIGHLIGHT");
	draw_color_picker(ctx, &g_color_text_l);
	nk_spacer(ctx);
	set_label(ctx, "DEFAULT");
	draw_color_picker(ctx, &g_color_text_d);
	nk_spacer(ctx);
	set_label(ctx, "STATE GOOD");
	draw_color_picker(ctx, &g_color_good);
	nk_spacer(ctx);
	set_label(ctx, "STATE WARN");
	draw_color_picker(ctx, &g_color_warning);
	nk_spacer(ctx);
	set_label(ctx, "STATE ERROR");
	draw_color_picker(ctx, &g_color_error);
	nk_spacer(ctx);
	set_label(ctx, "STATE UNKNOWN");
	draw_color_picker(ctx, &g_color_unknown);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.8f, 0.2f });
	nk_spacer(ctx);
	if (nk_button_label(ctx, N_(N__SAVE)))
	{
		gnwinfo_set_ini_value(L"Widgets", L"MainFlags", L"0x%08X", g_ctx.main_flag);
		gnwinfo_set_ini_value(L"Widgets", L"SmartFormat", L"%d", g_ctx.smart_hex);
		gnwinfo_set_ini_value(L"Widgets", L"SmartFlags", L"0x%08X", g_ctx.smart_flag);
		gnwinfo_set_ini_value(L"Window", L"Width", L"%u", g_init_width);
		gnwinfo_set_ini_value(L"Window", L"Height", L"%u", g_init_height);
		gnwinfo_set_ini_value(L"Window", L"Alpha", L"%u", g_init_alpha);
		gnwinfo_set_ini_value(L"Window", L"BGInfo", L"%d", !g_ctx.gui_bginfo);
		gnwinfo_set_ini_value(L"Window", L"HideSensitive", L"%u", g_ctx.lib.HideSensitive);
		gnwinfo_set_ini_value(L"Window", L"DpiScaling", L"%d", g_dpi_scaling);
		gnwinfo_set_ini_value(L"Window", L"AntiAliasing", L"%u", g_ctx.gui_aa);
		set_ini_color(L"Background", g_color_back);
		set_ini_color(L"Highlight", g_color_text_l);
		set_ini_color(L"Default", g_color_text_d);
		set_ini_color(L"StateGood", g_color_good);
		set_ini_color(L"StateWarn", g_color_warning);
		set_ini_color(L"StateError", g_color_error);
		set_ini_color(L"StateUnknown", g_color_unknown);
		g_ctx.window_flag &= ~GUI_WINDOW_SETTINGS;
	}
out:
	nk_end(ctx);
}
