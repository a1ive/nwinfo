// SPDX-License-Identifier: Unlicense

#pragma once

#include "targetver.h"
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>

#include "resource.h"

#include <libnw.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GNWINFO_HOMEPAGE "https://github.com/a1ive/nwinfo"
#define GNWINFO_TITLE    "NWinfo GUI"

typedef struct _GNW_CONTEXT
{
	HINSTANCE hInst;
	HWND hWnd;
	HANDLE hMutex;
	NWLIB_CONTEXT nCtx;

	LANGID wLang;
	HIMAGELIST hImageList;
	HMENU hMenu;

	HTREEITEM htRoot;
	PNODE pnRoot;
	HTREEITEM htAcpi;
	PNODE pnAcpi;
	HTREEITEM htBattery;
	PNODE pnBattery;
	HTREEITEM htCpuid;
	PNODE pnCpuid;
	HTREEITEM htDisk;
	PNODE pnDisk;
	HTREEITEM htEdid;
	PNODE pnEdid;
	HTREEITEM htNetwork;
	PNODE pnNetwork;
	HTREEITEM htPci;
	PNODE pnPci;
	HTREEITEM htSmbios;
	PNODE pnSmbios;
	HTREEITEM htSpd;
	PNODE pnSpd;
	HTREEITEM htSystem;
	PNODE pnSystem;
	HTREEITEM htUsb;
	PNODE pnUsb;
} GNW_CONTEXT;
extern GNW_CONTEXT GNWC;

typedef struct
{
	LPCSTR lpEng;
	LPCSTR lpDst;
} GNW_LANG;

VOID GNW_Init(HINSTANCE hInstance, INT nCmdShow, DLGPROC lpDialogFunc);
VOID GNW_Reload(VOID);
VOID GNW_Export(VOID);
VOID __declspec(noreturn) GNW_Exit(INT nExitCode);

PNODE GNW_LibInfo(VOID);

VOID GNW_TreeInit(VOID);
HTREEITEM GNW_TreeAdd(HTREEITEM hParent, LPCSTR lpszItem, INT nLevel, INT nIcon, LPVOID lpConfig);
VOID GNW_TreeExpand(HTREEITEM hTree);
VOID GNW_TreeDelete(HTREEITEM hTree);
INT_PTR GNW_TreeUpdate(HWND hWnd, WPARAM wParam, LPARAM lParam);

VOID GNW_ListInit(VOID);
VOID GNW_ListAdd(PNODE node, BOOL bSkipChild);
VOID GNW_ListClean(VOID);

INT GNW_IconFromAcpi(PNODE node, LPCSTR name);
INT GNW_IconFromDisk(PNODE node, LPCSTR name);
INT GNW_IconFromNetwork(PNODE node, LPCSTR name);
INT GNW_IconFromPci(PNODE node, LPCSTR name);
INT GNW_IconFromSmbios(PNODE node, LPCSTR name);
INT GNW_IconFromUsb(PNODE node, LPCSTR name);

LPSTR GNW_GetText(LPCSTR lpEng);
VOID GNW_SetMenuText(VOID);

#ifdef __cplusplus
} /* extern "C" */
#endif
