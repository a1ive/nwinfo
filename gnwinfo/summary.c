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

#define MAIN_GUI_LABEL(title,icon) \
{ \
	nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 2); \
	struct nk_rect _rect = nk_layout_widget_bounds(ctx); \
	nk_layout_row_push(ctx, _rect.h / _rect.w); \
	nk_image(ctx, icon); \
	nk_layout_row_push(ctx, 1.0f - _rect.h / _rect.w); \
	nk_label(ctx, title, NK_TEXT_LEFT); \
	nk_layout_row_end(ctx); \
}

static VOID
draw_os(struct nk_context* ctx)
{
	MAIN_GUI_LABEL("Operating System", g_ctx.image_os);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.4f, 0.6f });

	nk_label(ctx, "    Name", NK_TEXT_LEFT);
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		NK_COLOR_WHITE,
		"%s %s (%s)",
		gnwinfo_get_node_attr(g_ctx.system, "OS"),
		gnwinfo_get_node_attr(g_ctx.system, "Processor Architecture"),
		gnwinfo_get_node_attr(g_ctx.system, "Build Number"));

	nk_spacer(ctx);
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		NK_COLOR_WHITE,
		"%s@%s%s%s",
		gnwinfo_get_node_attr(g_ctx.system, "Username"),
		gnwinfo_get_node_attr(g_ctx.system, "Computer Name"),
		strcmp(gnwinfo_get_node_attr(g_ctx.system, "Safe Mode"), "Yes") == 0 ? " SafeMode" : "",
		strcmp(gnwinfo_get_node_attr(g_ctx.system, "BitLocker Boot"), "Yes") == 0 ? " BitLocker" : "");

	nk_label(ctx, "    Uptime", NK_TEXT_LEFT);
	nk_label_colored(ctx,
		NWL_GetUptime(),
		NK_TEXT_LEFT,
		NK_COLOR_WHITE);
}

static VOID
draw_bios(struct nk_context* ctx)
{
	LPCSTR tpm = gnwinfo_get_node_attr(g_ctx.system, "TPM");
	MAIN_GUI_LABEL("BIOS", g_ctx.image_bios);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.4f, 0.6f });

	nk_label(ctx, "    Firmware", NK_TEXT_LEFT);
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		NK_COLOR_WHITE,
		"%s%s%s%s",
		gnwinfo_get_node_attr(g_ctx.system, "Firmware"),
		strcmp(gnwinfo_get_node_attr(g_ctx.uefi, "Secure Boot"), "ENABLED") == 0 ? " Secure Boot" : "",
		tpm[0] == 'v' ? " TPM" : "",
		tpm[0] == 'v' ? tpm : "");

	nk_label(ctx, "    Version", NK_TEXT_LEFT);
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		NK_COLOR_WHITE,
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
	MAIN_GUI_LABEL("Computer", g_ctx.image_board);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.4f, 0.6f });

	nk_labelf(ctx, NK_TEXT_LEFT,
		"    %s",
		get_smbios_attr("1", "Manufacturer", NULL));
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		NK_COLOR_WHITE,
		"%s %s %s",
		get_smbios_attr("1", "Product Name", NULL),
		get_smbios_attr("3", "Type", NULL),
		get_smbios_attr("1", "Serial Number", NULL));

	nk_labelf(ctx, NK_TEXT_LEFT,
		"    %s",
		get_smbios_attr("2", "Manufacturer", is_motherboard));
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		NK_COLOR_WHITE,
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
	INT i, j;
	MAIN_GUI_LABEL("Processor", g_ctx.image_cpu);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.4f, 0.6f });

	for (i = 0, j = 0; g_ctx.smbios->Children[i].LinkedNode; i++)
	{
		PNODE tab = g_ctx.smbios->Children[i].LinkedNode;
		LPCSTR attr = gnwinfo_get_node_attr(tab, "Table Type");
		if (strcmp(attr, "4") != 0)
			continue;

		nk_labelf(ctx, NK_TEXT_LEFT, "    CPU%d", j++);
		nk_label_colored(ctx,
			gnwinfo_get_node_attr(tab, "Processor Version"),
			NK_TEXT_LEFT,
			NK_COLOR_WHITE);

		nk_spacer(ctx);
		nk_labelf_colored(ctx, NK_TEXT_LEFT, NK_COLOR_WHITE,
			"%s %s cores %s threads",
			gnwinfo_get_node_attr(tab, "Socket Designation"),
			gnwinfo_get_node_attr(tab, "Core Count"),
			gnwinfo_get_node_attr(tab, "Thread Count"));
	}

	LPCSTR cache_size[4];
	for (cache_level = 1; cache_level <= 4; cache_level++)
		cache_size[cache_level - 1] = get_smbios_attr("7", "Installed Cache Size", is_cache_level_equal);

	nk_label(ctx, "    Cache Size", NK_TEXT_LEFT);
	if (cache_size[0][0] == '-')
		nk_label_colored(ctx, cache_size[0], NK_TEXT_LEFT, NK_COLOR_WHITE);
	else if (cache_size[1][0] == '-')
		nk_labelf_colored(ctx, NK_TEXT_LEFT, NK_COLOR_WHITE,
			"L1 %s", cache_size[0]);
	else if (cache_size[2][0] == '-')
		nk_labelf_colored(ctx, NK_TEXT_LEFT, NK_COLOR_WHITE,
			"L1 %s L2 %s", cache_size[0], cache_size[1]);
	else if (cache_size[3][0] == '-')
		nk_labelf_colored(ctx, NK_TEXT_LEFT, NK_COLOR_WHITE,
			"L1 %s L2 %s L3 %s", cache_size[0], cache_size[1], cache_size[2]);
	else
		nk_labelf_colored(ctx, NK_TEXT_LEFT, NK_COLOR_WHITE,
			"L1 %s L2 %s L3 %s L4 %s", cache_size[0], cache_size[1], cache_size[2], cache_size[3]);
}

