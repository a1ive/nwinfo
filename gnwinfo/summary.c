// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

static LPCSTR
get_node_attr(PNODE node, LPCSTR key)
{
	LPCSTR str;
	if (!node)
		return "~";
	str = NWL_NodeAttrGet(node, key);
	if (!str)
		return "~";
	return str;
}

static LPCSTR
get_smbios_attr(LPCSTR type, LPCSTR key, BOOL(*cond)(PNODE node))
{
	INT i, count;
	count = NWL_NodeChildCount(g_ctx.smbios);
	for (i = 0; i < count; i++)
	{
		LPCSTR attr;
		PNODE tab = g_ctx.smbios->Children[i].LinkedNode;
		attr = get_node_attr(tab, "Table Type");
		if (!attr || strcmp(attr, type) != 0)
			continue;
		if (!cond || cond(tab) == TRUE)
			return get_node_attr(tab, key);
	}
	return "~";
}

#define MAIN_GUI_ROW_2_BEGIN \
	nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 2); \
	nk_layout_row_push(ctx, 0.40f);

#define MAIN_GUI_ROW_2_MID1 \
	nk_layout_row_push(ctx, 0.60f);

#define MAIN_GUI_ROW_2_END \
	nk_layout_row_end(ctx);

#define MAIN_GUI_ROW_3_BEGIN \
	nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 3); \
	nk_layout_row_push(ctx, 0.20f);

#define MAIN_GUI_ROW_3_MID1 \
	nk_layout_row_push(ctx, 0.20f);

#define MAIN_GUI_ROW_3_MID2 \
	nk_layout_row_push(ctx, 0.60f);

#define MAIN_GUI_ROW_3_END \
	nk_layout_row_end(ctx);

#define MAIN_GUI_LABEL(title,icon) \
{ \
	nk_layout_row_begin(ctx, NK_DYNAMIC, GNWINFO_FONT_SIZE + ctx->style.window.spacing.y, 4); \
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
	MAIN_GUI_ROW_2_BEGIN;
	nk_label(ctx, "    Name", NK_TEXT_LEFT);
	MAIN_GUI_ROW_2_MID1;
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		nk_rgb(255, 255, 255),
		"%s %s (%s)",
		get_node_attr(g_ctx.system, "OS"),
		get_node_attr(g_ctx.system, "Processor Architecture"),
		get_node_attr(g_ctx.system, "Build Number"));
	MAIN_GUI_ROW_2_END;
	MAIN_GUI_ROW_2_BEGIN;
	nk_spacer(ctx);
	MAIN_GUI_ROW_2_MID1;
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		nk_rgb(255, 255, 255),
		"%s@%s%s%s",
		get_node_attr(g_ctx.system, "Username"),
		get_node_attr(g_ctx.system, "Computer Name"),
		strcmp(get_node_attr(g_ctx.system, "Safe Mode"), "Yes") == 0 ? " SafeMode" : "",
		strcmp(get_node_attr(g_ctx.system, "BitLocker Boot"), "Yes") == 0 ? " BitLocker" : "");
	MAIN_GUI_ROW_2_END;
	MAIN_GUI_ROW_2_BEGIN;
	nk_label(ctx, "    Uptime", NK_TEXT_LEFT);
	MAIN_GUI_ROW_2_MID1;
	nk_label_colored(ctx,
		NWL_GetUptime(),
		NK_TEXT_LEFT,
		nk_rgb(255, 255, 255));
	MAIN_GUI_ROW_2_END;
}

