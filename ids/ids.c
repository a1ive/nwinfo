// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <pathcch.h>
#include "libnw.h"
#include "utils.h"
#include "libcpuid.h"
#include "ioctl.h"

#pragma comment(lib, "pathcch.lib")

static CHAR*
IdsGetline(PNWLIB_IDS Ids, DWORD* Offset)
{
	CHAR* Line = NULL;
	DWORD i = 0, Len = 0;
	if (*Offset >= Ids->Size)
		return NULL;
	for (i = *Offset; i < Ids->Size; i++)
	{
		if (Ids->Ids[i] == '\n' || Ids->Ids[i] == '\r')
			break;
	}
	Len = i - *Offset;
	Line = malloc((SIZE_T)Len + 1);
	if (!Line)
		return NULL;
	memcpy(Line, Ids->Ids + *Offset, Len);
	Line[Len] = 0;
	*Offset += Len;
	for (i = *Offset; i < Ids->Size; i++, (*Offset)++)
	{
		if (Ids->Ids[i] != '\n' && Ids->Ids[i] != '\r')
			break;
	}
	return Line;
}

static BOOL
NWL_FindVendor(PNODE nd, PNWLIB_IDS Ids, CONST CHAR* v, CONST CHAR* key, DWORD* outOffset)
{
	DWORD Offset = 0;
	CHAR* Line = NULL;
	if (outOffset)
		*outOffset = 0;
	if (!v || !v[0])
		return FALSE;
	Line = IdsGetline(Ids, &Offset);
	while (Line)
	{
		size_t Len = 0;
		if (!Line[0] || Line[0] == '#' || Line[0] == '\t')
		{
			free(Line);
			Line = IdsGetline(Ids, &Offset);
			continue;
		}
		Len = strlen(Line);
		if (Len < 7)
		{
			free(Line);
			Line = IdsGetline(Ids, &Offset);
			continue;
		}
		if (_strnicmp(v, Line, 4) == 0)
		{
			if (key)
				NWL_NodeAttrSet(nd, key, Line + 6, 0);
			free(Line);
			if (outOffset)
				*outOffset = Offset;
			return TRUE;
		}
		free(Line);
		Line = IdsGetline(Ids, &Offset);
	}
	return FALSE;
}

static void
NWL_FindId(PNODE nd, PNWLIB_IDS Ids, CONST CHAR* v, CONST CHAR* d, CONST CHAR* s, INT usb)
{
	DWORD Offset = 0;
	CHAR* Line = NULL;
	BOOL FoundDevice = FALSE;
	if (!v || !v[0] || !d || !d[0])
		return;

	if (!NWL_FindVendor(nd, Ids, v, "Vendor", &Offset))
		return;

	Line = IdsGetline(Ids, &Offset);
	while (Line)
	{
		size_t Len = 0;
		if (!Line[0] || Line[0] == '#')
		{
			free(Line);
			Line = IdsGetline(Ids, &Offset);
			continue;
		}

		Len = strlen(Line);
		if (!FoundDevice)
		{
			if (Line[0] != '\t')
			{
				free(Line);
				break;
			}
			if (Line[1] == '\t' || Len < 8)
			{
				free(Line);
				Line = IdsGetline(Ids, &Offset);
				continue;
			}
			if (_strnicmp(d, Line + 1, 4) != 0)
			{
				free(Line);
				Line = IdsGetline(Ids, &Offset);
				continue;
			}
			NWL_NodeAttrSet(nd, "Device", Line + 7, 0);
			FoundDevice = TRUE;
			free(Line);
			if (!s)
				break;
			Line = IdsGetline(Ids, &Offset);
			continue;
		}

		if (Line[0] != '\t' || Line[1] != '\t' || Len < 14)
		{
			free(Line);
			break;
		}
		if (_strnicmp(s, Line + 2, 9) != 0)
		{
			free(Line);
			Line = IdsGetline(Ids, &Offset);
			continue;
		}
		NWL_NodeAttrSet(nd, usb ? "Interface" : "Subsys", Line + 13, 0);
		free(Line);
		break;
	}
}

