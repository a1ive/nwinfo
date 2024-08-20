// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <lm.h>

#include "libnw.h"
#include "utils.h"

static void
EnumConnectedDrives(PNODE pParent)
{
	HKEY root = NULL;
	DWORD i;
	DWORD dwIndex = 0;

	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Network", 0, KEY_READ, &root) != ERROR_SUCCESS)
		goto fail;
	if (RegQueryInfoKeyW(root, NULL, NULL, NULL, &dwIndex, NULL, NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
		goto fail;
	for (i = 0; i < dwIndex; i++)
	{
		DWORD dwType = 0;
		DWORD dwSize = NWINFO_BUFSZW;
		if (RegEnumKeyExW(root, i, NWLC->NwBufW, &dwSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
			continue;
		PNODE nd = NWL_NodeAppendNew(pParent, "Drive", NFLG_TABLE_ROW);
		NWL_NodeAttrSetf(nd, "Local Name", 0, "%s:\\", NWL_Ucs2ToUtf8(NWLC->NwBufW));
		PVOID lpData = NWL_NtGetRegValue(root, NWLC->NwBufW, L"RemotePath", &dwSize, &dwType);
		if (lpData)
		{
			NWL_NodeAttrSet(nd, "Remote Name", NWL_Ucs2ToUtf8(lpData), 0);
			free(lpData);
		}
		lpData = NWL_NtGetRegValue(root, NWLC->NwBufW, L"ProviderName", &dwSize, &dwType);
		if (lpData)
		{
			NWL_NodeAttrSet(nd, "Provider", NWL_Ucs2ToUtf8(lpData), 0);
			free(lpData);
		}
	}

fail:
	if (root)
		RegCloseKey(root);
}

static LPCSTR
GetSharedFolderType(DWORD dwType)
{
	dwType &= STYPE_MASK;
	switch (dwType)
	{
	case STYPE_DISKTREE: return "Disk Drive";
	case STYPE_PRINTQ: return "Print Queue";
	case STYPE_DEVICE: return "Communication device";
	case STYPE_IPC: return "IPC";
	default: return "Unknown";
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
				NWL_NodeAttrSet(nd, "Remark", NWL_Ucs2ToUtf8(p->shi502_remark), 0);
				NWL_NodeAttrSetf(nd, "Current Uses", NAFLG_FMT_NUMERIC, "%lu", p->shi502_current_uses);
				NWL_NodeAttrSetBool(nd, "Special Share", (p->shi502_type & STYPE_SPECIAL), 0);
				NWL_NodeAttrSetBool(nd, "Temporary Share", (p->shi502_type & STYPE_TEMPORARY), 0);
				NWL_NodeAttrSet(nd, "Type", GetSharedFolderType(p->shi502_type), 0);
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
	PNODE node = NWL_NodeAlloc("NetworkDrives", NFLG_TABLE);
	if (NWLC->ShareInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	EnumSharedFolders(node);

	EnumConnectedDrives(node);

	return node;
}
