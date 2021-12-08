// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winioctl.h>
#include "nwinfo.h"

BOOL IsAdmin(void)
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

DWORD ObtainPrivileges(LPCTSTR privilege)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp = { 0 };
	BOOL res;
	// Obtain required privileges
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return GetLastError();
	res = LookupPrivilegeValue(NULL, privilege, &tkp.Privileges[0].Luid);
	if (!res)
		return GetLastError();
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
	return GetLastError();
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

CHAR* GetRegSzValue(HKEY Key, LPCSTR SubKey, LPCSTR ValueName)
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

void
FindClass(CONST CHAR* Class)
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
	vLine = IdsGetline(&Offset);
	while (vLine)
	{
		if (!vLine[0] || vLine[0] != 'C' || strlen(vLine) < 7 || vLine[1] != ' ')
		{
			free(vLine);
			vLine = IdsGetline(&Offset);
			continue;
		}
		if (_strnicmp(v, vLine + 2, 2) != 0)
		{
			free(vLine);
			vLine = IdsGetline(&Offset);
			continue;
		}
		printf("  Class: %s", vLine + 6);
		free(vLine);
		if (!d)
			goto out;
		dLine = IdsGetline(&Offset);
		while (dLine)
		{
			if (!dLine[0] || dLine[0] == '#')
			{
				free(dLine);
				dLine = IdsGetline(&Offset);
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
				dLine = IdsGetline(&Offset);
				continue;
			}
			printf(", %s", dLine + 5);
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
				if (sLine[0] != '\t' || !sLine[1] || sLine[1] != '\t' || strlen(sLine) < 7)
				{
					free(sLine);
					break;
				}
				if (_strnicmp(s, sLine + 2, 2) != 0)
				{
					free(sLine);
					sLine = IdsGetline(&Offset);
					continue;
				}
				printf(", %s", sLine + 6);
				free(sLine);
				break;
			}
			break;
		}
	out:
		printf("\n");
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

#ifndef _WIN64
static BOOL
NT5ReadMem(PVOID buffer, DWORD address, DWORD length)
{
	BOOL bRet = FALSE;
	if (!InitPhysicalMemory())
		return FALSE;
	if (ReadPhysicalMemory(buffer, address, length))
		bRet = TRUE;
	ExitPhysicalMemory();
	return bRet;
}

static UINT
NT5GetSmbios(struct RawSMBIOSData* buf, DWORD buflen)
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
		if (memcmp(ptr, "_SM_", 4) == 0 && AcpiChecksum(ptr, sizeof(struct smbios_eps)) == 0)
		{
			struct smbios_eps* eps = (struct smbios_eps*)ptr;
			smbios_len = eps->intermediate.table_length;
			if (!buf || buflen < smbios_len + sizeof(struct RawSMBIOSData))
				goto fail;
			buf->Length = smbios_len;
			buf->MajorVersion = eps->version_major;
			buf->MinorVersion = eps->version_minor;
			buf->DmiRevision = eps->intermediate.revision;
			NT5ReadMem(buf->Data, eps->intermediate.table_address, smbios_len);
			goto fail;
		}
		if (memcmp(ptr, "_SM3_", 5) == 0 && AcpiChecksum(ptr, sizeof(struct smbios_eps3)) == 0)
		{
			struct smbios_eps3* eps3 = (struct smbios_eps3*)ptr;
			smbios_len = eps3->maximum_table_length;
			if (!buf || buflen < smbios_len + sizeof(struct RawSMBIOSData))
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
	return smbios_len + sizeof(struct RawSMBIOSData);
}
#endif

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
#ifndef _WIN64
	if (FirmwareTableProviderSignature == 'RSMB')
		return NT5GetSmbios(pFirmwareTableBuffer, BufferSize);
#endif
	return 0;
}

DWORD
NT5GetFirmwareEnvironmentVariable(LPCSTR lpName, LPCSTR lpGuid,
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

static CHAR Mbs[256] = { 0 };

const CHAR*
NT5WcsToMbs(PWCHAR Wcs)
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

#ifdef NT5_COMPAT
UINT64
NT5GetTickCount(void)
{
	UINT64 (WINAPI * NT6GetTickCount64) (void) = NULL;
	UINT32(WINAPI * NT6GetTickCount) (void) = NULL;
	HMODULE hMod = GetModuleHandleA("kernel32");

	if (hMod) {
		*(FARPROC*)&NT6GetTickCount64 = GetProcAddress(hMod, "GetTickCount64");
		*(FARPROC*)&NT6GetTickCount = GetProcAddress(hMod, "GetTickCount");
	}

	if (NT6GetTickCount64)
		return NT6GetTickCount64();
	else if (NT6GetTickCount)
		return (UINT32)NT6GetTickCount();
	else
		return 0;
}

static const CHAR*
inet_ntop4(const UCHAR* src, CHAR* dst, size_t size)
{
	char tmp[] = "255.255.255.255";
	size_t len = 0;
	len = snprintf(tmp, sizeof(tmp), "%u.%u.%u.%u", src[0], src[1], src[2], src[3]);
	if (len >= size) {
		return NULL;
	}
	memcpy(dst, tmp, len + 1);
	return dst;
}

const CHAR*
NT5InetNtop(INT af, const void* src, CHAR* dst, size_t size)
{
	const CHAR* (FAR PASCAL *NT6InetNtop) (INT af, const void* src, CHAR * dst, size_t size) = NULL;
	HINSTANCE hL = LoadLibraryA("ws2_32.dll");
	if (hL)
		*(FARPROC*)&NT6InetNtop = GetProcAddress(hL, "inet_ntop");
	if (NT6InetNtop)
		return NT6InetNtop(af, src, dst, size);
	if (af == AF_INET)
		return inet_ntop4(src, dst, size);
	return NULL;
}
#endif
