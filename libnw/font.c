// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "libnw.h"
#include "utils.h"

static int CALLBACK
EnumFontFamCallBack(ENUMLOGFONTW* lpelf, TEXTMETRICW* lptm, DWORD dwFontType, PNODE node)
{
	PNODE pFont = NWL_NodeAppendNew(node, "Font", NFLG_TABLE_ROW);
	NWL_NodeAttrSet(pFont, "Name", NWL_Ucs2ToUtf8(lpelf->elfFullName), 0);
	NWL_NodeAttrSet(pFont, "Style", NWL_Ucs2ToUtf8(lpelf->elfStyle), 0);

	NWL_NodeAttrSetBool(pFont, "TrueType Font", (dwFontType & TRUETYPE_FONTTYPE), 0);
	NWL_NodeAttrSetBool(pFont, "Raster Font", (dwFontType & RASTER_FONTTYPE), 0);
	NWL_NodeAttrSetBool(pFont, "Device Font", (dwFontType & DEVICE_FONTTYPE), 0);

	NWL_NodeAttrSetf(pFont, "Height", NAFLG_FMT_NUMERIC, "%ld", lptm->tmHeight);
	NWL_NodeAttrSetf(pFont, "Ascent", NAFLG_FMT_NUMERIC, "%ld", lptm->tmAscent);
	NWL_NodeAttrSetf(pFont, "Descent", NAFLG_FMT_NUMERIC, "%ld", lptm->tmDescent);
	NWL_NodeAttrSetf(pFont, "MaxCharWidth", NAFLG_FMT_NUMERIC, "%ld", lptm->tmMaxCharWidth);
	NWL_NodeAttrSetf(pFont, "Weight", NAFLG_FMT_NUMERIC, "%ld", lptm->tmWeight);
	NWL_NodeAttrSetBool(pFont, "Italic Font", lptm->tmItalic, 0);
	NWL_NodeAttrSetBool(pFont, "Underlined Font", lptm->tmUnderlined, 0);
	NWL_NodeAttrSetBool(pFont, "StruckOut Font", lptm->tmStruckOut, 0);

	return 1;
}

PNODE NW_Font(VOID)
{
	PNODE node = NWL_NodeAlloc("Fonts", NFLG_TABLE);
	if (NWLC->FontInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	HDC hDC = GetDC(NULL);
	EnumFontFamiliesW(hDC, NULL, (FONTENUMPROCW)EnumFontFamCallBack, (LPARAM)node);
	ReleaseDC(NULL, hDC);

	return node;
}
