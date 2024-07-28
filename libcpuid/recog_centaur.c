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
#include "libcpuid.h"
#include "libcpuid_ctype.h"
#include "libcpuid_util.h"
#include "libcpuid_internal.h"
#include "recog_centaur.h"

const struct centaur_code_str { centaur_code_t code; char *str; } centaur_code_str[] = {
	#define CODE(x) { x, #x }
	#define CODE2(x, y) CODE(x)
	#include "centaur_code_t.h"
	#undef CODE
};

typedef struct {
	int code;
	uint64_t bits;
} centaur_code_and_bits_t;

enum _centaur_model_t {
	UNKNOWN = -1,
	_4000 = 100, /* Zhaoxin KaiXian (KX) / KaisHeng (KH) Zhangjiang */
	_5000, /* Zhaoxin KaiXian (KX) WuDaoKou */
	_6000, /* Zhaoxin KaiXian (KX) LuJiaZui */
	_7000, /* Zhaoxin KaiXian (KX) Yongfeng */
	_20000 = 1000, /* Zhaoxin KaisHeng (KH) WuDaoKou */
	_30000, /* Zhaoxin KaisHeng (KH) LuJiaZui */
	_40000, /* Zhaoxin KaisHeng (KH) Yongfeng */
};
typedef enum _centaur_model_t centaur_model_t;

const struct match_entry_t cpudb_centaur[] = {
//     F   M   S  EF  EM  #cores  L2$   L3$  BC  ModelBits     ModelCode  Name
	{ -1, -1, -1, -1,   -1,  -1,    -1,    -1, NC, 0,                 0, "Unknown Centaur CPU" },
//     F   M   S  EF  EM  #cores  L2$   L3$  BC  ModelBits     ModelCode  Name


	/* VIA */
//     F  M    S   EF  EM  #cores  L2$   L3$    BC   ModelBits   ModelCode  Name
	{  6, -1, -1, -1,   -1,  -1,    -1,    -1, VIA, 0            ,    0, "Unknown VIA CPU"        },
	/* Samuel (2000, 180 nm) */
	{  6,  6, -1, -1,   -1,  -1,    -1,    -1, VIA, SAMUEL_      ,    0, "VIA Cyrix III (Samuel)" },
	/* Samuel 2 (2001, 150 nm) */
	{  6,  7, -1, -1,   -1,  -1,    -1,    -1, VIA, SAMUEL_      ,    0, "VIA C3 (Samuel 2)"      },
	/* Ezra (2001, 130 nm) */
	{  6,  7, -1, -1,   -1,  -1,    -1,    -1, VIA, EZRA_        ,    0, "VIA C3 (Ezra)"          },
	{  6,  8, -1, -1,   -1,  -1,    -1,    -1, VIA, EZRA_        ,    0, "VIA C3 (Ezra-T)"        },
	/* Nehemiah (2003, 130 nm) */
	{  6,  9, -1, -1,   -1,  -1,    -1,    -1, VIA, NEHEMIAH_    ,    0, "VIA C3 (Nehemiah)"      },
	/* Esther (2005, 90 nm) */
	{  6,  10, -1, -1,   -1,  -1,    -1,    -1, VIA, ESTHER_     ,    0, "VIA C7 (Esther)"        },
	{  6,  13, -1, -1,   -1,  -1,    -1,    -1, VIA, ESTHER_     ,    0, "VIA C7-M (Esther)"      },
	/* Isaiah (2008, 65 nm) */
	{  6,  15, -1, -1,   -1,  -1,    -1,    -1, VIA, CNA_        ,    0, "VIA Nano (Isaiah)"      },
	{  6,  15, -1, -1,   -1,   1,    -1,    -1, VIA, NANO_       ,    0, "VIA Nano (Isaiah)"      },
	{  6,  15, -1, -1,   -1,   2,    -1,    -1, VIA, NANO_       ,    0, "VIA Nano X2 (Isaiah)"   },
	{  6,  15, -1, -1,   -1,  -1,    -1,    -1, VIA, QUADCORE_   ,    0, "VIA Nano X4 (Isaiah)"   },
	{  6,  15, -1, -1,   -1,   4,    -1,    -1, VIA, EDEN_       ,    0, "VIA Eden X4 (Isaiah)"   },
//     F  M    S   EF  EM  #cores  L2$   L3$    BC   ModelBits   ModelCode  Name


	/* Zhaoxin */
//     F   M   S  EF  EM  #cores  L2$   L3$  BC  ModelBits              ModelCode  Name
	{  7, -1, -1, -1,   -1,  -1,    -1,    -1, ZHAOXIN, 0                ,      0, "Unknown Zhaoxin CPU"           },
	/* Zhangjiang (2015, 28 nm) */
	{  7, -1, -1, -1,   15,  -1,    -1,    -1, ZHAOXIN, KAISHENG_|_KH_|_C,      0, "Zhaoxin KaisHeng (ZhangJiang)" }, // C+ (4000)
	{  7, -1, -1, -1,   15,  -1,    -1,    -1, ZHAOXIN, KAIXIAN_|_ZX_|_C ,      0, "Zhaoxin KaiXian (ZhangJiang)"  }, // C/C+ (4000)
	/* WuDaoKou (2017, 28 nm) */
	{  7, -1, -1, -1,   27,  -1,    -1,    -1, ZHAOXIN, KAISHENG_|_KH_   , _20000, "Zhaoxin KaisHeng (WuDaoKou)"   }, // KH (20000)
	{  7, -1, -1, -1,   27,  -1,    -1,    -1, ZHAOXIN, KAIXIAN_|_KX_    ,  _5000, "Zhaoxin KaiXian (WuDaoKou)"    }, // KX (5000)
	/* LuJiaZui (2019, 16 nm) */
	{  7, -1, -1, -1,   59,  -1,    -1,    -1, ZHAOXIN, KAISHENG_|_KH_   , _30000, "Zhaoxin KaisHeng (LuJiaZui)"   }, // KH (30000)
	{  7, -1, -1, -1,   59,  -1,    -1,    -1, ZHAOXIN, KAIXIAN_|_KX_    ,  _6000, "Zhaoxin KaiXian (LuJiaZui)"    }, // KX (6000)
	/* Yongfeng (2022, 16 nm) */
	{  7, -1, -1, -1,   91,  -1,    -1,    -1, ZHAOXIN, KAISHENG_|_KH_   , _40000, "Zhaoxin KaisHeng (Yongfeng)"   }, // KH (40000)
	{  7, -1, -1, -1,   91,  -1,    -1,    -1, ZHAOXIN, KAIXIAN_|_KX_    ,  _7000, "Zhaoxin KaiXian (Yongfeng)"    }, // KX (7000)
//     F   M   S  EF  EM  #cores  L2$   L3$  BC  ModelBits              ModelCode  Name
};

