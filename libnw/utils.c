// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winioctl.h>
#include "libnw.h"
#include "utils.h"
#include "smbios.h"
#include "acpi.h"
#include <libcpuid.h>
#include <winring0.h>

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

DWORD NWL_ObtainPrivileges(LPWSTR privilege)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp = { 0 };
	BOOL res;
	// Obtain required privileges
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return GetLastError();
	res = LookupPrivilegeValueW(NULL, privilege, &tkp.Privileges[0].Luid);
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

BOOL
NWL_ReadMemory(PVOID buffer, DWORD_PTR address, DWORD length)
{
	if (!NWLC->NwDrv)
		return FALSE;
	if (address == 0) // Reject 0x0000
		return FALSE;
	if (phymem_read(NWLC->NwDrv, address, buffer, length, 1) == 0)
		return FALSE;
	return TRUE;
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
	if (!NWL_ReadMemory(bios, 0xf0000, 0x10000))
		goto fail;
	for (ptr = bios; ptr < bios + 0x10000; ptr += 16)
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
			NWL_ReadMemory(buf->Data, eps->intermediate.table_address, smbios_len);
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
			NWL_ReadMemory(buf->Data, (DWORD_PTR)eps3->table_address, smbios_len);
			goto fail;
		}
	}
fail:
	free(bios);
	return smbios_len + sizeof(struct RAW_SMBIOS_DATA);
}

UINT
NWL_GetSystemFirmwareTable(DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID,
	PVOID pFirmwareTableBuffer, DWORD BufferSize)
{
	UINT(WINAPI * NT6GetSystemFirmwareTable)
		(DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID, PVOID pFirmwareTableBuffer, DWORD BufferSize) = NULL;
	HMODULE hMod = GetModuleHandleW(L"kernel32");

	if (hMod)
		*(FARPROC*)&NT6GetSystemFirmwareTable = GetProcAddress(hMod, "GetSystemFirmwareTable");

	if (NT6GetSystemFirmwareTable)
		return NT6GetSystemFirmwareTable(FirmwareTableProviderSignature, FirmwareTableID, pFirmwareTableBuffer, BufferSize);

	if (FirmwareTableProviderSignature == 'RSMB')
		return NT5GetSmbios(pFirmwareTableBuffer, BufferSize);

	return 0;
}

struct RAW_SMBIOS_DATA*
NWL_GetSmbios(void)
{
	struct RAW_SMBIOS_DATA* smBiosData = NULL;
	DWORD smBiosDataSize = 0;
	smBiosDataSize = NWL_GetSystemFirmwareTable('RSMB', 0, NULL, 0);
	if (smBiosDataSize == 0)
		return NULL;
	smBiosData = (struct RAW_SMBIOS_DATA*)malloc(smBiosDataSize);
	if (!smBiosData)
		return NULL;
	smBiosDataSize = NWL_GetSystemFirmwareTable('RSMB', 0, smBiosData, smBiosDataSize);
	if (smBiosDataSize == 0)
	{
		free(smBiosData);
		return NULL;
	}
	return smBiosData;
}

static struct acpi_rsdp_v2*
NWL_GetRsdpHelper(struct acpi_rsdp_v2* ptr, DWORD_PTR addr)
{
	UINT32 len;
	struct acpi_rsdp_v2* ret = NULL;
	if (ptr->rsdpv1.revision == 0)
		len = sizeof(struct acpi_rsdp_v1);
	else if (ptr->length > sizeof(struct acpi_rsdp_v2))
		len = ptr->length;
	else
		len = sizeof(struct acpi_rsdp_v2);

	ret = calloc(1, len);
	if (!ret)
		return NULL;
	if (!NWL_ReadMemory(ret, addr, len))
	{
		free(ret);
		return NULL;
	}
	return ret;
}

struct acpi_rsdp_v2 *
NWL_GetRsdp(VOID)
{
	struct acpi_rsdp_v2* ret = NULL;
	UCHAR* ptr = NULL;
	UCHAR* bios = NULL;
	bios = malloc(0x20000);
	if (!bios)
		return NULL;
	// EBDA 0x080000 - 0x09FFFF, 0x10000
	if (!NWL_ReadMemory(bios, 0x80000, 0x10000))
		goto out;
	for (ptr = bios; ptr < bios + 0x10000; ptr += 16)
	{
		if (memcmp(ptr, RSDP_SIGNATURE, RSDP_SIGNATURE_SIZE) == 0
			&& NWL_AcpiChecksum(ptr, sizeof(struct acpi_rsdp_v1)) == 0)
		{
			ret = NWL_GetRsdpHelper((struct acpi_rsdp_v2*)ptr, 0x80000 + (ptr - bios));
			goto out;
		}
	}
	// BIOS 0x0E0000 - 0x100000, 0x20000
	if (!NWL_ReadMemory(bios, 0xE0000, 0x20000))
		goto out;
	for (ptr = bios; ptr < bios + 0x20000; ptr += 16)
	{
		if (memcmp(ptr, RSDP_SIGNATURE, RSDP_SIGNATURE_SIZE) == 0
			&& NWL_AcpiChecksum(ptr, sizeof(struct acpi_rsdp_v1)) == 0)
		{
			ret = NWL_GetRsdpHelper((struct acpi_rsdp_v2*)ptr, 0xE0000 + (ptr - bios));
			goto out;
		}
	}
out:
	free(bios);
	return ret;
}

