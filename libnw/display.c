// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>
#include <math.h>

#include "libnw.h"
#include "utils.h"

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

union DisplayDescriptor
{
	UINT8 Raw[18];
	struct DetailedTimingDescriptor DT;
	struct DisplayDescriptorBlock
	{
		// 0x0000 = Display Descriptor
		UINT16 Reserved1;
		// 0x00
		UINT8 Reserved2;
		// 0x00 - 0x0F = OEM reserved
		// 0xFF ASCII Serial number
		// 0xFE ASCII Unspecified text
		// 0xFD Display Range Limits
		// 0xFC ASCII Display name
		UINT8 Type;
		// 0x00
		// If Type = 0xFD, Offsets for display range limits
		UINT8 Reserved3;
		UINT8 Data[13];
	} DD;
};

struct StandardTimingInformation
{
	// 00 = reserved
	// otherwise (value + 31) * 8 (256-2288px)
	UINT8 X;
	// Bits 7 - 6 : Aspect ratio, 00 = 16:10, 01 = 4:3, 10 = 5:4, 11 = 16:9
	// Bits 5 - 0 : Refresh rate, value + 60 (60-123Hz)
	UINT8 Y;
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
	struct StandardTimingInformation Std[8]; //38-53
	union DisplayDescriptor Desc[4];
	UCHAR NumOfExts;
	UCHAR Checksum;
};
#pragma pack()

struct MonitorInfo
{
	UINT64 XRes;
	UINT64 YRes;
	double Freq;
	UINT64 Width; //mm
	UINT64 Height; //mm
	CHAR Name[14];
};

static const char* hz_human_sizes[6] =
{ "Hz", "kHz", "MHz", "GHz", "THz", "PHz", };

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

static UINT64
GetYRes(PNODE node, UINT64 x, UINT64 AspectRatio, UINT16 ver)
{
	switch (AspectRatio)
	{
	case 0:
		if (ver < 0x0103)
		{
			NWL_NodeAttrSet(node, "Aspect Ratio", "1:1", 0);
			return x; // 1:1
		}
		NWL_NodeAttrSet(node, "Aspect Ratio", "16:10", 0);
		return (x * 10) / 16; // 16:10
	case 1:
		NWL_NodeAttrSet(node, "Aspect Ratio", "4:3", 0);
		return (x * 3) / 4; // 4:3
	case 2:
		NWL_NodeAttrSet(node, "Aspect Ratio", "5:4", 0);
		return (x * 4) / 5; // 5:4
	case 3:
		NWL_NodeAttrSet(node, "Aspect Ratio", "16:9", 0);
		return (x * 9) / 16; // 16:9
	}
	NWL_NodeAttrSet(node, "Aspect Ratio", "-", 0);
	return 0;
}

static void
DecodeStandardTiming(PNODE nm, struct EDID* pEDID, struct MonitorInfo* mi)
{
	int i;
	UINT16 ver = ((UINT16)pEDID->Version << 8) | pEDID->Revision;
	for (i = 0; i < 8; i++)
	{
		UINT64 x, y, a, f;
		CHAR name[32];
		PNODE nstd;

		snprintf(name, 32, "Standard Timing Block %d", i);
		nstd = NWL_NodeAppendNew(nm, name, NFLG_ATTGROUP);

		if (pEDID->Std[i].X == 0x01 && pEDID->Std[i].Y == 0x01)
		{
			NWL_NodeAttrSet(nstd, "Status", "Disabled", 0);
			continue;
		}
		NWL_NodeAttrSet(nstd, "Status", "Enabled", 0);
		x = (31ULL + pEDID->Std[i].X) * 8;
		a = (pEDID->Std[i].Y & 0xC0) >> 6;
		f = (pEDID->Std[i].Y & 0x3F) + 60;
		y = GetYRes(nstd, x, a, ver);
		NWL_NodeAttrSetf(nstd, "Vertical Frequency", NAFLG_FMT_NUMERIC, "%llu", f);
		NWL_NodeAttrSetf(nstd, "X Resolution", NAFLG_FMT_NUMERIC, "%llu", x);
		NWL_NodeAttrSetf(nstd, "Y Resolution", NAFLG_FMT_NUMERIC, "%llu", y);
		if (x * y > mi->XRes * mi->YRes)
		{
			mi->XRes = x;
			mi->YRes = y;
		}
		if (f > mi->Freq)
			mi->Freq = (double)f;
	}
}

