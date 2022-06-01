// SPDX-License-Identifier: Unlicense


#include "libnw.h"
#include "utils.h"
#include "acpi.h"

static void
PrintU8Str(PNODE pNode, LPCSTR Key, UINT8 *Str, DWORD Len)
{
	DWORD i = 0;
	NWLC->NwBuf[0] = '\0';
	for (i = 0; i < Len; i++)
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s%c", NWLC->NwBuf, Str[i]);
	NWL_NodeAttrSet(pNode, Key, NWLC->NwBuf, 0);
}

static void
PrintMSDM(PNODE pNode, struct acpi_table_header* Hdr)
{
	struct acpi_msdm* msdm = (struct acpi_msdm*)Hdr;
	if (Hdr->length < sizeof(struct acpi_msdm))
		return;
	NWL_NodeAttrSetf(pNode, "SLS Version", 0, "0x%08x", msdm->version);
	NWL_NodeAttrSetf(pNode, "SLS Data Type", 0, "0x%08x", msdm->data_type);
	NWL_NodeAttrSetf(pNode, "SLS Data Length", 0, "0x%08x", msdm->data_length);
	PrintU8Str(pNode, "Product Key", msdm->data, 29);
}

static void
PrintMADT(PNODE pNode, struct acpi_table_header* Hdr)
{
	struct acpi_madt* madt = (struct acpi_madt*)Hdr;
	if (Hdr->length < sizeof(struct acpi_madt))
		return;
	NWL_NodeAttrSetf(pNode, "Local APIC Address", 0, "%08Xh", madt->lapic_addr);
	NWL_NodeAttrSetBool(pNode, "PC-AT-compatible", madt->flags & 0x01, 0);
	// TODO: print interrupt controller structures
}

static void
PrintBGRT(PNODE pNode, struct acpi_table_header* Hdr)
{
	struct acpi_bgrt* bgrt = (struct acpi_bgrt*)Hdr;
	PNODE nimg;
	if (Hdr->length < sizeof(struct acpi_bgrt))
		return;
	NWL_NodeAttrSetf(pNode, "BGRT Version", NAFLG_FMT_NUMERIC, "%u", bgrt->version);
	NWL_NodeAttrSet(pNode, "BGRT Status", (bgrt->status & 0x01) ? "Valid" : "Invalid", 0);
	nimg = NWL_NodeAppendNew(pNode, "Image", NFLG_ATTGROUP);
	NWL_NodeAttrSet(nimg, "Type", (bgrt->type == 0) ? "BMP" : "Reserved", 0);
	NWL_NodeAttrSetf(nimg, "Address", 0, "0x%llx", bgrt->addr);
	NWL_NodeAttrSetf(nimg, "Offset X", NAFLG_FMT_NUMERIC, "%u", bgrt->x);
	NWL_NodeAttrSetf(nimg, "Offset Y", NAFLG_FMT_NUMERIC, "%u", bgrt->y);
}

