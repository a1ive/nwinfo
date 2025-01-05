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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "libcpuid.h"
#include "libcpuid_ctype.h"
#include "libcpuid_util.h"
#include "libcpuid_internal.h"

void match_features(const struct feature_map_t* matchtable, int count, uint32_t reg, struct cpu_id_t* data)
{
	int i;
	for (i = 0; i < count; i++)
		if (reg & (1u << matchtable[i].bit))
			data->flags[matchtable[i].feature] = 1;
}

#ifndef HAVE_POPCOUNT64
static unsigned int popcount64(uint64_t mask)
{
	unsigned int num_set_bits = 0;

	while (mask) {
		mask &= mask - 1;
		num_set_bits++;
	}

	return num_set_bits;
}
#endif

static int score(const struct match_entry_t* entry, const struct cpu_id_t* data,
                 int brand_code, uint64_t bits, int model_code)
{
	int i, tmp, res = 0;
	const struct { const char *field; int entry; int data; int score; } array[] = {
		{ "family",     entry->family,     data->x86.family,     2 },
		{ "model",      entry->model,      data->x86.model,      2 },
		{ "stepping",   entry->stepping,   data->x86.stepping,   2 },
		{ "ext_family", entry->ext_family, data->x86.ext_family, 2 },
		{ "ext_model",  entry->ext_model,  data->x86.ext_model,  2 },
		{ "ncores",     entry->ncores,     data->num_cores,      2 },
		{ "l2cache",    entry->l2cache,    data->l2_cache,       1 },
		{ "l3cache",    entry->l3cache,    data->l3_cache,       1 },
		{ "brand_code", entry->brand_code, brand_code,           2 },
		{ "model_code", entry->model_code, model_code,           2 },
	};
	for (i = 0; i < sizeof(array) / sizeof(array[0]); i++) {
		if (array[i].entry == array[i].data) {
			res += array[i].score;
			debugf(4, "Score: %-12s matches, adding %2i (current score for this entry: %2i)\n", array[i].field, array[i].score, res);
		}
	}

	tmp = popcount64(entry->model_bits & bits) * 2;
	res += tmp;
	debugf(4, "Score: %-12s matches, adding %2i (current score for this entry: %2i)\n", "model_bits", tmp, res);
	return res;
}

int match_cpu_codename(const struct match_entry_t* matchtable, int count,
                       struct cpu_id_t* data, int brand_code, uint64_t bits,
                       int model_code)
{
	int bestscore = -1;
	int bestindex = 0;
	int i, t;

	debugf(3, "Matching cpu f:%d, m:%d, s:%d, xf:%d, xm:%d, ncore:%d, l2:%d, bcode:%d, bits:%llu, code:%d\n",
		data->x86.family, data->x86.model, data->x86.stepping, data->x86.ext_family,
		data->x86.ext_model, data->num_cores, data->l2_cache, brand_code, (unsigned long long) bits, model_code);

	for (i = 0; i < count; i++) {
		t = score(&matchtable[i], data, brand_code, bits, model_code);
		debugf(3, "Entry %d, `%s', score %d\n", i, matchtable[i].name, t);
		if (t > bestscore) {
			debugf(2, "Entry `%s' selected - best score so far (%d)\n", matchtable[i].name, t);
			bestscore = t;
			bestindex = i;
		}
	}
	strncpy_s(data->cpu_codename, CODENAME_STR_MAX, matchtable[bestindex].name, CODENAME_STR_MAX);
	return bestscore;
}

static int xmatch_entry(char c, const char* p)
{
	int i, j;
	if (c == 0) return -1;
	if (c == p[0]) return 1;
	if (p[0] == '.') return 1;
	if (p[0] == '#' && isdigit(c)) return 1;
	if (p[0] == '[') {
		j = 1;
		while (p[j] && p[j] != ']') j++;
		if (!p[j]) return -1;
		for (i = 1; i < j; i++)
			if (p[i] == c) return j + 1;
	}
	return -1;
}

