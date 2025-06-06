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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "libcpuid.h"
#include "libcpuid_util.h"
#include "libcpuid_internal.h"
#include "recog_amd.h"

/*
 * Useful links:
 * - List of AMD CPU microarchitectures: https://en.wikipedia.org/wiki/List_of_AMD_CPU_microarchitectures
 * - List of AMD Athlon processors: https://en.wikipedia.org/wiki/List_of_AMD_Athlon_processors#Desktop_processors
 * - List of AMD Duron processors: https://en.wikipedia.org/wiki/List_of_AMD_Duron_processors
 * - List of AMD Sempron processors: https://en.wikipedia.org/wiki/List_of_AMD_Sempron_processors
 * - List of AMD Turion processors: https://en.wikipedia.org/wiki/List_of_AMD_Turion_processors
 * - List of AMD Opteron processors: https://en.wikipedia.org/wiki/List_of_AMD_Opteron_processors
 * - List of AMD Phenom processors: https://en.wikipedia.org/wiki/List_of_AMD_Phenom_processors
 * - List of AMD FX processors: https://en.wikipedia.org/wiki/List_of_AMD_FX_processors
 * - List of AMD processors with 3D graphics: https://en.wikipedia.org/wiki/List_of_AMD_processors_with_3D_graphics
 * - List of AMD Ryzen processors: https://en.wikipedia.org/wiki/List_of_AMD_Ryzen_processors
 * - List of AMD Epyc processors: https://en.wikipedia.org/wiki/Epyc#List_of_Epyc_processors
 */
