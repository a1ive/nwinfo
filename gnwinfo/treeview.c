// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

VOID
GNW_TreeInit(VOID)
{
	PNODE node;
	INT i, count;
	TreeView_SetImageList(GetDlgItem(GNWC.hWnd, IDC_MAIN_TREE), GNWC.hImageList, TVSIL_NORMAL);

	GNWC.htRoot = GNW_TreeAdd(NULL, "NWinfo", 1, IDI_ICON_TVD_PC, NULL);

	GNWC.htAcpi = GNW_TreeAdd(GNWC.htRoot, "ACPI Table", 2, IDI_ICON_TVN_ACPI, NULL);
	count = NWL_NodeChildCount(GNWC.pnAcpi);
	for (i = 0; i < count; i++)
	{
		INT icon = IDI_ICON_TVN_DMI;
		LPSTR name;
		node = GNWC.pnAcpi->Children[i].LinkedNode;
		name = NWL_NodeAttrGet(node, "Signature");
		if (!name)
			continue;
		if (_stricmp(name, "UEFI") == 0)
			icon = IDI_ICON_TVD_EFI;
		else if (_stricmp(name, "FACP") == 0)
			icon = IDI_ICON_TVN_ACPI;
		else if (_stricmp(name, "XSDT") == 0)
			icon = IDI_ICON_TVN_ACPI;
		else if (_stricmp(name, "APIC") == 0)
			icon = IDI_ICON_TVD_FW;
		else if (_stricmp(name, "MCFG") == 0)
			icon = IDI_ICON_TVN_SPD;
		else if (_stricmp(name, "DSDT") == 0)
			icon = IDI_ICON_TVD_BAT;
		else if (_stricmp(name, "TPM2") == 0)
			icon = IDI_ICON_TVD_ENC;
		else if (_stricmp(name, "WSMT") == 0)
			icon = IDI_ICON_TVD_ENC;
		else if (_stricmp(name, "BGRT") == 0)
			icon = IDI_ICON_TVN_EDID;
		else if (_stricmp(name, "MSDM") == 0)
			icon = IDI_ICON_TVN_SYS;
		GNW_TreeAdd(GNWC.htAcpi, name, 3, icon, node);
	}

	GNWC.htCpuid = GNW_TreeAdd(GNWC.htRoot, "Processor", 2, IDI_ICON_TVN_CPU, GNWC.pnCpuid);
	count = NWL_NodeChildCount(GNWC.pnCpuid);
	for (i = 0; i < count; i++)
	{
		node = GNWC.pnCpuid->Children[i].LinkedNode;
		GNW_TreeAdd(GNWC.htCpuid, node->Name, 3, IDI_ICON_TVN_CPU, node);
	}

	GNWC.htDisk = GNW_TreeAdd(GNWC.htRoot, "Physical Storage", 2, IDI_ICON_TVN_DISK, NULL);
	count = NWL_NodeChildCount(GNWC.pnDisk);
	for (i = 0; i < count; i++)
	{
		HTREEITEM disk;
		LPSTR path, rm;
		INT icon = IDI_ICON_TVD_HDD;
		node = GNWC.pnDisk->Children[i].LinkedNode;
		path = NWL_NodeAttrGet(node, "Path");
		if (!path)
			continue;
		rm = NWL_NodeAttrGet(node, "Removable");
		if (rm && _stricmp(rm, "Yes") == 0)
			icon = IDI_ICON_TVD_RMD;
		disk = GNW_TreeAdd(GNWC.htDisk, path, 3, icon, node);
		if (node->Children[0].LinkedNode)
		{
			PNODE tab;
			INT j, tab_count;
			LPSTR letter;
			tab_count = NWL_NodeChildCount(node->Children[0].LinkedNode);
			for (j = 0; j < tab_count; j++)
			{
				tab = node->Children[0].LinkedNode->Children[j].LinkedNode;
				letter = NWL_NodeAttrGet(tab, "Drive Letter");
				if (!letter)
					continue;
				GNW_TreeAdd(disk, letter, 4, icon, tab);
			}
		}
	}

	GNWC.htEdid = GNW_TreeAdd(GNWC.htRoot, "Display", 2, IDI_ICON_TVN_EDID, NULL);
	count = NWL_NodeChildCount(GNWC.pnEdid);
	for (i = 0; i < count; i++)
	{
		LPSTR hwid;
		node = GNWC.pnEdid->Children[i].LinkedNode;
		hwid = NWL_NodeAttrGet(node, "ID");
		if (!hwid)
			continue;
		GNW_TreeAdd(GNWC.htEdid, hwid, 3, IDI_ICON_TVN_EDID, node);
	}

	GNWC.htNetwork = GNW_TreeAdd(GNWC.htRoot, "Network", 2, IDI_ICON_TVN_NET, NULL);
	count = NWL_NodeChildCount(GNWC.pnNetwork);
	for (i = 0; i < count; i++)
	{
		INT icon = IDI_ICON_TVN_NET;
		LPSTR name, type;
		node = GNWC.pnNetwork->Children[i].LinkedNode;
		name = NWL_NodeAttrGet(node, "Description");
		if (!name)
			name = NWL_NodeAttrGet(node, "Network Adapter");
		type = NWL_NodeAttrGet(node, "Type");
		if (!type)
			icon = IDI_ICON_TVD_HLP;
		else if (_stricmp(type, "Ethernet") == 0)
			icon = IDI_ICON_TVD_NE;
		else if (_stricmp(type, "IEEE 802.11 Wireless") == 0)
			icon = IDI_ICON_TVD_NW;
		else if (_stricmp(type, "Tunnel") == 0)
			icon = IDI_ICON_TVD_ENC;
		GNW_TreeAdd(GNWC.htNetwork, name, 3, icon, node);
	}

	GNWC.htPci = GNW_TreeAdd(GNWC.htRoot, "PCI Devices", 2, IDI_ICON_TVN_PCI, NULL);
	count = NWL_NodeChildCount(GNWC.pnPci);
	for (i = 0; i < count; i++)
	{
		INT icon = IDI_ICON_TVN_PCI;
		LPSTR hwid, hwclass;
		node = GNWC.pnPci->Children[i].LinkedNode;
		hwid = NWL_NodeAttrGet(node, "HWID");
		hwclass = NWL_NodeAttrGet(node, "Class");
		if (!hwid)
			continue;
		if (!hwclass || _stricmp(hwclass, "Unclassified device") == 0)
			icon = IDI_ICON_TVD_HLP;
		else if (_stricmp(hwclass, "Mass storage controller") == 0)
			icon = IDI_ICON_TVD_HDD;
		else if (_stricmp(hwclass, "Network controller") == 0)
			icon = IDI_ICON_TVD_NE;
		else if (_stricmp(hwclass, "Display controller") == 0)
			icon = IDI_ICON_TVN_EDID;
		else if (_stricmp(hwclass, "Multimedia controller") == 0)
			icon = IDI_ICON_TVD_MM;
		else if (_stricmp(hwclass, "Memory controller") == 0)
			icon = IDI_ICON_TVN_SPD;
		else if (_stricmp(hwclass, "Bridge") == 0)
			icon = IDI_ICON_TVD_FW;
		else if (_stricmp(hwclass, "Processor") == 0)
			icon = IDI_ICON_TVN_CPU;
		else if (_stricmp(hwclass, "Wireless controller") == 0)
			icon = IDI_ICON_TVD_NW;
		else if (_stricmp(hwclass, "Encryption controller") == 0)
			icon = IDI_ICON_TVD_ENC;

		GNW_TreeAdd(GNWC.htPci, hwid, 3, icon, node);
	}

	node = NULL;
	count = NWL_NodeChildCount(GNWC.pnSmbios);
	for (i = 0; i < count; i++)
	{
		node = GNWC.pnSmbios->Children[i].LinkedNode;
		if (NWL_NodeAttrGet(node, "SMBIOS Version"))
			break;
	}
	GNWC.htSmbios = GNW_TreeAdd(GNWC.htRoot, "SMBIOS", 2, IDI_ICON_TVN_DMI, node);
	for (i = 0; i < count; i++)
	{
		INT icon = IDI_ICON_TVN_DMI;
		LPSTR name, type;
		CHAR tmp[] = "Type 1024";
		node = GNWC.pnSmbios->Children[i].LinkedNode;
		type = NWL_NodeAttrGet(node, "Table Type");
		if (!type)
			continue;
		name = NWL_NodeAttrGet(node, "Description");
		if (!name)
		{
			snprintf(tmp, sizeof(tmp), "Type %s", type);
			name = tmp;
		}
		if (_stricmp(type, "0") == 0)
			icon = IDI_ICON_TVD_EFI;
		else if (_stricmp(type, "1") == 0)
			icon = IDI_ICON_TVD_PC;
		else if (_stricmp(type, "2") == 0)
			icon = IDI_ICON_TVD_FW;
		else if (_stricmp(type, "3") == 0)
			icon = IDI_ICON_TVD_PC;
		else if (_stricmp(type, "4") == 0)
			icon = IDI_ICON_TVN_CPU;
		else if (_stricmp(type, "5") == 0)
			icon = IDI_ICON_TVN_SPD;
		else if (_stricmp(type, "6") == 0)
			icon = IDI_ICON_TVN_SPD;
		else if (_stricmp(type, "7") == 0)
			icon = IDI_ICON_TVN_CPU;
		else if (_stricmp(type, "11") == 0)
			icon = IDI_ICON_TVD_HLP;
		else if (_stricmp(type, "12") == 0)
			icon = IDI_ICON_TVD_HLP;
		else if (_stricmp(type, "13") == 0)
			icon = IDI_ICON_TVD_HLP;
		else if (_stricmp(type, "16") == 0)
			icon = IDI_ICON_TVN_SPD;
		else if (_stricmp(type, "17") == 0)
			icon = IDI_ICON_TVN_SPD;
		else if (_stricmp(type, "19") == 0)
			icon = IDI_ICON_TVN_SPD;
		else if (_stricmp(type, "20") == 0)
			icon = IDI_ICON_TVN_SPD;
		else if (_stricmp(type, "22") == 0)
			icon = IDI_ICON_TVD_BAT;
		else if (_stricmp(type, "43") == 0)
			icon = IDI_ICON_TVD_ENC;

		GNW_TreeAdd(GNWC.htSmbios, name, 3, icon, node);
	}

	GNWC.htSpd = GNW_TreeAdd(GNWC.htRoot, "Memory SPD", 2, IDI_ICON_TVN_SPD, NULL);

	GNWC.htSystem = GNW_TreeAdd(GNWC.htRoot, "Operating System", 2, IDI_ICON_TVN_SYS, GNWC.pnSystem);

	GNWC.htUsb = GNW_TreeAdd(GNWC.htRoot, "USB Devices", 2, IDI_ICON_TVN_USB, NULL);
	count = NWL_NodeChildCount(GNWC.pnUsb);
	for (i = 0; i < count; i++)
	{
		LPSTR hwid;
		node = GNWC.pnUsb->Children[i].LinkedNode;
		hwid = NWL_NodeAttrGet(node, "HWID");
		if (!hwid)
			continue;
		GNW_TreeAdd(GNWC.htUsb, hwid, 3, IDI_ICON_TVD_USB, node);
	}
}

