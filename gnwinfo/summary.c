// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include <shellapi.h>

LPCSTR NWL_Ucs2ToUtf8(LPCWSTR src);
LPCWSTR NWL_Utf8ToUcs2(LPCSTR src);

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

struct nk_color
gnwinfo_get_color(double value, double warn, double err)
{
	if (value > err)
		return g_color_error;
	if (value > warn)
		return g_color_warning;
	if (value <= 0.0)
		return g_color_unknown;
	return g_color_good;
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

void
gnwinfo_draw_percent_prog(struct nk_context* ctx, double percent)
{
	nk_size size = (nk_size)percent;
	struct nk_color color = gnwinfo_get_color(percent, 70.0, 90.0);
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
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });

	nk_image_label(ctx, g_ctx.image_os, gnwinfo_get_text(L"Operating System"), NK_TEXT_LEFT, g_color_text_d);
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		g_color_text_l,
		"%s %s (%s)",
		gnwinfo_get_node_attr(g_ctx.system, "OS"),
		gnwinfo_get_node_attr(g_ctx.system, "Processor Architecture"),
		gnwinfo_get_node_attr(g_ctx.system, "Build Number"));

	if (g_ctx.main_flag & MAIN_OS_DETAIL)
	{
		nk_space_label(ctx, gnwinfo_get_text(L"Login Status"), NK_TEXT_LEFT);
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			g_color_text_l,
			"%s@%s%s%s",
			gnwinfo_get_node_attr(g_ctx.system, "Username"),
			gnwinfo_get_node_attr(g_ctx.system, "Computer Name"),
			strcmp(gnwinfo_get_node_attr(g_ctx.system, "Safe Mode"), "Yes") == 0 ? " SafeMode" : "",
			strcmp(gnwinfo_get_node_attr(g_ctx.system, "BitLocker Boot"), "Yes") == 0 ? " BitLocker" : "");
	}

	if (g_ctx.main_flag & MAIN_OS_UPTIME)
	{
		nk_space_label(ctx, gnwinfo_get_text(L"Uptime"), NK_TEXT_LEFT);
		nk_label_colored(ctx, g_ctx.sys_uptime, NK_TEXT_LEFT, g_color_text_l);
	}
}