LPCSTR NWL_GetHumanSize(UINT64 size, LPCSTR human_sizes[6], UINT64 base);
static const char* mem_human_sizes[6] =
{ "B", "K", "M", "G", "T", "P", };

static VOID
draw_memory(struct nk_context* ctx)
{
	INT i;
	struct nk_color color = NK_COLOR_GREEN;
	MEMORYSTATUSEX statex = { 0 };
	char buf[48];
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	strcpy_s(buf, sizeof(buf), NWL_GetHumanSize(statex.ullAvailPhys, mem_human_sizes, 1024));
	if (statex.dwMemoryLoad > 60)
		color = NK_COLOR_YELLOW;
	if (statex.dwMemoryLoad > 80)
		color = NK_COLOR_RED;
	MAIN_GUI_LABEL("Memory", g_ctx.image_ram);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.4f, 0.6f });

	nk_label(ctx, "    Usage", NK_TEXT_LEFT);
	nk_labelf_colored(ctx, NK_TEXT_LEFT, color,
		"%lu%% %s / %s",
		statex.dwMemoryLoad, buf,
		NWL_GetHumanSize(statex.ullTotalPhys, mem_human_sizes, 1024));

	for (i = 0; g_ctx.smbios->Children[i].LinkedNode; i++)
	{
		PNODE tab = g_ctx.smbios->Children[i].LinkedNode;
		LPCSTR attr = gnwinfo_get_node_attr(tab, "Table Type");
		if (strcmp(attr, "17") != 0)
			continue;
		LPCSTR ddr = gnwinfo_get_node_attr(tab, "Device Type");
		if (ddr[0] == '-')
			continue;
		nk_labelf(ctx, NK_TEXT_LEFT, "    %s", gnwinfo_get_node_attr(tab, "Bank Locator"));
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			NK_COLOR_WHITE,
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
	MAIN_GUI_LABEL("Display Devices", g_ctx.image_edid);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.4f, 0.6f });

	for (i = 0; g_ctx.pci->Children[i].LinkedNode; i++)
	{
		PNODE pci = g_ctx.pci->Children[i].LinkedNode;
		LPCSTR attr = gnwinfo_get_node_attr(pci, "Class Code");
		if (strncmp("03", attr, 2) != 0)
			continue;
		nk_labelf(ctx, NK_TEXT_LEFT, "    %s",
			gnwinfo_get_node_attr(pci, "Vendor"));
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			NK_COLOR_WHITE,
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
		nk_labelf(ctx, NK_TEXT_LEFT, "    %s",
			gnwinfo_get_node_attr(mon, "Manufacturer"));
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			NK_COLOR_WHITE,
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
draw_volume(struct nk_context* ctx, PNODE disk)
{
	INT i;
	PNODE vol = NWL_NodeGetChild(disk, "Volumes");
	if (!vol)
		return;
	nk_layout_row(ctx, NK_DYNAMIC, 0, 4, (float[4]) { 0.2f, 0.2f, 0.4f, 0.2f });
	for (i = 0; vol->Children[i].LinkedNode; i++)
	{
		PNODE tab = vol->Children[i].LinkedNode;
		nk_spacer(ctx);
		if (nk_button_image_label(ctx, g_ctx.image_dir, get_drive_letter(tab), NK_TEXT_CENTERED))
			ShellExecuteA(NULL, "explore", gnwinfo_get_node_attr(tab, "Volume GUID"), NULL, NULL, SW_NORMAL);
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			NK_COLOR_WHITE,
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
	MAIN_GUI_LABEL("Storage", g_ctx.image_disk);
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

		nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.2f, 0.2f, 0.6f });
		nk_labelf(ctx, NK_TEXT_LEFT,
			"    %s%s",
			cdrom ? "CDROM" : "DISK",
			id);
		nk_labelf(ctx, NK_TEXT_LEFT,
			"%s %s",
			gnwinfo_get_node_attr(disk, "Type"),
			type);
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			NK_COLOR_WHITE,
			"%s %s %s",
			gnwinfo_get_node_attr(disk, "Size"),
			gnwinfo_get_node_attr(disk, "Partition Table"),
			gnwinfo_get_node_attr(disk, "Product ID"));

		LPCSTR health = gnwinfo_get_node_attr(disk, "Health Status");
		LPCSTR temp = gnwinfo_get_node_attr(disk, "Temperature (C)");
		struct nk_color color = NK_COLOR_YELLOW;
		if (strcmp(health, "-") != 0)
		{
			if (strncmp(health, "Good", 4) == 0)
				color = NK_COLOR_GREEN;
			else if (strncmp(health, "Bad", 3) == 0)
				color = NK_COLOR_RED;
			nk_spacer(ctx);
			nk_label(ctx, "S.M.A.R.T", NK_TEXT_LEFT);
			nk_labelf_colored(ctx, NK_TEXT_LEFT,
				color, "%s %s%s", health,
				temp[0] != '-' ? temp : "",
				temp[0] != '-' ? u8"¡ãC" : "");
		}
		draw_volume(ctx, disk);
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
	if (g_ctx.network)
		NWL_NodeFree(g_ctx.network, 1);
	g_ctx.network = NW_Network();
	MAIN_GUI_LABEL("Network", g_ctx.image_net);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.6f, 0.4f });
	for (i = 0; g_ctx.network->Children[i].LinkedNode; i++)
	{
		PNODE nw = g_ctx.network->Children[i].LinkedNode;
		struct nk_color color = NK_COLOR_RED;
		if (!nw)
			continue;
		nk_labelf(ctx, NK_TEXT_LEFT, "    %s", gnwinfo_get_node_attr(nw, "Description"));
		if (strcmp(gnwinfo_get_node_attr(nw, "Status"), "Active") == 0)
			color = NK_COLOR_GREEN;
		nk_labelf_colored(ctx,
			NK_TEXT_LEFT, color,
			"%s%s",
			get_first_ipv4(nw),
			strcmp(gnwinfo_get_node_attr(nw, "DHCP Enabled"), "Yes") == 0 ? " DHCP" : "");
	}
}

