// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winioctl.h>
#include "libnw.h"
#include "utils.h"
#include "smbios.h"

BOOL NWL_IsAdmin(void)
{
	BOOL b;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;
	b = AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0, &AdministratorsGroup);
	if (b)
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
			b = FALSE;
		FreeSid(AdministratorsGroup);
	}
	return b;
}

DWORD NWL_ObtainPrivileges(LPCSTR privilege)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp = { 0 };
	BOOL res;
	// Obtain required privileges
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return GetLastError();
	res = LookupPrivilegeValueA(NULL, privilege, &tkp.Privileges[0].Luid);
	if (!res)
		return GetLastError();
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
	return GetLastError();
}

LPCSTR NWL_GetHumanSize(UINT64 size, LPCSTR human_sizes[6], UINT64 base)
{
	UINT64 fsize = size, frac = 0;
	unsigned units = 0;
	static char buf[48];
	const char* umsg;

	if (!NWLC->HumanSize)
	{
		snprintf(buf, sizeof(buf), "%llu", size);
		return buf;
	}

	while (fsize >= base && units < 5)
	{
		frac = fsize % base;
		fsize = fsize / base;
		units++;
	}

	umsg = human_sizes[units];

	if (units)
	{
		if (frac)
			frac = frac * 100 / base;
		snprintf(buf, sizeof(buf), "%llu.%02llu %s", fsize, frac, umsg);
	}
	else
		snprintf(buf, sizeof(buf), "%llu %s", size, umsg);
	return buf;
}

static UINT32 nt5_htonl(UINT32 x)
{
	UCHAR* s = (UCHAR*)&x;
	return (UINT32)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
}

void
NWL_ConvertLengthToIpv4Mask(ULONG MaskLength, ULONG* Mask)
{
	if (MaskLength > 32UL)
		*Mask = INADDR_NONE;
	else if (MaskLength == 0)
		*Mask = 0;
	else
		*Mask = nt5_htonl(~0U << (32UL - MaskLength));
}

static BOOL
NT5ReadMem(PVOID buffer, DWORD address, DWORD length)
{
	BOOL bRet = FALSE;
	if (!NWL_NT5InitMemory())
		return FALSE;
	if (NWL_NT5ReadMemory(buffer, address, length))
		bRet = TRUE;
	NWL_NT5ExitMemory();
	return bRet;
}

static UINT
NT5GetSmbios(struct RAW_SMBIOS_DATA* buf, DWORD buflen)
{
	UCHAR* ptr = NULL;
	UCHAR* bios = NULL;
	DWORD smbios_len = 0;
	bios = malloc(0x10000);
	if (!bios)
		return 0;
	if (!NT5ReadMem(bios, 0xf0000, 0x10000))
		goto fail;
	for (ptr = bios; ptr < bios + 0x100000; ptr += 16)
	{
		if (memcmp(ptr, "_SM_", 4) == 0 && NWL_AcpiChecksum(ptr, sizeof(struct smbios_eps)) == 0)
		{
			struct smbios_eps* eps = (struct smbios_eps*)ptr;
			smbios_len = eps->intermediate.table_length;
			if (!buf || buflen < smbios_len + sizeof(struct RAW_SMBIOS_DATA))
				goto fail;
			buf->Length = smbios_len;
			buf->MajorVersion = eps->version_major;
			buf->MinorVersion = eps->version_minor;
			buf->DmiRevision = eps->intermediate.revision;
			NT5ReadMem(buf->Data, eps->intermediate.table_address, smbios_len);
			goto fail;
		}
		if (memcmp(ptr, "_SM3_", 5) == 0 && NWL_AcpiChecksum(ptr, sizeof(struct smbios_eps3)) == 0)
		{
			struct smbios_eps3* eps3 = (struct smbios_eps3*)ptr;
			smbios_len = eps3->maximum_table_length;
			if (!buf || buflen < smbios_len + sizeof(struct RAW_SMBIOS_DATA))
				goto fail;
			buf->Length = smbios_len;
			buf->MajorVersion = eps3->version_major;
			buf->MinorVersion = eps3->version_minor;
			NT5ReadMem(buf->Data, (DWORD)eps3->table_address, smbios_len);
			goto fail;
		}
	}
fail:
	free(bios);
	return smbios_len + sizeof(struct RAW_SMBIOS_DATA);
}

UINT
NWL_EnumSystemFirmwareTables(DWORD FirmwareTableProviderSignature,
	PVOID pFirmwareTableEnumBuffer, DWORD BufferSize)
{
	UINT(WINAPI * NT6EnumSystemFirmwareTables)
		(DWORD FirmwareTableProviderSignature, PVOID pFirmwareTableEnumBuffer, DWORD BufferSize) = NULL;
	HMODULE hMod = GetModuleHandleA("kernel32");

	if (hMod)
		*(FARPROC*)&NT6EnumSystemFirmwareTables = GetProcAddress(hMod, "EnumSystemFirmwareTables");

	if (NT6EnumSystemFirmwareTables)
		return NT6EnumSystemFirmwareTables(FirmwareTableProviderSignature,
			pFirmwareTableEnumBuffer, BufferSize);
	else
		return 0;
}

UINT
NWL_GetSystemFirmwareTable(DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID,
	PVOID pFirmwareTableBuffer, DWORD BufferSize)
{
	UINT(WINAPI * NT6GetSystemFirmwareTable)
		(DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID, PVOID pFirmwareTableBuffer, DWORD BufferSize) = NULL;
	HMODULE hMod = GetModuleHandleA("kernel32");

	if (hMod)
		*(FARPROC*)&NT6GetSystemFirmwareTable = GetProcAddress(hMod, "GetSystemFirmwareTable");

	if (NT6GetSystemFirmwareTable)
		return NT6GetSystemFirmwareTable(FirmwareTableProviderSignature, FirmwareTableID, pFirmwareTableBuffer, BufferSize);

	if (FirmwareTableProviderSignature == 'RSMB')
		return NT5GetSmbios(pFirmwareTableBuffer, BufferSize);

	return 0;
}

