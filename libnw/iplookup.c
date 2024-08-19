// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wininet.h>

#include "libnw.h"
#include "utils.h"

static HINTERNET (WINAPI *OsInetOpen) (LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
static HINTERNET (WINAPI *OsInetOpenUrl) (HINTERNET, LPCWSTR, LPCWSTR, DWORD, DWORD, DWORD_PTR);
static BOOL (WINAPI *OsInetReadFile) (HINTERNET, LPVOID, DWORD, LPDWORD);
static BOOL (WINAPI *OsInetCloseHandle) (HINTERNET);

static LPSTR
GetYamlField(LPCSTR lpszBuffer, LPCSTR lpszField)
{
	LPSTR p, q;
	LPSTR str = _strdup(lpszBuffer);
	if (!str)
		return NULL;
	for (p = str; p && *p; p = strchr(p, '\n'))
	{
		p++;
		size_t len = strlen(lpszField);
		if (strncmp(p, lpszField, len) == 0 && p[len] == ':')
		{
			for (p = p + len + 1; *p == ' '; p++)
				;
			if (*p == '\'')
				p++;
			q = strchr(p, '\n');
			if (!q)
				break;
			if (*(q - 1) == '\'')
				q--;
			*q = '\0';
			memmove(str, p, q - p + 1);
			return str;
		}
	}
	free(str);
	return NULL;
}

static DWORD
GetUrlData(LPCWSTR lpszUrl, void* lpBuffer, DWORD dwSize)
{
	HINTERNET net = NULL, file = NULL;
	DWORD size = 0;
	if (dwSize <= 1)
		goto fail;
	ZeroMemory(lpBuffer, dwSize);
	net = OsInetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!net)
		goto fail;
	file = OsInetOpenUrl(net, lpszUrl, NULL, 0, INTERNET_FLAG_RELOAD, 0);
	if (!file)
		goto fail;
	OsInetReadFile(file, lpBuffer, dwSize - 1, &size);
fail:
	if (file)
		OsInetCloseHandle(file);
	if (net)
		OsInetCloseHandle(net);
	return size;
}

static void PrintIpAddress(PNODE node)
{
	if (GetUrlData(L"https://api.ipify.org", NWLC->NwBuf, NWINFO_BUFSZ))
		NWL_NodeAttrSet(node, "IPv4", NWLC->NwBuf, 0);
	if (GetUrlData(L"https://api64.ipify.org", NWLC->NwBuf, NWINFO_BUFSZ))
		NWL_NodeAttrSet(node, "IPv6", NWLC->NwBuf, 0);
}

static void PrintGeoInfo(PNODE node)
{
	if (!GetUrlData(L"https://ipapi.co/yaml", NWLC->NwBuf, NWINFO_BUFSZ))
		return;

	LPSTR lpCountry = GetYamlField(NWLC->NwBuf, "country_name");
	LPSTR lpRegion = GetYamlField(NWLC->NwBuf, "region");
	LPSTR lpCity = GetYamlField(NWLC->NwBuf, "city");
	NWL_NodeAttrSetf(node, "City", 0, "%s, %s, %s", lpCity, lpRegion, lpCountry);
	free(lpCountry);
	free(lpRegion);
	free(lpCity);

	LPSTR lpTimezone = GetYamlField(NWLC->NwBuf, "timezone");
	LPSTR lpUtcOffset = GetYamlField(NWLC->NwBuf, "utc_offset");
	NWL_NodeAttrSetf(node, "Timezone", 0, "%s (%s)", lpTimezone, lpUtcOffset);
	free(lpTimezone);
	free(lpUtcOffset);

	LPSTR lpOrg = GetYamlField(NWLC->NwBuf, "org");
	NWL_NodeAttrSet(node, "Organization", lpOrg, 0);
	free(lpOrg);
}

PNODE NW_PublicIp(VOID)
{
	HMODULE inet = LoadLibraryW(L"wininet.dll");
	PNODE node = NWL_NodeAlloc("PublicIP", 0);
	if (NWLC->PublicIpInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	if (inet == NULL)
		goto out;
	*(FARPROC*)&OsInetOpen = GetProcAddress(inet, "InternetOpenW");
	if (OsInetOpen == NULL)
		goto out;
	*(FARPROC*)&OsInetOpenUrl = GetProcAddress(inet, "InternetOpenUrlW");
	if (OsInetOpenUrl == NULL)
		goto out;
	*(FARPROC*)&OsInetReadFile = GetProcAddress(inet, "InternetReadFile");
	if (OsInetReadFile == NULL)
		goto out;
	*(FARPROC*)&OsInetCloseHandle = GetProcAddress(inet, "InternetCloseHandle");
	if (OsInetCloseHandle == NULL)
		goto out;
	PrintIpAddress(node);
	PrintGeoInfo(node);

out:
	if (inet)
		FreeLibrary(inet);
	OsInetOpen = NULL;
	OsInetOpenUrl = NULL;
	OsInetReadFile = NULL;
	OsInetCloseHandle = NULL;
	return node;
}
