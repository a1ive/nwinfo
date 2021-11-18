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
}

void nwinfo_acpi(void)
{
	struct acpi_table_header *AcpiHdr = NULL;
	DWORD* AcpiList = NULL;
	UINT AcpiListSize = 0, i = 0, j = 0, k = 0;
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
		if (AcpiList[i] == 'MDSM' && AcpiHdr->length >= sizeof(struct acpi_msdm))
		{
			struct acpi_msdm* msdm = (struct acpi_msdm*)AcpiHdr;
			printf("  Software Licensing:\n");
			printf("    Version: 0x%08x\n", msdm->version);
			printf("    Reserved: 0x%08x\n", msdm->reserved);
			printf("    Data Type: 0x%08x\n", msdm->data_type);
			printf("    Data Reserved: 0x%08x\n", msdm->data_reserved);
			printf("    Data Length: 0x%08x\n", msdm->data_length);
			printf("    Product Key: ");
			PrintU8Str(msdm->data, 29);
			printf("\n");
		}

		free(AcpiHdr);
	}

	free(AcpiList);
}
