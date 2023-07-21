// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include <shellapi.h>

LPCSTR
gnwinfo_get_node_attr(PNODE node, LPCSTR key)
{
	LPCSTR str;
	if (!node)
		return "-\0";
	str = NWL_NodeAttrGet(node, key);
	if (!str)
		return "-\0";
	return str;
}

static LPCSTR
get_smbios_attr(LPCSTR type, LPCSTR key, BOOL(*cond)(PNODE node))
{
	INT i;
	for (i = 0; g_ctx.smbios->Children[i].LinkedNode; i++)
	{
		PNODE tab = g_ctx.smbios->Children[i].LinkedNode;
		LPCSTR attr = gnwinfo_get_node_attr(tab, "Table Type");
		if (strcmp(attr, type) != 0)
			continue;
		if (!cond || cond(tab) == TRUE)
			return gnwinfo_get_node_attr(tab, key);
	}
	return "-";
}
#if 0
static void
draw_label_cl(struct nk_context* ctx, LPCWSTR label, struct nk_color color)
{
	char* str = gnwinfo_get_ini_value(L"Translation", label, label);
	nk_label_colored(ctx, str, NK_TEXT_LEFT, color);
	free(str);
}
#endif
static void
draw_label_l(struct nk_context* ctx, LPCWSTR label)
{
	char* str = gnwinfo_get_ini_value(L"Translation", label, label);
	nk_label(ctx, str, NK_TEXT_LEFT);
	free(str);
}

static float
draw_icon_label(struct nk_context* ctx, LPCWSTR label, struct nk_image image)
{
	struct nk_rect rect = nk_layout_widget_bounds(ctx);
	float ratio[2];
	ratio[0] = rect.h / rect.w;
	ratio[1] = 1.0f - ratio[0];
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratio);
	nk_image(ctx, image);
	draw_label_l(ctx, label);
	return ratio[0];
}

static struct nk_color
get_percent_color(double percent)
{
	if (percent > 90.0)
		return g_color_error;
	if (percent > 70.0)
		return g_color_warning;
	return g_color_good;
}

static void
draw_percent_prog(struct nk_context* ctx, double percent)
{
	nk_size size = (nk_size)percent;
	struct nk_color color = get_percent_color(percent);
	if (size == 0)
		size = 1;
	else if (size > 100)
		size = 100;
	ctx->style.progress.cursor_normal = nk_style_item_color(color);
	ctx->style.progress.cursor_hover = nk_style_item_color(color);
	ctx->style.progress.cursor_active = nk_style_item_color(color);
	nk_progress(ctx, &size, 100, 0);
}

static VOID
draw_os(struct nk_context* ctx)
{
	float ratio = draw_icon_label(ctx, L"Operating System", g_ctx.image_os);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { ratio, 0.3f, 0.7f - ratio });

	nk_spacer(ctx);
	draw_label_l(ctx, L"Name");
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		g_color_text_l,
		"%s %s (%s)",
		gnwinfo_get_node_attr(g_ctx.system, "OS"),
		gnwinfo_get_node_attr(g_ctx.system, "Processor Architecture"),
		gnwinfo_get_node_attr(g_ctx.system, "Build Number"));

	nk_spacer(ctx);
	nk_spacer(ctx);
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		g_color_text_l,
		"%s@%s%s%s",
		gnwinfo_get_node_attr(g_ctx.system, "Username"),
		gnwinfo_get_node_attr(g_ctx.system, "Computer Name"),
		strcmp(gnwinfo_get_node_attr(g_ctx.system, "Safe Mode"), "Yes") == 0 ? " SafeMode" : "",
		strcmp(gnwinfo_get_node_attr(g_ctx.system, "BitLocker Boot"), "Yes") == 0 ? " BitLocker" : "");

	nk_spacer(ctx);
	draw_label_l(ctx, L"Uptime");
	nk_label_colored(ctx, g_ctx.sys_uptime, NK_TEXT_LEFT, g_color_text_l);
}

