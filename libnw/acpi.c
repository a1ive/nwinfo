// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"
#include "acpi.h"

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

static PNODE PrintTableHeader(PNODE pNode, DESC_HEADER* Hdr)
{
	const UINT32 dwSignature = ACPI_SIG(Hdr->Signature[0], Hdr->Signature[1], Hdr->Signature[2], Hdr->Signature[3]);
	PNODE tab = NWL_NodeAppendNew(pNode, "Table", NFLG_TABLE_ROW);

	PrintU8Str(tab, "Signature", Hdr->Signature, 4);
	NWL_NodeAttrSet(tab, "Description", GetAcpiTableDescription(dwSignature), 0);
	NWL_NodeAttrSetf(tab, "Revision", 0, "0x%02x", Hdr->Revision);
	NWL_NodeAttrSetf(tab, "Length", 0, "0x%x", Hdr->Length);
	NWL_NodeAttrSetf(tab, "Checksum", 0, "0x%02x", Hdr->Checksum);
	NWL_NodeAttrSet(tab, "Checksum Status", NWL_AcpiChecksum(Hdr, Hdr->Length) == 0 ? "OK" : "ERR", 0);
	PrintU8Str(tab, "OEM ID", Hdr->OemId, 6);
	PrintU8Str(tab, "OEM Table ID", Hdr->OemTableId, 8);
	NWL_NodeAttrSetf(tab, "OEM Revision", 0, "0x%lx", Hdr->OemRevision);
	PrintU8Str(tab, "Creator ID", Hdr->CreatorId, 4);
	NWL_NodeAttrSetf(tab, "Creator Revision", 0, "0x%x", Hdr->CreatorRevision);
	return tab;
}

static void PrintTableInfo(PNODE pNode, DESC_HEADER* Hdr)
{
	PNODE tab = PrintTableHeader(pNode, Hdr);
}

static void
PrintXSDT(PNODE pNode, DESC_HEADER* Hdr)
{
	UINT32 i, count;
	PNODE entries;
	ACPI_XSDT* xsdt = (ACPI_XSDT*)Hdr;
	PNODE tab = PrintTableHeader(pNode, Hdr);

	count = (xsdt->Header.Length - sizeof(DESC_HEADER)) / sizeof(xsdt->Entry[0]);
	entries = NWL_NodeAppendNew(tab, "Entries", NFLG_TABLE);
	for (i = 0; i < count; i++)
	{
		PNODE entry;
		CHAR name[5];
		DESC_HEADER *t = NWL_GetAcpiByAddr((DWORD_PTR)xsdt->Entry[i]);
		if (!t)
			continue;
		snprintf(name, 5, "%c%c%c%c",
			t->Signature[0], t->Signature[1], t->Signature[2], t->Signature[3]);
		entry = NWL_NodeAppendNew(entries, name, NFLG_TABLE_ROW);
		NWL_NodeAttrSetf(entry, "Address", 0, "0x%016llx", xsdt->Entry[i]);
		PrintTableInfo(pNode, t);
		free(t);
	}
}

static void
PrintRSDT(PNODE pNode, DESC_HEADER* Hdr)
{
	UINT32 i, count;
	PNODE entries;
	ACPI_RSDT* rsdt = (ACPI_RSDT*)Hdr;
	PNODE tab = PrintTableHeader(pNode, Hdr);

	count = (rsdt->Header.Length - sizeof(DESC_HEADER)) / sizeof(rsdt->Entry[0]);
	entries = NWL_NodeAppendNew(tab, "Entries", NFLG_TABLE);
	for (i = 0; i < count; i++)
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
		PrintTableInfo(pNode, t);
		free(t);
	}
}

static void PrintRSDP(PNODE pNode, ACPI_RSDP_V2* rsdp)
{
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
		return;

	NWL_NodeAttrSetf(tab, "Length", 0, "0x%x", rsdp->Length);
	NWL_NodeAttrSetf(tab, "Checksum", 0, "0x%02x", rsdp->Checksum);
	NWL_NodeAttrSet(tab, "Checksum Status",
		NWL_AcpiChecksum(rsdp, rsdp->Length) == 0 ? "OK" : "ERR", 0);
	NWL_NodeAttrSetf(tab, "XSDT Address", 0, "0x%016llx", rsdp->XsdtAddr);
}

// Reading from physical memory will be flagged by Windows Defender
PNODE NW_Acpi(VOID)
{
	PNODE pNode = NWL_NodeAlloc("ACPI", NFLG_TABLE);
	if (NWLC->AcpiInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, pNode);
	if (NWLC->AcpiTable)
	{
		DESC_HEADER* AcpiHdr = NWL_GetAcpi(NWLC->AcpiTable);
		if (AcpiHdr)
		{
			PrintTableInfo(pNode, AcpiHdr);
			free(AcpiHdr);
		}
		return pNode;
	}
	if (NWLC->NwRsdp)
		PrintRSDP(pNode, NWLC->NwRsdp);
	if (NWLC->NwXsdt)
		PrintXSDT(pNode, (DESC_HEADER*)NWLC->NwXsdt);
	else if (NWLC->NwRsdt)
		PrintRSDT(pNode, (DESC_HEADER*)NWLC->NwRsdt);
	return pNode;
}