HTREEITEM
GNW_TreeAdd(HTREEITEM hParent, LPCSTR lpszItem, INT nLevel, INT nIcon, LPVOID lpConfig)
{
	TVITEMA tvi;
	TVINSERTSTRUCTA tvins;
	static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
	HTREEITEM hti;
	HWND hwndTV = GetDlgItem(GNWC.hWnd, IDC_MAIN_TREE);

	if (!hwndTV || (nLevel != 1 && !hParent) || !lpszItem)
		return NULL;
	tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvi.iImage = (nIcon >= IDI_ICON) ? (nIcon - IDI_ICON) : 0;
	tvi.iSelectedImage = tvi.iImage;
	tvi.pszText = (LPSTR)lpszItem;
	tvi.cchTextMax = (int)(strlen(lpszItem) + 1);
	tvi.lParam = (LPARAM)lpConfig;
	tvins.item = tvi;
	tvins.hInsertAfter = hPrev;
	if (nLevel == 1)
		tvins.hParent = TVI_ROOT;
	else
		tvins.hParent = hParent;
	hPrev = (HTREEITEM)SendMessageA(hwndTV, TVM_INSERTITEMA,
		0, (LPARAM)(LPTVINSERTSTRUCTA)&tvins);

	if (hPrev == NULL)
		return NULL;

	if (nLevel > 1)
	{
		hti = TreeView_GetParent(hwndTV, hPrev);
		tvi.mask = 0;
		tvi.hItem = hti;
		TreeView_SetItem(hwndTV, &tvi);
	}

	return hPrev;
}

