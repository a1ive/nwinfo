/*
 * Copyright 2008  Veselin Georgiev,
 * anrieffNOSPAM @ mgail_DOT.com (convert to gmail)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <string.h>
#include <ctype.h>
#include "libcpuid.h"
#include "libcpuid_util.h"
#include "libcpuid_internal.h"
#include "recog_intel.h"


/*
 * Useful links:
 * - List of Intel CPU microarchitectures: https://en.wikipedia.org/wiki/List_of_Intel_CPU_microarchitectures
 * - List of Intel Pentium processors: https://en.wikipedia.org/wiki/List_of_Intel_Pentium_processors
 * - List of Intel Celeron processors: https://en.wikipedia.org/wiki/List_of_Intel_Celeron_processors
 * - List of Intel Core processors: https://en.wikipedia.org/wiki/List_of_Intel_Core_processors
 * - List of Intel Xeon processors: https://en.wikipedia.org/wiki/List_of_Intel_Xeon_processors
*/
const struct match_entry_t cpudb_intel[] = {
//     F   M   S  EF    EM #cores L2$    L3$ Pattern                 Codename                   Technology
	{ -1, -1, -1, -1, -1,   1,    -1,    -1, { "",              0 }, "Unknown Intel CPU",       UNKN_STR },

	/* i486 */
	{  4, -1, -1, -1, -1,   1,    -1,    -1, { "",              0 }, "Unknown i486",            UNKN_STR },
	{  4,  0, -1, -1, -1,   1,    -1,    -1, { "",              0 }, "i486 DX-25/33",           UNKN_STR },
	{  4,  1, -1, -1, -1,   1,    -1,    -1, { "",              0 }, "i486 DX-50",              UNKN_STR },
	{  4,  2, -1, -1, -1,   1,    -1,    -1, { "",              0 }, "i486 SX",                 UNKN_STR },
	{  4,  3, -1, -1, -1,   1,    -1,    -1, { "",              0 }, "i486 DX2",                UNKN_STR },
	{  4,  4, -1, -1, -1,   1,    -1,    -1, { "",              0 }, "i486 SL",                 UNKN_STR },
	{  4,  5, -1, -1, -1,   1,    -1,    -1, { "",              0 }, "i486 SX2",                UNKN_STR },
	{  4,  7, -1, -1, -1,   1,    -1,    -1, { "",              0 }, "i486 DX2 WriteBack",      UNKN_STR },
	{  4,  8, -1, -1, -1,   1,    -1,    -1, { "",              0 }, "i486 DX4",                UNKN_STR },
	{  4,  9, -1, -1, -1,   1,    -1,    -1, { "",              0 }, "i486 DX4 WriteBack",      UNKN_STR },

	/* P6 CPUs */
	{  5,  0, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium A-Step",    UNKN_STR  },
	{  5,  1, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium 1",         "0.8 µm"  },
	{  5,  2, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium 1",         "0.35 µm" },
	{  5,  3, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium OverDrive", UNKN_STR  },
	{  5,  4, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium 1",         "0.35 µm" },
	{  5,  7, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium 1",         "0.35 µm" },
	{  5,  8, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium MMX",       "0.25 µm" },

	/* P6 CPUs */
	{  6,  0, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium Pro",                UNKN_STR  },
	{  6,  1, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium Pro",                UNKN_STR  },
	{  6,  3, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium II (Klamath)",       "0.18 µm" },
	{  6,  5, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium II (Deschutes)",     "0.18 µm" },
	{  6,  5, -1, -1, -1,   1,    -1,    -1, { "Pentium(R) M",  4 }, "Mobile Pentium II (Tonga)",  "0.18 µm" },
	{  6,  6, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium II (Dixon)",         "0.25 µm" },
	{  6,  3, -1, -1, -1,   1,    -1,    -1, { "Xeon(TM)",      2 }, "P-II Xeon (Klamath)",        "0.35 µm" },
	{  6,  5, -1, -1, -1,   1,    -1,    -1, { "Xeon(TM)",      2 }, "P-II Xeon (Drake)",          "0.25 µm" },
	{  6,  6, -1, -1, -1,   1,    -1,    -1, { "Xeon(TM)",      2 }, "P-II Xeon (Dixon)",          "0.25 µm" },
	{  6,  5, -1, -1, -1,   1,    -1,    -1, { "Celeron(R)",    2 }, "P-II Celeron (Covington)",   "0.25 µm" },
	{  6,  6, -1, -1, -1,   1,    -1,    -1, { "Celeron(R)",    2 }, "P-II Celeron (Mendocino)",   "0.25 µm" },
	{  6,  7, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium III (Katmai)",       "0.25 µm" },
	{  6,  8, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium III (Coppermine)",   "0.18 µm" },
	{  6, 10, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium III (Coppermine)",   "0.18 µm" },
	{  6, 11, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",    2 }, "Pentium III (Tualatin)",     "0.13 µm" },
	{  6, 11, -1, -1, -1,   1,   512,    -1, { "Pentium(R)",    2 }, "Pentium III (Tualatin)",     "0.13 µm" },
	{  6,  7, -1, -1, -1,   1,    -1,    -1, { "Xeon(TM)",      2 }, "P-III Xeon (Tanner)",        "0.25 µm" },
	{  6,  8, -1, -1, -1,   1,    -1,    -1, { "Xeon(TM)",      2 }, "P-III Xeon (Cascades)",      "0.18 µm" },
	{  6, 10, -1, -1, -1,   1,    -1,    -1, { "Xeon(TM)",      2 }, "P-III Xeon (Cascades)",      "0.18 µm" },
	{  6, 11, -1, -1, -1,   1,    -1,    -1, { "Xeon(TM)",      2 }, "P-III Xeon (Tualatin)",      "0.13 µm" },
	{  6,  7, -1, -1, -1,   1,   128,    -1, { "Celeron(R)",    2 }, "P-III Celeron (Katmai)",     "0.25 µm" },
	{  6,  8, -1, -1, -1,   1,   128,    -1, { "Celeron(R)",    2 }, "P-III Celeron (Coppermine)", "0.18 µm" },
	{  6, 10, -1, -1, -1,   1,   128,    -1, { "Celeron(R)",    2 }, "P-III Celeron (Coppermine)", "0.18 µm" },
	{  6, 11, -1, -1, -1,   1,   256,    -1, { "Celeron(R)",    2 }, "P-III Celeron (Tualatin)",   "0.13 µm" },

	/* NetBurst CPUs */
	/* Willamette (2000, 180 nm): */
	{ 15,  0, -1, 15, -1,   1,    -1,    -1, { "Pentium(R)",        2 }, "Pentium 4 (Willamette)",   "0.18 µm" },
	{ 15,  1, -1, 15, -1,   1,    -1,    -1, { "Pentium(R)",        2 }, "Pentium 4 (Willamette)",   "0.18 µm" },
	{ 15,  0, -1, 15, -1,   1,    -1,    -1, { "Pentium(R) 4 - M",  6 }, "Mobile P-4 (Willamette)",  "0.18 µm" },
	{ 15,  1, -1, 15, -1,   1,    -1,    -1, { "Pentium(R) 4 - M",  6 }, "Mobile P-4 (Willamette)",  "0.18 µm" },
	{ 15,  1, -1, 15, -1,   1,    -1,    -1, { "Celeron(R)",        2 }, "P-4 Celeron (Willamette)", "0.18 µm" },
	{ 15,  0, -1, 15, -1,   1,    -1,    -1, { "Xeon(TM)",          2 }, "Xeon (Foster)",            "0.18 µm" },
	{ 15,  1, -1, 15, -1,   1,    -1,    -1, { "Xeon(TM)",          2 }, "Xeon (Foster)",            "0.18 µm" },
	/* Northwood / Mobile Pentium 4 / Banias (2002, 130 nm): */
	{ 15,  2, -1, 15, -1,   1,    -1,    -1, { "Pentium(R) 4",      2 }, "Pentium 4 (Northwood)",   "0.13 µm" },
	{ 15,  2, -1, 15, -1,   1,    -1,    -1, { "Pentium(R) 4 - M",  6 }, "Mobile P-4 (Northwood)",  "0.13 µm" },
	{ 15,  2, -1, 15, -1,   1,    -1,    -1, { "Celeron(R)",        2 }, "P-4 Celeron (Northwood)", "0.13 µm" },
	{  6,  9, -1, -1, -1,   1,    -1,    -1, { "Pentium(R)",        2 }, "Pentium M (Banias)",      "0.13 µm" },
	{  6,  9, -1, -1, -1,   1,    -1,    -1, { "Pentium(R) 4 - M",  6 }, "Pentium M (Banias)",      "0.13 µm" },
	{  6,  9, -1, -1, -1,   1,    -1,    -1, { "Celeron(R)",        2 }, "Celeron M (Banias)",      "0.13 µm" },
	{  6,  9, -1, -1, -1,   1,    -1,    -1, { "Celeron(R) M",      4 }, "Celeron M (Banias)",      "0.13 µm" },
	{  6,  9, -1, -1, -1,   1,     0,    -1, { "Celeron(R)",        2 }, "Celeron M (Shelton)",     "0.13 µm" },
	{ 15,  2, -1, 15, -1,   1,    -1,    -1, { "Xeon(TM)",          2 }, "Xeon (Prestonia)",        "0.13 µm" },
	{ 15,  2, -1, 15, -1,   1,    -1,    -1, { "Xeon(TM) MP",       4 }, "Xeon (Gallatin)",         "0.13 µm" },
	/* Prescott / Dothan (2004, 90 nm): */
	{ 15,  3, -1, 15, -1,   1,    -1,    -1, { "Pentium(R) 4",      4 }, "Pentium 4 (Prescott)",     "90 nm" },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, { "Pentium(R) 4",      4 }, "Pentium 4 (Prescott)",     "90 nm" },
	{ 15,  3, -1, 15, -1,   1,    -1,    -1, { "Pentium(R) 4 - M",  6 }, "Mobile P-4 (Prescott)",    "90 nm" },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, { "Pentium(R) 4 - M",  6 }, "Mobile P-4 (Prescott)",    "90 nm" },
	{ 15,  3, -1, 15, -1,   1,    -1,    -1, { "Celeron(R)",        2 }, "P-4 Celeron D (Prescott)", "90 nm" },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, { "Celeron(R)",        2 }, "P-4 Celeron D (Prescott)", "90 nm" },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, { "Pentium(R) D",      4 }, "Pentium D (SmithField)",   "90 nm" },
	{  6, 13, -1, -1, -1,   1,    -1,    -1, { "Pentium(R) M",      4 }, "Pentium M (Dothan)",       "90 nm" },
	{  6, 13, -1, -1, -1,   1,    -1,    -1, { "Pentium(R) 4 - M",  6 }, "Pentium M (Dothan)",       "90 nm" },
	{  6, 13, -1, -1, -1,   1,    -1,    -1, { "Celeron(R)",        2 }, "Celeron M (Dothan)",       "90 nm" },
	{  6, 13, -1, -1, -1,   1,    -1,    -1, { "Celeron(R) M",      4 }, "Celeron M (Dothan)",       "90 nm" },
	{ 15,  3, -1, 15, -1,   1,    -1,    -1, { "Xeon(TM)",          2 }, "Xeon (Nocona)",            "90 nm" },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, { "Xeon(TM)",          2 }, "Xeon (Nocona)",            "90 nm" },
	{ 15,  4,  3, 15, -1,   1,    -1,    -1, { "Xeon(TM)",          2 }, "Xeon (Irwindale)",         "90 nm" },
	{ 15,  4, 10, 15, -1,   1,    -1,    -1, { "Xeon(TM)",          2 }, "Xeon (Irwindale)",         "90 nm" },
	{ 15,  4,  1, 15, -1,   1,    -1,    -1, { "Xeon(TM)",          2 }, "Xeon (Cranford)",          "90 nm" },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, { "Xeon(TM)",          2 }, "Xeon (Potomac)",           "90 nm" },
	{ 15,  4,  8, 15, -1,   1,    -1,    -1, { "Xeon(TM)",          2 }, "Xeon (Paxville)",          "90 nm" },
	/* Cedar Mill / Yonah / Presler (2006, 65 nm): */
	{ 15,  6, -1, 15, -1,   1,    -1,    -1, { "Pentium(R)",                 2 }, "Pentium 4 (Cedar Mill)",            "65 nm" },
	{ 15,  6, -1, 15, -1,   1,    -1,    -1, { "Pentium(R) 4 - M",           6 }, "Mobile P-4 (Cedar Mill)",           "65 nm" },
	{ 15,  6, -1, 15, -1,   1,    -1,    -1, { "Celeron(R) D",               4 }, "P-4 Celeron D (Cedar Mill)",        "65 nm" },
	{  6, 14, -1, 14, -1,   1,    -1,    -1, { "Core(TM) [UT]1#[05]0",       6 }, "Core Solo (Yonah)",                 "65 nm" },
	{  6, 14, -1, 14, -1,   1,    -1,    -1, { "Core(TM) 1#[05]0",           4 }, "Core Solo (Yonah)",                 "65 nm" },
	{  6, 14, -1, 14, -1,   2,    -1,    -1, { "Core(TM) Duo [UTL]2#[05]#",  6 }, "Core Duo (Yonah)",                  "65 nm" },
	{  6, 14, -1, 14, -1,   2,    -1,    -1, { "Core(TM) Duo 2#[05]#",       4 }, "Core Duo (Yonah)",                  "65 nm" },
	{  6, 14, -1, 14, -1,  -1,    -1,    -1, { "Celeron(R) 215",             6 }, "Celeron (Yonah-512)",               "65 nm" },
	{  6, 14, -1, 14, -1,  -1,    -1,    -1, { "Celeron(R) M",               4 }, "Celeron (Yonah-1024)",              "65 nm" },
	{ 15,  6, -1, 15, -1,   1,    -1,    -1, { "Pentium(R) D",               4 }, "Pentium D (Presler)",               "65 nm" },
	{ 15,  6,  2, 15, -1,   1,    -1,    -1, { "Pentium(R)",                 2 }, "Pentium Extreme Edition (Presler)", "65 nm" },
	{ 15,  6,  4, 15, -1,   1,    -1,    -1, { "Xeon(TM)",                   2 }, "Xeon (Dempsey)",                    "65 nm" },
	{ 15,  6,  6, 15, -1,   1,    -1,    -1, { "Xeon(TM)",                   2 }, "Xeon (Tulsa)",                      "65 nm" },
	{ 15,  6,  8, 15, -1,   1,    -1,    -1, { "Xeon(TM)",                   2 }, "Xeon (Tulsa)",                      "65 nm" },

	/* Core CPUs (2006, 65 nm): https://en.wikipedia.org/wiki/Intel_Core_(microarchitecture)*/
	{  6, 15, -1, -1, -1,   2,  2048,    -1, { "Core(TM)2 Duo E6###",    8 }, "Core 2 Duo (Conroe-2M)",        "65 nm" },
	{  6, 15, -1, -1, -1,   2,  4096,    -1, { "Core(TM)2 Duo E6###",    8 }, "Core 2 Duo (Conroe)",           "65 nm" },
	{  6, 15, -1, -1, -1,   2,  4096,    -1, { "Core(TM)2 6###",         4 }, "Core 2 Duo (Conroe)",           "65 nm" },
	{  6, 15, -1, -1, -1,   2,  4096,    -1, { "Core(TM)2 X6###",        6 }, "Core 2 Extreme (Conroe XE)",    "65 nm" },
	{  6, 15, -1, -1, -1,   4,  4096,    -1, { "Core(TM)2 Quad Q6###",   8 }, "Core 2 Quad (Kentsfield)",      "65 nm" },
	{  6, 15, -1, -1, -1,   2,  2048,    -1, { "Core(TM)2 Duo E4###",    8 }, "Core 2 Duo (Allendale)",        "65 nm" },
	{  6, 15, -1, -1, 15,   2,  2048,    -1, { "Core(TM)2 U7###",        6 }, "Core 2 Duo (Merom-2M)",         "65 nm" },
	{  6, 15, -1, -1, 15,   2,  2048,    -1, { "Core(TM)2 T[57]###",     6 }, "Core 2 Duo (Merom-2M)",         "65 nm" },
	{  6, 15, -1, -1, 15,   2,  4096,    -1, { "Core(TM)2 T7###",        6 }, "Core 2 Duo (Merom)",            "65 nm" },
	{  6, 15, -1, -1, 15,   2,  4096,    -1, { "Core(TM)2 S[LP]7###",    6 }, "Core 2 Duo (Merom)",            "65 nm" },
	{  6, 15, -1, -1, 15,   2,  4096,    -1, { "Core(TM)2 L7###",        6 }, "Core 2 Duo (Merom)",            "65 nm" },
	{  6, 15, -1, -1, 15,   2,    -1,    -1, { "Pentium(R) Dual E2###",  8 }, "Pentium Dual-Core (Allendale)", "65 nm" },
	{  6, 15, -1, -1, 15,   2,    -1,    -1, { "Celeron(R) E1###",       6 }, "Celeron (Allendale)",           "65 nm" },
	{  6,  6, -1, -1, 22,   1,    -1,    -1, { "Celeron(R) [24]##",      4 }, "Celeron (Conroe-L)",            "65 nm" },
	{  6, 14, -1, -1, 14,   1,    -1,    -1, { "Xeon(R) 51##",           4 }, "Xeon LV (Woodcrest)",           "65 nm" },
	{  6, 15, -1, -1, 15,   2,    -1,    -1, { "Xeon(R) 51##",           4 }, "Xeon (Woodcrest)",              "65 nm" },
	{  6, 15, -1, -1, 15,   2,    -1,    -1, { "Xeon(R) 30##",           4 }, "Xeon (Conroe)",                 "65 nm" },
	{  6, 15, -1, -1, 15,   4,    -1,    -1, { "Xeon(R) X32##",          6 }, "Xeon (Kentsfield)",             "65 nm" },
	{  6, 15, -1, -1, 15,   4,    -1,    -1, { "Xeon(R) [EXL]53##",      6 }, "Xeon (Clovertown)",             "65 nm" },

	/* Penryn CPUs (2007, 45 nm): https://en.wikipedia.org/wiki/Penryn_(microarchitecture) */
	{  6,  7, -1, -1, 23,   2,  1024,    -1, { "Celeron(R) E3###",            6 }, "Celeron (Wolfdale-3M)",        "45 nm" },
	{  6,  7, -1, -1, 23,   2,  1024,    -1, { "Pentium(R) E2###",            6 }, "Celeron (Wolfdale-3M)",        "45 nm" },
	{  6,  7, -1, -1, 23,   2,  2048,    -1, { "Pentium(R) E[56]###",         6 }, "Pentium (Wolfdale-3M)",        "45 nm" },
	{  6,  7, -1, -1, 23,   2,  3072,    -1, { "Core(TM)2 Duo E7###",         8 }, "Core 2 Duo (Wolfdale-3M)",     "45 nm" },
	{  6,  7, -1, -1, 23,   2,  6144,    -1, { "Core(TM)2 Duo E8###",         8 }, "Core 2 Duo (Wolfdale)",        "45 nm" },
	{  6,  7, -1, -1, 23,   2,  1024,    -1, { "Pentium(R) Dual-Core T4###",  8 }, "Pentium Dual-Core (Penryn-L)", "45 nm" },
	{  6,  7, -1, -1, 23,   1,  1024,    -1, { "Celeron(R) [79]##",           4 }, "Celeron (Penryn-L)",           "45 nm" },
	{  6,  7, -1, -1, 23,   2,  3072,    -1, { "Core(TM)2 Duo SU[78]###",     8 }, "Core 2 Duo (Penryn-3M)",       "45 nm" },
	{  6,  7, -1, -1, 23,   2,  3072,    -1, { "Core(TM)2 Duo P[78]###",      8 }, "Core 2 Duo (Penryn-3M)",       "45 nm" },
	{  6,  7, -1, -1, 23,   2,  2048,    -1, { "Core(TM)2 Duo T6###",         8 }, "Core 2 Duo (Penryn-3M)",       "45 nm" },
	{  6,  7, -1, -1, 23,   2,  3072,    -1, { "Core(TM)2 Duo T8###",         8 }, "Core 2 Duo (Penryn-3M)",       "45 nm" },
	{  6,  7, -1, -1, 23,   2,  6144,    -1, { "Core(TM)2 Duo S[LP]9###",     8 }, "Core 2 Duo (Penryn)",          "45 nm" },
	{  6,  7, -1, -1, 23,   2,  6144,    -1, { "Core(TM)2 Duo [PT]9###",      8 }, "Core 2 Duo (Penryn)",          "45 nm" },
	{  6,  7, -1, -1, 23,   2,  6144,    -1, { "Core(TM)2 Duo E8###",         8 }, "Core 2 Duo (Penryn)",          "45 nm" },
	{  6,  7, -1, -1, 23,   4,  2048,    -1, { "Core(TM)2 Quad Q8###",        8 }, "Core 2 Quad (Yorkfield-6M)",   "45 nm" }, /* 2×2 MB L2$ */
	{  6,  7, -1, -1, 23,   4,  3072,    -1, { "Core(TM)2 Quad Q9#0#",        8 }, "Core 2 Quad (Yorkfield-6M)",   "45 nm" }, /* 2×3 MB L2$ */
	{  6,  7, -1, -1, 23,   4,  6144,    -1, { "Core(TM)2 Quad Q9#5#",        8 }, "Core 2 Quad (Yorkfield)",      "45 nm" }, /* 2×6 MB L2$ */
	{  6,  7, -1, -1, 23,   2,    -1,    -1, { "Xeon(R) [EL]31##",            6 }, "Xeon (Wolfdale)",              "45 nm" },
	{  6,  7, -1, -1, 23,   2,    -1,    -1, { "Xeon(R) [EXL]52##",           6 }, "Xeon (Wolfdale DP)",           "45 nm" },
	{  6,  7, -1, -1, 23,   4,    -1,    -1, { "Xeon(R) [EXL]54##",           6 }, "Xeon (Harpertown)",            "45 nm" },
	{  6,  7, -1, -1, 23,   4,    -1,    -1, { "Xeon(R) [XL]33##",            6 }, "Xeon (Yorkfield)" ,            "45 nm" },
	{  6, 13, -1, -1, 29,  -1,    -1,    -1, { "Xeon(R) [EXL]74##",           6 }, "Xeon (Dunnington)",            "45 nm" },

	/* Nehalem CPUs (2008, 1st Core i gen, 45 nm): https://en.wikipedia.org/wiki/Nehalem_(microarchitecture) */
	{  6, 10, -1, -1, 26,  -1,    -1,    -1, { "Xeon(R) [WELX]5###",        6 }, "Xeon (Gainestown)",            "45 nm" },
	{  6, 10, -1, -1, 26,  -1,    -1,    -1, { "Xeon(R) [WELX]3###",        6 }, "Xeon (Bloomfield)",            "45 nm" },
	{  6, 10, -1, -1, 26,  -1,    -1,    -1, { "Core(TM) i7 9#5",           8 }, "Core i7 Extreme (Bloomfield)", "45 nm" },
	{  6, 10, -1, -1, 26,  -1,    -1,    -1, { "Core(TM) i7 9#0",           8 }, "Core i7 (Bloomfield)",         "45 nm" },
	{  6, 14, -1, -1, 30,  -1,    -1,    -1, { "Core(TM) i7 8##",           8 }, "Core i7 (Lynnfield)",          "45 nm" },
	{  6, 14, -1, -1, 30,  -1,    -1,    -1, { "Core(TM) i5 7##",           8 }, "Core i5 (Lynnfield)",          "45 nm" },
	{  6, 14, -1, -1, 30,  -1,    -1,    -1, { "Core(TM) i7 [QX] [789]##", 10 }, "Core i7 (Clarksfield)",        "45 nm" },
	{  6, 14, -1, -1, 30,  -1,    -1,    -1, { "Core(TM) [QX] [789]##",     8 }, "Core i7 (Clarksfield)",        "45 nm" },

	/* Bonnell CPUs (2008, Atom, 45 nm): https://en.wikipedia.org/wiki/Bonnell_(microarchitecture) */
	{  6,  6, -1, -1, 38,  -1,    -1,    -1, { "Atom(TM) E6##T",       8 }, "Atom (Tunnel Creek)", "45 nm" },
	{  6,  6, -1, -1, 38,  -1,    -1,    -1, { "Atom(TM) E6##",        6 }, "Atom (Tunnel Creek)", "45 nm" },
	{  6,  6, -1, -1, 38,  -1,    -1,    -1, { "Atom(TM) E6##C",       8 }, "Atom (Stellarton)",   "45 nm" },
	{  6, 12, -1, -1, 28,  -1,    -1,    -1, { "Atom(TM) Z5##",        6 }, "Atom (Silverthorne)", "45 nm" },
	{  6, 12, -1, -1, 28,  -1,    -1,    -1, { "Atom(TM) N2##",        6 }, "Atom (Diamondville)", "45 nm" },
	{  6, 12, -1, -1, 28,  -1,    -1,    -1, { "Atom(TM) [23]##",      6 }, "Atom (Diamondville)", "45 nm" },
	{  6,  6, -1, -1, 38,  -1,    -1,    -1, { "Atom(TM) Z6##",        6 }, "Atom (Lincroft)",     "45 nm" },
	{  6, 12, -1, -1, 28,  -1,    -1,    -1, { "Atom(TM) Z6##",        6 }, "Atom (Lincroft)",     "45 nm" },
	{  6, 12, -1, -1, 28,  -1,    -1,    -1, { "Atom(TM) [ND][45]##",  6 }, "Atom (Pineview)",     "45 nm" },

	/* Westmere CPUs (2010, 1st Core i gen, 32 nm): https://en.wikipedia.org/wiki/Westmere_(microarchitecture) */
	{  6, 14, -1, -1, 46,  -1,    -1,    -1, { "Xeon(R) [EXL]75##",        6 }, "Xeon 7000 (Beckton)",         "32 nm" },
	{  6, 14, -1, -1, 46,  -1,    -1,    -1, { "Xeon(R) E65##",            6 }, "Xeon 6000 (Beckton)",         "32 nm" },
	{  6, 14, -1, -1, 46,  -1,    -1,    -1, { "Xeon(R) [XELW]5[56]##",    6 }, "Xeon 5000 (Beckton)",         "32 nm" },
	{  6, 14, -1, -1, 46,  -1,    -1,    -1, { "Xeon(R) [XLW]3[456]###",   6 }, "Xeon 3000 (Beckton)",         "32 nm" },
	{  6, 15, -1, -1, 47,  -1,    -1,    -1, { "Xeon(R) E7-#8##",          6 }, "Xeon E7 (Westmere-EX)",       "32 nm" },
	{  6, 12, -1, -1, 44,  -1,    -1,    -1, { "Xeon(R) [XEL]5###",        6 }, "Xeon (Westmere-EP)",          "32 nm" },
	{  6, 12, -1, -1, 44,  -1,    -1,    -1, { "Xeon(R) W3###",            6 }, "Xeon (Gulftown)",             "32 nm" },
	{  6, 12, -1, -1, 44,  -1,    -1,    -1, { "Core(TM) i7 X 9##",       10 }, "Core i7 Extreme (Gulftown)",  "32 nm" },
	{  6, 12, -1, -1, 44,  -1,    -1,    -1, { "Core(TM) i7 9##",          8 }, "Core i7 (Gulftown)",          "32 nm" },
	{  6,  5, -1, -1, 37,  -1,    -1,    -1, { "Xeon(R) L3###",            6 }, "Xeon (Clarkdale)",            "32 nm" },
	{  6,  5, -1, -1, 37,  -1,    -1,    -1, { "Core(TM) i5 6##",          8 }, "Core i5 (Clarkdale)",         "32 nm" },
	{  6,  5, -1, -1, 37,  -1,    -1,    -1, { "Core(TM) i3 5##",          8 }, "Core i3 (Clarkdale)",         "32 nm" },
	{  6,  5, -1, -1, 37,  -1,    -1,    -1, { "Pentium(R) G6###",         6 }, "Pentium (Clarkdale)",         "32 nm" },
	{  6,  5, -1, -1, 37,  -1,    -1,    -1, { "Celeron(R) G1###",         6 }, "Celeron (Clarkdale)",         "32 nm" },
	{  6,  5, -1, -1, 37,  -1,    -1,    -1, { "Core(TM) i7 M 6##",        8 }, "Core i7 (Arrandale)",         "32 nm" },
	{  6,  5, -1, -1, 37,  -1,    -1,    -1, { "Core(TM) i5 M [45]##",     8 }, "Core i5 (Arrandale)",         "32 nm" },
	{  6,  5, -1, -1, 37,  -1,    -1,    -1, { "Core(TM) i3 M 3##",        8 }, "Core i3 (Arrandale)",         "32 nm" },
	{  6,  5, -1, -1, 37,  -1,    -1,    -1, { "Pentium(R) P6###",         6 }, "Pentium (Arrandale)",         "32 nm" },
	{  6,  5, -1, -1, 37,  -1,    -1,    -1, { "Pentium(R) 5U###",         6 }, "Pentium (Arrandale)",         "32 nm" },
	{  6,  5, -1, -1, 37,  -1,    -1,    -1, { "Celeron(R) P4###",         6 }, "Celeron (Arrandale)",         "32 nm" },
	{  6,  5, -1, -1, 37,  -1,    -1,    -1, { "Celeron(R) U3###",         6 }, "Celeron (Arrandale)",         "32 nm" },

	/* Saltwell CPUs (2011, Atom, 32 nm): https://en.wikipedia.org/wiki/Bonnell_(microarchitecture)#Third_generation_cores */
	{  6, 12, -1, -1, -1,  -1,    -1,    -1, { "Atom(TM) [ND]2###",    6 }, "Atom (Cedarview)",    "32 nm" },
	{  6,  6, -1, -1, 54,  -1,    -1,    -1, { "Atom(TM) [ND]2###",    6 }, "Atom (Cedarview)",    "32 nm" },
	{  6,  7, -1, -1, 39,  -1,    -1,    -1, { "Atom(TM) Z2###",       6 }, "Atom (Penwell)",      "32 nm" },

	/* Sandy Bridge CPUs (2011, 2nd Core i gen, 32 nm): https://en.wikipedia.org/wiki/Sandy_Bridge */
	{  6, 10, -1, -1, 42,  -1,    -1,    -1, { "Xeon(R) E5####[LW]",    8 }, "Xeon E5 (Sandy Bridge)",           "32 nm" },
	{  6, 10, -1, -1, 42,  -1,    -1,    -1, { "Xeon(R) E5####",        6 }, "Xeon E5 (Sandy Bridge)",           "32 nm" },
	{  6, 10, -1, -1, 42,  -1,    -1,    -1, { "Xeon(R) E3####[CL]",    8 }, "Xeon E3 (Sandy Bridge)",           "32 nm" },
	{  6, 10, -1, -1, 42,  -1,    -1,    -1, { "Xeon(R) E3####",        6 }, "Xeon E3 (Sandy Bridge)",           "32 nm" },
	{  6, 10, -1, -1, 42,  -1,    -1,    -1, { "Core(TM) i7-2###",      8 }, "Core i7 (Sandy Bridge)",           "32 nm" },
	{  6, 10, -1, -1, 42,  -1,    -1,    -1, { "Core(TM) i5-2###",      8 }, "Core i5 (Sandy Bridge)",           "32 nm" },
	{  6, 10, -1, -1, 42,  -1,    -1,    -1, { "Core(TM) i3-2###",      8 }, "Core i3 (Sandy Bridge)",           "32 nm" },
	{  6, 10, -1, -1, 42,  -1,    -1,    -1, { "Pentium(R) G[68]##",    6 }, "Pentium (Sandy Bridge)",           "32 nm" },
	{  6, 10, -1, -1, 42,  -1,    -1,    -1, { "Celeron(R) G[45]##",    6 }, "Celeron (Sandy Bridge)",           "32 nm" },
	{  6, 13, -1, -1, 45,  -1,    -1,    -1, { "Core(TM) i7-3###[KX]", 10 }, "Core i7 Extreme (Sandy Bridge-E)", "32 nm" },
	{  6, 13, -1, -1, 45,  -1,    -1,    -1, { "Xeon(R) E5-####",       4 }, "Xeon E5 (Sandy Bridge-E)",         "32 nm" },
	{  6, 13, -1, -1, 45,  -1,    -1,    -1, { "Xeon(R) E3-####",       4 }, "Xeon E3 (Sandy Bridge-E)",         "32 nm" },

	/* Ivy Bridge CPUs (2012, 3rd Core i gen, 22 nm): https://en.wikipedia.org/wiki/Ivy_Bridge_(microarchitecture) */
	{  6, 10, -1, -1, 58,  -1,    -1,    -1, { "Xeon(R) E7-####L v2",     8 }, "Xeon E7 (Ivy Bridge)",           "22 nm" },
	{  6, 10, -1, -1, 58,  -1,    -1,    -1, { "Xeon(R) E7-#### v2",      6 }, "Xeon E7 (Ivy Bridge)",           "22 nm" },
	{  6, 10, -1, -1, 58,  -1,    -1,    -1, { "Xeon(R) E5-####[LW] v2",  8 }, "Xeon E5 (Ivy Bridge)",           "22 nm" },
	{  6, 10, -1, -1, 58,  -1,    -1,    -1, { "Xeon(R) E5-#### v2",      6 }, "Xeon E5 (Ivy Bridge)",           "22 nm" },
	{  6, 10, -1, -1, 58,  -1,    -1,    -1, { "Xeon(R) E3-####[CL] v2",  8 }, "Xeon E3 (Ivy Bridge)",           "22 nm" },
	{  6, 10, -1, -1, 58,  -1,    -1,    -1, { "Xeon(R) E3-#### v2",      6 }, "Xeon E3 (Ivy Bridge)",           "22 nm" },
	{  6, 10, -1, -1, 58,  -1,    -1,    -1, { "Core(TM) i7-3###",        8 }, "Core i7 (Ivy Bridge)",           "22 nm" },
	{  6, 10, -1, -1, 58,  -1,    -1,    -1, { "Core(TM) i5-3###",        8 }, "Core i5 (Ivy Bridge)",           "22 nm" },
	{  6, 10, -1, -1, 58,  -1,    -1,    -1, { "Core(TM) i3-3###",        8 }, "Core i3 (Ivy Bridge)",           "22 nm" },
	{  6, 10, -1, -1, 58,  -1,    -1,    -1, { "Pentium(R) G2###",        6 }, "Pentium (Ivy Bridge)",           "22 nm" },
	{  6, 10, -1, -1, 58,  -1,    -1,    -1, { "Celeron(R) G1###",        6 }, "Celeron (Ivy Bridge)",           "22 nm" },
	{  6, 14, -1, -1, 62,  -1,    -1,    -1, { "Xeon(R) E7-#### v2",      6 }, "Xeon E7 (Ivy Bridge-E)",         "22 nm" },
	{  6, 14, -1, -1, 62,  -1,    -1,    -1, { "Xeon(R) E5-#### v2",      6 }, "Xeon E5 (Ivy Bridge-E)",         "22 nm" },
	{  6, 14, -1, -1, 62,  -1,    -1,    -1, { "Xeon(R) E3-#### v2",      6 }, "Xeon E3 (Ivy Bridge-E)",         "22 nm" },
	{  6, 14, -1, -1, 62,  -1,    -1,    -1, { "Core(TM) i7-4###X",      10 }, "Core i7 Extreme (Ivy Bridge-E)", "22 nm" },
	{  6, 14, -1, -1, 62,  -1,    -1,    -1, { "Core(TM) i7-4###K",       8 }, "Core i7 (Ivy Bridge-E)",         "22 nm" },

	/* Silvermont CPUs (2013, Atom, 22 nm): https://en.wikipedia.org/wiki/Silvermont */
	{  6,  7, -1, -1,  55,  -1,    -1,    -1, { "Pentium(R) J2###",  6 }, "Pentium (Bay Trail-D)", "22 nm" },
	{  6,  7, -1, -1,  55,  -1,    -1,    -1, { "Celeron(R) J1###",  6 }, "Celeron (Bay Trail-D)", "22 nm" },
	{  6,  7, -1, -1,  55,  -1,    -1,    -1, { "Pentium(R) N3###",  6 }, "Pentium (Bay Trail-M)", "22 nm" },
	{  6,  7, -1, -1,  55,  -1,    -1,    -1, { "Celeron(R) N2###",  6 }, "Celeron (Bay Trail-M)", "22 nm" },
	{  6,  7, -1, -1,  55,  -1,    -1,    -1, { "Atom(TM) Z3###",    6 }, "Atom (Bay Trail-T)",    "22 nm" },
	{  6,  7, -1, -1,  55,  -1,    -1,    -1, { "Atom(TM) E3###",    6 }, "Atom (Bay Trail-I)",    "22 nm" },
	{  6, 13, -1, -1,  77,  -1,    -1,    -1, { "Atom(TM) C2##0",    8 }, "Atom (Avoton)",         "22 nm" },
	{  6, 13, -1, -1,  77,  -1,    -1,    -1, { "Atom(TM) C2##[68]", 8 }, "Atom (Rangeley)",       "22 nm" },

	/* Haswell CPUs (2013, 4th Core i gen, 22 nm): https://en.wikipedia.org/wiki/Haswell_(microarchitecture) */
	{  6, 12, -1, -1, 60,  -1,    -1,    -1, { "Xeon(R) E7-####L v3",       8 }, "Xeon E7 (Haswell)",         "22 nm" },
	{  6, 12, -1, -1, 60,  -1,    -1,    -1, { "Xeon(R) E7-#### v3",        6 }, "Xeon E7 (Haswell)",         "22 nm" },
	{  6, 12, -1, -1, 60,  -1,    -1,    -1, { "Xeon(R) E5-####[ABLW] v3",  8 }, "Xeon E5 (Haswell)",         "22 nm" },
	{  6, 12, -1, -1, 60,  -1,    -1,    -1, { "Xeon(R) E5-#### v3",        6 }, "Xeon E5 (Haswell)",         "22 nm" },
	{  6, 12, -1, -1, 60,  -1,    -1,    -1, { "Xeon(R) E3-####L v3",       8 }, "Xeon E3 (Haswell)",         "22 nm" },
	{  6, 12, -1, -1, 60,  -1,    -1,    -1, { "Xeon(R) E3-#### v3",        6 }, "Xeon E3 (Haswell)",         "22 nm" },
	{  6, 12, -1, -1, 60,  -1,    -1,    -1, { "Core(TM) i7-4###",          8 }, "Core i7 (Haswell)",         "22 nm" },
	{  6, 12, -1, -1, 60,  -1,    -1,    -1, { "Core(TM) i5-4###",          8 }, "Core i5 (Haswell)",         "22 nm" },
	{  6, 12, -1, -1, 60,  -1,    -1,    -1, { "Core(TM) i3-4###",          8 }, "Core i3 (Haswell)",         "22 nm" },
	{  6, 12, -1, -1, 60,  -1,    -1,    -1, { "Pentium(R) G3###",          6 }, "Pentium (Haswell)",         "22 nm" },
	{  6, 12, -1, -1, 60,  -1,    -1,    -1, { "Celeron(R) G1###",          6 }, "Celeron (Haswell)",         "22 nm" },
	{  6, 15, -1, -1, 63,  -1,    -1,    -1, { "Core(TM) i7-5###[KX]",      8 }, "Core i7 Extreme (Haswell)", "22 nm" },
	{  6,  5, -1, -1, 69,  -1,    -1,    -1, { "Core(TM) i7-4###",          8 }, "Core i7 (Haswell)",         "22 nm" },
	{  6,  5, -1, -1, 69,  -1,    -1,    -1, { "Core(TM) i5-4###",          8 }, "Core i5 (Haswell)",         "22 nm" },
	{  6,  5, -1, -1, 69,  -1,    -1,    -1, { "Core(TM) i3-4###",          8 }, "Core i3 (Haswell)",         "22 nm" },
	{  6,  6, -1, -1, 70,  -1,    -1,    -1, { "Core(TM) i7-4###R",        10 }, "Core i7 (Haswell-H)",       "22 nm" }, /* GT3e */
	{  6,  6, -1, -1, 70,  -1,    -1,    -1, { "Core(TM) i5-4###R",        10 }, "Core i5 (Haswell-H)",       "22 nm" }, /* GT3e */
	{  6,  5, -1, -1, 69,  -1,    -1,    -1, { "Core(TM) i7-4###U",        10 }, "Core i7 (Haswell-ULT)",     "22 nm" },
	{  6,  5, -1, -1, 69,  -1,    -1,    -1, { "Core(TM) i5-4###U",        10 }, "Core i5 (Haswell-ULT)",     "22 nm" },
	{  6,  5, -1, -1, 69,  -1,    -1,    -1, { "Core(TM) i3-4###U",        10 }, "Core i3 (Haswell-ULT)",     "22 nm" },
	{  6,  5, -1, -1, 69,  -1,    -1,    -1, { "Core(TM) i7-4###Y",        10 }, "Core i7 (Haswell-ULX)",     "22 nm" },
	{  6,  5, -1, -1, 69,  -1,    -1,    -1, { "Core(TM) i5-4###Y",        10 }, "Core i5 (Haswell-ULX)",     "22 nm" },
	{  6,  5, -1, -1, 69,  -1,    -1,    -1, { "Core(TM) i3-4###Y",        10 }, "Core i3 (Haswell-ULX)",     "22 nm" },

	/* Broadwell CPUs (2014, 5th Core i gen, 14 nm): https://en.wikipedia.org/wiki/Broadwell_(microarchitecture) */
	{  6,  6, -1, -1, 86,  -1,    -1,    -1, { "Xeon(R) D-15##",              6 }, "Xeon D (Broadwell)",    "14 nm" },
	{  6,  6, -1, -1, 86,  -1,    -1,    -1, { "Pentium(R) D15##",            6 }, "Pentium D (Broadwell)", "14 nm" },
	{  6,  7, -1, -1, 71,   4,    -1,    -1, { "Core(TM) i7-5###[CR]",       10 }, "Core i7 (Broadwell-H)", "14 nm" },
	{  6,  7, -1, -1, 71,   4,    -1,    -1, { "Core(TM) i5-5###[CR]",       10 }, "Core i5 (Broadwell-H)", "14 nm" },
	{  6, 13, -1, -1, 61,   4,    -1,    -1, { "Core(TM) i7-5###HQ",         12 }, "Core i7 (Broadwell-U)", "14 nm" },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, { "Core(TM) i7-5###U",          10 }, "Core i7 (Broadwell-U)", "14 nm" },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, { "Core(TM) i5-5###[HU]",       10 }, "Core i5 (Broadwell-U)", "14 nm" },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, { "Core(TM) i3-5###U",          10 }, "Core i3 (Broadwell-U)", "14 nm" },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, { "Pentium(R) 3###U",            6 }, "Pentium (Broadwell-U)", "14 nm" },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, { "Celeron(R) 3###U",            6 }, "Celeron (Broadwell-U)", "14 nm" },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, { "5Y##",                        4 }, "Core M (Broadwell-Y)",  "14 nm" },
	{  6, 15, -1, -1, 79,  -1,    -1,    -1, { "Xeon(R) E7-#### v4",          6 }, "Xeon E7 (Broadwell)",   "14 nm" },
	{  6, 15, -1, -1, 79,  -1,    -1,    -1, { "Xeon(R) E5-####[ACLPRW] v4",  8 }, "Xeon E5 (Broadwell)",   "14 nm" },
	{  6, 15, -1, -1, 79,  -1,    -1,    -1, { "Xeon(R) E5-#### v4",          6 }, "Xeon E5 (Broadwell)",   "14 nm" },
	{  6, 15, -1, -1, 79,  -1,    -1,    -1, { "Xeon(R) E3-####L v4",         8 }, "Xeon E3 (Broadwell)",   "14 nm" },
	{  6, 15, -1, -1, 79,  -1,    -1,    -1, { "Xeon(R) E3-#### v4",          6 }, "Xeon E3 (Broadwell)",   "14 nm" },
	{  6, 15, -1, -1, 79,   4,    -1,    -1, { "Core(TM) i7-6###[KX]",       10 }, "Core i7 (Broadwell-E)", "14 nm" },

	/* Airmont CPUs (2014, Atom, 14 nm): https://en.wikipedia.org/wiki/Silvermont#List_of_Airmont_processors */
	{  6, 12, -1, -1,  76, -1,    -1,    -1, { "Pentium(R) [JN]3###",  6 }, "Pentium (Braswell)",     "14 nm" },
	{  6, 12, -1, -1,  76, -1,    -1,    -1, { "Celeron(R) [JN]3###",  6 }, "Celeron (Braswell)",     "14 nm" },
	{  6, 12, -1, -1,  76,  4,    -1,    -1, { "Atom(TM) x7-Z8###",    8 }, "Atom x7 (Cherry Trail)", "14 nm" },
	{  6, 12, -1, -1,  76,  4,    -1,    -1, { "Atom(TM) x5-Z8###",    8 }, "Atom x5 (Cherry Trail)", "14 nm" },
	{  6,  5, -1, -1, 117, -1,    -1,    -1, { "Spreadtrum",           2 }, "Spreadtrum (Airmont)",   "14 nm" }, /* Spreadtrum SC9853I-IA */

	/* Skylake (client) CPUs (2015, 6th Core i gen, 14 nm): https://en.wikichip.org/wiki/intel/microarchitectures/skylake_(client) */
	{  6, 14, -1, -1, 94,   4,    -1,    -1, { "Core(TM) i7-6###",         8 }, "Core i7 (Skylake)",   "14 nm" },
	{  6, 14, -1, -1, 94,   4,    -1,    -1, { "Core(TM) i5-6###",         8 }, "Core i5 (Skylake)",   "14 nm" },
	{  6, 14, -1, -1, 94,   2,    -1,    -1, { "Core(TM) i3-6###",         8 }, "Core i3 (Skylake)",   "14 nm" },
	{  6, 14, -1, -1, 94,   2,    -1,    -1, { "Pentium(R) G4###",         6 }, "Pentium (Skylake)",   "14 nm" },
	{  6, 14, -1, -1, 94,   2,    -1,    -1, { "Celeron(R) G3###",         6 }, "Celeron (Skylake)",   "14 nm" },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, { "Core(TM) m7-6Y##",         8 }, "Core m7 (Skylake)",   "14 nm" },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, { "Core(TM) m5-6Y##",         8 }, "Core m5 (Skylake)",   "14 nm" },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, { "Core(TM) m3-6Y##",         8 }, "Core m3 (Skylake)",   "14 nm" },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, { "Pentium(R) 4###[UY]",      6 }, "Pentium (Skylake)",   "14 nm" },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, { "Celeron(R) 3###U",         6 }, "Celeron (Skylake)",   "14 nm" },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, { "Celeron(R) G3###E",        8 }, "Celeron (Skylake)",   "14 nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Core(TM) i9-7###X",       10 }, "Core i9 (Skylake-X)", "14 nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Core(TM) i7-7###X",       10 }, "Core i7 (Skylake-X)", "14 nm" }, /* Core i7 7800X + 7820X */
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Core(TM) i9-9###X",       10 }, "Core i9 (Skylake-X)", "14 nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Core(TM) i7-9###X",       10 }, "Core i7 (Skylake-X)", "14 nm" }, /* Core i7 9800X */
	{  6, 14, -1, -1, 94,  -1,    -1,    -1, { "Xeon(R) W-#1##X",          8 }, "Xeon (Skylake-X)",    "14 nm" },
	/* Skylake (server) CPUs (2017, 1st Xeon Scalable gen, 14 nm): https://en.wikichip.org/wiki/intel/microarchitectures/skylake_(server) */
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Xeon(R) D-#1##",          6 }, "Xeon D (Skylake-D)",         "14 nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Xeon(R) E3-####[ML] v5",  8 }, "Xeon E3 (Skylake-S)",        "14 nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Xeon(R) E3-#### v5",      6 }, "Xeon E3 (Skylake-S)",        "14 nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Xeon(R) W-#1##",          6 }, "Xeon W (Skylake-W)",         "14 nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Xeon(R) Platinum #1##",   6 }, "Xeon Platinum (Skylake-SP)", "14 nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Xeon(R) Gold #1##",       6 }, "Xeon Gold (Skylake-SP)",     "14 nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Xeon(R) Silver #1##",     6 }, "Xeon Silver (Skylake-SP)",   "14 nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Xeon(R) Bronze #1##",     6 }, "Xeon Bronze (Skylake-SP)",   "14 nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Montage(R) Jintide(R)",   4 }, "Jintide (Skylake-SP)",       "14 nm" }, /* Montage(R) Jintide(R) C2460 */
	/* Kaby Lake CPUs (2016, 7th Core i gen, 14+ nm): https://en.wikipedia.org/wiki/Kaby_Lake */
	{  6, 14, -1, -1, 158,  4,    -1,    -1, { "Core(TM) i7-7###",          8 }, "Core i7 (Kaby Lake)",         "14+ nm" },
	{  6, 14, -1, -1, 158,  4,    -1,    -1, { "Core(TM) i5-7###",          8 }, "Core i5 (Kaby Lake)",         "14+ nm" },
	{  6, 14, -1, -1, 158,  2,    -1,    -1, { "Core(TM) i3-7###",          8 }, "Core i3 (Kaby Lake)",         "14+ nm" },
	{  6, 14, -1, -1, 158,  2,    -1,    -1, { "Pentium(R) G4###",          6 }, "Pentium (Kaby Lake)",         "14+ nm" },
	{  6, 14, -1, -1, 158,  2,    -1,    -1, { "Celeron(R) G3###",          6 }, "Celeron (Kaby Lake)",         "14+ nm" },
	{  6, 14, -1, -1, 158,  4,    -1,    -1, { "Core(TM) i7-7###X",        10 }, "Core i7 (Kaby Lake-X)",       "14+ nm" },
	{  6, 14, -1, -1, 158,  4,    -1,    -1, { "Core(TM) i5-7###X",        10 }, "Core i5 (Kaby Lake-X)",       "14+ nm" },
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Core(TM) i7-7.##",          8 }, "Core i7 (Kaby Lake-U)",       "14+ nm" },
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Core(TM) i5-7.##",          8 }, "Core i5 (Kaby Lake-U)",       "14+ nm" },
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Core(TM) i3-7.##",          8 }, "Core i3 (Kaby Lake-U)",       "14+ nm" },
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Core(TM) m3-7.##",          8 }, "Core m3 (Kaby Lake-U)",       "14+ nm" },
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Pentium(R) 441#[UY]",       8 }, "Pentium Gold (Kaby Lake-U)",  "14+ nm" },
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Celeron(R) 3###[UY]",       6 }, "Celeron (Kaby Lake-U)",       "14+ nm" },
	{  6, 14, -1, -1, 158,  4,    -1,    -1, { "Core(TM) i7-8###G",        10 }, "Core i7 (Kaby Lake-G)",       "14+ nm" },
	{  6, 14, -1, -1, 158,  4,    -1,    -1, { "Core(TM) i5-8###G",        10 }, "Core i5 (Kaby Lake-G)",       "14+ nm" },
	{  6, 14, -1, -1, 142,  4,    -1,    -1, { "Core(TM) i7-8##0U",        10 }, "Core i7 (Kaby Lake-R)",       "14+ nm" }, /* i7-8550U + i7-8650U */
	{  6, 14, -1, -1, 142,  4,    -1,    -1, { "Core(TM) i5-8##0U",        10 }, "Core i5 (Kaby Lake-R)",       "14+ nm" }, /* i5-8250U + i5-8350U */
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Core(TM) i3-8##0U",        10 }, "Core i3 (Kaby Lake-R)",       "14+ nm" }, /* i3-8130U */
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Pentium(R) 4###U",          6 }, "Pentium Gold (Kaby Lake-R)",  "14+ nm" }, /* Pentium 4417U */
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Celeron(R) 3###U",          6 }, "Celeron (Kaby Lake-R)",       "14+ nm" }, /* Celeron 3867U */
	/* Coffee Lake CPUs (2017, 8th Core i gen, 14++ nm): https://en.wikipedia.org/wiki/Coffee_Lake */
	{  6, 14, -1, -1, 158,  6,    -1,    -1, { "Core(TM) i7-8###",       8 }, "Core i7 (Coffee Lake-S)",       "14++ nm" },
	{  6, 14, -1, -1, 158,  6,    -1,    -1, { "Core(TM) i5-8###",       8 }, "Core i5 (Coffee Lake-S)",       "14++ nm" },
	{  6, 14, -1, -1, 158,  4,    -1,    -1, { "Core(TM) i3-8###",       8 }, "Core i3 (Coffee Lake-S)",       "14++ nm" },
	{  6, 14, -1, -1, 158,  2,    -1,    -1, { "Pentium(R) G5###",       6 }, "Pentium Gold (Coffee Lake-S)",  "14++ nm" },
	{  6, 14, -1, -1, 158,  2,    -1,    -1, { "Celeron(R) G4###",       6 }, "Celeron (Coffee Lake-S)",       "14++ nm" },
	{  6, 14, -1, -1, 158,  6,    -1,    -1, { "Xeon(R) E-21##M",        8 }, "Xeon E (Coffee Lake-H)",        "14++ nm" },
	{  6, 14, -1, -1, 158,  6,    -1,    -1, { "Core(TM) i9-8###[HB]",  10 }, "Core i9 (Coffee Lake-H)",       "14++ nm" },
	{  6, 14, -1, -1, 158,  6,    -1,    -1, { "Core(TM) i7-8###[HB]",  10 }, "Core i7 (Coffee Lake-H)",       "14++ nm" },
	{  6, 14, -1, -1, 158,  4,    -1,    -1, { "Core(TM) i5-8###[HB]",  10 }, "Core i5 (Coffee Lake-H)",       "14++ nm" },
	{  6, 14, -1, -1, 158,  4,    -1,    -1, { "Core(TM) i5-8###[HB]",  10 }, "Core i5 (Coffee Lake-H)",       "14++ nm" },
	{  6, 14, -1, -1, 158,  4,    -1,    -1, { "Core(TM) i3-8###[HB]",  10 }, "Core i3 (Coffee Lake-H)",       "14++ nm" },
	{  6, 14, -1, -1, 158,  4,    -1,    -1, { "Core(TM) i7-8###U",     10 }, "Core i7 (Coffee Lake-U)",       "14++ nm" },
	{  6, 14, -1, -1, 158,  4,    -1,    -1, { "Core(TM) i5-8###U",     10 }, "Core i5 (Coffee Lake-U)",       "14++ nm" },
	{  6, 14, -1, -1, 158,  2,    -1,    -1, { "Core(TM) i3-8###U",     10 }, "Core i3 (Coffee Lake-U)",       "14++ nm" },
	/* Coffee Lake Refresh CPUs (2018, 9th Core i gen, 14++ nm): https://en.wikipedia.org/wiki/Coffee_Lake#List_of_9th_generation_Coffee_Lake_processors_(Coffee_Lake_Refresh) */
	{  6, 14, -1, -1, 158,  8,    -1,    -1, { "Xeon(R) E-2###",         6 }, "Xeon E (Coffee Lake-S WS)",       "14++ nm" },
	{  6, 14, -1, -1, 158,  8,    -1,    -1, { "CC###",                  4 }, "CC (Coffee Lake)",                "14++ nm" }, /* CC150 */
	{  6, 14, -1, -1, 158,  8,    -1,    -1, { "Core(TM) i9-9###",       8 }, "Core i9 (Coffee Lake-S)",         "14++ nm" },
	{  6, 14, -1, -1, 158,  8,    -1,    -1, { "Core(TM) i7-9###",       8 }, "Core i7 (Coffee Lake-S)",         "14++ nm" },
	{  6, 14, -1, -1, 158,  6,    -1,    -1, { "Core(TM) i5-9###",       8 }, "Core i5 (Coffee Lake-S)",         "14++ nm" },
	{  6, 14, -1, -1, 158,  4,    -1,    -1, { "Core(TM) i3-9###",       8 }, "Core i3 (Coffee Lake-S)",         "14++ nm" },
	{  6, 14, -1, -1, 158,  2,    -1,    -1, { "Pentium(R) Gold G5###",  8 }, "Pentium Gold (Coffee Lake-S)",    "14++ nm" },
	{  6, 14, -1, -1, 158,  2,    -1,    -1, { "Celeron(R) G4###",       6 }, "Celeron (Coffee Lake-S)",         "14++ nm" },
	{  6, 14, -1, -1, 158, -1,    -1,    -1, { "Xeon(R) E-22##M",        8 }, "Xeon E (Coffee Lake-H Refresh)",  "14++ nm" },
	{  6, 14, -1, -1, 158,  8,    -1,    -1, { "Core(TM) i9-9###H",     10 }, "Core i9 (Coffee Lake-H Refresh)", "14++ nm" },
	{  6, 14, -1, -1, 158,  6,    -1,    -1, { "Core(TM) i7-9###H",     10 }, "Core i7 (Coffee Lake-H Refresh)", "14++ nm" },
	{  6, 14, -1, -1, 158,  4,    -1,    -1, { "Core(TM) i5-9###H",     10 }, "Core i5 (Coffee Lake-H Refresh)", "14++ nm" },
	/* Whiskey Lake CPUs (2018, 8th Core i gen, 14++ nm): https://en.wikipedia.org/wiki/Whiskey_Lake */
	{  6, 14, -1, -1, 142,  4,    -1,    -1, { "Core(TM) i7-8##5U",     10 }, "Core i7 (Whiskey Lake-U)",      "14++ nm" },
	{  6, 14, -1, -1, 142,  4,    -1,    -1, { "Core(TM) i5-8##5U",     10 }, "Core i5 (Whiskey Lake-U)",      "14++ nm" },
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Core(TM) i3-8##5U",     10 }, "Core i3 (Whiskey Lake-U)",      "14++ nm" },
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Pentium(R) 5###U",       6 }, "Pentium Gold (Whiskey Lake-U)", "14++ nm" },
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Celeron(R) 4###U",       6 }, "Celeron (Whiskey Lake-U)",      "14++ nm" },
	/* Amber Lake CPUs (2018, 8th Core i gen, 14++ nm): https://en.wikipedia.org/wiki/Kaby_Lake#Amber_Lake */
	{  6, 14, -1, -1, 142,  4,    -1,    -1, { "Core(TM) i7-8###Y",        10 }, "Core i7 (Amber Lake-Y)",      "14+ nm" }, /* i7-8500Y */
	{  6, 14, -1, -1, 142,  4,    -1,    -1, { "Core(TM) i5-8###Y",        10 }, "Core i5 (Amber Lake-Y)",      "14+ nm" }, /* i5-8200Y + i5-82010Y + i5-8310Y */
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Core(TM) m3-8###Y",        10 }, "Core m3 (Amber Lake-Y)",      "14+ nm" }, /* m3-8100Y */
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Pentium(R) 442#Y",          8 }, "Pentium Gold (Amber Lake-Y)", "14+ nm" }, /* Pentium 4425Y */
	{  6, 14, -1, -1, 142,  4,    -1,    -1, { "Core(TM) i7-10###Y",       10 }, "Core i7 (Amber Lake-Y)",      "14+ nm" }, /* i7-10510Y */
	{  6, 14, -1, -1, 142,  4,    -1,    -1, { "Core(TM) i5-10###Y",       10 }, "Core i5 (Amber Lake-Y)",      "14+ nm" }, /* i5-10210Y + i5-10310Y + i5-8310Y */
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Core(TM) i3-10###Y",       10 }, "Core i3 (Amber Lake-Y)",      "14+ nm" }, /* i3-10100Y + i3-10110Y */
	{  6, 14, -1, -1, 142,  2,    -1,    -1, { "Pentium(R) 65##Y",          6 }, "Pentium Gold (Amber Lake-Y)", "14+ nm" }, /* Pentium 6500Y */
	/* Cascade Lake CPUs (2019, 2nd Xeon Scalable gen, 14++ nm): https://en.wikichip.org/wiki/intel/microarchitectures/cascade_lake */
	{  6,  5,  7, -1, 85,  -1,    -1,    -1, { "Core(TM) i9-10###X",    10 }, "Core i9 (Cascade Lake-X)",        "14++ nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Xeon(R) W-#[23]##",      6 }, "Xeon W (Cascade Lake-W)",         "14++ nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Xeon(R) Platinum #2##",  6 }, "Xeon Platinum (Cascade Lake-SP)", "14++ nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Xeon(R) Gold #2##",      6 }, "Xeon Gold (Cascade Lake-SP)",     "14++ nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Xeon(R) Silver #2##",    6 }, "Xeon Silver (Cascade Lake-SP)",   "14++ nm" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, { "Xeon(R) Bronze #2##",    6 }, "Xeon Bronze (Cascade Lake-SP)",   "14++ nm" },
	/* Comet Lake CPUs (2019, 10th Core i gen, 14++ nm): https://en.wikipedia.org/wiki/Comet_Lake */
	{  6,  5, -1, -1, 165, -1,    -1,    -1, { "Xeon(R) W-12##",         6 }, "Xeon W (Comet Lake-S)",       "14++ nm" },
	{  6,  5, -1, -1, 165, 10,    -1,    -1, { "Core(TM) i9-10###",      8 }, "Core i9 (Comet Lake-S)",      "14++ nm" },
	{  6,  5, -1, -1, 165,  8,    -1,    -1, { "Core(TM) i7-10###",      8 }, "Core i7 (Comet Lake-S)",      "14++ nm" },
	{  6,  5, -1, -1, 165,  6,    -1,    -1, { "Core(TM) i5-10###",      8 }, "Core i5 (Comet Lake-S)",      "14++ nm" },
	{  6,  5, -1, -1, 165,  4,    -1,    -1, { "Core(TM) i3-10###",      8 }, "Core i3 (Comet Lake-S)",      "14++ nm" },
	{  6,  5, -1, -1, 165,  2,    -1,    -1, { "Pentium(R) Gold G6###",  8 }, "Pentium Gold (Comet Lake-S)", "14++ nm" },
	{  6,  5, -1, -1, 165,  2,    -1,    -1, { "Celeron(R) G5###",       6 }, "Celeron (Comet Lake-S)",      "14++ nm" },
	{  6,  6, -1, -1, 166,  6,    -1,    -1, { "Core(TM) i7-10###U",    10 }, "Core i7 (Comet Lake-U)",      "14++ nm" },
	{  6, 14, 12, -1, 142,  6,    -1,    -1, { "Core(TM) i7-10###U",    10 }, "Core i7 (Comet Lake-U)",      "14++ nm" },
	{  6, 14, 12, -1, 142,  4,    -1,    -1, { "Core(TM) i7-10###U",    10 }, "Core i7 (Comet Lake-U)",      "14++ nm" },
	{  6, 14, 12, -1, 142,  4,    -1,    -1, { "Core(TM) i5-10###U",    10 }, "Core i5 (Comet Lake-U)",      "14++ nm" },
	{  6, 14, 12, -1, 142,  4,    -1,    -1, { "Core(TM) i3-10###U",    10 }, "Core i3 (Comet Lake-U)",      "14++ nm" },
	{  6, 14, 12, -1, 142,  2,    -1,    -1, { "Pentium(R) Gold 6###U",  8 }, "Pentium Gold (Comet Lake-U)", "14++ nm" },
	{  6, 14, 12, -1, 142,  2,    -1,    -1, { "Celeron(R) 5###U",       6 }, "Celeron (Comet Lake-U)",      "14++ nm" },
	{  6,  5, -1, -1, 165, -1,    -1,    -1, { "Xeon(R) W-10###M",      10 }, "Xeon W (Comet Lake-H)",       "14++ nm" },
	{  6,  5, -1, -1, 165, -1,    -1,    -1, { "Core(TM) i9-10###H",    10 }, "Core i9 (Comet Lake-H)",      "14++ nm" },
	{  6,  5, -1, -1, 165, -1,    -1,    -1, { "Core(TM) i7-10###H",    10 }, "Core i7 (Comet Lake-H)",      "14++ nm" },
	{  6,  5, -1, -1, 165, -1,    -1,    -1, { "Core(TM) i5-10###H",    10 }, "Core i5 (Comet Lake-H)",      "14++ nm" },

	/* Goldmont CPUs (2016, Atom, 14 nm): https://en.wikipedia.org/wiki/Goldmont */
	{  6, 12, -1, -1,  92, -1,    -1,    -1, { "Pentium(R) J4###",  6 }, "Pentium (Apollo Lake)", "14 nm" },
	{  6, 12, -1, -1,  92, -1,    -1,    -1, { "Celeron(R) J3###",  6 }, "Celeron (Apollo Lake)", "14 nm" },
	{  6, 12, -1, -1,  92, -1,    -1,    -1, { "Pentium(R) N4###",  6 }, "Pentium (Apollo Lake)", "14 nm" },
	{  6, 12, -1, -1,  92, -1,    -1,    -1, { "Celeron(R) N3###",  6 }, "Celeron (Apollo Lake)", "14 nm" },
	{  6, 12, -1, -1,  92, -1,    -1,    -1, { "Atom(TM) E39##",    6 }, "Atom (Apollo Lake)",    "14 nm" },
	{  6, 15, -1, -1,  95, -1,    -1,    -1, { "Atom(TM) C39##",    6 }, "Atom (Denverton)" ,     "14 nm" },

	/* Goldmont Plus CPUs (2017, Atom, 14 nm): https://en.wikipedia.org/wiki/Goldmont_Plus */
	{  6, 10, -1, -1, 122, -1,    -1,    -1, { "Pentium(R) Silver [JN]5###",  8 }, "Pentium Silver (Gemini Lake)", "14 nm" },
	{  6, 10, -1, -1, 122, -1,    -1,    -1, { "Celeron(R) [JN]4###",         6 }, "Celeron (Gemini Lake)",        "14 nm" },

	/* Palm Cove CPUs (2018, 8th Core i gen, 14++ nm): https://en.wikipedia.org/wiki/Cannon_Lake_(microprocessor)*/
	{  6,  6, -1, -1, 102,  2,    -1,    -1, { "Core(TM) i3-8###U",     10 }, "Core i3 (Cannon Lake-U)",       "14++ nm" }, /* Core i3 8121U */
	{  6,  6, -1, -1, 102,  2,    -1,    -1, { "Core(TM) m3-8###Y",     10 }, "Core m3 (Cannon Lake-Y)",       "14++ nm" }, /* Core m3 8114Y */

	/* Sunny Cove CPUs (2019, 10th Core i gen, 10 nm): https://en.wikipedia.org/wiki/Sunny_Cove_(microarchitecture) */
	{  6, 14, -1, -1, 126,  4,    -1,    -1, { "Core(TM) i7-10##NG7",    10 }, "Core i7 (Ice Lake)", "10 nm" },
	{  6, 14, -1, -1, 126,  4,    -1,    -1, { "Core(TM) i7-10##G7",     10 }, "Core i7 (Ice Lake)", "10 nm" },
	{  6, 14, -1, -1, 126,  4,    -1,    -1, { "Core(TM) i5-10##NG7",    10 }, "Core i5 (Ice Lake)", "10 nm" },
	{  6, 14, -1, -1, 126,  4,    -1,    -1, { "Core(TM) i5-10##G[741]", 10 }, "Core i5 (Ice Lake)", "10 nm" },
	{  6, 14, -1, -1, 126,  2,    -1,    -1, { "Core(TM) i3-10##G[14]",  10 }, "Core i3 (Ice Lake)", "10 nm" },
	{  6, 14, -1, -1, 126,  2,    -1,    -1, { "Core(TM) i3-10##NG4",    10 }, "Core i3 (Ice Lake)", "10 nm" },
	{  6, 14, -1, -1, 126,  2,    -1,    -1, { "Pentium(R) 68##",         4 }, "Pentium (Ice Lake)", "10 nm" }, /* Pentium 6805 */
	/* Ice Lake (server) CPUs (2021, 3rd Xeon Scalable gen, 10 nm): https://en.wikichip.org/wiki/intel/microarchitectures/ice_lake_(server) */
	{  6, 12, -1, -1, 108,  -1,   -1,    -1, { "Xeon(R) D-[12]7##",      6 }, "Xeon D (Ice Lake-D)",         "10 nm" },
	{  6, 10, -1, -1, 106,  -1,   -1,    -1, { "Xeon(R) W-#3##",         6 }, "Xeon W (Ice Lake-W)",         "10 nm" },
	{  6, 10, -1, -1, 106,  -1,   -1,    -1, { "Xeon(R) Platinum #3##",  6 }, "Xeon Platinum (Ice Lake-SP)", "10 nm" },
	{  6, 10, -1, -1, 106,  -1,   -1,    -1, { "Xeon(R) Gold #3##",      6 }, "Xeon Gold (Ice Lake-SP)",     "10 nm" },
	{  6, 10, -1, -1, 106,  -1,   -1,    -1, { "Xeon(R) Silver #3##",    6 }, "Xeon Silver (Ice Lake-SP)",   "10 nm" },
	{  6, 10, -1, -1, 106,  -1,   -1,    -1, { "Xeon(R) Bronze #3##",    6 }, "Xeon Bronze (Ice Lake-SP)",   "10 nm" },

	/* Tremont CPUs (2020, Atom, 10 nm): https://en.wikipedia.org/wiki/Tremont_(microarchitecture) */
	{  6,  6, -1, -1, 150, -1,    -1,    -1, { "Pentium(R) [JN]6###",      6 }, "Pentium (Elkhart Lake)",       "10 nm" },
	{  6,  6, -1, -1, 150, -1,    -1,    -1, { "Celeron(R) [JN]6###",      6 }, "Celeron (Elkhart Lake)",       "10 nm" },
	{  6,  6, -1, -1, 150, -1,    -1,    -1, { "Atom(TM) x6###",           6 }, "Atom (Elkhart Lake)",          "10 nm" },
	{  6, 10, -1, -1, 138, -1,    -1,    -1, { "Core(TM) i5-L##G7",       12 }, "Core i5 (Lakefield)",          "10 nm" },
	{  6, 10, -1, -1, 138, -1,    -1,    -1, { "Core(TM) i3-L##G4",       12 }, "Core i3 (Lakefield)",          "10 nm" },
	{  6, 12, -1, -1, 156, -1,    -1,    -1, { "Pentium(R) Silver N6###",  8 }, "Pentium Silver (Jasper Lake)", "10 nm" },
	{  6, 12, -1, -1, 156, -1,    -1,    -1, { "Celeron(R) N[45]###",      6 }, "Celeron (Jasper Lake)",        "10 nm" },

	/* Willow Cove CPUs (2020, 11th Core i gen, 10 nm SuperFin): https://en.wikipedia.org/wiki/Willow_Cove */
	{  6, 12, -1, -1, 140, -1,    -1,    -1, { "Core(TM) i7-11#5G7",   12 }, "Core i7 (Tiger Lake-UP3)",      "10SF" },
	{  6, 12, -1, -1, 140, -1,    -1,    -1, { "Core(TM) i5-11#5G7",   12 }, "Core i5 (Tiger Lake-UP3)",      "10SF" },
	{  6, 12, -1, -1, 140, -1,    -1,    -1, { "Core(TM) i3-11#5G4",   12 }, "Core i3 (Tiger Lake-UP3)",      "10SF" },
	{  6, 12, -1, -1, 140,  2,    -1,    -1, { "Pentium(R) Gold 7##5",  6 }, "Pentium Gold (Tiger Lake-UP3)", "10SF" },
	{  6, 12, -1, -1, 140,  2,    -1,    -1, { "Celeron(R) 6##5",       4 }, "Celeron (Tiger Lake-UP3)",      "10SF" },
	{  6, 12, -1, -1, 140, -1,    -1,    -1, { "Core(TM) i7-11#0G7",   12 }, "Core i7 (Tiger Lake-UP4)",      "10SF" },
	{  6, 12, -1, -1, 140, -1,    -1,    -1, { "Core(TM) i5-11#0G7",   12 }, "Core i5 (Tiger Lake-UP4)",      "10SF" },
	{  6, 12, -1, -1, 140, -1,    -1,    -1, { "Core(TM) i3-11#0G4",   12 }, "Core i3 (Tiger Lake-UP4)",      "10SF" },
	{  6, 12, -1, -1, 140, -1,    -1,    -1, { "Core(TM) i7-11###H",   10 }, "Core i7 (Tiger Lake-H35)",      "10SF" },
	{  6, 12, -1, -1, 140, -1,    -1,    -1, { "Core(TM) i5-11###H",   10 }, "Core i5 (Tiger Lake-H35)",      "10SF" },
	{  6, 13, -1, -1, 141, -1,    -1,    -1, { "Xeon(R) W-11###M",     10 }, "Xeon W (Tiger Lake-H)",         "10SF" },
	{  6, 13, -1, -1, 141, -1,    -1,    -1, { "Core(TM) i9-11###H",   10 }, "Core i9 (Tiger Lake-H)",        "10SF" },
	{  6, 13, -1, -1, 141, -1,    -1,    -1, { "Core(TM) i7-11###H",   10 }, "Core i7 (Tiger Lake-H)",        "10SF" },
	{  6, 13, -1, -1, 141, -1,    -1,    -1, { "Core(TM) i5-11###H",   10 }, "Core i5 (Tiger Lake-H)",        "10SF" },
	{  6, 13, -1, -1, 141, -1,    -1,    -1, { "Core(TM) i9-11###KB",  12 }, "Core i9 (Tiger Lake-B)",        "10SF" }, /* i9-11900KB */
	{  6, 13, -1, -1, 141, -1,    -1,    -1, { "Core(TM) i7-11###B",   10 }, "Core i7 (Tiger Lake-B)",        "10SF" }, /* i7-11700B */
	{  6, 13, -1, -1, 141, -1,    -1,    -1, { "Core(TM) i5-11###B",   10 }, "Core i5 (Tiger Lake-B)",        "10SF" }, /* i5-11500B */
	{  6, 13, -1, -1, 141, -1,    -1,    -1, { "Core(TM) i5-11###B",   10 }, "Core i5 (Tiger Lake-B)",        "10SF" }, /* i3-11100B */

	/* Cypress Cove CPUs (2021, 11th Core i gen, 14++ nm): https://en.wikipedia.org/wiki/Sunny_Cove_(microarchitecture)#Cypress_Cove */
	{  6, 7, -1, -1, 167,  -1,    -1,    -1, { "Core(TM) i9-11###",  8 }, "Core i9 (Rocket Lake-S)", "14++ nm" },
	{  6, 7, -1, -1, 167,  -1,    -1,    -1, { "Core(TM) i7-11###",  8 }, "Core i7 (Rocket Lake-S)", "14++ nm" },
	{  6, 7, -1, -1, 167,  -1,    -1,    -1, { "Core(TM) i5-11###",  8 }, "Core i5 (Rocket Lake-S)", "14++ nm" },
	{  6, 7, -1, -1, 167,  -1,    -1,    -1, { "Xeon(R) E-23##",     6 }, "Xeon E (Rocket Lake)"   , "14++ nm" },

	/* Golden Cove (P-cores) / Gracemont (E-cores) CPUs (2021, 12th Core i gen, Intel 7): https://en.wikipedia.org/wiki/Golden_Cove */
	{  6,  7, -1, -1, 151, -1,    -1,    -1, { "Core(TM) i9-12###",      8 }, "Core i9 (Alder Lake-S)",         "Intel 7" },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, { "Core(TM) i7-12###",      8 }, "Core i7 (Alder Lake-S)",         "Intel 7" },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, { "Core(TM) i5-12###",      8 }, "Core i5 (Alder Lake-S)",         "Intel 7" },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, { "Core(TM) i3-12###",      8 }, "Core i3 (Alder Lake-S)",         "Intel 7" },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, { "Pentium(R) Gold G7###",  8 }, "Pentium Gold (Alder Lake-S)"   , "Intel 7" },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, { "Celeron(R) G6###",       6 }, "Celeron (Alder Lake-S)",         "Intel 7" },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, { "Core(TM) i9-12###HX",   12 }, "Core i9 (Alder Lake-HX)",        "Intel 7" },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, { "Core(TM) i7-12###HX",   12 }, "Core i7 (Alder Lake-HX)",        "Intel 7" },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, { "Core(TM) i5-12###HX",   12 }, "Core i5 (Alder Lake-HX)",        "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Core(TM) i7-12##P",     10 }, "Core i7 (Alder Lake-P)",         "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Core(TM) i5-12##P",     10 }, "Core i5 (Alder Lake-P)",         "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Core(TM) i3-12##P",     10 }, "Core i3 (Alder Lake-P)",         "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Core(TM) i7-12##U",     10 }, "Core i7 (Alder Lake-U)",         "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Core(TM) i5-12##U",     10 }, "Core i5 (Alder Lake-U)",         "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Core(TM) i3-12##U",     10 }, "Core i3 (Alder Lake-U)",         "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Pentium(R) Gold 8###",   6 }, "Pentium Gold (Alder Lake-U)",    "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Celeron(R) 7###",        4 }, "Celeron (Alder Lake-U)",         "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Core(TM) i9-12###H",    10 }, "Core i9 (Alder Lake-H)",         "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Core(TM) i7-12###H",    10 }, "Core i7 (Alder Lake-H)",         "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Core(TM) i5-12###H",    10 }, "Core i5 (Alder Lake-H)",         "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Core(TM) i7-12##UL",    12 }, "Core i7 (Alder Lake-PS)",        "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Core(TM) i5-12##UL",    12 }, "Core i5 (Alder Lake-PS)",        "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Core(TM) i3-12##UL",    12 }, "Core i3 (Alder Lake-PS)",        "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Celeron(R) 7###L",       6 }, "Celeron (Alder Lake-PS)",        "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Core(TM) i7-12###HL",   12 }, "Core i7 (Alder Lake-PS)",        "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Core(TM) i5-12###HL",   12 }, "Core i5 (Alder Lake-PS)",        "Intel 7" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, { "Core(TM) i3-12###HL",   12 }, "Core i3 (Alder Lake-PS)",        "Intel 7" },
	/* Sapphire Rapids CPUs (2023, 4th Xeon Scalable gen, Intel 7): https://en.wikichip.org/wiki/intel/microarchitectures/sapphire_rapids */
	{  6, 15, -1, -1, 143, -1,    -1,    -1, { "Xeon(R) w9-#4##",        6 }, "Xeon w9 (Sapphire Rapids-WS)",       "Intel 7" },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, { "Xeon(R) w7-#4##",        6 }, "Xeon w7 (Sapphire Rapids-WS)",       "Intel 7" },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, { "Xeon(R) w5-#4##",        6 }, "Xeon w5 (Sapphire Rapids-WS)",       "Intel 7" },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, { "Xeon(R) w3-#4##",        6 }, "Xeon w3 (Sapphire Rapids-WS)",       "Intel 7" },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, { "Xeon(R) Max #4##",       6 }, "Xeon Max (Sapphire Rapids-HBM)",     "Intel 7" },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, { "Xeon(R) Platinum #4##",  6 }, "Xeon Platinum (Sapphire Rapids-SP)", "Intel 7" },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, { "Xeon(R) Gold #4##",      6 }, "Xeon Gold (Sapphire Rapids-SP)",     "Intel 7" },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, { "Xeon(R) Silver #4##",    6 }, "Xeon Silver (Sapphire Rapids-SP)"  , "Intel 7" },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, { "Xeon(R) Bronze #4##",    6 }, "Xeon Bronze (Sapphire Rapids-SP)"  , "Intel 7" },

	/* Gracemont CPUs (2021, Atom, Intel 7): https://en.wikipedia.org/wiki/Gracemont_(microarchitecture) */
	{  6, 14, -1, -1, 190, -1,    -1,    -1, { "Core(TM) i3-N3##",      10 }, "Core i3 (Alder Lake-N)",         "Intel 7" }, /* Core i3 N300 + Core i3 N305 */
	{  6, 14, -1, -1, 190,  4,    -1,    -1, { "N##",                    2 }, "Intel Processor (Alder Lake-N)", "Intel 7" },
	{  6, 14, -1, -1, 190,  2,    -1,    -1, { "N##",                    2 }, "Intel Processor (Alder Lake-N)", "Intel 7" }, /* Intel Processor N50 */
	{  6, 14, -1, -1, 190, -1,    -1,    -1, { "Atom(TM) x7###E",        8 }, "Atom (Alder Lake-N)",            "Intel 7" },
	/* Twin Lake CPUs (2025, Atom, Intel 7): https://en.wikichip.org/wiki/intel/microarchitectures/twin_lake */
	{  6, 14, -1, -1, 190,  8,    -1,    -1, { "Core(TM) 3 N#5#",        8 }, "Core 3 (Twin Lake-N)",            "Intel 7" }, /* Core 3 N350 + Core 3 N355 */
	{  6, 14, -1, -1, 190,  4,    -1,    -1, { "N#5#",                   4 }, "Intel Processor (Twin Lake-N)",   "Intel 7" }, /* Intel Processor N150 + Intel Processor N150 */

	/* Raptor Cove (P-cores) / Gracemont (E-cores) CPUs (2022, 13th Core i gen, Intel 7): https://en.wikipedia.org/wiki/Golden_Cove#Raptor_Cove */
	{  6, 15, -1, -1, 191, -1,    -1,    -1, { "Core(TM) i5-13###",      8 }, "Core i5 (Raptor Lake-S)",         "Intel 7" }, /* "Golden Cove" cores */
	{  6, 15, -1, -1, 191, -1,    -1,    -1, { "Core(TM) i3-13###",      8 }, "Core i3 (Raptor Lake-S)",         "Intel 7" }, /* "Golden Cove" cores */
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) i9-13###",      8 }, "Core i9 (Raptor Lake-S)",         "Intel 7" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) i7-13###",      8 }, "Core i7 (Raptor Lake-S)",         "Intel 7" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) i5-13###",      8 }, "Core i5 (Raptor Lake-S)",         "Intel 7" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) i3-13###",      8 }, "Core i3 (Raptor Lake-S)",         "Intel 7" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) i9-13###HX",   12 }, "Core i9 (Raptor Lake-HX)",        "Intel 7" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) i7-13###HX",   12 }, "Core i7 (Raptor Lake-HX)",        "Intel 7" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) i5-13###HX",   12 }, "Core i5 (Raptor Lake-HX)",        "Intel 7" },
	{  6, 10,  2, -1, 186, -1,    -1,    -1, { "Core(TM) i7-13###P",    10 }, "Core i7 (Raptor Lake-P)",         "Intel 7" },
	{  6, 10,  2, -1, 186, -1,    -1,    -1, { "Core(TM) i5-13###P",    10 }, "Core i5 (Raptor Lake-P)",         "Intel 7" },
	{  6, 10,  3, -1, 186, -1,    -1,    -1, { "Core(TM) i7-13###U",    10 }, "Core i7 (Raptor Lake-U)",         "Intel 7" },
	{  6, 10,  3, -1, 186, -1,    -1,    -1, { "Core(TM) i5-13###U",    10 }, "Core i5 (Raptor Lake-U)",         "Intel 7" },
	{  6, 10,  3, -1, 186, -1,    -1,    -1, { "Core(TM) i3-13###U",    10 }, "Core i3 (Raptor Lake-U)",         "Intel 7" },
	{  6, 10,  3, -1, 186, -1,    -1,    -1, { "U300",                   4 }, "Intel Processor (Raptor Lake-U)", "Intel 7" }, /* Intel Processor U300 */
	{  6, 10, -1, -1, 186, -1,    -1,    -1, { "Core(TM) i9-13###H",    10 }, "Core i9 (Raptor Lake-H)",         "Intel 7" },
	{  6, 10, -1, -1, 186, -1,    -1,    -1, { "Core(TM) i7-13###H",    10 }, "Core i7 (Raptor Lake-H)",         "Intel 7" },
	{  6, 10, -1, -1, 186, -1,    -1,    -1, { "Core(TM) i5-13###H",    10 }, "Core i5 (Raptor Lake-H)",         "Intel 7" },
	/* Emerald Rapids CPUs (2023, 5th Xeon Scalable gen, Intel 7): https://en.wikichip.org/wiki/intel/microarchitectures/emerald_rapids */
	{  6, 15, -1, -1, 207, -1,    -1,    -1, { "Xeon(R) Platinum #5##",  6 }, "Xeon Platinum (Emerald Rapids-SP)", "Intel 7" }, /* Xeon Platinum (8500) */
	{  6, 15, -1, -1, 207, -1,    -1,    -1, { "Xeon(R) Gold #5##",      6 }, "Xeon Gold (Emerald Rapids-SP)",     "Intel 7" }, /* Xeon Gold (5500 and 6500) */
	{  6, 15, -1, -1, 207, -1,    -1,    -1, { "Xeon(R) Silver #5##",    6 }, "Xeon Silver (Emerald Rapids-SP)",   "Intel 7" }, /* Xeon Silver (4500) */
	{  6, 15, -1, -1, 207, -1,    -1,    -1, { "Xeon(R) Bronze #5##",    6 }, "Xeon Bronze (Emerald Rapids-SP)",   "Intel 7" }, /* Xeon Bronze (3500) */
	/* Raptor Lake Refresh CPUs (2023, 14th Core i gen, Intel 7): https://en.wikipedia.org/wiki/Raptor_Lake#List_of_14th_generation_Raptor_Lake_processors */
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) i9-14###",      8 }, "Core i9 (Raptor Lake-S)" ,        "Intel 7" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) i7-14###",      8 }, "Core i7 (Raptor Lake-S)" ,        "Intel 7" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) i5-14###",      8 }, "Core i5 (Raptor Lake-S)" ,        "Intel 7" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) i3-14###",      8 }, "Core i3 (Raptor Lake-S)" ,        "Intel 7" },
	{  6,  7, -1, -1, 183,  2,    -1,    -1, { "300",                    2 }, "Intel Processor (Raptor Lake-S)", "Intel 7" }, /* Intel Processor 300 + Intel Processor 300T */
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) i9-14###HX",   12 }, "Core i9 (Raptor Lake-HX)",        "Intel 7" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) i7-14###HX",   12 }, "Core i7 (Raptor Lake-HX)",        "Intel 7" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) i5-14###HX",   12 }, "Core i5 (Raptor Lake-HX)",        "Intel 7" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Xeon(R) E-24##",         6 }, "Xeon E (Raptor Lake)",            "Intel 7" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) 7 1##U",        8 }, "Core 7 (Raptor Lake-U)",          "Intel 7" }, /* Core 7 150U */
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) 5 1##U",        8 }, "Core 5 (Raptor Lake-U)",          "Intel 7" }, /* Core 5 120U */
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) 3 1##U",        8 }, "Core 3 (Raptor Lake-U)",          "Intel 7" }, /* Core 3 100U */
	/* Raptor Lake Re-refresh CPUs (2025, Core Series 2, Intel 7): https://en.wikipedia.org/wiki/Raptor_Lake#List_of_Core_Series_2_processors */
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) 7 2##U",        8 }, "Core 7 (Raptor Lake-U)",          "Intel 7" }, /* Core 7 250U */
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) 5 2##U",        8 }, "Core 5 (Raptor Lake-U)",          "Intel 7" }, /* Core 5 220U */
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) 9 2##H",        8 }, "Core 9 (Raptor Lake-H)",          "Intel 7" }, /* Core 9 270H */
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) 7 2##H",        8 }, "Core 7 (Raptor Lake-H)",          "Intel 7" }, /* Core 7 240H + Core 7 250H */
	{  6,  7, -1, -1, 183, -1,    -1,    -1, { "Core(TM) 5 2##H",        8 }, "Core 5 (Raptor Lake-H)",          "Intel 7" }, /* Core 5 210H + Core 5 220H */

	/* Redwood Cove (P-cores) / Crestmont (E-cores) CPUs (2023, Core Ultra Series 1, Intel 4): https://en.wikipedia.org/wiki/Meteor_Lake */
	{  6, 10, -1, -1, 170, -1,    -1,    -1, { "Core(TM) Ultra 9 1##H", 10 }, "Core Ultra 9 (Meteor Lake-H)", "Intel 4" },
	{  6, 10, -1, -1, 170, -1,    -1,    -1, { "Core(TM) Ultra 7 1##H", 10 }, "Core Ultra 7 (Meteor Lake-H)", "Intel 4" },
	{  6, 10, -1, -1, 170, -1,    -1,    -1, { "Core(TM) Ultra 5 1##H", 10 }, "Core Ultra 5 (Meteor Lake-H)", "Intel 4" },
	{  6, 10, -1, -1, 170, -1,    -1,    -1, { "Core(TM) Ultra 7 1##U", 10 }, "Core Ultra 7 (Meteor Lake-U)", "Intel 4" },
	{  6, 10, -1, -1, 170, -1,    -1,    -1, { "Core(TM) Ultra 5 1##U", 10 }, "Core Ultra 5 (Meteor Lake-U)", "Intel 4" },
	/* Granite Rapids CPUs (2024, 6th Xeon Scalable gen, Intel 7): https://en.wikipedia.org/wiki/Granite_Rapids */
	{  6, 13, -1, -1, 173, -1,    -1,    -1, { "Xeon(R) 6[57]##P",  6 }, "Xeon 6 (Granite Rapids-SP)",  "Intel 3" },
	//{  6, ??, -1, -1, ???, -1,    -1,    -1, { "Xeon(R) 6[57]##P",  6 }, "Xeon 6 (Granite Rapids-AP)",  "Intel 3" },
	//{  6, 14, -1, -1, 174, -1,    -1,    -1, { "Xeon(R) ????",      4 }, "Xeon ??? (Granite Rapids-D)", "Intel 3" },

	/* Lion Cove (P-cores) / Skymont (E-cores) CPUs (2024, Core Ultra Series 2, TSMC N3B): https://en.wikipedia.org/wiki/Arrow_Lake_(microprocessor) */
	{  6,  6, -1, -1, 198, -1,    -1,    -1, { "Core(TM) Ultra 9 2##",    8 }, "Core Ultra 9 (Arrow Lake-S)",  "TSMC N3B" },
	{  6,  6, -1, -1, 198, -1,    -1,    -1, { "Core(TM) Ultra 7 2##",    8 }, "Core Ultra 7 (Arrow Lake-S)",  "TSMC N3B" },
	{  6,  6, -1, -1, 198, -1,    -1,    -1, { "Core(TM) Ultra 5 2##",    8 }, "Core Ultra 5 (Arrow Lake-S)",  "TSMC N3B" },
	{  6,  6, -1, -1, 181, -1,    -1,    -1, { "Core(TM) Ultra 7 2##U",  10 }, "Core Ultra 7 (Arrow Lake-U)",  "TSMC N3B" }, /* Core Ultra 7 255U + Core Ultra 7 265U */
	{  6,  6, -1, -1, 181, -1,    -1,    -1, { "Core(TM) Ultra 5 2##U",  10 }, "Core Ultra 5 (Arrow Lake-U)",  "TSMC N3B" }, /* Core Ultra 5 225U + Core Ultra 5 235U */
	{  6,  6, -1, -1, 197, -1,    -1,    -1, { "Core(TM) Ultra 9 2##H",  10 }, "Core Ultra 9 (Arrow Lake-H)",  "TSMC N3B" }, /* Core Ultra 9 285H */
	{  6,  6, -1, -1, 197, -1,    -1,    -1, { "Core(TM) Ultra 7 2##H",  10 }, "Core Ultra 7 (Arrow Lake-H)",  "TSMC N3B" }, /* Core Ultra 7 255H + Core Ultra 7 265H */
	{  6,  6, -1, -1, 197, -1,    -1,    -1, { "Core(TM) Ultra 5 2##H",  10 }, "Core Ultra 5 (Arrow Lake-H)",  "TSMC N3B" }, /* Core Ultra 5 225H + Core Ultra 5 235H */
	{  6,  6, -1, -1, 197, -1,    -1,    -1, { "Core(TM) Ultra 9 2##HX", 12 }, "Core Ultra 9 (Arrow Lake-HX)", "TSMC N3B" }, /* Core Ultra 9 275HX + Core Ultra 9 285HX */
	{  6,  6, -1, -1, 197, -1,    -1,    -1, { "Core(TM) Ultra 7 2##HX", 12 }, "Core Ultra 7 (Arrow Lake-HX)", "TSMC N3B" }, /* Core Ultra 7 255HX + Core Ultra 7 265HX */
	{  6,  6, -1, -1, 197, -1,    -1,    -1, { "Core(TM) Ultra 5 2##HX", 12 }, "Core Ultra 5 (Arrow Lake-HX)", "TSMC N3B" }, /* Core Ultra 5 235HX + Core Ultra 5 245HX */
	{  6, 13, -1, -1, 189, -1,    -1,    -1, { "Core(TM) Ultra 9 2##V",  10 }, "Core Ultra 9 (Lunar Lake-V)",  "TSMC N3B" },
	{  6, 13, -1, -1, 189, -1,    -1,    -1, { "Core(TM) Ultra 7 2##V",  10 }, "Core Ultra 7 (Lunar Lake-V)",  "TSMC N3B" },
	{  6, 13, -1, -1, 189, -1,    -1,    -1, { "Core(TM) Ultra 5 2##V",  10 }, "Core Ultra 5 (Lunar Lake-V)",  "TSMC N3B" },

	/* Cougar Cove (P-cores) / Darkmont (E-cores and LP E-cores) CPUs (2025, Core Ultra Series 3, Intel 18A): https://en.wikipedia.org/wiki/Panther_Lake_(microprocessor) */
	// TBA
