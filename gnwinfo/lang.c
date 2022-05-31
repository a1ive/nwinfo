// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

#pragma warning(disable:4566)

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

	{"Cache", "缓存"},
	{"Features", "特性"},
	{"SGX", "SGX"},

	{"Status", "状态"},
	{"ID", "ID"},
	{"HWID", "硬件 ID"},
	{"HW Name", "硬件名称"},
	{"Device", "设备"},
	{"Type", "类型"},
	{"Class", "种类"},
	{"Subclass", "子类"},
	{"Signature", "签名"},
	{"Length", "长度"},
	{"Checksum", "校验码"},
	{"Checksum Status", "校验码状态"},
	{"Description", "描述"},
	{"Date", "日期"},
	{"Version", "版本"},
	{"Revision", "修订版"},
	{"Vendor", "供应商"},
	{"Manufacturer", "制造商"},
	{"Serial", "序列号"},
	{"Serial Number", "序列号"},
	{"Path", "路径"},
	{"Size", "大小"},
	{"Capacity", "容量"},
	{"Drive Letter", "盘符"},
	{"Filesystem", "文件系统"},
	{"Label", "卷标"},
	{"Free Space", "可用空间"},
	{"Total Space", "总空间"},
	{"Partition Table", "分区表"},
	{"Removable", "可移动"},
	{"Table Type", "表类型"},
	{"Table Length", "表长度"},
	{"Table Handle", "表句柄"},
	{"Voltage", "电压"},
	{"Prog IF", "编程接口"},
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
