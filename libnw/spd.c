// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "libnw.h"
#include "utils.h"
#include "smbus.h"

// https://www.ti.com/lit/ug/smmu001/smmu001.pdf
// https://github.com/hardinfo2/hardinfo2/blob/master/modules/devices/spd-decode.c
// https://github.com/memtest86plus/memtest86plus/blob/main/system/spd.c

#define SPD_SKU_LEN             32
#define SPD_SN_LEN              4

#define DDR5_ROUNDING_FACTOR    30
#define DDR4_ROUNDING_FACTOR    0.9f

static const CHAR*
DDR5ModuleType(UINT8 Type)
{
	Type &= 0x0F;
	switch (Type)
	{
	case 1: return "RDIMM";
	case 2: return "UDIMM";
	case 3: return "SO-DIMM";
	case 4: return "LRDIMM";
	case 5: return "CUDIMM";
	case 6: return "CSOUDIMM";
	case 7: return "MRDIMM";
	case 8: return "CAMM2";
	case 10: return "DDIM";
	case 11: return "Soldered";
	}
	return "UNKNOWN";
}

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

static const CHAR*
DDR5Capacity(UINT8* rawSpd)
{
	UINT64 moduleSize = 0;
	UINT32 channelsPerDimm = (((rawSpd[235] >> 5) & 3) == 1) ? 2 : 1;
	UINT32 busWidthPerChannel = 1U << ((rawSpd[235] & 3) + 3);
	UINT32 pkgRanksPerChannel = 1U << ((rawSpd[234] >> 3) & 7);

	for (int i = 1; i <= 2; i++)
	{
		UINT32 curRank = 0;
		UINT8 sb = rawSpd[i * 4];
		// SDRAM Density per die
		switch (sb & 0x1F)
		{
		case 0b00001:
			curRank = 512;
			break;
		case 0b00010:
			curRank = 1024;
			break;
		case 0b00011:
			curRank = 1536;
			break;
		case 0b00100:
			curRank = 2048;
			break;
		case 0b00101:
			curRank = 3072;
			break;
		case 0b00110:
			curRank = 4096;
			break;
		case 0b00111:
			curRank = 6144;
			break;
		case 0b01000:
			curRank = 8192;
			break;
		}

		// Die per package
		if ((sb >> 5) > 1 && (sb >> 5) <= 5)
			curRank *= 1U << (((sb >> 5) & 7) - 1);

		curRank *= channelsPerDimm;
		curRank *= busWidthPerChannel;

		// I/O Width
		sb = rawSpd[i * 4 + 2];
		curRank /= 1U << (((sb >> 5) & 3) + 2);

		curRank *= pkgRanksPerChannel;

		moduleSize += curRank;

		// If not Asymmetrical, don't process the second rank
		if ((sb >> 6) == 0)
			break;
	}
	return NWL_GetHumanSize(moduleSize << 20, NWLC->NwUnits, 1024);
}

static const CHAR*
DDR4Capacity(UINT8* rawSpd)
{
	UINT64 moduleSize = 0;
	UINT64 ramSize = (rawSpd[4] & 0x0F) + 5;
	UINT64 busWidth = (rawSpd[13] & 0x07) + 3;
	UINT64 devWidth = (rawSpd[12] & 0x07) + 2;
	UINT64 numRanks = (rawSpd[12] >> 3) & 0x07;
	UINT64 dieCount = (rawSpd[6] >> 4) & 0x07;
	if (ramSize + busWidth + numRanks + dieCount > devWidth)
		moduleSize = 1ULL << (ramSize + busWidth - devWidth + numRanks + dieCount);
	return NWL_GetHumanSize(moduleSize << 20, NWLC->NwUnits, 1024);
}

static const CHAR*
DDR3Capacity(UINT8* rawSpd)
{
	UINT64 moduleSize = 0;
	UINT64 ramSize = (rawSpd[4] & 0x0F) + 5;
	UINT64 busWidth = (rawSpd[8] & 0x07) + 3;
	UINT64 devWidth = (rawSpd[7] & 0x07) + 2;
	UINT64 numRanks = ((rawSpd[7] >> 3) & 0x07) + 1;
	if (ramSize + busWidth > devWidth)
		moduleSize = 1ULL << (ramSize + busWidth - devWidth);
	moduleSize *= numRanks;
	return NWL_GetHumanSize(moduleSize << 20, NWLC->NwUnits, 1024);
}

static const CHAR*
DDR2Capacity(UINT8* rawSpd)
{
	UINT64 moduleSize = 0;
	switch (rawSpd[31])
	{
	case 1: moduleSize = 1024; break;
	case 2: moduleSize = 2048; break;
	case 4: moduleSize = 4096; break;
	case 8: moduleSize = 8192; break;
	case 16: moduleSize = 16384; break;
	case 32: moduleSize = 128; break;
	case 64: moduleSize = 256; break;
	case 128: moduleSize = 512; break;
	}
	moduleSize *= (rawSpd[5] & 0x07U) + 1U;
	return NWL_GetHumanSize(moduleSize << 20, NWLC->NwUnits, 1024);
}

static const CHAR*
DDR1Capacity(UINT8* rawSpd)
{
	UINT64 moduleSize = 0;
	switch (rawSpd[31])
	{
	case 1: moduleSize = 1024; break;
	case 2: moduleSize = 2048; break;
	case 4: moduleSize = 4096; break;
	case 8: moduleSize = 32; break;
	case 16: moduleSize = 64; break;
	case 32: moduleSize = 128; break;
	case 64: moduleSize = 256; break;
	case 128: moduleSize = 512; break;
	}
	moduleSize *= rawSpd[5];
	return NWL_GetHumanSize(moduleSize << 20, NWLC->NwUnits, 1024);
}

