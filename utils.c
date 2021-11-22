// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winioctl.h>
#include <sysinfoapi.h>
#include "nwinfo.h"

void ObtainPrivileges(LPCTSTR privilege) {
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp = { 0 };
	BOOL res;
	DWORD error;
	// Obtain required privileges
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		printf("OpenProcessToken failed!\n");
		error = GetLastError();
		exit(error);
	}

	res = LookupPrivilegeValue(NULL, privilege, &tkp.Privileges[0].Luid);
	if (!res) {
		printf("LookupPrivilegeValue failed!\n");
		error = GetLastError();
		exit(error);
	}
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

	error = GetLastError();
	if (error != ERROR_SUCCESS) {
		printf("AdjustTokenPrivileges failed\n");
		exit(error);
	}

}

const char* GetHumanSize(UINT64 size, const char* human_sizes[6], UINT64 base)
{
	UINT64 fsize = size, frac = 0;
	unsigned units = 0;
	static char buf[48];
	const char* umsg;

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

PVOID GetAcpi(DWORD TableId)
{
	PVOID pFirmwareTableBuffer = NULL;
	UINT BufferSize = 0;
	BufferSize = NT5GetSystemFirmwareTable('ACPI', TableId, NULL, 0);
	if (BufferSize == 0)
		return NULL;
	pFirmwareTableBuffer = malloc(BufferSize);
	if (!pFirmwareTableBuffer)
		return NULL;
	NT5GetSystemFirmwareTable('ACPI', TableId, pFirmwareTableBuffer, BufferSize);
	return pFirmwareTableBuffer;
}

UINT8
AcpiChecksum(void* base, UINT size)
{
	UINT8* ptr;
	UINT8 ret = 0;
	for (ptr = (UINT8*)base; ptr < ((UINT8*)base) + size;
		ptr++)
		ret += *ptr;
	return ret;
}

void TrimString(CHAR* String)
{
	CHAR* Pos1 = String;
	CHAR* Pos2 = String;
	size_t Len = strlen(String);

	while (Len > 0)
	{
		if (String[Len - 1] != ' ' && String[Len - 1] != '\t')
		{
			break;
		}
		String[Len - 1] = 0;
		Len--;
	}

	while (*Pos1 == ' ' || *Pos1 == '\t')
	{
		Pos1++;
	}

	while (*Pos1)
	{
		*Pos2++ = *Pos1++;
	}
	*Pos2++ = 0;

	return;
}

int GetRegDwordValue(HKEY Key, LPCSTR SubKey, LPCSTR ValueName, DWORD* pValue)
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
	else
	{
		return 1;
	}
}

CHAR* IDS = NULL;
DWORD IDS_SIZE = 0;

static CHAR*
IdsGetline(DWORD* Offset)
{
	CHAR* Line = NULL;
	DWORD i = 0, Len = 0;
	if (*Offset >= IDS_SIZE)
		return NULL;
	for (i = *Offset; i < IDS_SIZE; i++)
	{
		if (IDS[i] == '\n' || IDS[i] == '\r')
			break;
	}
	Len = i - *Offset;
	Line = malloc((SIZE_T)Len + 1);
	if (!Line)
		return NULL;
	memcpy(Line, IDS + *Offset, Len);
	Line[Len] = 0;
	*Offset += Len;
	for (i = *Offset; i < IDS_SIZE; i++, (*Offset)++)
	{
		if (IDS[i] != '\n' && IDS[i] != '\r')
			break;
	}
	return Line;
}

void
FindId(CONST CHAR* v, CONST CHAR* d, CONST CHAR* s, int usb)
{
	DWORD Offset = 0;
	CHAR* vLine = NULL;
	CHAR* dLine = NULL;
	CHAR* sLine = NULL;
	if (!v || !d)
		return;
	vLine = IdsGetline(&Offset);
	while (vLine)
	{
		if (!vLine[0] || vLine[0] == '#' || strlen(vLine) < 7)
		{
			free(vLine);
			vLine = IdsGetline(&Offset);
			continue;
		}
		if (_strnicmp(v, vLine, 4) != 0)
		{
			free(vLine);
			vLine = IdsGetline(&Offset);
			continue;
		}
		printf("  Vendor: %s\n", vLine + 6);
		free(vLine);
		dLine = IdsGetline(&Offset);
		while (dLine)
		{
			if (!dLine[0] || dLine[0] == '#')
			{
				free(dLine);
				dLine = IdsGetline(&Offset);
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
				dLine = IdsGetline(&Offset);
				continue;
			}
			printf("  Device: %s\n", dLine + 7);
			free(dLine);
			if (!s)
				break;
			sLine = IdsGetline(&Offset);
			while (sLine)
			{
				if (!sLine[0] || sLine[0] == '#')
				{
					free(sLine);
					sLine = IdsGetline(&Offset);
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
					sLine = IdsGetline(&Offset);
					continue;
				}
				printf("  %s: %s\n", usb? "Interface" : "Subsys", sLine + 13);
				free(sLine);
				break;
			}
			break;
		}
		break;
	}
}

const CHAR*
GuidToStr(UCHAR Guid[16]) {
	static CHAR GuidStr[37] = { 0 };
	snprintf(GuidStr, 37, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
		Guid[0], Guid[1], Guid[2], Guid[3], Guid[4], Guid[5], Guid[6], Guid[7],
		Guid[8], Guid[9], Guid[10], Guid[11], Guid[12], Guid[13], Guid[14], Guid[15]);
	return GuidStr;
}

