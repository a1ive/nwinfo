// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <shellapi.h>
#include "gnwinfo.h"
#include "utils.h"

static CHAR m_buf[MAX_PATH];

static VOID
draw_os(struct nk_context* ctx)
{
	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.3f, 0.7f - g_ctx.gui_ratio, g_ctx.gui_ratio });
	nk_image_label(ctx, GET_PNG(IDR_PNG_OS), gnwinfo_get_text(L"Operating System"), NK_TEXT_LEFT, g_color_text_d);
	snprintf(m_buf, MAX_PATH, "%s %s",
		NWL_NodeAttrGet(g_ctx.system, "OS"),
		NWL_NodeAttrGet(g_ctx.system, "Processor Architecture"));
	if (g_ctx.main_flag & MAIN_OS_EDITIONID)
	{
		LPCSTR edition = NWL_NodeAttrGet(g_ctx.system, "Edition");
		if (edition[0] != '-')
			snprintf(m_buf, MAX_PATH, "%s %s", m_buf, edition);
	}
	if (g_ctx.main_flag & MAIN_OS_BUILD)
		snprintf(m_buf, MAX_PATH, "%s (%s)", m_buf, NWL_NodeAttrGet(g_ctx.system, "Build Number"));
	nk_lhc(ctx, m_buf, NK_TEXT_LEFT, g_color_text_l);
	if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_INFO), NULL))
		ShellExecuteW(GetDesktopWindow(), NULL,
			L"::{26EE0668-A00A-44D7-9371-BEB064C98683}\\5\\::{BB06C0E4-D293-4F75-8A90-CB05B6477EEE}",
			NULL, NULL, SW_NORMAL);

	if (g_ctx.main_flag & MAIN_OS_DETAIL)
	{
		nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.3f, 0.7f - g_ctx.gui_ratio, g_ctx.gui_ratio });
		nk_lhsc(ctx, gnwinfo_get_text(L"Login Status"), NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
		nk_lhcf(ctx, NK_TEXT_LEFT,
			g_color_text_l,
			"%s@%s%s%s%s%s",
			NWL_NodeAttrGet(g_ctx.system, "Username"),
			g_ctx.sys_hostname,
			strcmp(NWL_NodeAttrGet(g_ctx.system, "Safe Mode"), "Yes") == 0 ? " SafeMode" : "",
			strcmp(NWL_NodeAttrGet(g_ctx.system, "BitLocker Boot"), "Yes") == 0 ? " BitLocker" : "",
			strcmp(NWL_NodeAttrGet(g_ctx.system, "VHD Boot"), "Yes") == 0 ? " VHD" : "",
			strcmp(NWL_NodeAttrGet(g_ctx.system, "Fast Startup"), "Yes") == 0 ? " FastStartup" : "");
		if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_EDIT), gnwinfo_get_text(L"Hostname")))
			gnwinfo_init_hostname_window(ctx);
	}

	if (g_ctx.main_flag & MAIN_OS_UPTIME)
	{
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });
		nk_lhsc(ctx, gnwinfo_get_text(L"Uptime"), NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
		nk_lhc(ctx, g_ctx.sys_uptime, NK_TEXT_LEFT, g_color_text_l);
	}
}

static VOID
draw_bios(struct nk_context* ctx)
{
	LPCSTR tpm = NWL_NodeAttrGet(g_ctx.system, "TPM");
	LPCSTR sb = NWL_NodeAttrGet(g_ctx.uefi, "Secure Boot");

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.3f, 0.7f - g_ctx.gui_ratio, g_ctx.gui_ratio });
	nk_image_label(ctx, GET_PNG(IDR_PNG_FIRMWARE), gnwinfo_get_text(L"BIOS"), NK_TEXT_LEFT, g_color_text_d);

	strcpy_s(m_buf, MAX_PATH, NWL_NodeAttrGet(g_ctx.system, "Firmware"));
	if (sb[0] == 'E')
		snprintf(m_buf, MAX_PATH, "%s %s", m_buf, gnwinfo_get_text(L"SecureBoot"));
	else if (sb[0] == 'D')
		snprintf(m_buf, MAX_PATH, "%s %s", m_buf, gnwinfo_get_text(L"SecureBootOff"));

	if (tpm[0] == 'v')
		snprintf(m_buf, MAX_PATH, "%s TPM%s", m_buf, tpm);

	nk_lhc(ctx, m_buf, NK_TEXT_LEFT, g_color_text_l);

	if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_DMI), "SMBIOS"))
		g_ctx.window_flag |= GUI_WINDOW_DMI;

	if (g_ctx.main_flag & MAIN_B_VENDOR)
	{
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });
		nk_lhsc(ctx, gnwinfo_get_text(L"Vendor"), NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
		nk_lhc(ctx, gnwinfo_get_smbios_attr("0", "Vendor", NULL, NULL), NK_TEXT_LEFT, g_color_text_l);
	}
	if (g_ctx.main_flag & MAIN_B_VERSION)
	{
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });
		nk_lhsc(ctx, gnwinfo_get_text(L"Version"), NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l,
			"%s %s",
			gnwinfo_get_smbios_attr("0", "Version", NULL, NULL),
			gnwinfo_get_smbios_attr("0", "Release Date", NULL, NULL));
	}
}

