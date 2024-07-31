// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winioctl.h>
#include <inttypes.h>
#include "gnwinfo.h"
#include "utils.h"

#define CLEAN_SYS_WORKING_SET   (1 << 0)
#define CLEAN_WORKING_SET       (1 << 1)
#define CLEAN_STANDBY_LIST      (1 << 2)
#define CLEAN_PR0_STANDBY_LIST  (1 << 3)
#define CLEAN_MODIFIED_PAGE     (1 << 4)
#define CLEAN_COMBINED_MM_LIST  (1 << 5)

static struct
{
	UINT64 clean_size;
	UINT32 clean_flag;
	BOOL page_drive_change;
	nk_size page_drive_mb;
	nk_size page_file_size;
	int page_rc;
	float col_height;
} m_ctx;

VOID
gnwinfo_init_mm_window(struct nk_context* ctx)
{
	g_ctx.window_flag |= GUI_WINDOW_MM;
	m_ctx.clean_size = 0;
	m_ctx.clean_flag = CLEAN_SYS_WORKING_SET | CLEAN_WORKING_SET | CLEAN_STANDBY_LIST | CLEAN_PR0_STANDBY_LIST | CLEAN_MODIFIED_PAGE | CLEAN_COMBINED_MM_LIST;
	m_ctx.page_drive_change = TRUE;
	m_ctx.page_drive_mb = 0;
	m_ctx.page_file_size = 8192;
	m_ctx.page_rc = 0;
	m_ctx.col_height = g_font_size + ctx->style.edit.padding.y * 2;
}

static UINT64
clean_memory(UINT32 clean_flag)
{
	UINT64 old_size = g_ctx.mem_status.PhysInUse;
	UINT64 new_size = 0;
	SYSTEM_MEMORY_LIST_COMMAND cmd;
	MEMORY_COMBINE_INFORMATION_EX combine_info = { 0 };
	SYSTEM_FILECACHE_INFORMATION sfci = { 0 };

	NWL_ObtainPrivileges(SE_INCREASE_QUOTA_NAME);
	NWL_ObtainPrivileges(SE_PROF_SINGLE_PROCESS_NAME);

	if (clean_flag & CLEAN_SYS_WORKING_SET)
	{
		sfci.MinimumWorkingSet = (ULONG_PTR)-1;
		sfci.MaximumWorkingSet = (ULONG_PTR)-1;
		NWL_NtSetSystemInformation(SystemFileCacheInformation, &sfci, sizeof(sfci));
	}
	if ((clean_flag & CLEAN_SYS_WORKING_SET) && (g_ctx.lib.NwOsInfo.dwMajorVersion >= 6)) // vista
	{
		cmd = MemoryEmptyWorkingSets;
		NWL_NtSetSystemInformation(SystemMemoryListInformation, &cmd, sizeof(cmd));
	}
	if ((clean_flag & CLEAN_PR0_STANDBY_LIST) && (g_ctx.lib.NwOsInfo.dwMajorVersion >= 6)) // vista
	{
		cmd = MemoryPurgeLowPriorityStandbyList;
		NWL_NtSetSystemInformation(SystemMemoryListInformation, &cmd, sizeof(cmd));
	}
	if ((clean_flag & CLEAN_STANDBY_LIST) && (g_ctx.lib.NwOsInfo.dwMajorVersion >= 6)) // vista
	{
		cmd = MemoryPurgeStandbyList;
		NWL_NtSetSystemInformation(SystemMemoryListInformation, &cmd, sizeof(cmd));
	}
	if ((clean_flag & CLEAN_MODIFIED_PAGE) && (g_ctx.lib.NwOsInfo.dwMajorVersion >= 6)) // vista
	{
		cmd = MemoryFlushModifiedList;
		NWL_NtSetSystemInformation(SystemMemoryListInformation, &cmd, sizeof(cmd));
	}
	if ((clean_flag & CLEAN_COMBINED_MM_LIST) && (g_ctx.lib.NwOsInfo.dwMajorVersion >= 10)) // win10
	{
		NWL_NtSetSystemInformation(SystemCombinePhysicalMemoryInformation, &combine_info, sizeof(combine_info));
	}
	NWL_GetMemInfo(&g_ctx.mem_status);
	new_size = g_ctx.mem_status.PhysInUse;
	if (new_size < old_size)
		return old_size - new_size;
	return 0;
}