VOID GNW_TreeExpand(HTREEITEM hTree)
{
	HWND hwndTV = GetDlgItem(GNWC.hWnd, IDC_MAIN_TREE);
	if (hwndTV)
		TreeView_Expand(hwndTV, hTree, TVE_EXPAND);
}

VOID GNW_TreeDelete(HTREEITEM hTree)
{
	HWND hwndTV = GetDlgItem(GNWC.hWnd, IDC_MAIN_TREE);
	if (hwndTV)
		TreeView_DeleteItem(hwndTV, hTree);
}

INT_PTR
GNW_TreeUpdate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	LPNMTREEVIEWA pnmtv = (LPNMTREEVIEWA)lParam;
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(hWnd);
	HWND hwndTV = GetDlgItem(hWnd, IDC_MAIN_TREE);
	if (!hwndTV || pnmtv->hdr.code != (UINT)TVN_SELCHANGINGA)
		return (INT_PTR)FALSE;
	GNW_ListClean();
	if (pnmtv->itemNew.hItem == GNWC.htSpd && GNWC.pnSpd == NULL)
	{
		PNODE node;
		INT i, count;
		GNWC.nCtx.SpdInfo = TRUE;
		GNWC.pnSpd = NW_Spd();
		count = NWL_NodeChildCount(GNWC.pnSpd);
		for (i = 0; i < count; i++)
		{
			LPSTR mt;
			node = GNWC.pnSpd->Children[i].LinkedNode;
			mt = NWL_NodeAttrGet(node, "Memory Type");
			if (!mt)
				continue;
			GNW_TreeAdd(GNWC.htSpd, mt, 3, IDI_ICON_TVN_SPD, node);
		}
		GNW_TreeExpand(GNWC.htSpd);
	}
	if (pnmtv->itemNew.lParam)
	{
		BOOL bSkipChild = FALSE;
		HTREEITEM htParent = TreeView_GetParent(hwndTV, pnmtv->itemNew.hItem);
		if (htParent == GNWC.htDisk)
			bSkipChild = TRUE;
		else if (pnmtv->itemNew.hItem == GNWC.htCpuid)
			bSkipChild = TRUE;
		GNW_ListAdd((PNODE)pnmtv->itemNew.lParam, bSkipChild);
	}
	return (INT_PTR)TRUE;
}
