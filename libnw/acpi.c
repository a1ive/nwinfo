// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"
#include "acpi.h"

#define ACPI_FIELD_CHK(Hdr, Type, Field) \
	((Hdr)->Length >= (offsetof(Type, Field) + sizeof(((Type *)0)->Field)))

LPCSTR GetAcpiTableDescription(UINT32 dwSignature)
{
	switch (dwSignature)
	{
	case ACPI_SIG('A', 'A', 'F', 'T'): return "ASRock ACPI Firmware Table";
	case ACPI_SIG('A', 'E', 'S', 'T'): return "Arm Error Source Table";
	case ACPI_SIG('A', 'G', 'D', 'I'): return "Arm Generic Diagnostic Dump and Reset Interface";
	case ACPI_SIG('A', 'P', 'I', 'C'): return "Multiple APIC Description Table";
	case ACPI_SIG('A', 'P', 'M', 'T'): return "Arm Performance Monitoring Unit Table";
	case ACPI_SIG('A', 'S', 'F', '!'): return "Alert Standard Format Table";
	case ACPI_SIG('A', 'S', 'P', 'T'): return "AMD Secure Processor Table";
	case ACPI_SIG('B', 'B', 'R', 'T'): return "Boot Background Resource Table";
	case ACPI_SIG('B', 'D', 'A', 'T'): return "BIOS Data Table";
	case ACPI_SIG('B', 'E', 'R', 'T'): return "Boot Error Record Table";
	case ACPI_SIG('B', 'G', 'R', 'T'): return "Boot Graphics Resource Table";
	case ACPI_SIG('B', 'O', 'O', 'T'): return "Simple Boot Flag Table";
	case ACPI_SIG('C', 'C', 'E', 'L'): return "CC-Event-Log";
	case ACPI_SIG('C', 'D', 'I', 'T'): return "Component Distance Information Table";
	case ACPI_SIG('C', 'E', 'D', 'T'): return "CXL Early Discovery Table";
	case ACPI_SIG('C', 'P', 'E', 'P'): return "Corrected Platform Error Polling Table";
	case ACPI_SIG('C', 'R', 'A', 'T'): return "Component Resource Attribute Table";
	case ACPI_SIG('C', 'S', 'R', 'T'): return "Core System Resource Table";
	case ACPI_SIG('D', 'B', 'G', 'P'): return "Debug Port Table";
	case ACPI_SIG('D', 'B', 'G', '2'): return "Debug Port Table Version 2";
	case ACPI_SIG('D', 'M', 'A', 'R'): return "DMA Remapping Table";
	case ACPI_SIG('D', 'R', 'T', 'M'): return "Dynamic Root of Trust for Measurement Table";
	case ACPI_SIG('D', 'S', 'D', 'T'): return "Differentiated System Description Table";
	case ACPI_SIG('D', 'T', 'P', 'R'): return "DMA TXT Protected Range";
	case ACPI_SIG('E', 'C', 'D', 'T'): return "Embedded Controller Boot Resources Table";
	case ACPI_SIG('E', 'I', 'N', 'J'): return "Error Injection Table";
	case ACPI_SIG('E', 'R', 'D', 'T'): return "Enhanced Resource Director Technology";
	case ACPI_SIG('E', 'R', 'S', 'T'): return "Error Record Serialization Table";
	case ACPI_SIG('E', 'T', 'D', 'T'): return "Event Timer Description Table";
	case ACPI_SIG('F', 'A', 'C', 'P'): return "Fixed ACPI Description Table";
	case ACPI_SIG('F', 'I', 'D', 'T'): return "Firmware ID Table";
	case ACPI_SIG('F', 'P', 'D', 'T'): return "Firmware Performance Data Table";
	case ACPI_SIG('G', 'S', 'C', 'I'): return "GMCH SCI Table";
	case ACPI_SIG('G', 'T', 'D', 'T'): return "Generic Timer Description Table";
	case ACPI_SIG('H', 'E', 'S', 'T'): return "Hardware Error Source Table";
	case ACPI_SIG('H', 'M', 'A', 'T'): return "Heterogeneous Memory Attribute Table";
	case ACPI_SIG('H', 'P', 'E', 'T'): return "High Precision Event Timer Table";
	case ACPI_SIG('I', 'B', 'F', 'T'): return "iSCSI Boot Firmware Table";
	case ACPI_SIG('I', 'E', 'R', 'S'): return "Inline Encryption Reporting Structure";
	case ACPI_SIG('I', 'O', 'R', 'T'): return "I/O Remapping Table";
	case ACPI_SIG('I', 'V', 'R', 'S'): return "I/O Virtualization Reporting Structure";
	case ACPI_SIG('K', 'E', 'Y', 'P'): return "Key Programming Interface for Root Complex Integrity and Data Encryption (IDE)";
	case ACPI_SIG('L', 'P', 'I', 'T'): return "Low Power Idle Table";
	case ACPI_SIG('M', 'A', 'T', 'R'): return "Memory Address Translation Table";
	case ACPI_SIG('M', 'C', 'F', 'G'): return "PCIe Memory Mapped Configuration Table";
	case ACPI_SIG('M', 'C', 'H', 'I'): return "Management Controller Host Interface Table";
	case ACPI_SIG('M', 'H', 'S', 'P'): return "Microsoft Pluton Security Processor Table";
	case ACPI_SIG('M', 'I', 'S', 'C'): return "Miscellaneous GUIDed Table Entries";
	case ACPI_SIG('M', 'P', 'A', 'M'): return "Arm Memory Partitioning And Monitoring";
	case ACPI_SIG('M', 'P', 'S', 'T'): return "Memory Power State Table";
	case ACPI_SIG('M', 'R', 'R', 'M'): return "Memory Range and Region Mapping Table";
	case ACPI_SIG('M', 'S', 'C', 'T'): return "Maximum System Characteristics Table";
	case ACPI_SIG('M', 'S', 'D', 'M'): return "Microsoft Data Management Table";
	case ACPI_SIG('N', 'B', 'F', 'T'): return "NVMe-oF Boot Firmware Table";
	case ACPI_SIG('N', 'F', 'I', 'T'): return "NVDIMM Firmware Interface Table";
	case ACPI_SIG('N', 'H', 'L', 'T'): return "Non HD Audio Link Table";
	case ACPI_SIG('O', 'S', 'D', 'T'): return "Override System Description Table";
	case ACPI_SIG('P', 'C', 'C', 'T'): return "Platform Communications Channel Table";
	case ACPI_SIG('P', 'D', 'T', 'T'): return "Platform Debug Trigger Table";
	case ACPI_SIG('P', 'H', 'A', 'T'): return "Platform Health Assessment Table";
	case ACPI_SIG('P', 'M', 'T', 'T'): return "Platform Memory Topology Table";
	case ACPI_SIG('P', 'O', 'A', 'T'): return "Un-Initialized Phoenix OEM activation Table";
	case ACPI_SIG('P', 'P', 'T', 'T'): return "Processor Properties Topology Table";
	case ACPI_SIG('P', 'R', 'M', 'T'): return "Platform Runtime Mechanism Table";
	case ACPI_SIG('P', 'S', 'D', 'T'): return "Persistent System Description Table";
	case ACPI_SIG('P', 'T', 'D', 'T'): return "Platform Telemetry Data Table";
	case ACPI_SIG('R', 'A', 'S', '2'): return "RAS2 Feature Table";
	case ACPI_SIG('R', 'A', 'S', 'F'): return "RAS Feature Table";
	case ACPI_SIG('R', 'G', 'R', 'T'): return "Regulatory Graphics Resource Table";
	case ACPI_SIG('R', 'H', 'C', 'T'): return "RISC-V Hart Capabilities Table";
	case ACPI_SIG('R', 'I', 'M', 'T'): return "RISC-V IO Mapping Table";
	case ACPI_SIG('R', 'S', 'D', 'T'): return "Root System Description Table";
	case ACPI_SIG('S', 'B', 'S', 'T'): return "Smart Battery Specification Table";
	case ACPI_SIG('S', 'D', 'E', 'I'): return "Software Delegated Exceptions Interface";
	case ACPI_SIG('S', 'D', 'E', 'V'): return "Secure Devices Table";
	case ACPI_SIG('S', 'L', 'I', 'C'): return "Microsoft Software Licensing Table";
	case ACPI_SIG('S', 'L', 'I', 'T'): return "System Locality Distance Information Table";
	case ACPI_SIG('S', 'P', 'C', 'R'): return "Microsoft Serial Port Console Redirection Table";
	case ACPI_SIG('S', 'P', 'M', 'I'): return "Service Processor Management Interface Table";
	case ACPI_SIG('S', 'R', 'A', 'T'): return "System Resource Affinity Table";
	case ACPI_SIG('S', 'S', 'D', 'T'): return "Secondary System Description Table";
	case ACPI_SIG('S', 'T', 'A', 'O'): return "_STA Override Table";
	case ACPI_SIG('S', 'V', 'K', 'L'): return "Storage Volume Key Data Table";
	case ACPI_SIG('S', 'W', 'F', 'T'): return "Sound Wire File Table";
	case ACPI_SIG('T', 'C', 'P', 'A'): return "TCG Hardware Interface Table (TPM 1.2)";
	case ACPI_SIG('T', 'D', 'E', 'L'): return "TD Event Log Table";
	case ACPI_SIG('T', 'P', 'M', '2'): return "Trusted Platform Module 2 Table";
	case ACPI_SIG('U', 'E', 'F', 'I'): return "UEFI ACPI Data Table";
	case ACPI_SIG('V', 'F', 'C', 'T'): return "AMD Video BIOS Firmware Content Table";
	case ACPI_SIG('V', 'I', 'O', 'T'): return "Virtual I/O Translation Table";
	case ACPI_SIG('V', 'T', 'O', 'Y'): return "Ventoy OS Param";
	case ACPI_SIG('W', 'A', 'E', 'T'): return "Windows ACPI Emulated Devices Table";
	case ACPI_SIG('W', 'D', 'A', 'T'): return "Microsoft Hardware Watch Dog Action Table";
	case ACPI_SIG('W', 'D', 'D', 'T'): return "Intel Watchdog Descriptor Table";
	case ACPI_SIG('W', 'D', 'R', 'T'): return "Windows 2k3 Watchdog Resource Table";
	case ACPI_SIG('W', 'P', 'B', 'T'): return "Windows Platform Binary Table";
	case ACPI_SIG('W', 'S', 'M', 'T'): return "Windows SMM Security Mitigations Table";
	case ACPI_SIG('X', 'E', 'N', 'V'): return "Xen Environment Table";
	case ACPI_SIG('X', 'S', 'D', 'T'): return "Extended System Description Table";
	}
	if ((dwSignature & 0x00FFFFFF) == ACPI_SIG('O', 'E', 'M', '\0'))
		return "OEM Specific Information Table";
	return "Unknown ACPI Table";
}