static void
append_reg_multi_sz(HKEY root, LPCWSTR key, LPCWSTR value, LPCWSTR buf)
{
	WCHAR* old_reg = NULL;
	WCHAR* new_reg = NULL;
	DWORD old_size = 0;
	DWORD new_size = 0;
	DWORD type = 0;
	DWORD pos = 0;

	old_reg = NWL_NtGetRegValue(root, key, value, &old_size, &type);
	if (type != REG_MULTI_SZ)
		goto fail;
	old_size /= sizeof(WCHAR);
	if (old_reg != NULL && old_size > 2)
		pos = old_size - 1;
	else
		old_size = 0;
	new_size = old_size + (DWORD)wcslen(buf) + 1;
	new_reg = calloc(new_size, sizeof(WCHAR));
	if (!new_reg)
		goto fail;
	if (old_reg)
		memcpy(new_reg, old_reg, old_size * sizeof(WCHAR));
	memcpy(&new_reg[pos], buf, wcslen(buf) * sizeof(WCHAR));
	new_reg[new_size - 1] = L'\0';
	NWL_NtSetRegValue(root, key, value, new_reg, new_size * sizeof(WCHAR), REG_MULTI_SZ);
fail:
	if (old_reg)
		free(old_reg);
	if (new_reg)
		free(new_reg);
}

#define PAGE_FILE_REG L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"
static void
set_page_file_reg(WCHAR drive, LPCWSTR path, nk_size min_mb, nk_size max_mb)
{
	WCHAR buf[MAX_PATH];
	LPCWSTR key = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management";

	swprintf(buf, MAX_PATH, L"%C:%s %" PRIuPTR " %" PRIuPTR, drive, path, min_mb, max_mb);
	append_reg_multi_sz(HKEY_LOCAL_MACHINE, PAGE_FILE_REG, L"PagingFiles", buf);
	swprintf(buf, MAX_PATH, L"\\??\\%C:%s", drive, path);
	append_reg_multi_sz(HKEY_LOCAL_MACHINE, PAGE_FILE_REG, L"ExistingPageFiles", buf);
}

static WCHAR
draw_drive_list(struct nk_context* ctx)
{
	static const char* items[] =
		{ "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" };
	static int count = 0;
	static int selected = 2;
	int i = 0;
	if (nk_combo_begin_label(ctx, items[selected], nk_vec2(nk_widget_width(ctx), m_ctx.col_height * 4)))
	{
		DWORD mask = GetLogicalDrives();
		if (mask == 0)
			mask = 1 << 2;
		m_ctx.page_drive_change = TRUE;
		nk_layout_row_dynamic(ctx, m_ctx.col_height, 1);
		for (i = 0; i < 26; ++i)
		{
			if (mask & (1 << i))
			{
				if (nk_combo_item_label(ctx, items[i], NK_TEXT_LEFT))
					selected = i;
			}
		}
		nk_combo_end(ctx);
	}
	return items[selected][0];
}

static nk_size
get_drive_mb(WCHAR drive)
{
	WCHAR path[] = L"?:\\";
	ULARGE_INTEGER space = { 0 };
	path[0] = drive;
	if (GetDiskFreeSpaceExW(path, NULL, NULL, &space))
		return (nk_size)(space.QuadPart / (1024 * 1024));
	return 0;
}

static void
draw_size_editor(struct nk_context* ctx, WCHAR drive)
{
#define EDIT_SIZE 32
	static char buf[EDIT_SIZE] = "8192";
	nk_size size;
	if (m_ctx.page_drive_change)
	{
		m_ctx.page_drive_change = FALSE;
		m_ctx.page_drive_mb = get_drive_mb(drive);
		if (m_ctx.page_file_size > m_ctx.page_drive_mb)
			m_ctx.page_file_size = m_ctx.page_drive_mb / 2;
		snprintf(buf, EDIT_SIZE, "%" PRIuPTR, m_ctx.page_file_size);
	}
	if (m_ctx.page_file_size > m_ctx.page_drive_mb)
	{
		m_ctx.page_file_size = m_ctx.page_drive_mb;
		snprintf(buf, EDIT_SIZE, "%" PRIuPTR, m_ctx.page_file_size);
	}
	ctx->style.progress.cursor_normal = nk_style_item_color(g_color_good);
	ctx->style.progress.cursor_hover = nk_style_item_color(g_color_good);
	ctx->style.progress.cursor_active = nk_style_item_color(g_color_good);
	if (nk_progress(ctx, &m_ctx.page_file_size, m_ctx.page_drive_mb, TRUE))
	{
		snprintf(buf, EDIT_SIZE, "%" PRIuPTR, m_ctx.page_file_size);
	}
	size = (nk_size)strtoul(buf, NULL, 10);
	if (size != m_ctx.page_file_size)
	{
		m_ctx.page_file_size = size;
	}
	nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, buf, EDIT_SIZE, nk_filter_decimal);
}