static VOID
draw_bios(struct nk_context* ctx)
{
	LPCSTR tpm = get_node_attr(g_ctx.system, "TPM");
	MAIN_GUI_LABEL("BIOS", g_ctx.image_bios);
	MAIN_GUI_ROW_2_BEGIN;
	nk_label(ctx, "    Firmware", NK_TEXT_LEFT);
	MAIN_GUI_ROW_2_MID1;
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		nk_rgb(255, 255, 255),
		"%s%s%s%s",
		get_node_attr(g_ctx.uefi, "Firmware Type"),
		strcmp(get_node_attr(g_ctx.uefi, "Secure Boot"), "ENABLED") == 0 ? " Secure Boot" : "",
		tpm[0] == 'v' ? " TPM" : "",
		tpm[0] == 'v' ? tpm : "");
	MAIN_GUI_ROW_2_END;
	MAIN_GUI_ROW_2_BEGIN;
	nk_label(ctx, "    Version", NK_TEXT_LEFT);
	MAIN_GUI_ROW_2_MID1;
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		nk_rgb(255, 255, 255),
		"%s %s",
		get_smbios_attr("0", "Vendor", NULL),
		get_smbios_attr("0", "Version", NULL));
	MAIN_GUI_ROW_2_END;
}

static BOOL
is_motherboard(PNODE node)
{
	LPCSTR str = get_node_attr(node, "Board Type");
	return (strcmp(str, "Motherboard") == 0);
}

static VOID
draw_motherboard(struct nk_context* ctx)
{
	MAIN_GUI_LABEL("Motherboard", g_ctx.image_board);
	MAIN_GUI_ROW_2_BEGIN;
	nk_label(ctx, "    Product Name", NK_TEXT_LEFT);
	MAIN_GUI_ROW_2_MID1;
	nk_labelf_colored(ctx, NK_TEXT_LEFT,
		nk_rgb(255, 255, 255),
		"%s %s",
		get_smbios_attr("2", "Manufacturer", is_motherboard),
		get_smbios_attr("2", "Product Name", is_motherboard));
	MAIN_GUI_ROW_2_END;
	MAIN_GUI_ROW_2_BEGIN;
	nk_label(ctx, "    Serial Number", NK_TEXT_LEFT);
	MAIN_GUI_ROW_2_MID1;
	nk_label_colored(ctx,
		get_smbios_attr("2", "Serial Number", is_motherboard),
		NK_TEXT_LEFT,
		nk_rgb(255, 255, 255));
	MAIN_GUI_ROW_2_END;
}

static VOID
draw_processor(struct nk_context* ctx)
{
	INT i, j, count;
	MAIN_GUI_LABEL("Processor", g_ctx.image_cpu);
	count = NWL_NodeChildCount(g_ctx.smbios);
	for (i = 0, j = 0; i < count; i++)
	{
		LPCSTR attr;
		PNODE tab = g_ctx.smbios->Children[i].LinkedNode;
		attr = get_node_attr(tab, "Table Type");
		if (!attr || strcmp(attr, "4") != 0)
			continue;
		MAIN_GUI_ROW_3_BEGIN;
		nk_labelf(ctx, NK_TEXT_LEFT, "    CPU%d", j++);
		MAIN_GUI_ROW_3_MID1;
		nk_label(ctx, "Version", NK_TEXT_LEFT);
		MAIN_GUI_ROW_3_MID2;
		nk_label_colored(ctx,
			get_node_attr(tab, "Processor Version"),
			NK_TEXT_LEFT,
			nk_rgb(255, 255, 255));
		MAIN_GUI_ROW_3_END;
		MAIN_GUI_ROW_3_BEGIN;
		nk_spacer(ctx);
		MAIN_GUI_ROW_3_MID1;
		nk_spacer(ctx);
		MAIN_GUI_ROW_3_MID2;
		nk_labelf_colored(ctx, NK_TEXT_LEFT, nk_rgb(255, 255, 255),
			"%s %s cores %s threads",
			get_node_attr(tab, "Socket Designation"),
			get_node_attr(tab, "Core Count"),
			get_node_attr(tab, "Thread Count"));
		MAIN_GUI_ROW_3_END;
	}
}

LPCSTR NWL_GetHumanSize(UINT64 size, LPCSTR human_sizes[6], UINT64 base);
static const char* mem_human_sizes[6] =
{ "B", "K", "M", "G", "T", "P", };

