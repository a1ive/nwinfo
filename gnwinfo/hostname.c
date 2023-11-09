// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winioctl.h>
#include "gnwinfo.h"
#include "utils.h"

static struct
{
	char hostname[MAX_COMPUTERNAME_LENGTH + 1];
	float col_height;
	char prefix[MAX_COMPUTERNAME_LENGTH + 1];
} m_ctx;

static nk_bool
nk_filter_hostname(const struct nk_text_edit* box, nk_rune unicode)
{
	NK_UNUSED(box);
	// The standard character set includes letters, numbers,
	// and the following symbols: ! @ # $ % ^ & ' ) ( . - _ { } ~
	if (unicode >= 'A' && unicode <= 'Z')
		return nk_true;
	if (unicode >= 'a' && unicode <= 'z')
		return nk_true;
	if (unicode >= '0' && unicode <= '9')
		return nk_true;
	if (unicode == '!' || unicode == '@' || unicode == '#'
		|| unicode == '$' || unicode == '%' || unicode == '^'
		|| unicode == '&' || unicode == '\'' || unicode == ')'
		|| unicode == '(' || unicode == '.' || unicode == '-'
		|| unicode == '_' || unicode == '{' || unicode == '}'
		|| unicode == '~')
		return nk_true;
	return nk_false;
}

VOID
gnwinfo_init_hostname_window(struct nk_context* ctx)
{
	time_t t;
	g_ctx.window_flag |= GUI_WINDOW_HOSTNAME;
	memcpy(m_ctx.hostname, g_ctx.sys_hostname, MAX_COMPUTERNAME_LENGTH + 1);
	m_ctx.col_height = 1.5f * g_font_size + 2.0f * ctx->style.edit.padding.y;
	strcpy_s(m_ctx.prefix, MAX_COMPUTERNAME_LENGTH + 1, gnwinfo_get_ini_value(L"Widgets", L"HostnamePrefix", L"DESKTOP-"));
	srand((unsigned)time(&t));
}

static void
gen_hostname(void)
{
	char tmp[8];
	const char* list = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	int i;
	for (i = 0; i < 8; i++)
		tmp[i] = list[rand() % 36];
	tmp[7] = '\0';
	snprintf(m_ctx.hostname, MAX_COMPUTERNAME_LENGTH + 1, "%s%s", m_ctx.prefix, tmp);
}

static void
exec_cmd(LPCWSTR cmd)
{
	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi;
	BOOL rc = FALSE;
	WCHAR desktop[] = L"WinSta0\\Default";
	WCHAR buf[MAX_PATH];

	si.cb = sizeof(STARTUPINFOW);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.lpDesktop = desktop;
	wcscpy_s(buf, MAX_PATH, cmd);

	rc = CreateProcessW(NULL, buf, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	if (rc)
	{
		SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}

static void
set_hostname(const char* text)
{
	LPCWSTR hostname = NWL_Utf8ToUcs2(text);
	DWORD len = (DWORD)((wcslen(hostname) + 1) * sizeof(WCHAR));

	if (len <= sizeof(WCHAR))
		return;

	NWL_NtSetRegValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName", L"ComputerName", hostname, len, REG_SZ);
	NWL_NtSetRegValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName", L"ComputerName", hostname, len, REG_SZ);

	NWL_NtSetRegValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", L"Hostname", hostname, len, REG_SZ);
	NWL_NtSetRegValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", L"NV Hostname", hostname, len, REG_SZ);

	NWL_NtSetRegValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\State", L"LastComputerName", hostname, len, REG_SZ);

	NWL_NtSetRegValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", L"AltDefaultDomainName", hostname, len, REG_SZ);
	NWL_NtSetRegValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", L"DefaultDomainName", hostname, len, REG_SZ);

	memcpy(g_ctx.sys_hostname, m_ctx.hostname, MAX_COMPUTERNAME_LENGTH + 1);

	// restart Windows Connection Manager Service (WcmSvc)
	exec_cmd(L"net stop WlanSvc");
	exec_cmd(L"net stop WcmSvc");
	exec_cmd(L"net start WcmSvc");
	exec_cmd(L"net start WlanSvc");
}

VOID
gnwinfo_draw_hostname_window(struct nk_context* ctx, float width, float height)
{
	if (!(g_ctx.window_flag & GUI_WINDOW_HOSTNAME))
		return;
	if (!nk_begin(ctx, gnwinfo_get_text(L"Hostname"),
		nk_rect(width / 8.0f, height / 4.0f, width * 0.75f, NK_MIN(height / 2.0f, 6 * m_ctx.col_height)),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE))
	{
		g_ctx.window_flag &= ~GUI_WINDOW_HOSTNAME;
		goto out;
	}

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_spacer(ctx);

	nk_layout_row(ctx, NK_DYNAMIC, m_ctx.col_height, 3, (float[3]) { 0.1f, 0.8f, 0.1f });
	nk_spacer(ctx);
	nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, m_ctx.hostname, MAX_COMPUTERNAME_LENGTH + 1, nk_filter_hostname);
	nk_spacer(ctx);

	nk_layout_row_dynamic(ctx, 0, 1);
	nk_spacer(ctx);

	nk_layout_row(ctx, NK_DYNAMIC, m_ctx.col_height, 5, (float[5]) { 0.15f, 0.3f, 0.1f, 0.3f, 0.15f });
	nk_spacer(ctx);
	if (nk_button_label(ctx, gnwinfo_get_text(L"Random")))
		gen_hostname();
	nk_spacer(ctx);
	if (nk_button_label(ctx, gnwinfo_get_text(L"OK")))
	{
		set_hostname(m_ctx.hostname);
		g_ctx.window_flag &= ~GUI_WINDOW_HOSTNAME;
	}
	nk_spacer(ctx);

out:
	nk_end(ctx);
}