static BOOL
is_motherboard(PNODE node, const PVOID ctx)
{
	LPCSTR str = NWL_NodeAttrGet(node, "Board Type");
	return (strcmp(str, "Motherboard") == 0);
}

static VOID
draw_computer(struct nk_context* ctx)
{
	struct nk_color color = g_color_unknown;
	BOOL has_battery = TRUE;
	LPCSTR time = "";
	LPCSTR bat = NWL_NodeAttrGet(g_ctx.battery, "Battery Status");
	LPCSTR ac = "";

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 1.0f - g_ctx.gui_ratio, g_ctx.gui_ratio });
	nk_image_label(ctx, GET_PNG(IDR_PNG_PC), gnwinfo_get_text(L"Computer"), NK_TEXT_LEFT, g_color_text_d);
	if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_PCI), "PCI"))
		g_ctx.window_flag |= GUI_WINDOW_PCI;

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });
	nk_lhsc(ctx, gnwinfo_get_smbios_attr("1", "Manufacturer", NULL, NULL), NK_TEXT_LEFT, g_color_text_d, nk_true, nk_true);
	nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l,
		"%s %s %s",
		gnwinfo_get_smbios_attr("1", "Product Name", NULL, NULL),
		gnwinfo_get_smbios_attr("3", "Type", NULL, NULL),
		gnwinfo_get_smbios_attr("1", "Serial Number", NULL, NULL));

	nk_lhsc(ctx, gnwinfo_get_smbios_attr("2", "Manufacturer", NULL, is_motherboard), NK_TEXT_LEFT, g_color_text_d, nk_true, nk_true);
	nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l,
		"%s %s",
		gnwinfo_get_smbios_attr("2", "Product Name", NULL, is_motherboard),
		gnwinfo_get_smbios_attr("2", "Serial Number", NULL, is_motherboard));

	if (strcmp(bat, "Charging") == 0)
	{
		color = g_color_good;
		time = NWL_NodeAttrGet(g_ctx.battery, "Battery Life Full");
	}
	else if (strcmp(bat, "Not Charging") == 0)
	{
		color = g_color_warning;
		time = NWL_NodeAttrGet(g_ctx.battery, "Battery Life Remaining");
	}
	else
		has_battery = FALSE;

	if (strcmp(time, "UNKNOWN") == 0)
		time = "";

	if (strcmp(NWL_NodeAttrGet(g_ctx.battery, "AC Power"), "Online") == 0)
		ac = u8"AC ";

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.3f, 0.7f - g_ctx.gui_ratio, g_ctx.gui_ratio });
	nk_lhsc(ctx, gnwinfo_get_text(L"Power Status"), NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
	snprintf(m_buf, MAX_PATH, "%s %s",
		ac, NWL_NodeAttrGet(g_ctx.battery, "Active Power Scheme Name"));
	if (has_battery)
	{
		snprintf(m_buf, MAX_PATH, "%s %s %s",
			m_buf,
			NWL_NodeAttrGet(g_ctx.battery, "Battery Life Percentage"),
			time);
	}
	nk_lhc(ctx, m_buf, NK_TEXT_LEFT, g_color_text_l);
	if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_BATTERY), NULL))
		ShellExecuteW(GetDesktopWindow(), NULL,
			L"shell:::{025A5937-A6BE-4686-A844-36FE4BEC8B6D}",
			NULL, NULL, SW_NORMAL);
}

