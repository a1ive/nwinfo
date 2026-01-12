// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "gettext.h"

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
	LPCSTR feature = NWL_NodeAttrGet(cpu, "Features");
	if (nk_group_begin(ctx, N_(N__FEATURES), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		nk_layout_row_dynamic(ctx, 0, 1);
		for (c = feature; *c != '\0'; c += strlen(c) + 1)
			nk_lhc(ctx, c, NK_TEXT_CENTERED, g_color_text_l);
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
	if (nk_group_begin(ctx, N_(N__CACHE), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		const float ratio[] = { 0.2f, 0.8f };
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratio);
		nk_l(ctx, N_(N__L1_D), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(cache, "L1 D"), NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__L1_I), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(cache, "L1 I"), NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__L2), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(cache, "L2"), NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__L3), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(cache, "L3"), NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__L4), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(cache, "L4"), NK_TEXT_LEFT, g_color_text_l);
		nk_group_end(ctx);
	}
}

static void
draw_msr(struct nk_context* ctx, int index, PNODE cpu)
{
	if (nk_group_begin(ctx, N_(N__MSR), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		const float ratio[] = { 0.5f, 0.5f };
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratio);

		nk_l(ctx, N_(N__SOCKET), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Socket Type"), NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__MULTIPLIER), NK_TEXT_LEFT);
		nk_lhc(ctx, g_ctx.cpu_info[index].MsrMulti, NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__BASE_CLOCK), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%s MHz", NWL_NodeAttrGet(cpu, "Base Clock (MHz)"));
		nk_l(ctx, N_(N__CORE_SPEED), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%.2f MHz", g_ctx.cpu_info[index].MsrFreq);
		nk_l(ctx, N_(N__BUS_CLOCK), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%.2f MHz", g_ctx.cpu_info[index].MsrBus);
		nk_l(ctx, N_(N__TEMPERATURE), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, u8"%d \u2103", g_ctx.cpu_info[index].MsrTemp);
		nk_l(ctx, N_(N__VOLTAGE), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%.2f V", g_ctx.cpu_info[index].MsrVolt);
		nk_l(ctx, N_(N__CPU_POWER), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%.2f W", g_ctx.cpu_info[index].MsrPower);
		nk_l(ctx, N_(N__PL1), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%.2f W", g_ctx.cpu_info[index].MsrPl1);
		nk_l(ctx, N_(N__PL2), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%.2f W", g_ctx.cpu_info[index].MsrPl2);
#ifdef ENABLE_IGPU_MONITOR
		nk_l(ctx, N_(N__IGPU_TEMPERATURE), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, u8"%d \u2103", g_ctx.cpu_info[index].GpuTemp);
		nk_l(ctx, N_(N__IGPU_POWER), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%.2f W", g_ctx.cpu_info[index].GpuPower);
#endif
		nk_l(ctx, N_(N__MICROCODE), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "0x%X", g_ctx.cpu_info[index].BiosRev);
		nk_group_end(ctx);
	}
}

VOID
gnwinfo_draw_cpuid_window(struct nk_context* ctx, float width, float height)
{
	CHAR buf[MAX_PATH];
	PNODE cpu = NULL;
	static int cpu_index = 0;
	if (!(g_ctx.window_flag & GUI_WINDOW_CPUID))
		return;
	if (!nk_begin_ex(ctx, "CPUID",
		nk_rect(0, height / 4.0f, width * 0.98f, height / 2.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE,
		GET_PNG(IDR_PNG_CPU), GET_PNG(IDR_PNG_CLOSE)))
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
	snprintf(buf, MAX_PATH, "%s %d, %s %s, %lu MHz",
		N_(N__TOTAL), g_ctx.cpu_count,
		NWL_NodeAttrGet(g_ctx.cpuid, "Total CPUs"), N_(N__THREADS),
		g_ctx.cpu_freq);
	nk_l(ctx, buf, NK_TEXT_CENTERED);
	CPUID_ROW_END;

	cpu = NWL_NodeEnumChild(g_ctx.cpuid, cpu_index);
	if (cpu == NULL)
		goto out;

	CPUID_ROW_BEGIN(2, 0.2f);
	nk_l(ctx, N_(N__BRAND), NK_TEXT_LEFT);
	CPUID_ROW_PUSH(0.8f);
	nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Brand"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_END;

	CPUID_ROW_BEGIN(6, 0.2f);
	nk_l(ctx, N_(N__CORES_UPPER), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Cores"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_PUSH(0.2f);
	nk_l(ctx, N_(N__CODE_NAME), NK_TEXT_LEFT);
	CPUID_ROW_PUSH(0.3f);
	nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Code Name"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_END;

	CPUID_ROW_BEGIN(6, 0.2f);
	nk_l(ctx, N_(N__THREADS_UPPER), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Logical CPUs"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_PUSH(0.2f);
	nk_l(ctx, N_(N__HYPERVISOR), NK_TEXT_LEFT);
	CPUID_ROW_PUSH(0.3f);
	nk_lhc(ctx, NWL_NodeAttrGet(g_ctx.cpuid, "Hypervisor"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_END;

	CPUID_ROW_BEGIN(6, 0.2f);
	nk_l(ctx, N_(N__FAMILY), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Family"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_PUSH(0.2f);
	nk_l(ctx, N_(N__MODEL), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Model"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_PUSH(0.2f);
	nk_l(ctx, N_(N__STEPPING), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Stepping"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_END;

	CPUID_ROW_BEGIN(6, 0.2f);
	nk_l(ctx, N_(N__EXT_FAMILY), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Ext.Family"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_PUSH(0.2f);
	nk_l(ctx, N_(N__EXT_MODEL), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Ext.Model"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_PUSH(0.2f);
	nk_l(ctx, N_(N__AFF_MASK), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Affinity Mask"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_END;

	CPUID_ROW_BEGIN(6, 0.2f);
	nk_l(ctx, N_(N__TECHNOLOGY), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Technology"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_END;

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_spacer(ctx);

	struct nk_rect rect = nk_layout_widget_bounds(ctx);
	CPUID_ROW_BEGIN_EX(3, 0.2f, NK_MAX(rect.h * 10, 100.0f));
	draw_features(ctx, cpu);
	CPUID_ROW_PUSH(0.4f);
	draw_cache(ctx, cpu);
	CPUID_ROW_PUSH(0.4f);
	draw_msr(ctx, cpu_index, cpu);
	CPUID_ROW_END;

out:
	nk_end(ctx);
}
