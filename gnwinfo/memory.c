// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winioctl.h>
#include "gnwinfo.h"
#include "utils.h"

#define CLEAN_SYS_WORKING_SET   (1 << 0)
#define CLEAN_WORKING_SET       (1 << 1)
#define CLEAN_STANDBY_LIST      (1 << 2)
#define CLEAN_PR0_STANDBY_LIST  (1 << 3)
#define CLEAN_MODIFIED_PAGE     (1 << 4)
#define CLEAN_COMBINED_MM_LIST  (1 << 5)

static const char* human_sizes[6] =
{ "B", "K", "M", "G", "T", "P", };

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

VOID
gnwinfo_draw_mm_window(struct nk_context* ctx, float width, float height)
{
	static UINT64 clean_size = 0;
	if (g_ctx.gui_mm == FALSE)
		return;
	if (!nk_begin(ctx, gnwinfo_get_text(L"Memory"),
		nk_rect(width / 8.0f, height / 4.0f, width * 0.75f, height / 2.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE))
	{
		clean_size = 0;
		g_ctx.gui_mm = FALSE;
		goto out;
	}

	nk_layout_row_dynamic(ctx, 0, 2);

	nk_label(ctx, gnwinfo_get_text(L"Physical Memory"), NK_TEXT_LEFT);
	gnwinfo_draw_percent_prog(ctx, (double)g_ctx.mem_status.PhysUsage);
	nk_spacer(ctx);
	nk_labelf(ctx, NK_TEXT_LEFT, "%3lu%% %s / %s",
		g_ctx.mem_status.PhysUsage, g_ctx.mem_avail, g_ctx.mem_total);

	nk_label(ctx, gnwinfo_get_text(L"Page File"), NK_TEXT_LEFT);
	gnwinfo_draw_percent_prog(ctx, (double)g_ctx.mem_status.PageUsage);
	nk_spacer(ctx);
	nk_labelf(ctx, NK_TEXT_LEFT, "%3lu%% %s / %s",
		g_ctx.mem_status.PageUsage, g_ctx.page_avail, g_ctx.page_total);

	nk_label(ctx, gnwinfo_get_text(L"System Working Set"), NK_TEXT_LEFT);
	gnwinfo_draw_percent_prog(ctx, (double)g_ctx.mem_status.SfciUsage);
	nk_spacer(ctx);
	nk_labelf(ctx, NK_TEXT_LEFT, "%3lu%% %s / %s",
		g_ctx.mem_status.SfciUsage, g_ctx.sfci_avail, g_ctx.sfci_total);

	nk_layout_row_dynamic(ctx, 0, 3);
	nk_spacer(ctx);
	if (nk_button_label(ctx, gnwinfo_get_text(L"Clean Memory")))
	{

		UINT32 clean_flag = CLEAN_SYS_WORKING_SET | CLEAN_WORKING_SET |
			CLEAN_STANDBY_LIST | CLEAN_PR0_STANDBY_LIST | CLEAN_MODIFIED_PAGE | CLEAN_COMBINED_MM_LIST;
		clean_size = clean_memory(clean_flag);
	}
	if (clean_size)
		nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_good,"+%s", NWL_GetHumanSize(clean_size, human_sizes, 1024));
	else
		nk_spacer(ctx);

out:
	nk_end(ctx);
}