static void
DecodeDetailedTiming(PNODE node, struct DetailedTimingDescriptor* desc, struct MonitorInfo* mi)
{
	UINT32 ha, va, hb, vb, w, h;
	UINT64 pc;
	double hz, inch;
	NWL_NodeAttrSet(node, "Type", "Detailed Timing Descriptor", 0);
	pc = ((UINT64)desc->PixelClock) * 10 * 1000;
	NWL_NodeAttrSet(node, "Pixel Clock", NWL_GetHumanSize(pc, hz_human_sizes, 1000), NAFLG_FMT_HUMAN_SIZE);
	ha = (UINT32)desc->HActiveLSB + (UINT32)((desc->HPixelsMSB & 0xf0) << 4);
	va = (UINT32)desc->VActiveLSB + (UINT32)((desc->VLinesMSB & 0xf0) << 4);
	hb = (UINT32)desc->HBlankingLSB + (UINT32)((desc->HPixelsMSB & 0x0f) << 8);
	vb = (UINT32)desc->VBlankingLSB + (UINT32)((desc->VLinesMSB & 0x0f) << 8);
	hz = ((double)pc) / (((UINT64)ha + hb) * ((UINT64)va + vb));
	NWL_NodeAttrSetf(node, "X Resolution", NAFLG_FMT_NUMERIC, "%u", ha);
	NWL_NodeAttrSetf(node, "Y Resolution", NAFLG_FMT_NUMERIC, "%u", va);
	NWL_NodeAttrSetf(node, "Refresh Rate (Hz)", NAFLG_FMT_NUMERIC, "%.2f", hz);

	if ((UINT64)ha * va > mi->XRes * mi->YRes)
	{
		mi->XRes = ha;
		mi->YRes = va;
	}
	if (hz > mi->Freq)
		mi->Freq = hz;

	w = (UINT32)desc->WidthLSB + (UINT32)((desc->WHMSB & 0xf0) << 4);
	h = (UINT32)desc->HeightLSB + (UINT32)((desc->WHMSB & 0x0f) << 8);
	inch = sqrt((double)((UINT64)w) * w + ((UINT64)h) * h) * 0.0393701;
	NWL_NodeAttrSetf(node, "Width (mm)", NAFLG_FMT_NUMERIC, "%u", w);
	NWL_NodeAttrSetf(node, "Height (mm)", NAFLG_FMT_NUMERIC, "%u", h);
	NWL_NodeAttrSetf(node, "Diagonal (in)", NAFLG_FMT_NUMERIC, "%.1f", inch);

	if ((UINT64)w * h > mi->Width * mi->Height)
	{
		mi->Width = w;
		mi->Height = h;
	}
}

static void
DecodeDisplayDescriptor(PNODE node, struct DisplayDescriptorBlock* desc, struct MonitorInfo* mi)
{
	int i;
	CHAR text[14] = { 0 };
	for (i = 0; i < 13; i++)
	{
		if (!isprint(desc->Data[i]))
			break;
		text[i] = desc->Data[i];
	}
	switch (desc->Type)
	{
	case 0xFF:
		NWL_NodeAttrSet(node, "Type", "Display Serial Number", 0);
		NWL_NodeAttrSet(node, "Text", text, 0);
		break;
	case 0xFE:
		NWL_NodeAttrSet(node, "Type", "Unspecified Text", 0);
		NWL_NodeAttrSet(node, "Text", text, 0);
		break;
	case 0xFD:
		NWL_NodeAttrSet(node, "Type", "Display Range Limits", 0);
		// TODO: decode display range limits
		break;
	case 0xFC:
		NWL_NodeAttrSet(node, "Type", "Display Name", 0);
		NWL_NodeAttrSet(node, "Text", text, 0);
		if (text[0])
			memcpy(mi->Name, text, sizeof (mi->Name));
		break;
	default:
		NWL_NodeAttrSet(node, "Type", "Display Descriptor", 0);
		break;
	}
}

static void
DecodeDescriptor(PNODE nm, struct EDID* pEDID, struct MonitorInfo* mi)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		CHAR name[32];
		PNODE nd;
		snprintf(name, 32, "Descriptor %d", i);
		nd = NWL_NodeAppendNew(nm, name, NFLG_ATTGROUP);
		if (pEDID->Desc[i].DD.Reserved1 == 0x0000 &&
			pEDID->Desc[i].DD.Reserved2 == 0x00)
			DecodeDisplayDescriptor(nd, &pEDID->Desc[i].DD, mi);
		else
			DecodeDetailedTiming(nd, &pEDID->Desc[i].DT, mi);
	}
}

