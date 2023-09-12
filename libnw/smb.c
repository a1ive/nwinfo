// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "libnw.h"
#include "utils.h"

static void
EnumShares(PNODE pNode, HKEY root, LPCWSTR key)
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

PNODE NW_NetShare(VOID)
{
	HKEY root = NULL;
	DWORD i;
	DWORD dwIndex = 0;
	PNODE node = NWL_NodeAlloc("NetworkDrives", NFLG_TABLE);
	if (NWLC->ShareInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Network", 0, KEY_READ, &root) != ERROR_SUCCESS)
		goto fail;
	if (RegQueryInfoKeyW(root, NULL, NULL, NULL, &dwIndex, NULL, NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
		goto fail;
	for (i = 0; i < dwIndex; i++)
	{
		DWORD dwSize = NWINFO_BUFSZ / sizeof(WCHAR);
		if (RegEnumKeyExW(root, i, (LPWSTR)NWLC->NwBuf, &dwSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
			continue;
		EnumShares(node, root, (LPCWSTR)NWLC->NwBuf);
	}

fail:
	if (root)
		RegCloseKey(root);
	return node;
}
