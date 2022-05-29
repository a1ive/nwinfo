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

	lvcName.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lvcName.fmt = LVCFMT_LEFT;
	lvcName.cx = rect.right ? rect.right / 4 : 100;
	lvcName.pszText = "Name";
	lvcName.cchTextMax = 0;
	lvcName.iSubItem = 0;
	ListView_InsertColumn(hwndLV, lvcName.iSubItem, &lvcName);

	lvcAttr.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lvcAttr.fmt = LVCFMT_LEFT;
	lvcAttr.cx = rect.right ? rect.right / 4 : 100;
	lvcAttr.pszText = "Attribute";
	lvcAttr.cchTextMax = 0;
	lvcAttr.iSubItem = 1;
	ListView_InsertColumn(hwndLV, lvcAttr.iSubItem, &lvcAttr);

	lvcValue.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lvcValue.fmt = LVCFMT_LEFT;
	lvcValue.cx = rect.right ? rect.right - lvcName.cx - lvcAttr.cx : 384;
	lvcValue.pszText = "Value";
	lvcValue.cchTextMax = 0;
	lvcValue.iSubItem = 2;
	ListView_InsertColumn(hwndLV, lvcValue.iSubItem, &lvcValue);
}

static int cur_lvi = 0;

VOID GNW_ListAdd(PNODE node, BOOL bSkipChild)
{
	INT i, count;
	HWND hwndLV = GetDlgItem(GNWC.hWnd, IDC_MAIN_LIST);
	if (!hwndLV || !node)
		return;
	count = NWL_NodeAttrCount(node);
	for (i = 0; i < count; i++)
	{
		PNODE_ATT att = node->Attributes[i].LinkedAttribute;
		LVITEMA lvi;
		lvi.mask = LVIF_TEXT;
		lvi.cchTextMax = 0;
		lvi.iSubItem = 0;
		lvi.pszText = att->Key;
		ListView_InsertItem(hwndLV, &lvi);
		ListView_SetItemText(hwndLV, cur_lvi, 2, (att->Value && att->Value[0] != '\0') ? att->Value : "-");
		cur_lvi++;
	}
	if (bSkipChild)
		return;
	count = NWL_NodeChildCount(node);
	for (i = 0; i < count; i++)
	{
		PNODE child = node->Children[i].LinkedNode;
		INT j, att_count, tab_count;
		att_count = NWL_NodeAttrCount(child);
		for (j = 0; j < att_count; j++)
		{
			PNODE_ATT att = child->Attributes[j].LinkedAttribute;
			LVITEMA lvi;
			lvi.mask = LVIF_TEXT;
			lvi.cchTextMax = 0;
			lvi.iSubItem = 0;
			lvi.pszText = child->Name;
			ListView_InsertItem(hwndLV, &lvi);
			ListView_SetItemText(hwndLV, cur_lvi, 1, att->Key);
			ListView_SetItemText(hwndLV, cur_lvi, 2, (att->Value && att->Value[0] != '\0') ? att->Value : "-");
			cur_lvi++;
		}
		tab_count = NWL_NodeChildCount(child);
		for (j = 0; j < tab_count; j++)
		{
			PNODE row = child->Children[j].LinkedNode;
			INT k, row_count;
			row_count = NWL_NodeAttrCount(row);
			for (k = 0; k < row_count; k++)
			{
				PNODE_ATT att = row->Attributes[k].LinkedAttribute;
				LVITEMA lvi;
				lvi.mask = LVIF_TEXT;
				lvi.cchTextMax = 0;
				lvi.iSubItem = 0;
				lvi.pszText = row->Name;
				ListView_InsertItem(hwndLV, &lvi);
				ListView_SetItemText(hwndLV, cur_lvi, 1, att->Key);
				ListView_SetItemText(hwndLV, cur_lvi, 2, (att->Value && att->Value[0] != '\0') ? att->Value : "-");
				cur_lvi++;
			}
		}
	}
}

VOID GNW_ListClean(VOID)
{
	HWND hwndLV = GetDlgItem(GNWC.hWnd, IDC_MAIN_LIST);
	cur_lvi = 0;
	if (hwndLV)
		ListView_DeleteAllItems(hwndLV);
}