static void
PrintU8Str(PNODE pNode, LPCSTR Key, UINT8 *Str, DWORD Len)
{
	if (Len >= NWINFO_BUFSZ)
		Len = NWINFO_BUFSZ - 1;
	memcpy(NWLC->NwBuf, Str, Len);
	NWLC->NwBuf[Len] = 0;
	NWL_NodeAttrSet(pNode, Key, NWLC->NwBuf, 0);
}

static LPCSTR
AddrSpaceIdToStr(UINT8 SpaceId)
{
	switch (SpaceId)
	{
	case 0x00: return "MEM";
	case 0x01: return "IO";
	case 0x02: return "CFG";
	case 0x03: return "EC";
	case 0x04: return "SMBUS";
	case 0x05: return "CMOS";
	case 0x06: return "BAR";
	case 0x07: return "IPMI";
	case 0x08: return "GPIO";
	case 0x09: return "SER";
	case 0x0A: return "PCC";
	case 0x7F: return "FFH";
	}
	if (SpaceId >= 0xC0 && SpaceId <= 0xFF)
		return "OEM";
	return "RSVD";
}

static LPCSTR
AddrAccessSizeToStr(UINT8 AccessSize)
{
	switch (AccessSize)
	{
	case 0x00: return "UNDEF";
	case 0x01: return "BYTE";
	case 0x02: return "WORD";
	case 0x03: return "DWORD";
	case 0x04: return "QWORD";
	}
	return "RSVD";
}