//     F   M   S  EF    EM #cores L2$    L3$ Pattern                          Codename                       Technology


	/* Itaniums */
	{  7, -1, -1, -1, -1,   1,    -1,    -1, { "",  0 }, "Itanium",   UNKN_STR },
	{ 15, -1, -1, 16, -1,   1,    -1,    -1, { "",  0 }, "Itanium 2", UNKN_STR },
};


static void load_intel_features(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	const struct feature_map_t matchtable_edx1[] = {
		{ 18, CPU_FEATURE_PN },
		{ 21, CPU_FEATURE_DTS },
		{ 22, CPU_FEATURE_ACPI },
		{ 27, CPU_FEATURE_SS },
		{ 29, CPU_FEATURE_TM },
		{ 30, CPU_FEATURE_IA64 },
		{ 31, CPU_FEATURE_PBE },
	};
	const struct feature_map_t matchtable_ecx1[] = {
		{  2, CPU_FEATURE_DTS64 },
		{  4, CPU_FEATURE_DS_CPL },
		{  5, CPU_FEATURE_VMX },
		{  6, CPU_FEATURE_SMX },
		{  7, CPU_FEATURE_EST },
		{  8, CPU_FEATURE_TM2 },
		{ 10, CPU_FEATURE_CID },
		{ 14, CPU_FEATURE_XTPR },
		{ 15, CPU_FEATURE_PDCM },
		{ 18, CPU_FEATURE_DCA },
		/* id 21 is handled in common */
	};
	const struct feature_map_t matchtable_eax6[] = {
		{ 0, CPU_FEATURE_INTEL_DTS },
		{ 6, CPU_FEATURE_INTEL_PTM },
	};
	const struct feature_map_t matchtable_edx81[] = {
		{ 20, CPU_FEATURE_XD },
	};
	const struct feature_map_t matchtable_ebx7[] = {
		{  2, CPU_FEATURE_SGX },
		{  4, CPU_FEATURE_HLE },
		{ 11, CPU_FEATURE_RTM },
		/* id 16 to 19 are handled in common */
		{ 26, CPU_FEATURE_AVX512PF },
		{ 27, CPU_FEATURE_AVX512ER },
		/* id 28 to 31 are handled in common */
	};

	if (raw->basic_cpuid[0][EAX] >= 1) {
		match_features(matchtable_edx1, COUNT_OF(matchtable_edx1), raw->basic_cpuid[1][EDX], data);
		match_features(matchtable_ecx1, COUNT_OF(matchtable_ecx1), raw->basic_cpuid[1][ECX], data);
	}
	if (raw->ext_cpuid[0][EAX] >= 1) {
		match_features(matchtable_edx81, COUNT_OF(matchtable_edx81), raw->ext_cpuid[1][EDX], data);
	}
	// detect INTEL DTM/PTM:
	if (raw->basic_cpuid[0][EAX] >= 6) {
		match_features(matchtable_eax6, COUNT_OF(matchtable_eax6), raw->basic_cpuid[6][EAX], data);
	}
	// detect TSX/AVX512:
	if (raw->basic_cpuid[0][EAX] >= 7) {
		match_features(matchtable_ebx7, COUNT_OF(matchtable_ebx7), raw->basic_cpuid[7][EBX], data);
	}
}