static BOOL
NWL_ParseHwidId(LPCWSTR Hwid, LPCWSTR Needle, CHAR out[5], LPCWSTR* Next)
{
	LPCWSTR p = wcsstr(Hwid, Needle);
	size_t i;
	if (p == NULL)
		return FALSE;
	p += 3;
	if (p[0] == L'_')
		p++;
	for (i = 0; i < 4 && p[i]; i++)
		out[i] = (CHAR)p[i];
	out[i] = 0;
	if (Next)
		*Next = p + i;
	return TRUE;
}

static BOOL
NWL_ParseSubsys(LPCWSTR Hwid, CHAR subsys[10], CHAR subvendor[5])
{
	LPCWSTR p = wcsstr(Hwid, L"SUBSYS_");
	size_t i;
	if (p == NULL)
		return FALSE;
	p += 7;
	for (i = 0; i < 8 && p[i]; i++)
		;
	if (i < 8)
		return FALSE;
	for (i = 0; i < 4; i++)
	{
		subvendor[i] = (CHAR)p[i + 4];
		subsys[i] = (CHAR)p[i + 4];
		subsys[i + 5] = (CHAR)p[i];
	}
	subvendor[4] = 0;
	subsys[4] = ' ';
	subsys[9] = 0;
	return TRUE;
}

BOOL
NWL_ParseHwid(PNODE nd, struct _NWLIB_IDS* Ids, LPCWSTR Hwid, INT usb)
{
	// PCI\VEN_XXXX&DEV_XXXX
	// PCI\VEN_XXXX&DEV_XXXX&SUBSYS_XXXXXXXX
	// USB\VID_XXXX&PID_XXXX
	// USB\ROOT_HUBXX&VIDXXXX&PIDXXXX
	CHAR vid[5] = { 0 };
	CHAR did[5] = { 0 };
	CHAR subsys[10] = { 0 };
	CHAR subvendor[5] = { 0 };
	LPCWSTR p = Hwid;
	LPCWSTR vidNeedle = L"VEN";
	LPCWSTR didNeedle = L"DEV";
	BOOL hasSubsys = FALSE;
	if (usb)
	{
		vidNeedle = L"VID";
		didNeedle = L"PID";
	}

	if (!Hwid)
		return FALSE;
	if (!NWL_ParseHwidId(p, vidNeedle, vid, &p))
		return FALSE;
	if (!NWL_ParseHwidId(p, didNeedle, did, &p))
		return FALSE;
	if (!usb)
	{
		hasSubsys = NWL_ParseSubsys(p, subsys, subvendor);
	}

	NWL_NodeAttrSet(nd, "Vendor ID", vid, 0);
	NWL_NodeAttrSet(nd, "Device ID", did, 0);
	if (!usb && hasSubsys)
		NWL_NodeAttrSet(nd, "Subvendor ID", subvendor, 0);
	NWL_FindId(nd, Ids, vid, did, hasSubsys ? subsys : NULL, usb);
	if (!usb && hasSubsys && subvendor[0])
		NWL_FindVendor(nd, Ids, subvendor, "Subvendor", NULL);
	return 1;
}

