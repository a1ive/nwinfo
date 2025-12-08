// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>

#ifdef USE_MATH_SQRT
#include <math.h>
#endif

#include "libnw.h"
#include "utils.h"

// We don't include ntddvdeo.h directly to avoid potential conflicts or dependencies.
// This GUID is for the device interface class for monitors.
DEFINE_GUID(GUID_DEVINTERFACE_MONITOR,
	0xe6f07b5f, 0xee97, 0x4a90,
	0xb0, 0x76, 0x33, 0xf5, 0x7b, 0xf4, 0xea, 0xa7);

struct MONITOR_INFO
{
	UINT64 XRes;
	UINT64 YRes;
	double Freq;
	UINT64 Width; //mm
	UINT64 Height; //mm
	CHAR Name[14];
	CHAR Serial[14];
	UINT16 Ver;
};

/**
 * @brief Cleans a string read from EDID by removing trailing newlines and spaces.
 *
 * @param str The string to clean. The string is modified in place.
 */
static void CleanupEdidString(char* str)
{
	if (!str)
		return;

	size_t len = strlen(str);
	while (len > 0)
	{
		if (isspace(str[len - 1]))
			len--;
		else
			break;
	}
	str[len] = '\0';
}

/**
 * @brief Calculates the greatest common divisor (GCD) of two integers.
 *
 * @param a The first integer.
 * @param b The second integer.
 * @return The GCD of a and b.
 */
static UINT64 GetGcd(UINT64 a, UINT64 b)
{
	while (b)
	{
		UINT64 temp = b;
		b = a % b;
		a = temp;
	}
	return a;
}

/**
 * @brief Calculates the integer square root of a number using a fast bitwise algorithm.
 *
 * @details This method is significantly faster than division-based approaches
 *          because it avoids slow integer division operations. It works by
 *          determining one bit of the result at a time, from the most significant
 *          to the least significant. The number of iterations is fixed based on the
 *          bit-width of the input, making its performance highly predictable.
 *          Internal calculations use 64-bit integers to handle the scaled input without overflow.
 *
 * @param n The number to find the square root of.
 * @return The integer square root of n, scaled by 10 and correctly rounded.
 */
#ifndef USE_MATH_SQRT
static UINT64 IntegerSqrt(UINT64 n)
{
	if (n == 0)
		return 0;

	// Scale up by 10000 for two decimal places of precision, enabling rounding.
	UINT64 nScaled = n * 10000;

	UINT64 result = 0;
	UINT64 remainder = nScaled;
	// Start with the highest possible bit in the result.
	// For a 64-bit n, the root is at most 32 bits, so the highest bit
	// is at position 31. The bit value is 1LL << 62 for the number itself.
	// We can start from a safe high bit. 1LL << 62 is a good start for 64-bit numbers.
	UINT64 bit = 1ULL << 62;

	while (bit > 0)
	{
		// Calculate the trial value by adding the current bit to the result.
		UINT64 trial = result + bit;

		if (remainder >= trial)
		{
			remainder -= trial;
			// The bit is set. Update the result by adding the bit.
			// The `result >> 1` shifts the existing result to make space.
			result = (result >> 1) + bit;
		}
		else
		{
			// The bit is not set. Just shift the result.
			result = result >> 1;
		}
		// Move to the next bit pair (equivalent to two bit shifts right).
		bit >>= 2;
	}

	// Now `result` holds sqrt(n) * 100.
	// To round to one decimal place, add 5 and then divide by 10.
	return ((result + 5) / 10);
}
#endif

static const char*
GetVideoInterface(BYTE videoIf)
{
	switch (videoIf)
	{
	case 1: return "DVI";
	case 2: return "HDMIa";
	case 3: return "HDMIb";
	case 4: return "MDDI";
	case 5: return "DisplayPort";
	}
	return "UNKNOWN";
}

static const char*
GetSignalLevel(BYTE signalLevel)
{
	switch (signalLevel)
	{
	case 0: return "+0.7/-0.3 V";
	case 1: return "+0.714/-0.286 V";
	case 2: return "+1.0/-0.4 V";
	case 3: return "+0.7/0 V (EVC)";
	}
	return "UNKNOWN";
}

static const char*
GetAspectRatio(BYTE aspectRatio, UINT16 edidVer, UINT64 hRes, UINT64* vRes)
{
	switch (aspectRatio)
	{
	case 0:
		if (edidVer < 0x0103)
		{
			*vRes = hRes;
			return "1:1";
		}
		*vRes = (hRes * 10) / 16;
		return "16:10";
	case 1:
		*vRes = (hRes * 3) / 4;
		return "4:3";
	case 2:
		*vRes = (hRes * 4) / 5;
		return "5:4";
	case 3:
		*vRes = (hRes * 9) / 16;
		return "16:9";
	}
	// Should never happen
	*vRes = 0;
	return "UNKNOWN";
}