static void
PrintGenAddr(PNODE pNode, LPCSTR Key, GEN_ADDR* pAddr)
{
	switch (pAddr->AddressSpaceId)
	{
	case 2: // PCI Config Space
	{
		UINT16 device = (UINT16)((pAddr->Address >> 32) & 0xFFFF);
		UINT16 function = (UINT16)((pAddr->Address >> 16) & 0xFFFF);
		UINT16 offset = (UINT16)(pAddr->Address & 0xFFFF);
		NWL_NodeAttrSetf(pNode, Key, 0, "CFG@S0.B0.D%u.F%u+%04Xh", device, function, offset);
		break;
	}
	case 6: // PCI BAR Target
	{
		UINT8 segment = (UINT8)((pAddr->Address >> 56) & 0xFF);
		UINT8 bus = (UINT8)((pAddr->Address >> 48) & 0xFF);
		UINT8 device = (UINT8)((pAddr->Address >> 43) & 0x1F);
		UINT8 function = (UINT8)((pAddr->Address >> 40) & 0x07);
		UINT8 bar_index = (UINT8)((pAddr->Address >> 37) & 0x07);
		UINT64 bar_offset = (pAddr->Address & 0x1FFFFFFFFFFULL) * 4;
		NWL_NodeAttrSetf(pNode, Key, 0, "BAR@S%u.B%u.D%u.F%u-BAR%u+%llXh",
			segment, bus, device, function, bar_index, bar_offset);
		break;
	}
	default:
	{
		NWL_NodeAttrSetf(pNode, Key, 0, "%s@0x%llx,%u,0x%02x,%s",
			AddrSpaceIdToStr(pAddr->AddressSpaceId),
			(unsigned long long)pAddr->Address,
			pAddr->RegisterBitWidth,
			pAddr->RegisterBitOffset,
			AddrAccessSizeToStr(pAddr->AccessSize));
		break;
	}
	}
}

static PNODE PrintTableHeader(PNODE pNode, DESC_HEADER* Hdr)
{
	const UINT32 dwSignature = ACPI_SIG(Hdr->Signature[0], Hdr->Signature[1], Hdr->Signature[2], Hdr->Signature[3]);
	PNODE tab = NWL_NodeAppendNew(pNode, "Table", NFLG_TABLE_ROW);

	PrintU8Str(tab, "Signature", Hdr->Signature, 4);
	NWL_NodeAttrSet(tab, "Description", GetAcpiTableDescription(dwSignature), 0);
	NWL_NodeAttrSetf(tab, "Revision", 0, "0x%02x", Hdr->Revision);
	NWL_NodeAttrSetf(tab, "Length", NAFLG_FMT_NUMERIC, "%u", Hdr->Length);
	NWL_NodeAttrSetf(tab, "Checksum", 0, "0x%02x", Hdr->Checksum);
	NWL_NodeAttrSet(tab, "Checksum Status", NWL_AcpiChecksum(Hdr, Hdr->Length) == 0 ? "OK" : "ERR", 0);
	PrintU8Str(tab, "OEM ID", Hdr->OemId, 6);
	PrintU8Str(tab, "OEM Table ID", Hdr->OemTableId, 8);
	NWL_NodeAttrSetf(tab, "OEM Revision", 0, "0x%lx", Hdr->OemRevision);
	PrintU8Str(tab, "Creator ID", Hdr->CreatorId, 4);
	NWL_NodeAttrSetf(tab, "Creator Revision", 0, "0x%x", Hdr->CreatorRevision);
	NWL_NodeAttrSetRaw(tab, "Binary Data", Hdr, (size_t)Hdr->Length);
	return tab;
}

static PNODE
PrintXSDT(PNODE pNode, DESC_HEADER* Hdr)
{
	ACPI_XSDT* xsdt = (ACPI_XSDT*)Hdr;
	UINT32 count = (xsdt->Header.Length - sizeof(DESC_HEADER)) / sizeof(xsdt->Entry[0]);
	PNODE entries = NWL_NodeAppendNew(pNode, "Entries", NFLG_TABLE);
	for (UINT32 i = 0; i < count; i++)
	{
		PNODE entry;
		CHAR name[5];
		DESC_HEADER* t = NWL_GetAcpiByAddr((DWORD_PTR)xsdt->Entry[i]);
		if (!t)
			continue;
		snprintf(name, 5, "%c%c%c%c",
			t->Signature[0], t->Signature[1], t->Signature[2], t->Signature[3]);
		entry = NWL_NodeAppendNew(entries, name, NFLG_TABLE_ROW);
		NWL_NodeAttrSetf(entry, "Address", 0, "0x%016llx", xsdt->Entry[i]);
		free(t);
	}
	return pNode;
}