static PVOID NWL_GetSysAcpi(DWORD TableId)
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

struct acpi_rsdt *
NWL_GetRsdt(VOID)
{
	struct acpi_rsdt tmp;
	struct acpi_rsdt* ret = NULL;
	if (!NWLC->NwRsdp)
		return NWL_GetSysAcpi('TDSR');
	if (!NWL_ReadMemory(&tmp, NWLC->NwRsdp->rsdpv1.rsdt_addr, sizeof(struct acpi_rsdt)))
		return NWL_GetSysAcpi('TDSR');
	if (tmp.header.length < sizeof(struct acpi_table_header))
		tmp.header.length = sizeof(struct acpi_table_header);
	ret = malloc(tmp.header.length);
	if (!ret)
		return NWL_GetSysAcpi('TDSR');
	if (!NWL_ReadMemory(ret, NWLC->NwRsdp->rsdpv1.rsdt_addr, tmp.header.length))
	{
		free(ret);
		return NULL;
	}
	return ret;
}

struct acpi_xsdt *
NWL_GetXsdt(VOID)
{
	struct acpi_xsdt tmp;
	struct acpi_xsdt* ret = NULL;
	if (!NWLC->NwRsdp)
		return NWL_GetSysAcpi('TDSX');
	if (NWLC->NwRsdp->rsdpv1.revision == 0) // v1
		return NULL;
	if (!NWL_ReadMemory(&tmp, (DWORD_PTR)NWLC->NwRsdp->xsdt_addr, sizeof(struct acpi_xsdt)))
		return NWL_GetSysAcpi('TDSX');
	if (tmp.header.length < sizeof(struct acpi_table_header))
		tmp.header.length = sizeof(struct acpi_table_header);
	ret = malloc(tmp.header.length);
	if (!ret)
		return NWL_GetSysAcpi('TDSX');
	if (!NWL_ReadMemory(ret, (DWORD_PTR)NWLC->NwRsdp->xsdt_addr, tmp.header.length))
	{
		free(ret);
		return NULL;
	}
	return ret;
}

PVOID NWL_GetAcpi(DWORD TableId)
{
	if (TableId == 'TDSR')
		return NWL_GetRsdt();
	if (TableId == 'TDSX')
		return NWL_GetXsdt();
	return NWL_GetSysAcpi(TableId);
}

