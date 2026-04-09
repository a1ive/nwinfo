// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

static void draw_pci_node(struct nk_context* ctx, PNODE node, nk_bool visible, int* id)
{
	if (!node || !node->attributes)
		return;
	(*id)++;
	if (!visible)
		return;
	if (nk_tree_image_push_id(ctx, NK_TREE_TAB,
		GET_PNG(IDR_PNG_PCI),
		NWL_NodeAttrGet(node, "HWID"),
		NK_MINIMIZED, *id))
	{
		const float ratio[] = { 0.2f, 0.8f };
		INT count = NWL_NodeAttrCount(node);
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratio);
		for (INT i = 0; i < count; i++)
		{
			PNODE_ATT att = NWL_NodeAttrEnum(node, i);
			if (!att || strcmp(att->key, "HWID") == 0)
				continue;
			nk_l(ctx, att->key, NK_TEXT_LEFT);
			nk_lhc(ctx, att->value, NK_TEXT_RIGHT, g_color_text_l);
		}
		nk_tree_pop(ctx);
	}
}

static void draw_pci_class(struct nk_context* ctx, const char* title, struct nk_image image, const char* code, int* id)
{
	(*id)++;
	nk_bool expanded = nk_false;
	if (nk_tree_image_push_id(ctx, NK_TREE_TAB, image, title, NK_MINIMIZED, *id))
		expanded = nk_true;

	INT count = NWL_NodeChildCount(g_ctx.pci);
	for (INT i = 0; i < count; i++)
	{
		PNODE pci = NWL_NodeEnumChild(g_ctx.pci, i);
		const char* cl = NWL_NodeAttrGet(pci, "Class Code");
		if (_strnicmp(cl, code, 2) != 0)
			continue;
		draw_pci_node(ctx, pci, expanded, id);
	}

	if (expanded)
		nk_tree_pop(ctx);
}

VOID gnwinfo_draw_pci_window(struct nk_context* ctx, float width, float height)
{
	if (!(g_ctx.window_flag & GUI_WINDOW_PCI))
		return;
	if (!nk_begin_ex(ctx, "PCI",
		nk_rect(0, height / 4.0f, width * 0.98f, height / 2.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE,
		GET_PNG(IDR_PNG_PCI), GET_PNG(IDR_PNG_CLOSE)))
	{
		g_ctx.window_flag &= ~GUI_WINDOW_PCI;
		goto out;
	}
	int id = 0;
	draw_pci_class(ctx, "Unclassified device", GET_PNG(IDR_PNG_INFO), "00", &id);
	draw_pci_class(ctx, "Mass storage controller", GET_PNG(IDR_PNG_DISK), "01", &id);
	draw_pci_class(ctx, "Network controller", GET_PNG(IDR_PNG_ETH), "02", &id);
	draw_pci_class(ctx, "Display controller", GET_PNG(IDR_PNG_DISPLAY), "03", &id);
	draw_pci_class(ctx, "Multimedia controller", GET_PNG(IDR_PNG_MM), "04", &id);
	draw_pci_class(ctx, "Memory controller", GET_PNG(IDR_PNG_MEMORY), "05", &id);
	draw_pci_class(ctx, "Bridge", GET_PNG(IDR_PNG_PCI), "06", &id);
	draw_pci_class(ctx, "Communication controller", GET_PNG(IDR_PNG_FIRMWARE), "07", &id);
	draw_pci_class(ctx, "Generic system peripheral", GET_PNG(IDR_PNG_PC), "08", &id);
	draw_pci_class(ctx, "Input device controller", GET_PNG(IDR_PNG_PCI), "09", &id);
	draw_pci_class(ctx, "Docking station", GET_PNG(IDR_PNG_PCI), "0a", &id);
	draw_pci_class(ctx, "Processor", GET_PNG(IDR_PNG_CPU), "0b", &id);
	draw_pci_class(ctx, "Serial bus controller", GET_PNG(IDR_PNG_FIRMWARE), "0c", &id);
	draw_pci_class(ctx, "Wireless controller", GET_PNG(IDR_PNG_WLAN), "0d", &id);
	draw_pci_class(ctx, "Intelligent controller", GET_PNG(IDR_PNG_PCI), "0e", &id);
	draw_pci_class(ctx, "Satellite communications controller", GET_PNG(IDR_PNG_WLAN), "0f", &id);
	draw_pci_class(ctx, "Encryption controller", GET_PNG(IDR_PNG_FIRMWARE), "10", &id);
	id++;
	nk_bool other_expanded = nk_false;
	if (nk_tree_image_push_id(ctx, NK_TREE_TAB, GET_PNG(IDR_PNG_PCI), "Other", NK_MINIMIZED, id))
		other_expanded = nk_true;

	INT other_count = NWL_NodeChildCount(g_ctx.pci);
	for (INT i = 0; i < other_count; i++)
	{
		PNODE pci = NWL_NodeEnumChild(g_ctx.pci, i);
		const char* cl = NWL_NodeAttrGet(pci, "Class Code");
		if (cl[0] == '0' || (cl[0] == '1' && cl[1] == '0'))
			continue;
		draw_pci_node(ctx, pci, other_expanded, &id);
	}

	if (other_expanded)
		nk_tree_pop(ctx);
out:
	nk_end(ctx);
}
