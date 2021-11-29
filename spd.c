// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "nwinfo.h"
#include "spd.h"

static void
PrintSpd(UINT8* rawSpd)
{
	UINT i = 0, j = 0;
	for (i = 0; i < SPD_DATA_LEN; i += 0x10)
	{
		for (j = 0; j < 0x10; j++)
			printf(" %02X", rawSpd[i + j]);
		printf("\n");
	}
}

static int Parity(int value)
{
	value ^= value >> 16;
	value ^= value >> 8;
	value ^= value >> 4;
	value &= 0xf;
	return (0x6996 >> value) & 1;
}

static const CHAR*
DDR34ModuleType(UINT8 Type)
{
	Type &= 0x0F;
	switch (Type) {
	case 0: return "EXT.DIMM";
	case 1: return "RDIMM";
	case 2: return "UDIMM";
	case 3: return "SO-DIMM";
	case 4: return "LRDIMM";
	case 5: return "Mini-RDIMM";
	case 6: return "Mini-UDIMM";
	case 8: return "72b-SO-RDIMM";
	case 9: return "72b-SO-UDIMM";
	case 12: return "16b-SO-DIMM";
	case 13: return "32b-SO-DIMM";
	}
	return "UNKNOWN";
}

static const CHAR*
DDR2ModuleType(UINT8 Type)
{
	Type &= 0x3F;
	switch (Type) {
	case 1: return "RDIMM";
	case 2: return "UDIMM";
	case 4: return "SO-DIMM";
	case 6: return "72b-SO-CDIMM";
	case 7: return "72b-SO-RDIMM";
	case 8: return "Micro-DIMM";
	case 16: return "Mini-RDIMM";
	case 32: return "Mini-UDIMM";
	}
	return "UNKNOWN";
}

static UINT64
DDR4Capacity(UINT8* rawSpd)
{
	UINT64 Size = 0;
	UINT64 SdrCapacity = 256ULL << (rawSpd[4] & 0x0FU);
	UINT32 BusWidth = 8U << (rawSpd[13] & 0x07U);
	UINT32 SdrWidth = 4U << (rawSpd[12] & 0x07U);
	UINT32 SignalLoading = rawSpd[6] & 0x03U;
	UINT32 RanksPerDimm = ((rawSpd[12] >> 3U) & 0x07U) + 1;

	if (SignalLoading == 2)
		RanksPerDimm *= ((rawSpd[6] >> 4U) & 0x07U) + 1;

	Size = SdrCapacity / 8 * BusWidth / SdrWidth * RanksPerDimm;
	return Size;
}

static UINT64
DDR3Capacity(UINT8* rawSpd)
{
	UINT64 Size = 0;
	UINT64 sdrCapacity = 256ULL << (rawSpd[4] & 0x0FU);
	UINT32 sdrWidth = 4U << (rawSpd[7] & 0x07U);
	UINT32 busWidth = 8U << (rawSpd[8] & 0x07U);
	UINT32 Ranks = 1 + ((rawSpd[7] >> 3) & 0x07U);
	Size = sdrCapacity / 8 * busWidth / sdrWidth * Ranks;
	return Size;
}

UINT64
DDR2Capacity(UINT8* rawSpd)
{
	UINT64 Size = 0;
	INT i, k;
	i = (rawSpd[3] & 0x0FU) + (rawSpd[4] & 0x0FU) - 17;
	k = ((rawSpd[5] & 0x07U) + 1) * rawSpd[17];
	if (i > 0 && i <= 12 && k > 0)
		Size = (1ULL << i) * k;
	return Size;
}

UINT64 DDRCapacity(UINT8* rawSpd)
{
	UINT64 Size = 0;
	INT i, k;
	i = (rawSpd[3] & 0x0FU) + (rawSpd[4] & 0x0FU) - 17;
	k = (rawSpd[5] <= 8 && rawSpd[17] <= 8) ? rawSpd[5] * rawSpd[17] : 0;
	if (i > 0 && i <= 12 && k > 0)
		Size = (1ULL << i) * k;
	return Size;
}

