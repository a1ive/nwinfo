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

enum _cache_type_t {
	L1I,
	L1D,
	L2,
	L3,
	L4,
	NUM_CACHE_TYPES
};
typedef enum _cache_type_t cache_type_t;

struct internal_id_info_t {
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

int cpu_ident_internal(struct cpu_raw_data_t* raw, struct cpu_id_t* data,
		       struct internal_id_info_t* internal);

#endif /* __LIBCPUID_INTERNAL_H__ */
