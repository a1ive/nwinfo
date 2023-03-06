// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
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

CHAR* NWL_LoadIdsToMemory(LPCSTR lpFileName, LPDWORD lpSize)
{
	HANDLE Fp = INVALID_HANDLE_VALUE;
	CHAR* Ids = NULL;
	DWORD dwSize = 0;
	BOOL bRet = TRUE;
	CHAR* FilePath = NWLC->NwBuf;
	CHAR* p;
	size_t i = 0;
	if (!GetModuleFileNameA(NULL, FilePath, MAX_PATH) || strlen(FilePath) == 0)
	{
		fprintf(stderr, "GetModuleFileName failed\n");
		goto fail;
	}
	p = strrchr(FilePath, '\\');
	if (!p)
	{
		fprintf(stderr, "Invalid file path %s\n", FilePath);
		goto fail;
	}
	p++;
	strcpy_s(p, MAX_PATH - (p - FilePath), lpFileName);
	Fp = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (Fp == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Cannot open %s\n", FilePath);
		goto fail;
	}
	dwSize = GetFileSize(Fp, NULL);
	if (dwSize == INVALID_FILE_SIZE || dwSize == 0)
	{
		fprintf(stderr, "bad %s file\n", lpFileName);
		goto fail;
	}
	Ids = malloc(dwSize);
	if (!Ids)
	{
		fprintf(stderr, "out of memory\n");
		goto fail;
	}
	bRet = ReadFile(Fp, Ids, dwSize, &dwSize, NULL);
	if (bRet == FALSE)
	{
		fprintf(stderr, "%s read error\n", FilePath);
		goto fail;
	}
	CloseHandle(Fp);
	*lpSize = dwSize;
	return Ids;
fail:
	if (Fp != INVALID_HANDLE_VALUE)
		CloseHandle(Fp);
	if (Ids)
		free(Ids);
	*lpSize = 0;
	return NULL;
}