static const CHAR*
DDR34Manufacturer(UINT8 Lsb, UINT8 Msb)
{
	UINT Bank = 0, Index = 0;
	if (Msb == 0x00 || Msb == 0xFF)
		return "Unknown";

	if (Parity(Lsb) != 1 || Parity(Msb) != 1)
		return "Invalid";

	Bank = Lsb & 0x7f;
	Index = Msb & 0x7f;
	if (Bank >= VENDORS_BANKS)
		return "Unknown";

	return JEDEC_MFG_STR(Bank, Index - 1);
}

static INT SpdWritten(UINT8* Raw, int Length)
{
	do {
		if (*Raw == 0x00 || *Raw == 0xFF) return 1;
	} while (--Length && Raw++);
	return 0;
}

static const CHAR*
DDRManufacturer(unsigned char* Raw)
{
	UINT8 First = 0;
	UINT i = 0;
	UINT Length = 8;

	if (!SpdWritten(Raw, 8))
		return "Undefined";

	do { i++; }
	while ((--Length && (*Raw++ == 0x7FU)));
	First = *--Raw;

	if (i < 1)
		return "Invalid";

	if (Parity(First) != 1)
		return "Invalid";

	return JEDEC_MFG_STR(i - 1, (First & 0x7FU) - 1);
}

static const CHAR*
DDR234Date(UINT8 rawYear, UINT8 rawWeek) {
	UINT32 Year = 0, Week = 0;
	static CHAR Date[] = "Week52/2021";
	if (rawYear == 0x0 || rawYear == 0xff ||
		rawWeek == 0x0 || rawWeek == 0xff) {
		return "-";
	}
	Week = ((rawWeek >> 4) & 0x0FU) * 10U + (rawWeek & 0x0FU);
	Year = ((rawYear >> 4) & 0x0FU) * 10U + (rawYear & 0x0FU) + 2000;
	snprintf(Date, sizeof(Date), "Week%02u/%04u", Week, Year);
	return Date;
}

static const CHAR*
DDRDate(UINT8 rawYear, UINT8 rawWeek) {
	UINT32 Year = 0, Week = 0;
	static CHAR Date[] = "Week52/1990";
	if (rawYear == 0x0 || rawYear == 0xff ||
		rawWeek == 0x0 || rawWeek == 0xff) {
		return "-";
	}
	Week = ((rawWeek >> 4) & 0x0FU) * 10U + (rawWeek & 0x0FU);
	Year = ((rawYear >> 4) & 0x0FU) * 10U + (rawYear & 0x0FU) + 1900;
	snprintf(Date, sizeof(Date), "Week%02u/%04u", Week, Year);
	return Date;
}

static void
PrintDDR4(UINT8* rawSpd)
{
	UINT i = 0;
	printf("  Revision: %u.%u\n", rawSpd[1] >> 4, rawSpd[1] & 0x0FU);
	printf("  Module Type: %s%s\n", DDR34ModuleType(rawSpd[3]), (rawSpd[13] & 0x08U) ? " (ECC)" : "");
	printf("  Capacity: %llu MB\n", DDR4Capacity(rawSpd));
	printf("  Voltage: %s\n", (rawSpd[11] & 0x01U) ? "1.2 V" : "(Unknown)");
	printf("  Manufacturer: %s\n", DDR34Manufacturer(rawSpd[320], rawSpd[321]));
	printf("  Date: %s\n", DDR234Date(rawSpd[323], rawSpd[324]));
	printf("  Serial: ");
	for (i = 0; i < 4; i++)
		printf("%02X", rawSpd[325 + i]);
	printf("\n");
}

