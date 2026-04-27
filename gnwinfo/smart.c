// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "gettext.h"
#include "../libcdi/libcdi.h"
#include "utils.h"

static struct nk_color
get_attr_color(int status)
{
	switch (status)
	{
	case CDI_DISK_STATUS_GOOD:
		return g_color_good;
	case CDI_DISK_STATUS_CAUTION:
		return g_color_warning;
	case CDI_DISK_STATUS_BAD:
		return g_color_error;
	}
	return g_color_unknown;
}

static inline LPCSTR
get_health_status(enum CDI_DISK_STATUS status)
{
	switch (status)
	{
	case CDI_DISK_STATUS_GOOD:
		return N_(N__GOOD);
	case CDI_DISK_STATUS_CAUTION:
		return N_(N__CAUTION);
	case CDI_DISK_STATUS_BAD:
		return N_(N__BAD);
	}
	return N_(N__UNKNOWN);
}

static void
draw_health(struct nk_context* ctx, CDI_SMART* smart, int disk, float height)
{
	if (nk_group_begin_ex(ctx, "SMART Health", 0))
	{
		int n;
		int health;
		char tmp[32];
		nk_layout_row_dynamic(ctx, height / 5.0f, 1);
		nk_l(ctx, N_(N__HEALTH_STATUS), NK_TEXT_CENTERED);
		n = cdi_get_int(smart, disk, CDI_INT_LIFE);
		health = cdi_get_int(smart, disk, CDI_INT_DISK_STATUS);
		if (n >= 0)
			snprintf(tmp, sizeof(tmp), "%s %d%%", get_health_status(health), n);
		else
			strncpy_s(tmp, sizeof(tmp), get_health_status(health), _TRUNCATE);
		nk_block(ctx, get_attr_color(health), tmp);

		nk_l(ctx, N_(N__TEMPERATURE), NK_TEXT_CENTERED);
		int alarm = cdi_get_int(smart, disk, CDI_INT_TEMPERATURE_ALARM);
		if (alarm <= 0)
			alarm = 60;
		n = cdi_get_int(smart, disk, CDI_INT_TEMPERATURE);
		snprintf(tmp, sizeof(tmp), u8"%.0f %s", NWL_GetTemperature((float)n), g_ctx.temp_unit);
		nk_block(ctx, gnwinfo_get_color((double)n, (double) alarm, 90.0), tmp);
		nk_group_end(ctx);
	}
}