static VOID
draw_bios(struct nk_context* ctx)
{
	LPCSTR tpm = gnwinfo_get_node_attr(g_ctx.system, "TPM");
	LPCSTR sb = gnwinfo_get_node_attr(g_ctx.uefi, "Secure Boot");
	float ratio = draw_icon_label(ctx, L"BIOS", g_ctx.image_bios);

	if (sb[0] == 'E')
		sb = " SecureBoot";
	else if (sb[0] == 'D')
		sb = " SecureBootOff";
	else
		sb = "";

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { ratio, 0.3f, 0.7f - ratio });

	nk_spacer(ctx);
	draw_label_l(ctx, L"Firmware");
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		g_color_text_l,
		"%s%s%s%s",
		gnwinfo_get_node_attr(g_ctx.system, "Firmware"),
		sb,
		tpm[0] == 'v' ? " TPM" : "",
		tpm[0] == 'v' ? tpm : "");

	nk_spacer(ctx);
	draw_label_l(ctx, L"Version");
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		g_color_text_l,
		"%s %s",
		get_smbios_attr("0", "Vendor", NULL),
		get_smbios_attr("0", "Version", NULL));
}

static BOOL
is_motherboard(PNODE node)
{
	LPCSTR str = gnwinfo_get_node_attr(node, "Board Type");
	return (strcmp(str, "Motherboard") == 0);
}

static VOID
draw_battery(struct nk_context* ctx, float ratio)
{
	struct nk_color color = g_color_error;
	LPCSTR time = NULL;
	LPCSTR status = gnwinfo_get_node_attr(g_ctx.battery, "Battery Status");
	if (strcmp(status, "Charging") == 0)
	{
		color = g_color_warning;
		time = gnwinfo_get_node_attr(g_ctx.battery, "Battery Life Full");
	}
	else if (strcmp(status, "Not Charging") == 0)
	{
		if (strcmp(gnwinfo_get_node_attr(g_ctx.battery, "AC Power"), "Online") == 0)
			color = g_color_good;
		time = gnwinfo_get_node_attr(g_ctx.battery, "Battery Life Remaining");
	}
	else
		return;
	
	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { ratio, 0.3f, 0.7f - ratio });
	nk_spacer(ctx);
	draw_label_l(ctx, L"Battery");
	nk_labelf_colored(ctx, NK_TEXT_LEFT, color,
		u8"\u26a1 %s %s",
		gnwinfo_get_node_attr(g_ctx.battery, "Battery Life Percentage"),
		time);
}

static VOID
draw_computer(struct nk_context* ctx)
{
	float ratio = draw_icon_label(ctx, L"Computer", g_ctx.image_board);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { ratio, 0.3f, 0.7f - ratio });

	nk_spacer(ctx);
	nk_label(ctx, get_smbios_attr("1", "Manufacturer", NULL), NK_TEXT_LEFT);
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		g_color_text_l,
		"%s %s %s",
		get_smbios_attr("1", "Product Name", NULL),
		get_smbios_attr("3", "Type", NULL),
		get_smbios_attr("1", "Serial Number", NULL));

	nk_spacer(ctx);
	nk_label(ctx, get_smbios_attr("2", "Manufacturer", is_motherboard), NK_TEXT_LEFT);
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		g_color_text_l,
		"%s %s",
		get_smbios_attr("2", "Product Name", is_motherboard),
		get_smbios_attr("2", "Serial Number", is_motherboard));

	draw_battery(ctx, ratio);
}

static uint8_t cache_level = 0;
static BOOL
is_cache_level_equal(PNODE node)
{
	LPCSTR str = gnwinfo_get_node_attr(node, "Cache Level");
	CHAR buf[] = "L1";
	buf[1] = '0' + cache_level;
	return (strcmp(str, buf) == 0);
}

