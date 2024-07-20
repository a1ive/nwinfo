// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <pathcch.h>
#include "libnw.h"
#include "utils.h"
#include <libcpuid.h>
#include <winring0.h>

static CHAR*
IdsGetline(CHAR* Ids, DWORD IdsSize, DWORD* Offset)
{
	CHAR* Line = NULL;
	DWORD i = 0, Len = 0;
	if (*Offset >= IdsSize)
		return NULL;
	for (i = *Offset; i < IdsSize; i++)
	{
		if (Ids[i] == '\n' || Ids[i] == '\r')
			break;
	}
	Len = i - *Offset;
	Line = malloc((SIZE_T)Len + 1);
	if (!Line)
		return NULL;
	memcpy(Line, Ids + *Offset, Len);
	Line[Len] = 0;
	*Offset += Len;
	for (i = *Offset; i < IdsSize; i++, (*Offset)++)
	{
		if (Ids[i] != '\n' && Ids[i] != '\r')
			break;
	}
	return Line;
}

VOID
NWL_FindId(PNODE nd, CHAR* Ids, DWORD IdsSize, CONST CHAR* v, CONST CHAR* d, CONST CHAR* s, INT usb)
{
	DWORD Offset = 0;
	CHAR* vLine = NULL;
	CHAR* dLine = NULL;
	CHAR* sLine = NULL;
	if (!v || !d)
		return;
	vLine = IdsGetline(Ids, IdsSize, &Offset);
	while (vLine)
	{
		if (!vLine[0] || vLine[0] == '#' || strlen(vLine) < 7)
		{
			free(vLine);
			vLine = IdsGetline(Ids, IdsSize, &Offset);
			continue;
		}
		if (_strnicmp(v, vLine, 4) != 0)
		{
			free(vLine);
			vLine = IdsGetline(Ids, IdsSize, &Offset);
			continue;
		}
		NWL_NodeAttrSet(nd, "Vendor", vLine + 6, 0);
		free(vLine);
		dLine = IdsGetline(Ids, IdsSize, &Offset);
		while (dLine)
		{
			if (!dLine[0] || dLine[0] == '#')
			{
				free(dLine);
				dLine = IdsGetline(Ids, IdsSize, &Offset);
				continue;
			}
			if (dLine[0] != '\t' || strlen(dLine) < 8)
			{
				free(dLine);
				break;
			}
			if (_strnicmp(d, dLine + 1, 4) != 0)
			{
				free(dLine);
				dLine = IdsGetline(Ids, IdsSize, &Offset);
				continue;
			}
			NWL_NodeAttrSet(nd, "Device", dLine + 7, 0);
			free(dLine);
			if (!s)
				break;
			sLine = IdsGetline(Ids, IdsSize, &Offset);
			while (sLine)
			{
				if (!sLine[0] || sLine[0] == '#')
				{
					free(sLine);
					sLine = IdsGetline(Ids, IdsSize, &Offset);
					continue;
				}
				if (sLine[0] != '\t' || !sLine[1] || sLine[1] != '\t' || strlen(sLine) < 14)
				{
					free(sLine);
					break;
				}
				if (_strnicmp(s, sLine + 2, 9) != 0)
				{
					free(sLine);
					sLine = IdsGetline(Ids, IdsSize, &Offset);
					continue;
				}
				NWL_NodeAttrSet(nd, usb ? "Interface" : "Subsys", sLine + 13, 0);
				free(sLine);
				break;
			}
			break;
		}
		break;
	}
}