DWORD
NWL_GetFirmwareEnvironmentVariable(LPCSTR lpName, LPCSTR lpGuid,
	PVOID pBuffer, DWORD nSize)
{
	DWORD(WINAPI * NT6GetFirmwareEnvironmentVariable)
		(LPCSTR lpName, LPCSTR lpGuid,
			PVOID pBuffer, DWORD nSize) = NULL;
	HMODULE hMod = GetModuleHandleA("kernel32");

	if (hMod)
		*(FARPROC*)&NT6GetFirmwareEnvironmentVariable = GetProcAddress(hMod, "GetFirmwareEnvironmentVariableA");

	if (NT6GetFirmwareEnvironmentVariable)
		return NT6GetFirmwareEnvironmentVariable(lpName, lpGuid, pBuffer, nSize);
	SetLastError(ERROR_INVALID_FUNCTION);
	return 0;
}

PVOID NWL_GetAcpi(DWORD TableId)
{
	PVOID pFirmwareTableBuffer = NULL;
	UINT BufferSize = 0;
	BufferSize = NWL_GetSystemFirmwareTable('ACPI', TableId, NULL, 0);
	if (BufferSize == 0)
		return NULL;
	pFirmwareTableBuffer = malloc(BufferSize);
	if (!pFirmwareTableBuffer)
		return NULL;
	NWL_GetSystemFirmwareTable('ACPI', TableId, pFirmwareTableBuffer, BufferSize);
	return pFirmwareTableBuffer;
}

UINT8
NWL_AcpiChecksum(VOID* base, UINT size)
{
	UINT8* ptr;
	UINT8 ret = 0;
	for (ptr = (UINT8*)base; ptr < ((UINT8*)base) + size;
		ptr++)
		ret += *ptr;
	return ret;
}

VOID NWL_TrimString(CHAR* String)
{
	CHAR* Pos1 = String;
	CHAR* Pos2 = String;
	size_t Len = strlen(String);

	while (Len > 0)
	{
		if (String[Len - 1] != ' ' && String[Len - 1] != '\t')
			break;
		String[Len - 1] = 0;
		Len--;
	}

	while (*Pos1 == ' ' || *Pos1 == '\t')
		Pos1++;

	while (*Pos1)
		*Pos2++ = *Pos1++;
	*Pos2++ = 0;

	return;
}

INT NWL_GetRegDwordValue(HKEY Key, LPCSTR SubKey, LPCSTR ValueName, DWORD* pValue)
{
	HKEY hKey;
	DWORD Type;
	DWORD Size;
	LSTATUS lRet;
	DWORD Value = 0;
	lRet = RegOpenKeyExA(Key, SubKey, 0, KEY_QUERY_VALUE, &hKey);
	if (ERROR_SUCCESS == lRet)
	{
		Size = sizeof(Value);
		lRet = RegQueryValueExA(hKey, ValueName, NULL, &Type, (LPBYTE)&Value, &Size);
		*pValue = Value;
		RegCloseKey(hKey);
		return 0;
	}
	return 1;
}

CHAR* NWL_GetRegSzValue(HKEY Key, LPCSTR SubKey, LPCSTR ValueName)
{
	HKEY hKey;
	DWORD Type;
	DWORD Size = 1024;
	LSTATUS lRet;
	CHAR* sRet = NULL;
	lRet = RegOpenKeyExA(Key, SubKey, 0, KEY_QUERY_VALUE, &hKey);
	if (lRet != ERROR_SUCCESS)
		return NULL;
	sRet = malloc(Size);
	if (!sRet)
		return NULL;
	lRet = RegQueryValueExA(hKey, ValueName, NULL, &Type, (LPBYTE)sRet, &Size);
	if (lRet != ERROR_SUCCESS)
	{
		free(sRet);
		return NULL;
	}
	RegCloseKey(hKey);
	return sRet;
}

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
NWL_FindClass(PNODE nd, CHAR* Ids, DWORD IdsSize, CONST CHAR* Class)
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
				NWL_NodeAttrSet(nd, "Prog IF", sLine + 6, 0);
				free(sLine);
				break;
			}
			break;
		}
	out:
		break;
	}
}

CHAR* NWL_LoadFileToMemory(LPCSTR lpFileName, LPDWORD lpSize)
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
		fprintf(stderr, "pci.ids read error\n");
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

LPCSTR
NWL_GuidToStr(UCHAR Guid[16])
{
	static CHAR GuidStr[37] = { 0 };
	snprintf(GuidStr, 37, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
		Guid[0], Guid[1], Guid[2], Guid[3], Guid[4], Guid[5], Guid[6], Guid[7],
		Guid[8], Guid[9], Guid[10], Guid[11], Guid[12], Guid[13], Guid[14], Guid[15]);
	return GuidStr;
}

static CHAR Mbs[256] = { 0 };

LPCSTR
NWL_WcsToMbs(PWCHAR Wcs)
{
	size_t i = 0;
	for (i = 0; i < 255; i++)
	{
		if (Wcs[i] == 0 || !isprint(Wcs[i]) || Wcs[i] > 128)
			break;
		Mbs[i] = (CHAR)Wcs[i];
	}
	Mbs[i] = 0;
	return Mbs;
}
