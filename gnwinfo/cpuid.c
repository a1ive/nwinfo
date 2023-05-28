// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

#define CPUID_ROW_BEGIN_EX(row,ratio,h) \
	nk_layout_row_begin(ctx, NK_DYNAMIC, h, row); \
	nk_layout_row_push(ctx, ratio);

#define CPUID_ROW_BEGIN(row,ratio) \
	CPUID_ROW_BEGIN_EX(row,ratio,0)

#define CPUID_ROW_PUSH(ratio) \
	nk_layout_row_push(ctx, ratio);

#define CPUID_ROW_END \
	nk_layout_row_end(ctx);

static void
draw_features(struct nk_context* ctx, PNODE cpu)
{
	LPCSTR c;
	LPCSTR feature = gnwinfo_get_node_attr(cpu, "Features");
	if (nk_group_begin(ctx, "Features", NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		nk_layout_row_dynamic(ctx, 0, 1);
		for (c = feature; *c != '\0'; c += strlen(c) + 1)
			nk_label_colored(ctx, c, NK_TEXT_CENTERED, NK_COLOR_WHITE);
		nk_group_end(ctx);
	}
}

static void
draw_cache(struct nk_context* ctx, PNODE cpu)
{
	PNODE cache = NULL;
	if (!cpu)
		goto draw;
	cache = NWL_NodeGetChild(cpu, "Cache");
draw:
	if (nk_group_begin(ctx, "Cache", NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		const float ratio[] = { 0.2f, 0.8f };
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratio);
		nk_label(ctx, "L1 D", NK_TEXT_LEFT);
		nk_label_colored(ctx, gnwinfo_get_node_attr(cache, "L1 D"), NK_TEXT_LEFT, NK_COLOR_WHITE);
		nk_label(ctx, "L1 I", NK_TEXT_LEFT);
		nk_label_colored(ctx, gnwinfo_get_node_attr(cache, "L1 I"), NK_TEXT_LEFT, NK_COLOR_WHITE);
		nk_label(ctx, "L2", NK_TEXT_LEFT);
		nk_label_colored(ctx, gnwinfo_get_node_attr(cache, "L2"), NK_TEXT_LEFT, NK_COLOR_WHITE);
		nk_label(ctx, "L3", NK_TEXT_LEFT);
		nk_label_colored(ctx, gnwinfo_get_node_attr(cache, "L3"), NK_TEXT_LEFT, NK_COLOR_WHITE);
		nk_label(ctx, "L4", NK_TEXT_LEFT);
		nk_label_colored(ctx, gnwinfo_get_node_attr(cache, "L4"), NK_TEXT_LEFT, NK_COLOR_WHITE);
		nk_group_end(ctx);
	}
}

static void
draw_core(struct nk_context* ctx, PNODE cpu)
{
	//NWL_NodeAttrSet(cpu, "Cores", "9", NAFLG_FMT_NUMERIC);
	//NWL_NodeAppendNew(cpu, "CORE9", 0);
	LPCSTR core_count_str = gnwinfo_get_node_attr(cpu, "Cores");
	INT count = strtol(core_count_str, NULL, 10);
	if (nk_group_begin(ctx, "Cores", NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		CHAR name[32];
		PNODE core = NULL;
		static int cur_core = 0;
		if (count > 0)
		{
			const float ratio[] = { 0.5f, 0.5f };
			nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratio);
			nk_property_int(ctx, "Core", 0, &cur_core, count - 1, 1, 1);
			nk_labelf(ctx, NK_TEXT_CENTERED, "Total %s", core_count_str);
			snprintf(name, sizeof(name), "CORE%d", cur_core);
			core = NWL_NodeGetChild(cpu, name);
			nk_label(ctx, "Multiplier", NK_TEXT_LEFT);
			nk_label_colored(ctx, gnwinfo_get_node_attr(core, "Multiplier"), NK_TEXT_LEFT, NK_COLOR_WHITE);
			nk_label(ctx, "Bus Clock", NK_TEXT_LEFT);
			nk_labelf_colored(ctx, NK_TEXT_LEFT, NK_COLOR_WHITE, "%s MHz",
				gnwinfo_get_node_attr(core, "Bus Clock (MHz)"));
			nk_label(ctx, "Temperature", NK_TEXT_LEFT);
			nk_labelf_colored(ctx, NK_TEXT_LEFT, NK_COLOR_WHITE, "%s (C)",
				gnwinfo_get_node_attr(core, "Temperature (C)"));
			nk_label(ctx, "Voltage", NK_TEXT_LEFT);
			nk_labelf_colored(ctx, NK_TEXT_LEFT, NK_COLOR_WHITE, "%s V",
				gnwinfo_get_node_attr(core, "Core Voltage (V)"));
		}
		nk_group_end(ctx);
	}
}

VOID
gnwinfo_draw_cpuid_window(struct nk_context* ctx, float width, float height)
{
	INT count;
	CHAR name[32];
	LPCSTR cpu_count_str;
	PNODE cpu = NULL;
	static int cur_cpu = 0;
	if (g_ctx.gui_cpuid == FALSE)
		return;
	if (!nk_begin(ctx, "CPUID",
		nk_rect(0, height / 4.0f, width, height / 2.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE))
	{
		g_ctx.gui_cpuid = FALSE;
		goto out;
	}
	//NWL_NodeAttrSet(g_ctx.cpuid, "Processor Count", "2", NAFLG_FMT_NUMERIC);
	//NWL_NodeAppendNew(g_ctx.cpuid, "CPU1", 0);
	cpu_count_str = gnwinfo_get_node_attr(g_ctx.cpuid, "Processor Count");
	count = strtol(cpu_count_str, NULL, 10);
	if (count <= 0)
		goto out;

	CPUID_ROW_BEGIN(2, 0.16f);
	nk_property_int(ctx, "CPU", 0, &cur_cpu, count - 1, 1, 1);
	CPUID_ROW_PUSH(0.84f);
	nk_labelf(ctx, NK_TEXT_CENTERED,
		"Total %s, %s threads, %s MHz",
		cpu_count_str,
		gnwinfo_get_node_attr(g_ctx.cpuid, "Total CPUs"),
		gnwinfo_get_node_attr(g_ctx.cpuid, "CPU Clock (MHz)"));
	CPUID_ROW_END;

	snprintf(name, sizeof(name), "CPU%d", cur_cpu);
	cpu = NWL_NodeGetChild(g_ctx.cpuid, name);

	CPUID_ROW_BEGIN(2, 0.2f);
	nk_label(ctx, "Brand", NK_TEXT_LEFT);
	CPUID_ROW_PUSH(0.8f);
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Brand"), NK_TEXT_LEFT, NK_COLOR_WHITE);
	CPUID_ROW_END;

	CPUID_ROW_BEGIN(4, 0.2f);
	nk_label(ctx, "Hypervisor", NK_TEXT_LEFT);
	CPUID_ROW_PUSH(0.3f);
	nk_label_colored(ctx, gnwinfo_get_node_attr(g_ctx.cpuid, "Hypervisor"), NK_TEXT_LEFT, NK_COLOR_WHITE);
	CPUID_ROW_PUSH(0.2f);
	nk_label(ctx, "Code Name", NK_TEXT_LEFT);
	CPUID_ROW_PUSH(0.3f);
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Code Name"), NK_TEXT_LEFT, NK_COLOR_WHITE);
	CPUID_ROW_END;

	CPUID_ROW_BEGIN(4, 0.2f);
	nk_label(ctx, "Cores", NK_TEXT_LEFT);
	CPUID_ROW_PUSH(0.3f);
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Cores"), NK_TEXT_LEFT, NK_COLOR_WHITE);
	CPUID_ROW_PUSH(0.2f);
	nk_label(ctx, "Threads", NK_TEXT_LEFT);
	CPUID_ROW_PUSH(0.3f);
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Logical CPUs"), NK_TEXT_LEFT, NK_COLOR_WHITE);
	CPUID_ROW_END;

	CPUID_ROW_BEGIN(6, 0.2f);
	nk_label(ctx, "Family", NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Family"), NK_TEXT_LEFT, NK_COLOR_WHITE);
	CPUID_ROW_PUSH(0.2f);
	nk_label(ctx, "Model", NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Model"), NK_TEXT_LEFT, NK_COLOR_WHITE);
	CPUID_ROW_PUSH(0.2f);
	nk_label(ctx, "Stepping", NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Stepping"), NK_TEXT_LEFT, NK_COLOR_WHITE);
	CPUID_ROW_END;

	CPUID_ROW_BEGIN(6, 0.2f);
	nk_label(ctx, "Ext.Family", NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Ext.Family"), NK_TEXT_LEFT, NK_COLOR_WHITE);
	CPUID_ROW_PUSH(0.2f);
	nk_label(ctx, "Ext.Model", NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Ext.Model"), NK_TEXT_LEFT, NK_COLOR_WHITE);
	CPUID_ROW_PUSH(0.2f);
	nk_label(ctx, "Aff.Mask", NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Affinity Mask"), NK_TEXT_LEFT, NK_COLOR_WHITE);
	CPUID_ROW_END;

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_spacer(ctx);

	struct nk_rect rect = nk_layout_widget_bounds(ctx);
	CPUID_ROW_BEGIN_EX(3, 0.2f, NK_MAX(rect.h * 10, 100.0f));
	draw_features(ctx, cpu);
	CPUID_ROW_PUSH(0.4f);
	draw_cache(ctx, cpu);
	CPUID_ROW_PUSH(0.4f);
	draw_core(ctx, cpu);
	CPUID_ROW_END;

out:
	nk_end(ctx);
}