const char*
nt5_inet_ntop4(const unsigned char* src, char* dst, size_t size)
{
	static const char* fmt = "%u.%u.%u.%u";
	char tmp[sizeof "255.255.255.255"];
	size_t len;

	len = snprintf(tmp, sizeof tmp, fmt, src[0], src[1], src[2], src[3]);
	if (len >= size) {
		errno = ENOSPC;
		return (NULL);
	}
	memcpy(dst, tmp, len + 1);

	return (dst);
}

#define NS_INT16SZ   2
#define NS_IN6ADDRSZ  16
const char*
nt5_inet_ntop6(const unsigned char* src, char* dst, size_t size)
{
	char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], * tp;
	struct { int base, len; } best, cur;
	unsigned int words[NS_IN6ADDRSZ / NS_INT16SZ];
	int i, inc;

	memset(words, '\0', sizeof words);
	for (i = 0; i < NS_IN6ADDRSZ; i++)
		words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
	best.base = -1;
	cur.base = -1;
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
		if (words[i] == 0) {
			if (cur.base == -1)
				cur.base = i, cur.len = 1;
			else
				cur.len++;
		}
		else {
			if (cur.base != -1) {
				if (best.base == -1 || cur.len > best.len)
					best = cur;
				cur.base = -1;
			}
		}
	}
	if (cur.base != -1) {
		if (best.base == -1 || cur.len > best.len)
			best = cur;
	}
	if (best.base != -1 && best.len < 2)
		best.base = -1;

	tp = tmp;
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
		if (best.base != -1 && i >= best.base &&
			i < (best.base + best.len)) {
			if (i == best.base)
				*tp++ = ':';
			continue;
		}

		if (i != 0)
			*tp++ = ':';
		if (i == 6 && best.base == 0 &&
			(best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
			if (!nt5_inet_ntop4(src + 12, tp, sizeof tmp - (tp - tmp)))
				return (NULL);
			tp += strlen(tp);
			break;
		}
		inc = snprintf(tp, 5, "%x", words[i]);
		tp += inc;
	}

	if (best.base != -1 && (best.base + best.len) ==
		(NS_IN6ADDRSZ / NS_INT16SZ))
		*tp++ = ':';
	*tp++ = '\0';

	if ((size_t)(tp - tmp) > size) {
		errno = ENOSPC;
		return (NULL);
	}
	memcpy(dst, tmp, tp - tmp);
	return (dst);
}

static UINT32 nt5_htonl(UINT32 x)
{
	UCHAR* s = (UCHAR*)&x;
	return (UINT32)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
}

void
NT5ConvertLengthToIpv4Mask(ULONG MaskLength, ULONG* Mask)
{
	if (MaskLength > 32UL)
		*Mask = INADDR_NONE;
	else if (MaskLength == 0)
		*Mask = 0;
	else
		*Mask = nt5_htonl(~0U << (32UL - MaskLength));
}

ULONGLONG
NT5GetTickCount(void)
{
	ULONGLONG(WINAPI * NT6GetTickCount64) (void) = NULL;
	HMODULE hMod = GetModuleHandleA("kernel32");

	if (hMod)
		*(FARPROC*)&NT6GetTickCount64 = GetProcAddress(hMod, "GetTickCount64");

	if (NT6GetTickCount64)
	{
		//printf("Use GetTickCount64\n");
		return NT6GetTickCount64();
	}
	else
	{
		//printf("Use GetTickCount\n");
		return (ULONGLONG)GetTickCount();
	}
}

UINT
NT5EnumSystemFirmwareTables(DWORD FirmwareTableProviderSignature, PVOID pFirmwareTableEnumBuffer, DWORD BufferSize)
{
	UINT (WINAPI *NT6EnumSystemFirmwareTables)
		(DWORD FirmwareTableProviderSignature, PVOID pFirmwareTableEnumBuffer, DWORD BufferSize) = NULL;
	HMODULE hMod = GetModuleHandleA("kernel32");

	if (hMod)
		*(FARPROC*)&NT6EnumSystemFirmwareTables = GetProcAddress(hMod, "EnumSystemFirmwareTables");

	if (NT6EnumSystemFirmwareTables)
		return NT6EnumSystemFirmwareTables(FirmwareTableProviderSignature, pFirmwareTableEnumBuffer, BufferSize);
	else
		return 0;
}

UINT
NT5GetSystemFirmwareTable(DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID,
	PVOID pFirmwareTableBuffer, DWORD BufferSize)
{
	UINT(WINAPI * NT6GetSystemFirmwareTable)
		(DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID, PVOID pFirmwareTableBuffer, DWORD BufferSize) = NULL;
	HMODULE hMod = GetModuleHandleA("kernel32");

	if (hMod)
		*(FARPROC*)&NT6GetSystemFirmwareTable = GetProcAddress(hMod, "GetSystemFirmwareTable");

	if (NT6GetSystemFirmwareTable)
		return NT6GetSystemFirmwareTable(FirmwareTableProviderSignature, FirmwareTableID, pFirmwareTableBuffer, BufferSize);
	else
		return 0;
}