VOID
NWL_FindClass(PNODE nd, struct _NWLIB_IDS* Ids, CONST CHAR* Class, INT usb)
{
	DWORD Offset = 0;
	CHAR* Line = NULL;
	BOOL FoundClass = FALSE;
	BOOL FoundSubclass = FALSE;
	size_t ClassLen = 0;
	if (!Class || !Class[0])
		return;
	ClassLen = strlen(Class);
	CONST CHAR* v = Class;
	CONST CHAR* d = ClassLen >= 4 ? Class + 2 : NULL;
	CONST CHAR* s = ClassLen >= 6 ? Class + 4 : NULL;
	Line = IdsGetline(Ids, &Offset);
	while (Line)
	{
		size_t Len = 0;
		if (!Line[0] || Line[0] == '#')
		{
			free(Line);
			Line = IdsGetline(Ids, &Offset);
			continue;
		}

		Len = strlen(Line);
		if (!FoundClass)
		{
			if (Len < 7 || Line[0] != 'C' || Line[1] != ' ')
			{
				free(Line);
				Line = IdsGetline(Ids, &Offset);
				continue;
			}
			if (_strnicmp(v, Line + 2, 2) != 0)
			{
				free(Line);
				Line = IdsGetline(Ids, &Offset);
				continue;
			}
			NWL_NodeAttrSet(nd, "Class", Line + 6, 0);
			FoundClass = TRUE;
			free(Line);
			if (!d)
				break;
			Line = IdsGetline(Ids, &Offset);
			continue;
		}

		if (!FoundSubclass)
		{
			if (Line[0] != '\t' || Len < 6)
			{
				free(Line);
				break;
			}
			if (Line[1] == '\t')
			{
				free(Line);
				Line = IdsGetline(Ids, &Offset);
				continue;
			}
			if (_strnicmp(d, Line + 1, 2) != 0)
			{
				free(Line);
				Line = IdsGetline(Ids, &Offset);
				continue;
			}
			NWL_NodeAttrSet(nd, "Subclass", Line + 5, 0);
			FoundSubclass = TRUE;
			free(Line);
			if (!s)
				break;
			Line = IdsGetline(Ids, &Offset);
			continue;
		}

		if (Line[0] != '\t' || Line[1] != '\t' || Len < 7)
		{
			free(Line);
			break;
		}
		if (_strnicmp(s, Line + 2, 2) != 0)
		{
			free(Line);
			Line = IdsGetline(Ids, &Offset);
			continue;
		}
		NWL_NodeAttrSet(nd, usb ? "Protocol" : "Prog IF", Line + 6, 0);
		free(Line);
		break;
	}
}

VOID
NWL_GetPnpManufacturer(PNODE nd, struct _NWLIB_IDS* Ids, CONST CHAR* Code)
{
	DWORD Offset = 0;
	CHAR* Line = NULL;
	if (!Code || !Code[0])
		return;

	Line = IdsGetline(Ids, &Offset);
	while (Line)
	{
		size_t Len = strlen(Line);
		if (Len >= 4 && isprint(Line[0]) && isprint(Line[1]) && isprint(Line[2]) && isspace(Line[3])
			&& _strnicmp(Code, Line, 3) == 0)
		{
			NWL_NodeAttrSet(nd, "Manufacturer", Line + 4, 0);
			free(Line);
			return;
		}
		free(Line);
		Line = IdsGetline(Ids, &Offset);
	}
	NWL_NodeAttrSet(nd, "Manufacturer", Code, 0);
}

VOID
NWL_GetSpdManufacturer(PNODE nd, LPCSTR Key, struct _NWLIB_IDS* Ids, UINT Bank, UINT Item)
{
	DWORD Offset = 0;
	CHAR* bLine = NULL;
	CHAR* iLine = NULL;
	CHAR* p = NULL;
	UINT targetBank = Bank + 1;

	bLine = IdsGetline(Ids, &Offset);
	while (bLine)
	{
		if (isdigit(bLine[0])
			&& targetBank == strtoul(bLine, NULL, 10))
		{
			iLine = IdsGetline(Ids, &Offset);
			while (iLine)
			{
				if (!isspace(iLine[0]) || !isdigit(iLine[1]))
				{
					free(iLine);
					free(bLine);
					goto fail;
				}
				if (Item == strtoul(iLine, &p, 10) && isspace(p[0]))
				{
					NWL_NodeAttrSet(nd, Key, &p[1], 0);
					free(iLine);
					free(bLine);
					return;
				}
				free(iLine);
				iLine = IdsGetline(Ids, &Offset);
			}
		}
		free(bLine);
		bLine = IdsGetline(Ids, &Offset);
	}
fail:
	NWL_NodeAttrSetf(nd, Key, 0, "%02X%02X", Bank, Item);
}

