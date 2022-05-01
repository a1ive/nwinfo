// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "nwinfo.h"

static void
PrintU8Str(PNODE pNode, LPCSTR Key, UINT8 *Str, DWORD Len)
{
	DWORD i = 0;
	nwinfo_buffer[0] = '\0';
	for (i = 0; i < Len; i++)
		snprintf(nwinfo_buffer, NWINFO_BUFSZ, "%s%c", nwinfo_buffer, Str[i]);
	node_att_set(pNode, Key, nwinfo_buffer, 0);
}

static void
PrintMSDM(PNODE pNode, struct acpi_table_header* Hdr)
{
	struct acpi_msdm* msdm = (struct acpi_msdm*)Hdr;
	if (Hdr->length < sizeof(struct acpi_msdm))
		return;
	node_att_setf(pNode, "SLS Version", 0, "0x%08x", msdm->version);
	node_att_setf(pNode, "SLS Data Type", 0, "0x%08x", msdm->data_type);
	node_att_setf(pNode, "SLS Data Length", 0, "0x%08x", msdm->data_length);
	PrintU8Str(pNode, "Product Key", msdm->data, 29);
}

static void
PrintMADT(PNODE pNode, struct acpi_table_header* Hdr)
{
	struct acpi_madt* madt = (struct acpi_madt*)Hdr;
	if (Hdr->length < sizeof(struct acpi_madt))
		return;
	node_att_setf(pNode, "Local APIC Address", 0, "%08Xh", madt->lapic_addr);
	node_att_set_bool(pNode, "PC-AT-compatible", madt->flags & 0x01, 0);
	// TODO: print interrupt controller structures
}

static void
PrintBGRT(PNODE pNode, struct acpi_table_header* Hdr)
{
	struct acpi_bgrt* bgrt = (struct acpi_bgrt*)Hdr;
	PNODE nimg;
	if (Hdr->length < sizeof(struct acpi_bgrt))
		return;
	node_att_setf(pNode, "BGRT Version", NAFLG_FMT_NUMERIC, "%u", bgrt->version);
	node_att_set(pNode, "BGRT Status", (bgrt->status & 0x01) ? "Valid" : "Invalid", 0);
	nimg = node_append_new(pNode, "Image", NFLG_ATTGROUP);
	node_att_set(nimg, "Type", (bgrt->type == 0) ? "BMP" : "Reserved", 0);
	node_att_setf(nimg, "Address", 0, "0x%llx", bgrt->addr);
	node_att_setf(nimg, "Offset X", NAFLG_FMT_NUMERIC, "%u", bgrt->x);
	node_att_setf(nimg, "Offset Y", NAFLG_FMT_NUMERIC, "%u", bgrt->y);
}

static void
PrintWPBT(PNODE pNode, struct acpi_table_header* Hdr)
{
	struct acpi_wpbt* wpbt = (struct acpi_wpbt*)Hdr;
	if (Hdr->length < sizeof(struct acpi_wpbt))
		return;
	node_att_setf(pNode, "Platform Binary", 0, "@0x%llx<0x%x>", wpbt->binary_addr, wpbt->binary_size);
	node_att_setf(pNode, "Binary Content", 0, "%s, %s\n", (wpbt->content_layout == 1) ? "PE" : "Unknown",
		(wpbt->content_type == 1) ? "Native Application" : "unknown");
	if (wpbt->cmdline_length && wpbt->cmdline[0])
		node_att_setf(pNode, "Cmdline", 0, "%wS\n", wpbt->cmdline);
}

static const CHAR*
PmProfileToStr(uint8_t profile)
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
		node_att_set(pNode, "PM Profile", PmProfileToStr(fadt->preferred_pm_profile), 0);
	if (Hdr->length >= offsetof(struct acpi_fadt, sci_int))
		node_att_setf(pNode, "SCI Interrupt Vector", 0, "0x%04X", fadt->sci_int);
	if (Hdr->length >= offsetof(struct acpi_fadt, smi_cmd))
		node_att_setf(pNode, "SMI Command Port", 0, "0x%08X", fadt->smi_cmd);
	if (Hdr->length >= offsetof(struct acpi_fadt, acpi_enable))
		node_att_setf(pNode, "ACPI Enable", 0, "0x%02X", fadt->acpi_enable);
	if (Hdr->length >= offsetof(struct acpi_fadt, acpi_disable))
		node_att_setf(pNode, "ACPI Disable", 0, "0x%02X", fadt->acpi_disable);
	if (Hdr->length >= offsetof(struct acpi_fadt, s4bios_req))
		node_att_setf(pNode, "S4 Request", 0, "0x%02X", fadt->s4bios_req);
	if (Hdr->length >= offsetof(struct acpi_fadt, pm_tmr_blk))
		node_att_setf(pNode, "PM Timer", 0, "0x%08X", fadt->pm_tmr_blk);
}

static void PrintTableInfo(PNODE pNode, struct acpi_table_header* Hdr)
{
	PNODE tab = node_append_new(pNode, "Table", NFLG_TABLE_ROW);
	PrintU8Str(tab, "Signature", Hdr->signature, 4);
	node_att_setf(tab, "Revision", 0, "0x%02x", Hdr->revision);
	node_att_setf(tab, "Length", 0, "0x%x", Hdr->length);
	node_att_setf(tab, "Checksum", 0, "0x%02x", Hdr->checksum);
	node_att_set(tab, "Checksum Status", AcpiChecksum(Hdr, Hdr->length) == 0 ? "OK" : "ERR", 0);
	PrintU8Str(tab, "OEM ID", Hdr->oemid, 6);
	PrintU8Str(tab, "OEM Table ID", Hdr->oemtable, 8);
	node_att_setf(tab, "OEM Revision", 0, "0x%lx", Hdr->oemrev);
	PrintU8Str(tab, "Creator ID", Hdr->creator_id, 4);
	node_att_setf(tab, "Creator Revision", 0, "0x%x", Hdr->creator_rev);
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
#pragma warning(push)
// fuck you microsoft
#pragma warning(disable:6385)
#pragma warning(disable:6386)
PNODE nwinfo_acpi(DWORD signature)
{
	struct acpi_table_header *AcpiHdr = NULL;
	DWORD* AcpiList = NULL;
	UINT AcpiListSize = 0, i = 0, j = 0;
	PNODE pNode = node_alloc("ACPI", NFLG_TABLE);
	if (signature)
	{
		AcpiHdr = GetAcpi(signature);
		if (AcpiHdr)
		{
			PrintTableInfo(pNode, AcpiHdr);
			free(AcpiHdr);
		}
		return pNode;
	}
	AcpiListSize = NT5EnumSystemFirmwareTables('ACPI', NULL, 0);
	if (AcpiListSize < 4)
		return pNode;
	AcpiList = malloc(AcpiListSize);
	if (!AcpiList)
		return pNode;
	NT5EnumSystemFirmwareTables('ACPI', AcpiList, AcpiListSize);
	AcpiListSize = AcpiListSize / 4;
	for (i = 0; i < AcpiListSize; i++)
	{
		if (AcpiList[i] == 0)
			continue;
		AcpiHdr = GetAcpi(AcpiList[i]);
		if (!AcpiHdr)
			continue;
		PrintTableInfo(pNode, AcpiHdr);
		free(AcpiHdr);
		// remove duplicate elements
		for (j = i + 1; j < AcpiListSize; j++)
		{
			if (AcpiList[i] == AcpiList[j])
				AcpiList[j] = 0;
		}
	}
	free(AcpiList);
	return pNode;
}
#pragma warning(pop)
