// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

static int tree_id = 0;

static void draw_dmi_node(struct nk_context* ctx, PNODE node)
{
	int i;
	char name[48];
	if (!node || !node->attributes || node->name[0] != 'T')
		return;

	snprintf(name, sizeof(name), "%s", NWL_NodeAttrGet(node, "Description"));
	if (name[0] == '-' && name[1] == '\0')
		snprintf(name, sizeof(name), "Type %s", NWL_NodeAttrGet(node, "Table Type"));

	if (nk_tree_push_id(ctx, NK_TREE_TAB, name, NK_MINIMIZED, tree_id++))
	{
		const float ratio[] = { 0.4f, 0.6f };
		INT count = NWL_NodeAttrCount(node);
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratio);
		for (i = 0; i < count; i++)
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

VOID gnwinfo_draw_dmi_window(struct nk_context* ctx, float width, float height)
{
	if (!(g_ctx.window_flag & GUI_WINDOW_DMI))
		return;
	if (!nk_begin_ex(ctx, "SMBIOS",
		nk_rect(0, height / 4.0f, width * 0.98f, height / 2.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE,
		GET_PNG(IDR_PNG_DMI), GET_PNG(IDR_PNG_CLOSE)))
	{
		g_ctx.window_flag &= ~GUI_WINDOW_DMI;
		goto out;
	}
	tree_id = 0;

	INT count = NWL_NodeChildCount(g_ctx.smbios);
	for (INT i = 0; i < count; i++)
	{
		draw_dmi_node(ctx, NWL_NodeEnumChild(g_ctx.smbios, i));
	}

out:
	nk_end(ctx);
}