#define MAIN_INFO_OS        (1U << 0)
#define MAIN_INFO_BIOS      (1U << 1)
#define MAIN_INFO_BOARD     (1U << 2)
#define MAIN_INFO_CPU       (1U << 3)
#define MAIN_INFO_MEMORY    (1U << 4)
#define MAIN_INFO_MONITOR   (1U << 5)
#define MAIN_INFO_STORAGE   (1U << 6)
#define MAIN_INFO_NETWORK   (1U << 7)

VOID
gnwinfo_draw_main_window(struct nk_context* ctx, float width, float height)
{
	if (!nk_begin(ctx, "Summary",
		nk_rect(0, 0, width, height),
		NK_WINDOW_BACKGROUND))
		goto out;
	nk_layout_row_begin(ctx, NK_DYNAMIC, 20, 12);
	struct nk_rect rect = nk_layout_widget_bounds(ctx);
	float ratio = rect.h / rect.w;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_os))
		g_ctx.main_flag ^= MAIN_INFO_OS;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_bios))
		g_ctx.main_flag ^= MAIN_INFO_BIOS;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_board))
		g_ctx.main_flag ^= MAIN_INFO_BOARD;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_cpu))
		g_ctx.main_flag ^= MAIN_INFO_CPU;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_ram))
		g_ctx.main_flag ^= MAIN_INFO_MEMORY;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_edid))
		g_ctx.main_flag ^= MAIN_INFO_MONITOR;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_disk))
		g_ctx.main_flag ^= MAIN_INFO_STORAGE;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_net))
		g_ctx.main_flag ^= MAIN_INFO_NETWORK;
	nk_layout_row_push(ctx, 1.0f > 12.0f * ratio ? (1.0f - 12.0f * ratio) : 0);
	nk_spacer(ctx);
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_cpuid))
		g_ctx.gui_cpuid = TRUE;
	nk_layout_row_push(ctx, ratio);
	if (nk_button_image(ctx, g_ctx.image_smart))
		g_ctx.gui_smart = TRUE;
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