static VOID
draw_bios(struct nk_context* ctx)
{
	LPCSTR tpm = gnwinfo_get_node_attr(g_ctx.system, "TPM");
	LPCSTR sb = gnwinfo_get_node_attr(g_ctx.uefi, "Secure Boot");
	LPCWSTR wsb;

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });
	nk_image_label(ctx, g_ctx.image_bios, gnwinfo_get_text(L"BIOS"), NK_TEXT_LEFT, g_color_text_d);

	if (sb[0] == 'E')
		wsb = L"SecureBoot";
	else if (sb[0] == 'D')
		wsb = L"SecureBootOff";
	else
		wsb = L"";

	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		g_color_text_l,
		"%s%s%s%s%s",
		gnwinfo_get_node_attr(g_ctx.system, "Firmware"),
		wsb[0] ? " " : "",
		gnwinfo_get_text(wsb),
		tpm[0] == 'v' ? " TPM" : "",
		tpm[0] == 'v' ? tpm : "");

	if (g_ctx.main_flag & MAIN_B_VENDOR)
	{
		nk_space_label(ctx, gnwinfo_get_text(L"Vendor"), NK_TEXT_LEFT);
		nk_label_colored(ctx, get_smbios_attr("0", "Vendor", NULL), NK_TEXT_LEFT, g_color_text_l);
	}
	if (g_ctx.main_flag & MAIN_B_VERSION)
	{
		nk_space_label(ctx, gnwinfo_get_text(L"Version"), NK_TEXT_LEFT);
		nk_label_colored(ctx, get_smbios_attr("0", "Version", NULL), NK_TEXT_LEFT, g_color_text_l);
	}
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
	struct nk_color color = g_color_error;
	LPCSTR time = NULL;
	LPCSTR bat = gnwinfo_get_node_attr(g_ctx.battery, "Battery Status");

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });
	nk_image_label(ctx, g_ctx.image_board, gnwinfo_get_text(L"Computer"), NK_TEXT_LEFT, g_color_text_d);
	nk_spacer(ctx);

	nk_space_label(ctx, get_smbios_attr("1", "Manufacturer", NULL), NK_TEXT_LEFT);
	nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_text_l,
		"%s %s %s",
		get_smbios_attr("1", "Product Name", NULL),
		get_smbios_attr("3", "Type", NULL),
		get_smbios_attr("1", "Serial Number", NULL));

	nk_space_label(ctx, get_smbios_attr("2", "Manufacturer", is_motherboard), NK_TEXT_LEFT);
	nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_text_l,
		"%s %s",
		get_smbios_attr("2", "Product Name", is_motherboard),
		get_smbios_attr("2", "Serial Number", is_motherboard));

	if (strcmp(bat, "Charging") == 0)
	{
		color = g_color_warning;
		time = gnwinfo_get_node_attr(g_ctx.battery, "Battery Life Full");
	}
	else if (strcmp(bat, "Not Charging") == 0)
	{
		if (strcmp(gnwinfo_get_node_attr(g_ctx.battery, "AC Power"), "Online") == 0)
			color = g_color_good;
		time = gnwinfo_get_node_attr(g_ctx.battery, "Battery Life Remaining");
	}
	else
		return;

	nk_space_label(ctx, gnwinfo_get_text(L"Battery"), NK_TEXT_LEFT);
	nk_labelf_colored(ctx, NK_TEXT_LEFT, color,
		u8"\u26a1 %s %s",
		gnwinfo_get_node_attr(g_ctx.battery, "Battery Life Percentage"),
		time);
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
	INT i;
	CHAR name[32];

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.3f, 0.4f, 0.3f });
	nk_image_label(ctx, g_ctx.image_cpu, gnwinfo_get_text(L"Processor"), NK_TEXT_LEFT, g_color_text_d);

	nk_labelf_colored(ctx, NK_TEXT_LEFT, gnwinfo_get_color(g_ctx.cpu_usage, 70.0, 90.0),
		"%.2f%% %s MHz",
		g_ctx.cpu_usage,
		gnwinfo_get_node_attr(g_ctx.cpuid, "CPU Clock (MHz)"));
	gnwinfo_draw_percent_prog(ctx, g_ctx.cpu_usage);

	for (i = 0; i < g_ctx.cpu_count; i++)
	{
		snprintf(name, sizeof(name), "CPU%d", i);
		PNODE cpu = NWL_NodeGetChild(g_ctx.cpuid, name);
		CHAR buf[MAX_PATH];

		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });
		nk_space_label(ctx, name, NK_TEXT_LEFT);
		nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Brand"), NK_TEXT_LEFT, g_color_text_l);

		if (!(g_ctx.main_flag & MAIN_CPU_DETAIL))
			continue;

		nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.3f, 0.4f, 0.3f });
		nk_spacer(ctx);
		snprintf(buf, MAX_PATH, "%s %s", gnwinfo_get_node_attr(cpu, "Cores"), gnwinfo_get_text(L"cores"));
		snprintf(buf, MAX_PATH, "%s %s %s", buf, gnwinfo_get_node_attr(cpu, "Logical CPUs"), gnwinfo_get_text(L"threads"));
		if (g_ctx.cpu_info[i].cpu_msr_power > 0.0)
			snprintf(buf, MAX_PATH, "%s %.2f W", buf, g_ctx.cpu_info[i].cpu_msr_power);

		nk_label_colored(ctx, buf, NK_TEXT_LEFT, g_color_text_l);
		if (g_ctx.cpu_info[i].cpu_msr_temp > 0)
			nk_labelf_colored(ctx, NK_TEXT_LEFT, gnwinfo_get_color((double)g_ctx.cpu_info[i].cpu_msr_temp, 65.0, 85.0),
				u8"%d \u00B0C", g_ctx.cpu_info[i].cpu_msr_temp);
		else
			nk_spacer(ctx);
	}

	if (g_ctx.main_flag & MAIN_CPU_CACHE)
	{
		LPCSTR cache_size[4];
		for (cache_level = 1; cache_level <= 4; cache_level++)
			cache_size[cache_level - 1] = get_smbios_attr("7", "Installed Cache Size", is_cache_level_equal);

		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });
		nk_space_label(ctx, gnwinfo_get_text(L"Cache"), NK_TEXT_LEFT);
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
}