static VOID
draw_processor(struct nk_context* ctx)
{
	INT i, count;
	CHAR name[32];
	float ratio = draw_icon_label(ctx, L"Processor", g_ctx.image_cpu);

	count = strtol(gnwinfo_get_node_attr(g_ctx.cpuid, "Processor Count"), NULL, 10);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 4, (float[4]) { ratio, 0.3f, 0.4f, 0.3f - ratio });
	nk_spacer(ctx);
	draw_label_l(ctx, L"Usage");
	nk_labelf_colored(ctx, NK_TEXT_LEFT, get_percent_color(g_ctx.cpu_usage),
		"%.2f%% %s MHz",
		g_ctx.cpu_usage,
		gnwinfo_get_node_attr(g_ctx.cpuid, "CPU Clock (MHz)"));
	draw_percent_prog(ctx, g_ctx.cpu_usage);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { ratio, 0.3f, 0.7f - ratio });
	for (i = 0; i < count; i++)
	{
		snprintf(name, sizeof(name), "CPU%d", i);
		PNODE cpu = NWL_NodeGetChild(g_ctx.cpuid, name);

		nk_spacer(ctx);
		nk_label(ctx, name, NK_TEXT_LEFT);
		nk_label_colored(ctx,
			gnwinfo_get_node_attr(cpu, "Brand"),
			NK_TEXT_LEFT,
			g_color_text_l);

		nk_spacer(ctx);
		nk_spacer(ctx);
		nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_text_l,
			"%s cores %s threads",
			gnwinfo_get_node_attr(cpu, "Cores"),
			gnwinfo_get_node_attr(cpu, "Logical CPUs"));
	}

	LPCSTR cache_size[4];
	for (cache_level = 1; cache_level <= 4; cache_level++)
		cache_size[cache_level - 1] = get_smbios_attr("7", "Installed Cache Size", is_cache_level_equal);

	nk_spacer(ctx);
	draw_label_l(ctx, L"Cache");
	if (cache_size[0][0] == '-')
		nk_label_colored(ctx, cache_size[0], NK_TEXT_LEFT, g_color_text_l);
	else if (cache_size[1][0] == '-')
		nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_text_l,
			"L1 %s", cache_size[0]);
	else if (cache_size[2][0] == '-')
		nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_text_l,
			"L1 %s L2 %s", cache_size[0], cache_size[1]);
	else if (cache_size[3][0] == '-')
		nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_text_l,
			"L1 %s L2 %s L3 %s", cache_size[0], cache_size[1], cache_size[2]);
	else
		nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_text_l,
			"L1 %s L2 %s L3 %s L4 %s", cache_size[0], cache_size[1], cache_size[2], cache_size[3]);
}

static VOID
draw_memory(struct nk_context* ctx)
{
	INT i;
	float ratio = draw_icon_label(ctx, L"Memory", g_ctx.image_ram);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 4, (float[4]) { ratio, 0.3f, 0.4f, 0.3f - ratio });
	nk_spacer(ctx);
	draw_label_l(ctx, L"Usage");
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		get_percent_color((double)g_ctx.mem_status.dwMemoryLoad),
		"%lu%% %s / %s",
		g_ctx.mem_status.dwMemoryLoad, g_ctx.mem_avail, g_ctx.mem_total);
	draw_percent_prog(ctx, (double)g_ctx.mem_status.dwMemoryLoad);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { ratio, 0.3f, 0.7f - ratio });
	for (i = 0; g_ctx.smbios->Children[i].LinkedNode; i++)
	{
		PNODE tab = g_ctx.smbios->Children[i].LinkedNode;
		LPCSTR attr = gnwinfo_get_node_attr(tab, "Table Type");
		if (strcmp(attr, "17") != 0)
			continue;
		LPCSTR ddr = gnwinfo_get_node_attr(tab, "Device Type");
		if (ddr[0] == '-')
			continue;
		nk_spacer(ctx);
		nk_label(ctx, gnwinfo_get_node_attr(tab, "Bank Locator"), NK_TEXT_LEFT);
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			g_color_text_l,
			"%s-%s %s %s %s",
			ddr,
			gnwinfo_get_node_attr(tab, "Speed (MT/s)"),
			gnwinfo_get_node_attr(tab, "Device Size"),
			gnwinfo_get_node_attr(tab, "Manufacturer"),
			gnwinfo_get_node_attr(tab, "Serial Number"));
	}
}

