// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>
#include <math.h>
#include "nwinfo.h"

#include "pnp_id.h"

#pragma pack(1)
struct DetailedTimingDescriptor
{
	UINT16 PixelClock;
	UINT8 HActiveLSB;
	UINT8 HBlankingLSB;
	UINT8 HPixelsMSB;
	UINT8 VActiveLSB;
	UINT8 VBlankingLSB;
	UINT8 VLinesMSB;
	UINT8 Data1[4];
	UINT8 WidthLSB;
	UINT8 HeightLSB;
	UINT8 WHMSB;
	UINT8 Data2[3];
};

struct EDID
{
	UCHAR Magic[8]; //0-7, 00 FF FF FF FF FF FF 00
	UINT16 Manufacturer; //8
	UINT16 Product; //10
	UINT32 Serial; //12
	UCHAR Week; //16
	UCHAR Year; //17, +1990
	UCHAR Version; //18, 1
	UCHAR Revision; //19, 3 | 4
	UCHAR Flags; //20
	UCHAR Width; //21, cm
	UCHAR Height; //22, cm
	UCHAR Gamma; //23
	UCHAR Features; //24
	UCHAR RGLSB; //25
	UCHAR BWLSB; //26
	UCHAR RXMSB; //27
	UCHAR RYMSB; //28
	UCHAR GXMSB; //29
	UCHAR GYMSB; //30
	UCHAR BXMSB; //31
	UCHAR BYMSB; //32
	UCHAR WXMSB; //33
	UCHAR WYMSB; //34
	UCHAR Modes[3]; //35
	UCHAR StandardTiming[2 * 8]; //38-53
	struct DetailedTimingDescriptor Desc[4];
	UCHAR NumOfExts;
	UCHAR Checksum;
};
#pragma pack()

static const char* hz_human_sizes[6] =
{ "Hz", "kHz", "MHz", "GHz", "THz", "PHz", };

static const char*
GetPnpManufacturer(const char* Id)
{
	DWORD i;
	if (!Id)
		return "UNKNOWN";
	for (i = 0; i < PNP_ID_NUM; i++)
	{
		if (strcmp(Id, PNP_ID_LIST[i].id) == 0)
			return PNP_ID_LIST[i].vendor;
	}
	return Id;
}

static const CHAR*
InterfaceToStr(UINT8 Interface)
{
	switch (Interface)
	{
	case 2: return "HDMIa";
	case 3: return "HDMIb";
	case 4: return "MDDI";
	case 5: return "DisplayPort";
	}
	return "UNKNOWN";
}