static char* DecodeAsciiDescriptor(const BYTE* block, PNODE node, const char* name)
{
	PNODE ascii = NWL_NodeAppendNew(node, "ASCII Descriptor", NFLG_TABLE_ROW);
	NWL_NodeAttrSet(ascii, "Type", name, 0);
	// A string descriptor has a 5-byte header, followed by up to 13 bytes of text.
	// We allocate 14 bytes to ensure null termination.
	char* text = (char*)malloc(14);
	if (!text)
		return NULL;

	// Copy the string data from the block.
	memcpy(text, block + 5, 13);
	text[13] = '\0'; // Ensure null termination.
	CleanupEdidString(text);

	NWL_NodeAttrSet(ascii, "Text", text, 0);

	// The caller will use this string for the summary, so we return a copy.
	// Or rather, we return the original allocation and the caller must free it.
	return text;
}

static void DecodeDetailedTimingDescriptor(const BYTE* block, PNODE node, struct MONITOR_INFO* mi)
{
	PNODE dtd = NWL_NodeAppendNew(node, "Detailed Timing Descriptor", NFLG_TABLE_ROW);
	NWL_NodeAttrSet(dtd, "Type", "Detailed Timing Descriptor", 0);
	// All values are calculated from the 18-byte block.
	// Pixel clock is in units of 10 kHz.
	int pixelClock = ((block[1] << 8) | block[0]) * 10; // in kHz
	int hActive = block[2] + ((block[4] & 0xF0) << 4);
	int hBlank = block[3] + ((block[4] & 0x0F) << 8);
	int vActive = block[5] + ((block[7] & 0xF0) << 4);
	int vBlank = block[6] + ((block[7] & 0x0F) << 8);

	int hTotal = hActive + hBlank;
	int vTotal = vActive + vBlank;

	// Avoid division by zero
	if (hTotal == 0 || vTotal == 0)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Invalid timing data");
		return;
	}

	// Refresh rate calculation using 64-bit integers to prevent overflow
	// Refresh Rate = PixelClock / (HTotal * VTotal)
	double refreshRate = (pixelClock * 1000.0) / ((ULONGLONG)hTotal * vTotal);
	if (hActive * vActive > mi->XRes * mi->YRes)
	{
		mi->XRes = hActive;
		mi->YRes = vActive;
	}
	if (mi->Freq < refreshRate)
		mi->Freq = refreshRate;
	NWL_NodeAttrSetf(dtd, "Pixel Clock (kHz)", NAFLG_FMT_NUMERIC, "%d", pixelClock);
	NWL_NodeAttrSetf(dtd, "Resolution", 0, "%dx%d", hActive, vActive);
	NWL_NodeAttrSetf(dtd, "Refresh Rate (Hz)", NAFLG_FMT_NUMERIC, "%.2f", refreshRate);
	NWL_NodeAttrSetf(dtd, "H Active", NAFLG_FMT_NUMERIC, "%d", hActive);
	NWL_NodeAttrSetf(dtd, "H Blank", NAFLG_FMT_NUMERIC, "%d", hBlank);
	NWL_NodeAttrSetf(dtd, "H Total", NAFLG_FMT_NUMERIC, "%d", hTotal);
	NWL_NodeAttrSetf(dtd, "V Active", NAFLG_FMT_NUMERIC, "%d", vActive);
	NWL_NodeAttrSetf(dtd, "V Blank", NAFLG_FMT_NUMERIC, "%d", vBlank);
	NWL_NodeAttrSetf(dtd, "V Total", NAFLG_FMT_NUMERIC, "%d", vTotal);

	// Extract physical dimensions from DTD (bytes 12-14)
	int hSizeMm = block[12] | ((block[14] & 0xF0) << 4);
	int vSizeMm = block[13] | ((block[14] & 0x0F) << 8);

	if (hSizeMm > 0 && vSizeMm > 0)
	{
		NWL_NodeAttrSetf(dtd, "Image Size (mm)", 0, "%dx%d", hSizeMm, vSizeMm);

		// If the main size variables are not yet set, update them from this DTD.
		// The first DTD is considered the primary one for this information.
		if (mi->Width == 0 && mi->Height == 0)
		{
			mi->Width = (UINT64)hSizeMm;
			mi->Height = (UINT64)vSizeMm;
		}
	}

	// Parse features bitmap in byte 17
	BYTE features = block[17];

	// Bit 7: Interlaced or Non-interlaced
	NWL_NodeAttrSet(dtd, "Signal", (features & 0x80) ? "Interlaced" : "Non-interlaced", 0);

	// Bits 6, 5, 0: Stereo mode
	BYTE stereoCode = (features >> 5) & 0x03;
	BYTE stereoLsb = features & 0x01;
	const char* stereoMode;
	switch (stereoCode)
	{
	case 0x00:
		stereoMode = "Normal display (No stereo)";
		break;
	case 0x01:
		stereoMode = (stereoLsb) ? "2-way interleaved, right image on even lines" : "Field sequential, right during stereo sync";
		break;
	case 0x02:
		stereoMode = (stereoLsb) ? "2-way interleaved, left image on even lines" : "Field sequential, left during stereo sync";
		break;
	case 0x03:
		stereoMode = (stereoLsb) ? "Side-by-side interleaved" : "4-way interleaved";
		break;
	default:
		stereoMode = "UNKNOWN"; // Should not happen
		break;
	}
	NWL_NodeAttrSet(dtd, "Stereo Mode", stereoMode, 0);


	// Bits 4, 3, 2, 1: Sync configuration
	BYTE syncCode = (features >> 3) & 0x03;
	switch (syncCode)
	{
	case 0: // Analog Composite
	case 1: // Bipolar Analog Composite
		NWL_NodeAttrSet(dtd, "Sync Type", (syncCode == 0) ? "Analog Composite" : "Bipolar Analog Composite", 0);
		NWL_NodeAttrSetBool(dtd, "Serrations", (features & 0x04), 0);
		NWL_NodeAttrSetBool(dtd, "Sync on RGB", (features & 0x02), 0);
		break;
	case 2: // Digital Composite
		NWL_NodeAttrSet(dtd, "Sync Type", "Digital Composite (on HSync)", 0);
		NWL_NodeAttrSetBool(dtd, "Serration", (features & 0x04), 0);
		NWL_NodeAttrSet(dtd, "H-Sync Polarity", (features & 0x02) ? "Positive" : "Negative", 0);
		break;
	case 3: // Digital Separate
		NWL_NodeAttrSet(dtd, "Sync Type", "Digital Separate", 0);
		NWL_NodeAttrSet(dtd, "V-Sync Polarity", (features & 0x04) ? "Positive" : "Negative", 0);
		NWL_NodeAttrSet(dtd, "H-Sync Polarity", (features & 0x02) ? "Positive" : "Negative", 0);
		break;
	}
}