static VOID
draw_display(struct nk_context* ctx)
{
	INT i, j;
	float ratio = draw_icon_label(ctx, L"Display Devices", g_ctx.image_edid);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { ratio, 0.3f, 0.7f - ratio });

	for (i = 0; g_ctx.pci->Children[i].LinkedNode; i++)
	{
		PNODE pci = g_ctx.pci->Children[i].LinkedNode;
		LPCSTR attr = gnwinfo_get_node_attr(pci, "Class Code");
		if (strncmp("03", attr, 2) != 0)
			continue;
		LPCSTR vendor = gnwinfo_get_node_attr(pci, "Vendor");
		if (strcmp(vendor, "-") == 0)
			continue;
		nk_spacer(ctx);
		nk_label(ctx, vendor, NK_TEXT_LEFT);
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			g_color_text_l,
			"%s",
			gnwinfo_get_node_attr(pci, "Device"));
	}
	for (i = 0; g_ctx.edid->Children[i].LinkedNode; i++)
	{
		CHAR name[32];
		CHAR res[32] = { 0 };
		LPCSTR product = NULL;
		PNODE mon = g_ctx.edid->Children[i].LinkedNode;
		if (!mon)
			continue;
		nk_spacer(ctx);
		nk_label(ctx, gnwinfo_get_node_attr(mon, "Manufacturer"), NK_TEXT_LEFT);
		
		for (j = 0; j < 4; j++)
		{
			PNODE desc;
			snprintf(name, 32, "Descriptor %d", j);
			desc = NWL_NodeGetChild(mon, name);
			if (res[0] == '\0' &&
				strcmp(gnwinfo_get_node_attr(desc, "Type"), "Detailed Timing Descriptor") == 0)
				snprintf(res, 32, "%sx%s@%sHz",
					gnwinfo_get_node_attr(desc, "X Resolution"),
					gnwinfo_get_node_attr(desc, "Y Resolution"),
					gnwinfo_get_node_attr(desc, "Refresh Rate (Hz)"));
			if (product == NULL
				&& strcmp(gnwinfo_get_node_attr(desc, "Type"), "Display Name") == 0)
				product = gnwinfo_get_node_attr(desc, "Text");
		}
		
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			g_color_text_l,
			"%s %s %s %s\"",
			gnwinfo_get_node_attr(mon, "ID"),
			product,
			res,
			gnwinfo_get_node_attr(mon, "Diagonal (in)"));
	}
}

static LPCSTR
get_drive_letter(PNODE volume)
{
	INT i;
	PNODE vol_path_name = NWL_NodeGetChild(volume, "Volume Path Names");
	if (!vol_path_name)
		goto fail;
	for (i = 0; vol_path_name->Children[i].LinkedNode; i++)
	{
		PNODE mnt = vol_path_name->Children[i].LinkedNode;
		LPCSTR attr = gnwinfo_get_node_attr(mnt, "Drive Letter");
		if (attr[0] != '-')
			return attr;
	}
fail:
	return "Volume";
}

