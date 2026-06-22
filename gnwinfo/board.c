// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "gettext.h"
#include "utils.h"

static VOID
draw_mainboard_view(struct nk_context* ctx)
{
	nk_layout_row_dynamic(ctx, 7 * g_col_height, 1);
	if (nk_group_begin_ex(ctx, N_(N__MAINBOARD), NK_WINDOW_BORDER | NK_WINDOW_TITLE, GET_PNG(IDR_PNG_PC)))
	{
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.2f, 0.8f });
		nk_l(ctx, N_(N__VENDOR), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(g_ctx.board, "Manufacturer"), NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__CHIPSET), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(g_ctx.board, "Chipset"), NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__NAME), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(g_ctx.board, "Board Name"), NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__VERSION), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(g_ctx.board, "Board Version"), NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__S_N), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(g_ctx.board, "Serial Number"), NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__UUID), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(g_ctx.board, "System UUID"), NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__LPCIO), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(g_ctx.board, "LPC0"), NK_TEXT_LEFT, g_color_text_l);
		nk_group_end(ctx);
	}
}

static int crt_tree_id = 0;

static void draw_crt_node(struct nk_context* ctx, PNODE db)
{
	if (db == NULL)
		return;
	if (nk_tree_image_push_id(ctx, NK_TREE_TAB, GET_PNG(IDR_PNG_UEFI), db->name, NK_MAXIMIZED, crt_tree_id++))
	{
		PNODE sigs = NWL_NodeGetChild(db, "Signatures");
		if (sigs == NULL)
			return;
		INT crt_count = NWL_NodeChildCount(sigs);
		for (INT i = 0; i < crt_count; i++)
		{
			PNODE crt = NWL_NodeEnumChild(sigs, i);
			if (crt == NULL)
				continue;
			LPCSTR crt_name = NWL_NodeAttrGet(crt, "Name");
			if (crt_name[0] == '-' && crt_name[1] == '\0')
				continue;
			nk_layout_row_dynamic(ctx, 0, 1);
			nk_image_label(ctx, GET_PNG(IDR_PNG_LOCK), crt_name, NK_TEXT_LEFT, g_color_text_d);
		}
		nk_tree_pop(ctx);
	}
}

static VOID
draw_uefi_view(struct nk_context* ctx)
{
	WORD group_img_id = IDR_PNG_UEFI;
	float group_height = 12 * g_col_height;
	LPCSTR fw_type = NWL_NodeAttrGet(g_ctx.uefi, "Firmware Type");
	if (fw_type[0] != 'U')
	{
		group_img_id = IDR_PNG_FIRMWARE;
		group_height = 2 * g_col_height;
	}
	nk_layout_row_dynamic(ctx, group_height, 1);
	if (nk_group_begin_ex(ctx, N_(N__BIOS), NK_WINDOW_BORDER | NK_WINDOW_TITLE, GET_PNG(group_img_id)))
	{
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.2f, 0.8f });
		nk_l(ctx, N_(N__VENDOR), NK_TEXT_LEFT);
		nk_lhc(ctx, NWL_NodeAttrGet(g_ctx.board, "BIOS Vendor"), NK_TEXT_LEFT, g_color_text_l);
		nk_l(ctx, N_(N__VERSION), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%s %s",
			NWL_NodeAttrGet(g_ctx.board, "BIOS Version"),
			NWL_NodeAttrGet(g_ctx.board, "BIOS Date"));
		
		PNODE sb_sig = NWL_NodeGetChild(g_ctx.uefi, "Secure Boot Signatures");
		if (sb_sig == NULL)
			goto end_group;
		crt_tree_id = 0;
		INT db_count = NWL_NodeChildCount(sb_sig);
		for (INT i = 0; i < db_count; i++)
		{
			PNODE db = NWL_NodeEnumChild(sb_sig, i);
			draw_crt_node(ctx, db);
		}
end_group:
		nk_group_end(ctx);
	}
}

static VOID
draw_tpm_view(struct nk_context* ctx)
{
	LPCSTR tpm_ver = NWL_NodeAttrGet(g_ctx.board, "TPM Version");
	if (tpm_ver[0] != 'v')
		return;
	nk_layout_row_dynamic(ctx, 8 * g_col_height, 1);
	if (nk_group_begin_ex(ctx, N_(N__TPM), NK_WINDOW_BORDER | NK_WINDOW_TITLE, GET_PNG(IDR_PNG_LOCK)))
	{
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.2f, 0.8f });
		nk_l(ctx, N_(N__VENDOR), NK_TEXT_LEFT);
		nk_lhcf(ctx, NK_TEXT_LEFT, g_color_text_l, "%s (%s, ID=%s, %s)",
			NWL_NodeAttrGet(g_ctx.board, "TPM Manufacturer"),
			NWL_NodeAttrGet(g_ctx.board, "TPM Vendor String"),
			NWL_NodeAttrGet(g_ctx.board, "TPM Manufacturer ID"),
			NWL_NodeAttrGet(g_ctx.board, "TPM Interface"));
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

static int dmi_tree_id = 0;

static void draw_dmi_node(struct nk_context* ctx, PNODE node)
{
	char name[48];
	if (!node || !node->attributes || node->name[0] != 'T')
		return;

	strncpy_s(name, sizeof(name), NWL_NodeAttrGet(node, "Description"), _TRUNCATE);
	if (name[0] == '-' && name[1] == '\0')
		snprintf(name, sizeof(name), "Type %s", NWL_NodeAttrGet(node, "Table Type"));

	if (nk_tree_push_id(ctx, NK_TREE_TAB, name, NK_MINIMIZED, dmi_tree_id++))
	{
		const float ratio[] = { 0.4f, 0.6f };
		INT count = NWL_NodeAttrCount(node);
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratio);
		for (INT i = 0; i < count; i++)
		{
			PNODE_ATT att = NWL_NodeAttrEnum(node, i);
			if (!att)
				continue;
			nk_l(ctx, att->key, NK_TEXT_LEFT);
			nk_lhc(ctx, att->value, NK_TEXT_RIGHT, g_color_text_l);
		}
		nk_tree_pop(ctx);
	}
}

static VOID
draw_dmi_view(struct nk_context* ctx)
{
	nk_layout_row_dynamic(ctx, 12 * g_col_height, 1);
	if (nk_group_begin_ex(ctx, "SMBIOS", NK_WINDOW_BORDER | NK_WINDOW_TITLE, GET_PNG(IDR_PNG_DMI)))
	{
		dmi_tree_id = 0;
		INT count = NWL_NodeChildCount(g_ctx.smbios);
		for (INT i = 0; i < count; i++)
		{
			draw_dmi_node(ctx, NWL_NodeEnumChild(g_ctx.smbios, i));
		}
		nk_group_end(ctx);
	}
}

VOID
gnwinfo_draw_board_window(struct nk_context* ctx, float width, float height)
{
	draw_mainboard_view(ctx);

	draw_uefi_view(ctx);

	draw_tpm_view(ctx);

	draw_dmi_view(ctx);
}