int match_pattern(const char* s, const char* p)
{
	int i, j, dj, k, n, m;
	n = (int) strlen(s);
	m = (int) strlen(p);
	for (i = 0; i < n; i++) {
		if (xmatch_entry(s[i], p) != -1) {
			j = 0;
			k = 0;
			while (j < m && ((dj = xmatch_entry(s[i + k], p + j)) != -1)) {
				k++;
				j += dj;
			}
			if (j == m) return i + 1;
		}
	}
	return 0;
}

struct cpu_id_t* get_cached_cpuid(void)
{
	static int initialized = 0;
	static struct cpu_id_t id;
	if (initialized) return &id;
	if (cpu_identify(NULL, &id) != ERR_OK) {
		memset(&id, 0, sizeof(id));
		id.architecture = ARCHITECTURE_UNKNOWN;
		id.vendor       = VENDOR_UNKNOWN;
	}
	initialized = 1;
	return &id;
}

int match_all(uint64_t bits, uint64_t mask)
{
	return (bits & mask) == mask;
}

/* Functions to manage cpu_affinity_mask_t type
 * Adapted from https://electronics.stackexchange.com/a/200070
 */
void init_affinity_mask(cpu_affinity_mask_t *affinity_mask)
{
	memset(affinity_mask->__bits, 0x00, __MASK_SETSIZE);
}

void copy_affinity_mask(cpu_affinity_mask_t *dest_affinity_mask, cpu_affinity_mask_t *src_affinity_mask)
{
	memcpy(dest_affinity_mask->__bits, src_affinity_mask->__bits, __MASK_SETSIZE);
}

void set_affinity_mask_bit(logical_cpu_t logical_cpu, cpu_affinity_mask_t *affinity_mask)
{
	affinity_mask->__bits[logical_cpu / __MASK_NCPUBITS] |= 0x1 << (logical_cpu % __MASK_NCPUBITS);
}

bool get_affinity_mask_bit(logical_cpu_t logical_cpu, cpu_affinity_mask_t *affinity_mask)
{
	return (affinity_mask->__bits[logical_cpu / __MASK_NCPUBITS] & (0x1 << (logical_cpu % __MASK_NCPUBITS))) != 0x00;
}

void clear_affinity_mask_bit(logical_cpu_t logical_cpu, cpu_affinity_mask_t *affinity_mask)
{
	affinity_mask->__bits[logical_cpu / __MASK_NCPUBITS] &= ~(0x1 << (logical_cpu % __MASK_NCPUBITS));
}

/* https://github.com/torvalds/linux/blob/3e5c673f0d75bc22b3c26eade87e4db4f374cd34/include/linux/bitops.h#L210-L216 */
static int get_count_order(unsigned int x)
{
	int r = 32;

	if (x == 0)
		return -1;

	--x;
	if (!x)
		return 0;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}

void assign_cache_data(uint8_t on, cache_type_t cache, int size, int assoc, int linesize, struct cpu_id_t* data)
{
	if (!on) return;
	switch (cache) {
		case L1I:
			data->l1_instruction_cache = size;
			data->l1_instruction_assoc = assoc;
			data->l1_instruction_cacheline = linesize;
			break;
		case L1D:
			data->l1_data_cache = size;
			data->l1_data_assoc = assoc;
			data->l1_data_cacheline = linesize;
			break;
		case L2:
			data->l2_cache = size;
			data->l2_assoc = assoc;
			data->l2_cacheline = linesize;
			break;
		case L3:
			data->l3_cache = size;
			data->l3_assoc = assoc;
			data->l3_cacheline = linesize;
			break;
		case L4:
			data->l4_cache = size;
			data->l4_assoc = assoc;
			data->l4_cacheline = linesize;
			break;
		default:
			break;
	}
}