static BOOL
is_cache_level_equal(PNODE node, const PVOID ctx)
{
	UINT8 cache_level = *(const PUINT8)ctx;
	LPCSTR str = NWL_NodeAttrGet(node, "Cache Level");
	CHAR buf[] = "L1";
	buf[1] += cache_level;
	return (strcmp(str, buf) == 0);
}

static VOID
draw_processor(struct nk_context* ctx)
{
	INT i;
	CHAR name[32];

	nk_layout_row(ctx, NK_DYNAMIC, 0, 4, (float[4]) { 0.3f, 0.4f, 0.3f - g_ctx.gui_ratio, g_ctx.gui_ratio });
	nk_image_label(ctx, GET_PNG(IDR_PNG_CPU), gnwinfo_get_text(L"Processor"), NK_TEXT_LEFT, g_color_text_d);
	nk_lhcf(ctx, NK_TEXT_LEFT, gnwinfo_get_color(g_ctx.cpu_usage, 70.0, 90.0),
		"%.2f%% %lu MHz",
		g_ctx.cpu_usage,
		g_ctx.cpu_freq);
	gnwinfo_draw_percent_prog(ctx, g_ctx.cpu_usage);
	if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_CPUID), "CPUID"))
		g_ctx.window_flag |= GUI_WINDOW_CPUID;

	for (i = 0; i < g_ctx.cpu_count; i++)
	{
		snprintf(name, sizeof(name), "CPU%d", i);
		PNODE cpu = NWL_NodeGetChild(g_ctx.cpuid, name);
		LPCSTR brand = NWL_NodeAttrGet(cpu, "Brand");

		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });
		nk_lhsc(ctx, name, NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
		nk_lhc(ctx, brand, NK_TEXT_LEFT, g_color_text_l);

		if (!(g_ctx.main_flag & MAIN_CPU_DETAIL))
			continue;

		nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.3f, 0.4f, 0.3f });
		nk_spacer(ctx);
		snprintf(m_buf, MAX_PATH, "%s %s", NWL_NodeAttrGet(cpu, "Cores"), gnwinfo_get_text(L"cores"));
		snprintf(m_buf, MAX_PATH, "%s %s %s", m_buf,
			NWL_NodeAttrGet(cpu, "Logical CPUs"),
			gnwinfo_get_text(L"threads"));
		if (g_ctx.cpu_info[i].MsrPower > 0.0)
			snprintf(m_buf, MAX_PATH, "%s %.2fW", m_buf, g_ctx.cpu_info[i].MsrPower);

		nk_lhc(ctx, m_buf, NK_TEXT_LEFT, g_color_text_l);
		if (g_ctx.cpu_info[i].MsrTemp > 0)
			nk_lhcf(ctx, NK_TEXT_LEFT,
				gnwinfo_get_color((double)g_ctx.cpu_info[i].MsrTemp, 65.0, 85.0),
				u8"%d\u2103", g_ctx.cpu_info[i].MsrTemp);
		else
			nk_spacer(ctx);
	}

	if (g_ctx.main_flag & MAIN_CPU_CACHE)
	{
		LPCSTR cache_size[4];
		for (UINT8 cache_level = 0; cache_level < 4; cache_level++)
			cache_size[cache_level] = gnwinfo_get_smbios_attr("7", "Installed Cache Size", &cache_level, is_cache_level_equal);

		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });
		nk_lhsc(ctx, gnwinfo_get_text(L"Cache"), NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
		snprintf(m_buf, MAX_PATH, "L1 %s", cache_size[0]);
		if (cache_size[1][0] != '-')
			snprintf(m_buf, MAX_PATH, "%s L2 %s", m_buf, cache_size[1]);
		if (cache_size[2][0] != '-')
			snprintf(m_buf, MAX_PATH, "%s L3 %s", m_buf, cache_size[2]);
		if (cache_size[3][0] != '-')
			snprintf(m_buf, MAX_PATH, "%s L4 %s", m_buf, cache_size[3]);
		nk_lhc(ctx, m_buf, NK_TEXT_LEFT, g_color_text_l);
	}
}