const struct match_entry_t cpudb_amd[] = {
//     F   M   S  EF    EM #cores  L2$    L3$  Pattern                          Codename                          Technology
	{ -1, -1, -1, -1,   -1,   1,    -1,    -1, { "",                       0 }, "Unknown AMD CPU",                UNKN_STR },

	/* 486 and the likes */
	{  4, -1, -1, -1,   -1,   1,    -1,    -1, { "",                       0 }, "Unknown 486",                    UNKN_STR },
	{  4,  3, -1, -1,   -1,   1,    -1,    -1, { "",                       0 }, "AMD 486DX2",                     UNKN_STR },
	{  4,  7, -1, -1,   -1,   1,    -1,    -1, { "",                       0 }, "AMD 486DX2WB",                   UNKN_STR },
	{  4,  8, -1, -1,   -1,   1,    -1,    -1, { "",                       0 }, "AMD 486DX4",                     UNKN_STR },
	{  4,  9, -1, -1,   -1,   1,    -1,    -1, { "",                       0 }, "AMD 486DX4WB",                   UNKN_STR },

	/* Pentia clones */
	{  5, -1, -1, -1,   -1,   1,    -1,    -1, { "",                       0 }, "Unknown K5",                     UNKN_STR },
	{  5,  0, -1, -1,   -1,   1,    -1,    -1, { "",                       0 }, "K5 (SSA/5)",                     UNKN_STR },
	{  5,  1, -1, -1,   -1,   1,    -1,    -1, { "K5(tm)",                 2 }, "K5 (5k86)",                      "350 nm" },
	{  5,  2, -1, -1,   -1,   1,    -1,    -1, { "K5(tm)",                 2 }, "K5 (5k86)",                      "350 nm" },
	{  5,  3, -1, -1,   -1,   1,    -1,    -1, { "K5(tm)",                 2 }, "K5 (5k86)",                      "350 nm" },

	/* K6 Architecture */
	{  5, -1, -1, -1,   -1,   1,    -1,    -1, { "",                       0 }, "Unknown K6",                     UNKN_STR },
	{  5,  6, -1, -1,   -1,   1,    -1,    -1, { "K6",                     2 }, "K6",                             "350 nm" },
	{  5,  7, -1, -1,   -1,   1,    -1,    -1, { "K6",                     2 }, "K6 (Little Foot)",               "250 nm" },
	{  5,  8,  0, -1,   -1,   1,    -1,    -1, { "K6(tm)",                 2 }, "K6-2 (Chomper)",                 "250 nm" },
	{  5,  8, 12, -1,   -1,   1,    -1,    -1, { "K6(tm)",                 2 }, "K6-2 (Chomper Extended)",        "250 nm" },
	{  5,  9, -1, -1,   -1,   1,    -1,    -1, { "K6(tm)",                 2 }, "K6-III (Sharptooth)",            "250 nm" },
	{  5, 13, -1, -1,   -1,   1,    -1,    -1, { "K6(tm)",                 2 }, "K6-2+",                          "180 nm" },
	{  5, 13, -1, -1,   -1,   1,    -1,    -1, { "K6(tm)-III ",            4 }, "K6-III+",                        "180 nm" },

	/* K7 Architecture */
	{  6, -1, -1, 15,   -1,   1,    -1,    -1, { "",                       0 }, "Unknown K7",                     UNKN_STR },

	{  6,  1, -1, -1,   -1,   1,    -1,    -1, { "K7(tm)",                 0 }, "Athlon (Argon)",                 "250 nm" },

	{  6,  2, -1, -1,   -1,   1,    -1,    -1, { "Athlon(tm)",             0 }, "Athlon (Pluto/Orion)",           "180 nm" },

	{  6,  3, -1, -1,   -1,   1,    -1,    -1, { "Duron(tm)",              2 }, "Duron (Spitfire)",               "180 nm" },
	{  6,  3, -1, -1,   -1,   1,    -1,    -1, { "Duron(tm) M",            4 }, "Mobile Duron (Spitfire)",        "180 nm" },

	{  6,  4, -1, -1,   -1,   1,    -1,    -1, { "Athlon(tm)",             2 }, "Athlon (ThunderBird)",           "180 nm" },

	{  6,  6, -1, -1,   -1,   1,    -1,    -1, { "Athlon XP",              4 }, "Athlon (Palomino)",              "180 nm" },
	{  6,  6, -1, -1,   -1,   1,    -1,    -1, { "Athlon(tm) MP",          4 }, "Athlon MP (Palomino)",           "180 nm" },
	{  6,  6, -1, -1,   -1,   1,    -1,    -1, { "Duron(tm)",              2 }, "Duron (Palomino)",               "180 nm" },

	{  6,  7, -1, -1,   -1,   1,    -1,    -1, { "Duron(tm)",              2 }, "Duron (Morgan)",                 "180 nm" },
	{  6,  7, -1, -1,   -1,   1,    -1,    -1, { "Duron(tm) M",            4 }, "Mobile Duron (Camaro)",          "180 nm" },

	{  6,  8, -1, -1,   -1,   1,    -1,    -1, { "Athlon",                 2 }, "Athlon XP (Thoroughbred)",       "130 nm" },
	{  6,  8, -1, -1,   -1,   1,    -1,    -1, { "Athlon(tm) XP",          4 }, "Athlon XP (Thoroughbred)",       "130 nm" },
	{  6,  8, -1, -1,   -1,   1,    -1,    -1, { "Duron(tm)",              2 }, "Duron (Applebred)",              "130 nm" },
	{  6,  8, -1, -1,   -1,   1,    -1,    -1, { "Sempron(tm)",            2 }, "Sempron (Thoroughbred)",         "130 nm" },
	{  6,  8, -1, -1,   -1,   1,   128,    -1, { "Sempron(tm)",            2 }, "Sempron (Thoroughbred)",         "130 nm" },
	{  6,  8, -1, -1,   -1,   1,   256,    -1, { "Sempron(tm)",            2 }, "Sempron (Thoroughbred)",         "130 nm" },
	{  6,  8, -1, -1,   -1,   1,    -1,    -1, { "Athlon(tm) MP",          4 }, "Athlon MP (Thoroughbred)",       "130 nm" },
	{  6,  8, -1, -1,   -1,   1,    -1,    -1, { "Athlon(tm) XP-M",        6 }, "Mobile Athlon (Thoroughbred)",   "130 nm" },

	{  6, 10, -1, -1,   -1,   1,   512,    -1, { "Athlon(tm) XP",          4 }, "Athlon XP (Barton)",             "130 nm" },
	{  6, 10, -1, -1,   -1,   1,   512,    -1, { "Sempron(tm)",            2 }, "Sempron (Barton)",               "130 nm" },
	{  6, 10, -1, -1,   -1,   1,   256,    -1, { "Sempron(tm)",            2 }, "Sempron (Thorton)",              "130 nm" },
	{  6, 10, -1, -1,   -1,   1,   256,    -1, { "Athlon(tm) XP",          4 }, "Athlon XP (Thorton)",            "130 nm" },
	{  6, 10, -1, -1,   -1,   1,    -1,    -1, { "Athlon(tm) MP",          4 }, "Athlon MP (Barton)",             "130 nm" },
	{  6, 10, -1, -1,   -1,   1,    -1,    -1, { "Athlon(tm) XP-M",        6 }, "Mobile Athlon (Barton)",         "130 nm" },

	/* K8 Architecture */
	{ 15, -1, -1, 15,   -1,   1,    -1,    -1, { "",                       0 }, "Unknown K8",                     UNKN_STR },
	{ 15, -1, -1, 16,   -1,   1,    -1,    -1, { "",                       0 }, "Unknown K9",                     UNKN_STR },
	{ 15, -1, -1, 15,   -1,   1,    -1,    -1, { "",                       0 }, "Unknown A64",                    UNKN_STR },
	{ 15, -1, -1, 15,   -1,   1,    -1,    -1, { "Opteron(tm)",            2 }, "Opteron",                        UNKN_STR },
	{ 15, -1, -1, 15,   -1,   2,    -1,    -1, { "Dual Core AMD Opteron",  8 }, "Opteron (Dual Core)",            UNKN_STR },
	{ 15,  1, -1, 15,   65,   2,    -1,    -1, { "Opteron(tm) 22##",       6 }, "Opteron (Santa Rosa)",           "90 nm" },
	{ 15,  3, -1, 15,   -1,   1,    -1,    -1, { "Opteron(tm)",            2 }, "Opteron",                        UNKN_STR },
	{ 15,  3, -1, 15,   -1,   2,    -1,    -1, { "Dual Core AMD Opteron",  8 }, "Opteron (Dual Core)",            UNKN_STR },
	{ 15,  5, -1, 15,    5,  -1,    -1,    -1, { "Opteron(tm) [128]##",    4 }, "Opteron (SledgeHammer)",         "130 nm" },
	{ 15, -1, -1, 15,   -1,   1,   512,    -1, { "Athlon(tm) 64",          4 }, "Athlon 64 (512K)",               UNKN_STR },
	{ 15, -1, -1, 15,   -1,   1,  1024,    -1, { "Athlon(tm) 64",          4 }, "Athlon 64 (1024K)",              UNKN_STR },
	{ 15, -1, -1, 15,   -1,   1,    -1,    -1, { "Athlon(tm) FX",          4 }, "Athlon FX",                      UNKN_STR },
	{ 15, -1, -1, 15,   -1,   1,    -1,    -1, { "Athlon(tm) 64 FX",       6 }, "Athlon 64 FX",                   UNKN_STR },
	{ 15,  3, -1, 15,   35,   2,    -1,    -1, { "Athlon(tm) 64 FX",       6 }, "Athlon 64 FX X2 (Toledo)",       "90 nm"  },
	{ 15, -1, -1, 15,   -1,   2,   512,    -1, { "Athlon(tm) 64 X2",       6 }, "Athlon 64 X2 (512K)",            UNKN_STR },
	{ 15, -1, -1, 15,   -1,   2,  1024,    -1, { "Athlon(tm) 64 X2",       6 }, "Athlon 64 X2 (1024K)",           UNKN_STR },
	{ 15, -1, -1, 15,   -1,   1,   512,    -1, { "Turion(tm) 64",          4 }, "Turion 64 (512K)",               UNKN_STR },
	{ 15, -1, -1, 15,   -1,   1,  1024,    -1, { "Turion(tm) 64",          4 }, "Turion 64 (1024K)",              UNKN_STR },
	{ 15, -1, -1, 15,   -1,   2,   512,    -1, { "Turion(tm) X2",          4 }, "Turion 64 X2 (512K)",            UNKN_STR },
	{ 15, -1, -1, 15,   -1,   2,  1024,    -1, { "Turion(tm) X2",          4 }, "Turion 64 X2 (1024K)",           UNKN_STR },
	{ 15, -1, -1, 15,   -1,   1,   128,    -1, { "Sempron(tm)",            2 }, "A64 Sempron (128K)",             UNKN_STR },
	{ 15, -1, -1, 15,   -1,   1,   256,    -1, { "Sempron(tm)",            2 }, "A64 Sempron (256K)",             UNKN_STR },
	{ 15, -1, -1, 15,   -1,   1,   512,    -1, { "Sempron(tm)",            2 }, "A64 Sempron (512K)",             UNKN_STR },
	{ 15, -1, -1, 15, 0x4f,   1,   512,    -1, { "Athlon(tm) 64",          4 }, "Athlon 64 (Orleans/512K)",       "90 nm"  },
	{ 15, -1, -1, 15, 0x5f,   1,   512,    -1, { "Athlon(tm) 64",          4 }, "Athlon 64 (Orleans/512K)",       "90 nm"  },
	{ 15, -1, -1, 15, 0x2f,   1,   512,    -1, { "Athlon(tm) 64",          4 }, "Athlon 64 (Venice/512K)",        "90 nm"  },
	{ 15, -1, -1, 15, 0x2c,   1,   512,    -1, { "Athlon(tm) 64",          4 }, "Athlon 64 (Venice/512K)",        "90 nm"  },
	{ 15, -1, -1, 15, 0x1f,   1,   512,    -1, { "Athlon(tm) 64",          4 }, "Athlon 64 (Winchester/512K)",    "90 nm"  },
	{ 15, -1, -1, 15, 0x0c,   1,   512,    -1, { "Athlon(tm) 64",          4 }, "Athlon 64 (Newcastle/512K)",     "130 nm" },
	{ 15, -1, -1, 15, 0x27,   1,   512,    -1, { "Athlon(tm) 64",          4 }, "Athlon 64 (San Diego/512K)",     "90 nm"  },
	{ 15, -1, -1, 15, 0x37,   1,   512,    -1, { "Athlon(tm) 64",          4 }, "Athlon 64 (San Diego/512K)",     "90 nm"  },
	{ 15, -1, -1, 15, 0x04,   1,   512,    -1, { "Athlon(tm) 64",          4 }, "Athlon 64 (ClawHammer/512K)",    "130 nm" },

	{ 15, -1, -1, 15, 0x5f,   1,  1024,    -1, { "Athlon(tm) 64",          4 }, "Athlon 64 (Orleans/1024K)",      "90 nm"  },
	{ 15, -1, -1, 15, 0x27,   1,  1024,    -1, { "Athlon(tm) 64",          4 }, "Athlon 64 (San Diego/1024K)",    "90 nm"  },
	{ 15, -1, -1, 15, 0x04,   1,  1024,    -1, { "Athlon(tm) 64",          4 }, "Athlon 64 (ClawHammer/1024K)",   "130 nm" },

	{ 15, -1, -1, 15, 0x4b,   2,   256,    -1, { "Athlon(tm) 64 X2",       6 }, "Athlon 64 X2 (Windsor/256K)",    "90 nm" },

	{ 15, -1, -1, 15, 0x23,   2,   512,    -1, { "Athlon(tm) 64 X2",       6 }, "Athlon 64 X2 (Toledo/512K)",     "90 nm" },
	{ 15, -1, -1, 15, 0x4b,   2,   512,    -1, { "Athlon(tm) 64 X2",       6 }, "Athlon 64 X2 (Windsor/512K)",    "90 nm" },
	{ 15, -1, -1, 15, 0x43,   2,   512,    -1, { "Athlon(tm) 64 X2",       6 }, "Athlon 64 X2 (Windsor/512K)",    "90 nm" },
	{ 15, -1, -1, 15, 0x6b,   2,   512,    -1, { "Athlon(tm) 64 X2",       6 }, "Athlon 64 X2 (Brisbane/512K)",   "65 nm" },
	{ 15, -1, -1, 15, 0x2b,   2,   512,    -1, { "Athlon(tm) 64 X2",       6 }, "Athlon 64 X2 (Manchester/512K)", "90 nm" },

	{ 15, -1, -1, 15, 0x23,   2,  1024,    -1, { "Athlon(tm) 64 X2",       6 }, "Athlon 64 X2 (Toledo/1024K)",    "90 nm" },
	{ 15, -1, -1, 15, 0x43,   2,  1024,    -1, { "Athlon(tm) 64 X2",       6 }, "Athlon 64 X2 (Windsor/1024K)",   "90 nm" },

	{ 15, -1, -1, 15, 0x08,   1,   128,    -1, { "Mobile AMD Sempron(tm)", 6 }, "Mobile Sempron 64 (Dublin/128K)", "130 nm" },
	{ 15, -1, -1, 15, 0x08,   1,   256,    -1, { "Mobile AMD Sempron(tm)", 6 }, "Mobile Sempron 64 (Dublin/256K)", "130 nm" },
	{ 15, -1, -1, 15, 0x0c,   1,   256,    -1, { "Sempron(tm)",            2 }, "Sempron 64 (Paris)",              "130 nm" },
	{ 15, -1, -1, 15, 0x1c,   1,   128,    -1, { "Sempron(tm)",            2 }, "Sempron 64 (Palermo/128K)",       "90 nm"  },
	{ 15, -1, -1, 15, 0x1c,   1,   256,    -1, { "Sempron(tm)",            2 }, "Sempron 64 (Palermo/256K)",       "90 nm"  },
	{ 15, -1, -1, 15, 0x1c,   1,   128,    -1, { "Mobile AMD Sempron(tm)", 6 }, "Mobile Sempron 64 (Sonora/128K)", "90 nm"  },
	{ 15, -1, -1, 15, 0x1c,   1,   256,    -1, { "Mobile AMD Sempron(tm)", 6 }, "Mobile Sempron 64 (Sonora/256K)", "90 nm"  },
	{ 15, -1, -1, 15, 0x2c,   1,   128,    -1, { "Sempron(tm)",            2 }, "Sempron 64 (Palermo/128K)",       "90 nm"  },
	{ 15, -1, -1, 15, 0x2c,   1,   256,    -1, { "Sempron(tm)",            2 }, "Sempron 64 (Palermo/256K)",       "90 nm"  },
	{ 15, -1, -1, 15, 0x2c,   1,   128,    -1, { "Mobile AMD Sempron(tm)", 6 }, "Mobile Sempron 64 (Albany/128K)", "90 nm"  },
	{ 15, -1, -1, 15, 0x2c,   1,   256,    -1, { "Mobile AMD Sempron(tm)", 6 }, "Mobile Sempron 64 (Albany/256K)", "90 nm"  },
	{ 15, -1, -1, 15, 0x2f,   1,   128,    -1, { "Sempron(tm)",            2 }, "Sempron 64 (Palermo/128K)",       "90 nm"  },
	{ 15, -1, -1, 15, 0x2f,   1,   256,    -1, { "Sempron(tm)",            2 }, "Sempron 64 (Palermo/256K)",       "90 nm"  },
	{ 15, -1, -1, 15, 0x4f,   1,   128,    -1, { "Sempron(tm)",            2 }, "Sempron 64 (Manila/128K)",        "90 nm"  },
	{ 15, -1, -1, 15, 0x4f,   1,   256,    -1, { "Sempron(tm)",            2 }, "Sempron 64 (Manila/256K)",        "90 nm"  },
	{ 15, -1, -1, 15, 0x5f,   1,   128,    -1, { "Sempron(tm)",            2 }, "Sempron 64 (Manila/128K)",        "90 nm"  },
	{ 15, -1, -1, 15, 0x5f,   1,   256,    -1, { "Sempron(tm)",            2 }, "Sempron 64 (Manila/256K)",        "90 nm"  },
	{ 15, -1, -1, 15, 0x6b,   2,   256,    -1, { "Sempron(tm)",            2 }, "Sempron 64 Dual (Sherman/256K)",  "65 nm"  },
	{ 15, -1, -1, 15, 0x6b,   2,   512,    -1, { "Sempron(tm)",            2 }, "Sempron 64 Dual (Sherman/512K)",  "65 nm"  },
	{ 15, -1, -1, 15, 0x7c,   1,   512,    -1, { "Athlon(tm) 64",          4 }, "Athlon 64 (Sherman/512K)",        "65 nm"  },
	{ 15, -1, -1, 15, 0x7f,   1,   256,    -1, { "Sempron(tm)",            2 }, "Sempron 64 (Sparta/256K)",        "65 nm"  },
	{ 15, -1, -1, 15, 0x7f,   1,   512,    -1, { "Sempron(tm)",            2 }, "Sempron 64 (Sparta/512K)",        "65 nm"  },
	{ 15, -1, -1, 15, 0x4c,   1,   256,    -1, { "Mobile AMD Sempron(tm)", 6 }, "Mobile Sempron 64 (Keene/256K)",  "90 nm"  },
	{ 15, -1, -1, 15, 0x4c,   1,   512,    -1, { "Mobile AMD Sempron(tm)", 6 }, "Mobile Sempron 64 (Keene/512K)",  "90 nm"  },

	{ 15, -1, -1, 15, 0x24,   1,   512,    -1, { "Turion(tm) 64",          4 }, "Turion 64 (Lancaster/512K)",      "90 nm" },
	{ 15, -1, -1, 15, 0x24,   1,  1024,    -1, { "Turion(tm) 64",          4 }, "Turion 64 (Lancaster/1024K)",     "90 nm" },
	{ 15, -1, -1, 15, 0x48,   2,   256,    -1, { "Turion(tm) X2",          4 }, "Turion X2 (Taylor)",              "90 nm" },
	{ 15, -1, -1, 15, 0x48,   2,   512,    -1, { "Turion(tm) X2",          4 }, "Turion X2 (Trinidad)",            "90 nm" },
	{ 15, -1, -1, 15, 0x4c,   1,   512,    -1, { "Turion(tm) 64",          4 }, "Turion 64 (Richmond)",            "90 nm" },
	{ 15, -1, -1, 15, 0x68,   2,   256,    -1, { "Turion(tm) X2",          4 }, "Turion X2 (Tyler/256K)",          "65 nm" },
	{ 15, -1, -1, 15, 0x68,   2,   512,    -1, { "Turion(tm) X2",          4 }, "Turion X2 (Tyler/512K)",          "65 nm" },
	{ 15, -1, -1, 17,    3,   2,   512,    -1, { "Turion(tm) X2",          4 }, "Turion X2 (Griffin/512K)",        "65 nm" },
	{ 15, -1, -1, 17,    3,   2,  1024,    -1, { "Turion(tm) X2",          4 }, "Turion X2 (Griffin/1024K)",       "65 nm" },

	/* K10 Architecture (2007) */
	{ 15,  2, -1, 16,   -1,   3,    -1,    -1, { "Phenom(tm)",             2 }, "Phenom X3 (Toliman)",            "65 nm" },
	{ 15,  2, -1, 16,   -1,   4,    -1,    -1, { "Phenom(tm)",             2 }, "Phenom X4 (Agena)",              "65 nm" },
	{ 15,  2, -1, 16,   -1,   3,   512,    -1, { "Phenom(tm)",             2 }, "Phenom X3 (Toliman/256K)",       "65 nm" },
	{ 15,  2, -1, 16,   -1,   3,   512,    -1, { "Phenom(tm)",             2 }, "Phenom X3 (Toliman/512K)",       "65 nm" },
	{ 15,  2, -1, 16,   -1,   4,   128,    -1, { "Phenom(tm)",             2 }, "Phenom X4 (Agena/128K)",         "65 nm" },
	{ 15,  2, -1, 16,   -1,   4,   256,    -1, { "Phenom(tm)",             2 }, "Phenom X4 (Agena/256K)",         "65 nm" },
	{ 15,  2, -1, 16,   -1,   4,   512,    -1, { "Phenom(tm)",             2 }, "Phenom X4 (Agena/512K)",         "65 nm" },
	{ 15,  2, -1, 16,   -1,   2,   512,    -1, { "Athlon(tm) 64 X2",       6 }, "Athlon X2 (Kuma)",               "65 nm" },
	/* Phenom II derivates: */
	{ 15,  4, -1, 16,   -1,   1,  1024,    -1, { "Sempron(tm)",            2 }, "Sempron (Sargas)",               "45 nm" },
	{ 15,  4, -1, 16,   -1,   2,   512,    -1, { "Phenom(tm) II",          4 }, "Phenom II X2 (Callisto)",        "45 nm" },
	{ 15,  4, -1, 16,   -1,   3,   512,    -1, { "Phenom(tm) II",          4 }, "Phenom II X3 (Heka)",            "45 nm" },
	{ 15,  4, -1, 16,    4,   4,   512,    -1, { "Phenom(tm) II",          4 }, "Phenom II X4 (Deneb)",           "45 nm" },
	{ 15,  5, -1, 16,    5,   4,   512,    -1, { "Phenom(tm) II",          4 }, "Phenom II X4 (Deneb)",           "45 nm" },
	{ 15,  4, -1, 16,   10,   4,   512,    -1, { "Phenom(tm) II",          4 }, "Phenom II X4 (Zosma)",           "45 nm" },
	{ 15,  4, -1, 16,   10,   6,   512,    -1, { "Phenom(tm) II",          4 }, "Phenom II X6 (Thuban)",          "45 nm" },
	/* Athlon II derivates: */
	{ 15,  6, -1, 16,    6,   2,   512,    -1, { "Athlon(tm) II",          4 }, "Athlon II (Champlain)",          "45 nm" },
	{ 15,  6, -1, 16,    6,   2,   512,    -1, { "Athlon(tm) II X2",       6 }, "Athlon II X2 (Regor)",           "45 nm" },
	{ 15,  6, -1, 16,    6,   2,  1024,    -1, { "Athlon(tm) II X2",       6 }, "Athlon II X2 (Regor)",           "45 nm" },
	{ 15,  5, -1, 16,    5,   3,   512,    -1, { "Athlon(tm) II X3",       6 }, "Athlon II X3 (Rana)",            "45 nm" },
	{ 15,  5, -1, 16,    5,   4,   512,    -1, { "Athlon(tm) X4",          4 }, "Athlon II X4 (Propus)",          "45 nm" },
	{ 15,  5, -1, 16,    5,   4,   512,    -1, { "Athlon(tm) II X4",       6 }, "Athlon II X4 (Propus)",          "45 nm" },
	/* Opteron derivates: */
	{ 15,  2, -1, 16,    2,  -1,    -1,    -1, { "Opteron(tm) [28]3##",    4 }, "Opteron (Barcelona)",   "45 nm" },
	{ 15,  4, -1, 16,    4,  -1,    -1,    -1, { "Opteron(tm) [28]3##",    4 }, "Opteron (Shanghai)",    "45 nm" },
	{ 15,  8, -1, 16,    8,  -1,    -1,    -1, { "Opteron(tm) [28]4##",    4 }, "Opteron (Istanbul)",    "45 nm" },
	{ 15,  8, -1, 16,    8,  -1,    -1,    -1, { "Opteron(tm) 41##",       4 }, "Opteron (Lisbon)",      "45 nm" },
	{ 15,  9, -1, 16,    9,   8,    -1,    -1, { "Opteron(tm) 64##",       4 }, "Opteron (Magny-Cours)", "45 nm" },

	/* Llano APUs (2011): */
	{ 15,  1, -1, 18,    1,   4,    -1,    -1, { "Athlon(tm) II X4 6##",     6 }, "Athlon II X4 (Llano)", "GF 32SHP" },
	{ 15,  1, -1, 18,    1,   2,    -1,    -1, { "Athlon(tm) II X2 2##",     6 }, "Athlon II X2 (Llano)", "GF 32SHP" },
	{ 15,  1, -1, 18,    1,   2,    -1,    -1, { "Sempron(tm) X2 1##",       6 }, "Sempron X2 (Llano)",   "GF 32SHP" },
	{ 15,  1, -1, 18,    1,  -1,    -1,    -1, { "E2-3###",                  4 }, "E-Series (Llano)",     "GF 32SHP" },
	{ 15,  1, -1, 18,    1,  -1,    -1,    -1, { "A[468]-3###",              4 }, "A-Series (Llano)",     "GF 32SHP" },

	/* Family 14h: Bobcat Architecture (2011) */
	{ 15,  1, -1, 20,   -1,  -1,    -1,    -1, { "C-[356]#",                 4 }, "C-Series (Ontario)",  "TSMC N40" },
	{ 15,  1, -1, 20,   -1,  -1,    -1,    -1, { "E-[234]##",                4 }, "E-Series (Zacate)",   "TSMC N40" },
	{ 15,  1, -1, 20,   -1,  -1,    -1,    -1, { "G-T##[LRNE]",              6 }, "G-Series (Zacate)",   "TSMC N40" },
	{ 15,  1, -1, 20,   -1,  -1,    -1,    -1, { "Z-##",                     4 }, "Z-Series (Desna)",    "TSMC N40" },

	/* Family 15h: Bulldozer Architecture (2011) */
	{ 15, -1, -1, 21,    0,  -1,    -1,    -1, { "FX(tm)-[468]###",          4 }, "FX (Zambezi)",         "GF 32SHP" },
	{ 15, -1, -1, 21,    1,  -1,    -1,    -1, { "FX(tm)-[468]###",          4 }, "FX (Zambezi)",         "GF 32SHP" },
	{ 15, -1, -1, 21,    1,  -1,    -1,    -1, { "Opteron(tm)",              2 }, "Opteron (Interlagos)", "GF 32SHP" },
	/* 2nd-gen, Piledriver core (2012): */
	{ 15, -1, -1, 21,    2,  -1,    -1,    -1, { "FX(tm)-[4689]###",         4 }, "FX (Vishera)",          "GF 32SHP" },
	{ 15,  0, -1, 21,   16,   4,    -1,    -1, { "Athlon(tm) X4 7##",        6 }, "Athlon X4 (Trinity)",   "GF 32SHP" },
	{ 15,  0, -1, 21,   16,   2,    -1,    -1, { "Athlon(tm) X2 3##",        6 }, "Athlon X2 (Trinity)",   "GF 32SHP" },
	{ 15,  0, -1, 21,   16,   2,    -1,    -1, { "Sempron(tm) X2 2##",       6 }, "Sempron X2 (Trinity)",  "GF 32SHP" },
	{ 15,  0, -1, 21,   16,  -1,    -1,    -1, { "A[468]-5###",              4 }, "A-Series (Trinity)",    "GF 32SHP" },
	{ 15,  0, -1, 21,   16,  -1,    -1,    -1, { "A10-5###",                 4 }, "A-Series (Trinity)",    "GF 32SHP" },
	{ 15,  0, -1, 21,   16,  -1,    -1,    -1, { "A[468]-4###M",             6 }, "A-Series (Trinity)",    "GF 32SHP" },
	{ 15,  0, -1, 21,   16,  -1,    -1,    -1, { "A10-4###M",                6 }, "A-Series (Trinity)",    "GF 32SHP" },
	{ 15,  0, -1, 21,   19,  -1,    -1,    -1, { "FX(tm)-6##K",              6 }, "FX (Richland)",         "GF 32SHP" },
	{ 15,  0, -1, 21,   19,   4,    -1,    -1, { "Athlon(tm) X4 7##",        6 }, "Athlon X4 (Richland)",  "GF 32SHP" },
	{ 15,  0, -1, 21,   19,   2,    -1,    -1, { "Athlon(tm) X2 3##",        6 }, "Athlon X2 (Richland)",  "GF 32SHP" },
	{ 15,  0, -1, 21,   19,   2,    -1,    -1, { "Sempron(tm) X2 2##",       6 }, "Sempron X2 (Richland)", "GF 32SHP" },
	{ 15,  0, -1, 21,   19,  -1,    -1,    -1, { "A4 PRO-7###B",             8 }, "A-Series (Richland)",   "GF 32SHP" },
	{ 15,  0, -1, 21,   19,  -1,    -1,    -1, { "A[468]-[467]###",          4 }, "A-Series (Richland)",   "GF 32SHP" },
	{ 15,  0, -1, 21,   19,  -1,    -1,    -1, { "A10-6###",                 4 }, "A-Series (Richland)",   "GF 32SHP" },
	{ 15,  0, -1, 21,   19,  -1,    -1,    -1, { "A[468]-5###M",             6 }, "A-Series (Richland)",   "GF 32SHP" },
	{ 15,  0, -1, 21,   19,  -1,    -1,    -1, { "A10-5###M",                6 }, "A-Series (Richland)",   "GF 32SHP" },
	{ 15,  2, -1, 21,    2,  -1,    -1,    -1, { "Opteron(tm)",              2 }, "Opteron (Abu Dhabi)",   "GF 32SHP" },
	/* 3rd-gen, Steamroller core (2014): */
	{ 15,  8, -1, 21,   48,  -1,    -1,    -1, { "FX(tm)-7##K",              6 }, "FX (Kaveri)",            "TSMC N28" },
	{ 15,  8, -1, 21,   48,  -1,    -1,    -1, { "FX(tm)-7###",              4 }, "FX (Kaveri)",            "TSMC N28" },
	{ 15,  8, -1, 21,   48,   4,    -1,    -1, { "Athlon(tm) X4 8##",        6 }, "Athlon X4 (Kaveri)",     "TSMC N28" },
	{ 15,  8, -1, 21,   48,   2,    -1,    -1, { "Athlon(tm) X2 4##",        6 }, "Athlon X2 (Kaveri)",     "TSMC N28" },
	{ 15,  8, -1, 21,   48,  -1,    -1,    -1, { "A[468] PRO-[78]###B",      8 }, "A-Series (Kaveri)",      "TSMC N28" },
	{ 15,  8, -1, 21,   48,  -1,    -1,    -1, { "A[468]-7###",              4 }, "A-Series (Kaveri)",      "TSMC N28" },
	{ 15,  8, -1, 21,   48,  -1,    -1,    -1, { "A10 PRO-[78]###B",         8 }, "A-Series (Kaveri)",      "TSMC N28" },
	{ 15,  8, -1, 21,   48,  -1,    -1,    -1, { "A10-7###",                 4 }, "A-Series (Kaveri)",      "TSMC N28" },
	{ 15,  8, -1, 21,   56,  -1,    -1,    -1, { "A[468]-[78]###",           4 }, "A-Series (Godavari)",    "TSMC N28" },
	{ 15,  8, -1, 21,   56,  -1,    -1,    -1, { "A10-[78]###",              4 }, "A-Series (Godavari)",    "TSMC N28" },
	{ 15,  8, -1, 21,   56,   4,    -1,    -1, { "Athlon(tm) X4 8##",        6 }, "Athlon X4 (Godavari)",   "TSMC N28" },
	{ 15,  0, -1, 21,   48,  -1,    -1,    -1, { "RX-###",                   4 }, "R-Series (Bald Eagle)",  "TSMC N28" },
	/* 4th-gen, Excavator core (2015): */
	{ 15,  0, -1, 21,   96,  -1,    -1,    -1, { "FX-8###P",                 6 }, "FX (Carrizo)",              "GF 28SHP" },
	{ 15,  1, -1, 21,   96,   4,    -1,    -1, { "Athlon(tm) X4 8##",        6 }, "Athlon X4 (Carrizo)",       "GF 28SHP" },
	{ 15,  1, -1, 21,   96,  -1,    -1,    -1, { "A[68] PRO-8###",           6 }, "A-Series (Carrizo)",        "GF 28SHP" },
	{ 15,  1, -1, 21,   96,  -1,    -1,    -1, { "A[68]-[78]###",            4 }, "A-Series (Carrizo)",        "GF 28SHP" },
	{ 15,  1, -1, 21,   96,  -1,    -1,    -1, { "A1[02] PRO-8###",          6 }, "A-Series (Carrizo)",        "GF 28SHP" },
	{ 15,  1, -1, 21,   96,  -1,    -1,    -1, { "A1[02]-8###",              4 }, "A-Series (Carrizo)",        "GF 28SHP" },
	{ 15,  5, -1, 21,  101,  -1,    -1,    -1, { "FX-9###P",                 6 }, "FX (Bristol Ridge)",        "GF 28SHP" },
	{ 15,  5, -1, 21,  101,  -1,    -1,    -1, { "Athlon(tm) X4 9##",        6 }, "Athlon X4 (Bristol Ridge)", "GF 28SHP" },
	{ 15,  5, -1, 21,  101,  -1,    -1,    -1, { "A[68] PRO-9###",           6 }, "A-Series (Bristol Ridge)",  "GF 28SHP" },
	{ 15,  5, -1, 21,  101,  -1,    -1,    -1, { "A[68]-9###",               4 }, "A-Series (Bristol Ridge)",  "GF 28SHP" },
	{ 15,  5, -1, 21,  101,  -1,    -1,    -1, { "A1[02] PRO-9###",          6 }, "A-Series (Bristol Ridge)",  "GF 28SHP" },
	{ 15,  5, -1, 21,  101,  -1,    -1,    -1, { "A1[02]-9###",              4 }, "A-Series (Bristol Ridge)",  "GF 28SHP" },
	{ 15,  0, -1, 21,  112,   2,    -1,    -1, { "A[469]-9###",              4 }, "A-Series (Stoney Ridge)",   "GF 28SHP" },
	{ 15,  0, -1, 21,  112,  -1,    -1,    -1, { "E2-9###",                  4 }, "E-Series (Stoney Ridge)",   "GF 28SHP" },
	{ 15,  0, -1, 21,   96,  -1,    -1,    -1, { "Opteron(tm) X3###",        6 }, "Opteron (Toronto)",         "GF 28SHP" },

	/* Family 16h: Jaguar Architecture (2013) */
	{ 15,  0, -1, 22,    0,   4,    -1,    -1, { "Athlon(tm) X4 5##",        6 }, "Athlon X4 (Kabini)",  "TSMC N28" },
	{ 15,  0, -1, 22,    0,   4,    -1,    -1, { "Athlon(tm) 5###",          4 }, "Athlon X4 (Kabini)",  "TSMC N28" },
	{ 15,  0, -1, 22,    0,  -1,    -1,    -1, { "Sempron(tm) [23]###",      4 }, "Sempron (Kabini)",    "TSMC N28" },
	{ 15,  0, -1, 22,    0,  -1,    -1,    -1, { "E1-2###",                  4 }, "E-Series (Kabini)",   "TSMC N28" },
	{ 15,  0, -1, 22,    0,  -1,    -1,    -1, { "E2-3###",                  4 }, "E-Series (Kabini)",   "TSMC N28" },
	{ 15,  0, -1, 22,    0,  -1,    -1,    -1, { "A4 PRO-3###",              6 }, "A-Series (Kabini)",   "TSMC N28" },
	{ 15,  0, -1, 22,    0,  -1,    -1,    -1, { "A[46]-5###",               4 }, "A-Series (Kabini)",   "TSMC N28" },
	/* 2nd-gen, Puma core (2013): */
	{ 15,  0, -1, 22,   48,   2,    -1,    -1, { "E1 Micro-62##T",           8 }, "E-Series (Mullins)",       "GF 28SHP" },
	{ 15,  0, -1, 22,   48,   4,    -1,    -1, { "A4 Micro-64##T",           8 }, "A-Series (Mullins)",       "GF 28SHP" },
	{ 15,  0, -1, 22,   48,   4,    -1,    -1, { "A10 Micro-67##T",          8 }, "A-Series (Mullins)",       "GF 28SHP" },
	{ 15,  0,  1, 22,   48,  -1,    -1,    -1, { "E[12]-6###",               4 }, "E-Series (Beema)",         "GF 28SHP" },
	{ 15,  0,  1, 22,   48,  -1,    -1,    -1, { "A[468]-6###",              4 }, "A-Series (Beema)",         "GF 28SHP" },
	{ 15,  0,  1, 22,   48,  -1,    -1,    -1, { "GX-###",                   4 }, "G-Series (Steppe Eagle)",  "GF 28SHP" },

	/* Family 17h */
	/* Zen (2017) => https://en.wikichip.org/wiki/amd/microarchitectures/zen */
	{ 15, -1, -1, 23,    1,  -1,    -1,    -1, { "EPYC 7##1",              4 }, "EPYC (Naples)",                  "GF 14LP" },
	{ 15, -1, -1, 23,    1,  -1,    -1,    -1, { "Threadripper 1###",      4 }, "Threadripper (Whitehaven)",      "GF 14LP" },
	{ 15, -1, -1, 23,    1,  -1,    -1,    -1, { "Ryzen 7 1###",           6 }, "Ryzen 7 (Summit Ridge)",         "GF 14LP" },
	{ 15, -1, -1, 23,    1,  -1,    -1,    -1, { "Ryzen 5 1###",           6 }, "Ryzen 5 (Summit Ridge)",         "GF 14LP" },
	{ 15, -1, -1, 23,    1,  -1,    -1,    -1, { "Ryzen 3 1###",           6 }, "Ryzen 3 (Summit Ridge)",         "GF 14LP" },
	{ 15, -1, -1, 23,   17,  -1,    -1,    -1, { "Ryzen 7 2###",           6 }, "Ryzen 7 (Raven Ridge)",          "GF 14LP" },
	{ 15, -1, -1, 23,   17,  -1,    -1,    -1, { "Ryzen 5 2###",           6 }, "Ryzen 5 (Raven Ridge)",          "GF 14LP" },
	{ 15, -1, -1, 23,   17,  -1,    -1,    -1, { "Ryzen 3 2###",           6 }, "Ryzen 3 (Raven Ridge)",          "GF 14LP" },
	{ 15, -1, -1, 23,   17,  -1,    -1,    -1, { "Athlon",                 2 }, "Athlon (Raven Ridge)",           "GF 14LP" },
	{ 15, -1, -1, 23,   32,  -1,    -1,    -1, { "Ryzen 3 3###",           6 }, "Ryzen 3 (Dali)",                 "GF 14LP" },
	{ 15, -1, -1, 23,   32,  -1,    -1,    -1, { "Athlon",                 2 }, "Athlon (Dali)",                  "GF 14LP" },
	{ 15, -1,  1, 23,   32,  -1,    -1,    -1, { "",                       0 }, "Dali",                           "GF 14LP" }, /* AMD 3020e */
	/* Zen+ (2018) => https://en.wikichip.org/wiki/amd/microarchitectures/zen%2B */
	{ 15, -1, -1, 23,    8,  -1,    -1,    -1, { "Threadripper 2###",      4 }, "Threadripper (Colfax)",          "GF 12LP" },
	{ 15, -1, -1, 23,    8,  -1,    -1,    -1, { "Ryzen 7 2###",           6 }, "Ryzen 7 (Pinnacle Ridge)",       "GF 12LP" },
	{ 15, -1, -1, 23,    8,  -1,    -1,    -1, { "Ryzen 5 2###",           6 }, "Ryzen 5 (Pinnacle Ridge)",       "GF 12LP" },
	{ 15, -1, -1, 23,    8,  -1,    -1,    -1, { "Ryzen 3 2###",           6 }, "Ryzen 3 (Pinnacle Ridge)",       "GF 12LP" },
	{ 15, -1, -1, 23,   24,  -1,    -1,    -1, { "Ryzen 7 3###",           6 }, "Ryzen 7 (Picasso)",              "GF 12LP" },
	{ 15, -1, -1, 23,   24,  -1,    -1,    -1, { "Ryzen 5 3###",           6 }, "Ryzen 5 (Picasso)",              "GF 12LP" },
	{ 15, -1, -1, 23,   24,  -1,    -1,    -1, { "Ryzen 3 3###",           6 }, "Ryzen 3 (Picasso)",              "GF 12LP" },
	{ 15, -1, -1, 23,   24,  -1,    -1,    -1, { "Athlon",                 2 }, "Athlon (Picasso)",               "GF 12LP" },
	/* Zen 2 (2019) => https://en.wikichip.org/wiki/amd/microarchitectures/zen_2 */
	{ 15, -1, -1, 23,   49,  -1,    -1,    -1, { "EPYC 7##2",              4 }, "EPYC (Rome)",                    "TSMC N7FF" },
	{ 15, -1, -1, 23,   49,  -1,    -1,    -1, { "Threadripper 3###",      4 }, "Threadripper (Castle Peak)",     "TSMC N7FF" },
	{ 15, -1, -1, 23,  113,  -1,    -1,    -1, { "Ryzen 9 3###",           6 }, "Ryzen 9 (Matisse)",              "TSMC N7FF" },
	{ 15, -1, -1, 23,  113,  -1,    -1,    -1, { "Ryzen 7 3###",           6 }, "Ryzen 7 (Matisse)",              "TSMC N7FF" },
	{ 15, -1, -1, 23,  113,  -1,    -1,    -1, { "Ryzen 5 3###",           6 }, "Ryzen 5 (Matisse)",              "TSMC N7FF" },
	{ 15, -1, -1, 23,  113,  -1,    -1,    -1, { "Ryzen 3 3###",           6 }, "Ryzen 3 (Matisse)",              "TSMC N7FF" },
	{ 15, -1, -1, 23,   96,  -1,    -1,    -1, { "Ryzen 9 4###",           6 }, "Ryzen 9 (Renoir)",               "TSMC N7FF" },
	{ 15, -1, -1, 23,   96,  -1,    -1,    -1, { "Ryzen 7 4###",           6 }, "Ryzen 7 (Renoir)",               "TSMC N7FF" },
	{ 15, -1, -1, 23,   96,  -1,    -1,    -1, { "Ryzen 5 4###",           6 }, "Ryzen 5 (Renoir)",               "TSMC N7FF" },
	{ 15, -1, -1, 23,   96,  -1,    -1,    -1, { "Ryzen 3 4###",           6 }, "Ryzen 3 (Renoir)",               "TSMC N7FF" },
	{ 15, -1, -1, 23,  104,  -1,    -1,    -1, { "Ryzen 7 5###",           6 }, "Ryzen 7 (Lucienne)",             "TSMC N7FF" },
	{ 15, -1, -1, 23,  104,  -1,    -1,    -1, { "Ryzen 5 5###",           6 }, "Ryzen 5 (Lucienne)",             "TSMC N7FF" },
	{ 15, -1, -1, 23,  104,  -1,    -1,    -1, { "Ryzen 3 5###",           6 }, "Ryzen 3 (Lucienne)",             "TSMC N7FF" },
	{ 15, -1, -1, 23,   71,  -1,    -1,    -1, { "Desktop Kit",            4 }, "Desktop Kit (Zen 2)",            "TSMC N7FF" }, /* 4700S Desktop Kit */
	{ 15, -1, -1, 23,  132,  -1,    -1,    -1, { "Desktop Kit",            4 }, "Desktop Kit (Zen 2)",            "TSMC N7FF" }, /* 4800S Desktop Kit */
	{ 15, -1,  2, 23,  144,  -1,    -1,    -1, { "Custom APU",             4 }, "Van Gogh",                       "TSMC N7FF" }, /* Custom APU 0405 */
	{ 15, -1,  0, 23,  145,  -1,    -1,    -1, { "Custom APU",             4 }, "Van Gogh",                       "TSMC N7FF" }, /* Custom APU 0932 */
	{ 15, -1, -1, 23,  160,  -1,    -1,    -1, { "Ryzen 5 7###",           6 }, "Ryzen 5 (Mendocino)",            "TSMC N6"   },
	{ 15, -1, -1, 23,  160,  -1,    -1,    -1, { "Ryzen 3 7###",           6 }, "Ryzen 3 (Mendocino)",            "TSMC N6"   },
	{ 15, -1, -1, 23,  160,  -1,    -1,    -1, { "Athlon",                 2 }, "Athlon (Mendocino)",             "TSMC N6"   },

	/* Family 18h */
	/* Zen Architecture for Hygon (2018) => https://en.wikichip.org/wiki/hygon/microarchitectures/dhyana */
	{ 15, -1, -1, 24,    0,  -1,    -1,    -1, { "C86",                    2 }, "C86 (Dhyana)",                   UNKN_STR },

	/* Family 19h */
	/* Zen 3 (2020) => https://en.wikichip.org/wiki/amd/microarchitectures/zen_3 */
	{ 15, -1, -1, 25,    1,  -1,    -1,    -1, { "EPYC 7##3",              4 }, "EPYC (Milan)",                   "TSMC N7FF" },
	{ 15, -1, -1, 25,    8,  -1,    -1,    -1, { "Threadripper 5###",      4 }, "Threadripper (Chagall)",         "TSMC N7FF" },
	{ 15, -1, -1, 25,   33,  -1,    -1,    -1, { "Ryzen 9 5###",           6 }, "Ryzen 9 (Vermeer)",              "TSMC N7FF" },
	{ 15, -1, -1, 25,   33,  -1,    -1,    -1, { "Ryzen 7 5###",           6 }, "Ryzen 7 (Vermeer)",              "TSMC N7FF" },
	{ 15, -1, -1, 25,   33,  -1,    -1,    -1, { "Ryzen 5 5###",           6 }, "Ryzen 5 (Vermeer)",              "TSMC N7FF" },
	{ 15, -1, -1, 25,   33,  -1,    -1,    -1, { "Ryzen 3 5###",           6 }, "Ryzen 3 (Vermeer)",              "TSMC N7FF" },
	{ 15, -1, -1, 25,   80,  -1,    -1,    -1, { "Ryzen 9 5##0[HU]",       8 }, "Ryzen 9 (Cezanne)",              "TSMC N7FF" },
	{ 15, -1, -1, 25,   80,  -1,    -1,    -1, { "Ryzen 7 5##0[HU]",       8 }, "Ryzen 7 (Cezanne)",              "TSMC N7FF" },
	{ 15, -1, -1, 25,   80,  -1,    -1,    -1, { "Ryzen 5 5##0[HU]",       8 }, "Ryzen 5 (Cezanne)",              "TSMC N7FF" },
	{ 15, -1, -1, 25,   80,  -1,    -1,    -1, { "Ryzen 3 5##0[HU]",       8 }, "Ryzen 3 (Cezanne)",              "TSMC N7FF" },
	{ 15, -1, -1, 25,   80,  -1,    -1,    -1, { "Ryzen 7 5##5U",          8 }, "Ryzen 7 (Barceló)",              "TSMC N7FF" }, /* Ryzen 7 5825U */
	{ 15, -1, -1, 25,   80,  -1,    -1,    -1, { "Ryzen 5 5##5U",          8 }, "Ryzen 5 (Barceló)",              "TSMC N7FF" }, /* Ryzen 5 5625U */
	{ 15, -1, -1, 25,   80,  -1,    -1,    -1, { "Ryzen 3 5##5C",          8 }, "Ryzen 3 (Barceló)",              "TSMC N7FF" }, /* Ryzen 3 5125C */
	/* Zen 3+ (2022) */
	{ 15, -1, -1, 25,   68,  -1,    -1,    -1, { "Ryzen 9 6###",           6 }, "Ryzen 9 (Rembrandt)",            "TSMC N6"  },
	{ 15, -1, -1, 25,   68,  -1,    -1,    -1, { "Ryzen 7 6###",           6 }, "Ryzen 7 (Rembrandt)",            "TSMC N6"  },
	{ 15, -1, -1, 25,   68,  -1,    -1,    -1, { "Ryzen 5 6###",           6 }, "Ryzen 5 (Rembrandt)",            "TSMC N6"  },
	{ 15, -1, -1, 25,   68,  -1,    -1,    -1, { "Ryzen 3 6###",           6 }, "Ryzen 3 (Rembrandt)",            "TSMC N6"  },
	{ 15, -1, -1, 25,   68,  -1,    -1,    -1, { "Ryzen 7 7###",           6 }, "Ryzen 7 (Rembrandt-R)",          "TSMC N6"  },
	{ 15, -1, -1, 25,   68,  -1,    -1,    -1, { "Ryzen 5 7###",           6 }, "Ryzen 5 (Rembrandt-R)",          "TSMC N6"  },
	{ 15, -1, -1, 25,   68,  -1,    -1,    -1, { "Ryzen 3 7###",           6 }, "Ryzen 3 (Rembrandt-R)",          "TSMC N6"  },
	/* Zen 4 (2022) => https://en.wikichip.org/wiki/amd/microarchitectures/zen_4 */
	{ 15, -1, -1, 25,   17,  -1,    -1,    -1, { "EPYC 9##4",              4 }, "EPYC (Genoa)",                   "TSMC N5"  },
	{ 15, -1, -1, 25,   24,  -1,    -1,    -1, { "Threadripper 7###",      4 }, "Threadripper (Storm Peak)",      "TSMC N5"  },
	/*  => Raphael (7000 series, Zen 4/RDNA2 based) */
	{ 15, -1,  2, 25,   97,  -1,    -1,    -1, { "Ryzen 9 7###",           6 }, "Ryzen 9 (Raphael)",              "TSMC N5"  },
	{ 15, -1,  2, 25,   97,  -1,    -1,    -1, { "Ryzen 7 7###",           6 }, "Ryzen 7 (Raphael)",              "TSMC N5"  },
	{ 15, -1,  2, 25,   97,  -1,    -1,    -1, { "Ryzen 5 7###",           6 }, "Ryzen 5 (Raphael)",              "TSMC N5"  },
	{ 15, -1,  2, 25,   97,  -1,    -1,    -1, { "Ryzen 3 7###",           6 }, "Ryzen 3 (Raphael)",              "TSMC N5"  },
	/*  => Dragon Range (7045 series, Zen 4/RDNA2 based) */
	{ 15, -1, -1, 25,   97,  -1,    -1,    -1, { "Ryzen 9 7###H",          8 }, "Ryzen 9 (Dragon Range)",         "TSMC N5"  },
	{ 15, -1, -1, 25,   97,  -1,    -1,    -1, { "Ryzen 7 7###H",          8 }, "Ryzen 7 (Dragon Range)",         "TSMC N5"  },
	{ 15, -1, -1, 25,   97,  -1,    -1,    -1, { "Ryzen 5 7###H",          8 }, "Ryzen 5 (Dragon Range)",         "TSMC N5"  },
	/*  => Phoenix (7040 series, Zen 4/RDNA3/XDNA based) */
	{ 15, -1, -1, 25,  116,  -1,    -1,    -1, { "Ryzen 9 7###H",          8 }, "Ryzen 9 (Phoenix)",              "TSMC N4"  },
	{ 15, -1, -1, 25,  116,  -1,    -1,    -1, { "Ryzen 7 7###H",          8 }, "Ryzen 7 (Phoenix)",              "TSMC N4"  },
	{ 15, -1, -1, 25,  116,  -1,    -1,    -1, { "Ryzen 7 7###U",          8 }, "Ryzen 7 (Phoenix)",              "TSMC N4"  },
	{ 15, -1, -1, 25,  116,  -1,    -1,    -1, { "Ryzen 5 7###H",          8 }, "Ryzen 5 (Phoenix)",              "TSMC N4"  },
	{ 15, -1, -1, 25,  116,  -1,    -1,    -1, { "Ryzen 5 7###U",          8 }, "Ryzen 5 (Phoenix)",              "TSMC N4"  },
	{ 15, -1, -1, 25,  116,  -1,    -1,    -1, { "Ryzen 3 7###U",          8 }, "Ryzen 3 (Phoenix)",              "TSMC N4"  },
	{ 15, -1, -1, 25,  116,  -1,    -1,    -1, { "Ryzen Z1",               4 }, "Ryzen Z1 (Phoenix)",             "TSMC N4"  },
	/*  => Phoenix (8000 series, Zen 4 based) */
	{ 15, -1, -1, 25,  117,  -1,    -1,    -1, { "Ryzen 7 8###F",          8 }, "Ryzen 7 (Phoenix)",              "TSMC N4"  },
	{ 15, -1, -1, 25,  117,  -1,    -1,    -1, { "Ryzen 5 8###F",          8 }, "Ryzen 5 (Phoenix)",              "TSMC N4"  },
	/*  => Phoenix (8000 series with Radeon Graphics, Zen 4/RDNA3/XDNA based) */
	{ 15, -1, -1, 25,  117,  -1,    -1,    -1, { "Ryzen 9 8###G",          8 }, "Ryzen 9 (Phoenix)",              "TSMC N4"  },
	{ 15, -1, -1, 25,  117,  -1,    -1,    -1, { "Ryzen 7 8###G",          8 }, "Ryzen 7 (Phoenix)",              "TSMC N4"  },
	{ 15, -1, -1, 25,  117,  -1,    -1,    -1, { "Ryzen 5 8###G",          8 }, "Ryzen 5 (Phoenix)",              "TSMC N4"  },
	{ 15, -1, -1, 25,  117,  -1,    -1,    -1, { "Ryzen 3 8###G",          8 }, "Ryzen 3 (Phoenix)",              "TSMC N4"  },
	/*  => Hawk Point (8040 series, Zen 4/RDNA3/XDNA based) */
	{ 15, -1, -1, 25,  117,  -1,    -1,    -1, { "Ryzen 9 8###H",          8 }, "Ryzen 9 (Hawk Point)",           "TSMC N4"  },
	{ 15, -1, -1, 25,  117,  -1,    -1,    -1, { "Ryzen 7 8###H",          8 }, "Ryzen 7 (Hawk Point)",           "TSMC N4"  },
	{ 15, -1, -1, 25,  117,  -1,    -1,    -1, { "Ryzen 7 8###U",          8 }, "Ryzen 7 (Hawk Point)",           "TSMC N4"  },
	{ 15, -1, -1, 25,  117,  -1,    -1,    -1, { "Ryzen 5 8###H",          8 }, "Ryzen 5 (Hawk Point)",           "TSMC N4"  },
	{ 15, -1, -1, 25,  117,  -1,    -1,    -1, { "Ryzen 5 8###U",          8 }, "Ryzen 5 (Hawk Point)",           "TSMC N4"  },
	{ 15, -1, -1, 25,  117,  -1,    -1,    -1, { "Ryzen 3 8###U",          8 }, "Ryzen 3 (Hawk Point)",           "TSMC N4"  },
	/* Zen 5 (2024) => https://en.wikichip.org/wiki/amd/microarchitectures/zen_5 */
	{ 15, -1, -1, 26,    2,  -1,    -1,    -1, { "EPYC 9##5",              4 }, "EPYC (Turin)",                   "TSMC N4X" },
	{ 15, -1, -1, 26,   17,  -1,    -1,    -1, { "EPYC 9##5",              4 }, "EPYC (Turin Dense)",             "TSMC N3E" },
	/*  => Granite Ridge (9000 series, Zen 5 based) */
	{ 15, -1, -1, 26,   68,  -1,    -1,    -1, { "Ryzen 9 9###",           6 }, "Ryzen 9 (Granite Ridge)",        "TSMC N4"  },
	{ 15, -1, -1, 26,   68,  -1,    -1,    -1, { "Ryzen 7 9###",           6 }, "Ryzen 7 (Granite Ridge)",        "TSMC N4"  },
	{ 15, -1, -1, 26,   68,  -1,    -1,    -1, { "Ryzen 5 9###",           6 }, "Ryzen 5 (Granite Ridge)",        "TSMC N4"  },
	{ 15, -1, -1, 26,   68,  -1,    -1,    -1, { "Ryzen 3 9###",           6 }, "Ryzen 3 (Granite Ridge)",        "TSMC N4"  },
	/*  => Strix Point and Krackan Point (Zen 5/RDNA3.5/XDNA2 based) */
	{ 15, -1, -1, 26,   36,  -1,    -1,    -1, { "Ryzen AI 9",             6 }, "Ryzen AI 9 (Strix Point)",       "TSMC N4P" },
	{ 15, -1, -1, 26,   36,  -1,    -1,    -1, { "Ryzen AI 7",             6 }, "Ryzen AI 7 (Strix Point)",       "TSMC N4P" }, /* Ryzen AI 7 PRO 360 */
	{ 15, -1, -1, 26,   96,  -1,    -1,    -1, { "Ryzen AI 7",             6 }, "Ryzen AI 7 (Krackan Point)",     "TSMC N4P" }, /* Ryzen AI 7 (PRO) 350 */
	{ 15, -1, -1, 26,   96,  -1,    -1,    -1, { "Ryzen AI 5",             6 }, "Ryzen AI 5 (Krackan Point)",     "TSMC N4P" }, /* Ryzen AI 5 (PRO) 340 */
	/* => Strix Halo (Zen 5/RDNA3.5/XDNA2 based) */
	{ 15, -1, -1, 26,  112,  -1,    -1,    -1, { "Ryzen AI MAX+",          8 }, "Ryzen AI MAX+ (Strix Halo)",     "TSMC N4P" }, /* Ryzen AI MAX+ (PRO) 395 */
	{ 15, -1, -1, 26,  112,  -1,    -1,    -1, { "Ryzen AI MAX",           6 }, "Ryzen AI MAX (Strix Halo)",      "TSMC N4P" },
//     F   M   S  EF    EM #cores  L2$    L3$  Pattern                          Codename                          Technology
};


