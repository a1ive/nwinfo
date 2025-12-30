// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include "gnwinfo.h"
#include "gettext.h"
#include "utils.h"

static void
draw_node(struct nk_context* ctx, int level, int* id, PNODE node)
{
	if (node == NULL)
		return;
	(*id)++;
	if (nk_tree_image_push_id(ctx, NK_TREE_TAB, GET_PNG(IDR_PNG_SENSOR), node->name, NK_MAXIMIZED, *id))
	{
		int count = NWL_NodeAttrCount(node);
		for (int i = 0; i < count; i++)
		{
			PNODE_ATT att = NWL_NodeAttrEnum(node, i);
			if (!att)
				continue;
			nk_layout_row_dynamic(ctx, 0, 2);
			nk_l(ctx, att->key, NK_TEXT_LEFT);
			nk_lhc(ctx, att->value, NK_TEXT_RIGHT, g_color_text_l);
		}
		count = NWL_NodeChildCount(node);
		for (int i = 0; i < count; i++)
		{
			draw_node(ctx, level + 1, id, NWL_NodeEnumChild(node, i));
		}
		nk_tree_pop(ctx);
	}
}

VOID
gnwinfo_draw_sensor_window(struct nk_context* ctx, float width, float height)
{
	int id = 0;
	int count = NWL_NodeChildCount(g_ctx.sensors);
	for (int i = 0; i < count; i++)
	{
		draw_node(ctx, 0, &id, NWL_NodeEnumChild(g_ctx.sensors, i));
	}
}