static void
draw_info(struct nk_context* ctx, CDI_SMART* smart, int disk)
{
	INT n;
	DWORD d;
	WCHAR* str;
	if (nk_group_begin_ex(ctx, "SMART Info", 0))
	{
		BOOL is_ssd = cdi_get_bool(smart, disk, CDI_BOOL_SSD);
		BOOL is_nvme = cdi_get_bool(smart, disk, CDI_BOOL_SSD_NVME);
		BOOL has_aam = cdi_get_bool(smart, disk, CDI_BOOL_AAM);
		BOOL has_apm = cdi_get_bool(smart, disk, CDI_BOOL_APM);

		nk_layout_row_dynamic(ctx, 0, 1);
		nk_spacer(ctx);

		nk_layout_row(ctx, NK_DYNAMIC, 0, 4, (float[4]) { 0.2f, 0.4f, 0.24f, 0.16f });

		nk_l(ctx, N_(N__FIRMWARE), NK_TEXT_LEFT);
		str = cdi_get_string(smart, disk, CDI_STRING_FIRMWARE);
		nk_lhc(ctx, NWL_Ucs2ToUtf8(str), NK_TEXT_LEFT, g_color_text_l);
		cdi_free_string(str);
		if (is_ssd)
		{
			n = cdi_get_int(smart, disk, CDI_INT_HOST_READS);
			nk_l(ctx, N_(N__TOTAL_READS), NK_TEXT_LEFT);
			if (n < 0)
				nk_lhc(ctx, "-", NK_TEXT_RIGHT, g_color_text_l);
			else
				nk_lhcf(ctx, NK_TEXT_RIGHT, g_color_text_l, "%d G", n);
		}
		else
		{
			d = cdi_get_dword(smart, disk, CDI_DWORD_BUFFER_SIZE);
			nk_l(ctx, N_(N__BUFFER_SIZE), NK_TEXT_LEFT);
			if (d >= 10 * 1024 * 1024) // 10 MB
				nk_lhcf(ctx, NK_TEXT_RIGHT, g_color_text_l, "%lu M", d / 1024 / 1024);
			else if (d > 1024)
				nk_lhcf(ctx, NK_TEXT_RIGHT, g_color_text_l, "%lu K", d / 1024);
			else
				nk_lhcf(ctx, NK_TEXT_RIGHT, g_color_text_l, "%lu B", d);
		}

		nk_l(ctx, N_(N__S_N), NK_TEXT_LEFT);
		str = cdi_get_string(smart, disk, CDI_STRING_SN);
		nk_lhc(ctx, NWL_Ucs2ToUtf8(str), NK_TEXT_LEFT, g_color_text_l);
		cdi_free_string(str);
		if (is_ssd)
		{
			n = cdi_get_int(smart, disk, CDI_INT_HOST_WRITES);
			nk_l(ctx, N_(N__TOTAL_WRITES), NK_TEXT_LEFT);
			if (n < 0)
				nk_lhc(ctx, "-", NK_TEXT_RIGHT, g_color_text_l);
			else
				nk_lhcf(ctx, NK_TEXT_RIGHT, g_color_text_l, "%d G", n);
		}
		else
		{
			nk_l(ctx, "-", NK_TEXT_CENTERED);
			nk_lhc(ctx, "-", NK_TEXT_RIGHT, g_color_text_l);
		}

		nk_l(ctx, N_(N__INTERFACE), NK_TEXT_LEFT);
		str = cdi_get_string(smart, disk, CDI_STRING_INTERFACE);
		nk_lhc(ctx, NWL_Ucs2ToUtf8(str), NK_TEXT_LEFT, g_color_text_l);
		cdi_free_string(str);
		if (is_ssd && !is_nvme)
		{
			n = cdi_get_int(smart, disk, CDI_INT_NAND_WRITES);
			nk_l(ctx, N_(N__NAND_WRITES), NK_TEXT_LEFT);
			if (n < 0)
				nk_lhc(ctx, "-", NK_TEXT_RIGHT, g_color_text_l);
			else
				nk_lhcf(ctx, NK_TEXT_RIGHT, g_color_text_l, "%d G", n);
		}
		else
		{
			nk_l(ctx, N_(N__RPM), NK_TEXT_LEFT);
			if (is_ssd)
				nk_lhc(ctx, "(SSD)", NK_TEXT_RIGHT, g_color_text_l);
			else
			{
				d = cdi_get_dword(smart, disk, CDI_DWORD_ROTATION_RATE);
				nk_lhcf(ctx, NK_TEXT_RIGHT, g_color_text_l, "%lu", d);
			}
			
		}

		nk_l(ctx, N_(N__MODE), NK_TEXT_LEFT);
		str = cdi_get_string(smart, disk, CDI_STRING_TRANSFER_MODE_CUR);
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s|", NWL_Ucs2ToUtf8(str));
		cdi_free_string(str);
		str = cdi_get_string(smart, disk, CDI_STRING_TRANSFER_MODE_MAX);
		strcat_s(NWLC->NwBuf, NWINFO_BUFSZ, NWL_Ucs2ToUtf8(str));
		cdi_free_string(str);
		nk_lhc(ctx, NWLC->NwBuf, NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__POWER_ON_COUNT), NK_TEXT_LEFT);
		d = cdi_get_dword(smart, disk, CDI_DWORD_POWER_ON_COUNT);
		nk_lhcf(ctx, NK_TEXT_RIGHT, g_color_text_l, "%lu", d);

		nk_l(ctx, N_(N__DRIVE), NK_TEXT_LEFT);
		str = cdi_get_string(smart, disk, CDI_STRING_DRIVE_MAP);
		nk_lhc(ctx, NWL_Ucs2ToUtf8(str), NK_TEXT_LEFT, g_color_text_l);
		cdi_free_string(str);
		nk_l(ctx, N_(N__POWER_ON_HOURS), NK_TEXT_LEFT);
		n = cdi_get_int(smart, disk, CDI_INT_POWER_ON_HOURS);
		if (n < 0)
			nk_lhc(ctx, "-", NK_TEXT_RIGHT, g_color_text_l);
		else
			nk_lhcf(ctx, NK_TEXT_RIGHT, g_color_text_l, "%d", n);

		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.2f, 0.8f });

		nk_l(ctx, N_(N__STANDARD), NK_TEXT_LEFT);
		str = cdi_get_string(smart, disk, CDI_STRING_VERSION_MAJOR);
		nk_lhc(ctx, NWL_Ucs2ToUtf8(str), NK_TEXT_LEFT, g_color_text_l);
		cdi_free_string(str);

		nk_l(ctx, N_(N__FEATURES), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%s%s%s%s%s%s%s%s%s%s",
			cdi_get_bool(smart, disk, CDI_BOOL_SMART) ? "SMART " : "",
			has_aam ?  "AAM " : "",
			has_apm ? "APM " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_NCQ) ? "NCQ " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_NV_CACHE) ? "NVCache " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_DEVSLP) ? "DEVSLP " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_STREAMING) ? "Streaming " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_GPL) ? "GPL " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_TRIM) ? "TRIM " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_VOLATILE_WRITE_CACHE) ? "VolatileWriteCache " : "");

		if (has_aam)
		{
			nk_l(ctx, "AAM", NK_TEXT_LEFT);
			nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%u | %s = %u",
				cdi_get_current_aam(smart, disk), N_(N__RECOMMEND_VAL), cdi_get_recommend_aam(smart, disk));
		}
		if (has_apm)
		{
			nk_l(ctx, "APM", NK_TEXT_LEFT);
			nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%u | %s = %u",
				cdi_get_current_apm(smart, disk), N_(N__RECOMMEND_VAL), cdi_get_recommend_apm(smart, disk));
		}

		nk_group_end(ctx);
	}
}