static void
PrintDDR3(UINT8* rawSpd)
{
	UINT i = 0;
	printf("  Revision: %u.%u\n", rawSpd[1] >> 4, rawSpd[1] & 0x0FU);
	printf("  Module Type: %s%s\n", DDR34ModuleType(rawSpd[3]), (rawSpd[8] >> 3 == 1) ? " (ECC)" : "");
	printf("  Capacity: %llu MB\n", DDR3Capacity(rawSpd));
	printf("  Supported Voltages:%s%s%s\n", (rawSpd[6] & 0x04U) ? " 1.25V" : "",
		(rawSpd[6] & 0x02U) ? " 1.35V" : "", (rawSpd[6] & 0x01U) ? "" : " 1.5V");
	printf("  Manufacturer: %s\n", DDR34Manufacturer(rawSpd[94], rawSpd[95]));
	printf("  Date: %s\n", DDR234Date(rawSpd[120], rawSpd[121]));
	printf("  Serial: ");
	for (i = 0; i < 4; i++)
		printf("%02X", rawSpd[122 + i]);
	printf("\n");
}

static void
PrintDDR2(UINT8* rawSpd)
{
	UINT i = 0;
	printf("  Revision: %u.%u\n", rawSpd[1] >> 4, rawSpd[1] & 0x0FU);
	printf("  Module Type: %s%s\n", DDR2ModuleType(rawSpd[3]), (rawSpd[11] >> 1 == 1) ? " (ECC)" : "");
	printf("  Capacity: %llu MB\n", DDR2Capacity(rawSpd));
	printf("  Manufacturer: %s\n", DDRManufacturer(rawSpd + 64));
	printf("  Date: %s\n", DDR234Date(rawSpd[93], rawSpd[94]));
	printf("  Serial: ");
	for (i = 0; i < 4; i++)
		printf("%02X", rawSpd[95 + i]);
	printf("\n");
}

static void
PrintDDR(UINT8* rawSpd)
{
	UINT i = 0;
	printf("  Revision: %u.%u\n", rawSpd[1] >> 4, rawSpd[1] & 0x0FU);
	printf("  Capacity: %llu MB\n", DDRCapacity(rawSpd));
	printf("  Manufacturer: %s\n", DDRManufacturer(rawSpd + 64));
	printf("  Date: %s\n", DDRDate(rawSpd[93], rawSpd[94]));
	printf("  Serial: ");
	for (i = 0; i < 4; i++)
		printf("%02X", rawSpd[95 + i]);
	printf("\n");
}

void
nwinfo_spd(void)
{
	int i = 0;
	UINT8* rawSpd = NULL;
	SpdInit();
	for (i = 0; i < 8; i++) {
		rawSpd = SpdGet(i);
		if (!rawSpd)
			continue;
		switch (rawSpd[2]) {
		case 4:
			printf("Slot %d: SDRAM\n", i);
			PrintDDR(rawSpd);
			break;
		case 5:
			printf("Slot %d: ROM\n", i);
			break;
		case 6:
			printf("Slot %d: DDR SGRAM\n", i);
			break;
		case 7:
			printf("Slot %d: DDR SDRAM\n", i);
			PrintDDR(rawSpd);
			break;
		case 8:
			printf("Slot %d: DDR2 SDRAM\n", i);
			PrintDDR2(rawSpd);
			break;
		case 9:
			printf("Slot %d: DDR2 SDRAM FB-DIMM\n", i);
			break;
		case 10:
			printf("Slot %d: DDR2 SDRAM FB-DIMM PROBE\n", i);
			break;
		case 11:
			printf("Slot %d: DDR3 SDRAM\n", i);
			PrintDDR3(rawSpd);
			break;
		case 12:
			printf("Slot %d: DDR4 SDRAM\n", i);
			PrintDDR4(rawSpd);
			break;
		case 14:
			printf("Slot %d: DDR4E SDRAM\n", i);
			break;
		case 15:
			printf("Slot %d: LPDDR3 SDRAM\n", i);
			break;
		case 16:
			printf("Slot %d: LPDDR4 SDRAM\n", i);
			break;
		default:
			printf("Slot %d: UNKNOWN\n", i);
		}
		//PrintSpd(rawSpd);
	}
	SpdFini();
}