static void decode_intel_oldstyle_cache_info(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	uint8_t f[256] = {0};
	int reg, off;
	uint32_t x;
	for (reg = 0; reg < 4; reg++) {
		x = raw->basic_cpuid[2][reg];
		if (x & 0x80000000) continue;
		for (off = 0; off < 4; off++) {
			f[x & 0xff] = 1;
			x >>= 8;
		}
	}

	assign_cache_data(f[0x06], L1I,      8,  4,  32, data);
	assign_cache_data(f[0x08], L1I,     16,  4,  32, data);
	assign_cache_data(f[0x0A], L1D,      8,  2,  32, data);
	assign_cache_data(f[0x0C], L1D,     16,  4,  32, data);
	assign_cache_data(f[0x22],  L3,    512,  4,  64, data);
	assign_cache_data(f[0x23],  L3,   1024,  8,  64, data);
	assign_cache_data(f[0x25],  L3,   2048,  8,  64, data);
	assign_cache_data(f[0x29],  L3,   4096,  8,  64, data);
	assign_cache_data(f[0x2C], L1D,     32,  8,  64, data);
	assign_cache_data(f[0x30], L1I,     32,  8,  64, data);
	assign_cache_data(f[0x39],  L2,    128,  4,  64, data);
	assign_cache_data(f[0x3A],  L2,    192,  6,  64, data);
	assign_cache_data(f[0x3B],  L2,    128,  2,  64, data);
	assign_cache_data(f[0x3C],  L2,    256,  4,  64, data);
	assign_cache_data(f[0x3D],  L2,    384,  6,  64, data);
	assign_cache_data(f[0x3E],  L2,    512,  4,  64, data);
	assign_cache_data(f[0x41],  L2,    128,  4,  32, data);
	assign_cache_data(f[0x42],  L2,    256,  4,  32, data);
	assign_cache_data(f[0x43],  L2,    512,  4,  32, data);
	assign_cache_data(f[0x44],  L2,   1024,  4,  32, data);
	assign_cache_data(f[0x45],  L2,   2048,  4,  32, data);
	assign_cache_data(f[0x46],  L3,   4096,  4,  64, data);
	assign_cache_data(f[0x47],  L3,   8192,  8,  64, data);
	assign_cache_data(f[0x4A],  L3,   6144, 12,  64, data);
	assign_cache_data(f[0x4B],  L3,   8192, 16,  64, data);
	assign_cache_data(f[0x4C],  L3,  12288, 12,  64, data);
	assign_cache_data(f[0x4D],  L3,  16384, 16,  64, data);
	assign_cache_data(f[0x4E],  L2,   6144, 24,  64, data);
	assign_cache_data(f[0x60], L1D,     16,  8,  64, data);
	assign_cache_data(f[0x66], L1D,      8,  4,  64, data);
	assign_cache_data(f[0x67], L1D,     16,  4,  64, data);
	assign_cache_data(f[0x68], L1D,     32,  4,  64, data);
	/* The following four entries are trace cache. Intel does not
	 * specify a cache-line size, so we use -1 instead
	 */
	assign_cache_data(f[0x70], L1I,     12,  8,  -1, data);
	assign_cache_data(f[0x71], L1I,     16,  8,  -1, data);
	assign_cache_data(f[0x72], L1I,     32,  8,  -1, data);
	assign_cache_data(f[0x73], L1I,     64,  8,  -1, data);

	assign_cache_data(f[0x78],  L2,   1024,  4,  64, data);
	assign_cache_data(f[0x79],  L2,    128,  8,  64, data);
	assign_cache_data(f[0x7A],  L2,    256,  8,  64, data);
	assign_cache_data(f[0x7B],  L2,    512,  8,  64, data);
	assign_cache_data(f[0x7C],  L2,   1024,  8,  64, data);
	assign_cache_data(f[0x7D],  L2,   2048,  8,  64, data);
	assign_cache_data(f[0x7F],  L2,    512,  2,  64, data);
	assign_cache_data(f[0x82],  L2,    256,  8,  32, data);
	assign_cache_data(f[0x83],  L2,    512,  8,  32, data);
	assign_cache_data(f[0x84],  L2,   1024,  8,  32, data);
	assign_cache_data(f[0x85],  L2,   2048,  8,  32, data);
	assign_cache_data(f[0x86],  L2,    512,  4,  64, data);
	assign_cache_data(f[0x87],  L2,   1024,  8,  64, data);

	if (f[0x49]) {
		/* This flag is overloaded with two meanings. On Xeon MP
		 * (family 0xf, model 0x6) this means L3 cache. On all other
		 * CPUs (notably Conroe et al), this is L2 cache. In both cases
		 * it means 4MB, 16-way associative, 64-byte line size.
		 */
		if (data->x86.family == 0xf && data->x86.model == 0x6) {
			data->l3_cache = 4096;
			data->l3_assoc = 16;
			data->l3_cacheline = 64;
		} else {
			data->l2_cache = 4096;
			data->l2_assoc = 16;
			data->l2_cacheline = 64;
		}
	}
	if (f[0x40]) {
		/* Again, a special flag. It means:
		 * 1) If no L2 is specified, then CPU is w/o L2 (0 KB)
		 * 2) If L2 is specified by other flags, then, CPU is w/o L3.
		 */
		if (data->l2_cache == -1) {
			data->l2_cache = 0;
		} else {
			data->l3_cache = 0;
		}
	}
}