static void
DecodeEDID(PNODE nm, void* pData, DWORD dwSize, CHAR* Ids, DWORD IdsSize)
{
	struct EDID* pEDID = pData;
	static UCHAR Magic[8] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
	char Manufacturer[4];
	PNODE nflags;
	struct MonitorInfo mi = { 0 };
	if (dwSize < sizeof(struct EDID))
		return;
	if (memcmp(pEDID->Magic, Magic, 8) != 0)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Bad EDID magic");
		return;
	}
	pEDID->Manufacturer = (pEDID->Manufacturer << 8U) | (pEDID->Manufacturer >> 8U); // BE
	snprintf(Manufacturer, sizeof (Manufacturer), "%c%c%c",
		(CHAR)(((pEDID->Manufacturer & 0x7c00U) >> 10U) + 'A' - 1),
		(CHAR)(((pEDID->Manufacturer & 0x3e0U) >> 5U) + 'A' - 1),
		(CHAR)((pEDID->Manufacturer & 0x1fU) + 'A' - 1));
	NWL_GetPnpManufacturer(nm, Ids, IdsSize, Manufacturer);
	NWL_NodeAttrSetf(nm, "ID", 0, "%s%04X", Manufacturer, pEDID->Product);
	NWL_NodeAttrSetf(nm, "Serial Number", NAFLG_FMT_SENSITIVE, "%08X", pEDID->Serial);
	NWL_NodeAttrSetf(nm, "Date", 0, "%u, Week %u", pEDID->Year + 1990, pEDID->Week & 0x7F);
	NWL_NodeAttrSetf(nm, "EDID Version", 0, "%u.%u", pEDID->Version, pEDID->Revision);
	nflags = NWL_NodeAppendNew(nm, "Video Input", NFLG_ATTGROUP);
	if (pEDID->Flags & 0x80)
	{
		UINT8 Depth = (pEDID->Flags & 0x70U) >> 4U;
		NWL_NodeAttrSet(nflags, "Type", "Digital", 0);
		if (Depth > 0 && Depth < 7)
			NWL_NodeAttrSetf(nflags, "Bits per color", NAFLG_FMT_NUMERIC, "%u", Depth * 2 + 4);
		NWL_NodeAttrSet(nflags, "Interface", InterfaceToStr(pEDID->Flags & 0x07U), 0);
	}
	else
	{
		NWL_NodeAttrSet(nflags, "Type", "Analog", 0);
	}

	mi.Width = 10ULL * pEDID->Width;
	mi.Height = 10ULL * pEDID->Height;

	DecodeStandardTiming(nm, pEDID, &mi);

	DecodeDescriptor(nm, pEDID, &mi);

	NWL_NodeAttrSet(nm, "Display Name", mi.Name, 0);
	NWL_NodeAttrSetf(nm, "Max Resolution", 0, "%llux%llu", mi.XRes, mi.YRes);
	NWL_NodeAttrSetf(nm, "Max Refresh Rate (Hz)", NAFLG_FMT_NUMERIC, "%.1f", mi.Freq);
	NWL_NodeAttrSetf(nm, "Width (cm)", NAFLG_FMT_NUMERIC, "%.1f", mi.Width / 10.0);
	NWL_NodeAttrSetf(nm, "Height (cm)", NAFLG_FMT_NUMERIC, "%.1f", mi.Height / 10.0);
	NWL_NodeAttrSetf(nm, "Diagonal (in)", NAFLG_FMT_NUMERIC, "%.1f",
		sqrt((double)(mi.Width * mi.Width + mi.Height * mi.Height)) * 0.0393701);
}

static void
GetEDID(PNODE node, HDEVINFO devInfo, PSP_DEVINFO_DATA devInfoData, CHAR* Ids, DWORD IdsSize, DWORD index)
{
	HKEY hDevRegKey;
	LSTATUS lRet;
	UCHAR* EDIDdata = NWLC->NwBuf;
	DWORD EDIDsize;

	if (!SetupDiGetDeviceRegistryPropertyW(devInfo, devInfoData,
		SPDRP_HARDWAREID, NULL, (PBYTE)NWLC->NwBufW, NWINFO_BUFSZB, NULL))
		return;

	if (NWLC->NwOsInfo.dwMajorVersion <= 5)
	{
		// Windows XP: prevent duplicate entries
		static WCHAR savedHwid[32] = { 0 };
		if (index && wcscmp(savedHwid, NWLC->NwBufW) == 0)
			return;
		wcscpy_s(savedHwid, 32, NWLC->NwBufW);
	}

	PNODE nm = NWL_NodeAppendNew(node, "Monitor", NFLG_TABLE_ROW);
	NWL_NodeAttrSet(nm, "HWID", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);

	hDevRegKey = SetupDiOpenDevRegKey(devInfo, devInfoData,
		DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_ALL_ACCESS);

	if (!hDevRegKey)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "SetupDiOpenDevRegKey failed");
		return;
	}
	EDIDsize = NWINFO_BUFSZ;
	ZeroMemory(EDIDdata, EDIDsize);
	lRet = RegGetValueW(hDevRegKey, NULL, L"EDID", RRF_RT_REG_BINARY, NULL, EDIDdata, &EDIDsize);
	if (lRet == ERROR_SUCCESS || lRet == ERROR_MORE_DATA)
	{
		DecodeEDID(nm, EDIDdata, EDIDsize, Ids, IdsSize);
	}
	RegCloseKey(hDevRegKey);
}