static VOID
draw_mem_capacity(struct nk_context* ctx)
{
	nk_lhsc(ctx, gnwinfo_get_text(L"Max Capacity"), NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
	LPCSTR id = "16";
	LPCSTR capacity = gnwinfo_get_smbios_attr(id, "Max Capacity", NULL, NULL);
	if (capacity[0] == '-')
	{
		id = "5";
		capacity = gnwinfo_get_smbios_attr(id, "Max Memory Module Size (MB)", NULL, NULL);
	}
	snprintf(m_buf, MAX_PATH, "%s %s %s%s",
		gnwinfo_get_smbios_attr(id, "Number of Slots", NULL, NULL),
		gnwinfo_get_text(L"slots"),
		capacity,
		id[0] == '5' ? " MB" : "");
	nk_lhc(ctx, m_buf, NK_TEXT_LEFT, g_color_text_l);
}

static VOID
draw_memory(struct nk_context* ctx)
{
	INT i;

	nk_layout_row(ctx, NK_DYNAMIC, 0, 4, (float[4]) { 0.3f, 0.4f, 0.3f - g_ctx.gui_ratio, g_ctx.gui_ratio });
	nk_image_label(ctx, GET_PNG(IDR_PNG_MEMORY), gnwinfo_get_text(L"Memory"), NK_TEXT_LEFT, g_color_text_d);
	nk_lhcf(ctx, NK_TEXT_LEFT,
		gnwinfo_get_color((double)g_ctx.mem_status.PhysUsage, 70.0, 90.0),
		"%lu%% %s / %s",
		g_ctx.mem_status.PhysUsage, g_ctx.mem_status.StrPhysAvail, g_ctx.mem_status.StrPhysTotal);
	gnwinfo_draw_percent_prog(ctx, (double)g_ctx.mem_status.PhysUsage);
	if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_ROCKET), gnwinfo_get_text(L"Clean Memory")))
		gnwinfo_init_mm_window(ctx);

	if (g_ctx.main_flag & MAIN_MEM_DETAIL)
	{
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });
		draw_mem_capacity(ctx);
		for (i = 0; g_ctx.smbios->Children[i].LinkedNode; i++)
		{
			PNODE tab = g_ctx.smbios->Children[i].LinkedNode;
			LPCSTR attr = NWL_NodeAttrGet(tab, "Table Type");
			if (strcmp(attr, "17") != 0)
				continue;
			LPCSTR ddr = NWL_NodeAttrGet(tab, "Device Type");
			if (ddr[0] == '-')
				continue;
			nk_lhsc(ctx, NWL_NodeAttrGet(tab, "Bank Locator"), NK_TEXT_LEFT, g_color_text_d, nk_true, nk_true);
			nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l,
				"%s-%s %s %s %s",
				ddr,
				NWL_NodeAttrGet(tab, "Speed (MT/s)"),
				NWL_NodeAttrGet(tab, "Device Size"),
				NWL_NodeAttrGet(tab, "Manufacturer"),
				NWL_NodeAttrGet(tab, "Serial Number"));
		}
	}
}