static int decode_intel_extended_topology(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int i, level_type, num_smt = -1, num_core = -1;

	for (i = 0; (i < MAX_INTELFN11_LEVEL) && (raw->intel_fn11[i][EAX] != 0x0) && (raw->intel_fn11[i][EBX] != 0x0); i++) {
		level_type = EXTRACTS_BITS(raw->intel_fn11[i][ECX], 15, 8);
		switch (level_type) {
			case 0x01:
				num_smt = EXTRACTS_BITS(raw->intel_fn11[i][EBX], 15, 0);
				break;
			case 0x02:
				num_core = EXTRACTS_BITS(raw->intel_fn11[i][EBX], 15, 0);
				break;
			default:
				break;
		}
	}
	if (num_smt == -1 || num_core == -1) return 0;
	data->num_logical_cpus = num_core;
	data->num_cores = num_core / num_smt;
	// make sure num_cores is at least 1. In VMs, the CPUID instruction
	// is rigged and may give nonsensical results, but we should at least
	// avoid outputs like data->num_cores == 0.
	if (data->num_cores <= 0) data->num_cores = 1;
	return 1;
}

static void decode_intel_sgx_features(const struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	struct cpu_epc_t epc;
	int i;

	if (raw->basic_cpuid[0][EAX] < 0x12) return; // no 12h leaf
	if (raw->basic_cpuid[0x12][EAX] == 0) return; // no sub-leafs available, probably it's disabled by BIOS

	// decode sub-leaf 0:
	if (raw->basic_cpuid[0x12][EAX] & 1) data->x86.sgx.flags[INTEL_SGX1] = 1;
	if (raw->basic_cpuid[0x12][EAX] & 2) data->x86.sgx.flags[INTEL_SGX2] = 1;
	if (data->x86.sgx.flags[INTEL_SGX1] || data->x86.sgx.flags[INTEL_SGX2])
		data->x86.sgx.present = 1;
	data->x86.sgx.misc_select = raw->basic_cpuid[0x12][EBX];
	data->x86.sgx.max_enclave_32bit = (raw->basic_cpuid[0x12][EDX]     ) & 0xff;
	data->x86.sgx.max_enclave_64bit = (raw->basic_cpuid[0x12][EDX] >> 8) & 0xff;

	// decode sub-leaf 1:
	data->x86.sgx.secs_attributes = raw->intel_fn12h[1][EAX] | (((uint64_t) raw->intel_fn12h[1][EBX]) << 32);
	data->x86.sgx.secs_xfrm       = raw->intel_fn12h[1][ECX] | (((uint64_t) raw->intel_fn12h[1][EDX]) << 32);

	// decode higher-order subleafs, whenever present:
	data->x86.sgx.num_epc_sections = -1;
	for (i = 0; i < 1000000; i++) {
		epc = cpuid_get_epc(i, raw);
		if (epc.length == 0) {
			debugf(2, "SGX: epc section request for %d returned null, no more EPC sections.\n", i);
			data->x86.sgx.num_epc_sections = i;
			break;
		}
	}
	if (data->x86.sgx.num_epc_sections == -1) {
		debugf(1, "SGX: warning: seems to be infinitude of EPC sections.\n");
		data->x86.sgx.num_epc_sections = 1000000;
	}
}

