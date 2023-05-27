// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "../libcdi/libcdi.h"

LPCSTR NWL_GetHumanSize(UINT64 size, LPCSTR human_sizes[6], UINT64 base);
static const char* disk_human_sizes[6] =
{ "MB", "GB", "TB", "PB", "EB", "ZB", };

static void
draw_rect(struct nk_context* ctx, struct nk_color bg, const char* str)
{
	static struct nk_style_button style;
	memcpy(&style, &ctx->style.button, sizeof(struct nk_style_button));
	style.normal.type = NK_STYLE_ITEM_COLOR;
	style.normal.data.color = bg;
	style.hover.type = NK_STYLE_ITEM_COLOR;
	style.hover.data.color = bg;
	style.active.type = NK_STYLE_ITEM_COLOR;
	style.active.data.color = bg;
	style.text_normal = NK_COLOR_BLACK;
	style.text_hover = NK_COLOR_BLACK;
	style.text_active = NK_COLOR_BLACK;
	nk_button_label_styled(ctx, &style, str);
}

static void
draw_health(struct nk_context* ctx, CDI_SMART* smart, int disk, float height)
{
	int n;
	char* str;
	char tmp[32];

	if (nk_group_begin(ctx, "SMART Health", 0))
	{
		struct nk_color color = NK_COLOR_YELLOW;
		nk_layout_row_dynamic(ctx, height / 5.0f, 1);
		nk_label(ctx, "Health Status", NK_TEXT_CENTERED);
		n = cdi_get_int(smart, disk, CDI_INT_LIFE);
		str = cdi_get_string(smart, disk, CDI_STRING_DISK_STATUS);
		if (strncmp(str, "Good", 4) == 0)
			color = NK_COLOR_GREEN;
		else if (strncmp(str, "Bad", 3) == 0)
			color = NK_COLOR_RED;
		if (n >= 0)
			snprintf(tmp, sizeof(tmp), "%s\n%d%%", str, n);
		else
			snprintf(tmp, sizeof(tmp), "%s", str);
		draw_rect(ctx, color, tmp);
		cdi_free_string(str);
		nk_label(ctx, "Temperature", NK_TEXT_CENTERED);
		color = NK_COLOR_YELLOW;
		int alarm = cdi_get_int(smart, disk, CDI_INT_TEMPERATURE_ALARM);
		if (alarm <= 0)
			alarm = 60;
		n = cdi_get_int(smart, disk, CDI_INT_TEMPERATURE);
		if (n > 0 && n < alarm)
			color = NK_COLOR_GREEN;
		snprintf(tmp, sizeof(tmp), u8"%d ¡ãC", n);
		draw_rect(ctx, color, tmp);
		nk_group_end(ctx);
	}
}