static void DecodeDisplayRangeLimits(const BYTE* block, PNODE node)
{
	PNODE drl = NWL_NodeAppendNew(node, "Display Range Limits", NFLG_TABLE_ROW);
	NWL_NodeAttrSet(drl, "Type", "Display Range Limits", 0);
	// Offsets within the 18-byte block.
	int minVRate = block[5];
	int maxVRate = block[6];
	int minHRate = block[7];
	int maxHRate = block[8];
	int maxPixelClock = block[9] * 10; // In MHz

	NWL_NodeAttrSetf(drl, "Vertical Rate (Hz)", 0, "%d-%d", minVRate, maxVRate);
	NWL_NodeAttrSetf(drl, "Horizontal Rate (kHz)", 0, "%d-%d", minHRate, maxHRate);
	NWL_NodeAttrSetf(drl, "Max Pixel Clock (MHz)", NAFLG_FMT_NUMERIC, "%d", maxPixelClock);
}

static void DecodeUnsuopportedDescriptor(const BYTE* block, PNODE node, const char* name)
{
	PNODE unsup = NWL_NodeAppendNew(node, name, NFLG_TABLE_ROW);
	NWL_NodeAttrSet(unsup, "Type", name, 0);
}

static void DecodeDescriptorBlocks(const BYTE* edid, PNODE node, struct MONITOR_INFO* mi)
{
	PNODE desc = NWL_NodeAppendNew(node, "Descriptor Blocks", NFLG_TABLE);
	// There are four 18-byte descriptor blocks, starting at offset 54.
	for (int i = 0; i < 4; ++i)
	{
		int offset = 54 + (i * 18);
		const BYTE* block = edid + offset;

		// Check if it's a Detailed Timing Descriptor (DTD).
		// A DTD does not start with 0x00 0x00.
		if (block[0] != 0x00 || block[1] != 0x00)
		{
			DecodeDetailedTimingDescriptor(block, desc, mi);
		}
		else
		{
			// It's a different type of descriptor, identified by block[3].
			switch (block[3])
			{
			case 0xFF: // Serial Number
			{
				char* serial = DecodeAsciiDescriptor(block, desc, "Serial Number");
				if (serial)
				{
					strcpy_s(mi->Serial, sizeof(mi->Serial), serial);
					free(serial);
				}
				break;
			}
			case 0xFE: // Unspecified Text / Dummy
			{
				char* text = DecodeAsciiDescriptor(block, desc, "Unspecified Text");
				free(text);
				break;
			}
			case 0xFD: // Display Range Limits
				DecodeDisplayRangeLimits(block, desc);
				break;
			case 0xFC: // Display Name
			{
				char* name = DecodeAsciiDescriptor(block, desc, "Display Name");
				if (name)
				{
					strcpy_s(mi->Name, sizeof(mi->Name), name);
					free(name);
				}
				break;
			}
			case 0xFB: // Additional white point data
				DecodeUnsuopportedDescriptor(block, desc, "White Point Data");
				break;
			case 0xFA: // Additional standard timing identifier
				DecodeUnsuopportedDescriptor(block, desc, "Standard Timing Identifier");
				break;
			case 0xF9: // Display Color Management
				DecodeUnsuopportedDescriptor(block, desc, "Display Color Management");
				break;
			case 0xF8: // CVT 3_byte Timing Code
				DecodeUnsuopportedDescriptor(block, desc, "CVT 3-byte Timing Code");
				break;
			case 0xF7: // Additional standard timing 3
				DecodeUnsuopportedDescriptor(block, desc, "Additional Standard Timing 3");
				break;
			default:
				DecodeUnsuopportedDescriptor(block, desc, "Vendor-defined");
				break;
			}
		}
	}
}

