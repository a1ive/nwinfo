// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "gettext.h"
#include "utils.h"
#include <pathcch.h>
#include <shellapi.h>
#include <setupapi.h>

extern const DEVPROPKEY DEVPKEY_DriverPackage_DriverInfName;
extern const DEVPROPKEY DEVPKEY_DriverPackage_OriginalInfName;
extern const DEVPROPKEY DEVPKEY_DriverPackage_ProviderName;
extern const DEVPROPKEY DEVPKEY_DriverPackage_ClassGuid;
extern const DEVPROPKEY DEVPKEY_DriverPackage_DriverDate;
extern const DEVPROPKEY DEVPKEY_DriverPackage_DriverVersion;
extern const DEVPROPKEY DEVPKEY_DriverPackage_ImportDate;
extern const DEVPROPKEY DEVPKEY_DeviceClass_ClassName;

PNODE_ATT
NWL_DrvStoreSetProperty(PNODE node, HDRVSTORE hDrvStore, DWORD objType, LPCWSTR objName, LPCSTR name, const DEVPROPKEY* propKey);

#define FLAG_DELETED 0x10000

static BOOL CALLBACK
DrvStoreEnumPackages(HDRVSTORE hDrvStore, LPCWSTR drvStorePath, PDRIVER_PACKAGE_INFO pkgInfo, LPARAM context)
{
	if (pkgInfo->Flags & DRIVER_PACKAGE_INBOX)
		return TRUE;

	PNODE root = (PNODE)context;
	PNODE node = NWL_NodeAlloc("Driver", NFLG_TABLE_ROW);
	PNODE_ATT attr;
	
	attr = NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Inf", &DEVPKEY_DriverPackage_OriginalInfName);
	if (attr == NULL)
		attr = NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Inf", &DEVPKEY_DriverPackage_DriverInfName);
	if (attr == NULL)
		attr = NWL_NodeAttrSet(node, "Inf", NWL_Ucs2ToUtf8(pkgInfo->PublishedInfName), 0);
	NWL_NodeAttrSet(node, "Published Inf", NWL_Ucs2ToUtf8(pkgInfo->PublishedInfName), 0);

	NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Provider", &DEVPKEY_DriverPackage_ProviderName);
	NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Version", &DEVPKEY_DriverPackage_DriverVersion);
	NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Date", &DEVPKEY_DriverPackage_DriverDate);

	LPCSTR group_name = "Unknown";

	attr = NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Class GUID", &DEVPKEY_DriverPackage_ClassGuid);
	if (attr)
	{
		group_name = attr->value;
		attr = NWL_DrvStoreSetProperty(node, hDrvStore, DeviceSetupClass, NWL_Utf8ToUcs2(attr->value), "Type", &DEVPKEY_DeviceClass_ClassName);
		if (attr)
			group_name = attr->value;
	}

	PNODE group = NWL_NodeGetChild(root, group_name);
	if (group == NULL)
		group = NWL_NodeAppendNew(root, group_name, NFLG_TABLE);
	NWL_NodeAppendChild(group, node);

	NWL_NodeAttrSet(node, "Path", NWL_Ucs2ToUtf8(drvStorePath), 0);

	WCHAR buf[MAX_PATH];
	wcsncpy_s(buf, MAX_PATH, drvStorePath, _TRUNCATE);
	PathCchRemoveFileSpec(buf, MAX_PATH);
	NWL_NodeAttrSet(node, "Folder", NWL_Ucs2ToUtf8(buf), 0);
	return TRUE;
}

static enum
{
	DRVSTORE_STATE_UPDATE = 0,
	DRVSTORE_STATE_UPDATE_PENDING,
	DRVSTORE_STATE_READY,
	DRV_STATE_NONE,
} m_drvstore_state;

static void
update_drvstore(struct nk_context* ctx, LPCSTR drive)
{
	m_drvstore_state = DRV_STATE_NONE;
	if (g_ctx.hdrvstore)
	{
		NWL_DriverStoreClose(g_ctx.hdrvstore);
		g_ctx.hdrvstore = NULL;
	}
	if (g_ctx.drvstore)
	{
		NWL_NodeFree(g_ctx.drvstore, 1);
		g_ctx.drvstore = NULL;
	}

	g_ctx.hdrvstore = NWL_DriverStoreOpen(drive, DRIVERSTORE_OPEN_NONE);
	if (!g_ctx.hdrvstore)
		return;
	g_ctx.drvstore = NWL_NodeAlloc("DrvStore", 0);
	if (!NWL_DriverStoreEnum(g_ctx.hdrvstore, DRIVERSTORE_ENUM_NONE, DrvStoreEnumPackages, (LPARAM)g_ctx.drvstore))
	{
		NWL_DriverStoreClose(g_ctx.hdrvstore);
		g_ctx.hdrvstore = NULL;
		NWL_NodeFree(g_ctx.drvstore, 1);
		g_ctx.drvstore = NULL;
		return;
	}

	m_drvstore_state = DRVSTORE_STATE_READY;
}

static inline BOOL
di_uninstall_driver(LPCWSTR inf, DWORD flags)
{
	HMODULE dll = LoadLibraryW(L"newdev.dll");
	if (!dll)
		return FALSE;
	BOOL result = FALSE;
	BOOL(WINAPI * func)(HWND, LPCWSTR, DWORD, PBOOL) = NULL;
	*(FARPROC*)&func = GetProcAddress(dll, "DiUninstallDriverW");
	if (func)
		result = func(g_ctx.wnd, inf, flags, NULL);
	FreeLibrary(dll);
	return result;
}

#ifndef DIURFLAG_NO_REMOVE_INF
#define DIURFLAG_NO_REMOVE_INF (0x00000001)
#endif