static PNODE
PrintRSDT(PNODE pNode, DESC_HEADER* Hdr)
{
	ACPI_RSDT* rsdt = (ACPI_RSDT*)Hdr;
	UINT32 count = (rsdt->Header.Length - sizeof(DESC_HEADER)) / sizeof(rsdt->Entry[0]);
	PNODE entries = NWL_NodeAppendNew(pNode, "Entries", NFLG_TABLE);
	for (UINT32 i = 0; i < count; i++)
	{
		PNODE entry;
		CHAR name[5];
		DESC_HEADER* t = NWL_GetAcpiByAddr((DWORD_PTR)rsdt->Entry[i]);
		if (!t)
			continue;
		snprintf(name, 5, "%c%c%c%c",
			t->Signature[0], t->Signature[1], t->Signature[2], t->Signature[3]);
		entry = NWL_NodeAppendNew(entries, name, NFLG_TABLE_ROW);
		NWL_NodeAttrSetf(entry, "Address", 0, "0x%08x", rsdt->Entry[i]);
		free(t);
	}
	return pNode;
}

static PNODE
PrintRSDP(PNODE pNode, ACPI_RSDP_V2* rsdp)
{
	if (NWLC->AcpiTable &&
		NWLC->AcpiTable != ACPI_SIG('R', 'S', 'D', 'P'))
		return NULL;
	PNODE tab = NWL_NodeAppendNew(pNode, "Table", NFLG_TABLE_ROW);
	PrintU8Str(tab, "Signature", rsdp->RsdpV1.Signature, RSDP_SIGNATURE_SIZE);
	NWL_NodeAttrSet(tab, "Description", "Root System Description Pointer", 0);
	NWL_NodeAttrSetf(tab, "V1 Checksum", 0, "0x%02x", rsdp->Checksum);
	NWL_NodeAttrSet(tab, "V1 Checksum Status",
		NWL_AcpiChecksum(&rsdp->RsdpV1, sizeof(ACPI_RSDP_V1)) == 0 ? "OK" : "ERR", 0);
	PrintU8Str(tab, "OEM ID", rsdp->RsdpV1.OemId, 6);
	NWL_NodeAttrSetf(tab, "Revision", 0, "0x%02x", rsdp->RsdpV1.Revision);
	NWL_NodeAttrSetf(tab, "RSDT Address", 0, "0x%08x", rsdp->RsdpV1.RsdtAddr);
	if (rsdp->RsdpV1.Revision == 0)
		return tab;

	NWL_NodeAttrSetf(tab, "Length", NAFLG_FMT_NUMERIC, "%u", rsdp->Length);
	NWL_NodeAttrSetf(tab, "Checksum", 0, "0x%02x", rsdp->Checksum);
	NWL_NodeAttrSet(tab, "Checksum Status",
		NWL_AcpiChecksum(rsdp, rsdp->Length) == 0 ? "OK" : "ERR", 0);
	NWL_NodeAttrSetf(tab, "XSDT Address", 0, "0x%016llx", rsdp->XsdtAddr);
	return tab;
}

static PNODE
PrintFACS(PNODE pNode)
{
	if (NWLC->AcpiTable &&
		NWLC->AcpiTable != ACPI_SIG('F', 'A', 'C', 'S'))
		return NULL;
	DWORD dwSize, dwType;
	ACPI_FACS* facs = NWL_NtGetRegValue(HKEY_LOCAL_MACHINE,
		L"HARDWARE\\ACPI\\FACS", L"00000000", &dwSize, &dwType);
	if (!facs)
		return NULL;
	if (dwType != REG_BINARY || dwSize < offsetof(ACPI_FACS, HwSignature)
		|| !ACPI_FIELD_CHK(facs, ACPI_FACS, OspmFlags))
	{
		free(facs);
		return NULL;
	}
	PNODE tab = NWL_NodeAppendNew(pNode, "Table", NFLG_TABLE_ROW);
	PrintU8Str(tab, "Signature", facs->Signature, 4);
	NWL_NodeAttrSet(tab, "Description", "Firmware ACPI Control Structure", 0);
	NWL_NodeAttrSetf(tab, "Length", NAFLG_FMT_NUMERIC, "%u", facs->Length);
	NWL_NodeAttrSetRaw(tab, "Binary Data", facs, (size_t)facs->Length);
	NWL_NodeAttrSetf(tab, "Hardware Signature", 0, "0x%08X", facs->HwSignature);
	NWL_NodeAttrSetf(tab, "Firmware Waking Vector", 0, "0x%08X", facs->FwWakingVector);
	NWL_NodeAttrSetf(tab, "Global Lock", 0, "0x%08X", facs->GlobalLock);
	NWL_NodeAttrSetBool(tab, "S4BIOS Support", (facs->Flags & 0x01), 0);
	NWL_NodeAttrSetBool(tab, "64-bit Wake Support", (facs->Flags & 0x02), 0);
	NWL_NodeAttrSetf(tab, "X Firmware Waking Vector", 0, "0x%016llx", facs->XFwWakingVector);
	NWL_NodeAttrSetf(tab, "FACS Version", 0, "0x%02X", facs->Version);
	NWL_NodeAttrSetBool(tab, "OSPM 64-bit Wake", (facs->OspmFlags & 0x01), 0);
	return tab;
}