static void
draw_info(struct nk_context* ctx, CDI_SMART* smart, int disk)
{
	INT n;
	DWORD d;
	char* str;
	char* tmp;
	if (nk_group_begin(ctx, "SMART Info", 0))
	{
		BOOL is_ssd = cdi_get_bool(smart, disk, CDI_BOOL_SSD);
		BOOL is_nvme = cdi_get_bool(smart, disk, CDI_BOOL_SSD_NVME);

		nk_layout_row_dynamic(ctx, 0, 1);
		nk_spacer(ctx);

		nk_layout_row(ctx, NK_DYNAMIC, 0, 4, (float[4]) { 0.2f, 0.4f, 0.24f, 0.16f });

		nk_label(ctx, "Firmware", NK_TEXT_LEFT);
		str = cdi_get_string(smart, disk, CDI_STRING_FIRMWARE);
		nk_label_colored(ctx, str, NK_TEXT_LEFT, NK_COLOR_WHITE);
		cdi_free_string(str);
		if (is_ssd)
		{
			n = cdi_get_int(smart, disk, CDI_INT_HOST_READS);
			nk_label(ctx, "Total Reads", NK_TEXT_LEFT);
			if (n < 0)
				nk_label_colored(ctx, "-", NK_TEXT_RIGHT, NK_COLOR_WHITE);
			else
				nk_labelf_colored(ctx, NK_TEXT_RIGHT, NK_COLOR_WHITE, "%d G", n);
		}
		else
		{
			d = cdi_get_dword(smart, disk, CDI_DWORD_BUFFER_SIZE);
			nk_label(ctx, "Buffer Size", NK_TEXT_LEFT);
			if (d >= 10 * 1024 * 1024) // 10 MB
				nk_labelf_colored(ctx, NK_TEXT_RIGHT, NK_COLOR_WHITE, "%lu M", d / 1024 / 1024);
			else if (d > 1024)
				nk_labelf_colored(ctx, NK_TEXT_RIGHT, NK_COLOR_WHITE, "%lu K", d / 1024);
			else
				nk_labelf_colored(ctx, NK_TEXT_RIGHT, NK_COLOR_WHITE, "%lu B", d);
		}

		nk_label(ctx, "S / N", NK_TEXT_LEFT);
		str = cdi_get_string(smart, disk, CDI_STRING_SN);
		nk_label_colored(ctx, str, NK_TEXT_LEFT, NK_COLOR_WHITE);
		cdi_free_string(str);
		if (is_ssd)
		{
			n = cdi_get_int(smart, disk, CDI_INT_HOST_WRITES);
			nk_label(ctx, "Total Writes", NK_TEXT_LEFT);
			if (n < 0)
				nk_label_colored(ctx, "-", NK_TEXT_RIGHT, NK_COLOR_WHITE);
			else
				nk_labelf_colored(ctx, NK_TEXT_RIGHT, NK_COLOR_WHITE, "%d G", n);
		}
		else
		{
			nk_label(ctx, "-", NK_TEXT_CENTERED);
			nk_label_colored(ctx, "-", NK_TEXT_RIGHT, NK_COLOR_WHITE);
		}

		nk_label(ctx, "Interface", NK_TEXT_LEFT);
		str = cdi_get_string(smart, disk, CDI_STRING_INTERFACE);
		nk_label_colored(ctx, str, NK_TEXT_LEFT, NK_COLOR_WHITE);
		cdi_free_string(str);
		if (is_ssd && !is_nvme)
		{
			n = cdi_get_int(smart, disk, CDI_INT_NAND_WRITES);
			nk_label(ctx, "NAND Writes", NK_TEXT_LEFT);
			if (n < 0)
				nk_label_colored(ctx, "-", NK_TEXT_RIGHT, NK_COLOR_WHITE);
			else
				nk_labelf_colored(ctx, NK_TEXT_RIGHT, NK_COLOR_WHITE, "%d G", n);
		}
		else
		{
			nk_label(ctx, "RPM", NK_TEXT_LEFT);
			if (is_ssd)
				nk_label_colored(ctx, "(SSD)", NK_TEXT_RIGHT, NK_COLOR_WHITE);
			else
			{
				d = cdi_get_dword(smart, disk, CDI_DWORD_ROTATION_RATE);
				nk_labelf_colored(ctx, NK_TEXT_RIGHT, NK_COLOR_WHITE, "%lu", d);
			}
			
		}

		nk_label(ctx, "Mode", NK_TEXT_LEFT);
		str = cdi_get_string(smart, disk, CDI_STRING_TRANSFER_MODE_CUR);
		tmp = cdi_get_string(smart, disk, CDI_STRING_TRANSFER_MODE_MAX);
		nk_labelf_colored(ctx, NK_TEXT_LEFT, NK_COLOR_WHITE, "%s|%s", str, tmp);
		cdi_free_string(str);
		cdi_free_string(tmp);
		nk_label(ctx, "Power On Count", NK_TEXT_LEFT);
		d = cdi_get_dword(smart, disk, CDI_DWORD_POWER_ON_COUNT);
		nk_labelf_colored(ctx, NK_TEXT_RIGHT, NK_COLOR_WHITE, "%lu", d);

		nk_label(ctx, "Drive", NK_TEXT_LEFT);
		str = cdi_get_string(smart, disk, CDI_STRING_DRIVE_MAP);
		nk_label_colored(ctx, str, NK_TEXT_LEFT, NK_COLOR_WHITE);
		cdi_free_string(str);
		nk_label(ctx, "Power On Hours", NK_TEXT_LEFT);
		n = cdi_get_int(smart, disk, CDI_INT_POWER_ON_HOURS);
		if (n < 0)
			nk_label_colored(ctx, "-", NK_TEXT_RIGHT, NK_COLOR_WHITE);
		else
			nk_labelf_colored(ctx, NK_TEXT_RIGHT, NK_COLOR_WHITE, "%d", n);

		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.2f, 0.8f });

		nk_label(ctx, "Standard", NK_TEXT_LEFT);
		str = cdi_get_string(smart, disk, CDI_STRING_VERSION_MAJOR);
		nk_label_colored(ctx, str, NK_TEXT_LEFT, NK_COLOR_WHITE);
		cdi_free_string(str);

		nk_label(ctx, "Features", NK_TEXT_LEFT);
		nk_labelf_colored(ctx, NK_TEXT_LEFT, NK_COLOR_WHITE, "%s%s%s%s%s%s%s%s%s%s",
			cdi_get_bool(smart, disk, CDI_BOOL_SMART) ? "SMART " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_AAM) ?  "AAM " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_APM) ? "APM " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_NCQ) ? "NCQ " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_NV_CACHE) ? "NVCache " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_DEVSLP) ? "DEVSLP " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_STREAMING) ? "Streaming " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_GPL) ? "GPL " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_TRIM) ? "TRIM " : "",
			cdi_get_bool(smart, disk, CDI_BOOL_VOLATILE_WRITE_CACHE) ? "VolatileWriteCache " : "");

		nk_group_end(ctx);
	}
}

