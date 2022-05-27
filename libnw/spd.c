// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "libnw.h"
#include "utils.h"
#include "spd.h"

#if 0
static int Parity(int value)
{
	value ^= value >> 16;
	value ^= value >> 8;
	value ^= value >> 4;
	value &= 0xf;
	return (0x6996 >> value) & 1;
}
#endif

static const CHAR*
DDR34ModuleType(UINT8 Type)
{
	Type &= 0x0F;
	switch (Type)
	{
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
	switch (Type)
	{
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

static const char* mem_human_sizes[6] =
{ "MB", "GB", "TB", "PB", "EB", "ZB", };

static const CHAR*
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
	return NWL_GetHumanSize(Size, mem_human_sizes, 1024);
}

static const CHAR*
DDR3Capacity(UINT8* rawSpd)
{
	UINT64 Size = 0;
	UINT64 sdrCapacity = 256ULL << (rawSpd[4] & 0x0FU);
	UINT32 sdrWidth = 4U << (rawSpd[7] & 0x07U);
	UINT32 busWidth = 8U << (rawSpd[8] & 0x07U);
	UINT32 Ranks = 1 + ((rawSpd[7] >> 3) & 0x07U);
	Size = sdrCapacity / 8 * busWidth / sdrWidth * Ranks;
	return NWL_GetHumanSize(Size, mem_human_sizes, 1024);
}

static const CHAR*
DDR2Capacity(UINT8* rawSpd)
{
	UINT64 Size = 0;
	INT i, k;
	i = (rawSpd[3] & 0x0FU) + (rawSpd[4] & 0x0FU) - 17;
	k = ((rawSpd[5] & 0x07U) + 1) * rawSpd[17];
	if (i > 0 && i <= 12 && k > 0)
		Size = (1ULL << i) * k;
	return NWL_GetHumanSize(Size, mem_human_sizes, 1024);
}

static const CHAR*
DDRCapacity(UINT8* rawSpd)
{
	UINT64 Size = 0;
	INT i, k;
	i = (rawSpd[3] & 0x0FU) + (rawSpd[4] & 0x0FU) - 17;
	k = (rawSpd[5] <= 8 && rawSpd[17] <= 8) ? rawSpd[5] * rawSpd[17] : 0;
	if (i > 0 && i <= 12 && k > 0)
		Size = (1ULL << i) * k;
	return NWL_GetHumanSize(Size, mem_human_sizes, 1024);
}

static const CHAR*
DDR345Manufacturer(UINT8 Lsb, UINT8 Msb)
{
	UINT Bank = 0, Index = 0;
	if (Msb == 0x00 || Msb == 0xFF)
		return "Unknown";
#if 0
	if (Parity(Lsb) != 1 || Parity(Msb) != 1)
		return "Invalid";
#endif
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
DDRManufacturer(UINT8* Raw)
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
#if 0
	if (Parity(First) != 1)
		return "Invalid";
#endif
	return JEDEC_MFG_STR(i - 1, (First & 0x7FU) - 1);
}

static const CHAR*
DDR2345Date(UINT8 rawYear, UINT8 rawWeek)
{
	UINT32 Year = 0, Week = 0;
	static CHAR Date[] = "Week52/2021";
	if (rawYear == 0x0 || rawYear == 0xff ||
		rawWeek == 0x0 || rawWeek == 0xff)
	{
		return "-";
	}
	Week = ((rawWeek >> 4) & 0x0FU) * 10U + (rawWeek & 0x0FU);
	Year = ((rawYear >> 4) & 0x0FU) * 10U + (rawYear & 0x0FU) + 2000;
	snprintf(Date, sizeof(Date), "Week%02u/%04u", Week, Year);
	return Date;
}

static const CHAR*
DDRDate(UINT8 rawYear, UINT8 rawWeek)
{
	UINT32 Year = 0, Week = 0;
	static CHAR Date[] = "Week52/1990";
	if (rawYear == 0x0 || rawYear == 0xff ||
		rawWeek == 0x0 || rawWeek == 0xff)
	{
		return "-";
	}
	Week = ((rawWeek >> 4) & 0x0FU) * 10U + (rawWeek & 0x0FU);
	Year = ((rawYear >> 4) & 0x0FU) * 10U + (rawYear & 0x0FU) + 1900;
	snprintf(Date, sizeof(Date), "Week%02u/%04u", Week, Year);
	return Date;
}

static UINT32
DDR4Speed(UINT8* rawSpd)
{
	switch (rawSpd[18])
	{
	case 0x05: return 3200;
	case 0x06: return 2666;
	case 0x07: return 2400;
	case 0x08: return 2133;
	case 0x09: return 1866;
	case 0x0A: return 1600;
	}
	return 0;
}

static UINT32
DDR3Speed(UINT8* rawSpd)
{
	switch (rawSpd[12])
	{
	case 0x05: return 2666;
	case 0x06: return 2533;
	case 0x07: return 2400;
	case 0x08: return 2133;
	case 0x09: return 1866;
	case 0x0A: return 1600;
	case 0x0C: return 1333;
	case 0x0F: return 1066;
	case 0x14: return 800;
	}
	return 0;
}

static UINT32
DDRSpeed(UINT8* rawSpd)
{
	return 2 * 10000 / ((rawSpd[9] >> 4) * 10 + (rawSpd[9] & 0x0F));
}

static void
PrintDDR5(PNODE nd, UINT8* rawSpd)
{
	UINT i = 0;
	NWL_NodeAttrSetf(nd, "Revision", 0, "%u.%u", rawSpd[1] >> 4, rawSpd[1] & 0x0FU);
#if 0
	NWL_NodeAttrSet(nd, "Manufacturer", DDR345Manufacturer(rawSpd[512], rawSpd[513]), 0);
	NWL_NodeAttrSet(nd, "Date", DDR2345Date(rawSpd[515], rawSpd[516]), 0);
	NWLC->NwBuf[0] = '\0';
	for (i = 0; i < 4; i++)
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s%02X", NWLC->NwBuf, rawSpd[517 + i]);
	NWL_NodeAttrSet(nd, "Serial", NWLC->NwBuf, 0);
	NWLC->NwBuf[0] = '\0';
	for (i = 0; i < 20; i++)
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s%c", NWLC->NwBuf, rawSpd[521 + i]);
	NWL_NodeAttrSet(nd, "Part", NWLC->NwBuf, 0);
#endif
}

static void
PrintDDR4(PNODE nd, UINT8* rawSpd)
{
	UINT i = 0;
	NWL_NodeAttrSetf(nd, "Revision", 0, "%u.%u", rawSpd[1] >> 4, rawSpd[1] & 0x0FU);
	NWL_NodeAttrSetf(nd, "Module Type", 0, "%s%s", DDR34ModuleType(rawSpd[3]), (rawSpd[13] & 0x08U) ? " (ECC)" : "");
	NWL_NodeAttrSet(nd, "Capacity", DDR4Capacity(rawSpd), 0);
	NWL_NodeAttrSetf(nd, "Speed (MHz)", NAFLG_FMT_NUMERIC, "%u", DDR4Speed(rawSpd));
	NWL_NodeAttrSet(nd, "Voltage", (rawSpd[11] & 0x01U) ? "1.2 V" : "(Unknown)", 0);
	NWL_NodeAttrSet(nd, "Manufacturer", DDR345Manufacturer(rawSpd[320], rawSpd[321]), 0);
	NWL_NodeAttrSet(nd, "Date", DDR2345Date(rawSpd[323], rawSpd[324]), 0);
	NWLC->NwBuf[0] = '\0';
	for (i = 0; i < 4; i++)
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s%02X", NWLC->NwBuf, rawSpd[325 + i]);
	NWL_NodeAttrSet(nd, "Serial", NWLC->NwBuf, 0);
	NWLC->NwBuf[0] = '\0';
	for (i = 0; i < 20; i++)
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s%c", NWLC->NwBuf, rawSpd[329 + i]);
	NWL_NodeAttrSet(nd, "Part", NWLC->NwBuf, 0);
}

static void
PrintDDR3(PNODE nd, UINT8* rawSpd)
{
	UINT i = 0;
	NWL_NodeAttrSetf(nd, "Revision", 0, "%u.%u", rawSpd[1] >> 4, rawSpd[1] & 0x0FU);
	NWL_NodeAttrSetf(nd, "Module Type", 0, "%s%s", DDR34ModuleType(rawSpd[3]), (rawSpd[8] >> 3 == 1) ? " (ECC)" : "");
	NWL_NodeAttrSet(nd, "Capacity", DDR3Capacity(rawSpd), 0);
	NWL_NodeAttrSetf(nd, "Speed (MHz)", NAFLG_FMT_NUMERIC, "%u", DDR3Speed(rawSpd));
	NWL_NodeAttrSetf(nd, "Supported Voltages", 0, "%s%s%s", (rawSpd[6] & 0x04U) ? " 1.25V" : "",
		(rawSpd[6] & 0x02U) ? " 1.35V" : "", (rawSpd[6] & 0x01U) ? "" : " 1.5V");
	NWL_NodeAttrSet(nd, "Manufacturer", DDR345Manufacturer(rawSpd[117], rawSpd[118]), 0);
	NWL_NodeAttrSet(nd, "Date", DDR2345Date(rawSpd[120], rawSpd[121]), 0);
	NWLC->NwBuf[0] = '\0';
	for (i = 0; i < 4; i++)
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s%02X", NWLC->NwBuf, rawSpd[122 + i]);
	NWL_NodeAttrSet(nd, "Serial", NWLC->NwBuf, 0);
	NWLC->NwBuf[0] = '\0';
	for (i = 0; i < 20; i++)
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s%c", NWLC->NwBuf, rawSpd[128 + i]);
	NWL_NodeAttrSet(nd, "Part", NWLC->NwBuf, 0);
}

static void
PrintDDR2(PNODE nd, UINT8* rawSpd)
{
	UINT i = 0;
	NWL_NodeAttrSetf(nd, "Revision", 0, "%u.%u", rawSpd[1] >> 4, rawSpd[1] & 0x0FU);
	NWL_NodeAttrSetf(nd, "Module Type", 0, "%s%s", DDR2ModuleType(rawSpd[3]), (rawSpd[11] >> 1 == 1) ? " (ECC)" : "");
	NWL_NodeAttrSet(nd, "Capacity", DDR2Capacity(rawSpd), 0);
	NWL_NodeAttrSetf(nd, "Speed (MHz)", NAFLG_FMT_NUMERIC, "%u", DDRSpeed(rawSpd));
	NWL_NodeAttrSet(nd, "Manufacturer", DDRManufacturer(rawSpd + 64), 0);
	NWL_NodeAttrSet(nd, "Date", DDR2345Date(rawSpd[93], rawSpd[94]), 0);
	NWLC->NwBuf[0] = '\0';
	for (i = 0; i < 4; i++)
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s%02X", NWLC->NwBuf, rawSpd[95 + i]);
	NWL_NodeAttrSet(nd, "Serial", NWLC->NwBuf, 0);
	NWLC->NwBuf[0] = '\0';
	for (i = 0; i < 18; i++)
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s%c", NWLC->NwBuf, rawSpd[73 + i]);
	NWL_NodeAttrSet(nd, "Part", NWLC->NwBuf, 0);
}

static void
PrintDDR(PNODE nd, UINT8* rawSpd)
{
	UINT i = 0;
	NWL_NodeAttrSetf(nd, "Revision", 0, "%u.%u", rawSpd[1] >> 4, rawSpd[1] & 0x0FU);
	NWL_NodeAttrSet(nd, "Capacity", DDRCapacity(rawSpd), 0);
	NWL_NodeAttrSetf(nd, "Speed (MHz)", NAFLG_FMT_NUMERIC, "%u", DDRSpeed(rawSpd));
	NWL_NodeAttrSet(nd, "Manufacturer", DDRManufacturer(rawSpd + 64), 0);
	NWL_NodeAttrSet(nd, "Date", DDRDate(rawSpd[93], rawSpd[94]), 0);
	NWLC->NwBuf[0] = '\0';
	for (i = 0; i < 4; i++)
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s%02X", NWLC->NwBuf, rawSpd[95 + i]);
	NWL_NodeAttrSet(nd, "Serial", NWLC->NwBuf, 0);
	NWLC->NwBuf[0] = '\0';
	for (i = 0; i < 18; i++)
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s%c", NWLC->NwBuf, rawSpd[73 + i]);
	NWL_NodeAttrSet(nd, "Part", NWLC->NwBuf, 0);
}

PNODE NW_Spd(VOID)
{
	int i = 0;
	UINT8* rawSpd = NULL;
	PNODE node = NWL_NodeAlloc("SPD", NFLG_TABLE);
	BOOL saved_human_size = NWLC->HumanSize;
	if (NWLC->SpdInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	NWLC->HumanSize = TRUE;
	NWL_SpdInit();
	for (i = 0; i < 8; i++)
	{
		PNODE nspd = NWL_NodeAppendNew(node, "Slot", NFLG_TABLE_ROW);
		NWL_NodeAttrSetf(nspd, "ID", NAFLG_FMT_NUMERIC, "%d", i);
		rawSpd = NWL_SpdGet(i);
		if (!rawSpd)
		{
			continue;
		}
		switch (rawSpd[2])
		{
		case 4:
			NWL_NodeAttrSet(nspd, "Memory Type", "SDRAM", 0);
			PrintDDR(nspd, rawSpd);
			break;
		case 5:
			NWL_NodeAttrSet(nspd, "Memory Type", "ROM", 0);
			break;
		case 6:
			NWL_NodeAttrSet(nspd, "Memory Type", "DDR SGRAM", 0);
			break;
		case 7:
			NWL_NodeAttrSet(nspd, "Memory Type", "DDR SDRAM", 0);
			PrintDDR(nspd, rawSpd);
			break;
		case 8:
			NWL_NodeAttrSet(nspd, "Memory Type", "DDR2 SDRAM", 0);
			PrintDDR2(nspd, rawSpd);
			break;
		case 9:
			NWL_NodeAttrSet(nspd, "Memory Type", "DDR2 SDRAM FB-DIMM", 0);
			PrintDDR2(nspd, rawSpd);
			break;
		case 10:
			NWL_NodeAttrSet(nspd, "Memory Type", "DDR2 SDRAM FB-DIMM PROBE", 0);
			PrintDDR2(nspd, rawSpd);
			break;
		case 11:
			NWL_NodeAttrSet(nspd, "Memory Type", "DDR3 SDRAM", 0);
			PrintDDR3(nspd, rawSpd);
			break;
		case 12:
			NWL_NodeAttrSet(nspd, "Memory Type", "DDR4 SDRAM", 0);
			PrintDDR4(nspd, rawSpd);
			break;
		case 14:
			NWL_NodeAttrSet(nspd, "Memory Type", "DDR4E SDRAM", 0);
			PrintDDR4(nspd, rawSpd);
			break;
		case 15:
			NWL_NodeAttrSet(nspd, "Memory Type", "LPDDR3 SDRAM", 0);
			PrintDDR4(nspd, rawSpd);
			break;
		case 16:
			NWL_NodeAttrSet(nspd, "Memory Type", "LPDDR4 SDRAM", 0);
			PrintDDR4(nspd, rawSpd);
			break;
		case 18:
			NWL_NodeAttrSet(nspd, "Memory Type", "DDR5 SDRAM", 0);
			PrintDDR5(nspd, rawSpd);
			break;
		default:
			NWL_NodeAttrSet(nspd, "Memory Type", "UNKNOWN", 0);
		}
	}
	NWL_SpdFini();
	NWLC->HumanSize = saved_human_size;
	return node;
}