BOOL
NWL_ParseHwid(PNODE nd, CHAR* Ids, DWORD IdsSize, LPCWSTR Hwid, INT usb)
{
	// PCI\VEN_XXXX&DEV_XXXX
	// PCI\VEN_XXXX&DEV_XXXX&SUBSYS_XXXXXXXX
	// USB\VID_XXXX&PID_XXXX
	// USB\ROOT_HUBXX&VIDXXXX&PIDXXXX
	size_t i;
	CHAR vid[5] = { 0 };
	CHAR did[5] = { 0 };
	CHAR subsys[10] = { 0 };
	LPCWSTR p = Hwid;
	LPCWSTR vidNeedle = L"VEN";
	LPCWSTR didNeedle = L"DEV";
	if (usb)
	{
		vidNeedle = L"VID";
		didNeedle = L"PID";
	}

	p = wcsstr(p, vidNeedle);
	if (p == NULL)
		return FALSE;
	p += 3;
	if (p[0] == '_')
		p++;
	for (i = 0; i < 4 && p[i]; i++)
		vid[i] = (CHAR)p[i];
	p = wcsstr(p, didNeedle);
	if (p == NULL)
		return FALSE;
	p += 3;
	if (p[0] == '_')
		p++;
	for (i = 0; i < 4 && p[i]; i++)
		did[i] = (CHAR)p[i];
	p = wcsstr(p, L"SUBSYS_");
	if (p != NULL)
	{
		p += 7;
		for (i = 0; i < 4 && p[i]; i++)
			subsys[i] = (CHAR)p[i];
		subsys[4] = ' ';
		for (i = 4; i < 8 && p[i]; i++)
			subsys[i + 1] = (CHAR)p[i];
	}

	NWL_NodeAttrSet(nd, "Vendor ID", vid, 0);
	NWL_NodeAttrSet(nd, "Device ID", did, 0);
	NWL_FindId(nd, Ids, IdsSize, vid, did, p ? subsys : NULL, usb);
	return 1;
}

VOID
NWL_FindClass(PNODE nd, CHAR* Ids, DWORD IdsSize, CONST CHAR* Class, INT usb)
{
	DWORD Offset = 0;
	CHAR* vLine = NULL;
	CHAR* dLine = NULL;
	CHAR* sLine = NULL;
	if (!Class)
		return;
	CONST CHAR* v = Class;
	CONST CHAR* d = strlen(Class) >= 4 ? Class + 2 : NULL;
	CONST CHAR* s = strlen(Class) >= 6 ? Class + 4 : NULL;
	vLine = IdsGetline(Ids, IdsSize, &Offset);
	while (vLine)
	{
		if (!vLine[0] || vLine[0] != 'C' || strlen(vLine) < 7 || vLine[1] != ' ')
		{
			free(vLine);
			vLine = IdsGetline(Ids, IdsSize, &Offset);
			continue;
		}
		if (_strnicmp(v, vLine + 2, 2) != 0)
		{
			free(vLine);
			vLine = IdsGetline(Ids, IdsSize, &Offset);
			continue;
		}
		NWL_NodeAttrSet(nd, "Class", vLine + 6, 0);
		free(vLine);
		if (!d)
			goto out;
		dLine = IdsGetline(Ids, IdsSize, &Offset);
		while (dLine)
		{
			if (!dLine[0] || dLine[0] == '#')
			{
				free(dLine);
				dLine = IdsGetline(Ids, IdsSize, &Offset);
				continue;
			}
			if (dLine[0] != '\t' || strlen(dLine) < 6)
			{
				free(dLine);
				break;
			}
			if (_strnicmp(d, dLine + 1, 2) != 0)
			{
				free(dLine);
				dLine = IdsGetline(Ids, IdsSize, &Offset);
				continue;
			}
			NWL_NodeAttrSet(nd, "Subclass", dLine + 5, 0);
			free(dLine);
			if (!s)
				break;
			sLine = IdsGetline(Ids, IdsSize, &Offset);
			while (sLine)
			{
				if (!sLine[0] || sLine[0] == '#')
				{
					free(sLine);
					sLine = IdsGetline(Ids, IdsSize, &Offset);
					continue;
				}
				if (sLine[0] != '\t' || !sLine[1] || sLine[1] != '\t' || strlen(sLine) < 7)
				{
					free(sLine);
					break;
				}
				if (_strnicmp(s, sLine + 2, 2) != 0)
				{
					free(sLine);
					sLine = IdsGetline(Ids, IdsSize, &Offset);
					continue;
				}
				NWL_NodeAttrSet(nd, usb ? "Protocol" : "Prog IF", sLine + 6, 0);
				free(sLine);
				break;
			}
			break;
		}
	out:
		break;
	}
}

