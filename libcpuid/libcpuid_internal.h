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

#define LBIT(x) ((1ULL) << x)

// common detection bits for CPUs:
#define _M_                     LBIT(  0 )
#define MOBILE_                 LBIT(  1 )
#define _MP_                    LBIT(  2 )
#define _3                      LBIT(  3 )
#define _5                      LBIT(  4 )
#define _7                      LBIT(  5 )
#define _9                      LBIT(  6 )
#define _H                      LBIT(  7 ) // powerful mobile processors for laptop
#define _S                      LBIT(  8 )
#define _U                      LBIT(  9 ) // ultra-low power
#define _X                      LBIT( 10 ) // CPU with great amount of power
#define _F                      LBIT( 11 ) // CPU that doesn’t have integrated graphics
#define _G                      LBIT( 12 ) // CPU with additional built-in integrated graphics
#define _E                      LBIT( 13 ) // Embedded (Intel)
#define LAST_COMMON_BIT               13

// additional detection bits for Intel CPUs:
#define PENTIUM_                LBIT( (LAST_COMMON_BIT +  1) )
#define CELERON_                LBIT( (LAST_COMMON_BIT +  2) )
#define CORE_                   LBIT( (LAST_COMMON_BIT +  3) )
#define _I_                     LBIT( (LAST_COMMON_BIT +  4) )
#define XEON_                   LBIT( (LAST_COMMON_BIT +  5) )
#define ATOM_                   LBIT( (LAST_COMMON_BIT +  6) )
#define _K                      LBIT( (LAST_COMMON_BIT +  7) ) // an unlocked desktop processor that allows for overclocking
#define _P                      LBIT( (LAST_COMMON_BIT +  8) )
#define _N                      LBIT( (LAST_COMMON_BIT +  9) )
#define _W_                     LBIT( (LAST_COMMON_BIT + 10) )
#define _D_                     LBIT( (LAST_COMMON_BIT + 11) )
#define _BRONZE_                LBIT( (LAST_COMMON_BIT + 12) )
#define _SILVER_                LBIT( (LAST_COMMON_BIT + 13) )
#define _GOLD_                  LBIT( (LAST_COMMON_BIT + 14) )
#define _PLATINIUM_             LBIT( (LAST_COMMON_BIT + 15) )
#define _MAX_                   LBIT( (LAST_COMMON_BIT + 16) )
#define _J_                     LBIT( (LAST_COMMON_BIT + 17) )
#define _N_                     LBIT( (LAST_COMMON_BIT + 18) )
#define _ULTRA_                 LBIT( (LAST_COMMON_BIT + 19) )
#define _V                      LBIT( (LAST_COMMON_BIT + 20) ) // Lunar Lake
#define _L                      LBIT( (LAST_COMMON_BIT + 21) ) // LGA package (UL = Power efficient, in LGA package / HL = Highest performance, in LGA package)
#define _T                      LBIT( (LAST_COMMON_BIT + 22) ) // Power-optimized lifestyle
#define _U_                     LBIT( (LAST_COMMON_BIT + 23) )

// additional detection bits for AMD CPUs:
#define ATHLON_      LBIT( (LAST_COMMON_BIT +  1) )
#define _XP_         LBIT( (LAST_COMMON_BIT +  2) )
#define DURON_       LBIT( (LAST_COMMON_BIT +  3) )
#define SEMPRON_     LBIT( (LAST_COMMON_BIT +  4) )
#define OPTERON_     LBIT( (LAST_COMMON_BIT +  5) )
#define TURION_      LBIT( (LAST_COMMON_BIT +  6) )
#define RYZEN_       LBIT( (LAST_COMMON_BIT +  7) )
#define RYZEN_TR_    LBIT( (LAST_COMMON_BIT +  8) )
#define EPYC_        LBIT( (LAST_COMMON_BIT +  9) )
#define _LV_         LBIT( (LAST_COMMON_BIT + 10) )
#define _64_         LBIT( (LAST_COMMON_BIT + 11) )
#define _X2          LBIT( (LAST_COMMON_BIT + 12) )
#define _X3          LBIT( (LAST_COMMON_BIT + 13) )
#define _X4          LBIT( (LAST_COMMON_BIT + 14) )
#define _X6          LBIT( (LAST_COMMON_BIT + 15) )
#define _FX          LBIT( (LAST_COMMON_BIT + 16) )
#define _APU_        LBIT( (LAST_COMMON_BIT + 17) )
#define C86_	     LBIT( (LAST_COMMON_BIT + 18) )
#define _Z           LBIT( (LAST_COMMON_BIT + 19) )
#define _AI_         LBIT( (LAST_COMMON_BIT + 20) )

// additional detection bits for Via CPUs:
#define SAMUEL_            LBIT( (LAST_COMMON_BIT +  1) )
#define EZRA_              LBIT( (LAST_COMMON_BIT +  2) )
#define NEHEMIAH_          LBIT( (LAST_COMMON_BIT +  3) )
#define ESTHER_            LBIT( (LAST_COMMON_BIT +  4) )
#define EDEN_              LBIT( (LAST_COMMON_BIT +  5) )
#define CNA_               LBIT( (LAST_COMMON_BIT +  6) )
#define NANO_              LBIT( (LAST_COMMON_BIT +  7) )
#define QUADCORE_          LBIT( (LAST_COMMON_BIT +  8) )

// additional detection bits for Zhaoxin CPUs:
#define KAISHENG_          LBIT( (LAST_COMMON_BIT +  1) )
#define KAIXIAN_           LBIT( (LAST_COMMON_BIT +  2) )
#define _KH_               LBIT( (LAST_COMMON_BIT +  3) )
#define _KX_               LBIT( (LAST_COMMON_BIT +  4) )
#define _ZX_               LBIT( (LAST_COMMON_BIT +  5) )
#define __C                LBIT( (LAST_COMMON_BIT +  6) )
#define _D                 LBIT( (LAST_COMMON_BIT +  7) )


int cpu_ident_internal(struct cpu_raw_data_t* raw, struct cpu_id_t* data,
		       struct internal_id_info_t* internal);

#endif /* __LIBCPUID_INTERNAL_H__ */