static void
DecodeStandardTimings(const BYTE* edid, PNODE node, struct MONITOR_INFO* mi)
{
	PNODE blks = NWL_NodeAppendNew(node, "Standard Timing Blocks", NFLG_TABLE);
	// 16 bytes for 8 standard timing blocks, starting at offset 38.
	for (int i = 0; i < 8; ++i)
	{
		int offset = 38 + (i * 2);
		BYTE b1 = edid[offset];
		BYTE b2 = edid[offset + 1];

		PNODE std = NWL_NodeAppendNew(blks, "Standard Timing Block", NFLG_TABLE_ROW);

		// If the first byte is 0x01, the timing is not used.
		if (b1 == 0x01 && b2 == 0x01)
		{
			NWL_NodeAttrSet(std, "Status", "Disabled", 0);
			continue;
		}
		NWL_NodeAttrSet(std, "Status", "Enabled", 0);

		UINT64 hRes = (31ULL + b1) * 8;
		UINT64 vRes;
		const char* aspectRatio = GetAspectRatio((b2 >> 6) & 0x03, mi->Ver, hRes, &vRes);
		int refresh = (b2 & 0x3F) + 60;

		NWL_NodeAttrSet(std, "Aspect Ratio", aspectRatio, 0);
		NWL_NodeAttrSetf(std, "Resolution", 0, "%llux%llu",
			(unsigned long long)hRes, (unsigned long long)vRes);
		NWL_NodeAttrSetf(std, "Refresh Rate (Hz)", NAFLG_FMT_NUMERIC, "%d", refresh);

		// Update max resolution if this one is bigger
		if (hRes * vRes > mi->XRes * mi->YRes)
		{
			mi->XRes = hRes;
			mi->YRes = vRes;
			// Note: We don't update max refresh rate from here as DTD is more precise.
		}
	}
}

static void DecodeVideoInput(const BYTE* edid, PNODE node)
{
	PNODE vi = NWL_NodeAppendNew(node, "Video Input", NFLG_ATTGROUP);

	BYTE videoInput = edid[20];
	if (videoInput & 0x80)
	{
		// Digital Input
		BYTE colorDepth = (videoInput & 0x70) >> 4U;
		NWL_NodeAttrSet(vi, "Type", "Digital", 0);
		NWL_NodeAttrSet(vi, "Interface", GetVideoInterface(videoInput & 0x07), 0);
		if (colorDepth > 0 && colorDepth < 7)
			NWL_NodeAttrSetf(vi, "Bits per Color", NAFLG_FMT_NUMERIC, "%u", colorDepth * 2 + 4);
	}
	else
	{
		// Analog Input
		NWL_NodeAttrSet(vi, "Type", "Analog", 0);
		// Video white and sync levels (bits 6-5)
		NWL_NodeAttrSet(vi, "Signal Level", GetSignalLevel((videoInput & 0x60) >> 5U), 0);
		// Blank-to-black setup (bit 4)
		NWL_NodeAttrSet(vi, "Blank-to-Black Setup (Pedestal)", (videoInput & 0x10) ? "Expected" : "Not Expected", 0);

		// Sync support
		NWL_NodeAttrSet(vi, "Separate Sync", (videoInput & 0x08) ? "Supported" : "Not Supported", 0);
		NWL_NodeAttrSet(vi, "Composite Sync (on HSync)", (videoInput & 0x04) ? "Supported" : "Not Supported", 0);
		NWL_NodeAttrSet(vi, "Sync on Green", (videoInput & 0x02) ? "Supported" : "Not Supported", 0);

		// Serrated VSync (bit 0)
		NWL_NodeAttrSet(vi, "Serrated VSync Pulse", (videoInput & 0x01) ? "Required" : "Not Required", 0);
	}
}

