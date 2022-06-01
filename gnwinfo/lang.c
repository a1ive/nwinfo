// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

#pragma warning(disable:4566)

#define CHS_LANG 2052

#include "lang_chs.h"

LPSTR
GNW_GetText(LPCSTR lpEng)
{
	size_t i;
	LPCSTR ret = lpEng;
	if (GNWC.wLang == CHS_LANG)
	{
		for (i = 0; i < sizeof(GNW_LANG_CHS) / sizeof(GNW_LANG_CHS[0]); i++)
		{
			if (strcmp(lpEng, GNW_LANG_CHS[i].lpEng) == 0)
			{
				ret = GNW_LANG_CHS[i].lpDst;
				break;
			}
		}
	}
	return (LPSTR)ret;
}

static VOID
GNW_SetMenuItemText(UINT id)
{
	CHAR cText[MAX_PATH];
	MENUITEMINFOA mii = { 0 };
	mii.cbSize = sizeof(MENUITEMINFOA);
	mii.fMask = MIIM_STRING;
	if (GetMenuStringA(GNWC.hMenu, id, cText, MAX_PATH, MF_BYCOMMAND) == 0)
		return;
	mii.dwTypeData = GNW_GetText(cText);
	SetMenuItemInfoA(GNWC.hMenu, id, FALSE, &mii);
}

VOID
GNW_SetMenuText(VOID)
{
	ModifyMenuA(GNWC.hMenu,
		0, MF_BYPOSITION | MF_STRING | MF_POPUP,
		(UINT_PTR)GetSubMenu(GNWC.hMenu, 0), GNW_GetText("File"));
	ModifyMenuA(GNWC.hMenu,
		1, MF_BYPOSITION | MF_STRING | MF_POPUP,
		(UINT_PTR)GetSubMenu(GNWC.hMenu, 1), GNW_GetText("Help"));
	GNW_SetMenuItemText(IDM_RELOAD);
	GNW_SetMenuItemText(IDM_EXPORT);
	GNW_SetMenuItemText(IDM_EXIT);
	GNW_SetMenuItemText(IDM_HOMEPAGE);
	GNW_SetMenuItemText(IDM_ABOUT);
}
