// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "gettext.h"
#include "utils.h"

#define CPUID_ROW_BEGIN_EX(row,ratio,h) \
	nk_layout_row_begin(ctx, NK_DYNAMIC, h, row); \
	nk_layout_row_push(ctx, ratio);

#define CPUID_ROW_BEGIN(row,ratio) \
	CPUID_ROW_BEGIN_EX(row,ratio,0)

#define CPUID_ROW_PUSH(ratio) \
	nk_layout_row_push(ctx, ratio);

#define CPUID_ROW_END \
	nk_layout_row_end(ctx);

VOID
gnwinfo_draw_cpuid_window(struct nk_context* ctx, float width, float height)
{
	CHAR buf[MAX_PATH];
	PNODE cpu = NULL;
	static int cpu_index = 0;

	(void)width;
	if (g_ctx.cpu_count <= 0)
		return;

	cpu = NWL_NodeEnumChild(g_ctx.cpuid, cpu_index);
	if (cpu == NULL)
		return;
	NWLIB_CPU_INFO empty = { 0 };
	NWLIB_CPU_INFO* msr = &empty;
	if (g_ctx.cpu_info)
		msr = &g_ctx.cpu_info[cpu_index];

	nk_layout_row(ctx, NK_DYNAMIC, g_col_height, 2, (float[2]) { 0.2f, 0.8f });

	struct nk_vec2 size;
	size.y = 8 * (g_col_height + ctx->style.window.spacing.y);
	size.y += ctx->style.window.spacing.y * 2 + ctx->style.window.combo_padding.y * 2;
	size.x = nk_widget_width(ctx);

	if (nk_combo_begin_text(ctx, cpu->name, nk_strlen(cpu->name), size))
	{
		nk_layout_row_dynamic(ctx, g_col_height, 1);
		for (int i = 0; i < g_ctx.cpu_count; i++)
		{
			PNODE c = NWL_NodeEnumChild(g_ctx.cpuid, i);
			if (c == NULL)
				continue;
			if (nk_combo_item_label(ctx, c->name, NK_TEXT_LEFT))
				cpu_index = i;
		}
		nk_combo_end(ctx);
	}

	snprintf(buf, MAX_PATH, "%s %d, %s %s, %lu MHz",
		N_(N__TOTAL), g_ctx.cpu_count,
		NWL_NodeAttrGet(g_ctx.cpuid, "Total CPUs"), N_(N__THREADS),
		g_ctx.cpu_freq);
	nk_l(ctx, buf, NK_TEXT_CENTERED);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.2f, 0.8f });
	nk_l(ctx, N_(N__BRAND), NK_TEXT_LEFT);
	nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Brand"), NK_TEXT_CENTERED, g_color_text_l);

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
	nk_l(ctx, N_(N__TECHNOLOGY), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Technology"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_PUSH(0.2f);
	nk_l(ctx, N_(N__SOCKET), NK_TEXT_LEFT);
	CPUID_ROW_PUSH(0.3f);
	nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Socket Type"), NK_TEXT_LEFT, g_color_text_l);
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
	nk_l(ctx, N_(N__TDP), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%s W", NWL_NodeAttrGet(cpu, "TDP (W)"));
	CPUID_ROW_PUSH(0.2f);
	nk_l(ctx, N_(N__PL1), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%s W", NWL_NodeAttrGet(cpu, "PL1 (W)"));
	CPUID_ROW_PUSH(0.2f);
	nk_l(ctx, N_(N__PL2), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%s W", NWL_NodeAttrGet(cpu, "PL2 (W)"));
	CPUID_ROW_END;

	CPUID_ROW_BEGIN(6, 0.2f);
	nk_l(ctx, N_(N__BASE_CLOCK), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%s MHz", NWL_NodeAttrGet(cpu, "Base Clock (MHz)"));
	CPUID_ROW_PUSH(0.2f);
	nk_l(ctx, N_(N__BUS_CLOCK), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%.2f MHz", msr->MsrBus);
	CPUID_ROW_PUSH(0.2f);
	nk_l(ctx, N_(N__MICROCODE), NK_TEXT_LEFT);
	CPUID_ROW_PUSH((1.0f / 3 - 0.2f));
	nk_lhc(ctx, NWL_NodeAttrGet(cpu, "Microcode Rev"), NK_TEXT_LEFT, g_color_text_l);
	CPUID_ROW_END;

	nk_layout_row_dynamic(ctx, 6 * g_col_height, 1);
	LPCSTR feature = NWL_NodeAttrGet(cpu, "Features");
	if (nk_group_begin_ex(ctx, N_(N__FEATURES), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		nk_layout_row_dynamic(ctx, 0, 4);
		for (LPCSTR c = feature; *c != '\0'; c += strlen(c) + 1)
			nk_lhc(ctx, c, NK_TEXT_CENTERED, g_color_text_l);
		nk_group_end(ctx);
	}

	nk_layout_row(ctx, NK_DYNAMIC, 6 * g_col_height, 2, (float[2]) { 0.5f, 0.5f });

	PNODE cache = NWL_NodeGetChild(cpu, "Cache");
	if (nk_group_begin_ex(ctx, N_(N__CACHE), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.2f, 0.8f });
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

	if (nk_group_begin_ex(ctx, N_(N__MSR), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.5f, 0.5f });
		nk_l(ctx, N_(N__MULTIPLIER), NK_TEXT_LEFT);
		nk_lhc(ctx, msr->MsrMulti, NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__CORE_SPEED), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%.2f MHz", msr->MsrFreq);
		nk_l(ctx, N_(N__TEMPERATURE), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, u8"%.0f %s", NWL_GetTemperature((float)msr->MsrTemp), g_ctx.temp_unit);
		nk_l(ctx, N_(N__VOLTAGE), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%.2f V", msr->MsrVolt);
		nk_l(ctx, N_(N__CPU_POWER), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%.2f W", msr->MsrPower);
		
		nk_group_end(ctx);
	}

	nk_layout_row_dynamic(ctx, 6 * g_col_height, 1);
	if (nk_group_begin_ex(ctx, N_(N__MAINBOARD), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.2f, 0.8f });
		nk_l(ctx, N_(N__VENDOR), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(g_ctx.board, "Manufacturer"), NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__CHIPSET), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(g_ctx.board, "Chipset"), NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__NAME), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%s %s",
			NWL_NodeAttrGet(g_ctx.board, "Board Name"), NWL_NodeAttrGet(g_ctx.board, "Board Version"));
		nk_l(ctx, N_(N__UUID), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(g_ctx.board, "System UUID"), NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__LPCIO), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(g_ctx.board, "LPC0"), NK_TEXT_LEFT, g_color_text_l);
		nk_group_end(ctx);
	}

	LPCSTR tpm_ver = NWL_NodeAttrGet(g_ctx.board, "TPM Version");
	if (tpm_ver[0] != 'v')
		return;
	nk_layout_row_dynamic(ctx, 8 * g_col_height, 1);
	if (nk_group_begin_ex(ctx, N_(N__TPM), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.2f, 0.8f });
		nk_l(ctx, N_(N__VENDOR), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%s (%s, ID=%s)",
			NWL_NodeAttrGet(g_ctx.board, "TPM Manufacturer"),
			NWL_NodeAttrGet(g_ctx.board, "TPM Vendor String"),
			NWL_NodeAttrGet(g_ctx.board, "TPM Manufacturer ID"));
		nk_l(ctx, N_(N__VERSION), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%s r%s %s fw%s", tpm_ver,
			NWL_NodeAttrGet(g_ctx.board, "TPM Spec Revision"),
			NWL_NodeAttrGet(g_ctx.board, "TPM Spec Date"),
			NWL_NodeAttrGet(g_ctx.board, "TPM Firmware Version"));

		nk_layout_row_dynamic(ctx, 0, 1);
		nk_l(ctx, N_(N__ALGORITHMS), NK_TEXT_LEFT);
		nk_layout_row_dynamic(ctx, 0, 4);
		LPCSTR algorithms = NWL_NodeAttrGet(g_ctx.board, "TPM Algorithms");
		for (LPCSTR c = algorithms; *c != '\0'; c += strlen(c) + 1)
			nk_lhc(ctx, c, NK_TEXT_CENTERED, g_color_text_l);
		nk_group_end(ctx);
	}
}