static VOID
draw_display(struct nk_context* ctx)
{
	INT i;

	nk_layout_row(ctx, NK_DYNAMIC, 0, 4, (float[4]) { 0.3f, 0.4f, 0.3f - g_ctx.gui_ratio, g_ctx.gui_ratio });
	nk_image_label(ctx, GET_PNG(IDR_PNG_DISPLAY), gnwinfo_get_text(L"Display Devices"), NK_TEXT_LEFT, g_color_text_d);
	nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l,
		"%ldx%ld %u DPI (%u%%)",
		g_ctx.cur_display.Width, g_ctx.cur_display.Height, g_ctx.cur_display.Dpi, g_ctx.cur_display.Scale);
	if (g_ctx.gpu_info.Usage3D > 0.0f || g_ctx.gpu_info.UsageDedicated > 0.0f)
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l,
			"GPU 3D %.1f%% RAM %.1f%%",
			g_ctx.gpu_info.Usage3D,
			g_ctx.gpu_info.UsageDedicated);
	else
		nk_spacer(ctx);
	if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_MONITOR), gnwinfo_get_text(L"Display Devices")))
		g_ctx.window_flag |= GUI_WINDOW_DISPLAY;

	for (i = 0; i < g_ctx.gpu_info.DeviceCount; i++)
	{
		if (g_ctx.gpu_info.Device[i].driver)
		{
			CHAR name[32];
			snprintf(name, sizeof(name), "GPU%d", i);
			nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.3f, 0.4f, 0.3f });
			nk_lhsc(ctx, name, NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
			nk_lhc(ctx, g_ctx.gpu_info.Device[i].gpu_device, NK_TEXT_LEFT, g_color_text_l);
			nk_lhc(ctx, NWL_GetHumanSize(g_ctx.gpu_info.Device[i].gpu_mem_size, NWLC->NwUnits, 1024),
				NK_TEXT_LEFT, g_color_text_l);
		}
		else
		{
			nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });
			nk_lhsc(ctx, g_ctx.gpu_info.Device[i].gpu_vendor, NK_TEXT_LEFT, g_color_text_d, nk_true, nk_true);
			nk_lhc(ctx, g_ctx.gpu_info.Device[i].gpu_device, NK_TEXT_LEFT, g_color_text_l);
		}
	}
	for (i = 0; g_ctx.edid->Children[i].LinkedNode; i++)
	{
		PNODE mon = g_ctx.edid->Children[i].LinkedNode;
		LPCSTR id = NWL_NodeAttrGet(mon, "ID");
		if (id[0] == '-')
			continue;
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.3f, 0.7f });
		nk_lhsc(ctx, NWL_NodeAttrGet(mon, "Manufacturer"), NK_TEXT_LEFT, g_color_text_d, nk_true, nk_true);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l,
			"%s %s@%sHz %s\" %s",
			id,
			NWL_NodeAttrGet(mon, "Max Resolution"),
			NWL_NodeAttrGet(mon, "Max Refresh Rate (Hz)"),
			NWL_NodeAttrGet(mon, "Diagonal (in)"),
			NWL_NodeAttrGet(mon, "Display Name"));
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
		LPCSTR attr = NWL_NodeAttrGet(mnt, "Drive Letter");
		if (attr[0] != '-')
			return attr;
	}
fail:
	return NULL;
}

static VOID
open_folder(LPCSTR drive_letter, LPCSTR volume_guid)
{
	LPCWSTR path = NULL;
	if (drive_letter)
		path = NWL_Utf8ToUcs2(drive_letter);
	else
		path = NWL_Utf8ToUcs2(volume_guid);
	ShellExecuteW(NULL, L"open", path, NULL, NULL, SW_NORMAL);
}

static VOID
draw_volume(struct nk_context* ctx, PNODE disk, BOOL cdrom)
{
	INT i;
	PNODE vol = NWL_NodeGetChild(disk, "Volumes");
	if (!vol)
		return;
	nk_layout_row(ctx, NK_DYNAMIC, 0, 5, (float[5]) { 0.12f, 0.18f, 0.4f, 0.3f - g_ctx.gui_ratio, g_ctx.gui_ratio });
	for (i = 0; vol->Children[i].LinkedNode; i++)
	{
		struct nk_image img = GET_PNG(IDR_PNG_DIR);
		PNODE tab = vol->Children[i].LinkedNode;
		LPCSTR path = NWL_NodeAttrGet(tab, "Path");
		LPCSTR drive = get_drive_letter(tab);
		LPCSTR volume_guid = NWL_NodeAttrGet(tab, "Volume GUID");
		double percent = strtod(NWL_NodeAttrGet(tab, "Usage"), NULL);
		if (strcmp(path, g_ctx.sys_disk) == 0)
			img = GET_PNG(IDR_PNG_OS);
		if (cdrom)
			img = GET_PNG(IDR_PNG_CD);
		nk_spacer(ctx);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_d, "[%s]",
			drive ? drive : NWL_NodeAttrGet(tab, "Partition Flag"));
		nk_lhcf(ctx, NK_TEXT_LEFT,
			g_color_text_l,
			"%s %s %s",
			NWL_NodeAttrGet(tab, "Total Space"),
			NWL_NodeAttrGet(tab, "Filesystem"),
			NWL_NodeAttrGet(tab, "Label"));
		if (g_ctx.main_flag & MAIN_VOLUME_PROG)
			gnwinfo_draw_percent_prog(ctx, percent);
		else
			nk_lhcf(ctx, NK_TEXT_LEFT,
				gnwinfo_get_color(percent, 70.0, 90.0),
				"%.0f%% %s: %s",
				percent,
				gnwinfo_get_text(L"Free"),
				NWL_NodeAttrGet(tab, "Free Space"));
		if (nk_button_image_hover(ctx, img, volume_guid))
			open_folder(drive, volume_guid);
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
		if (drive)
			count++;
	}
	nk_layout_row_begin(ctx, NK_STATIC, 0, count + 1);
	nk_layout_row_push(ctx, 0.3f * g_ctx.gui_width);
	nk_spacer(ctx);
	for (i = 0; vol->Children[i].LinkedNode; i++)
	{
		PNODE tab = vol->Children[i].LinkedNode;
		LPCSTR drive = get_drive_letter(tab);
		if (!drive)
			continue;
		buf[0] = drive[0];
		nk_layout_row_push(ctx, g_ctx.gui_ratio * g_ctx.gui_width);
		if (nk_button_label(ctx, buf))
			open_folder(drive, NULL);
	}
	nk_layout_row_end(ctx);
}