static VOID
draw_memory(struct nk_context* ctx)
{
	INT i;

	nk_layout_row(ctx, NK_DYNAMIC, 0, 4, (float[4]) { 0.3f, 0.4f - g_ctx.gui_ratio, g_ctx.gui_ratio, 0.3f });

	nk_image_label(ctx, g_ctx.image_ram, gnwinfo_get_text(L"Memory"), NK_TEXT_LEFT, g_color_text_d);
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		gnwinfo_get_color((double)g_ctx.mem_status.PhysUsage, 70.0, 90.0),
		"%lu%% %s / %s",
		g_ctx.mem_status.PhysUsage, g_ctx.mem_avail, g_ctx.mem_total);
	if (nk_button_symbol(ctx, NK_SYMBOL_PLUS))
	{
		g_ctx.gui_mm = TRUE;
		gnwinfo_init_mm_window(ctx);
	}
	gnwinfo_draw_percent_prog(ctx, (double)g_ctx.mem_status.PhysUsage);

	if (g_ctx.main_flag & MAIN_MEM_DETAIL)
	{
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });
		for (i = 0; g_ctx.smbios->Children[i].LinkedNode; i++)
		{
			PNODE tab = g_ctx.smbios->Children[i].LinkedNode;
			LPCSTR attr = gnwinfo_get_node_attr(tab, "Table Type");
			if (strcmp(attr, "17") != 0)
				continue;
			LPCSTR ddr = gnwinfo_get_node_attr(tab, "Device Type");
			if (ddr[0] == '-')
				continue;
			nk_space_label(ctx, gnwinfo_get_node_attr(tab, "Bank Locator"), NK_TEXT_LEFT);
			nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_text_l,
				"%s-%s %s %s %s",
				ddr,
				gnwinfo_get_node_attr(tab, "Speed (MT/s)"),
				gnwinfo_get_node_attr(tab, "Device Size"),
				gnwinfo_get_node_attr(tab, "Manufacturer"),
				gnwinfo_get_node_attr(tab, "Serial Number"));
		}
	}
}

static VOID
draw_display(struct nk_context* ctx)
{
	INT i, j;

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_image_label(ctx, g_ctx.image_edid, gnwinfo_get_text(L"Display Devices"), NK_TEXT_LEFT, g_color_text_d);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });

	for (i = 0; g_ctx.pci->Children[i].LinkedNode; i++)
	{
		PNODE pci = g_ctx.pci->Children[i].LinkedNode;
		LPCSTR attr = gnwinfo_get_node_attr(pci, "Class Code");
		if (strncmp("03", attr, 2) != 0)
			continue;
		LPCSTR vendor = gnwinfo_get_node_attr(pci, "Vendor");
		if (strcmp(vendor, "-") == 0)
			continue;
		nk_space_label(ctx, vendor, NK_TEXT_LEFT);
		nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_text_l,
			"%s",
			gnwinfo_get_node_attr(pci, "Device"));
	}
	for (i = 0; g_ctx.edid->Children[i].LinkedNode; i++)
	{
		CHAR name[32];
		CHAR res[32] = { 0 };
		LPCSTR product = NULL;
		LPCSTR id = NULL;
		PNODE mon = g_ctx.edid->Children[i].LinkedNode;
		id = gnwinfo_get_node_attr(mon, "ID");
		if (id[0] == '-')
			continue;
		nk_space_label(ctx, gnwinfo_get_node_attr(mon, "Manufacturer"), NK_TEXT_LEFT);
		
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
		
		nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_text_l,
			"%s %s %s %s\"",
			id,
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
draw_volume(struct nk_context* ctx, PNODE disk, BOOL cdrom)
{
	INT i;
	PNODE vol = NWL_NodeGetChild(disk, "Volumes");
	if (!vol)
		return;
	nk_layout_row(ctx, NK_DYNAMIC, 0, 4, (float[4]) { 0.12f, 0.18f, 0.4f, 0.3f });
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
		if (nk_button_image_label(ctx, img, get_drive_letter(tab), NK_TEXT_CENTERED))
			ShellExecuteA(NULL, "explore", gnwinfo_get_node_attr(tab, "Volume GUID"), NULL, NULL, SW_NORMAL);
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			g_color_text_l,
			"%s %s %s",
			gnwinfo_get_node_attr(tab, "Total Space"),
			gnwinfo_get_node_attr(tab, "Filesystem"),
			gnwinfo_get_node_attr(tab, "Label"));
		gnwinfo_draw_percent_prog(ctx, strtod(gnwinfo_get_node_attr(tab, "Usage"), NULL));
	}
}

static VOID
draw_volume_compact(struct nk_context* ctx, PNODE disk)
{
	INT i;
	INT count;
	CHAR buf[] = "A";
	PNODE vol = NWL_NodeGetChild(disk, "Volumes");
	if (!vol)
		return;
	for (i = 0, count = 0; vol->Children[i].LinkedNode; i++)
	{
		LPCSTR drive = get_drive_letter(vol->Children[i].LinkedNode);
		if (drive[1] == ':')
			count++;
	}
	nk_layout_row_begin(ctx, NK_DYNAMIC, 0, count + 1);
	nk_layout_row_push(ctx, 0.3f);
	nk_spacer(ctx);
	for (i = 0; vol->Children[i].LinkedNode; i++)
	{
		PNODE tab = vol->Children[i].LinkedNode;
		LPCSTR drive = get_drive_letter(tab);
		if (drive[1] != ':')
			continue;
		buf[0] = drive[0];
		nk_layout_row_push(ctx, g_ctx.gui_ratio);
		if (nk_button_label(ctx, buf))
			ShellExecuteA(NULL, "explore", gnwinfo_get_node_attr(tab, "Volume GUID"), NULL, NULL, SW_NORMAL);
	}
	nk_layout_row_end(ctx);
}