struct cpu_epc_t cpuid_get_epc(int index, const struct cpu_raw_data_t* raw)
{
	uint32_t regs[4];
	struct cpu_epc_t retval = {0, 0};
	if (raw && index < MAX_INTELFN12H_LEVEL - 2) {
		// this was queried already, use the data:
		memcpy(regs, raw->intel_fn12h[2 + index], sizeof(regs));
	} else {
		// query this ourselves:
		regs[0] = 0x12;
		regs[2] = 2 + index;
		regs[1] = regs[3] = 0;
		cpu_exec_cpuid_ext(regs);
	}

	// decode values:
	if ((regs[0] & 0xf) == 0x1) {
		retval.start_addr |= (regs[0] & 0xfffff000); // bits [12, 32) -> bits [12, 32)
		retval.start_addr |= ((uint64_t) (regs[1] & 0x000fffff)) << 32; // bits [0, 20) -> bits [32, 52)
		retval.length     |= (regs[2] & 0xfffff000); // bits [12, 32) -> bits [12, 32)
		retval.length     |= ((uint64_t) (regs[3] & 0x000fffff)) << 32; // bits [0, 20) -> bits [32, 52)
	}
	return retval;
}

int cpuid_identify_intel(struct cpu_raw_data_t* raw, struct cpu_id_t* data, struct internal_id_info_t* internal)
{
	load_intel_features(raw, data);
	if (raw->basic_cpuid[0][EAX] >= 4) {
		/* Deterministic way is preferred, being more generic */
		decode_deterministic_cache_info_x86(raw->intel_fn4, MAX_INTELFN4_LEVEL, data, internal);
	} else if (raw->basic_cpuid[0][EAX] >= 2) {
		decode_intel_oldstyle_cache_info(raw, data);
	}
	if ((raw->basic_cpuid[0][EAX] < 11) || (decode_intel_extended_topology(raw, data) == 0))
		decode_number_of_cores_x86(raw, data);
	decode_architecture_version_x86(data);
	data->purpose = cpuid_identify_purpose_intel(raw);
	internal->score = match_cpu_codename(cpudb_intel, COUNT_OF(cpudb_intel), data);

	if (data->flags[CPU_FEATURE_SGX]) {
		debugf(2, "SGX seems to be present, decoding...\n");
		// if SGX is indicated by the CPU, verify its presence:
		decode_intel_sgx_features(raw, data);
	}

	return 0;
}