static void load_amd_features(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	const struct feature_map_t matchtable_edx81[] = {
		{ 20, CPU_FEATURE_NX },
		{ 22, CPU_FEATURE_MMXEXT },
		{ 25, CPU_FEATURE_FXSR_OPT },
		{ 30, CPU_FEATURE_3DNOWEXT },
		{ 31, CPU_FEATURE_3DNOW },
	};
	const struct feature_map_t matchtable_ecx81[] = {
		{  1, CPU_FEATURE_CMP_LEGACY },
		{  2, CPU_FEATURE_SVM },
		/* id 5 is handled in common */
		{  6, CPU_FEATURE_SSE4A },
		{  7, CPU_FEATURE_MISALIGNSSE },
		{  8, CPU_FEATURE_3DNOWPREFETCH },
		{  9, CPU_FEATURE_OSVW },
		{ 10, CPU_FEATURE_IBS },
		{ 11, CPU_FEATURE_XOP },
		{ 12, CPU_FEATURE_SKINIT },
		{ 13, CPU_FEATURE_WDT },
		{ 16, CPU_FEATURE_FMA4 },
		{ 21, CPU_FEATURE_TBM },
	};
	const struct feature_map_t matchtable_edx87[] = {
		{  0, CPU_FEATURE_TS },
		{  1, CPU_FEATURE_FID },
		{  2, CPU_FEATURE_VID },
		{  3, CPU_FEATURE_TTP },
		{  4, CPU_FEATURE_TM_AMD },
		{  5, CPU_FEATURE_STC },
		{  6, CPU_FEATURE_100MHZSTEPS },
		{  7, CPU_FEATURE_HWPSTATE },
		/* id 8 is handled in common */
		{  9, CPU_FEATURE_CPB },
		{ 10, CPU_FEATURE_APERFMPERF },
		{ 11, CPU_FEATURE_PFI },
		{ 12, CPU_FEATURE_PA },
	};
	if (raw->ext_cpuid[0][EAX] >= 0x80000001) {
		match_features(matchtable_edx81, COUNT_OF(matchtable_edx81), raw->ext_cpuid[1][EDX], data);
		match_features(matchtable_ecx81, COUNT_OF(matchtable_ecx81), raw->ext_cpuid[1][ECX], data);
	}
	if (raw->ext_cpuid[0][EAX] >= 0x80000007)
		match_features(matchtable_edx87, COUNT_OF(matchtable_edx87), raw->ext_cpuid[7][EDX], data);
	if (raw->ext_cpuid[0][EAX] >= 0x8000001a) {
		/* We have the extended info about SSE unit size
		Extracted from BKDG, about CPUID_Fn8000001A_EAX [Performance Optimization Identifiers] (Core::X86::Cpuid::PerfOptId):
		- bit 2: FP256
		- bit 1: MOVU
		- bit 0: FP128 */
		data->detection_hints[CPU_HINT_SSE_SIZE_AUTH] = 1;
		if ((raw->ext_cpuid[0x1a][EAX] >> 2) & 1)
			data->x86.sse_size = 256;
		else if ((raw->ext_cpuid[0x1a][EAX]) & 1)
			data->x86.sse_size = 128;
		else
			data->x86.sse_size = 64;
	}
}