VOID
NWL_GetPnpManufacturer(PNODE nd, CHAR* Ids, DWORD IdsSize, CONST CHAR* Code)
{
	DWORD Offset = 0;
	CHAR* Line = NULL;

	Line = IdsGetline(Ids, IdsSize, &Offset);
	while (Line)
	{
		if (isalpha(Line[0]) && isalpha(Line[1]) && isalpha(Line[2]) && isspace(Line[3])
			&& _strnicmp(Code, Line, 3) == 0)
		{
			NWL_NodeAttrSet(nd, "Manufacturer", Line + 4, 0);
			free(Line);
			return;
		}
		free(Line);
		Line = IdsGetline(Ids, IdsSize, &Offset);
	}
	NWL_NodeAttrSet(nd, "Manufacturer", Code, 0);
}

VOID
NWL_GetSpdManufacturer(PNODE nd, CHAR* Ids, DWORD IdsSize, UINT Bank, UINT Item)
{
	DWORD Offset = 0;
	CHAR* bLine = NULL;
	CHAR* iLine = NULL;
	CHAR* p = NULL;

	Bank++;

	bLine = IdsGetline(Ids, IdsSize, &Offset);
	while (bLine)
	{
		if (isdigit(bLine[0])
			&& Bank == strtoul(bLine, NULL, 10))
		{
			iLine = IdsGetline(Ids, IdsSize, &Offset);
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
					NWL_NodeAttrSet(nd, "Manufacturer", &p[1], 0);
					free(iLine);
					free(bLine);
					return;
				}
				free(iLine);
				iLine = IdsGetline(Ids, IdsSize, &Offset);
			}
		}
		free(bLine);
		bLine = IdsGetline(Ids, IdsSize, &Offset);
	}
fail:
	NWL_NodeAttrSetf(nd, "Manufacturer", 0, "%02X%02X", Bank, Item);
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

CHAR* NWL_LoadIdsToMemory(LPCWSTR lpFileName, LPDWORD lpSize)
{
	HANDLE Fp = INVALID_HANDLE_VALUE;
	CHAR* Ids = NULL;
	DWORD dwSize = 0;
	BOOL bRet = TRUE;
	Fp = GetIdsHandle(lpFileName);
	if (Fp == INVALID_HANDLE_VALUE)
		goto fail;
	dwSize = GetFileSize(Fp, NULL);
	if (dwSize == INVALID_FILE_SIZE || dwSize == 0)
	{
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "Bad %s file", NWL_Ucs2ToUtf8(lpFileName));
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, NWLC->NwBuf);
		goto fail;
	}
	Ids = malloc(dwSize);
	if (!Ids)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Memory allocation failed in "__FUNCTION__);
		goto fail;
	}
	bRet = ReadFile(Fp, Ids, dwSize, &dwSize, NULL);
	if (bRet == FALSE)
	{
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s read error", NWL_Ucs2ToUtf8(lpFileName));
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, NWLC->NwBuf);
		goto fail;
	}
	CloseHandle(Fp);
	*lpSize = dwSize;
	return Ids;
fail:
	if (Fp && Fp != INVALID_HANDLE_VALUE)
		CloseHandle(Fp);
	if (Ids)
		free(Ids);
	*lpSize = 0;
	return NULL;
}

const CHAR* NWL_GetIdsDate(LPCWSTR lpFileName)
{
	static CHAR Date[] = "1453.05.29";
	DWORD IdsSize = 0;
	DWORD Offset = 0;
	CHAR* Ids = NULL;
	CHAR* Line = NULL;

	strcpy_s(Date, sizeof(Date), "UNKNOWN");

	Ids = NWL_LoadIdsToMemory(lpFileName, &IdsSize);
	Line = IdsGetline(Ids, IdsSize, &Offset);
	while (Line)
	{
		// # Version: 2022.09.09
		if (Line[0] == '#' && isspace(Line[1])
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
		Line = IdsGetline(Ids, IdsSize, &Offset);
	}
	if (Line)
		free(Line);
	if (Ids)
		free(Ids);

	return Date;
}