static LPCSTR
SDRCapacity(UINT8* rawSpd)
{
	UINT32 numRow = rawSpd[3] & 0x0F;
	UINT32 numCol = rawSpd[4] & 0x0F;
	UINT32 numModBanks = rawSpd[5];
	UINT32 numDevBanks = rawSpd[17];
	UINT64 moduleSize = 0;

	if (numRow != 0 && numCol != 0
		&& (numRow + numCol) > 17 && (numRow + numCol) <= 29
		&& numModBanks <= 8 && numDevBanks <= 8)
	{
		moduleSize = (1ULL << (numRow + numCol - 17)) * (numModBanks * numDevBanks); // MB
	}

	return NWL_GetHumanSize(moduleSize << 20, NWLC->NwUnits, 1024);
}

static void
DDR345Manufacturer(PNODE nd, UINT8 Lsb, UINT8 Msb, CHAR* Ids, DWORD IdsSize)
{
	if (Msb == 0x00 || Msb == 0xFF)
	{
		NWL_NodeAttrSet(nd, "Manufacturer", "UNKNOWN", 0);
		return;
	}
	NWL_GetSpdManufacturer(nd, Ids, IdsSize, Lsb & 0x7f, Msb & 0x7f);
}

static void
SDRDDR12Manufacturer(PNODE nd, UINT8* rawSpd, CHAR* Ids, DWORD IdsSize)
{
	UINT8 contCode;
	for (contCode = 64; contCode < 72; contCode++)
	{
		if (rawSpd[contCode] != 0x7F)
			break;
	}
	NWL_GetSpdManufacturer(nd, Ids, IdsSize, contCode - 64, rawSpd[contCode] & 0x7FU);
}

static LPCSTR
SDRDDR12345SKU(UINT8* rawSpd, UINT16 skuOffset, UINT8 maxLen)
{
	static CHAR sku[SPD_SKU_LEN];
	UINT8 skuLen;
	if (maxLen > SPD_SKU_LEN)
		maxLen = SPD_SKU_LEN;
	for (skuLen = 0; skuLen < maxLen; skuLen++)
	{
		UINT8 skuByte = rawSpd[skuOffset + skuLen];
		if (!isprint(skuByte))
			break;
		sku[skuLen] = (CHAR)skuByte;
	}
	// Trim spaces
	while (skuLen && sku[skuLen - 1] == ' ')
		skuLen--;
	sku[skuLen] = '\0';
	return sku;
}

static LPCSTR
SDRDDR12345SN(UINT8* rawSpd, UINT16 snOffset)
{
	static CHAR sn[18];
	snprintf(sn, sizeof(sn), "%02X%02X%02X%02X",
		rawSpd[snOffset], rawSpd[snOffset + 1], rawSpd[snOffset + 2], rawSpd[snOffset + 3]);
	return sn;
}

static const CHAR*
SDRDDR12345Date(UINT8 rawYear, UINT8 rawWeek)
{
	UINT32 Year = 0, Week = 0;
	static CHAR Date[] = "Week52/20";
	if (rawYear == 0x0 || rawYear == 0xff ||
		rawWeek == 0x0 || rawWeek == 0xff)
	{
		return "-";
	}
	Week = ((rawWeek >> 4) & 0x0FU) * 10U + (rawWeek & 0x0FU);
	Year = ((rawYear >> 4) & 0x0FU) * 10U + (rawYear & 0x0FU);
	snprintf(Date, sizeof(Date), "Week%02u/%02u", Week, Year);
	return Date;
}

static const CHAR*
DDR3Voltage(UINT8* rawSpd)
{
	static CHAR str[24];
	bool add_slash = false;
	ZeroMemory(str, sizeof(str));
	if (rawSpd[6] & 0x04U)
	{
		strcat_s(str, sizeof(str), "1.25V");
		add_slash = true;
	}
	if (rawSpd[6] & 0x02U)
	{
		if (add_slash)
			strcat_s(str, sizeof(str), " / ");
		strcat_s(str, sizeof(str), "1.35V");
		add_slash = true;
	}
	if (rawSpd[6] & 0x01U)
	{
		if (add_slash)
			strcat_s(str, sizeof(str), " / ");
		strcat_s(str, sizeof(str), "1.5V");
		add_slash = true;
	}
	if (!add_slash)
		strcpy_s(str, sizeof(str), "UNKNOWN");
	return str;
}

static const CHAR*
SDRDDR12Voltage(UINT8* rawSpd)
{
	switch (rawSpd[8])
	{
	case 0x0: return "5.0V / TTL";
	case 0x1: return "LVTTL";
	case 0x2: return "HSTL 1.5V";
	case 0x3: return "SSTL 3.3V";
	case 0x4: return "SSTL 2.5V";
	case 0x5: return "SSTL 1.8V";
	}
	return "UNKNOWN";
}