static HANDLE
GetIdsHandle(LPCWSTR lpFileName)
{
	HANDLE Fp = INVALID_HANDLE_VALUE;
	WCHAR FilePath[MAX_PATH];
	GetModuleFileNameW(NULL, FilePath, MAX_PATH);
	PathCchRemoveFileSpec(FilePath, MAX_PATH);
	PathCchAppend(FilePath, MAX_PATH, lpFileName);
	Fp = CreateFileW(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (Fp == NULL)
		Fp = INVALID_HANDLE_VALUE;
	if (Fp == INVALID_HANDLE_VALUE)
	{
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "Cannot open %s", NWL_Ucs2ToUtf8(FilePath));
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, NWLC->NwBuf);
	}
	return Fp;
}

BOOL NWL_LoadIdsToMemory(LPCWSTR lpFileName, struct _NWLIB_IDS* lpIds)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	CHAR* szIds = NULL;
	DWORD dwSize = 0;
	BOOL bRet = TRUE;
	lpIds->Ids = NULL;
	lpIds->Size = 0;
	hFile = GetIdsHandle(lpFileName);
	if (hFile == INVALID_HANDLE_VALUE)
		goto fail;
	dwSize = GetFileSize(hFile, NULL);
	if (dwSize == INVALID_FILE_SIZE || dwSize == 0)
	{
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "Bad %s file", NWL_Ucs2ToUtf8(lpFileName));
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, NWLC->NwBuf);
		goto fail;
	}
	szIds = malloc(dwSize);
	if (!szIds)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Memory allocation failed in "__FUNCTION__);
		goto fail;
	}
	bRet = ReadFile(hFile, szIds, dwSize, &dwSize, NULL);
	if (bRet == FALSE)
	{
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s read error", NWL_Ucs2ToUtf8(lpFileName));
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, NWLC->NwBuf);
		goto fail;
	}
	CloseHandle(hFile);
	lpIds->Ids = szIds;
	lpIds->Size = dwSize;
	lpIds->Alloc = TRUE;
	return TRUE;
fail:
	if (hFile && hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if (szIds)
		free(szIds);
	return FALSE;
}

VOID NWL_UnloadIds(struct _NWLIB_IDS* lpIds)
{
	if (!lpIds->Alloc)
		return;
	if (lpIds->Ids)
		free(lpIds->Ids);
	lpIds->Ids = NULL;
	lpIds->Size = 0;
}

const CHAR* NWL_GetIdsDate(struct _NWLIB_IDS* Ids)
{
	static CHAR Date[] = "1453.05.29";
	DWORD Offset = 0;
	CHAR* Line = NULL;

	strcpy_s(Date, sizeof(Date), "UNKNOWN");

	Line = IdsGetline(Ids, &Offset);
	while (Line)
	{
		size_t Len = 0;
		if (Line[0] != '#')
		{
			free(Line);
			Line = IdsGetline(Ids, &Offset);
			continue;
		}
		Len = strlen(Line);
		// # Version: 2022.09.09
		if (Len >= 21 && isspace(Line[1])
			&& _strnicmp("Version:", &Line[2], 8) == 0 && isspace(Line[10])
			&& isdigit(Line[11]) && isdigit(Line[12]) && isdigit(Line[13]) && isdigit(Line[14])
			&& Line[15] == '.' && isdigit(Line[16]) && isdigit(Line[17])
			&& Line[18] == '.' && isdigit(Line[19]) && isdigit(Line[20]))
		{
			snprintf(Date, sizeof(Date), "%c%c%c%c.%c%c.%c%c",
				Line[11], Line[12], Line[13], Line[14],
				Line[16], Line[17],
				Line[19], Line[20]);
			break;
		}
		free(Line);
		Line = IdsGetline(Ids, &Offset);
	}
	if (Line)
		free(Line);

	return Date;
}
