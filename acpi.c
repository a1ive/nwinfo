// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <sysinfoapi.h>
#include <stdio.h>
#include <stdlib.h>
#include "nwinfo.h"

static void
PrintU8Str(UINT8 *Str, DWORD Len)
{
	DWORD i = 0;
	for (i = 0; i < Len; i++)
		printf("%c", Str[i]);
}

static void
PrintMSDM(struct acpi_table_header* Hdr)
{
	struct acpi_msdm* msdm = (struct acpi_msdm*)Hdr;
	if (Hdr->length < sizeof(struct acpi_msdm))
		return;
	printf("  SLS Version: 0x%08x\n", msdm->version);
	printf("  SLS Data Type: 0x%08x\n", msdm->data_type);
	printf("  SLS Data Length: 0x%08x\n", msdm->data_length);
	printf("  Product Key: ");
	PrintU8Str(msdm->data, 29);
	printf("\n");
}

static void
PrintMADT(struct acpi_table_header* Hdr)
{
	struct acpi_madt* madt = (struct acpi_madt*)Hdr;
	if (Hdr->length < sizeof(struct acpi_madt))
		return;
	printf("  Local APIC Address: %08Xh\n", madt->lapic_addr);
	printf("  PC-AT-compatible: %s\n", (madt->flags & 0x01) ? "True" : "False");
	// TODO: print interrupt controller structures
}

static void
PrintBGRT(struct acpi_table_header* Hdr)
{
	struct acpi_bgrt* bgrt = (struct acpi_bgrt*)Hdr;
	if (Hdr->length < sizeof(struct acpi_bgrt))
		return;
	printf("  BGRT Version: %u\n", bgrt->version);
	printf("  BGRT Status: %s\n", (bgrt->status & 0x01) ? "Valid" : "Invalid");
	printf("  Image Type: %s\n", (bgrt->type == 0) ? "BMP" : "Reserved");
	printf("  Image Address: 0x%llx\n", bgrt->addr);
	printf("  Image Offset: %u,%u\n", bgrt->x, bgrt->y);
}

static void
PrintWPBT(struct acpi_table_header* Hdr)
{
	struct acpi_wpbt* wpbt = (struct acpi_wpbt*)Hdr;
	if (Hdr->length < sizeof(struct acpi_wpbt))
		return;
	printf("  Platform Binary: @0x%llx<0x%x>\n", wpbt->binary_addr, wpbt->binary_size);
	printf("  Binary Content: %s, %s\n", (wpbt->content_layout == 1) ? "PE" : "Unknown",
		(wpbt->content_type == 1) ? "native application" : "unknown");
	if (wpbt->cmdline_length && wpbt->cmdline[0])
		printf("  Cmdline: %wS\n", wpbt->cmdline);
}

static const CHAR*
PmProfileToStr(uint8_t profile)
{
	switch (profile) {
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
PrintFADT(struct acpi_table_header* Hdr)
{
	struct acpi_fadt* fadt = (struct acpi_fadt*)Hdr;
	if (Hdr->length >= offsetof(struct acpi_fadt, preferred_pm_profile))
		printf("  PM Profile: %s\n", PmProfileToStr(fadt->preferred_pm_profile));
	if (Hdr->length >= offsetof(struct acpi_fadt, sci_int))
		printf("  SCI Interrupt Vector: 0x%04X\n", fadt->sci_int);
	if (Hdr->length >= offsetof(struct acpi_fadt, smi_cmd))
		printf("  SMI Command Port: 0x%08X\n", fadt->smi_cmd);
	if (Hdr->length >= offsetof(struct acpi_fadt, acpi_enable))
		printf("  ACPI Enable: 0x%02X\n", fadt->acpi_enable);
	if (Hdr->length >= offsetof(struct acpi_fadt, acpi_disable))
		printf("  ACPI Disable: 0x%02X\n", fadt->acpi_disable);
	if (Hdr->length >= offsetof(struct acpi_fadt, s4bios_req))
		printf("  S4 Request: 0x%02X\n", fadt->s4bios_req);
	if (Hdr->length >= offsetof(struct acpi_fadt, pm_tmr_blk))
		printf("  PM Timer: 0x%08X\n", fadt->pm_tmr_blk);
}

static void PrintTableInfo(struct acpi_table_header* Hdr)
{
	PrintU8Str(Hdr->signature, 4);
	printf("\n  Revision: 0x%02x", Hdr->revision);
	printf("\n  Length: 0x%x", Hdr->length);
	printf("\n  Checksum: 0x%02x (%s)", Hdr->checksum, AcpiChecksum(Hdr, Hdr->length) == 0 ? "OK" : "ERR");
	printf("\n  OEM ID: ");
	PrintU8Str(Hdr->oemid, 6);
	printf("\n  OEM Table ID: ");
	PrintU8Str(Hdr->oemtable, 8);
	printf("\n  OEM Revision: 0x%lx", Hdr->oemrev);
	printf("\n  Creator ID: ");
	PrintU8Str(Hdr->creator_id, 4);
	printf("\n  Creator Revision: 0x%x\n", Hdr->creator_rev);
	if (memcmp(Hdr->signature, "MSDM", 4) == 0)
		PrintMSDM(Hdr);
	else if (memcmp(Hdr->signature, "APIC", 4) == 0)
		PrintMADT(Hdr);
	else if (memcmp(Hdr->signature, "BGRT", 4) == 0)
		PrintBGRT(Hdr);
	else if (memcmp(Hdr->signature, "WPBT", 4) == 0)
		PrintWPBT(Hdr);
	else if (memcmp(Hdr->signature, "FACP", 4) == 0)
		PrintFADT(Hdr);
}

void nwinfo_acpi(DWORD signature)
{
	struct acpi_table_header *AcpiHdr = NULL;
	DWORD* AcpiList = NULL;
	UINT AcpiListSize = 0, i = 0, j = 0, k = 0;
	if (signature) {
		AcpiHdr = GetAcpi(signature);
		if (AcpiHdr) {
			PrintTableInfo(AcpiHdr);
			free(AcpiHdr);
		}
		return;
	}
	AcpiListSize = EnumSystemFirmwareTables('ACPI', NULL, 0);
	if (AcpiListSize < 4)
		return;
	AcpiList = malloc(AcpiListSize);
	if (!AcpiList)
		return;
	EnumSystemFirmwareTables('ACPI', AcpiList, AcpiListSize);
	AcpiListSize = AcpiListSize / 4;
	// remove duplicate elements
	for (i = 0; i < AcpiListSize; i++)
	{
		for (j = i + 1; j < AcpiListSize; j++)
		{
			if (AcpiList[i] == AcpiList[j])
			{
				// Delete the current duplicate element
				for (k = j; k < AcpiListSize - 1; k++)
				{
					AcpiList[k] = AcpiList[k + 1];
				}

				AcpiListSize--;
				j--;
			}
		}
	}
	for (i = 0; i < AcpiListSize; i++)
	{
		AcpiHdr = GetAcpi(AcpiList[i]);
		if (!AcpiHdr)
			continue;
		PrintTableInfo(AcpiHdr);
		free(AcpiHdr);
	}

	free(AcpiList);
}