static WCHAR*
draw_alert_icon(struct nk_context* ctx, BYTE id, int status, LPCWSTR format, WCHAR* value)
{
	static WCHAR hex[18];
	// RawValues(N)
	if (format[0] == L'R')
	{
		wcscpy_s(hex, ARRAYSIZE(hex), value);
		value[0] = '\0';
	}
	// Cur RawValues(N)
	else if (wcsncmp(format, L"Cur R", 5) == 0)
	{
		wcscpy_s(hex, ARRAYSIZE(hex), &value[4]);
		value[4] = L'\0';
	}
	// Cur Wor --- RawValues(N)
	else if (wcsncmp(format, L"Cur Wor --- R", 13) == 0)
	{
		wcscpy_s(hex, ARRAYSIZE(hex), &value[8]);
		value[8] = L'\0';
	}
	// Cur Wor Thr RawValues(N)
	else
	{
		wcscpy_s(hex, ARRAYSIZE(hex), &value[12]);
		value[12] = L'\0';
	}

	nk_block(ctx, get_attr_color(status), "");
	return hex;
}

static void
draw_smart(struct nk_context* ctx, CDI_SMART* smart, int disk)
{
	WCHAR* format;
	DWORD i, count = cdi_get_dword(smart, disk, CDI_DWORD_ATTR_COUNT);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 4, (float[4]) { 0.05f, 0.05f, 0.55f, 0.35f });
	nk_spacer(ctx);
	nk_l(ctx, N_(N__ID), NK_TEXT_LEFT);
	nk_l(ctx, N_(N__ATTRIBUTE), NK_TEXT_LEFT);
	format = cdi_get_smart_format(smart, disk);
	nk_l(ctx, NWL_Ucs2ToUtf8(format), NK_TEXT_LEFT);

	for (i = 0; i < count; i++)
	{
		int id = cdi_get_smart_id(smart, disk, i);
		if (id == 0)
			continue;
		WCHAR* name = cdi_get_smart_name(smart, disk, id);
		WCHAR* value = cdi_get_smart_value(smart, disk, i, g_ctx.smart_hex);
		WCHAR* hex = draw_alert_icon(ctx, id, cdi_get_smart_status(smart, disk, i), format, value);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%02X", id);
		nk_lhc(ctx, NWL_Ucs2ToUtf8(name), NK_TEXT_LEFT, g_color_text_l);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%ls%ls", value, hex);
		cdi_free_string(name);
		cdi_free_string(value);
	}

	cdi_free_string(format);
}

VOID
gnwinfo_draw_smart_window(struct nk_context* ctx, float width, float height)
{
	INT count;
	WCHAR* str;
	CHAR count_str[16];
	static int cur_disk = 0;
	int cur_display;

	(void)width;
	if (NWLC->NwSmartInit == FALSE)
	{
		cdi_init_smart(NWLC->NwSmart, NWLC->NwSmartFlags);
		NWLC->NwSmartInit = TRUE;
	}
	count = cdi_get_disk_count(NWLC->NwSmart);
	if (count <= 0)
	{
		nk_layout_row_dynamic(ctx, 0, 1);
		nk_l(ctx, N_(N__NO_DISKS_FOUND), NK_TEXT_CENTERED);
		return;
	}
	
	snprintf(count_str, sizeof(count_str), "#%d/", count);
	nk_layout_row(ctx, NK_DYNAMIC, g_col_height, 4, (float[4]) { 0.14f, 0.6f, 0.18f, 0.08f });

	cur_display = cur_disk + 1;
	nk_property_int(ctx, count_str, 0, &cur_display, count + 1, 1, 1);
	cur_disk = cur_display - 1;
	if (cur_disk >= count)
		cur_disk = 0;
	else if (cur_disk < 0)
		cur_disk = count - 1;

	str = cdi_get_string(NWLC->NwSmart, cur_disk, CDI_STRING_MODEL);
	nk_lhcf(ctx, NK_TEXT_CENTERED, g_color_text_l,
		"%s %s",
		NWL_Ucs2ToUtf8(str),
		NWL_GetHumanSize(cdi_get_dword(NWLC->NwSmart, cur_disk, CDI_DWORD_DISK_SIZE), &NWLC->NwUnits[2], 1000));
	cdi_free_string(str);
	if (nk_button_image_label(ctx, GET_PNG(IDR_PNG_REFRESH), N_(N__REFRESH), NK_TEXT_CENTERED))
		cdi_update_smart(NWLC->NwSmart, cur_disk);
	g_ctx.smart_hex = !nk_check_label(ctx, N_(N__HEX), !g_ctx.smart_hex);
	
	nk_layout_row(ctx, NK_DYNAMIC, g_col_height * 9.0f, 2, (float[2]) {0.2f, 0.8f});
	draw_health(ctx, NWLC->NwSmart, cur_disk, g_col_height * 9.0f);
	draw_info(ctx, NWLC->NwSmart, cur_disk);

	nk_layout_row_dynamic(ctx, 0, 1);
	draw_smart(ctx, NWLC->NwSmart, cur_disk);
}
