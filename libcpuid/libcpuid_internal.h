/*
 * Copyright 2016  Veselin Georgiev,
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
#ifndef __LIBCPUID_INTERNAL_H__
#define __LIBCPUID_INTERNAL_H__
/*
 * This file contains internal undocumented declarations and function prototypes
 * for the workings of the internal library infrastructure.
 */

#define EXTRACTS_BIT(reg, bit)              ((reg >> bit)    & 0x1)
#define EXTRACTS_BITS(reg, highbit, lowbit) ((reg >> lowbit) & ((1ULL << (highbit - lowbit + 1)) - 1))

enum _common_codes_t {
	NA = 0,
	NC, /* No code */
};

enum _cache_type_t {
	L1I,
	L1D,
	L2,
	L3,
	L4,
	NUM_CACHE_TYPES
};
typedef enum _cache_type_t cache_type_t;

#define CODE(x) x
#define CODE2(x, y) x = y
enum _amd_code_t {
	#include "amd_code_t.h"
};
typedef enum _amd_code_t amd_code_t;

enum _centaur_code_t {
	#include "centaur_code_t.h"
};
typedef enum _centaur_code_t centaur_code_t;

enum _intel_code_t {
	#include "intel_code_t.h"
};
typedef enum _intel_code_t intel_code_t;
#undef CODE
#undef CODE2

struct internal_id_info_t {
	union {
		amd_code_t     amd;
		centaur_code_t centaur;
		intel_code_t   intel;
	} code;
	uint64_t bits;
	int score; // detection (matchtable) score
	int32_t cache_mask[NUM_CACHE_TYPES];
};

struct internal_topology_t {
	int32_t apic_id;
	int32_t package_id;
	int32_t core_id;
	int32_t smt_id;
	int32_t cache_id[NUM_CACHE_TYPES];
	logical_cpu_t logical_cpu;
};

struct internal_core_id_t {
	logical_cpu_t num_logical_cpu;
	int32_t core_id;
};

struct internal_cache_id_t {
	logical_cpu_t num_logical_cpu;
	int32_t cache_id;
};

#define CORES_HTABLE_SIZE 256
struct internal_core_instances_t {
	uint8_t instances;
	struct internal_core_id_t htable[CORES_HTABLE_SIZE];
};

#define CACHES_HTABLE_SIZE 256
struct internal_cache_instances_t {
	uint8_t instances[NUM_CACHE_TYPES];
	struct internal_cache_id_t htable[NUM_CACHE_TYPES][CACHES_HTABLE_SIZE];
};

struct internal_type_info_t {
	cpu_purpose_t purpose;
	int32_t package_id;
	struct internal_id_info_t id_info;
	struct internal_core_instances_t core_instances;
	struct internal_cache_instances_t cache_instances;
};

struct internal_type_info_array_t {
	uint8_t num;
	struct internal_type_info_t* data;
};

#define LBIT(x) (((long long) 1) << x)

enum _common_bits_t {
	_M_                     = LBIT(  0 ),
	MOBILE_                 = LBIT(  1 ),
	_MP_                    = LBIT(  2 ),
	_3                      = LBIT(  3 ),
	_5                      = LBIT(  4 ),
	_7                      = LBIT(  5 ),
	_9                      = LBIT(  6 ),
	_H                      = LBIT(  7 ), // powerful mobile processors for laptop
	_S                      = LBIT(  8 ),
	_U                      = LBIT(  9 ), // ultra-low power
	_X                      = LBIT( 10 ), // CPU with great amount of power
	_F                      = LBIT( 11 ), // CPU that doesn’t have integrated graphics
	_G                      = LBIT( 12 ), // CPU with additional built-in integrated graphics
};

// additional detection bits for Intel CPUs:
enum _intel_bits_t {
	PENTIUM_                = LBIT( 10 ),
	CELERON_                = LBIT( 11 ),
	CORE_                   = LBIT( 12 ),
	_I_                     = LBIT( 13 ),
	XEON_                   = LBIT( 14 ),
	ATOM_                   = LBIT( 15 ),
	_K                      = LBIT( 16 ), // an unlocked desktop processor that allows for overclocking
	_P                      = LBIT( 17 ),
	_N                      = LBIT( 18 ),
	_W_                     = LBIT( 19 ),
	_D_                     = LBIT( 20 ),
	_BRONZE_                = LBIT( 21 ),
	_SILVER_                = LBIT( 22 ),
	_GOLD_                  = LBIT( 23 ),
	_PLATINIUM_             = LBIT( 24 ),
	_MAX_                   = LBIT( 25 ),
	_J_                     = LBIT( 26 ),
	_N_                     = LBIT( 27 ),
	_ULTRA_                 = LBIT( 28 ),
};
typedef enum _intel_bits_t intel_bits_t;

enum _amd_bits_t {
	ATHLON_      = LBIT( 10 ),
	_XP_         = LBIT( 11 ),
	DURON_       = LBIT( 12 ),
	SEMPRON_     = LBIT( 13 ),
	OPTERON_     = LBIT( 14 ),
	TURION_      = LBIT( 15 ),
	RYZEN_       = LBIT( 16 ),
	RYZEN_TR_    = LBIT( 17 ),
	EPYC_        = LBIT( 18 ),
	_LV_         = LBIT( 19 ),
	_64_         = LBIT( 20 ),
	_X2          = LBIT( 21 ),
	_X3          = LBIT( 22 ),
	_X4          = LBIT( 23 ),
	_X6          = LBIT( 24 ),
	_FX          = LBIT( 25 ),
	_APU_        = LBIT( 26 ),
	C86_	     = LBIT( 27 ),
	_Z           = LBIT( 28 ),
	_AI_         = LBIT( 29 ),
};
typedef enum _amd_bits_t amd_bits_t;

enum _via_bits_t {
	SAMUEL_            = LBIT( 10 ),
	EZRA_              = LBIT( 11 ),
	NEHEMIAH_          = LBIT( 12 ),
	ESTHER_            = LBIT( 13 ),
	EDEN_              = LBIT( 14 ),
	CNA_               = LBIT( 15 ),
	NANO_              = LBIT( 16 ),
	QUADCORE_          = LBIT( 17 ),
};
typedef enum _via_bits_t via_bits_t;

enum _zhaoxin_bits_t {
	KAISHENG_          = LBIT( 10 ),
	KAIXIAN_           = LBIT( 11 ),
	_KH_               = LBIT( 12 ),
	_KX_               = LBIT( 13 ),
	_ZX_               = LBIT( 14 ),
	_C                 = LBIT( 15 ),
	_D                 = LBIT( 16 ),
	_E                 = LBIT( 17 ),
};
typedef enum _zhaoxin_bits_t zhaoxin_bits_t;


int cpu_ident_internal(struct cpu_raw_data_t* raw, struct cpu_id_t* data,
		       struct internal_id_info_t* internal);

#endif /* __LIBCPUID_INTERNAL_H__ */