static PNODE
PrintBGRT(PNODE pNode, DESC_HEADER* Hdr)
{
	ACPI_BGRT* bgrt = (ACPI_BGRT*)Hdr;
	if (!ACPI_FIELD_CHK(Hdr, ACPI_BGRT, ImageOffsetY))
		return pNode;
	NWL_NodeAttrSetf(pNode, "BGRT Version", NAFLG_FMT_NUMERIC, "%u", bgrt->Version);
	NWL_NodeAttrSet(pNode, "BGRT Status", (bgrt->Status & 0x01) ? "Valid" : "Invalid", 0);
	NWL_NodeAttrSet(pNode, "Image Type", (bgrt->ImageType == 0) ? "Bitmap" : "Reserved", 0);
	NWL_NodeAttrSetf(pNode, "Address", 0, "0x%llx", bgrt->ImageAddress);
	NWL_NodeAttrSetf(pNode, "Offset X", NAFLG_FMT_NUMERIC, "%u", bgrt->ImageOffsetX);
	NWL_NodeAttrSetf(pNode, "Offset Y", NAFLG_FMT_NUMERIC, "%u", bgrt->ImageOffsetY);
	return pNode;
}

static const CHAR*
PmProfileToStr(UINT8 profile)
{
	switch (profile)
	{
	case 0: return "Unspecified";
	case 1: return "Desktop";
	case 2: return "Mobile";
	case 3: return "Workstation";
	case 4: return "Enterprise Server";
	case 5: return "SOHO Server";
	case 6: return "Aplliance PC";
	case 7: return "Performance Server";
	case 8: return "Tablet";
	}
	return "Reserved";
}