static VOID
draw_net_drive(struct nk_context* ctx)
{
	INT i;
	for (i = 0; g_ctx.smb->Children[i].LinkedNode; i++)
	{
		PNODE nd = g_ctx.smb->Children[i].LinkedNode;
		if (strcmp(nd->Name, "Drive") != 0)
			continue;
		LPCSTR local = NWL_NodeAttrGet(nd, "Local Name");
		LPCSTR remote = NWL_NodeAttrGet(nd, "Remote Name");
		nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.3f, 0.7f - g_ctx.gui_ratio, g_ctx.gui_ratio });
		nk_lhsc(ctx, gnwinfo_get_text(L"Network Drives"), NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "[%s] %s", local, remote);
		if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_DIR), NULL))
			open_folder(local, remote);
	}
}

static VOID
draw_net_drive_compact(struct nk_context* ctx)
{
	INT i;
	INT count;
	CHAR buf[] = "A";
	count = NWL_NodeChildCount(g_ctx.smb);
	if (count <= 0)
		return;
	nk_layout_row_begin(ctx, NK_STATIC, 0, count + 1);
	nk_layout_row_push(ctx, 0.3f * g_ctx.gui_width);
	nk_lhsc(ctx, gnwinfo_get_text(L"Network Drives"), NK_TEXT_LEFT, g_color_text_d, nk_false, nk_true);
	for (i = 0; g_ctx.smb->Children[i].LinkedNode; i++)
	{
		PNODE tab = g_ctx.smb->Children[i].LinkedNode;
		LPCSTR drive = NWL_NodeAttrGet(tab, "Local Name");
		buf[0] = drive[0];
		nk_layout_row_push(ctx, g_ctx.gui_ratio * g_ctx.gui_width);
		if (nk_button_label(ctx, buf))
			open_folder(drive, NULL);
	}
	nk_layout_row_end(ctx);
}

