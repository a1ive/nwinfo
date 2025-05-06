/*
 * Copyright 2023  Veselin Georgiev,
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "libcpuid.h"
#include "libcpuid_util.h"
#include "libcpuid_internal.h"
#include "recog_centaur.h"


const struct match_entry_t cpudb_centaur[] = {
//     F   M   S  EF    EM #cores  L2$    L3$  Pattern                              Codename                         Technology
	{ -1, -1, -1, -1,   -1,  -1,    -1,    -1, { "",                           0 }, "Unknown Centaur CPU",           ""       },
//     F   M   S  EF    EM #cores  L2$    L3$  Pattern                              Codename                         Technology


	/* VIA */
//     F   M   S  EF    EM #cores  L2$    L3$  Pattern                              Codename                         Technology
	{  6, -1, -1,  -1,   -1,  -1,    -1,    -1, { "VIA",                       2 }, "Unknown VIA CPU",               ""       },

	/* Samuel (2000, 180 nm) */
	{  6,   6, -1, -1,   -1,  -1,    -1,    -1, { "VIA Samuel",                4 }, "VIA Cyrix III (Samuel)",        "180 nm" },
	/* Samuel 2 (2001, 150 nm) */
	{  6,   7, -1, -1,   -1,  -1,    -1,    -1, { "VIA Samuel 2",              6 }, "VIA C3 (Samuel 2)",             "150 nm" },
	/* Ezra (2001, 130 nm) */
	{  6,   7, -1, -1,   -1,  -1,    -1,    -1, { "VIA Ezra",                  4 }, "VIA C3 (Ezra)",                 "130 nm" },
	{  6,   8, -1, -1,   -1,  -1,    -1,    -1, { "VIA C3 Ezra",               6 }, "VIA C3 (Ezra-T)",               "130 nm" },
	/* Nehemiah (2003, 130 nm) */
	{  6,   9, -1, -1,   -1,  -1,    -1,    -1, { "VIA Nehemiah",              4 }, "VIA C3 (Nehemiah)",             "130 nm" },
	/* Esther (2005, 90 nm) */
	{  6,  10, -1, -1,   -1,  -1,    -1,    -1, { "VIA Esther",                4 }, "VIA C7 (Esther)",               "90 nm"  },
	{  6,  13, -1, -1,   -1,  -1,    -1,    -1, { "VIA C7-M",                  4 }, "VIA C7-M (Esther)",             "90 nm"  },
	/* Isaiah (2008, 65 nm) */
	{  6,  15, -1, -1,   -1,  -1,    -1,    -1, { "VIA Nano",                  4 }, "VIA Nano (Isaiah)",             "65 nm"  },
	{  6,  15, -1, -1,   -1,   1,    -1,    -1, { "VIA Nano",                  4 }, "VIA Nano (Isaiah)",             "65 nm"  },
	{  6,  15, -1, -1,   -1,   2,    -1,    -1, { "VIA Nano",                  4 }, "VIA Nano X2 (Isaiah)",          "65 nm"  },
	{  6,  15, -1, -1,   -1,  -1,    -1,    -1, { "VIA QuadCore",              4 }, "VIA Nano X4 (Isaiah)",          "65 nm"  },
	{  6,  15, -1, -1,   -1,   4,    -1,    -1, { "VIA Eden X4",               6 }, "VIA Eden X4 (Isaiah)",          "65 nm"  },
//     F   M   S  EF    EM #cores  L2$    L3$  Pattern                              Codename                         Technology


	/* Zhaoxin */
//     F   M   S  EF    EM #cores  L2$    L3$  Pattern                              Codename                         Technology
	{  7, -1, -1, -1,   -1,  -1,    -1,    -1, {"ZHAOXIN",                     2 }, "Unknown Zhaoxin CPU"            ""       },

	/* Zhangjiang (2015, 28 nm) */
	{  7, -1, -1, -1,   15,  -1,    -1,    -1, { "ZHAOXIN KaisHeng KH-C",      8 }, "Zhaoxin KaisHeng (ZhangJiang)", "28 nm"  }, // C+ (4000)
	{  7, -1, -1, -1,   15,  -1,    -1,    -1, { "ZHAOXIN KaiXian ZX-C",       8 }, "Zhaoxin KaiXian (ZhangJiang)",  "28 nm"  }, // C/C+ (4000)
	/* WuDaoKou (2017, 28 nm) */
	{  7, -1, -1, -1,   27,  -1,    -1,    -1, { "ZHAOXIN KaisHeng KH-20###",  8 }, "Zhaoxin KaisHeng (WuDaoKou)",   "28 nm"  }, // KH (20000)
	{  7, -1, -1, -1,   27,  -1,    -1,    -1, { "ZHAOXIN KaiXian KX-5###",    8 }, "Zhaoxin KaiXian (WuDaoKou)",    "28 nm"  }, // KX (5000)
	{  7, -1, -1, -1,   27,  -1,    -1,    -1, { "ZHAOXIN KaiXian KX-U5###",   8 }, "Zhaoxin KaiXian (WuDaoKou)",    "28 nm"  }, // KX (U5000)
	/* LuJiaZui (2019, 16 nm) */
	{  7, -1, -1, -1,   59,  -1,    -1,    -1, { "ZHAOXIN KaisHeng KH-30###",  8 }, "Zhaoxin KaisHeng (LuJiaZui)",   "16 nm"  }, // KH (30000)
	{  7, -1, -1, -1,   59,  -1,    -1,    -1, { "ZHAOXIN KaiXian KX-6###",    8 }, "Zhaoxin KaiXian (LuJiaZui)",    "16 nm"  }, // KX (6000)
	{  7, -1, -1, -1,   59,  -1,    -1,    -1, { "ZHAOXIN KaiXian KX-U6###",   8 }, "Zhaoxin KaiXian (LuJiaZui)",    "16 nm"  }, // KX (U6000)
	/* Yongfeng (2022, 16 nm) */
	{  7, -1, -1, -1,   91,  -1,    -1,    -1, { "ZHAOXIN KaisHeng KH-40###",  8 }, "Zhaoxin KaisHeng (Yongfeng)",   "16 nm"  }, // KH (40000)
	{  7, -1, -1, -1,   91,  -1,    -1,    -1, { "ZHAOXIN KaiXian KX-7###",    8 }, "Zhaoxin KaiXian (Yongfeng)",    "16 nm"  }, // KX (7000)
//     F   M   S  EF    EM #cores  L2$    L3$  Pattern                              Codename                         Technology
};

int cpuid_identify_centaur(struct cpu_raw_data_t* raw, struct cpu_id_t* data, struct internal_id_info_t* internal)
{
	if (raw->basic_cpuid[0][EAX] >= 4)
		decode_deterministic_cache_info_x86(raw->intel_fn4, MAX_INTELFN4_LEVEL, data, internal);
	decode_number_of_cores_x86(raw, data);
	decode_architecture_version_x86(data);
	internal->score = match_cpu_codename(cpudb_centaur, COUNT_OF(cpudb_centaur), data);

	return 0;
}

void cpuid_get_list_centaur(struct cpu_list_t* list)
{
	generic_get_cpu_list(cpudb_centaur, COUNT_OF(cpudb_centaur), list);
}