static VOID
draw_volume(struct nk_context* ctx, PNODE disk, BOOL cdrom, float ratio)
{
	INT i;
	PNODE vol = NWL_NodeGetChild(disk, "Volumes");
	if (!vol)
		return;
	nk_layout_row(ctx, NK_DYNAMIC, 0, 5, (float[5]) { ratio, 0.1f, 0.2f, 0.4f, 0.3f - ratio });
	for (i = 0; vol->Children[i].LinkedNode; i++)
	{
		struct nk_image img = g_ctx.image_hdd;
		PNODE tab = vol->Children[i].LinkedNode;
		LPCSTR path = gnwinfo_get_node_attr(tab, "Path");
		if (strcmp(path, g_ctx.sys_disk) == 0)
			img = g_ctx.image_sysdisk;
		if (cdrom)
			img = g_ctx.image_cd;
		nk_spacer(ctx);
		nk_spacer(ctx);
		if (nk_button_image_label(ctx, img, get_drive_letter(tab), NK_TEXT_CENTERED))
			ShellExecuteA(NULL, "explore", gnwinfo_get_node_attr(tab, "Volume GUID"), NULL, NULL, SW_NORMAL);
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			g_color_text_l,
			"%s %s %s",
			gnwinfo_get_node_attr(tab, "Total Space"),
			gnwinfo_get_node_attr(tab, "Filesystem"),
			gnwinfo_get_node_attr(tab, "Label"));
		draw_percent_prog(ctx, strtod(gnwinfo_get_node_attr(tab, "Usage"), NULL));
	}
}

static VOID
draw_storage(struct nk_context* ctx)
{
	INT i;
	float ratio = draw_icon_label(ctx, L"Storage", g_ctx.image_disk);
	for (i = 0; g_ctx.disk->Children[i].LinkedNode; i++)
	{
		BOOL cdrom;
		LPCSTR path, id, type;
		PNODE disk = g_ctx.disk->Children[i].LinkedNode;
		if (!disk)
			continue;
		path = gnwinfo_get_node_attr(disk, "Path");
		if (strncmp(path, "\\\\.\\CdRom", 9) == 0)
		{
			cdrom = TRUE;
			id = &path[9];
			type = "CDROM";
		}
		else if (strncmp(path, "\\\\.\\PhysicalDrive", 17) == 0)
		{
			cdrom = FALSE;
			id = &path[17];
			type = strcmp(gnwinfo_get_node_attr(disk, "SSD"), "Yes") == 0 ? "SSD" : "HDD";
		}
		else
			continue;

		nk_layout_row(ctx, NK_DYNAMIC, 0, 4, (float[4]) { ratio, 0.1f, 0.2f, 0.7f - ratio });
		nk_spacer(ctx);
		nk_labelf(ctx, NK_TEXT_LEFT,
			"%s%s",
			cdrom ? "CDROM" : "DISK",
			id);
		nk_labelf(ctx, NK_TEXT_LEFT,
			"%s %s",
			gnwinfo_get_node_attr(disk, "Type"),
			type);
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			g_color_text_l,
			"%s %s %s",
			gnwinfo_get_node_attr(disk, "Size"),
			gnwinfo_get_node_attr(disk, "Partition Table"),
			gnwinfo_get_node_attr(disk, "Product ID"));

		LPCSTR health = gnwinfo_get_node_attr(disk, "Health Status");
		if ((g_ctx.main_flag & MAIN_DISK_SMART) && strcmp(health, "-") != 0)
		{
			struct nk_color color = g_color_warning;
			LPCSTR temp = gnwinfo_get_node_attr(disk, "Temperature (C)");
			if (strncmp(health, "Good", 4) == 0)
				color = g_color_good;
			else if (strncmp(health, "Bad", 3) == 0)
				color = g_color_error;
			nk_spacer(ctx);
			nk_spacer(ctx);
			nk_label(ctx, "S.M.A.R.T.", NK_TEXT_LEFT);
			nk_labelf_colored(ctx, NK_TEXT_LEFT,
				color, u8"%s %s\u00B0C", health,
				temp[0] == '-' ? "-" : temp);
		}
		draw_volume(ctx, disk, cdrom, ratio);
	}
}

static LPCSTR
get_first_ipv4(PNODE node)
{
	INT i;
	PNODE unicasts = NWL_NodeGetChild(node, "Unicasts");
	if (!unicasts)
		return "";
	for (i = 0; unicasts->Children[i].LinkedNode; i++)
	{
		PNODE ip = unicasts->Children[i].LinkedNode;
		LPCSTR addr = gnwinfo_get_node_attr(ip, "IPv4");
		if (strcmp(addr, "-") != 0)
			return addr;
	}
	return "";
}

