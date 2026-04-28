// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <shellapi.h>
#include "gnwinfo.h"
#include "ioctl.h"
#include "gettext.h"
#include "utils.h"
#include "version.h"

void gnwinfo_add_systray(HWND wnd, HICON icon, LPCWSTR desc)
{
	NOTIFYICONDATAW nid = { 0 };
	nid.cbSize = sizeof(NOTIFYICONDATAW);
	nid.hWnd = wnd;
	nid.uID = 1; // Unique ID for the icon
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_TRAYICON;
	nid.hIcon = icon;
	wcscpy_s(nid.szTip, ARRAYSIZE(nid.szTip), desc);

	Shell_NotifyIconW(NIM_ADD, &nid);
}

void gnwinfo_remove_systray(HWND wnd)
{
	NOTIFYICONDATAW nid = { 0 };
	nid.cbSize = sizeof(NOTIFYICONDATAW);
	nid.hWnd = wnd;
	nid.uID = 1;
	Shell_NotifyIconW(NIM_DELETE, &nid);
}

#if 0
void gnwinfo_update_systray(HWND wnd, HICON icon, LPCWSTR desc)
{
	NOTIFYICONDATAW nid = { 0 };
	nid.cbSize = sizeof(NOTIFYICONDATAW);
	nid.hWnd = wnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_TIP;
	nid.hIcon = icon;
	wcscpy_s(nid.szTip, ARRAYSIZE(nid.szTip), desc);
	Shell_NotifyIconW(NIM_MODIFY, &nid);
}
#endif

#define MAX_POWER_SCHEMES 64

static inline void
show_power_schemes_menu(HMENU menu)
{
	PNODE table = NWL_NodeGetChild(g_ctx.battery, "Power Schemes");
	if (!table)
		return;
	INT count = NWL_NodeChildCount(table);
	if (count <= 0)
		return;
	if (count > MAX_POWER_SCHEMES)
		count = MAX_POWER_SCHEMES;
	for (UINT i = 0; i < (UINT)count; i++)
	{
		PNODE scheme = NWL_NodeEnumChild(table, i);
		if (!scheme)
			continue;
		LPCSTR name = NWL_NodeAttrGet(scheme, "Name");
		UINT id = IDM_POWER_SCHEME_BASE + i;
		UINT flags = MF_STRING;
		LPCSTR active = NWL_NodeAttrGet(scheme, "Active");
		if (strcmp(active, NA_BOOL_TRUE) == 0)
			flags |= MF_CHECKED;
		AppendMenuW(menu, flags, id, NWL_Utf8ToUcs2(name));
	}
}

void gnwinfo_show_systray_menu(HWND wnd)
{
	POINT pt;
	GetCursorPos(&pt);

	HMENU menu = CreatePopupMenu();
	if (menu)
	{
		if (g_ctx.lib.NwDrv != NULL && g_ctx.lib.NwDrv->type == WR0_DRIVER_PAWNIO)
		{
			UINT flags = MF_STRING;
			if (g_ctx.lib.NwDrv->installed == FALSE)
				flags |= MF_DISABLED | MF_CHECKED;
			AppendMenuW(menu, flags, IDM_INSTALL_PAWNIO, NWL_Utf8ToUcs2(N_(N__INSTALL_PAWNIO)));
			AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
		}

		AppendMenuW(menu, MF_STRING, IDM_CLEAN_MEM, NWL_Utf8ToUcs2(N_(N__CLEAN_MEMORY)));

		AppendMenuW(menu, MF_SEPARATOR, 0, NULL);

		HMENU power_menu = CreatePopupMenu();
		if (power_menu)
		{
			show_power_schemes_menu(power_menu);
			AppendMenuW(menu, MF_POPUP, (UINT_PTR)power_menu, NWL_Utf8ToUcs2(N_(N__POWER_OPTIONS)));
		}

		AppendMenuW(menu, MF_SEPARATOR, 0, NULL);

		AppendMenuW(menu, MF_STRING, IDM_EXIT, NWL_Utf8ToUcs2(N_(N__CLOSE)));

		SetForegroundWindow(wnd);
		TrackPopupMenuEx(menu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, wnd, NULL);
		DestroyMenu(menu);
	}
}

void gnwinfo_handle_systray_cmd(HWND wnd, WORD wmid)
{
	switch (wmid)
	{
	case IDM_INSTALL_PAWNIO:
		if (g_ctx.lib.NwDrv)
			g_ctx.lib.NwDrv->installed = FALSE;
		return;
	case IDM_CLEAN_MEM:
		gnwinfo_clean_memory();
		return;
	case IDM_EXIT:
		InterlockedExchange(&g_ctx.exit_pending, 1);
		return;
	}

	if (wmid >= IDM_POWER_SCHEME_BASE && wmid < IDM_POWER_SCHEME_BASE + MAX_POWER_SCHEMES)
	{
		DWORD (WINAPI *set_active_scheme)(HKEY, const GUID *) = NULL;
		INT index = wmid - IDM_POWER_SCHEME_BASE;
		PNODE table = NWL_NodeGetChild(g_ctx.battery, "Power Schemes");
		if (!table)
			return;
		PNODE scheme = NWL_NodeEnumChild(table, index);
		if (!scheme)
			return;
		LPCSTR attr = NWL_NodeAttrGet(scheme, "GUID");
		GUID guid = { 0 };
		if (!NWL_StrToGuid(attr, &guid))
			return;
		HMODULE dll = LoadLibraryW(L"powrprof.dll");
		if (!dll)
			return;
		*(FARPROC*)&set_active_scheme = GetProcAddress(dll, "PowerSetActiveScheme");
		if (set_active_scheme)
			set_active_scheme(NULL, &guid);
		FreeLibrary(dll);
		PostMessageW(wnd, WM_TIMER, (WPARAM)IDT_TIMER_1M, 0);
	}
}
