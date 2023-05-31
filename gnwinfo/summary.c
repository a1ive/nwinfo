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
	struct nk_color color = g_color_good;
	float ratio = draw_icon_label(ctx, L"Processor", g_ctx.image_cpu);

	if (g_ctx.cpu_usage > 60.0)
		color = g_color_warning;
	if (g_ctx.cpu_usage > 80.0)
		color = g_color_error;

	count = strtol(gnwinfo_get_node_attr(g_ctx.cpuid, "Processor Count"), NULL, 10);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { ratio, 0.3f, 0.7f - ratio });

	nk_spacer(ctx);
	draw_label_l(ctx, L"Usage");
	nk_labelf_colored(ctx, NK_TEXT_LEFT, color,
		"%.2f%% %s MHz",
		g_ctx.cpu_usage,
		gnwinfo_get_node_attr(g_ctx.cpuid, "CPU Clock (MHz)"));

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
	struct nk_color color = g_color_good;
	float ratio = draw_icon_label(ctx, L"Memory", g_ctx.image_ram);

	if (g_ctx.mem_status.dwMemoryLoad > 60)
		color = g_color_warning;
	if (g_ctx.mem_status.dwMemoryLoad > 80)
		color = g_color_error;

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { ratio, 0.3f, 0.7f - ratio });

	nk_spacer(ctx);
	draw_label_l(ctx, L"Usage");
	nk_labelf_colored(ctx, NK_TEXT_LEFT, color,
		"%lu%% %s / %s",
		g_ctx.mem_status.dwMemoryLoad, g_ctx.mem_avail, g_ctx.mem_total);

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
	INT i;
	float ratio = draw_icon_label(ctx, L"Display Devices", g_ctx.image_edid);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { ratio, 0.3f, 0.7f - ratio });

	for (i = 0; g_ctx.pci->Children[i].LinkedNode; i++)
	{
		PNODE pci = g_ctx.pci->Children[i].LinkedNode;
		LPCSTR attr = gnwinfo_get_node_attr(pci, "Class Code");
		if (strncmp("03", attr, 2) != 0)
			continue;
		nk_spacer(ctx);
		nk_label(ctx, gnwinfo_get_node_attr(pci, "Vendor"), NK_TEXT_LEFT);
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			g_color_text_l,
			"%s",
			gnwinfo_get_node_attr(pci, "Device"));
	}
	for (i = 0; g_ctx.edid->Children[i].LinkedNode; i++)
	{
		PNODE mon = g_ctx.edid->Children[i].LinkedNode;
		PNODE res, sz;
		if (!mon)
			continue;
		res = NWL_NodeGetChild(mon, "Resolution");
		sz = NWL_NodeGetChild(mon, "Screen Size");
		nk_spacer(ctx);
		nk_label(ctx, gnwinfo_get_node_attr(mon, "Manufacturer"), NK_TEXT_LEFT);
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			g_color_text_l,
			"%s %sx%s@%sHz %s\"",
			gnwinfo_get_node_attr(mon, "ID"),
			gnwinfo_get_node_attr(res, "Width"),
			gnwinfo_get_node_attr(res, "Height"),
			gnwinfo_get_node_attr(res, "Refresh Rate (Hz)"),
			gnwinfo_get_node_attr(sz, "Diagonal (in)"));
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
draw_volume(struct nk_context* ctx, PNODE disk, float ratio)
{
	INT i;
	PNODE vol = NWL_NodeGetChild(disk, "Volumes");
	if (!vol)
		return;
	nk_layout_row(ctx, NK_DYNAMIC, 0, 5, (float[5]) { ratio, 0.1f, 0.2f, 0.4f, 0.3f - ratio });
	for (i = 0; vol->Children[i].LinkedNode; i++)
	{
		PNODE tab = vol->Children[i].LinkedNode;
		nk_spacer(ctx);
		nk_spacer(ctx);
		if (nk_button_image_label(ctx, g_ctx.image_dir, get_drive_letter(tab), NK_TEXT_CENTERED))
			ShellExecuteA(NULL, "explore", gnwinfo_get_node_attr(tab, "Volume GUID"), NULL, NULL, SW_NORMAL);
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			g_color_text_l,
			"%s %s %s",
			gnwinfo_get_node_attr(tab, "Total Space"),
			gnwinfo_get_node_attr(tab, "Filesystem"),
			gnwinfo_get_node_attr(tab, "Label"));
		nk_prog(ctx, strtoul(gnwinfo_get_node_attr(tab, "Usage"), NULL, 10), 100, 0);
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
		LPCSTR temp = gnwinfo_get_node_attr(disk, "Temperature (C)");
		struct nk_color color = g_color_warning;
		if (strcmp(health, "-") != 0)
		{
			if (strncmp(health, "Good", 4) == 0)
				color = g_color_good;
			else if (strncmp(health, "Bad", 3) == 0)
				color = g_color_error;
			nk_spacer(ctx);
			nk_spacer(ctx);
			nk_label(ctx, "S.M.A.R.T.", NK_TEXT_LEFT);
			nk_labelf_colored(ctx, NK_TEXT_LEFT,
				color, "%s %s%s", health,
				temp[0] != '-' ? temp : "",
				temp[0] != '-' ? u8"\u00B0C" : "");
		}
		draw_volume(ctx, disk, ratio);
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
#ifdef PUBLIC_IP
	if (g_ctx.main_flag & MAIN_NET_PUB_IP)
	{
		nk_spacer(ctx);
		draw_label_l(ctx, L"Public IP");
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			g_color_text_l,
			"%s", g_ctx.pub_ip[0] ? g_ctx.pub_ip : "-");
	}
#endif
	for (i = 0; g_ctx.network->Children[i].LinkedNode; i++)
	{
		PNODE nw = g_ctx.network->Children[i].LinkedNode;
		struct nk_color color = g_color_error;
		if (!nw)
			continue;
		if (strcmp(gnwinfo_get_node_attr(nw, "Status"), "Active") == 0)
			color = g_color_good;
		else if (!(g_ctx.main_flag & MAIN_NET_INACTIVE))
			continue;
		nk_spacer(ctx);
		nk_label(ctx, gnwinfo_get_node_attr(nw, "Description"), NK_TEXT_LEFT);
		nk_labelf_colored(ctx,
			NK_TEXT_LEFT, color,
			"%s%s",
			get_first_ipv4(nw),
			strcmp(gnwinfo_get_node_attr(nw, "DHCP Enabled"), "Yes") == 0 ? " DHCP" : "");
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
	if (nk_button_image(ctx, g_ctx.image_cpu))
		g_ctx.gui_cpuid = TRUE;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_disk))
		g_ctx.gui_smart = TRUE;
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