static void decode_amd_cache_info(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int l3_result, l3_assoc;
	const int assoc_table[16] = {
		0, 1, 2, 0, 4, 0, 8, 0, 16, 16, 32, 48, 64, 96, 128, 255
	};
	unsigned n = raw->ext_cpuid[0][EAX];

	if (n >= 0x80000005) {
		/* L1 Data Cache */
		data->l1_data_cache     = EXTRACTS_BITS(raw->ext_cpuid[5][ECX], 31, 24); // L1DcSize
		data->l1_data_assoc     = EXTRACTS_BITS(raw->ext_cpuid[5][ECX], 23, 16); // L1DcAssoc
		data->l1_data_cacheline = EXTRACTS_BITS(raw->ext_cpuid[5][ECX],  7,  0); // L1DcLineSize

		/* L1 Instruction Cache */
		data->l1_instruction_cache     = EXTRACTS_BITS(raw->ext_cpuid[5][EDX], 31, 24); // L1IcSize
		data->l1_instruction_assoc     = EXTRACTS_BITS(raw->ext_cpuid[5][EDX], 23, 16); // L1IcAssoc
		data->l1_instruction_cacheline = EXTRACTS_BITS(raw->ext_cpuid[5][EDX],  7,  0); // L1IcLineSize
	}
	if (n >= 0x80000006) {
		data->l2_cache = (raw->ext_cpuid[6][ECX] >> 16) & 0xffff;
		data->l2_assoc = assoc_table[(raw->ext_cpuid[6][ECX] >> 12) & 0xf];
		data->l2_cacheline = (raw->ext_cpuid[6][ECX]) & 0xff;

		l3_result = (raw->ext_cpuid[6][EDX] >> 18);
		if (l3_result > 0) {
			l3_result *= 512; /* AMD spec says it's a range, but we take the lower bound */
			l3_assoc = (raw->ext_cpuid[6][EDX] >> 12) & 0xf;
			data->l3_cache = l3_result;
			data->l3_assoc = assoc_table[l3_assoc];
			data->l3_cacheline = (raw->ext_cpuid[6][EDX]) & 0xff;
		} else {
			data->l3_cache = -1;
		}
	}
}