static BOOL
delete_driver(PNODE pkg)
{
	WCHAR inf[MAX_PATH];
	wcsncpy_s(inf, MAX_PATH, NWL_Utf8ToUcs2(NWL_NodeAttrGet(pkg, "Path")), _TRUNCATE);
	if (g_ctx.lib.DrvStoreDrive)
	{
		DWORD status = NWL_DriverStoreDelete(g_ctx.hdrvstore, inf, DRIVERSTORE_DELETE_UNCONFIGURE);
		return (status == ERROR_SUCCESS);
	}
	// Try DiUninstallDriverW (>= Win10 1703)
	di_uninstall_driver(inf, DIURFLAG_NO_REMOVE_INF);
	// Try SetupUninstallOEMInfW
	wcsncpy_s(inf, MAX_PATH, NWL_Utf8ToUcs2(NWL_NodeAttrGet(pkg, "Published Inf")), _TRUNCATE);
	return SetupUninstallOEMInfW(inf, SUOI_FORCEDELETE, NULL);
}

static void
draw_drvstore_menu(struct nk_context* ctx, PNODE pkg, float width, struct nk_rect bounds)
{
	if (!nk_hc_begin(ctx, width, g_col_height * 2.0f, bounds))
		return;
	nk_layout_row_dynamic(ctx, 0, 1);
	if (nk_contextual_item_label(ctx, N_(N__OPEN_FOLDER_LOCATION), NK_TEXT_LEFT))
	{
		ShellExecuteW(NULL, L"open", NWL_Utf8ToUcs2(NWL_NodeAttrGet(pkg, "Folder")), NULL, NULL, SW_NORMAL);
		nk_contextual_close(ctx);
	}
	if (nk_contextual_item_label(ctx, N_(N__DELETE), NK_TEXT_LEFT))
	{
		WCHAR inf[MAX_PATH];
		wcsncpy_s(inf, MAX_PATH, NWL_Utf8ToUcs2(NWL_NodeAttrGet(pkg, "Inf")), _TRUNCATE);
		if (MessageBoxExW(g_ctx.wnd, NWL_Utf8ToUcs2(N_(N__ARE_YOU_SURE)), inf, MB_ICONWARNING | MB_YESNO, 0) == IDYES)
		{
			if (delete_driver(pkg))
				pkg->flags |= FLAG_DELETED;
			else
				MessageBoxExW(g_ctx.wnd, NWL_Utf8ToUcs2(N_(N__FAILED)), inf, MB_ICONERROR | MB_OK, 0);
		}
		nk_contextual_close(ctx);
	}
	nk_contextual_end(ctx);
}

VOID
gnwinfo_draw_drvstore_window(struct nk_context* ctx, float width, float height)
{
	if (m_drvstore_state == DRVSTORE_STATE_UPDATE)
	{
		nk_layout_row_dynamic(ctx, 0, 1);
		nk_spacer(ctx);
		nk_spacer(ctx);
		nk_label(ctx, N_(N__LOADING), NK_TEXT_CENTERED);
		m_drvstore_state = DRVSTORE_STATE_UPDATE_PENDING;
		return;
	}
	else if (m_drvstore_state == DRVSTORE_STATE_UPDATE_PENDING)
		update_drvstore(ctx, g_ctx.lib.DrvStoreDrive);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 1.0f - g_ctx.gui_ratio, g_ctx.gui_ratio });
	nk_spacer(ctx);
	if (nk_button_image_hover(ctx, GET_PNG(IDR_PNG_REFRESH), N_(N__REFRESH)))
		m_drvstore_state = DRVSTORE_STATE_UPDATE;

	if (m_drvstore_state == DRV_STATE_NONE)
		return;

	INT group_count = NWL_NodeChildCount(g_ctx.drvstore);
	for (INT i = 0; i < group_count; i++)
	{
		PNODE group = NWL_NodeEnumChild(g_ctx.drvstore, i);
		if (group == NULL)
			return;
		if (nk_tree_push_id(ctx, NK_TREE_TAB, group->name, NK_MAXIMIZED, i))
		{
			INT pkg_count = NWL_NodeChildCount(group);
			for (INT j = 0; j < pkg_count; j++)
			{
				PNODE pkg = NWL_NodeEnumChild(group, j);
				if (pkg == NULL)
					return;
				if (pkg->flags & FLAG_DELETED)
					continue;
				nk_layout_row(ctx, NK_DYNAMIC, 0, 5, (float[5]) { 0.4f - g_ctx.gui_ratio, 0.3f, 0.2f, 0.1f, g_ctx.gui_ratio });
				nk_uint scroll_x = 0, scroll_y = 0;
				struct nk_rect bounds = nk_layout_widget_bounds(ctx);
				nk_window_get_scroll(ctx, &scroll_x, &scroll_y);
				bounds.x -= (float)scroll_x;
				bounds.y -= (float)scroll_y;
				struct nk_color text_color = nk_input_is_mouse_hovering_rect(&ctx->input, bounds) ? g_color_text_l : g_color_text_d;
				nk_lhc(ctx, NWL_NodeAttrGet(pkg, "Inf"), NK_TEXT_LEFT, text_color);
				nk_lhc(ctx, NWL_NodeAttrGet(pkg, "Provider"), NK_TEXT_LEFT, text_color);
				nk_lhc(ctx, NWL_NodeAttrGet(pkg, "Version"), NK_TEXT_LEFT, text_color);
				nk_lhc(ctx, NWL_NodeAttrGet(pkg, "Date"), NK_TEXT_LEFT, text_color);
				nk_spacer(ctx);
				draw_drvstore_menu(ctx, pkg, 0.3f * width, bounds);
			}
			nk_tree_pop(ctx);
		}
	}
}
