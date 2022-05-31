// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

#define CHS_LANG 2052

typedef struct
{
	LPSTR lpEng;
	LPSTR lpDst;
} GNW_LANG;

static GNW_LANG GNW_LANG_CHS[] =
{
	{"Loading, please wait ...", "加载中，请稍候 ..."},
	{"File", "文件"},
	{"Refresh", "刷新"},
	{"Exit", "退出"},
	{"Help", "帮助"},
	{"About", "关于"},

	{"Name", "名称"},
	{"Attribute", "属性"},
	{"Data", "数据"},

	{"ACPI Table", "ACPI 表"},
	{"Processor", "处理器"},
	{"Physical Storage", "磁盘"},
	{"Display Devices", "显示设备"},
	{"Network Adapter", "网络适配器"},
	{"PCI Devices", "PCI 设备"},
	{"SMBIOS", "SMBIOS"},
	{"Memory SPD", "内存 SPD"},
	{"Operating System", "操作系统"},
	{"USB Devices", "USB 设备"},
};

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
	GNW_SetMenuItemText(IDM_EXIT);
	GNW_SetMenuItemText(IDM_ABOUT);
}