void cpuid_get_list_intel(struct cpu_list_t* list)
{
	generic_get_cpu_list(cpudb_intel, COUNT_OF(cpudb_intel), list);
}

cpu_purpose_t cpuid_identify_purpose_intel(struct cpu_raw_data_t* raw)
{
	/* Check for hybrid architecture
	From Intel® 64 and IA-32 Architectures Software Developer’s Manual Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
	Available at https://cdrdv2.intel.com/v1/dl/getContent/671200

	- CPUID[7h] is Structured Extended Feature Flags Enumeration Leaf (Output depends on ECX input value)
	  EDX, bit 15: Hybrid. If 1, the processor is identified as a hybrid part.

	- CPUID[1Ah] is Hybrid Information Enumeration Leaf (EAX = 1AH, ECX = 0)
	  EAX, bits 31-24: Core type
	*/
	if (EXTRACTS_BIT(raw->basic_cpuid[0x7][EDX], 15) == 0x1) {
		debugf(3, "Detected Intel CPU hybrid architecture\n");
		switch (EXTRACTS_BITS(raw->basic_cpuid[0x1a][EAX], 31, 24)) {
			case 0x20: /* Atom */
				/* Acccording to Ramyer M. from Intel, LP E-Cores do not have a L3 cache
				   https://community.intel.com/t5/Processors/Detecting-LP-E-Cores-on-Meteor-Lake-in-software/m-p/1584555/highlight/true#M70732
				   If sub-leaf 3 is set, it is an E-Cores.
				*/
				return (EXTRACTS_BITS(raw->intel_fn4[3][EAX], 31, 0)) ? PURPOSE_EFFICIENCY : PURPOSE_LP_EFFICIENCY;
			case 0x40: /* Core */
				return PURPOSE_PERFORMANCE;
			default:
				return PURPOSE_GENERAL;
		}
	}

	return PURPOSE_GENERAL;
}