static VOID
draw_storage(struct nk_context* ctx)
{
	INT i;

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 1.0f - g_ctx.gui_ratio, g_ctx.gui_ratio });
	nk_image_label(ctx, GET_PNG(IDR_PNG_DISK), gnwinfo_get_text(L"Storage"), NK_TEXT_LEFT, g_color_text_d);
	if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_SMART), "S.M.A.R.T."))
		g_ctx.window_flag |= GUI_WINDOW_SMART;

	for (i = 0; g_ctx.disk->Children[i].LinkedNode; i++)
	{
		BOOL cdrom;
		LPCSTR prefix = "HD";
		LPCSTR path, id;
		LPCSTR ssd = "";
		PNODE disk = g_ctx.disk->Children[i].LinkedNode;
		if (!disk)
			continue;
		path = NWL_NodeAttrGet(disk, "Path");
		if (strncmp(path, "\\\\.\\CdRom", 9) == 0)
		{
			cdrom = TRUE;
			prefix = "CD";
			id = &path[9];
		}
		else if (strncmp(path, "\\\\.\\PhysicalDrive", 17) == 0)
		{
			cdrom = FALSE;
			id = &path[17];
			if (strcmp(NWL_NodeAttrGet(disk, "SSD"), "Yes") == 0)
				ssd = " SSD";
			if (strcmp(NWL_NodeAttrGet(disk, "Removable"), "Yes") == 0)
				prefix = "RM";
		}
		else
			continue;

		nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.3f, 0.4f, 0.23f });
		snprintf(m_buf, MAX_PATH, "%s%s %s%s",
			prefix,
			id,
			NWL_NodeAttrGet(disk, "Type"),
			ssd);
		nk_lhsc(ctx, m_buf, NK_TEXT_LEFT, g_color_text_d, nk_true, nk_true);
		nk_lhcf(ctx, NK_TEXT_LEFT,
			g_color_text_l,
			"%s %s %s",
			NWL_NodeAttrGet(disk, "Size"),
			NWL_NodeAttrGet(disk, "Partition Table"),
			NWL_NodeAttrGet(disk, "Product ID"));

		LPCSTR health = NWL_NodeAttrGet(disk, "Health Status");
		if ((g_ctx.main_flag & MAIN_DISK_SMART) && strcmp(health, "-") != 0)
		{
			LPCSTR life = strchr(health, '(');
			LPCWSTR whealth = L"Unknown";
			struct nk_color color = g_color_unknown;
			LPCSTR temp = NWL_NodeAttrGet(disk, "Temperature (C)");
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

			nk_lhcf(ctx, NK_TEXT_LEFT, color,
				u8"%s%s %s\u2103", gnwinfo_get_text(whealth), life,
				temp[0] == '-' ? "-" : temp);
		}
		else
			nk_spacer(ctx);
		if (g_ctx.main_flag & MAIN_DISK_COMPACT)
			draw_volume(ctx, disk, cdrom);
		else
			draw_volume_compact(ctx, disk);
	}
	if (g_ctx.main_flag & MAIN_DISK_COMPACT)
		draw_net_drive(ctx);
	else
		draw_net_drive_compact(ctx);
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
		LPCSTR addr = NWL_NodeAttrGet(ip, "IPv4");
		if (strcmp(addr, "-") != 0)
			return addr;
	}
	return "";
}

static VOID
draw_network(struct nk_context* ctx)
{
	INT i;

	nk_layout_row(ctx, NK_DYNAMIC, 0, 4, (float[4]) { 0.64f, 0.18f - g_ctx.gui_ratio, 0.18f, g_ctx.gui_ratio });
	nk_image_label(ctx, GET_PNG(IDR_PNG_NETWORK), gnwinfo_get_text(L"Network"), NK_TEXT_LEFT, g_color_text_d);
	nk_lhcf(ctx, NK_TEXT_LEFT, g_color_warning, u8"\u2191 %s", g_ctx.net_traffic.StrSend);
	nk_lhcf(ctx, NK_TEXT_LEFT, g_color_unknown, u8"\u2193 %s", g_ctx.net_traffic.StrRecv);
	if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_EDIT), NULL))
		ShellExecuteW(NULL, NULL, L"::{7007ACC7-3202-11D1-AAD2-00805FC1270E}", NULL, NULL, SW_NORMAL);

	for (i = 0; g_ctx.network->Children[i].LinkedNode; i++)
	{
		BOOL is_active = FALSE;
		PNODE nw = g_ctx.network->Children[i].LinkedNode;
		struct nk_color color = g_color_error;
		if (!nw)
			continue;
		if (strcmp(NWL_NodeAttrGet(nw, "Status"), "Active") == 0)
		{
			color = g_color_good;
			is_active = TRUE;
		}

		nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.64f, 0.36f - g_ctx.gui_ratio, g_ctx.gui_ratio });
		nk_lhsc(ctx, NWL_NodeAttrGet(nw, "Description"), NK_TEXT_LEFT, g_color_text_d, nk_true, nk_true);
		nk_lhc(ctx, get_first_ipv4(nw), NK_TEXT_LEFT, color);
		if (nk_button_image_hover(ctx,
			strcmp(NWL_NodeAttrGet(nw, "Type"), "IEEE 802.11 Wireless") == 0 ? GET_PNG(IDR_PNG_WLAN) : GET_PNG(IDR_PNG_ETH), NULL))
		{
			swprintf((WCHAR*)m_buf, MAX_PATH / sizeof(WCHAR),
				L"::{7007ACC7-3202-11D1-AAD2-00805FC1270E}\\::%s", NWL_Utf8ToUcs2(NWL_NodeAttrGet(nw, "Network Adapter")));
			ShellExecuteW(NULL, NULL, (WCHAR*)m_buf, NULL, NULL, SW_NORMAL);
		}

		if (g_ctx.main_flag & MAIN_NET_DETAIL)
		{
			nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.64f, 0.36f });
			strcpy_s(m_buf, MAX_PATH, strcmp(NWL_NodeAttrGet(nw, "DHCP Enabled"), "Yes") == 0 ? " DHCP" : "");
			if (is_active)
				snprintf(m_buf, MAX_PATH, u8"%s \u21c5 %s / %s",
					m_buf,
					NWL_NodeAttrGet(nw, "Transmit Link Speed"),
					NWL_NodeAttrGet(nw, "Receive Link Speed"));
			nk_lhsc(ctx, m_buf, NK_TEXT_LEFT, g_color_text_d, nk_true, nk_true);
			nk_lhc(ctx,
				NWL_NodeAttrGet(nw, "MAC Address"), NK_TEXT_LEFT, g_color_text_l);

			if (strcmp(NWL_NodeAttrGet(nw, "WLAN State"), "Connected") == 0)
			{
				nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.64f, 0.36f });
				nk_lhscf(ctx, NK_TEXT_LEFT, g_color_text_d, nk_true, nk_true, " %s%% %s",
					NWL_NodeAttrGet(nw, "WLAN Signal Quality"),
					NWL_NodeAttrGet(nw, "WLAN Profile"));
				nk_lhscf(ctx, NK_TEXT_LEFT, g_color_text_d, nk_true, nk_false, "%s %s",
					NWL_NodeAttrGet(nw, "WLAN Auth"),
					NWL_NodeAttrGet(nw, "WLAN Cipher"));
			}
		}
	}
}