static centaur_code_and_bits_t get_brand_code_and_bits(struct cpu_id_t* data)
{
	centaur_code_t code = (centaur_code_t) NC;
	centaur_code_and_bits_t result;
	uint64_t bits = 0;
	int i = 0;

	const char* bs = data->brand_str;
	const struct { centaur_code_t c; const char *search; } code_matchtable[] = {
		{ VIA, "VIA" },
		{ ZHAOXIN, "ZHAOXIN" },
	};

	const struct { uint64_t bit; const char* search; } bit_matchtable_via[] = {
		{ SAMUEL_, "Samuel" },
		{ EZRA_, "Ezra" },
		{ NEHEMIAH_, "Nehemiah" },
		{ ESTHER_, "Esther" },
		{ EDEN_, "Eden" },
		{ CNA_, "CNA" },
		{ NANO_, "Nano" },
		{ QUADCORE_, "QuadCore" },
	};
	const struct { uint64_t bit; const char* search; } bit_matchtable_zhaoxin[] = {
		{ KAISHENG_, "KaisHeng" },
		{ KAIXIAN_, "KaiXian" },
		{ _KH_, "KH" },
		{ _KX_, "KX" },
		{ _ZX_, "ZX" },
		{ _C,   "-C" },
		{ _D,   "-D" },
		{ _E,   "-E" },
	};

	for (i = 0; i < COUNT_OF(code_matchtable); i++) {
		if (match_pattern(bs, code_matchtable[i].search)) {
			code = code_matchtable[i].c;
			break;
		}
	}

	if (code == VIA) {
		for (i = 0; i < COUNT_OF(bit_matchtable_via); i++) {
			if (match_pattern(bs, bit_matchtable_via[i].search))
				bits |= bit_matchtable_via[i].bit;
		}
	}
	else if (code == ZHAOXIN) {
		for (i = 0; i < COUNT_OF(bit_matchtable_zhaoxin); i++) {
			if (match_pattern(bs, bit_matchtable_zhaoxin[i].search))
				bits |= bit_matchtable_zhaoxin[i].bit;
		}
	}

	result.code = code;
	result.bits = bits;
	return result;
}

static centaur_model_t get_model_code(struct cpu_id_t* data, centaur_code_and_bits_t brand)
{
	int i = 0;
	int l = (int) strlen(data->brand_str);
	const char *bs = data->brand_str;

	if (brand.code == ZHAOXIN) {
		if ((i = match_pattern(bs, "KaiSheng KH-")) != 0) {
			i += 11;
			if (i + 4 >= l) return UNKNOWN;
			switch(bs[i]) {
				case '2': return _20000;
				case '3': return _30000;
				case '4': return _40000;
				default: return UNKNOWN;
			}
		}
		else if ((i = match_pattern(bs, "KaiXian KX-")) != 0) {
			i += 10;
			if (bs[i] == 'U') i++;
			if (i + 3 >= l) return UNKNOWN;
			switch(bs[i]) {
				case '4': return _4000;
				case '5': return _5000;
				case '6': return _6000;
				case '7': return _7000;
				default: return UNKNOWN;
			}
		}
	}

	return UNKNOWN;
}

int cpuid_identify_centaur(struct cpu_raw_data_t* raw, struct cpu_id_t* data, struct internal_id_info_t* internal)
{
	centaur_code_and_bits_t brand;
	centaur_model_t model_code;
	int i;
	char* brand_code_str = NULL;

	if (raw->basic_cpuid[0][EAX] >= 4)
		decode_deterministic_cache_info_x86(raw->intel_fn4, MAX_INTELFN4_LEVEL, data, internal);
	decode_number_of_cores_x86(raw, data);
	decode_architecture_version_x86(data);

	brand = get_brand_code_and_bits(data);
	model_code = get_model_code(data, brand);
	for (i = 0; i < COUNT_OF(centaur_code_str); i++) {
		if (brand.code == centaur_code_str[i].code) {
			brand_code_str = centaur_code_str[i].str;
			break;
		}
	}
	if (brand_code_str)
		debugf(2, "Detected Centaur brand code: %d (%s)\n", brand.code, brand_code_str);
	else
		debugf(2, "Detected Centaur brand code: %d\n", brand.code);
	if (brand.bits) {
		debugf(2, "Detected Centaur bits: ");
		debug_print_lbits(2, brand.bits);
	}
	debugf(2, "Detected Centaur model code: %d\n", model_code);

	internal->code.centaur = brand.code;
	internal->bits = brand.bits;
	internal->score = match_cpu_codename(cpudb_centaur, COUNT_OF(cpudb_centaur), data,
		brand.code, brand.bits, model_code);

	return 0;
}
