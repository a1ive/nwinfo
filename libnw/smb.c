// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <lm.h>

#include "libnw.h"
#include "utils.h"

static void
EnumConnectedDrives(PNODE pNode, HKEY root, LPCWSTR key)
{
	PVOID lpData = NULL;
	DWORD dwSize = 0;
	DWORD dwType = 0;
	PNODE nd = NWL_NodeAppendNew(pNode, "Drive", NFLG_TABLE_ROW);
	NWL_NodeAttrSetf(nd, "Local Name", 0, "%s:\\", NWL_Ucs2ToUtf8(key));
	lpData = NWL_NtGetRegValue(root, key, L"RemotePath", &dwSize, &dwType);
	if (lpData)
	{
		NWL_NodeAttrSet(nd, "Remote Name", NWL_Ucs2ToUtf8(lpData), 0);
		free(lpData);
	}
	lpData = NWL_NtGetRegValue(root, key, L"ProviderName", &dwSize, &dwType);
	if (lpData)
	{
		NWL_NodeAttrSet(nd, "Provider", NWL_Ucs2ToUtf8(lpData), 0);
		free(lpData);
	}
}

static void
EnumSharedFolders(PNODE pParent)
{
	PSHARE_INFO_502 bufPtr, p;
	NET_API_STATUS res;
	LPTSTR   lpszServer = NULL;
	DWORD er = 0, tr = 0, resume = 0, i;
	HMODULE hDll = LoadLibraryW(L"netapi32.dll");
	NET_API_STATUS(NET_API_FUNCTION * fpNetShareEnum)(LMSTR, DWORD, LPBYTE*, DWORD, LPDWORD, LPDWORD, LPDWORD);
	NET_API_STATUS(NET_API_FUNCTION * fpNetApiBufferFree)(LPVOID);

	if (hDll == NULL)
		return;
	*(FARPROC*)&fpNetShareEnum = GetProcAddress(hDll, "NetShareEnum");
	if (fpNetShareEnum == NULL)
		goto out;
	*(FARPROC*)&fpNetApiBufferFree = GetProcAddress(hDll, "NetApiBufferFree");
	if (fpNetApiBufferFree == NULL)
		goto out;

	do
	{
		res = fpNetShareEnum(lpszServer, 502, (LPBYTE*)&bufPtr, MAX_PREFERRED_LENGTH, &er, &tr, &resume);
		if (res == ERROR_SUCCESS || res == ERROR_MORE_DATA)
		{
			p = bufPtr;
			for (i = 1; i <= er; i++)
			{
				PNODE nd = NWL_NodeAppendNew(pParent, "Shared Folder", NFLG_TABLE_ROW);
				NWL_NodeAttrSet(nd, "Network Name", NWL_Ucs2ToUtf8(p->shi502_netname), 0);
				NWL_NodeAttrSet(nd, "Path", NWL_Ucs2ToUtf8(p->shi502_path), 0);
				NWL_NodeAttrSetf(nd, "Current Uses", NAFLG_FMT_NUMERIC, "%lu", p->shi502_current_uses);
				p++;
			}
			fpNetApiBufferFree(bufPtr);
		}
	}
	while (res == ERROR_MORE_DATA);

out:
	FreeLibrary(hDll);
}

PNODE NW_NetShare(VOID)
{
	HKEY root = NULL;
	DWORD i;
	DWORD dwIndex = 0;
	PNODE node = NWL_NodeAlloc("NetworkDrives", NFLG_TABLE);
	if (NWLC->ShareInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	EnumSharedFolders(node);

	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Network", 0, KEY_READ, &root) != ERROR_SUCCESS)
		goto fail;
	if (RegQueryInfoKeyW(root, NULL, NULL, NULL, &dwIndex, NULL, NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
		goto fail;
	for (i = 0; i < dwIndex; i++)
	{
		DWORD dwSize = NWINFO_BUFSZW;
		if (RegEnumKeyExW(root, i, NWLC->NwBufW, &dwSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
			continue;
		EnumConnectedDrives(node, root, NWLC->NwBufW);
	}

fail:
	if (root)
		RegCloseKey(root);
	return node;
}