static PNODE
PrintFADT(PNODE pNode, DESC_HEADER* Hdr)
{
	ACPI_FADT* fadt = (ACPI_FADT*)Hdr;
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, FwCtrl))
		NWL_NodeAttrSetf(pNode, "Firmware Control", 0, "0x%08X", fadt->FwCtrl);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, DsdtAddr))
		NWL_NodeAttrSetf(pNode, "DSDT Address", 0, "0x%08X", fadt->DsdtAddr);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, PreferredPmProfile))
		NWL_NodeAttrSet(pNode, "PM Profile", PmProfileToStr(fadt->PreferredPmProfile), 0);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, SciInt))
		NWL_NodeAttrSetf(pNode, "SCI Interrupt Vector", 0, "0x%04X", fadt->SciInt);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, SmiCmd))
		NWL_NodeAttrSetf(pNode, "SMI Command Port", 0, "0x%08X", fadt->SmiCmd);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, AcpiEnable))
		NWL_NodeAttrSetf(pNode, "ACPI Enable", 0, "0x%02X", fadt->AcpiEnable);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, AcpiDisable))
		NWL_NodeAttrSetf(pNode, "ACPI Disable", 0, "0x%02X", fadt->AcpiDisable);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, S4BiosReq))
		NWL_NodeAttrSetf(pNode, "S4 Request", 0, "0x%02X", fadt->S4BiosReq);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, PstateCnt))
		NWL_NodeAttrSetf(pNode, "PState Control", 0, "0x%02X", fadt->PstateCnt);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, Pm1aEvtBlk))
		NWL_NodeAttrSetf(pNode, "PM1a Event", 0, "0x%08X", fadt->Pm1aEvtBlk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, Pm1bEvtBlk))
		NWL_NodeAttrSetf(pNode, "PM1b Event", 0, "0x%08X", fadt->Pm1bEvtBlk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, Pm1aCntBlk))
		NWL_NodeAttrSetf(pNode, "PM1a Control", 0, "0x%08X", fadt->Pm1aCntBlk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, Pm1bCntBlk))
		NWL_NodeAttrSetf(pNode, "PM1b Control", 0, "0x%08X", fadt->Pm1bCntBlk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, Pm2CntBlk))
		NWL_NodeAttrSetf(pNode, "PM2 Control", 0, "0x%08X", fadt->Pm2CntBlk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, PmTmrBlk))
		NWL_NodeAttrSetf(pNode, "PM Timer", 0, "0x%08X", fadt->PmTmrBlk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, Gpe0Blk))
		NWL_NodeAttrSetf(pNode, "GPE0", 0, "0x%08X", fadt->Gpe0Blk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, Gpe1Blk))
		NWL_NodeAttrSetf(pNode, "GPE1", 0, "0x%08X", fadt->Gpe1Blk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, Pm1EvtLen))
		NWL_NodeAttrSetf(pNode, "PM1 Event Length", 0, "0x%02X", fadt->Pm1EvtLen);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, Pm1CntLen))
		NWL_NodeAttrSetf(pNode, "PM1 Control Length", 0, "0x%02X", fadt->Pm1CntLen);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, Pm2CntLen))
		NWL_NodeAttrSetf(pNode, "PM2 Control Length", 0, "0x%02X", fadt->Pm2CntLen);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, PmTmrLen))
		NWL_NodeAttrSetf(pNode, "PM Timer Length", 0, "0x%02X", fadt->PmTmrLen);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, Gpe0BlkLen))
		NWL_NodeAttrSetf(pNode, "GPE0 Block Length", 0, "0x%02X", fadt->Gpe0BlkLen);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, Gpe1BlkLen))
		NWL_NodeAttrSetf(pNode, "GPE1 Block Length", 0, "0x%02X", fadt->Gpe1BlkLen);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, Gpe1Base))
		NWL_NodeAttrSetf(pNode, "GPE1 Base", 0, "0x%02X", fadt->Gpe1Base);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, CstCnt))
		NWL_NodeAttrSetf(pNode, "CST Control", 0, "0x%02X", fadt->CstCnt);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, PLvl2Lat))
		NWL_NodeAttrSetf(pNode, "P-LVL2 Latency", 0, "0x%04X", fadt->PLvl2Lat);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, PLvl3Lat))
		NWL_NodeAttrSetf(pNode, "P-LVL3 Latency", 0, "0x%04X", fadt->PLvl3Lat);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, FlushSize))
		NWL_NodeAttrSetf(pNode, "Flush Size", 0, "0x%04X", fadt->FlushSize);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, FlushStride))
		NWL_NodeAttrSetf(pNode, "Flush Stride", 0, "0x%04X", fadt->FlushStride);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, DutyOffset))
		NWL_NodeAttrSetf(pNode, "Duty Offset", 0, "0x%02X", fadt->DutyOffset);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, DutyWidth))
		NWL_NodeAttrSetf(pNode, "Duty Width", 0, "0x%02X", fadt->DutyWidth);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, DayAlrm))
		NWL_NodeAttrSetf(pNode, "Day Alarm", 0, "0x%02X", fadt->DayAlrm);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, MonAlrm))
		NWL_NodeAttrSetf(pNode, "Month Alarm", 0, "0x%02X", fadt->MonAlrm);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, Century))
		NWL_NodeAttrSetf(pNode, "Century", 0, "0x%02X", fadt->Century);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, IapcBootArch))
		NWL_NodeAttrSetf(pNode, "IA-PC Boot Architecture Flags", 0, "0x%04X", fadt->IapcBootArch);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, Flags))
		NWL_NodeAttrSetf(pNode, "Flags", 0, "0x%08X", fadt->Flags);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, ResetReg))
		PrintGenAddr(pNode, "Reset Register", &fadt->ResetReg);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, ResetValue))
		NWL_NodeAttrSetf(pNode, "Reset Value", 0, "0x%02X", fadt->ResetValue);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, ArmBootArch))
		NWL_NodeAttrSetf(pNode, "ARM Boot Architecture Flags", 0, "0x%04X", fadt->ArmBootArch);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, MinorVersion))
		NWL_NodeAttrSetf(pNode, "FADT Minor Version", 0, "0x%02X", fadt->MinorVersion);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, XFwCtrl))
		NWL_NodeAttrSetf(pNode, "X Firmware Control", 0, "0x%016llX", fadt->XFwCtrl);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, XDsdt))
		NWL_NodeAttrSetf(pNode, "X DSDT Address", 0, "0x%016llX", fadt->XDsdt);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, XPm1aEvtBlk))
		PrintGenAddr(pNode, "X PM1a Event", &fadt->XPm1aEvtBlk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, XPm1bEvtBlk))
		PrintGenAddr(pNode, "X PM1b Event", &fadt->XPm1bEvtBlk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, XPm1aCntBlk))
		PrintGenAddr(pNode, "X PM1a Control", &fadt->XPm1aCntBlk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, XPm1bCntBlk))
		PrintGenAddr(pNode, "X PM1b Control", &fadt->XPm1bCntBlk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, XPm2CntBlk))
		PrintGenAddr(pNode, "X PM2 Control", &fadt->XPm2CntBlk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, XPmTmrBlk))
		PrintGenAddr(pNode, "X PM Timer", &fadt->XPmTmrBlk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, XGpe0Blk))
		PrintGenAddr(pNode, "X GPE0", &fadt->XGpe0Blk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, XGpe1Blk))
		PrintGenAddr(pNode, "X GPE1", &fadt->XGpe1Blk);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, SleepControlReg))
		PrintGenAddr(pNode, "Sleep Control Register", &fadt->SleepControlReg);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, SleepStatusReg))
		PrintGenAddr(pNode, "Sleep Status Register", &fadt->SleepStatusReg);
	if (ACPI_FIELD_CHK(Hdr, ACPI_FADT, HypervisorVendorId))
		NWL_NodeAttrSetf(pNode, "Hypervisor Vendor ID", 0, "0x%016llX", fadt->HypervisorVendorId);
	return pNode;
}

static void
PrintInterruptFlags(PNODE node, UINT16 flags)
{
	const CHAR* polarity;
	const CHAR* trigger;

	// Decode Polarity (Bits 1:0)
	switch (flags & 0x03)
	{
	case 0: polarity = "Conforms to bus spec"; break;
	case 1: polarity = "Active-high"; break;
	case 3: polarity = "Active-low"; break;
	default: polarity = "Reserved"; break;
	}

	// Decode Trigger Mode (Bits 3:2)
	switch ((flags >> 2) & 0x03)
	{
	case 0: trigger = "Conforms to bus spec"; break;
	case 1: trigger = "Edge-triggered"; break;
	case 3: trigger = "Level-triggered"; break;
	default: trigger = "Reserved"; break;
	}

	NWL_NodeAttrSet(node, "Polarity", polarity, 0);
	NWL_NodeAttrSet(node, "Trigger Mode", trigger, 0);
}