static void DecodeSupportedFeatures(const BYTE* edid, PNODE node, struct MONITOR_INFO* mi)
{
	PNODE feat = NWL_NodeAppendNew(node, "Supported Features", NFLG_ATTGROUP);

	BYTE features = edid[24];
	BYTE videoInput = edid[20];
	BOOL isDigital = (videoInput & 0x80);

	// DPMS Support (Bits 7, 6, 5)
	NWL_NodeAttrSetBool(feat, "DPMS Standby", (features & 0x80), 0);
	NWL_NodeAttrSetBool(feat, "DPMS Suspend", (features & 0x40), 0);
	NWL_NodeAttrSetBool(feat, "DPMS Active-Off", (features & 0x20), 0);

	// Display Type (Bits 4-3), meaning depends on whether the input is digital or analog
	BYTE displayTypeCode = (features >> 3) & 0x03;
	if (isDigital)
	{
		const char* digitalType;
		switch (displayTypeCode)
		{
		case 0: digitalType = "RGB 4:4:4"; break;
		case 1: digitalType = "RGB 4:4:4 + YCrCb 4:4:4"; break;
		case 2: digitalType = "RGB 4:4:4 + YCrCb 4:2:2"; break;
		case 3: digitalType = "RGB 4:4:4 + YCrCb 4:4:4 + YCrCb 4:2:2"; break;
		default: digitalType = "UNKNOWN"; break; // Should not happen
		}
		NWL_NodeAttrSet(feat, "Digital Color Formats", digitalType, 0);
	}
	else
	{
		// Analog
		const char* analogType;
		switch (displayTypeCode)
		{
		case 0: analogType = "Monochrome or Grayscale"; break;
		case 1: analogType = "RGB Color"; break;
		case 2: analogType = "Non-RGB Color"; break;
		case 3: analogType = "Undefined"; break;
		default: analogType = "UNKNOWN"; break; // Should not happen
		}
		NWL_NodeAttrSet(feat, "Analog Display Type", analogType, 0);
	}

	// sRGB Color Space (Bit 2)
	NWL_NodeAttrSetBool(feat, "sRGB Standard", (features & 0x04), 0);

	// Preferred Timing Mode (Bit 1)
	if (features & 0x02)
	{
		// For EDID 1.3+, this bit has a more specific meaning.
		NWL_NodeAttrSet(feat, "Preferred Timing Mode", (mi->Ver >= 0x0103) ?
			"Includes native pixel format and refresh rate (DTD 1)" :
			"Specified in Descriptor Block 1", 0);
	}
	else
		NWL_NodeAttrSet(feat, "Preferred Timing Mode", "Not specified (DTD 1)", 0);

	// Continuous Timings (Bit 0)
	NWL_NodeAttrSetBool(feat, "Continuous Timings (GTF/CVT)", (features & 0x01), 0);
}

static void PrintChromaticityForColor(const char* colorName, int x_msb, int x_lsb, int y_msb, int y_lsb, PNODE node)
{
	// Combine the 8-bit MSB and 2-bit LSB to form the full 10-bit value for each coordinate.
	int x_full = (x_msb << 2) | x_lsb;
	int y_full = (y_msb << 2) | y_lsb;

	// The 10-bit value represents a fraction: value / 1024.
	// To get four decimal places using integer math, we calculate (value * 10000) / 1024.
	// We use 64-bit integers for the intermediate multiplication to prevent any possibility of overflow.
	int x_scaled = (int)(((long long)x_full * 10000) / 1024);
	int y_scaled = (int)(((long long)y_full * 10000) / 1024);

	// Print the formatted coordinates.
	NWL_NodeAttrSetf(node, colorName, 0, "(x=0.%04d, y=0.%04d)", x_scaled, y_scaled);
}