static void
PrintWPBT(PNODE pNode, struct acpi_table_header* Hdr)
{
	struct acpi_wpbt* wpbt = (struct acpi_wpbt*)Hdr;
	if (Hdr->length < sizeof(struct acpi_wpbt))
		return;
	NWL_NodeAttrSetf(pNode, "Platform Binary", 0, "@0x%llx<0x%x>", wpbt->binary_addr, wpbt->binary_size);
	NWL_NodeAttrSetf(pNode, "Binary Content", 0, "%s, %s\n", (wpbt->content_layout == 1) ? "PE" : "Unknown",
		(wpbt->content_type == 1) ? "Native Application" : "unknown");
	if (wpbt->cmdline_length && wpbt->cmdline[0])
		NWL_NodeAttrSetf(pNode, "Cmdline", 0, "%wS\n", wpbt->cmdline);
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

static void
PrintFADT(PNODE pNode, struct acpi_table_header* Hdr)
{
	struct acpi_fadt* fadt = (struct acpi_fadt*)Hdr;
	if (Hdr->length >= offsetof(struct acpi_fadt, preferred_pm_profile))
		NWL_NodeAttrSet(pNode, "PM Profile", PmProfileToStr(fadt->preferred_pm_profile), 0);
	if (Hdr->length >= offsetof(struct acpi_fadt, sci_int))
		NWL_NodeAttrSetf(pNode, "SCI Interrupt Vector", 0, "0x%04X", fadt->sci_int);
	if (Hdr->length >= offsetof(struct acpi_fadt, smi_cmd))
		NWL_NodeAttrSetf(pNode, "SMI Command Port", 0, "0x%08X", fadt->smi_cmd);
	if (Hdr->length >= offsetof(struct acpi_fadt, acpi_enable))
		NWL_NodeAttrSetf(pNode, "ACPI Enable", 0, "0x%02X", fadt->acpi_enable);
	if (Hdr->length >= offsetof(struct acpi_fadt, acpi_disable))
		NWL_NodeAttrSetf(pNode, "ACPI Disable", 0, "0x%02X", fadt->acpi_disable);
	if (Hdr->length >= offsetof(struct acpi_fadt, s4bios_req))
		NWL_NodeAttrSetf(pNode, "S4 Request", 0, "0x%02X", fadt->s4bios_req);
	if (Hdr->length >= offsetof(struct acpi_fadt, pm_tmr_blk))
		NWL_NodeAttrSetf(pNode, "PM Timer", 0, "0x%08X", fadt->pm_tmr_blk);
}

static PNODE PrintTableHeader(PNODE pNode, struct acpi_table_header* Hdr)
{
	PNODE tab = NWL_NodeAppendNew(pNode, "Table", NFLG_TABLE_ROW);
	PrintU8Str(tab, "Signature", Hdr->signature, 4);
	NWL_NodeAttrSetf(tab, "Revision", 0, "0x%02x", Hdr->revision);
	NWL_NodeAttrSetf(tab, "Length", 0, "0x%x", Hdr->length);
	NWL_NodeAttrSetf(tab, "Checksum", 0, "0x%02x", Hdr->checksum);
	NWL_NodeAttrSet(tab, "Checksum Status", NWL_AcpiChecksum(Hdr, Hdr->length) == 0 ? "OK" : "ERR", 0);
	PrintU8Str(tab, "OEM ID", Hdr->oemid, 6);
	PrintU8Str(tab, "OEM Table ID", Hdr->oemtable, 8);
	NWL_NodeAttrSetf(tab, "OEM Revision", 0, "0x%lx", Hdr->oemrev);
	PrintU8Str(tab, "Creator ID", Hdr->creator_id, 4);
	NWL_NodeAttrSetf(tab, "Creator Revision", 0, "0x%x", Hdr->creator_rev);
	return tab;
}

static void PrintTableInfo(PNODE pNode, struct acpi_table_header* Hdr)
{
	PNODE tab = PrintTableHeader(pNode, Hdr);
	if (memcmp(Hdr->signature, "MSDM", 4) == 0)
		PrintMSDM(tab, Hdr);
	else if (memcmp(Hdr->signature, "APIC", 4) == 0)
		PrintMADT(tab, Hdr);
	else if (memcmp(Hdr->signature, "BGRT", 4) == 0)
		PrintBGRT(tab, Hdr);
	else if (memcmp(Hdr->signature, "WPBT", 4) == 0)
		PrintWPBT(tab, Hdr);
	else if (memcmp(Hdr->signature, "FACP", 4) == 0)
		PrintFADT(tab, Hdr);
}

static void
PrintXSDT(PNODE pNode, struct acpi_table_header* Hdr)
{
	UINT32 i, count;
	PNODE entries;
	struct acpi_xsdt* xsdt = (struct acpi_xsdt*)Hdr;
	PNODE tab = PrintTableHeader(pNode, Hdr);

	count = (xsdt->header.length - sizeof(struct acpi_table_header)) / sizeof(xsdt->entry[0]);
	entries = NWL_NodeAppendNew(tab, "Entries", NFLG_TABLE);
	for (i = 0; i < count; i++)
	{
		PNODE entry;
		CHAR name[5];
		struct acpi_table_header *t = NWL_GetAcpiByAddr((DWORD_PTR)xsdt->entry[i]);
		if (!t)
			continue;
		snprintf(name, 5, "%c%c%c%c",
			t->signature[0], t->signature[1], t->signature[2], t->signature[3]);
		entry = NWL_NodeAppendNew(entries, name, NFLG_TABLE_ROW);
		NWL_NodeAttrSetf(entry, "Address", 0, "0x%016llx", xsdt->entry[i]);
		PrintTableInfo(pNode, t);
		free(t);
	}
}

static void
PrintRSDT(PNODE pNode, struct acpi_table_header* Hdr)
{
	UINT32 i, count;
	PNODE entries;
	struct acpi_rsdt* rsdt = (struct acpi_rsdt*)Hdr;
	PNODE tab = PrintTableHeader(pNode, Hdr);

	count = (rsdt->header.length - sizeof(struct acpi_table_header)) / sizeof(rsdt->entry[0]);
	entries = NWL_NodeAppendNew(tab, "Entries", NFLG_TABLE);
	for (i = 0; i < count; i++)
	{
		PNODE entry;
		CHAR name[5];
		struct acpi_table_header* t = NWL_GetAcpiByAddr((DWORD_PTR)rsdt->entry[i]);
		if (!t)
			continue;
		snprintf(name, 5, "%c%c%c%c",
			t->signature[0], t->signature[1], t->signature[2], t->signature[3]);
		entry = NWL_NodeAppendNew(entries, name, NFLG_TABLE_ROW);
		NWL_NodeAttrSetf(entry, "Address", 0, "0x%08x", rsdt->entry[i]);
		PrintTableInfo(pNode, t);
		free(t);
	}
}

static void PrintRSDP(PNODE pNode, struct acpi_rsdp_v2* rsdp)
{
	PNODE tab = NWL_NodeAppendNew(pNode, "Table", NFLG_TABLE_ROW);
	PrintU8Str(tab, "Signature", rsdp->rsdpv1.signature, RSDP_SIGNATURE_SIZE);
	NWL_NodeAttrSetf(tab, "V1 Checksum", 0, "0x%02x", rsdp->checksum);
	NWL_NodeAttrSet(tab, "V1 Checksum Status",
		NWL_AcpiChecksum(&rsdp->rsdpv1, sizeof(struct acpi_rsdp_v1)) == 0 ? "OK" : "ERR", 0);
	PrintU8Str(tab, "OEM ID", rsdp->rsdpv1.oemid, 6);
	NWL_NodeAttrSetf(tab, "Revision", 0, "0x%02x", rsdp->rsdpv1.revision);
	NWL_NodeAttrSetf(tab, "RSDT Address", 0, "0x%08x", rsdp->rsdpv1.rsdt_addr);
	if (rsdp->rsdpv1.revision == 0)
		return;

	NWL_NodeAttrSetf(tab, "Length", 0, "0x%x", rsdp->length);
	NWL_NodeAttrSetf(tab, "Checksum", 0, "0x%02x", rsdp->checksum);
	NWL_NodeAttrSet(tab, "Checksum Status",
		NWL_AcpiChecksum(rsdp, rsdp->length) == 0 ? "OK" : "ERR", 0);
	NWL_NodeAttrSetf(tab, "XSDT Address", 0, "0x%016llx", rsdp->xsdt_addr);
}

PNODE NW_Acpi(VOID)
{
	PNODE pNode = NWL_NodeAlloc("ACPI", NFLG_TABLE);
	if (NWLC->AcpiInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, pNode);
	if (NWLC->AcpiTable)
	{
		struct acpi_table_header* AcpiHdr = NWL_GetAcpi(NWLC->AcpiTable);
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
		PrintXSDT(pNode, (struct acpi_table_header*)NWLC->NwXsdt);
	else if (NWLC->NwRsdt)
		PrintRSDT(pNode, (struct acpi_table_header*)NWLC->NwXsdt);
	return pNode;
}
