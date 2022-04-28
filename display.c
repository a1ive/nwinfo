// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>
#include <math.h>
#include "nwinfo.h"

#include "pnp_id.h"

#define NAME_SIZE 128

static const GUID GUID_CLASS_MONITOR =
	{ 0x4d36e96e, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 };

static UCHAR EDIDdata[1024];
static DWORD EDIDsize = sizeof(EDIDdata);

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
	return "UNKNOWN";
}

static void
DecodeEDID(void* pData, DWORD dwSize)
{
	DWORD i;
	struct EDID* pEDID = pData;
	static UCHAR Magic[8] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
	char Manufacturer[4];
	if (dwSize < sizeof(struct EDID))
		return;
	if (memcmp(pEDID->Magic, Magic, 8) != 0)
	{
		printf("ERROR: bad edid magic\n");
		return;
	}
	pEDID->Manufacturer = (pEDID->Manufacturer << 8U) | (pEDID->Manufacturer >> 8U); // BE
	snprintf(Manufacturer, sizeof (Manufacturer), "%c%c%c",
		(CHAR)(((pEDID->Manufacturer & 0x7c00U) >> 10U) + 'A' - 1),
		(CHAR)(((pEDID->Manufacturer & 0x3e0U) >> 5U) + 'A' - 1),
		(CHAR)((pEDID->Manufacturer & 0x1fU) + 'A' - 1));
	printf("Manufacturer: %s (%s)\n", GetPnpManufacturer(Manufacturer), Manufacturer);
	printf("Product: %04X\n", pEDID->Product);
	printf("Serial: %08X\n", pEDID->Serial);
	printf("Date: %u, Week %u\n", pEDID->Year + 1990, pEDID->Week & 0x7F);
	printf("EDID Version: %u.%u\n", pEDID->Version, pEDID->Revision);
	printf("Video Input: ");
	if (pEDID->Flags & 0x80)
	{
		printf("Digital");
		UINT8 Depth = (pEDID->Flags & 0x70U) >> 4U;
		if (Depth > 0 && Depth < 7)
			printf(", %u bits per color", Depth * 2 + 4);
		UINT8 Interface = pEDID->Flags & 0x07U;
		switch (Interface)
		{
		case 2:
			printf(", HDMIa");
			break;
		case 3:
			printf(", HDMIb");
			break;
		case 4:
			printf(", MDDI");
			break;
		case 5:
			printf(", DisplayPort");
			break;
		}
	}
	else
	{
		printf("Analog");
	}
	printf("\n");
	for (i = 0; i < 4; i++)
	{
		UINT32 ha, va, hb, vb, w, h;
		UINT64 pc;
		double hz, inch;
		if (pEDID->Desc[i].PixelClock == 0 && pEDID->Desc[i].HActiveLSB == 0)
			continue;
		pc = ((UINT64)pEDID->Desc[i].PixelClock) * 10 * 1000;
		printf("Pixel Clock: %s\n", GetHumanSize(pc, hz_human_sizes, 1000));
		ha = (UINT32)pEDID->Desc[i].HActiveLSB + (UINT32)((pEDID->Desc[i].HPixelsMSB & 0xf0) << 4);
		va = (UINT32)pEDID->Desc[i].VActiveLSB + (UINT32)((pEDID->Desc[i].VLinesMSB & 0xf0) << 4);
		hb = (UINT32)pEDID->Desc[i].HBlankingLSB + (UINT32)((pEDID->Desc[i].HPixelsMSB & 0x0f) << 8);
		vb = (UINT32)pEDID->Desc[i].VBlankingLSB + (UINT32)((pEDID->Desc[i].VLinesMSB & 0x0f) << 8);
		hz = ((double)pc) / (((UINT64)ha + hb) * ((UINT64)va + vb));
		printf("Resolution: %u x %u @%.2fHz\n", ha, va, hz);
		w = (UINT32)pEDID->Desc[i].WidthLSB + (UINT32)((pEDID->Desc[i].WHMSB & 0xf0) << 4);
		h = (UINT32)pEDID->Desc[i].HeightLSB + (UINT32)((pEDID->Desc[i].WHMSB & 0x0f) << 8);
		inch = sqrt((double)((UINT64)w) * w + ((UINT64)h) * h) * 0.0393701;
		printf("Screen Size: %u mm x %u mm (%.1f\")\n", w, h, inch);
		break;
	}
	printf("\n");
}

static void
PrintEDID(UCHAR* pData, DWORD dwSize)
{
	DWORD i;
	for (i = 0; i < dwSize; i++)
	{
		if (i && i % 16 == 0)
			printf("\n");
		printf("%02x ", pData[i]);
	}
	printf("\n");
}

static void
GetEDID(int raw, HDEVINFO devInfo, PSP_DEVINFO_DATA devInfoData)
{
	HKEY hDevRegKey;
#if 1
	CHAR hwID[256];
	if (SetupDiGetDeviceRegistryPropertyA(devInfo, devInfoData,
		SPDRP_HARDWAREID, NULL, hwID, sizeof(hwID), NULL))
	{
		printf ("HWID: %s\n", hwID);
	}
	else
	{
		printf("HWID: UNKNOWN\n");
	}
#endif
	hDevRegKey = SetupDiOpenDevRegKey(devInfo, devInfoData,
		DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_ALL_ACCESS);

	if (hDevRegKey)
	{
		LONG retValue, i;
		DWORD dwType, AcutalValueNameLength = NAME_SIZE;
		CHAR valueName[NAME_SIZE] = { 0 };

		for (i = 0, retValue = ERROR_SUCCESS; retValue != ERROR_NO_MORE_ITEMS; i++)
		{
			EDIDsize = sizeof(EDIDdata);
			ZeroMemory(EDIDdata, EDIDsize);
			retValue = RegEnumValueA(hDevRegKey, i,
				valueName, &AcutalValueNameLength, NULL, &dwType, EDIDdata, &EDIDsize);

			if (retValue == ERROR_SUCCESS)
			{
				if (!strcmp(valueName, "EDID"))
				{
					DecodeEDID(EDIDdata, EDIDsize);
					if (raw)
						PrintEDID(EDIDdata, EDIDsize);
					break;
				}
			}
		}
		RegCloseKey(hDevRegKey);
	}
}

void nwinfo_display(void)
{
	HDEVINFO devInfo = NULL;
	SP_DEVINFO_DATA devInfoData;
	ULONG i;
	do
	{
		devInfo = SetupDiGetClassDevsExA(&GUID_CLASS_MONITOR,
			NULL, NULL, DIGCF_PRESENT, NULL, NULL, NULL);
		if (!devInfo)
			break;

		for (i = 0; ERROR_NO_MORE_ITEMS != GetLastError(); i++)
		{
			memset(&devInfoData, 0, sizeof(devInfoData));
			devInfoData.cbSize = sizeof(devInfoData);
			if (SetupDiEnumDeviceInfo(devInfo, i, &devInfoData))
			{
				GetEDID(0, devInfo, &devInfoData);
			}
		}

	} while (FALSE);
}