static void DecodeChromaticity(const BYTE* edid, PNODE node)
{
	PNODE cc = NWL_NodeAppendNew(node, "Chromaticity Coordinates", NFLG_ATTGROUP);

	// Byte 25 contains the LSBs for Red and Green.
	BYTE lsb_rg = edid[25];
	int red_x_lsb = (lsb_rg >> 6) & 0x03;
	int red_y_lsb = (lsb_rg >> 4) & 0x03;
	int green_x_lsb = (lsb_rg >> 2) & 0x03;
	int green_y_lsb = (lsb_rg >> 0) & 0x03;

	// Byte 26 contains the LSBs for Blue and White.
	BYTE lsb_bw = edid[26];
	int blue_x_lsb = (lsb_bw >> 6) & 0x03;
	int blue_y_lsb = (lsb_bw >> 4) & 0x03;
	int white_x_lsb = (lsb_bw >> 2) & 0x03;
	int white_y_lsb = (lsb_bw >> 0) & 0x03;

	// Bytes 27-34 contain the MSBs for all colors.
	int red_x_msb = edid[27];
	int red_y_msb = edid[28];
	int green_x_msb = edid[29];
	int green_y_msb = edid[30];
	int blue_x_msb = edid[31];
	int blue_y_msb = edid[32];
	int white_x_msb = edid[33];
	int white_y_msb = edid[34];

	// Call the helper function to calculate and print coordinates for each color.
	PrintChromaticityForColor("Red", red_x_msb, red_x_lsb, red_y_msb, red_y_lsb, cc);
	PrintChromaticityForColor("Green", green_x_msb, green_x_lsb, green_y_msb, green_y_lsb, cc);
	PrintChromaticityForColor("Blue", blue_x_msb, blue_x_lsb, blue_y_msb, blue_y_lsb, cc);
	PrintChromaticityForColor("White", white_x_msb, white_x_lsb, white_y_msb, white_y_lsb, cc);
}

static void DecodeEstablishedTimings(const BYTE* edid, PNODE node)
{
	BYTE byte35 = edid[35];
	BYTE byte36 = edid[36];
	BYTE byte37 = edid[37];
	LPSTR supportedTimings = NULL;

	// --- Byte 35 ---
	if (byte35 & 0x80)
		NWL_NodeAppendMultiSz(&supportedTimings, "720x400@70Hz (VGA)");
	if (byte35 & 0x40)
		NWL_NodeAppendMultiSz(&supportedTimings, "720x400@88Hz (XGA)");
	if (byte35 & 0x20)
		NWL_NodeAppendMultiSz(&supportedTimings, "640x480@60Hz (VGA)");
	if (byte35 & 0x10)
		NWL_NodeAppendMultiSz(&supportedTimings, "640x480@67Hz (Mac II");
	if (byte35 & 0x08)
		NWL_NodeAppendMultiSz(&supportedTimings, "640x480@72Hz");
	if (byte35 & 0x04)
		NWL_NodeAppendMultiSz(&supportedTimings, "640x480@75Hz");
	if (byte35 & 0x02)
		NWL_NodeAppendMultiSz(&supportedTimings, "800x600@56Hz");
	if (byte35 & 0x01)
		NWL_NodeAppendMultiSz(&supportedTimings, "800x600@60Hz");

	// --- Byte 36 ---
	if (byte36 & 0x80)
		NWL_NodeAppendMultiSz(&supportedTimings, "800x600@72Hz");
	if (byte36 & 0x40)
		NWL_NodeAppendMultiSz(&supportedTimings, "800x600@75Hz");
	if (byte36 & 0x20)
		NWL_NodeAppendMultiSz(&supportedTimings, "832x624@75Hz (Mac II)");
	if (byte36 & 0x10)
		NWL_NodeAppendMultiSz(&supportedTimings, "1024x768@87Hz (Interlaced)");
	if (byte36 & 0x08)
		NWL_NodeAppendMultiSz(&supportedTimings, "1024x768@60Hz");
	if (byte36 & 0x04)
		NWL_NodeAppendMultiSz(&supportedTimings, "1024x768@70Hz");
	if (byte36 & 0x02)
		NWL_NodeAppendMultiSz(&supportedTimings, "1024x768@75Hz");
	if (byte36 & 0x01)
		NWL_NodeAppendMultiSz(&supportedTimings, "1280x1024@75Hz");

	// --- Byte 37 ---
	if (byte37 & 0x80)
		NWL_NodeAppendMultiSz(&supportedTimings, "1152x870@75Hz (Mac II)");
	// Bits 6-0 are for other manufacturer modes and are not parsed here.
	if (byte37 & 0x7F)
		NWL_NodeAppendMultiSz(&supportedTimings, "Vendor-specific");

	if (supportedTimings)
	{
		NWL_NodeAttrSetMulti(node, "Established Timings", supportedTimings, 0);
		free(supportedTimings);
	}
}