static VOID
draw_network(struct nk_context* ctx)
{
	INT i;
	float ratio = draw_icon_label(ctx, L"Network", g_ctx.image_net);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { ratio, 0.6f, 0.4f - ratio });
	nk_spacer(ctx);
	draw_label_l(ctx, L"Traffic /s");
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		g_color_text_l,
		u8"\u2191 %s \u2193 %s", g_ctx.net_send, g_ctx.net_recv);

	for (i = 0; g_ctx.network->Children[i].LinkedNode; i++)
	{
		BOOL is_active = FALSE;
		PNODE nw = g_ctx.network->Children[i].LinkedNode;
		struct nk_color color = g_color_error;
		if (!nw)
			continue;
		if (strcmp(gnwinfo_get_node_attr(nw, "Type"), "Software Loopback") == 0)
			continue;
		if (strcmp(gnwinfo_get_node_attr(nw, "Status"), "Active") == 0)
		{
			color = g_color_good;
			is_active = TRUE;
		}
		else if (!(g_ctx.main_flag & MAIN_NET_INACTIVE))
			continue;
		nk_spacer(ctx);
		nk_label(ctx, gnwinfo_get_node_attr(nw, "Description"), NK_TEXT_LEFT);
		nk_labelf_colored(ctx,
			NK_TEXT_LEFT, color,
			"%s%s",
			strcmp(gnwinfo_get_node_attr(nw, "DHCP Enabled"), "Yes") == 0 ? "DHCP " : "",
			get_first_ipv4(nw));

		if (g_ctx.main_flag & MAIN_NET_DETAIL)
		{
			nk_spacer(ctx);
			if (is_active)
				nk_labelf(ctx, NK_TEXT_LEFT, u8"    \u2191\u2193 %s / %s",
					gnwinfo_get_node_attr(nw, "Transmit Link Speed"),
					gnwinfo_get_node_attr(nw, "Receive Link Speed"));
			else
				nk_spacer(ctx);
			nk_label_colored(ctx,
				gnwinfo_get_node_attr(nw, "MAC Address"),
				NK_TEXT_LEFT, g_color_text_l);
		}
	}
}

VOID
gnwinfo_draw_main_window(struct nk_context* ctx, float width, float height)
{
	if (!nk_begin(ctx, "Summary",
		nk_rect(0, 0, width, height),
		NK_WINDOW_BACKGROUND))
		goto out;
	nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 6);
	struct nk_rect rect = nk_layout_widget_bounds(ctx);
	float ratio = rect.h / rect.w;

	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_cpuid))
		g_ctx.gui_cpuid = TRUE;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_smart))
		g_ctx.gui_smart = TRUE;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_pci))
		g_ctx.gui_pci = TRUE;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_dmi))
		g_ctx.gui_dmi = TRUE;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_set))
		g_ctx.gui_settings = TRUE;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_refresh))
		gnwinfo_ctx_update(IDT_TIMER_1M);
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_info))
		g_ctx.gui_about = TRUE;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_close))
		gnwinfo_ctx_exit();
	nk_layout_row_end(ctx);

	if (g_ctx.main_flag & MAIN_INFO_OS)
		draw_os(ctx);
	if (g_ctx.main_flag & MAIN_INFO_BIOS)
		draw_bios(ctx);
	if (g_ctx.main_flag & MAIN_INFO_BOARD)
		draw_computer(ctx);
	if (g_ctx.main_flag & MAIN_INFO_CPU)
		draw_processor(ctx);
	if (g_ctx.main_flag & MAIN_INFO_MEMORY)
		draw_memory(ctx);
	if (g_ctx.main_flag & MAIN_INFO_MONITOR)
		draw_display(ctx);
	if (g_ctx.main_flag & MAIN_INFO_STORAGE)
		draw_storage(ctx);
	if (g_ctx.main_flag & MAIN_INFO_NETWORK)
		draw_network(ctx);

out:
	nk_end(ctx);
}