static void
DecodeEDID(PNODE nm, void* pData, DWORD dwSize)
{
	DWORD i;
	struct EDID* pEDID = pData;
	static UCHAR Magic[8] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
	char Manufacturer[4];
	PNODE nflags;
	if (dwSize < sizeof(struct EDID))
		return;
	if (memcmp(pEDID->Magic, Magic, 8) != 0)
	{
		fprintf(stderr, "ERROR: bad edid magic\n");
		return;
	}
	pEDID->Manufacturer = (pEDID->Manufacturer << 8U) | (pEDID->Manufacturer >> 8U); // BE
	snprintf(Manufacturer, sizeof (Manufacturer), "%c%c%c",
		(CHAR)(((pEDID->Manufacturer & 0x7c00U) >> 10U) + 'A' - 1),
		(CHAR)(((pEDID->Manufacturer & 0x3e0U) >> 5U) + 'A' - 1),
		(CHAR)((pEDID->Manufacturer & 0x1fU) + 'A' - 1));
	node_att_set(nm, "Manufacturer", GetPnpManufacturer(Manufacturer), 0);
	node_att_setf(nm, "ID", 0, "%s%04X", Manufacturer, pEDID->Product);
	node_att_setf(nm, "Serial", 0, "%08X", pEDID->Serial);
	node_att_setf(nm, "Date", 0, "%u, Week %u", pEDID->Year + 1990, pEDID->Week & 0x7F);
	node_att_setf(nm, "EDID Version", 0, "%u.%u", pEDID->Version, pEDID->Revision);
	nflags = node_append_new(nm, "Video Input", NFLG_ATTGROUP);
	if (pEDID->Flags & 0x80)
	{
		UINT8 Depth = (pEDID->Flags & 0x70U) >> 4U;
		node_att_set(nflags, "Type", "Digital", 0);
		if (Depth > 0 && Depth < 7)
			node_att_setf(nflags, "Bits per color", NAFLG_FMT_NUMERIC, "%u", Depth * 2 + 4);
		node_att_set(nflags, "Interface", InterfaceToStr(pEDID->Flags & 0x07U), 0);
	}
	else
	{
		node_att_set(nflags, "Type", "Analog", 0);
	}
	for (i = 0; i < 4; i++)
	{
		UINT32 ha, va, hb, vb, w, h;
		UINT64 pc;
		double hz, inch;
		PNODE nres, nscr;
		if (pEDID->Desc[i].PixelClock == 0 && pEDID->Desc[i].HActiveLSB == 0)
			continue;
		pc = ((UINT64)pEDID->Desc[i].PixelClock) * 10 * 1000;
		node_att_set(nm, "Pixel Clock", GetHumanSize(pc, hz_human_sizes, 1000), 0);
		ha = (UINT32)pEDID->Desc[i].HActiveLSB + (UINT32)((pEDID->Desc[i].HPixelsMSB & 0xf0) << 4);
		va = (UINT32)pEDID->Desc[i].VActiveLSB + (UINT32)((pEDID->Desc[i].VLinesMSB & 0xf0) << 4);
		hb = (UINT32)pEDID->Desc[i].HBlankingLSB + (UINT32)((pEDID->Desc[i].HPixelsMSB & 0x0f) << 8);
		vb = (UINT32)pEDID->Desc[i].VBlankingLSB + (UINT32)((pEDID->Desc[i].VLinesMSB & 0x0f) << 8);
		hz = ((double)pc) / (((UINT64)ha + hb) * ((UINT64)va + vb));
		nres = node_append_new(nm, "Resolution", NFLG_ATTGROUP);
		node_att_setf(nres, "Width", NAFLG_FMT_NUMERIC, "%u", ha);
		node_att_setf(nres, "Height", NAFLG_FMT_NUMERIC, "%u", va);
		node_att_setf(nres, "Refresh Rate (Hz)", NAFLG_FMT_NUMERIC, "%.2f", hz);

		w = (UINT32)pEDID->Desc[i].WidthLSB + (UINT32)((pEDID->Desc[i].WHMSB & 0xf0) << 4);
		h = (UINT32)pEDID->Desc[i].HeightLSB + (UINT32)((pEDID->Desc[i].WHMSB & 0x0f) << 8);
		inch = sqrt((double)((UINT64)w) * w + ((UINT64)h) * h) * 0.0393701;
		nscr = node_append_new(nm, "Screen Size", NFLG_ATTGROUP);
		node_att_setf(nscr, "Width (mm)", NAFLG_FMT_NUMERIC, "%u", w);
		node_att_setf(nscr, "Height (mm)", NAFLG_FMT_NUMERIC, "%u", h);
		node_att_setf(nscr, "Diagonal (in)", NAFLG_FMT_NUMERIC, "%.1f", inch);
		break;
	}
}

static void
GetEDID(PNODE nm, HDEVINFO devInfo, PSP_DEVINFO_DATA devInfoData)
{
	HKEY hDevRegKey;
	LSTATUS lRet;
	BOOL bRet;
	UCHAR* EDIDdata = nwinfo_buffer;
	DWORD EDIDsize;

	bRet = SetupDiGetDeviceRegistryPropertyA(devInfo, devInfoData,
		SPDRP_HARDWAREID, NULL, nwinfo_buffer, NWINFO_BUFSZ, NULL);
	node_att_set (nm, "HWID", bRet ? nwinfo_buffer : "UNKNOWN", 0);

	hDevRegKey = SetupDiOpenDevRegKey(devInfo, devInfoData,
		DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_ALL_ACCESS);

	if (!hDevRegKey)
	{
		fprintf(stderr, "SetupDiOpenDevRegKey failed\n");
		return;
	}
	EDIDsize = NWINFO_BUFSZ;
	ZeroMemory(EDIDdata, EDIDsize);
	lRet = RegGetValueA(hDevRegKey, NULL, "EDID", RRF_RT_REG_BINARY, NULL, EDIDdata, &EDIDsize);
	if (lRet == ERROR_SUCCESS || lRet == ERROR_MORE_DATA)
	{
		DecodeEDID(nm, EDIDdata, EDIDsize);
	}
	RegCloseKey(hDevRegKey);
}

PNODE nwinfo_display(void)
{
	PNODE node = node_alloc("Display", NFLG_TABLE);
	HDEVINFO Info = NULL;
	DWORD i = 0;
	SP_DEVINFO_DATA DeviceInfoData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
	DWORD Flags = DIGCF_PRESENT | DIGCF_ALLCLASSES;
	Info = SetupDiGetClassDevsExA(NULL, "DISPLAY", NULL, Flags, NULL, NULL, NULL);
	if (Info == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "SetupDiGetClassDevs failed.\n");
		return node;
	}
	for (i = 0; SetupDiEnumDeviceInfo(Info, i, &DeviceInfoData); i++)
	{
		PNODE nm = node_append_new(node, "Monitor", NFLG_TABLE_ROW);
		GetEDID(nm, Info, &DeviceInfoData);
	}
	SetupDiDestroyDeviceInfoList(Info);
	return node;
}
