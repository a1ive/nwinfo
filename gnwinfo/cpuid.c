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
	if (nk_group_begin(ctx, gnwinfo_get_text(L"Features"), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		nk_layout_row_dynamic(ctx, 0, 1);
		for (c = feature; *c != '\0'; c += strlen(c) + 1)
			nk_label_colored(ctx, c, NK_TEXT_CENTERED, g_color_text_l);
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
	if (nk_group_begin(ctx, gnwinfo_get_text(L"Cache"), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		const float ratio[] = { 0.2f, 0.8f };
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratio);
		nk_label(ctx, "L1 D", NK_TEXT_LEFT);
		nk_label_colored(ctx, gnwinfo_get_node_attr(cache, "L1 D"), NK_TEXT_LEFT, g_color_text_l);
		nk_label(ctx, "L1 I", NK_TEXT_LEFT);
		nk_label_colored(ctx, gnwinfo_get_node_attr(cache, "L1 I"), NK_TEXT_LEFT, g_color_text_l);
		nk_label(ctx, "L2", NK_TEXT_LEFT);
		nk_label_colored(ctx, gnwinfo_get_node_attr(cache, "L2"), NK_TEXT_LEFT, g_color_text_l);
		nk_label(ctx, "L3", NK_TEXT_LEFT);
		nk_label_colored(ctx, gnwinfo_get_node_attr(cache, "L3"), NK_TEXT_LEFT, g_color_text_l);
		nk_label(ctx, "L4", NK_TEXT_LEFT);
		nk_label_colored(ctx, gnwinfo_get_node_attr(cache, "L4"), NK_TEXT_LEFT, g_color_text_l);
		nk_group_end(ctx);
	}
}

static void
draw_msr(struct nk_context* ctx, int index)
{
	if (nk_group_begin(ctx, gnwinfo_get_text(L"MSR"), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		const float ratio[] = { 0.5f, 0.5f };
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratio);

		nk_label(ctx, gnwinfo_get_text(L"Multiplier"), NK_TEXT_LEFT);
		nk_label_colored(ctx, g_ctx.cpu_info[index].cpu_msr_multi, NK_TEXT_LEFT, g_color_text_l);
		nk_label(ctx, gnwinfo_get_text(L"Bus Clock"), NK_TEXT_LEFT);
		nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_text_l, "%.2f MHz", g_ctx.cpu_info[index].cpu_msr_bus);
		nk_label(ctx, gnwinfo_get_text(L"Temperature"), NK_TEXT_LEFT);
		nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_text_l, u8"%d \u00B0C", g_ctx.cpu_info[index].cpu_msr_temp);
		nk_label(ctx, gnwinfo_get_text(L"Voltage"), NK_TEXT_LEFT);
		nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_text_l, "%.2f V", g_ctx.cpu_info[index].cpu_msr_volt);
		nk_label(ctx, gnwinfo_get_text(L"Power"), NK_TEXT_LEFT);
		nk_labelf_colored(ctx, NK_TEXT_LEFT, g_color_text_l, "%.2f W", g_ctx.cpu_info[index].cpu_msr_power);

		nk_group_end(ctx);
	}
}

VOID
gnwinfo_draw_cpuid_window(struct nk_context* ctx, float width, float height)
{
	CHAR name[32];
	CHAR buf[MAX_PATH];
	PNODE cpu = NULL;
	static int cpu_index = 0;
	if (!(g_ctx.window_flag & GUI_WINDOW_CPUID))
		return;
	if (!nk_begin(ctx, "CPUID",
		nk_rect(0, height / 4.0f, width * 0.98f, height / 2.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE))
	{
		g_ctx.window_flag &= ~GUI_WINDOW_CPUID;
		goto out;
	}

	if (g_ctx.cpu_count <= 0 || !g_ctx.cpu_info)
		goto out;
	if (cpu_index >= g_ctx.cpu_count)
		cpu_index = 0;

	CPUID_ROW_BEGIN(2, 0.16f);
	nk_property_int(ctx, "#CPU", 0, &cpu_index, g_ctx.cpu_count - 1, 1, 1);
	CPUID_ROW_PUSH(0.84f);
	snprintf(buf, MAX_PATH, "%s %d,", gnwinfo_get_text(L"Total"), g_ctx.cpu_count);
	snprintf(buf, MAX_PATH, "%s %s %s, %s MHz", buf,
		gnwinfo_get_node_attr(g_ctx.cpuid, "Total CPUs"), gnwinfo_get_text(L"threads"),
		gnwinfo_get_node_attr(g_ctx.cpuid, "CPU Clock (MHz)"));
	nk_label(ctx, buf, NK_TEXT_CENTERED);
	CPUID_ROW_END;

	snprintf(name, sizeof(name), "CPU%d", cpu_index);
	cpu = NWL_NodeGetChild(g_ctx.cpuid, name);

	CPUID_ROW_BEGIN(2, 0.2f);
	nk_label(ctx, gnwinfo_get_text(L"Brand"), NK_TEXT_LEFT);
	CPUID_ROW_PUSH(0.8f);
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Brand"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_END;

	CPUID_ROW_BEGIN(6, 0.2f);
	nk_label(ctx, gnwinfo_get_text(L"Cores"), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Cores"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_PUSH(0.2f);
	nk_label(ctx, gnwinfo_get_text(L"Code Name"), NK_TEXT_LEFT);
	CPUID_ROW_PUSH(0.3f);
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Code Name"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_END;

	CPUID_ROW_BEGIN(6, 0.2f);
	nk_label(ctx, gnwinfo_get_text(L"Threads"), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Logical CPUs"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_PUSH(0.2f);
	nk_label(ctx, gnwinfo_get_text(L"Hypervisor"), NK_TEXT_LEFT);
	CPUID_ROW_PUSH(0.3f);
	nk_label_colored(ctx, gnwinfo_get_node_attr(g_ctx.cpuid, "Hypervisor"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_END;

	CPUID_ROW_BEGIN(6, 0.2f);
	nk_label(ctx, gnwinfo_get_text(L"Family"), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Family"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_PUSH(0.2f);
	nk_label(ctx, gnwinfo_get_text(L"Model"), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Model"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_PUSH(0.2f);
	nk_label(ctx, gnwinfo_get_text(L"Stepping"), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Stepping"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_END;

	CPUID_ROW_BEGIN(6, 0.2f);
	nk_label(ctx, gnwinfo_get_text(L"Ext.Family"), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Ext.Family"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_PUSH(0.2f);
	nk_label(ctx, gnwinfo_get_text(L"Ext.Model"), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Ext.Model"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_PUSH(0.2f);
	nk_label(ctx, gnwinfo_get_text(L"Aff.Mask"), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_label_colored(ctx, gnwinfo_get_node_attr(cpu, "Affinity Mask"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_END;

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_spacer(ctx);

	struct nk_rect rect = nk_layout_widget_bounds(ctx);
	CPUID_ROW_BEGIN_EX(3, 0.2f, NK_MAX(rect.h * 10, 100.0f));
	draw_features(ctx, cpu);
	CPUID_ROW_PUSH(0.4f);
	draw_cache(ctx, cpu);
	CPUID_ROW_PUSH(0.4f);
	draw_msr(ctx, cpu_index);
	CPUID_ROW_END;

out:
	nk_end(ctx);
}
