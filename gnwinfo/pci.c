// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

static int tree_id = 0;

static void draw_pci_node(struct nk_context* ctx, PNODE node)
{
	int i;
	if (!node || !node->Attributes)
		return;
	if (nk_tree_image_push_id(ctx, NK_TREE_TAB,
		g_ctx.image_pci,
		gnwinfo_get_node_attr(node, "HWID"),
		NK_MINIMIZED, tree_id++))
	{
		const float ratio[] = { 0.2f, 0.8f };
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, ratio);
		for (i = 0; node->Attributes[i].LinkedAttribute; i++)
		{
			if (strcmp(node->Attributes[i].LinkedAttribute->Key, "HWID") == 0)
				continue;
			nk_label(ctx, node->Attributes[i].LinkedAttribute->Key, NK_TEXT_LEFT);
			nk_label_colored(ctx, node->Attributes[i].LinkedAttribute->Value, NK_TEXT_RIGHT, g_color_text_l);
		}
		nk_tree_pop(ctx);
	}
}

void draw_pci_class(struct nk_context* ctx, const char* title, struct nk_image image, const char* code)
{
	if (nk_tree_image_push_id(ctx, NK_TREE_TAB, image, title, NK_MINIMIZED, tree_id++))
	{
		PNODE_LINK pci;
		for (pci = &g_ctx.pci->Children[0]; pci->LinkedNode != NULL; pci++)
		{
			const char* cl = gnwinfo_get_node_attr(pci->LinkedNode, "Class Code");
			if (_strnicmp(cl, code, 2) != 0)
				continue;
			draw_pci_node(ctx, pci->LinkedNode);
		}
		nk_tree_pop(ctx);
	}
}

VOID gnwinfo_draw_pci_window(struct nk_context* ctx, float width, float height)
{
	if (!(g_ctx.window_flag & GUI_WINDOW_PCI))
		return;
	if (!nk_begin(ctx, "PCI",
		nk_rect(0, height / 4.0f, width * 0.98f, height / 2.0f),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE))
	{
		g_ctx.window_flag &= ~GUI_WINDOW_PCI;
		goto out;
	}
	tree_id = 0;
	draw_pci_class(ctx, "Unclassified device", g_ctx.image_info, "00");
	draw_pci_class(ctx, "Mass storage controller", g_ctx.image_disk, "01");
	draw_pci_class(ctx, "Network controller", g_ctx.image_net, "02");
	draw_pci_class(ctx, "Display controller", g_ctx.image_edid, "03");
	draw_pci_class(ctx, "Multimedia controller", g_ctx.image_mm, "04");
	draw_pci_class(ctx, "Memory controller", g_ctx.image_ram, "05");
	draw_pci_class(ctx, "Bridge", g_ctx.image_pci, "06");
	draw_pci_class(ctx, "Communication controller", g_ctx.image_bios, "07");
	draw_pci_class(ctx, "Generic system peripheral", g_ctx.image_board, "08");
	draw_pci_class(ctx, "Input device controller", g_ctx.image_pci, "09");
	draw_pci_class(ctx, "Docking station", g_ctx.image_pci, "0a");
	draw_pci_class(ctx, "Processor", g_ctx.image_cpu, "0b");
	draw_pci_class(ctx, "Serial bus controller", g_ctx.image_bios, "0c");
	draw_pci_class(ctx, "Wireless controller", g_ctx.image_net, "0d");
	draw_pci_class(ctx, "Intelligent controller", g_ctx.image_pci, "0e");
	draw_pci_class(ctx, "Satellite communications controller", g_ctx.image_net, "0f");
	draw_pci_class(ctx, "Encryption controller", g_ctx.image_bios, "10");
	if (nk_tree_image_push_id(ctx, NK_TREE_TAB, g_ctx.image_pci, "Other", NK_MINIMIZED, tree_id++))
	{
		PNODE_LINK pci;
		for (pci = &g_ctx.pci->Children[0]; pci->LinkedNode != NULL; pci++)
		{
			const char* cl = gnwinfo_get_node_attr(pci->LinkedNode, "Class Code");
			if (cl[0] == '0' ||
				(cl[0] == '1' && cl[1] == '0'))
				continue;
			draw_pci_node(ctx, pci->LinkedNode);
		}
		nk_tree_pop(ctx);
	}
out:
	nk_end(ctx);
}