static PNODE
PrintMADT(PNODE pNode, DESC_HEADER* Hdr)
{
	ACPI_MADT* madt = (ACPI_MADT*)Hdr;
	if (!ACPI_FIELD_CHK(Hdr, ACPI_MADT, Flags))
		return pNode;
	NWL_NodeAttrSetf(pNode, "Local APIC Address", 0, "%08Xh", madt->LocalApicAddress);
	NWL_NodeAttrSetBool(pNode, "PC-AT-compatible", madt->Flags & 0x01, 0);
	PNODE entries = NWL_NodeAppendNew(pNode, "Interrupt Controller Structures", NFLG_TABLE);
	APIC_HEADER* SubHdr = (APIC_HEADER*)((UINT8*)madt + sizeof(ACPI_MADT));
	UINT8* End = (UINT8*)Hdr + Hdr->Length;

	while ((UINT8*)SubHdr < End && SubHdr->Length > 0)
	{
		PNODE sub;
		CHAR t[] = "FF";
		sprintf_s(t, ARRAYSIZE(t), "%02X", SubHdr->Type);
		sub = NWL_NodeAppendNew(entries, t, NFLG_TABLE_ROW);
		switch (SubHdr->Type)
		{
		case ACPI_MADT_TYPE_PROCESSOR_LOCAL_APIC:
		{
			PROCESSOR_LOCAL_APIC* s = (PROCESSOR_LOCAL_APIC*)SubHdr;
			NWL_NodeAttrSet(sub, "Type", "Local APIC", 0);
			NWL_NodeAttrSetf(sub, "ACPI Processor UID", 0, "0x%02X", s->AcpiProcUid);
			NWL_NodeAttrSetf(sub, "APIC ID", 0, "0x%02X", s->ApicId);
			NWL_NodeAttrSetBool(sub, "Enabled", (s->Flags & 0x01), 0);
			NWL_NodeAttrSetBool(sub, "Online Capable", (s->Flags & 2), 0);
			break;
		}
		case ACPI_MADT_TYPE_IO_APIC:
		{
			IO_APIC* s = (IO_APIC*)SubHdr;
			NWL_NodeAttrSet(sub, "Type", "I/O APIC", 0);
			NWL_NodeAttrSetf(sub, "I/O APIC ID", 0, "0x%02X", s->IoApicId);
			NWL_NodeAttrSetf(sub, "Address", 0, "0x%08X", s->IoApicAddr);
			NWL_NodeAttrSetf(sub, "Global Interrupt Base", 0, "0x%08X", s->GlobalSystemInterruptBase);
			break;
		}
		case ACPI_MADT_TYPE_INTERRUPT_SOURCE_OVERRIDE:
		{
			INTERRUPT_SOURCE_OVERRIDE* s = (INTERRUPT_SOURCE_OVERRIDE*)SubHdr;
			NWL_NodeAttrSet(sub, "Type", "Interrupt Source Override", 0);
			NWL_NodeAttrSetf(sub, "Bus Source", 0, "0x%02X", s->Bus);
			NWL_NodeAttrSetf(sub, "IRQ Source", 0, "0x%02X", s->Source);
			NWL_NodeAttrSetf(sub, "Global Interrupt", 0, "0x%08X", s->GlobalSysInt);
			PrintInterruptFlags(sub, s->Flags);
			break;
		}
		case ACPI_MADT_TYPE_NMI_SOURCE:
		{
			NMI_SOURCE* s = (NMI_SOURCE*)SubHdr;
			NWL_NodeAttrSet(sub, "Type", "NMI Source", 0);
			NWL_NodeAttrSetf(sub, "Global Interrupt", 0, "0x%08X", s->GlobalSysInt);
			PrintInterruptFlags(sub, s->Flags);
			break;
		}
		case ACPI_MADT_TYPE_LOCAL_APIC_NMI:
		{
			LOCAL_APIC_NMI* s = (LOCAL_APIC_NMI*)SubHdr;
			NWL_NodeAttrSet(sub, "Type", "Local APIC NMI", 0);
			NWL_NodeAttrSetf(sub, "ACPI Processor UID", 0, "0x%02X", s->AcpiProcUid);
			NWL_NodeAttrSetf(sub, "LINT#", 0, "0x%02X", s->LocalApicLint);
			PrintInterruptFlags(sub, s->Flags);
			break;
		}
		case ACPI_MADT_TYPE_LOCAL_APIC_ADDR_OVERRIDE:
		{
			LOCAL_APIC_ADDR_OVERRIDE* s = (LOCAL_APIC_ADDR_OVERRIDE*)SubHdr;
			NWL_NodeAttrSet(sub, "Type", "Local APIC Address Override", 0);
			NWL_NodeAttrSetf(sub, "Address", 0, "0x%016llX", s->LocalApicAddr);
			break;
		}
		case ACPI_MADT_TYPE_PROCESSOR_LOCAL_X2APIC:
		{
			PROCESSOR_LOCAL_X2APIC* s = (PROCESSOR_LOCAL_X2APIC*)SubHdr;
			NWL_NodeAttrSet(sub, "Type", "Local x2APIC", 0);
			NWL_NodeAttrSetf(sub, "x2APIC ID", 0, "0x%08X", s->X2ApicId);
			NWL_NodeAttrSetf(sub, "ACPI Processor UID", 0, "0x%08X", s->AcpiProcUid);
			NWL_NodeAttrSetBool(sub, "Enabled", (s->Flags & 0x01), 0);
			NWL_NodeAttrSetBool(sub, "Online Capable", (s->Flags & 2), 0);
			break;
		}
		case ACPI_MADT_TYPE_LOCAL_X2APIC_NMI:
		{
			LOCAL_X2APIC_NMI* s = (LOCAL_X2APIC_NMI*)SubHdr;
			NWL_NodeAttrSet(sub, "Type", "Local x2APIC NMI", 0);
			NWL_NodeAttrSetf(sub, "ACPI Processor UID", 0, "0x%08X", s->AcpiProcUid);
			NWL_NodeAttrSetf(sub, "LINT#", 0, "0x%02X", s->LocalX2ApicLint);
			PrintInterruptFlags(sub, s->Flags);
			break;
		}
		case ACPI_MADT_TYPE_MULTIPROCESSOR_WAKEUP:
		{
			MULTIPROCESSOR_WAKEUP* s = (MULTIPROCESSOR_WAKEUP*)SubHdr;
			NWL_NodeAttrSet(sub, "Type", "Multiprocessor Wakeup", 0);
			NWL_NodeAttrSetf(sub, "Mailbox Address", 0, "0x%016llX", s->MailboxAddress);
			NWL_NodeAttrSetf(sub, "Mailbox Version", 0, "0x%04X", s->MailboxVersion);
			break;
		}
		default:
			NWL_NodeAttrSetf(sub, "Type", 0, "0x%02X", SubHdr->Type);
			NWL_NodeAttrSetf(sub, "Length", 0, "%u", SubHdr->Length);
			break;
		}
		SubHdr = (APIC_HEADER*)((UINT8*)SubHdr + SubHdr->Length);
	}

	return pNode;
}