void decode_number_of_cores_x86(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int logical_cpus = -1, num_cores = -1;

	if (raw->basic_cpuid[0][EAX] >= 1) {
		logical_cpus = (raw->basic_cpuid[1][EBX] >> 16) & 0xff;
		if (raw->basic_cpuid[0][EAX] >= 4) {
			num_cores = 1 + ((raw->basic_cpuid[4][EAX] >> 26) & 0x3f);
		}
	}
	if (data->flags[CPU_FEATURE_HT]) {
		if (num_cores > 1) {
			data->num_cores = num_cores;
			data->num_logical_cpus = logical_cpus;
		} else {
			data->num_cores = 1;
			data->num_logical_cpus = (logical_cpus >= 1 ? logical_cpus : 1);
			if (data->num_logical_cpus == 1)
				data->flags[CPU_FEATURE_HT] = 0;
		}
	} else {
		data->num_cores = data->num_logical_cpus = (logical_cpus >= 1 ? logical_cpus : 1);
	}
}

void decode_deterministic_cache_info_x86(uint32_t cache_regs[][NUM_REGS],
                                         uint8_t subleaf_count,
                                         struct cpu_id_t* data,
                                         struct internal_id_info_t* internal)
{
	uint8_t i;
	uint32_t cache_level, cache_type, ways, partitions, linesize, sets, size, num_sharing_cache, index_msb;
	cache_type_t type;

	for (i = 0; i < subleaf_count; i++) {
		cache_level = EXTRACTS_BITS(cache_regs[i][EAX], 7, 5);
		cache_type  = EXTRACTS_BITS(cache_regs[i][EAX], 4, 0);
		if ((cache_level == 0) || (cache_type == 0))
			break;
		if (cache_level == 1 && cache_type == 1)
			type = L1D;
		else if (cache_level == 1 && cache_type == 2)
			type = L1I;
		else if (cache_level == 2 && cache_type == 3)
			type = L2;
		else if (cache_level == 3 && cache_type == 3)
			type = L3;
		else if (cache_level == 4 && cache_type == 3)
			type = L4;
		else {
			warnf("deterministic_cache: unknown level/typenumber combo (%d/%d), cannot\n", cache_level, cache_type);
			warnf("deterministic_cache: recognize cache type\n");
			continue;
		}
		num_sharing_cache       = EXTRACTS_BITS(cache_regs[i][EAX], 25, 14) + 1;
		ways                    = EXTRACTS_BITS(cache_regs[i][EBX], 31, 22) + 1;
		partitions              = EXTRACTS_BITS(cache_regs[i][EBX], 21, 12) + 1;
		linesize                = EXTRACTS_BITS(cache_regs[i][EBX], 11,  0) + 1;
		sets                    = EXTRACTS_BITS(cache_regs[i][ECX], 31,  0) + 1;
		size                    = ways * partitions * linesize * sets / 1024;
		index_msb               = get_count_order(num_sharing_cache);
		internal->cache_mask[i] = ~((1 << index_msb) - 1);
		assign_cache_data(1, type, size, ways, linesize, data);
	}
}