static void decode_amd_number_of_cores(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int logical_cpus = -1, num_cores = -1;

	if (raw->basic_cpuid[0][EAX] >= 1) {
		logical_cpus = (raw->basic_cpuid[1][EBX] >> 16) & 0xff;
		if (raw->ext_cpuid[0][EAX] >= 8) {
			num_cores = 1 + (raw->ext_cpuid[8][ECX] & 0xff);
		}
	}
	if (data->flags[CPU_FEATURE_HT]) {
		if (num_cores > 1) {
			if ((data->x86.ext_family >= 23) && (raw->ext_cpuid[0][EAX] >= 30))
				/* Ryzen 3 has SMT flag, but in fact cores count is equal to threads count.
				Ryzen 5/7 reports twice as many "real" cores (e.g. 16 cores instead of 8) because of SMT. */
				/* On PPR 17h, page 82:
				CPUID_Fn8000001E_EBX [Core Identifiers][15:8] is ThreadsPerCore
				ThreadsPerCore: [...] The number of threads per core is ThreadsPerCore+1 */
				num_cores /= ((raw->ext_cpuid[30][EBX] >> 8) & 0xff) + 1;
			data->num_cores = num_cores;
			data->num_logical_cpus = logical_cpus;
		} else {
			data->num_cores = 1;
			data->num_logical_cpus = (logical_cpus >= 2 ? logical_cpus : 2);
		}
	} else {
		data->num_cores = data->num_logical_cpus = 1;
	}
}

