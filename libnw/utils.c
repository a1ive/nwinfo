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

HANDLE NWL_GetDiskHandleById(BOOL Cdrom, BOOL Write, DWORD Id)
{
	CHAR PhyPath[] = "\\\\.\\PhysicalDrive4294967295";
	if (Cdrom)
		snprintf(PhyPath, sizeof(PhyPath), "\\\\.\\CdRom%u", Id);
	else
		snprintf(PhyPath, sizeof(PhyPath), "\\\\.\\PhysicalDrive%u", Id);
	return CreateFileA(PhyPath, Write ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ,
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
	case BusTypeFileBackedVirtual: return "FileBackedVirtual";
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