void decode_architecture_version_x86(struct cpu_id_t* data)
{
	bool is_compliant, has_all_features;
	int i, j;
	cpu_feature_level_t feature_level = FEATURE_LEVEL_UNKNOWN;

	const struct { const int family; const cpu_feature_level_t feature_level; }
	architecture_matchtable_ia_32[] = {
		{  3, FEATURE_LEVEL_I386 },
		{  4, FEATURE_LEVEL_I486 },
		{  5, FEATURE_LEVEL_I586 },
		{  6, FEATURE_LEVEL_I686 },
		{ 15, FEATURE_LEVEL_I686 }, // Intel Pentium 4, AMD K8
	};

	const cpu_feature_t architecture_x86_64_v1[] = {
		CPU_FEATURE_CMOV,
		CPU_FEATURE_CX8,
		CPU_FEATURE_FPU,
		CPU_FEATURE_FXSR,
		CPU_FEATURE_MMX,
		CPU_FEATURE_SSE,
		CPU_FEATURE_SSE2,
		-1
	};

	const cpu_feature_t architecture_x86_64_v2[] = {
		CPU_FEATURE_CX16,
		CPU_FEATURE_LAHF_LM,
		CPU_FEATURE_POPCNT,
		CPU_FEATURE_PNI,
		CPU_FEATURE_SSE4_1,
		CPU_FEATURE_SSE4_2,
		CPU_FEATURE_SSSE3,
		-1
	};

	const cpu_feature_t architecture_x86_64_v3[] = {
		CPU_FEATURE_AVX,
		CPU_FEATURE_AVX2,
		CPU_FEATURE_BMI1,
		CPU_FEATURE_BMI2,
		CPU_FEATURE_F16C,
		CPU_FEATURE_FMA3,
		CPU_FEATURE_ABM,
		CPU_FEATURE_MOVBE,
		CPU_FEATURE_OSXSAVE,
		-1
	};

	const cpu_feature_t architecture_x86_64_v4[] = {
		CPU_FEATURE_AVX512F,
		CPU_FEATURE_AVX512BW,
		CPU_FEATURE_AVX512CD,
		CPU_FEATURE_AVX512DQ,
		CPU_FEATURE_AVX512VL,
		-1
	};

	const struct { const cpu_feature_t* features_array; const cpu_feature_level_t feature_level; }
	architecture_matchtable_x86_64[] = {
		{ architecture_x86_64_v1, FEATURE_LEVEL_X86_64_V1 },
		{ architecture_x86_64_v2, FEATURE_LEVEL_X86_64_V2 },
		{ architecture_x86_64_v3, FEATURE_LEVEL_X86_64_V3 },
		{ architecture_x86_64_v4, FEATURE_LEVEL_X86_64_V4 },
	};

	if (!data->flags[CPU_FEATURE_LM]) {
		/* Check Intel Architecture, 32-bit */
		for (i = 0; i < COUNT_OF(architecture_matchtable_ia_32); i++) {
			is_compliant = (data->x86.family == architecture_matchtable_ia_32[i].family);
			debugf(3, "Check if CPU is %s compliant: %s for family %i\n", cpu_feature_level_str(architecture_matchtable_ia_32[i].feature_level), is_compliant ? "yes" : "no", architecture_matchtable_ia_32[i].family);
			if (is_compliant) {
				feature_level = architecture_matchtable_ia_32[i].feature_level;
				break;
			}
		}
	}
	else {
		/* Check Intel Architecture, 64-bit */
		for (i = 0; i < COUNT_OF(architecture_matchtable_x86_64); i++) {
			debugf(3, "Check if CPU is %s compliant:\n", cpu_feature_level_str(architecture_matchtable_x86_64[i].feature_level));
			has_all_features = true;
			for (j = 0; architecture_matchtable_x86_64[i].features_array[j] != -1; j++) {
				is_compliant     = data->flags[ architecture_matchtable_x86_64[i].features_array[j] ];
				has_all_features = has_all_features && is_compliant;
				debugf(3, " - feature %s is %s\n", cpu_feature_str(architecture_matchtable_x86_64[i].features_array[j]), is_compliant ? "present" : "absent");
			}
			if (is_compliant)
				feature_level = architecture_matchtable_x86_64[i].feature_level;
			else
				break;
		}
	}

	data->feature_level = feature_level;
	if (feature_level == FEATURE_LEVEL_UNKNOWN)
		warnf("Warning: CPU with CPUID signature %02X_%02XH has an unknown architecture version (LM=%i).\n", data->x86.ext_family, data->x86.ext_model, data->flags[CPU_FEATURE_LM]);
	else
		debugf(2, "x86 architecture version is %s\n", cpu_feature_level_str(feature_level));
}