static VOID
draw_storage(struct nk_context* ctx)
{
	INT i;
	CHAR name[32];

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_image_label(ctx, g_ctx.image_disk, gnwinfo_get_text(L"Storage"), NK_TEXT_LEFT, g_color_text_d);
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

		nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.12f, 0.18f, 0.7f });
		snprintf(name, 32, "%s%s", cdrom ? "CD" : "DISK", id);
		nk_space_label(ctx, name, NK_TEXT_LEFT);
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
			LPCSTR life = strchr(health, '(');
			LPCWSTR whealth = L"Unknown";
			struct nk_color color = g_color_unknown;
			LPCSTR temp = gnwinfo_get_node_attr(disk, "Temperature (C)");
			if (strncmp(health, "Good", 4) == 0)
			{
				color = g_color_good;
				whealth = L"Good";
			}
			else if (strncmp(health, "Caution", 7) == 0)
			{
				color = g_color_warning;
				whealth = L"Caution";
			}
			else if (strncmp(health, "Bad", 3) == 0)
			{
				color = g_color_error;
				whealth = L"Bad";
			}
			if (life == NULL)
				life = "";
			nk_spacer(ctx);
			nk_label(ctx, "S.M.A.R.T.", NK_TEXT_LEFT);
			nk_labelf_colored(ctx, NK_TEXT_LEFT,
				color, u8"%s%s %s\u00B0C", gnwinfo_get_text(whealth), life,
				temp[0] == '-' ? "-" : temp);
		}
		if (g_ctx.main_flag & MAIN_DISK_COMPACT)
			draw_volume(ctx, disk, cdrom);
		else
			draw_volume_compact(ctx, disk);
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

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.64f, 0.36f });
	nk_image_label(ctx, g_ctx.image_net, gnwinfo_get_text(L"Network"), NK_TEXT_LEFT, g_color_text_d);
	nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_text_l,
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
		nk_space_label(ctx, gnwinfo_get_node_attr(nw, "Description"), NK_TEXT_LEFT);
		nk_labelf_colored(ctx,
			NK_TEXT_LEFT, color,
			"%s%s",
			strcmp(gnwinfo_get_node_attr(nw, "DHCP Enabled"), "Yes") == 0 ? "DHCP " : "",
			get_first_ipv4(nw));

		if (g_ctx.main_flag & MAIN_NET_DETAIL)
		{
			if (is_active)
				nk_labelf(ctx, NK_TEXT_LEFT, u8"      \u2191\u2193 %s / %s",
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
	if (!nk_begin(ctx, "NWinfo GUI",
		nk_rect(0, 0, width, height),
		g_bginfo ? NK_WINDOW_BACKGROUND : (NK_WINDOW_BACKGROUND | NK_WINDOW_CLOSABLE | NK_WINDOW_TITLE)))
	{
		nk_end(ctx);
		gnwinfo_ctx_exit();
	}

	nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 8);

	struct nk_rect rect = nk_layout_widget_bounds(ctx);
	g_ctx.gui_ratio = rect.h / rect.w;

	nk_layout_row_push(ctx, g_ctx.gui_ratio);
	if (nk_button_image(ctx, g_ctx.image_cpuid))
		g_ctx.gui_cpuid = TRUE;
	nk_layout_row_push(ctx, g_ctx.gui_ratio);
	if (nk_button_image(ctx, g_ctx.image_smart))
		g_ctx.gui_smart = TRUE;
	nk_layout_row_push(ctx, g_ctx.gui_ratio);
	if (nk_button_image(ctx, g_ctx.image_pci))
		g_ctx.gui_pci = TRUE;
	nk_layout_row_push(ctx, g_ctx.gui_ratio);
	if (nk_button_image(ctx, g_ctx.image_dmi))
		g_ctx.gui_dmi = TRUE;
	nk_layout_row_push(ctx, g_ctx.gui_ratio);
	if (nk_button_image(ctx, g_ctx.image_set))
		g_ctx.gui_settings = TRUE;
	nk_layout_row_push(ctx, g_ctx.gui_ratio);
	if (nk_button_image(ctx, g_ctx.image_refresh))
		gnwinfo_ctx_update(IDT_TIMER_1M);
	nk_layout_row_push(ctx, g_ctx.gui_ratio);
	if (nk_button_image(ctx, g_ctx.image_info))
		g_ctx.gui_about = TRUE;
	nk_layout_row_push(ctx, g_ctx.gui_ratio);
	if (g_bginfo)
	{
		if (nk_button_image(ctx, g_ctx.image_close))
			gnwinfo_ctx_exit();
	}
	else
		nk_spacer(ctx);
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

	nk_end(ctx);
}