static VOID
EnumRes(PNODE node, LPCWSTR dev)
{
	DWORD i;
	DEVMODEW dm = { .dmSize = sizeof(DEVMODEW) };
	PNODE nm = NWL_NodeAppendNew(node, "Modes", NFLG_ATTGROUP);
	for (i = 0; EnumDisplaySettingsExW(dev, i, &dm, 0); i++)
	{
		DWORD freq = 0;
		if (!(dm.dmFields & DM_PELSWIDTH) || !(dm.dmFields & DM_PELSHEIGHT)
			|| !(dm.dmFields & DM_DISPLAYFREQUENCY) || !(dm.dmFields & DM_BITSPERPEL))
			continue;
		snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%lux%lu", dm.dmPelsWidth, dm.dmPelsHeight);
		LPCSTR p = NWL_NodeAttrGet(nm, NWLC->NwBuf);
		if (p[0] != '-')
			freq = strtoul(p, NULL, 10);
		if (freq < dm.dmDisplayFrequency)
			NWL_NodeAttrSetf(nm, NWLC->NwBuf, NAFLG_FMT_NUMERIC, "%u", dm.dmDisplayFrequency);
	}
	if (EnumDisplaySettingsExW(dev, ENUM_CURRENT_SETTINGS, &dm, 0))
		NWL_NodeAttrSetf(node, "Current Mode", 0, "%lux%lu@%uHz",
			dm.dmPelsWidth, dm.dmPelsHeight, dm.dmDisplayFrequency);
}

static VOID
EnumDisp(PNODE node)
{
	DWORD i;
	DISPLAY_DEVICEW dd = { .cb = sizeof(DISPLAY_DEVICEW) };
	for (i = 0; EnumDisplayDevicesW(NULL, i, &dd, EDD_GET_DEVICE_INTERFACE_NAME); i++)
	{
		if (!(dd.StateFlags & DISPLAY_DEVICE_ACTIVE))
			continue;
		PNODE nm = NWL_NodeAppendNew(node, "Display", NFLG_TABLE_ROW);
		NWL_NodeAttrSet(nm, "Device", NWL_Ucs2ToUtf8(dd.DeviceName), 0);
		NWL_NodeAttrSet(nm, "Name", NWL_Ucs2ToUtf8(dd.DeviceString), 0);
		NWL_NodeAttrSetBool(nm, "Primary", dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE, 0);
		NWL_NodeAttrSetBool(nm, "Removable", dd.StateFlags & DISPLAY_DEVICE_REMOVABLE, 0);
		NWL_NodeAttrSetBool(nm, "Remote", dd.StateFlags & DISPLAY_DEVICE_REMOTE, 0);
		EnumRes(nm, dd.DeviceName);
	}
}

VOID
NWL_GetCurDisplay(HWND wnd, NWLIB_CUR_DISPLAY* info)
{
	MONITORINFO mni = { .cbSize = sizeof(MONITORINFO) };
	GetMonitorInfoW(MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST), &mni);
	info->Width = mni.rcMonitor.right - mni.rcMonitor.left;
	info->Height = mni.rcMonitor.bottom - mni.rcMonitor.top;
	info->Dpi = GetDpiForWindow(wnd);
	info->Scale = 100 * info->Dpi / USER_DEFAULT_SCREEN_DPI;
}

PNODE NW_Edid(VOID)
{
	PNODE node = NWL_NodeAlloc("Display", NFLG_TABLE);
	HDEVINFO Info = NULL;
	DWORD i = 0;
	SP_DEVINFO_DATA DeviceInfoData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
	DWORD Flags = DIGCF_PRESENT | DIGCF_ALLCLASSES;
	CHAR* Ids = NULL;
	DWORD IdsSize = 0;
	if (NWLC->EdidInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	Info = SetupDiGetClassDevsW(NULL, L"DISPLAY", NULL, Flags);
	if (Info == INVALID_HANDLE_VALUE)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "SetupDiGetClassDevs failed");
		goto disp;
	}
	Ids = NWL_LoadIdsToMemory(L"pnp.ids", &IdsSize);
	for (i = 0; SetupDiEnumDeviceInfo(Info, i, &DeviceInfoData); i++)
	{
		GetEDID(node, Info, &DeviceInfoData, Ids, IdsSize, i);
	}
	SetupDiDestroyDeviceInfoList(Info);
	free(Ids);
disp:
	EnumDisp(node);
	return node;
}