static void DecodeEdid(const BYTE* edidData, DWORD edidSize, PNODE nm, const WCHAR* hwId)
{
	struct MONITOR_INFO mi = { 0 };
	// An EDID block must be at least 128 bytes.
	if (edidSize < 128)
		return;

	// Verify checksum of the base block. The sum of all 128 bytes must be a multiple of 256.
	BYTE checksum = 0;
	for (int i = 0; i < 128; ++i)
		checksum += edidData[i];
	if (checksum != 0)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Invalid EDID checksum");
		return;
	}

	// Manufacturer Info
	char vendorId[4];
	vendorId[0] = ((edidData[8] >> 2) & 0x1F) + 'A' - 1;
	vendorId[1] = (((edidData[8] & 0x03) << 3) | ((edidData[9] >> 5) & 0x07)) + 'A' - 1;
	vendorId[2] = (edidData[9] & 0x1F) + 'A' - 1;
	vendorId[3] = '\0';
	int productCode = (edidData[11] << 8) | edidData[10];

	// Serial Number (binary)
	DWORD serialNumber = (edidData[15] << 24) | (edidData[14] << 16) | (edidData[13] << 8) | edidData[12];

	// Manufacture Date
	int week = edidData[16];
	int year = edidData[17] + 1990;

	// EDID Version
	int edidMajor = edidData[18];
	int edidMinor = edidData[19];

	mi.Ver = (edidMajor << 8) | edidMinor;
	mi.Width = 10ULL * edidData[21];
	mi.Height = 10ULL * edidData[22];
	snprintf(mi.Serial, sizeof(mi.Serial), "%08X", serialNumber);

	NWL_NodeAttrSet(nm, "HWID", NWL_Ucs2ToUtf8(hwId), 0);
	NWL_NodeAttrSetf(nm, "ID", 0, "%s%04X", vendorId, productCode);
	NWL_GetPnpManufacturer(nm, &NWLC->NwPnpIds, vendorId);
	NWL_NodeAttrSetf(nm, "EDID Version", 0, "%d.%d", edidMajor, edidMinor);

	if (week > 0 && week <= 54)
		NWL_NodeAttrSetf(nm, "Date", 0, "%d, Week %d", year, week);

	// Parse video input parameters
	DecodeVideoInput(edidData, nm);

	// Parse gamma - 0xff means gamma is defined by DI-EXT block.
	if (edidData[23] != 0xFF)
	{
		// Formula: gamma = (datavalue + 100) / 100
		// To print with two decimal places using integers:
		int gammaTimes100 = 100 + edidData[23];
		NWL_NodeAttrSetf(nm, "Gamma", NAFLG_FMT_NUMERIC, "%d.%02d", gammaTimes100 / 100, gammaTimes100 % 100);
	}

	// Parse supported features bitmap
	DecodeSupportedFeatures(edidData, nm, &mi);

	// Parse chromaticity coordinates
	DecodeChromaticity(edidData, nm);

	// Parse established timings
	DecodeEstablishedTimings(edidData, nm);

	// Parse timings to find max resolution
	DecodeStandardTimings(edidData, nm, &mi);

	// Parse descriptors to find max resolution
	DecodeDescriptorBlocks(edidData, nm, &mi);

	NWL_NodeAttrSet(nm, "Display Name", mi.Name, 0);
	NWL_NodeAttrSet(nm, "Serial Number", mi.Serial, NAFLG_FMT_SENSITIVE);

	if (mi.XRes > 0 && mi.YRes > 0)
	{
		NWL_NodeAttrSetf(nm, "Max Resolution", 0, "%llux%llu",
			(unsigned long long)mi.XRes, (unsigned long long)mi.YRes);
		NWL_NodeAttrSetf(nm, "Max Refresh Rate (Hz)", NAFLG_FMT_NUMERIC, "%.2f", mi.Freq);
		UINT64 commonDivisor = GetGcd(mi.XRes, mi.YRes);
		NWL_NodeAttrSetf(nm, "Aspect Ratio", 0, "%llu:%llu",
			(unsigned long long)mi.XRes / commonDivisor, (unsigned long long)mi.YRes / commonDivisor);
	}

	if (mi.Width > 0 && mi.Height > 0)
	{
		NWL_NodeAttrSetf(nm, "Width (cm)", NAFLG_FMT_NUMERIC, "%.1f", mi.Width / 10.0);
		NWL_NodeAttrSetf(nm, "Height (cm)", NAFLG_FMT_NUMERIC, "%.1f", mi.Height / 10.0);
		UINT64 diagonalSq = (mi.Width * mi.Width) + (mi.Height * mi.Height);
		// Convert mm to inches (1 inch = 25.4 mm).
#ifdef USE_MATH_SQRT
		double diagonal = sqrt((double)diagonalSq) / 25.4;
		NWL_NodeAttrSetf(nm, "Diagonal (in)", NAFLG_FMT_NUMERIC, "%.2f", diagonal);
#else
		UINT64 diagInt = IntegerSqrt(diagonalSq);
		UINT64 inchesTimes100 = (diagInt * 100 + 127) / 254;
		NWL_NodeAttrSetf(nm, "Diagonal (in)", NAFLG_FMT_NUMERIC, "%llu.%02llu",
			(unsigned long long)inchesTimes100 / 100, (unsigned long long)inchesTimes100 % 100);
#endif
	}

	// Check for extension blocks
	int extensionBlocks = edidData[126];
	NWL_NodeAttrSetf(nm, "EDID Extension Blocks", NAFLG_FMT_NUMERIC, "%d", extensionBlocks);
	if (extensionBlocks > 0 && edidSize < (DWORD)(128 * (extensionBlocks + 1)))
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Truncated EDID data");
	// Note: Parsing of extension blocks (e.g., CTA-861) is not implemented here.
}

