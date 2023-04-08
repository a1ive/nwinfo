// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

VOID GNW_ListInit(VOID)
{
	RECT rect = { 0 };
	LVCOLUMNA lvcName, lvcAttr, lvcValue;
	HWND hwndLV = GetDlgItem(GNWC.hWnd, IDC_MAIN_LIST);
	if (!hwndLV)
		return;
	if (GetClientRect(hwndLV, &rect) == FALSE)
		return;
	ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	lvcName.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lvcName.fmt = LVCFMT_LEFT;
	lvcName.cx = rect.right ? rect.right / 4 : 100;
	lvcName.pszText = GNW_GetText("Name");
	lvcName.cchTextMax = 0;
	lvcName.iSubItem = 0;
	ListView_InsertColumn(hwndLV, lvcName.iSubItem, &lvcName);

	lvcAttr.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lvcAttr.fmt = LVCFMT_LEFT;
	lvcAttr.cx = rect.right ? rect.right / 4 : 100;
	lvcAttr.pszText = GNW_GetText("Attribute");
	lvcAttr.cchTextMax = 0;
	lvcAttr.iSubItem = 1;
	ListView_InsertColumn(hwndLV, lvcAttr.iSubItem, &lvcAttr);

	lvcValue.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lvcValue.fmt = LVCFMT_LEFT;
	lvcValue.cx = rect.right ? rect.right - lvcName.cx - lvcAttr.cx : 384;
	lvcValue.pszText = GNW_GetText("Data");
	lvcValue.cchTextMax = 0;
	lvcValue.iSubItem = 2;
	ListView_InsertColumn(hwndLV, lvcValue.iSubItem, &lvcValue);
}

VOID GNW_ListAdd(PNODE node, BOOL bSkipChild)
{
	INT i, count, cur_lvi = 0;
	HWND hwndLV = GetDlgItem(GNWC.hWnd, IDC_MAIN_LIST);
	if (!hwndLV || !node)
		return;
	count = NWL_NodeAttrCount(node);
	for (i = 0; i < count; i++, cur_lvi++)
	{
		PNODE_ATT att = node->Attributes[i].LinkedAttribute;
		LVITEMA lvi;
		lvi.mask = LVIF_TEXT;
		lvi.cchTextMax = 0;
		lvi.iSubItem = 0;
		lvi.pszText = GNW_GetText(att->Key);
		lvi.iItem = cur_lvi;
		ListView_InsertItem(hwndLV, &lvi);
		ListView_SetItemText(hwndLV, cur_lvi, 2, (att->Value && att->Value[0] != '\0') ? att->Value : "-");
	}
	if (bSkipChild)
		return;
	count = NWL_NodeChildCount(node);
	for (i = 0; i < count; i++)
	{
		PNODE child = node->Children[i].LinkedNode;
		INT j, att_count, tab_count;
		att_count = NWL_NodeAttrCount(child);
		for (j = 0; j < att_count; j++, cur_lvi++)
		{
			PNODE_ATT att = child->Attributes[j].LinkedAttribute;
			LVITEMA lvi;
			lvi.mask = LVIF_TEXT;
			lvi.cchTextMax = 0;
			lvi.iSubItem = 0;
			lvi.pszText = GNW_GetText(child->Name);
			lvi.iItem = cur_lvi;
			ListView_InsertItem(hwndLV, &lvi);
			ListView_SetItemText(hwndLV, cur_lvi, 1, GNW_GetText(att->Key));
			ListView_SetItemText(hwndLV, cur_lvi, 2, (att->Value && att->Value[0] != '\0') ? att->Value : "-");
		}
		tab_count = NWL_NodeChildCount(child);
		for (j = 0; j < tab_count; j++)
		{
			PNODE row = child->Children[j].LinkedNode;
			INT k, row_count;
			row_count = NWL_NodeAttrCount(row);
			for (k = 0; k < row_count; k++, cur_lvi++)
			{
				PNODE_ATT att = row->Attributes[k].LinkedAttribute;
				LVITEMA lvi;
				lvi.mask = LVIF_TEXT;
				lvi.cchTextMax = 0;
				lvi.iSubItem = 0;
				lvi.pszText = GNW_GetText(row->Name);
				lvi.iItem = cur_lvi;
				ListView_InsertItem(hwndLV, &lvi);
				ListView_SetItemText(hwndLV, cur_lvi, 1, GNW_GetText(att->Key));
				ListView_SetItemText(hwndLV, cur_lvi, 2, (att->Value && att->Value[0] != '\0') ? att->Value : "-");
			}
		}
	}
}

VOID GNW_ListUpdate(VOID)
{
	HWND hwndTV = GetDlgItem(GNWC.hWnd, IDC_MAIN_TREE);
	if (!hwndTV || !GNWC.tvCurItem.lParam)
		return;
	BOOL bSkipChild = FALSE;
	HTREEITEM htParent = TreeView_GetParent(hwndTV, GNWC.tvCurItem.hItem);
	if (htParent == GNWC.htDisk)
		bSkipChild = TRUE;
	else if (htParent == GNWC.htCpuid || GNWC.tvCurItem.hItem == GNWC.htCpuid)
		bSkipChild = TRUE;
	else if (GNWC.tvCurItem.hItem == GNWC.htBattery)
		bSkipChild = TRUE;
	else if (GNWC.tvCurItem.hItem == GNWC.htRoot)
		bSkipChild = TRUE;
	GNW_ListAdd((PNODE)GNWC.tvCurItem.lParam, bSkipChild);
}

VOID GNW_ListClean(VOID)
{
	HWND hwndLV = GetDlgItem(GNWC.hWnd, IDC_MAIN_LIST);
	if (hwndLV)
		ListView_DeleteAllItems(hwndLV);
}