PVOID NWL_GetAcpiByAddr(DWORD_PTR Addr)
{
	PVOID ret;
	struct acpi_table_header tmp;
	if (!Addr)
		return NULL;
	if (!NWL_ReadMemory(&tmp, Addr, sizeof(struct acpi_table_header)))
		return NULL;
	if (tmp.length < sizeof(struct acpi_table_header))
		tmp.length = sizeof(struct acpi_table_header);
	ret = malloc(tmp.length);
	if (!ret)
		return NULL;
	if (!NWL_ReadMemory(ret, Addr, tmp.length))
	{
		free(ret);
		return NULL;
	}
	return ret;
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

INT NWL_GetRegDwordValue(HKEY Key, LPCWSTR SubKey, LPCWSTR ValueName, DWORD* pValue)
{
	HKEY hKey;
	DWORD Type;
	DWORD Size;
	LSTATUS lRet;
	DWORD Value = 0;
	lRet = RegOpenKeyExW(Key, SubKey, 0, KEY_QUERY_VALUE, &hKey);
	if (ERROR_SUCCESS == lRet)
	{
		Size = sizeof(Value);
		lRet = RegQueryValueExW(hKey, ValueName, NULL, &Type, (LPBYTE)&Value, &Size);
		*pValue = Value;
		RegCloseKey(hKey);
		return 0;
	}
	return 1;
}

HANDLE NWL_GetDiskHandleById(BOOL Cdrom, BOOL Write, DWORD Id)
{
	WCHAR PhyPath[28]; // L"\\\\.\\PhysicalDrive4294967295"
	if (Cdrom)
		swprintf(PhyPath, 28, L"\\\\.\\CdRom%u", Id);
	else
		swprintf(PhyPath, 28, L"\\\\.\\PhysicalDrive%u", Id);
	return CreateFileW(PhyPath, Write ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, 0);
}

LPCSTR NWL_GetBusTypeString(STORAGE_BUS_TYPE Type)
{
	switch (Type)
	{
	case BusTypeUnknown: return "unknown";
	case BusTypeScsi: return "SCSI";
	case BusTypeAtapi: return "Atapi";
	case BusTypeAta: return "ATA";
	case BusType1394: return "1394";
	case BusTypeSsa: return "SSA";
	case BusTypeFibre: return "Fibre";
	case BusTypeUsb: return "USB";
	case BusTypeRAID: return "RAID";
	case BusTypeiScsi: return "iSCSI";
	case BusTypeSas: return "SAS";
	case BusTypeSata: return "SATA";
	case BusTypeSd: return "SD";
	case BusTypeMmc: return "MMC";
	case BusTypeVirtual: return "Virtual";
	case BusTypeFileBackedVirtual: return "File";
	case BusTypeSpaces: return "Spaces";
	case BusTypeNvme: return "NVMe";
	case BusTypeSCM: return "SCM";
	case BusTypeUfs: return "UFS";
	}
	return "unknown";
}

LPCSTR
NWL_GuidToStr(UCHAR Guid[16])
{
	static CHAR GuidStr[37] = { 0 };
	snprintf(GuidStr, 37, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
		Guid[0], Guid[1], Guid[2], Guid[3], Guid[4], Guid[5], Guid[6], Guid[7],
		Guid[8], Guid[9], Guid[10], Guid[11], Guid[12], Guid[13], Guid[14], Guid[15]);
	return GuidStr;
}

LPCSTR
NWL_WinGuidToStr(BOOL bBracket, GUID* pGuid)
{
	static CHAR GuidStr[39] = { 0 };
	snprintf(GuidStr, 39, "%s%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X%s",
		bBracket ? "{" : "",
		pGuid->Data1, pGuid->Data2, pGuid->Data3,
		pGuid->Data4[0], pGuid->Data4[1], pGuid->Data4[2], pGuid->Data4[3],
		pGuid->Data4[4], pGuid->Data4[5], pGuid->Data4[6], pGuid->Data4[7],
		bBracket ? "}" : "");
	return GuidStr;
}

BOOL
NWL_StrToGuid(const CHAR* cchText, GUID* pGuid)
{
	CHAR p[37];
	size_t len = strlen(cchText);
	memset(pGuid, 0, sizeof(GUID));

	if (len == 38 && cchText[0] == '{' && cchText[37] == '}')
		memcpy(p, cchText + 1, 36);
	else if (len == 36)
		memcpy(p, cchText, 36);
	else
		return FALSE;
	p[36] = '\0';
	if (p[8] != '-' || p[13] != '-' || p[18] != '-' || p[23] != '-')
		return FALSE;
	p[8] = 0;
	pGuid->Data1 = strtoul(p, NULL, 16);
	p[13] = 0;
	pGuid->Data2 = (unsigned short)strtoul(p + 9, NULL, 16);
	p[18] = 0;
	pGuid->Data3 = (unsigned short)strtoul(p + 14, NULL, 16);
	pGuid->Data4[7] = (unsigned char)strtoul(p + 34, NULL, 16);
	p[34] = 0;
	pGuid->Data4[6] = (unsigned char)strtoul(p + 32, NULL, 16);
	p[32] = 0;
	pGuid->Data4[5] = (unsigned char)strtoul(p + 30, NULL, 16);
	p[30] = 0;
	pGuid->Data4[4] = (unsigned char)strtoul(p + 28, NULL, 16);
	p[28] = 0;
	pGuid->Data4[3] = (unsigned char)strtoul(p + 26, NULL, 16);
	p[26] = 0;
	pGuid->Data4[2] = (unsigned char)strtoul(p + 24, NULL, 16);
	p[23] = 0;
	pGuid->Data4[1] = (unsigned char)strtoul(p + 21, NULL, 16);
	p[21] = 0;
	pGuid->Data4[0] = (unsigned char)strtoul(p + 19, NULL, 16);
	return TRUE;
}

struct NWL_MONITOR_CTX
{
	HMONITOR hMonitor;
	LPCWSTR lpDevice;
};

static BOOL CALLBACK EnumMonIter(HMONITOR hMonitor, HDC hDC, LPRECT lpRect, LPARAM lParam)
{
	struct NWL_MONITOR_CTX* ctx = (struct NWL_MONITOR_CTX*)lParam;
	MONITORINFOEXW mi = { .cbSize = sizeof(MONITORINFOEXW) };
	if (GetMonitorInfoW(hMonitor, (LPMONITORINFO)&mi) && mi.szDevice[0] != '\0')
	{
		if (wcscmp(mi.szDevice, ctx->lpDevice) == 0)
		{
			ctx->hMonitor = hMonitor;
			return FALSE;
		}
	}
	return TRUE;
}

HMONITOR
NWL_GetMonitorFromName(LPCWSTR lpDevice)
{
	struct NWL_MONITOR_CTX ctx = { 0 };
	EnumDisplayMonitors(NULL, NULL, EnumMonIter, (LPARAM)&ctx);
	return ctx.hMonitor;
}

#define SECPERMIN 60
#define SECPERHOUR (60*SECPERMIN)
#define SECPERDAY (24*SECPERHOUR)
#define DAYSPERYEAR 365
#define DAYSPER4YEARS (4*DAYSPERYEAR+1)

LPCSTR
NWL_UnixTimeToStr(INT nix)
{
	static CHAR buf[28];
	UINT8 months[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	INT daysEpoch;
	UINT32 days;
	UINT32 secsInDay;
	UINT16 year;
	UINT8 month;
	UINT8 hour;

	if (nix < 0)
		daysEpoch = -(((INT64)(SECPERDAY) - nix - 1) / SECPERDAY);
	else
		daysEpoch = nix / SECPERDAY;

	secsInDay = nix - daysEpoch * SECPERDAY;
	days = daysEpoch + 69 * DAYSPERYEAR + 17;
	year = 1901 + 4 * (days / DAYSPER4YEARS);
	days %= DAYSPER4YEARS;
	if (days / DAYSPERYEAR == 4)
	{
		year += 3;
		days -= 3 * DAYSPERYEAR;
	}
	else
	{
		year += days / DAYSPERYEAR;
		days %= DAYSPERYEAR;
	}
	for (month = 0; month < 12 && days >= (month == 1 && year % 4 == 0 ? 29U : months[month]); month++)
		days -= (month == 1 && year % 4 == 0 ? 29U : months[month]);
	hour = (secsInDay / SECPERHOUR);
	secsInDay %= SECPERHOUR;
	snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u (UTC)",
		year, month + 1, days + 1, hour, secsInDay / SECPERMIN, secsInDay % SECPERMIN);
	return buf;
}

static CHAR Utf8Buf[NWINFO_BUFSZ + 1];

LPCSTR
NWL_Ucs2ToUtf8(LPCWSTR src)
{
	size_t i;
	CHAR* p = Utf8Buf;
	ZeroMemory(Utf8Buf, sizeof(Utf8Buf));
	for (i = 0; i < NWINFO_BUFSZ / 3; i++)
	{
		if (src[i] == 0x0000)
			break;
		else if (src[i] <= 0x007F)
			*p++ = (CHAR)src[i];
		else if (src[i] <= 0x07FF)
		{
			*p++ = (src[i] >> 6) | 0xC0;
			*p++ = (src[i] & 0x3F) | 0x80;
		}
		else if (src[i] >= 0xD800 && src[i] <= 0xDFFF)
		{
			*p++ = 0;
			break;
		}
		else
		{
			*p++ = (src[i] >> 12) | 0xE0;
			*p++ = ((src[i] >> 6) & 0x3F) | 0x80;
			*p++ = (src[i] & 0x3F) | 0x80;
		}
	}
	return Utf8Buf;
}

static WCHAR Ucs2Buf[NWINFO_BUFSZ + 1];

LPCWSTR NWL_Utf8ToUcs2(LPCSTR src)
{
	size_t i;
	size_t j = 0;
	WCHAR *p = Ucs2Buf;
	ZeroMemory(Ucs2Buf, sizeof(Ucs2Buf));
	for (i = 0; src[i] != '\0' && j < NWINFO_BUFSZ; )
	{
		if ((src[i] & 0x80) == 0)
		{
			p[j++] = (WCHAR)src[i++];
		}
		else if ((src[i] & 0xE0) == 0xC0)
		{
			p[j++] = (WCHAR)((0x1FU & src[i]) << 6) | (0x3FU & src[i + 1]);
			i += 2;
		}
		else if ((src[i] & 0xF0) == 0xE0)
		{
			p[j++] = (WCHAR)((0x0FU & src[i]) << 12) | ((0x3FU & src[i + 1]) << 6) | (0x3FU & src[i + 2]);
			i += 3;
		}
		else
			break;
	}
	p[j] = L'\0';
	return Ucs2Buf;
}