static void
PrintDDR5(PNODE nd, UINT8* rawSpd, CHAR* Ids, DWORD IdsSize)
{
	NWL_NodeAttrSetf(nd, "Revision", 0, "%u.%u", rawSpd[1] >> 4, rawSpd[1] & 0x0FU);
	NWL_NodeAttrSet(nd, "Module Type", DDR5ModuleType(rawSpd[3]), 0);
	NWL_NodeAttrSet(nd, "Capacity", DDR5Capacity(rawSpd), 0);
	NWL_NodeAttrSetBool(nd, "ECC", ((rawSpd[235] >> 3) & 3), 0);
	DDR345Manufacturer(nd, rawSpd[512], rawSpd[513], Ids, IdsSize);
	NWL_NodeAttrSet(nd, "Date", SDRDDR12345Date(rawSpd[515], rawSpd[516]), 0);
	NWL_NodeAttrSet(nd, "Serial Number", SDRDDR12345SN(rawSpd, 517), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(nd, "Part Number", SDRDDR12345SKU(rawSpd, 521, 20), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSetBool(nd, "Thermal Sensor", 1, 0); // ?

	bool xmp = false;
	UINT32 mhzFreq = 0;
	UINT16 tCK = 0;
	UINT16 xmpOffset = 0;
	if (rawSpd[640] == 0x0C && rawSpd[641] == 0x4A)
	{
		for (UINT16 offset = 0; offset < 2 * 64; offset += 64)
		{
			UINT16 tCKtmp = ((UINT16)rawSpd[710 + offset]) << 8 | rawSpd[709 + offset];

			if (tCKtmp == 0 || tCKtmp < 100 || (tCK != 0 && tCKtmp >= tCK))
				continue;

			xmpOffset = offset;
			tCK = tCKtmp;
		}
		xmp = true;
	}
	else
		tCK = ((UINT16)rawSpd[21]) << 8 | rawSpd[20]; // 357

	if (tCK != 0)
	{
		mhzFreq = (UINT32)(1.0f / tCK * 2.0f * 1000.0f * 1000.0f);
		mhzFreq = (mhzFreq + 50) / 100 * 100;
	}
	NWL_NodeAttrSetf(nd, "Speed (MHz)", NAFLG_FMT_NUMERIC, "%u", mhzFreq);

	if (xmp)
	{
		NWL_NodeAttrSet(nd, "XMP", "XMP 3.0", 0);
		NWL_NodeAttrSet(nd, "Voltage", (rawSpd[15] == 0) ? "1.1V" : "UNKNOWN", 0); //FIXME

		UINT16 tCL = 0;
		if (tCK != 0)
		{
			float t = (float)((UINT16)rawSpd[718 + xmpOffset] << 8 | rawSpd[717 + xmpOffset]);
			tCL = (UINT16)((t + tCK - DDR5_ROUNDING_FACTOR) / tCK);
			tCL += tCL % 2; // round to upper even
		}
		NWL_NodeAttrSetf(nd, "tCL", NAFLG_FMT_NUMERIC, "%u", tCL);

		UINT16 tRCD = 0;
		if (tCK != 0)
		{
			float t = (float)((UINT16)rawSpd[720 + xmpOffset] << 8 | rawSpd[719 + xmpOffset]);
			tRCD = (UINT16)((t + tCK - DDR5_ROUNDING_FACTOR) / tCK);
		}
		NWL_NodeAttrSetf(nd, "tRCD", NAFLG_FMT_NUMERIC, "%u", tRCD);

		UINT16 tRP = 0;
		if (tCK != 0)
		{
			float t = (float)((UINT16)rawSpd[722 + xmpOffset] << 8 | rawSpd[721 + xmpOffset]);
			tRP = (UINT16)((t + tCK - DDR5_ROUNDING_FACTOR) / tCK);
		}
		NWL_NodeAttrSetf(nd, "tRP", NAFLG_FMT_NUMERIC, "%u", tRP);

		UINT16 tRAS = 0;
		if (tCK != 0)
		{
			float t = (float)((UINT16)rawSpd[724 + xmpOffset] << 8 | rawSpd[723 + xmpOffset]);
			tRAS = (UINT16)((t + tCK - DDR5_ROUNDING_FACTOR) / tCK);
		}
		NWL_NodeAttrSetf(nd, "tRAS", NAFLG_FMT_NUMERIC, "%u", tRAS);

		UINT16 tRC = 0;
		if (tCK != 0)
		{
			float t = (float)((UINT16)rawSpd[726 + xmpOffset] << 8 | rawSpd[725 + xmpOffset]);
			tRC = (UINT16)((t + tCK - DDR5_ROUNDING_FACTOR) / tCK);
		}
		NWL_NodeAttrSetf(nd, "tRC", NAFLG_FMT_NUMERIC, "%u", tRC);
	}
	else
	{
		NWL_NodeAttrSet(nd, "XMP", "None", 0);
		NWL_NodeAttrSet(nd, "Voltage", (rawSpd[15]  == 0) ? "1.1V" : "UNKNOWN", 0);

		UINT16 tCL = 0;
		if (tCK != 0)
		{
			float t = (float)((UINT16)rawSpd[31] << 8 | rawSpd[30]); // 16000
			tCL = (UINT16)((t + tCK - DDR5_ROUNDING_FACTOR) / tCK);
			tCL += tCL % 2;
		}
		NWL_NodeAttrSetf(nd, "tCL", NAFLG_FMT_NUMERIC, "%u", tCL);

		UINT16 tRCD = 0;
		if (tCK != 0)
		{
			float t = (float)((UINT16)rawSpd[33] << 8 | rawSpd[32]);
			tRCD = (UINT16)((t + tCK - DDR5_ROUNDING_FACTOR) / tCK);
		}
		NWL_NodeAttrSetf(nd, "tRCD", NAFLG_FMT_NUMERIC, "%u", tRCD);

		UINT16 tRP = 0;
		if (tCK != 0)
		{
			float t = (float)((UINT16)rawSpd[35] << 8 | rawSpd[34]);
			tRP = (UINT16)((t + tCK - DDR5_ROUNDING_FACTOR) / tCK);
		}
		NWL_NodeAttrSetf(nd, "tRP", NAFLG_FMT_NUMERIC, "%u", tRP);

		UINT16 tRAS = 0;
		if (tCK != 0)
		{
			float t = (float)((UINT16)rawSpd[37] << 8 | rawSpd[36]);
			tRAS = (UINT16)((t + tCK - DDR5_ROUNDING_FACTOR) / tCK);
		}
		NWL_NodeAttrSetf(nd, "tRAS", NAFLG_FMT_NUMERIC, "%u", tRAS);

		UINT16 tRC = 0;
		if (tCK != 0)
		{
			float t = (float)((UINT16)rawSpd[39] << 8 | rawSpd[38]);
			tRC = (UINT16)((t + tCK - DDR5_ROUNDING_FACTOR) / tCK);
		}
		NWL_NodeAttrSetf(nd, "tRC", NAFLG_FMT_NUMERIC, "%u", tRC);
	}
}

static void
PrintDDR4(PNODE nd, UINT8* rawSpd, CHAR* Ids, DWORD IdsSize)
{
	NWL_NodeAttrSetf(nd, "Revision", 0, "%u.%u", rawSpd[1] >> 4, rawSpd[1] & 0x0FU);
	NWL_NodeAttrSet(nd, "Module Type", DDR34ModuleType(rawSpd[3]), 0);
	NWL_NodeAttrSet(nd, "Capacity", DDR4Capacity(rawSpd), 0);
	NWL_NodeAttrSetBool(nd, "ECC", ((rawSpd[13] >> 3) & 1), 0);
	DDR345Manufacturer(nd, rawSpd[320], rawSpd[321], Ids, IdsSize);
	NWL_NodeAttrSet(nd, "Date", SDRDDR12345Date(rawSpd[323], rawSpd[324]), 0);
	NWL_NodeAttrSet(nd, "Serial Number", SDRDDR12345SN(rawSpd, 325), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(nd, "Part Number", SDRDDR12345SKU(rawSpd, 329, 20), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSetBool(nd, "Thermal Sensor", (rawSpd[14] & 0x80), 0); // ?

	bool xmp = false;
	float mhzFreq = 0.0f;
	float tCK = 0.0f;
	if (rawSpd[384] == 0x0C && rawSpd[385] == 0x4A)
	{
		tCK = (UINT8)rawSpd[396] * 0.125f + (INT8)rawSpd[431] * 0.001f;
		xmp = true;
	}
	else
		tCK = (UINT8)rawSpd[18] * 0.125f + (INT8)rawSpd[125] * 0.001f;

	if (tCK > 0.0f)
	{
		mhzFreq = 1.0f / tCK * 2.0f * 1000.0f;
		// RoundFrequency to nearest x00/x33/x66
		float fround = ((int)(mhzFreq * 0.01f + .5f) / 0.01f) - mhzFreq;
		mhzFreq += fround;
		if (fround < -16.5f)
			mhzFreq += 33.0f;
		else if (fround > 16.5f)
			mhzFreq -= 34.0f;
	}
	NWL_NodeAttrSetf(nd, "Speed (MHz)", NAFLG_FMT_NUMERIC, "%u", (UINT32)mhzFreq);

	if (xmp)
	{
		NWL_NodeAttrSet(nd, "XMP", "XMP 2.0", 0);
		NWL_NodeAttrSetf(nd, "Voltage", 0, "%u.%uV", rawSpd[393] >> 7, rawSpd[393] & 0x7F);

		UINT16 tCL = 0;
		if (tCK > 0.0f)
		{
			float t = (UINT8)rawSpd[401] * 0.125f + (INT8)rawSpd[430] * 0.001f;
			tCL = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tCL", NAFLG_FMT_NUMERIC, "%u", tCL);

		UINT16 tRCD = 0;
		if (tCK > 0.0f)
		{
			float t = (UINT8)rawSpd[402] * 0.125f + (INT8)rawSpd[429] * 0.001f;
			tRCD = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRCD", NAFLG_FMT_NUMERIC, "%u", tRCD);

		UINT16 tRP = 0;
		if (tCK > 0.0f)
		{
			float t = (UINT8)rawSpd[403] * 0.125f + (INT8)rawSpd[428] * 0.001f;
			tRP = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRP", NAFLG_FMT_NUMERIC, "%u", tRP);

		UINT16 tRAS = 0;
		if (tCK > 0.0f)
		{
			float t = (UINT8)rawSpd[405] * 0.125f + (INT8)rawSpd[427] * 0.001f + (UINT8)(rawSpd[404] & 0x0F) * 32.0f;
			tRAS = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRAS", NAFLG_FMT_NUMERIC, "%u", tRAS);

		UINT16 tRC = 0;
		if (tCK > 0.0f)
		{
			float t = (UINT8)rawSpd[406] * 0.125f + (UINT8)(rawSpd[404] >> 4) * 32.0f;
			tRC = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRC", NAFLG_FMT_NUMERIC, "%u", tRC);
	}
	else
	{
		NWL_NodeAttrSet(nd, "XMP", "None", 0);
		NWL_NodeAttrSet(nd, "Voltage", (rawSpd[11] & 0x01U) ? "1.2V" : "UNKNOWN", 0);

		UINT16 tCL = 0;
		if (tCK > 0.0f)
		{
			float t = (UINT8)rawSpd[24] * 0.125f + (INT8)rawSpd[123] * 0.001f;
			tCL = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tCL", NAFLG_FMT_NUMERIC, "%u", tCL);

		UINT16 tRCD = 0;
		if (tCK > 0.0f)
		{
			float t = (UINT8)rawSpd[25] * 0.125f + (INT8)rawSpd[122] * 0.001f;
			tRCD = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRCD", NAFLG_FMT_NUMERIC, "%u", tRCD);

		UINT16 tRP = 0;
		if (tCK > 0.0f)
		{
			float t = (UINT8)rawSpd[26] * 0.125f + (INT8)rawSpd[121] * 0.001f;
			tRP = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRP", NAFLG_FMT_NUMERIC, "%u", tRP);

		UINT16 tRAS = 0;
		if (tCK > 0.0f)
		{
			float t = (UINT8)rawSpd[28] * 0.125f + (UINT8)(rawSpd[27] & 0x0F) * 32.0f;
			tRAS = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRAS", NAFLG_FMT_NUMERIC, "%u", tRAS);

		UINT16 tRC = 0;
		if (tCK > 0.0f)
		{
			float t = (UINT8)rawSpd[29] * 0.125f + (UINT8)(rawSpd[27] >> 4) * 32.0f;
			tRC = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRC", NAFLG_FMT_NUMERIC, "%u", tRC);
	}
}

static void
PrintDDR3(PNODE nd, UINT8* rawSpd, CHAR* Ids, DWORD IdsSize)
{
	NWL_NodeAttrSetf(nd, "Revision", 0, "%u.%u", rawSpd[1] >> 4, rawSpd[1] & 0x0FU);
	NWL_NodeAttrSet(nd, "Module Type", DDR34ModuleType(rawSpd[3]), 0);
	NWL_NodeAttrSet(nd, "Capacity", DDR3Capacity(rawSpd), 0);
	NWL_NodeAttrSetBool(nd, "ECC", ((rawSpd[8] >> 3) & 1), 0);
	DDR345Manufacturer(nd, rawSpd[117], rawSpd[118], Ids, IdsSize);
	NWL_NodeAttrSet(nd, "Date", SDRDDR12345Date(rawSpd[120], rawSpd[121]), 0);
	NWL_NodeAttrSet(nd, "Serial Number", SDRDDR12345SN(rawSpd, 122), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(nd, "Part Number", SDRDDR12345SKU(rawSpd, 128, 18), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(nd, "Voltage", DDR3Voltage(rawSpd), 0);
	NWL_NodeAttrSetBool(nd, "Thermal Sensor", (rawSpd[32] & 0x80), 0); // DDR3 MPR ?

	bool xmp = false;
	UINT32 mhzFreq = 0;
	UINT8 tCKMin = rawSpd[12];
	if (rawSpd[176] == 0x0C && rawSpd[177] == 0x4A)
	{
		if (rawSpd[221] > 5 && rawSpd[221] < rawSpd[186])
			tCKMin = rawSpd[221]; // profile #2
		else
			tCKMin = rawSpd[186]; // profile #1
		xmp = true;
	}

	switch (tCKMin)
	{
	case 20: mhzFreq = 800; break;
	case 15: mhzFreq = 1066; break;
	case 12: mhzFreq = 1333; break;
	case 10: mhzFreq = 1600; break;
	case 9: mhzFreq = 1866; break;
	case 8: mhzFreq = 2133; break;
	case 7: mhzFreq = 2400; break;
	case 6: mhzFreq = 2666; break;
	}

	float tCK = 0.0f;
	float mtb = 0.125f;
	if (xmp)
	{
		NWL_NodeAttrSet(nd, "XMP", "XMP 1.0", 0);

		if (rawSpd[181] != 0)
			mtb = ((float)rawSpd[180]) / rawSpd[181];

		tCK = (float)rawSpd[186];

		if (rawSpd[181] == 12 && tCK == 10.0f)
			mhzFreq = 2400;
		else if (rawSpd[181] == 14 && tCK == 15.0f)
			mhzFreq = 1866;

		if (rawSpd[181] == 8 && mhzFreq >= 1866)
			tCK -= 0.4f;

		tCK *= mtb;

		UINT16 tCL = 0;
		if (tCK > 0.0f)
			tCL = (UINT16)((rawSpd[187] * mtb) / tCK + DDR4_ROUNDING_FACTOR);
		NWL_NodeAttrSetf(nd, "tCL", NAFLG_FMT_NUMERIC, "%u", tCL);

		UINT16 tRCD = 0;
		if (tCK > 0.0f)
			tRCD = (UINT16)((rawSpd[192] * mtb) / tCK + DDR4_ROUNDING_FACTOR);
		NWL_NodeAttrSetf(nd, "tRCD", NAFLG_FMT_NUMERIC, "%u", tRCD);

		UINT16 tRP = 0;
		if (tCK > 0.0f)
			tRP = (UINT16)((rawSpd[191] * mtb) / tCK + DDR4_ROUNDING_FACTOR);
		NWL_NodeAttrSetf(nd, "tRP", NAFLG_FMT_NUMERIC, "%u", tRP);

		UINT16 tRAS = 0;
		if (tCK > 0.0f)
		{
			float t = (float)((((UINT32)rawSpd[194]) & 0x0F) << 8 | rawSpd[195]);
			tRAS = (UINT16)((t * mtb) / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRAS", NAFLG_FMT_NUMERIC, "%u", tRAS);

		UINT16 tRC = 0;
		if (tCK > 0.0f)
		{
			float t = (float)((((UINT32)rawSpd[194]) & 0xF0) << 4 | rawSpd[196]);
			tRC = (UINT16)((t * mtb) / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRC", NAFLG_FMT_NUMERIC, "%u", tRC);
	}
	else
	{
		NWL_NodeAttrSet(nd, "XMP", "None", 0);

		tCK = (UINT8)rawSpd[12] * mtb + (INT8)rawSpd[34] * 0.001f;

		UINT16 tCL = 0;
		if (tCK > 0.0f)
		{
			float t = (UINT8)rawSpd[16] * mtb + (INT8)rawSpd[35] * 0.001f;
			tCL = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tCL", NAFLG_FMT_NUMERIC, "%u", tCL);

		UINT16 tRCD = 0;
		if (tCK > 0.0f)
		{
			float t = (UINT8)rawSpd[18] * mtb + (INT8)rawSpd[36] * 0.001f;
			tRCD = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRCD", NAFLG_FMT_NUMERIC, "%u", tRCD);

		UINT16 tRP = 0;
		if (tCK > 0.0f)
		{
			float t = (UINT8)rawSpd[20] * mtb + (INT8)rawSpd[37] * 0.001f;
			tRP = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRP", NAFLG_FMT_NUMERIC, "%u", tRP);

		UINT16 tRAS = 0;
		if (tRAS > 0.0f)
		{
			float t = (UINT8)rawSpd[22] * mtb + (UINT8)(rawSpd[21] & 0x0F) * 32.0f;
			tRAS = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRAS", NAFLG_FMT_NUMERIC, "%u", tRAS);

		UINT16 tRC = 0;
		if (tRC > 0.0f)
		{
			float t = (UINT8)rawSpd[23] * mtb + (UINT8)(rawSpd[21] >> 4) * 32.0f + 1;
			tRC = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRC", NAFLG_FMT_NUMERIC, "%u", tRC);
	}
	NWL_NodeAttrSetf(nd, "Speed (MHz)", NAFLG_FMT_NUMERIC, "%u", mhzFreq);
}

static void
PrintDDR2(PNODE nd, UINT8* rawSpd, CHAR* Ids, DWORD IdsSize)
{
	NWL_NodeAttrSetf(nd, "Revision", 0, "%u.%u", rawSpd[62] >> 4, rawSpd[62] & 0x0FU);
	NWL_NodeAttrSet(nd, "Module Type", DDR2ModuleType(rawSpd[3]), 0);
	NWL_NodeAttrSet(nd, "Capacity", DDR2Capacity(rawSpd), 0);
	NWL_NodeAttrSetBool(nd, "ECC", (rawSpd[11] & 0x02), 0);
	SDRDDR12Manufacturer(nd, rawSpd, Ids, IdsSize);
	NWL_NodeAttrSet(nd, "Date", SDRDDR12345Date(rawSpd[93], rawSpd[94]), 0);
	NWL_NodeAttrSet(nd, "Serial Number", SDRDDR12345SN(rawSpd, 95), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(nd, "Part Number", SDRDDR12345SKU(rawSpd, 73, 18), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(nd, "Voltage", SDRDDR12Voltage(rawSpd), 0);
	NWL_NodeAttrSetBool(nd, "Thermal Sensor", 0, 0);

	bool epp = false;
	UINT8 eppOffset = 0, tByte = 0;
	UINT32 mhzFreq = 0;
	float tCK = 0.0f;

	if (rawSpd[99] == 0x6D && rawSpd[102] == 0xB1)
	{
		eppOffset = (rawSpd[103] & 0x03) * 12;
		tByte = rawSpd[109 + eppOffset];
		epp = true;
	}
	else
		tByte = rawSpd[9];
	tCK = (float)((tByte & 0xF0) >> 4);
	tByte &= 0x0F;
	if (tByte < 10)
		tCK += (tByte & 0xF) * 0.1f;
	else if (tByte == 10)
		tCK += 0.25f;
	else if (tByte == 11)
		tCK += 0.33f;
	else if (tByte == 12)
		tCK += 0.66f;
	else if (tByte == 13)
		tCK += 0.75f;
	else if (tByte == 14)
		tCK += 0.875f;

	if (tCK > 0.0f)
		mhzFreq = (UINT32)(1.0f / tCK * 1000.0f * 2.0f);
	NWL_NodeAttrSetf(nd, "Speed (MHz)", NAFLG_FMT_NUMERIC, "%u", mhzFreq);

	if (epp)
	{
		NWL_NodeAttrSet(nd, "XMP", "EPP", 0);
		UINT16 tCL = 0;
		tByte = rawSpd[110 + eppOffset];
		for (UINT16 shft = 0; shft < 7; shft++)
		{
			if ((tByte >> shft) & 1)
				tCL = shft;
		}
		NWL_NodeAttrSetf(nd, "tCL", NAFLG_FMT_NUMERIC, "%u", tCL);

		UINT16 tRCD = 0;
		if (tCK > 0.0f)
		{
			tByte = rawSpd[111 + eppOffset];
			float t = ((tByte & 0xFC) >> 2) + (tByte & 0x03) * 0.25f;
			tRCD = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRCD", NAFLG_FMT_NUMERIC, "%u", tRCD);

		UINT16 tRP = 0;
		if (tCK > 0.0f)
		{
			tByte = rawSpd[112 + eppOffset];
			float t = ((tByte & 0xFC) >> 2) + (tByte & 0x03) * 0.25f;
			tRP = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRP", NAFLG_FMT_NUMERIC, "%u", tRP);

		UINT16 tRAS = 0;
		if (tCK > 0.0f)
			tRAS = (UINT16)(rawSpd[113 + eppOffset] / tCK + DDR4_ROUNDING_FACTOR);
		NWL_NodeAttrSetf(nd, "tRAS", NAFLG_FMT_NUMERIC, "%u", tRAS);
	}
	else
	{
		NWL_NodeAttrSet(nd, "XMP", "None", 0);
		UINT16 tCL = 0;
		tByte = rawSpd[18];
		for (UINT16 shft = 0; shft < 7; shft++)
		{
			if ((tByte >> shft) & 1)
				tCL = shft;
		}
		NWL_NodeAttrSetf(nd, "tCL", NAFLG_FMT_NUMERIC, "%u", tCL);

		UINT16 tRCD = 0;
		if (tCK > 0.0f)
		{
			float t = ((rawSpd[29] & 0xFC) >> 2) + (rawSpd[29] & 0x03) * 0.25f;
			tRCD = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRCD", NAFLG_FMT_NUMERIC, "%u", tRCD);

		UINT16 tRP = 0;
		if (tCK > 0.0f)
		{
			float t = ((rawSpd[27] & 0xFC) >> 2) + (rawSpd[27] & 0x3) * 0.25f;
			tRP = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
		}
		NWL_NodeAttrSetf(nd, "tRP", NAFLG_FMT_NUMERIC, "%u", tRP);

		UINT16 tRAS = 0;
		if (tCK > 0.0f)
			tRAS = (UINT16)(rawSpd[30] / tCK + DDR4_ROUNDING_FACTOR);
		NWL_NodeAttrSetf(nd, "tRAS", NAFLG_FMT_NUMERIC, "%u", tRAS);
	}

}

static void
PrintDDR1(PNODE nd, UINT8* rawSpd, CHAR* Ids, DWORD IdsSize)
{
	NWL_NodeAttrSetf(nd, "Revision", 0, "%u.%u", rawSpd[62] >> 4, rawSpd[62] & 0x0FU);
	NWL_NodeAttrSet(nd, "Capacity", DDR1Capacity(rawSpd), NAFLG_FMT_HUMAN_SIZE);
	NWL_NodeAttrSetBool(nd, "ECC", (rawSpd[11] & 0x02), 0);
	SDRDDR12Manufacturer(nd, rawSpd, Ids, IdsSize);
	NWL_NodeAttrSet(nd, "Date", SDRDDR12345Date(rawSpd[93], rawSpd[94]), 0);
	NWL_NodeAttrSet(nd, "Serial Number", SDRDDR12345SN(rawSpd, 95), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(nd, "Part Number", SDRDDR12345SKU(rawSpd, 73, 18), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(nd, "Voltage", SDRDDR12Voltage(rawSpd), 0);
	NWL_NodeAttrSetBool(nd, "Thermal Sensor", 0, 0);

	UINT32 mhzFreq = 0;
	float tCK = (rawSpd[9] >> 4) + (rawSpd[9] & 0x0F) * 0.1f;

	if (tCK > 0.0f)
		mhzFreq = (UINT32)(1.0f / tCK * 1000.0f * 2.0f);
	NWL_NodeAttrSetf(nd, "Speed (MHz)", NAFLG_FMT_NUMERIC, "%u", mhzFreq);

	float tCL = 0.0f;
	for (int shft = 0; shft < 7; shft++)
	{
		if ((rawSpd[18] >> shft) & 1)
			tCL = 1.0f + shft * 0.5f;
	}
	NWL_NodeAttrSetf(nd, "tCL", NAFLG_FMT_NUMERIC, "%.1f", tCL);

	UINT16 tRCD = 0;
	if (tCK > 0.0f)
	{
		float t = (rawSpd[29] >> 2) + (rawSpd[29] & 0x03) * 0.25f;
		tRCD = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
	}
	NWL_NodeAttrSetf(nd, "tRCD", NAFLG_FMT_NUMERIC, "%u", tRCD);

	UINT16 tRP = 0;
	if (tCK > 0.0f)
	{
		float t = (rawSpd[27] >> 2) + (rawSpd[27] & 0x03) * 0.25f;
		tRP = (UINT16)(t / tCK + DDR4_ROUNDING_FACTOR);
	}
	NWL_NodeAttrSetf(nd, "tRP", NAFLG_FMT_NUMERIC, "%u", tRP);

	UINT16 tRAS = 0;
	if (tCK > 0.0f)
		tRAS = (UINT16)(rawSpd[30] / tCK + DDR4_ROUNDING_FACTOR);
	NWL_NodeAttrSetf(nd, "tRAS", NAFLG_FMT_NUMERIC, "%u", tRAS);
}

static void
PrintSDR(PNODE nd, UINT8* rawSpd, CHAR* Ids, DWORD IdsSize)
{
	NWL_NodeAttrSetf(nd, "Revision", 0, "%u.%u", rawSpd[62] >> 4, rawSpd[62] & 0x0FU);
	NWL_NodeAttrSet(nd, "Capacity", SDRCapacity(rawSpd), NAFLG_FMT_HUMAN_SIZE);
	NWL_NodeAttrSetBool(nd, "ECC", (rawSpd[11] & 0x02), 0);
	SDRDDR12Manufacturer(nd, rawSpd, Ids, IdsSize);
	NWL_NodeAttrSet(nd, "Date", SDRDDR12345Date(rawSpd[93], rawSpd[94]), 0);
	NWL_NodeAttrSet(nd, "Serial Number", SDRDDR12345SN(rawSpd, 95), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(nd, "Part Number", SDRDDR12345SKU(rawSpd, 73, 18), NAFLG_FMT_SENSITIVE);
	NWL_NodeAttrSet(nd, "Voltage", SDRDDR12Voltage(rawSpd), 0);

	UINT32 mhzFreq = 0;
	float tCK = (rawSpd[9] >> 4) + (rawSpd[9] & 0x0F) * 0.1f;

	if (tCK > 0.0f)
		mhzFreq = (UINT32)(1000.0f / tCK);
	NWL_NodeAttrSetf(nd, "Speed (MHz)", NAFLG_FMT_NUMERIC, "%u", mhzFreq);

	UINT16 tCL = 0;
	for (int shft = 0; shft < 7; shft++)
	{
		if ((rawSpd[18] >> shft) & 1)
			tCL = shft + 1;
	}
	NWL_NodeAttrSetf(nd, "tCL", NAFLG_FMT_NUMERIC, "%u", tCL);

	UINT16 tRCD = 0;
	if (tCK > 0.0f)
		tRCD = (UINT16)(rawSpd[29] / tCK + DDR4_ROUNDING_FACTOR);
	NWL_NodeAttrSetf(nd, "tRCD", NAFLG_FMT_NUMERIC, "%u", tRCD);

	UINT16 tRP = 0;
	if (tCK > 0.0f)
		tRP = (UINT16)(rawSpd[27] / tCK + DDR4_ROUNDING_FACTOR);
	NWL_NodeAttrSetf(nd, "tRP", NAFLG_FMT_NUMERIC, "%u", tRP);

	UINT16 tRAS = 0;
	if (tCK > 0.0f)
		tRAS = (UINT16)(rawSpd[30] / tCK + DDR4_ROUNDING_FACTOR);
	NWL_NodeAttrSetf(nd, "tRAS", NAFLG_FMT_NUMERIC, "%u", tRAS);
}

static LPCSTR
GetSpdTypeStr(UINT8 t)
{
	switch (t)
	{
	case MEM_TYPE_FPM_DRAM: return "FPM DRAM";
	case MEM_TYPE_EDO: return "EDO";
	case MEM_TYPE_PNEDO: return "PNEDO";
	case MEM_TYPE_SDRAM: return "SDR SDRAM";
	case MEM_TYPE_ROM: return "ROM";
	case MEM_TYPE_SGRAM: return "DDR SGRAM";
	case MEM_TYPE_DDR: return "DDR SDRAM";
	case MEM_TYPE_DDR2: return "DDR2 SDRAM";
	case MEM_TYPE_DDR2_FB: return "DDR2 SDRAM FB-DIMM";
	case MEM_TYPE_DDR2_FB_P: return "DDR2 SDRAM FB-DIMM PROBE";
	case MEM_TYPE_DDR3: return "DDR3 SDRAM";
	case MEM_TYPE_DDR4: return "DDR4 SDRAM";
	case MEM_TYPE_DDR4E: return "DDR4E SDRAM";
	case MEM_TYPE_LPDDR3: return "LPDDR3 SDRAM";
	case MEM_TYPE_LPDDR4: return "LPDDR4 SDRAM";
	case MEM_TYPE_LPDDR4X: return "LPDDR4X SDRAM";
	case MEM_TYPE_DDR5: return "DDR5 SDRAM";
	case MEM_TYPE_LPDDR5: return "LPDDR5 SDRAM";
	case MEM_TYPE_LPDDR5X: return "LPDDR5X SDRAM";
	}
	return "UNKNOWN";
}

PNODE NW_Spd(VOID)
{
	int i = 0;
	CHAR* ids = NULL;
	DWORD idsSize = 0;
	UINT8 rawSpd[SPD_MAX_SIZE];
	PNODE node = NWL_NodeAlloc("SPD", NFLG_TABLE);
	if (NWLC->SpdInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	smbus_t* ctx = SM_Init(NWLC->NwDrv);
	if (!ctx)
		return node;

	ids = NWL_LoadIdsToMemory(L"jep106.ids", &idsSize);
	UINT64 tStart = GetTickCount64();
	for (i = 0; i < SPD_MAX_SLOT; i++)
	{
		if (SM_GetSpd(ctx, i, rawSpd) != SM_OK)
			continue;

		PNODE nspd = NWL_NodeAppendNew(node, "Slot", NFLG_TABLE_ROW);
		NWL_NodeAttrSetf(nspd, "ID", NAFLG_FMT_NUMERIC, "%d", i);
		NWL_NodeAttrSet(nspd, "Memory Type", GetSpdTypeStr(rawSpd[SPD_MEMORY_TYPE_OFFSET]), 0);

		switch (rawSpd[SPD_MEMORY_TYPE_OFFSET])
		{
		case MEM_TYPE_SDRAM:
			PrintSDR(nspd, rawSpd, ids, idsSize);
			NWL_NodeAttrSetRaw(nspd, "Binary Data", rawSpd, 128);
			break;
		case MEM_TYPE_DDR:
			PrintDDR1(nspd, rawSpd, ids, idsSize);
			NWL_NodeAttrSetRaw(nspd, "Binary Data", rawSpd, 128);
			break;
		case MEM_TYPE_DDR2:
		case MEM_TYPE_DDR2_FB:
		case MEM_TYPE_DDR2_FB_P:
			PrintDDR2(nspd, rawSpd, ids, idsSize);
			NWL_NodeAttrSetRaw(nspd, "Binary Data", rawSpd, 256);
			break;
		case MEM_TYPE_DDR3:
		case MEM_TYPE_LPDDR3:
			PrintDDR3(nspd, rawSpd, ids, idsSize);
			NWL_NodeAttrSetRaw(nspd, "Binary Data", rawSpd, 256);
			break;
		case MEM_TYPE_DDR4:
		case MEM_TYPE_DDR4E:
		case MEM_TYPE_LPDDR4:
		case MEM_TYPE_LPDDR4X:
			PrintDDR4(nspd, rawSpd, ids, idsSize);
			NWL_NodeAttrSetRaw(nspd, "Binary Data", rawSpd, 512);
			break;
		case MEM_TYPE_DDR5:
		case MEM_TYPE_LPDDR5:
		case MEM_TYPE_LPDDR5X:
			PrintDDR5(nspd, rawSpd, ids, idsSize);
			NWL_NodeAttrSetRaw(nspd, "Binary Data", rawSpd, 1024);
			break;
		case MEM_TYPE_FPM_DRAM:
		case MEM_TYPE_EDO:
		case MEM_TYPE_PNEDO:
		case MEM_TYPE_ROM:
		case MEM_TYPE_SGRAM:
			// Unsupported
			NWL_NodeAttrSetRaw(nspd, "Binary Data", rawSpd, 128);
			break;
		default:
			// Unsupported
			NWL_NodeAttrSetRaw(nspd, "Binary Data", rawSpd, 256);
			break;
		}
	}
	SMBUS_DBG("Time: %llu", GetTickCount64() - tStart);
	SM_Free(ctx);
	free(ids);
	return node;
}