int cpuid_identify_amd(struct cpu_raw_data_t* raw, struct cpu_id_t* data, struct internal_id_info_t* internal)
{
	load_amd_features(raw, data);
	if ((EXTRACTS_BIT(raw->ext_cpuid[1][ECX], 22) == 1) && (EXTRACTS_BITS(raw->amd_fn8000001dh[0][EAX], 4, 0) != 0)) /* TopologyExtensions supported */
		decode_deterministic_cache_info_x86(raw->amd_fn8000001dh, MAX_AMDFN8000001DH_LEVEL, data, internal);
	else
		decode_amd_cache_info(raw, data);
	decode_amd_number_of_cores(raw, data);
	decode_architecture_version_x86(data);
	data->purpose = cpuid_identify_purpose_amd(raw);
	internal->score = match_cpu_codename(cpudb_amd, COUNT_OF(cpudb_amd), data);

	return 0;
}

void cpuid_get_list_amd(struct cpu_list_t* list)
{
	generic_get_cpu_list(cpudb_amd, COUNT_OF(cpudb_amd), list);
}

cpu_purpose_t cpuid_identify_purpose_amd(struct cpu_raw_data_t* raw)
{
	int i;

	/* Check if Extended CPU Topology is supported */
	if (raw->amd_fn80000026h[0][EAX] == 0x0)
		return PURPOSE_GENERAL;

	/* Check for heterogeneous cores
	From AMD64 Architecture Programmer’s Manual - Volume 3: General-Purpose and System Instructions
	Available at https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24594.pdf

	- CPUID_Fn80000026_EAX [Extended CPU Topology][30] is HeterogeneousCores.
	  Set to 1 if all components at the current hierarchy level do not consist of the cores that report the same core type (CoreType).

	- CPUID_Fn80000026_ECX [Extended CPU Topology][15:8] is LevelType.
	  LevelType 01h is Core.

	- CPUID_Fn80000026_EBX [Extended CPU Topology][31:28] is CoreType.
	  Only valid while LevelType=Core.
	*/
	for (i = 0; (raw->amd_fn80000026h[i][EBX] != 0x0) && (raw->amd_fn80000026h[i][ECX] != 0x0) && (i < MAX_AMDFN80000026H_LEVEL); i++) {
		if ((EXTRACTS_BIT(raw->amd_fn80000026h[i][EAX], 30) == 0x1) && (EXTRACTS_BITS(raw->amd_fn80000026h[i][ECX], 15, 8) == 0x1)) {
			debugf(3, "Detected AMD CPU with heterogeneous cores\n");
			switch (EXTRACTS_BITS(raw->amd_fn80000026h[i][EBX], 31, 28)) {
				case 0x0: return PURPOSE_PERFORMANCE;
				case 0x1: return PURPOSE_EFFICIENCY;
				default:  return PURPOSE_GENERAL;
			}
		}
	}

	return PURPOSE_GENERAL;
}