static VOID
draw_memory(struct nk_context* ctx)
{
	INT i, count;
	struct nk_color color = nk_rgb(0, 255, 0); // Green
	MEMORYSTATUSEX statex = { 0 };
	char buf[48];
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	strcpy_s(buf, sizeof(buf), NWL_GetHumanSize(statex.ullAvailPhys, mem_human_sizes, 1024));
	if (statex.dwMemoryLoad > 60)
		color = nk_rgb(0, 255, 255); // Yellow
	if (statex.dwMemoryLoad > 80)
		color = nk_rgb(255, 0, 0); // Red
	MAIN_GUI_LABEL("Memory", g_ctx.image_ram);
	MAIN_GUI_ROW_2_BEGIN;
	nk_label(ctx, "    Usage", NK_TEXT_LEFT);
	MAIN_GUI_ROW_2_MID1;
	nk_labelf_colored(ctx, NK_TEXT_LEFT, color,
		"%lu%% %s / %s",
		statex.dwMemoryLoad, buf,
		NWL_GetHumanSize(statex.ullTotalPhys, mem_human_sizes, 1024));
	MAIN_GUI_ROW_2_END;
	count = NWL_NodeChildCount(g_ctx.smbios);
	for (i = 0; i < count; i++)
	{
		LPCSTR attr;
		PNODE tab = g_ctx.smbios->Children[i].LinkedNode;
		attr = get_node_attr(tab, "Table Type");
		if (!attr || strcmp(attr, "17") != 0)
			continue;
		MAIN_GUI_ROW_2_BEGIN;
		nk_labelf(ctx, NK_TEXT_LEFT, "    %s", get_node_attr(tab, "Bank Locator"));
		MAIN_GUI_ROW_2_MID1;
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			nk_rgb(255, 255, 255),
			"%s-%s %s %s %s",
			get_node_attr(tab, "Device Type"),
			get_node_attr(tab, "Speed (MT/s)"),
			get_node_attr(tab, "Device Size"),
			get_node_attr(tab, "Manufacturer"),
			get_node_attr(tab, "Serial Number"));
		MAIN_GUI_ROW_2_END;
	}
}

static VOID
draw_monitor(struct nk_context* ctx)
{
	INT i, count;
	MAIN_GUI_LABEL("Display Devices", g_ctx.image_edid);
	count = NWL_NodeChildCount(g_ctx.edid);
	for (i = 0; i < count; i++)
	{
		PNODE mon = g_ctx.edid->Children[i].LinkedNode;
		PNODE res, sz;
		if (!mon)
			continue;
		res = NWL_NodeGetChild(mon, "Resolution");
		sz = NWL_NodeGetChild(mon, "Screen Size");
		MAIN_GUI_ROW_2_BEGIN;
		nk_labelf(ctx, NK_TEXT_LEFT, "    %s",
			get_node_attr(mon, "HWID"));
		MAIN_GUI_ROW_2_MID1;
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			nk_rgb(255, 255, 255),
			"%sx%s@%sHz %s\"",
			get_node_attr(res, "Width"),
			get_node_attr(res, "Height"),
			get_node_attr(res, "Refresh Rate (Hz)"),
			get_node_attr(sz, "Diagonal (in)"));
		MAIN_GUI_ROW_2_END;
	}
}