static PNODE
PrintWPBT(PNODE pNode, DESC_HEADER* Hdr)
{
	ACPI_WPBT* wpbt = (ACPI_WPBT*)Hdr;
	if (!ACPI_FIELD_CHK(Hdr, ACPI_WPBT, CommandLineArgumentsLength))
		return pNode;
	if (Hdr->Length < offsetof(ACPI_WPBT, CommandLineArgumentsLength) + wpbt->CommandLineArgumentsLength)
		return pNode;
	NWL_NodeAttrSetf(pNode, "Handoff Memory Size", NAFLG_FMT_NUMERIC, "%u", wpbt->HandoffMemorySize);
	NWL_NodeAttrSetf(pNode, "Handoff Memory Location", 0, "0x%016llX", wpbt->HandoffMemoryLocation);
	NWL_NodeAttrSetf(pNode, "Content Layout", 0, "%u (%s)", wpbt->ContentLayout, wpbt->ContentLayout == 1 ? "PE" : "Unknown");
	NWL_NodeAttrSetf(pNode, "Content Type", 0, "%u (%s)", wpbt->ContentType, wpbt->ContentType == 1 ? "Native" : "Unknown");
	if (wpbt->CommandLineArgumentsLength > 0)
		NWL_NodeAttrSet(pNode, "Commandline Arguments", NWL_Ucs2ToUtf8(wpbt->CommandLineArguments), NAFLG_FMT_NEED_QUOTE);
	return pNode;
}

static PNODE
PrintTableInfo(PNODE pNode, DESC_HEADER* Hdr)
{
	if (!Hdr)
		return NULL;
	const UINT32 dwSignature = ACPI_SIG(Hdr->Signature[0], Hdr->Signature[1], Hdr->Signature[2], Hdr->Signature[3]);
	if (NWLC->AcpiTable && NWLC->AcpiTable != dwSignature)
		return NULL;
	PNODE tab = PrintTableHeader(pNode, Hdr);
	switch (dwSignature)
	{
	case ACPI_SIG('A', 'P', 'I', 'C'): return PrintMADT(tab, Hdr);
	case ACPI_SIG('B', 'G', 'R', 'T'): return PrintBGRT(tab, Hdr);
	case ACPI_SIG('F', 'A', 'C', 'P'): return PrintFADT(tab, Hdr);
	case ACPI_SIG('R', 'S', 'D', 'T'): return PrintRSDT(tab, Hdr);
	case ACPI_SIG('W', 'P', 'B', 'T'): return PrintWPBT(tab, Hdr);
	case ACPI_SIG('X', 'S', 'D', 'T'): return PrintXSDT(tab, Hdr);
	}
	return tab;
}

// Reading from physical memory will be flagged by Windows Defender
PNODE NW_Acpi(VOID)
{
	UINT32 i, count;
	PNODE pNode = NWL_NodeAlloc("ACPI", NFLG_TABLE);
	if (NWLC->AcpiInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, pNode);

	if (NWLC->NwRsdp == NULL)
		NWLC->NwRsdp = NWL_GetRsdp();
	if (NWLC->NwRsdt == NULL)
		NWLC->NwRsdt = NWL_GetRsdt();
	if (NWLC->NwXsdt == NULL)
		NWLC->NwXsdt = NWL_GetXsdt();

	if (NWLC->NwRsdp)
		PrintRSDP(pNode, NWLC->NwRsdp);
	if (NWLC->NwXsdt)
	{
		count = (NWLC->NwXsdt->Header.Length - sizeof(DESC_HEADER)) / sizeof(NWLC->NwXsdt->Entry[0]);
		for (i = 0; i < count; i++)
		{
			DESC_HEADER* t = NWL_GetAcpiByAddr((DWORD_PTR)NWLC->NwXsdt->Entry[i]);
			if (t)
			{
				PrintTableInfo(pNode, t);
				free(t);
			}
		}
	}
	else if (NWLC->NwRsdt)
	{
		count = (NWLC->NwRsdt->Header.Length - sizeof(DESC_HEADER)) / sizeof(NWLC->NwRsdt->Entry[0]);
		for (i = 0; i < count; i++)
		{
			DESC_HEADER* t = NWL_GetAcpiByAddr((DWORD_PTR)NWLC->NwRsdt->Entry[i]);
			if (t)
			{
				PrintTableInfo(pNode, t);
				free(t);
			}
		}
	}
	PrintTableInfo(pNode, (DESC_HEADER*)NWLC->NwRsdt);
	PrintTableInfo(pNode, (DESC_HEADER*)NWLC->NwXsdt);
	PrintFACS(pNode);
	return pNode;
}
