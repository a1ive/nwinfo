// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

static int tree_id = 0;

static void draw_dmi_node(struct nk_context* ctx, PNODE node)
{
	int i;
	char name[48];
	if (!node || !node->Attributes || node->Name[0] != 'T')
		return;

	snprintf(name, sizeof(name), "%s", NWL_NodeAttrGet(node, "Description"));
	if (name[0] == '-' && name[1] == '\0')
		snprintf(name, sizeof(name), "Type %s", NWL_NodeAttrGet(node, "Table Type"));

	if (nk_tree_push_id(ctx, NK_TREE_TAB, name, NK_MINIMIZED, tree_id++))
	{
		const float ratio[] = { 0.4f, 0.6f };
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratio);
		for (i = 0; node->Attributes[i].LinkedAttribute; i++)
		{
			nk_l(ctx, node->Attributes[i].LinkedAttribute->Key, NK_TEXT_LEFT);
			nk_lhc(ctx, node->Attributes[i].LinkedAttribute->Value, NK_TEXT_RIGHT, g_color_text_l);
		}
		nk_tree_pop(ctx);
	}
}

VOID gnwinfo_draw_dmi_window(struct nk_context* ctx, float width, float height)
{
	PNODE_LINK dmi;
	if (!(g_ctx.window_flag & GUI_WINDOW_DMI))
		return;
	if (!nk_begin(ctx, "SMBIOS",
		nk_rect(0, height / 4.0f, width * 0.98f, height / 2.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE))
	{
		g_ctx.window_flag &= ~GUI_WINDOW_DMI;
		goto out;
	}
	tree_id = 0;

	for (dmi = &g_ctx.smbios->Children[0]; dmi->LinkedNode != NULL; dmi++)
	{
		draw_dmi_node(ctx, dmi->LinkedNode);
	}

out:
	nk_end(ctx);
}