static VOID
draw_storage(struct nk_context* ctx)
{
	INT i, count;
	MAIN_GUI_LABEL("Storage", g_ctx.image_disk);
	count = NWL_NodeChildCount(g_ctx.disk);
	for (i = 0; i < count; i++)
	{
		PNODE disk = g_ctx.disk->Children[i].LinkedNode;
		if (!disk)
			continue;
		MAIN_GUI_ROW_3_BEGIN;
		nk_labelf(ctx, NK_TEXT_LEFT, "    Disk%d", i);
		MAIN_GUI_ROW_3_MID1;
		nk_label(ctx, "Product Name", NK_TEXT_LEFT);
		MAIN_GUI_ROW_3_MID2;
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			nk_rgb(255, 255, 255),
			"%s (%s)",
			get_node_attr(disk, "Product ID"),
			get_node_attr(disk, "Product Rev"));
		MAIN_GUI_ROW_3_END;
		MAIN_GUI_ROW_3_BEGIN;
		nk_spacer(ctx);
		MAIN_GUI_ROW_3_MID1;
		nk_label(ctx, "Info", NK_TEXT_LEFT);
		MAIN_GUI_ROW_3_MID2;
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			nk_rgb(255, 255, 255),
			"%s %s %s %s",
			get_node_attr(disk, "Size"),
			get_node_attr(disk, "Type"),
			strcmp(get_node_attr(disk, "SSD"), "Yes") == 0 ? "SSD" : "HDD",
			get_node_attr(disk, "Partition Table"));
		MAIN_GUI_ROW_3_END;
		LPCSTR health = get_node_attr(disk, "Health Status");
		struct nk_color color = nk_rgb(0, 255, 255); // Yellow
		if (strcmp(health, "~") == 0)
			continue;
		if (strncmp(health, "Good", 4) == 0)
			color = nk_rgb(0, 255, 0); // Green
		else if (strncmp(health, "Bad", 3) == 0)
			color = nk_rgb(255, 0, 0); // Red
		MAIN_GUI_ROW_3_BEGIN;
		nk_spacer(ctx);
		MAIN_GUI_ROW_3_MID1;
		nk_label(ctx, "S.M.A.R.T", NK_TEXT_LEFT);
		MAIN_GUI_ROW_3_MID2;
		nk_labelf_colored(ctx, NK_TEXT_LEFT,
			color, "%s %s(C)", health,
			get_node_attr(disk, "Temperature (C)"));
		MAIN_GUI_ROW_3_END;
	}
}

static LPCSTR
get_first_ipv4(PNODE node)
{
	INT i, count;
	PNODE unicasts = NWL_NodeGetChild(node, "Unicasts");
	if (!unicasts)
		return "";
	count = NWL_NodeChildCount(unicasts);
	for (i = 0; i < count; i++)
	{
		PNODE ip = unicasts->Children[i].LinkedNode;
		LPCSTR addr = get_node_attr(ip, "IPv4");
		if (strcmp(addr, "~") != 0)
			return addr;
	}
	return "";
}

static VOID
draw_network(struct nk_context* ctx)
{
	INT i, count;
	if (g_ctx.network)
		NWL_NodeFree(g_ctx.network, 1);
	g_ctx.network = NW_Network();
	MAIN_GUI_LABEL("Network", g_ctx.image_net);
	count = NWL_NodeChildCount(g_ctx.network);
	for (i = 0; i < count; i++)
	{
		PNODE nw = g_ctx.network->Children[i].LinkedNode;
		struct nk_color color = nk_rgb(255, 0, 0); // Red
		if (!nw)
			continue;
		nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 2);
		nk_layout_row_push(ctx, 0.60f);
		nk_labelf(ctx, NK_TEXT_LEFT, "    %s", get_node_attr(nw, "Description"));
		nk_layout_row_push(ctx, 0.40f);
		if (strcmp(get_node_attr(nw, "Status"), "Active") == 0)
			color = nk_rgb(0, 255, 0); // Green
		nk_labelf_colored(ctx,
			NK_TEXT_LEFT, color,
			"%s%s",
			get_first_ipv4(nw),
			strcmp(get_node_attr(nw, "DHCP Enabled"), "Yes") == 0 ? " DHCP" : "");
		MAIN_GUI_ROW_2_END;
	}
}

VOID
gnwinfo_draw_main_window(struct nk_context* ctx, UINT width, UINT height)
{
	if (!nk_begin(ctx, "Summary",
		nk_rect(0, 0, (float)width, (float)height),
		NK_WINDOW_BACKGROUND | NK_WINDOW_TITLE))
		goto out;

	draw_os(ctx);
	draw_bios(ctx);
	draw_motherboard(ctx);
	draw_processor(ctx);
	draw_memory(ctx);
	draw_monitor(ctx);
	draw_storage(ctx);
	draw_network(ctx);

out:
	nk_end(ctx);
}