static void
draw_smart(struct nk_context* ctx, CDI_SMART* smart, int disk)
{
	char* str;
	if (nk_group_begin(ctx, "SMART Attr", NK_WINDOW_BORDER))
	{
		CDI_SMART_ATTRIBUTE* attr = cdi_get_smart_attribute(smart, disk);
		DWORD i, count = cdi_get_dword(smart, disk, CDI_DWORD_ATTR_COUNT);
		nk_layout_row(ctx, NK_DYNAMIC, 0, 4, (float[4]) { 0.05f, 0.05f, 0.55f, 0.35f });
		nk_spacer(ctx);
		nk_label(ctx, "ID", NK_TEXT_LEFT);
		nk_label(ctx, "Attribute", NK_TEXT_LEFT);
		str = cdi_get_smart_attribute_format(smart, disk);
		nk_label(ctx, str, NK_TEXT_LEFT);
		cdi_free_string(str);

		for (i = 0; i < count; i++)
		{
			if (attr[i].Id == 0)
				continue;
			draw_rect(ctx, NK_COLOR_BLUE, "");
			nk_labelf_colored(ctx, NK_TEXT_LEFT, NK_COLOR_WHITE, "%02X", attr[i].Id);
			str = cdi_get_smart_attribute_name(smart, disk, attr[i].Id);
			nk_label_colored(ctx, str, NK_TEXT_LEFT, NK_COLOR_WHITE);
			cdi_free_string(str);
			str = cdi_get_smart_attribute_value(smart, disk, i);
			nk_label_colored(ctx, str, NK_TEXT_LEFT, NK_COLOR_WHITE);
			cdi_free_string(str);
		}

		nk_group_end(ctx);
	}
}

VOID
gnwinfo_draw_smart_window(struct nk_context* ctx, float width, float height)
{
	INT count;
	char* str;
	static int cur_disk = 0;

	if (g_ctx.gui_smart == FALSE)
		return;
	if (!nk_begin(ctx, "S.M.A.R.T.",
		nk_rect(0, height / 6.0f, width, height / 1.5f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE))
	{
		g_ctx.gui_smart = FALSE;
		goto out;
	}

	count = cdi_get_disk_count(NWLC->NwSmart);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.2f, 0.8f });
	nk_property_int(ctx, "DISK", 0, &cur_disk, count - 1, 1, 1);
	str = cdi_get_string(NWLC->NwSmart, cur_disk, CDI_STRING_MODEL);
	nk_labelf_colored(ctx, NK_TEXT_CENTERED, NK_COLOR_WHITE,
		"%s %s",
		str,
		NWL_GetHumanSize(cdi_get_dword(NWLC->NwSmart, cur_disk, CDI_DWORD_DISK_SIZE), disk_human_sizes, 1000));
	cdi_free_string(str);

	
	nk_layout_row(ctx, NK_DYNAMIC, height / 4.0f, 2, (float[2]) {0.2f, 0.8f});
	draw_health(ctx, NWLC->NwSmart, cur_disk, height / 4.0f);
	draw_info(ctx, NWLC->NwSmart, cur_disk);

	nk_layout_row_dynamic(ctx, height / 3.0f, 1);
	draw_smart(ctx, NWLC->NwSmart, cur_disk);

out:
	nk_end(ctx);
}