static VOID
draw_audio(struct nk_context* ctx)
{
	UINT i;
	if (!g_ctx.audio)
		return;

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 1.0f - g_ctx.gui_ratio, g_ctx.gui_ratio });
	nk_image_label(ctx, GET_PNG(IDR_PNG_MM), gnwinfo_get_text(L"Audio Devices"), NK_TEXT_LEFT, g_color_text_d);
	if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_SETTINGS), NULL))
		ShellExecuteW(NULL, NULL, L"::{26EE0668-A00A-44D7-9371-BEB064C98683}\\2\\::{F2DDFC82-8F12-4CDD-B7DC-D4FE1425AA4D}", NULL, NULL, SW_NORMAL);
	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.7f, 0.3f });
	for (i = 0; i < g_ctx.audio_count; i++)
	{
		nk_lhsc(ctx, NWL_Ucs2ToUtf8(g_ctx.audio[i].name), NK_TEXT_LEFT, g_color_text_d, nk_true, nk_true);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l,
			"%s %.0f%%",
			g_ctx.audio[i].is_default ? "*" : " ",
			100.0f * g_ctx.audio[i].volume);
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

	nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 4);

	struct nk_rect rect = nk_layout_widget_bounds(ctx);
	g_ctx.gui_ratio = rect.h / rect.w;

	nk_layout_row_push(ctx, g_ctx.gui_ratio);
	if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_SETTINGS), gnwinfo_get_text(L"Settings")))
		g_ctx.window_flag |= GUI_WINDOW_SETTINGS;
	nk_layout_row_push(ctx, g_ctx.gui_ratio);
	if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_REFRESH), gnwinfo_get_text(L"Refresh")))
	{
		gnwinfo_ctx_update(IDT_TIMER_1M);
		gnwinfo_ctx_update(IDT_TIMER_DISK);
		gnwinfo_ctx_update(IDT_TIMER_DISPLAY);
		gnwinfo_ctx_update(IDT_TIMER_SMB);
	}
	nk_layout_row_push(ctx, g_ctx.gui_ratio);
	if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_INFO), gnwinfo_get_text(L"About")))
		g_ctx.window_flag |= GUI_WINDOW_ABOUT;
	nk_layout_row_push(ctx, g_ctx.gui_ratio);
	if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_CLOSE), gnwinfo_get_text(L"Close")))
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
	if (g_ctx.main_flag & MAIN_INFO_AUDIO)
		draw_audio(ctx);

	nk_end(ctx);
}