static BOOL
GetMonitorEdid(HDEVINFO hDevInfo, int idxMonitor, BYTE** edidData, DWORD* edidSize, WCHAR** hwId)
{
	*edidData = NULL;
	*edidSize = 0;
	*hwId = NULL;

	BOOL rc = FALSE;
	SP_DEVINFO_DATA devInfoData;
	devInfoData.cbSize = sizeof(devInfoData);

	// Enumerate the monitor devices.
	if (!SetupDiEnumDeviceInfo(hDevInfo, idxMonitor, &devInfoData))
		goto fail;
	rc = TRUE;

	// Get Hardware ID for the monitor
	DWORD hwidSize = 0;
	SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_HARDWAREID, NULL, NULL, 0, &hwidSize);
	if (hwidSize > 0)
	{
		*hwId = (WCHAR*)malloc(hwidSize);
		if (*hwId)
		{
			if (!SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)*hwId, hwidSize, NULL))
			{
				free(*hwId);
				*hwId = NULL;
			}
		}
	}

	// Windows XP: prevent duplicate entries
	if (NWLC->NwOsInfo.dwMajorVersion <= 5 && *hwId)
	{
		static WCHAR cachedHwId[32] = { 0 };
		if (idxMonitor && wcscmp(cachedHwId, *hwId) == 0)
			goto fail;
		wcscpy_s(cachedHwId, ARRAYSIZE(cachedHwId), *hwId);
	}

	// Open the device's registry key.
	HKEY devRegKey = SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
	if (devRegKey == INVALID_HANDLE_VALUE)
		goto fail;

	// Query for the size of the EDID data.
	DWORD dwType;
	DWORD dwSize = 0;
	if (RegQueryValueExW(devRegKey, L"EDID", NULL, &dwType, NULL, &dwSize) != ERROR_SUCCESS)
		goto fail;

	// Allocate a buffer and query the actual EDID data.
	*edidData = (BYTE*)malloc(dwSize);
	if (!*edidData)
		goto fail;

	LSTATUS lResult = RegQueryValueExW(devRegKey, L"EDID", NULL, &dwType, *edidData, &dwSize);

	RegCloseKey(devRegKey);

	if (lResult != ERROR_SUCCESS)
		goto fail;

	if (dwType != REG_BINARY || dwSize < 128)
		goto fail;

	*edidSize = dwSize;
	return rc;

fail:
	if (*edidData)
		free(*edidData);
	*edidData = NULL;
	*edidSize = 0;
	if (*hwId)
		free(*hwId);
	*hwId = NULL;
	return rc;
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
	DWORD i = 0;
	HDEVINFO hDevInfo = NULL;
	if (NWLC->EdidInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	hDevInfo = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_MONITOR, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "SetupDiGetClassDevs failed");
		goto disp;
	}

	for (i = 0; ; i++)
	{
		BYTE* edidData = NULL;
		DWORD edidSize = 0;
		WCHAR* hwId = NULL;

		if (!GetMonitorEdid(hDevInfo, i, &edidData, &edidSize, &hwId))
			break;
		if (!edidData)
			continue;
		PNODE nm = NWL_NodeAppendNew(node, "Monitor", NFLG_TABLE_ROW);
		DecodeEdid(edidData, edidSize, nm, hwId);
		NWL_NodeAttrSetRaw(nm, "Binary Data", edidData, (size_t)edidSize);
		free(edidData);
		free(hwId);
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);
disp:
	EnumDisp(node);
	return node;
}