static void
draw_page_file(struct nk_context* ctx)
{
	WCHAR drive = L'C';

	if (!nk_group_begin(ctx, gnwinfo_get_text(L"Create Paging Files"), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
		return;

	nk_layout_row_dynamic(ctx, 8, 1);
	nk_spacer(ctx);

	nk_layout_row(ctx, NK_DYNAMIC, m_ctx.col_height, 3, (float[3]) { 0.05f, 0.20f, 0.75f });
	nk_spacer(ctx);
	drive = draw_drive_list(ctx);
	nk_lf(ctx, NK_TEXT_LEFT, "%lC:\\pagefile.sys", drive);

	nk_layout_row_dynamic(ctx, 10, 1);
	nk_spacer(ctx);

	nk_layout_row(ctx, NK_DYNAMIC, m_ctx.col_height, 4, (float[4]) { 0.05f, 0.20f, 0.3f, 0.45f });
	nk_spacer(ctx);
	draw_size_editor(ctx, drive);
	nk_lf(ctx, NK_TEXT_LEFT, "MB / %" PRIuPTR " MB", m_ctx.page_drive_mb);

	nk_layout_row_dynamic(ctx, 10, 1);
	nk_spacer(ctx);

	nk_layout_row(ctx, NK_DYNAMIC, m_ctx.col_height, 3, (float[3]) { 0.45f, 0.1f, 0.45f });
	nk_spacer(ctx);
	if (nk_button_label(ctx, gnwinfo_get_text(L"OK")))
	{
		if (NWL_NtCreatePageFile(drive, L"\\pagefile.sys", m_ctx.page_file_size, m_ctx.page_file_size))
		{
			set_page_file_reg(drive, L"\\pagefile.sys", m_ctx.page_file_size, m_ctx.page_file_size);
			m_ctx.page_rc = 1;
		}
		else
			m_ctx.page_rc = -1;
	}
	if (m_ctx.page_rc > 0)
		nk_lhc(ctx, gnwinfo_get_text(L"SUCCESS"), NK_TEXT_CENTERED, g_color_good);
	else if (m_ctx.page_rc < 0)
		nk_lhc(ctx, gnwinfo_get_text(L"FAILED"), NK_TEXT_CENTERED, g_color_error);
	else
		nk_spacer(ctx);

	nk_group_end(ctx);
}

VOID
gnwinfo_draw_mm_window(struct nk_context* ctx, float width, float height)
{
	if (!(g_ctx.window_flag & GUI_WINDOW_MM))
		return;
	if (!nk_begin(ctx, gnwinfo_get_text(L"Memory"),
		nk_rect(width / 8.0f, height / 4.0f, width * 0.75f, height / 2.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE))
	{
		g_ctx.window_flag &= ~GUI_WINDOW_MM;
		goto out;
	}

	nk_layout_row_dynamic(ctx, 0, 2);

	nk_l(ctx, gnwinfo_get_text(L"Physical Memory"), NK_TEXT_LEFT);
	gnwinfo_draw_percent_prog(ctx, (double)g_ctx.mem_status.PhysUsage);
	nk_spacer(ctx);
	nk_lf(ctx, NK_TEXT_LEFT, "%3lu%% %s / %s",
		g_ctx.mem_status.PhysUsage, g_ctx.mem_status.StrPhysAvail, g_ctx.mem_status.StrPhysTotal);

	nk_l(ctx, gnwinfo_get_text(L"Page File"), NK_TEXT_LEFT);
	gnwinfo_draw_percent_prog(ctx, (double)g_ctx.mem_status.PageUsage);
	nk_spacer(ctx);
	nk_lf(ctx, NK_TEXT_LEFT, "%3lu%% %s / %s",
		g_ctx.mem_status.PageUsage, g_ctx.mem_status.StrPageAvail, g_ctx.mem_status.StrPageTotal);

	nk_l(ctx, gnwinfo_get_text(L"System Working Set"), NK_TEXT_LEFT);
	gnwinfo_draw_percent_prog(ctx, (double)g_ctx.mem_status.SfciUsage);
	nk_spacer(ctx);
	nk_lf(ctx, NK_TEXT_LEFT, "%3lu%% %s / %s",
		g_ctx.mem_status.SfciUsage, g_ctx.mem_status.StrSfciAvail, g_ctx.mem_status.StrSfciTotal);

	nk_layout_row_dynamic(ctx, 8, 1);
	nk_spacer(ctx);

	nk_layout_row_dynamic(ctx, m_ctx.col_height, 3);
	nk_spacer(ctx);
	if (nk_button_label(ctx, gnwinfo_get_text(L"Clean Memory")))
		m_ctx.clean_size = clean_memory(m_ctx.clean_flag);
	if (m_ctx.clean_size)
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_good,"+%s", NWL_GetHumanSize(m_ctx.clean_size, NWLC->NwUnits, 1024));
	else
		nk_spacer(ctx);

	nk_layout_row_dynamic(ctx, 8, 1);
	nk_spacer(ctx);

	nk_layout_row_dynamic(ctx, m_ctx.col_height * 9, 1);
	draw_page_file(ctx);

out:
	nk_end(ctx);
}
