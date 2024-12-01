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
#ifndef __LIBCPUID_H__
#define __LIBCPUID_H__
/**
 * \file     libcpuid.h
 * \author   Veselin Georgiev
 * \date     Oct 2008
 * \version  0.7.1
 *
 * Version history:
 *
 * * 0.1.0 (2008-10-15): initial adaptation from wxfractgui sources
 * * 0.1.1 (2009-07-06): Added intel_fn11 fields to cpu_raw_data_t to handle
 *                       new processor topology enumeration required on Core i7
 * * 0.1.2 (2009-09-26): Added support for MSR reading through self-extracting
 *                       kernel driver on Win32.
 * * 0.1.3 (2010-04-20): Added support for greater more accurate CPU clock
 *                       measurements with cpu_clock_by_ic()
 * * 0.2.0 (2011-10-11): Support for AMD Bulldozer CPUs, 128-bit SSE unit size
 *                       checking. A backwards-incompatible change, since the
 *                       sizeof cpu_id_t is now different.
 * * 0.2.1 (2012-05-26): Support for Ivy Bridge, and detecting the presence of
 *                       the RdRand instruction.
 * * 0.2.2 (2015-11-04): Support for newer processors up to Haswell and Vishera.
 *                       Fix clock detection in cpu_clock_by_ic() for Bulldozer.
 *                       More entries supported in cpu_msrinfo().
 *                       *BSD and Solaris support (unofficial).
 * * 0.3.0 (2016-07-09): Support for Skylake; MSR ops in FreeBSD; INFO_VOLTAGE
 *                       for AMD CPUs. Level 4 cache support for Crystalwell
 *                       (a backwards-incompatible change since the sizeof
 *                        cpu_raw_data_t is now different).
 * * 0.4.0 (2016-09-30): Better detection of AMD clock multiplier with msrinfo.
 *                       Support for Intel SGX detection
 *                       (a backwards-incompatible change since the sizeof
 *                        cpu_raw_data_t and cpu_id_t is now different).
 * * 0.4.1 (2019-02-05): A lot of DB updates, and better RDMSR
 * * 0.5.0 (2020-05-23): A lot of DB updates, detection of new CPU features,
 *                       (a backwards-incompatible change since the sizeof
 *                        cpu_raw_data_t and cpu_id_t is now different).
 * * 0.5.1 (2021-03-20): A lot of DB updates
 * * 0.6.0 (2022-09-23): Support for hybrid CPUs; CPU cache instances count;
 *                       a lot of DB updates
 *                       (a backwards-incompatible change since the sizeof
 *                        cpu_id_t is now different).
 * * 0.6.1 (2022-10-23): A lot of DB updates, fix set_cpu_affinity() on Windows,
 *                       fix cpu_identify_all() when HT is disabled.
 * * 0.6.2 (2022-11-11): A lot of DB updates, fix cpu_identify_all() for single-core CPUs.
 * * 0.6.3 (2023-04-02): A lot of DB updates, fix infinite loop in set_cpu_affinity() on macOS,
 *                       fix a misprint of extended CPUID in cpuid_basic_identify(),
 *                       restore previous thread CPU affinity before returning from cpuid_get_all_raw_data(),
 *                       query CPU info at least once even if set_cpu_affinity() fails,
 *                       rename set_error() to cpuid_set_error() and get_error() to cpuid_get_error().
 * * 0.6.4 (2023-10-08): A lot of DB updates, add support for Centaur CPUs (VIA and Zhaoxin),
 *                       fix floating point exception in cpu_identify_all(),
 *                       fix build for NetBSD and DragonFly BSD.
 * * 0.6.5 (2024-04-28): A lot of DB updates, support heterogeneous RAW dumps in cpu_identify_all(),
 *                       detection of LP E-Cores for Intel CPUs.
 * * 0.7.0 (2024-08-26): A lot of DB updates, add cpu_clock_by_tsc() function,
 *                       add support for ARM CPUs, add cpu_feature_level_t enumerated values,
 *                       add more fields in cpu_raw_data_t (amd_fn80000026h, arm_*),
 *                       detect x2APIC and AVX512 features for AMD x86 CPUs.
 * * 0.7.1 (2024-11-30): A lot of DB updates, fix build on ARM platforms.
 */

/** @mainpage A simple libcpuid introduction
 *
 * LibCPUID provides CPU identification and access to the CPUID and RDTSC
 * instructions on the x86.
 * <p>
 * To execute CPUID, use \ref cpu_exec_cpuid. <br>
 * To execute RDTSC, use \ref cpu_rdtsc. <br>
 * To fetch the CPUID info needed for CPU identification, use
 *   \ref cpuid_get_raw_data or \ref cpuid_get_all_raw_data. <br>
 * To make sense of that data (decode, extract features), use
 *    \ref cpu_identify or \ref cpu_identify_all. <br>
 * To detect the CPU speed, use either \ref cpu_clock, \ref cpu_clock_by_os,
 * \ref cpu_tsc_mark + \ref cpu_tsc_unmark + \ref cpu_clock_by_mark,
 * \ref cpu_clock_measure, \ref cpu_clock_by_ic or \ref cpu_clock_by_tsc.
 * Read carefully for pros/cons of each method. <br>
 *
 * To read MSRs, use \ref cpu_msr_driver_open to get a handle, and then
 * \ref cpu_rdmsr for querying abilities. Some MSR decoding is available on recent
 * CPUs, and can be queried through \ref cpu_msrinfo; the various types of queries
 * are described in \ref cpu_msrinfo_request_t.
 * </p>
 */

/** @defgroup libcpuid LibCPUID
 * @brief LibCPUID provides CPU identification
 @{ */

/* Include C99 booleans: */
#include <stdbool.h>

#include <winring0.h>
#define LIBCPUID_DISABLE_DEPRECATED

/* Include some integer type specifications: */
#include "libcpuid_types.h"

/* Some limits and other constants */
#include "libcpuid_constants.h"

#ifndef LIBCPUID_DEPRECATED
#  if defined (__cplusplus) && (__cplusplus >= 201402) /* C++14 or greater */
#    define LIBCPUID_DEPRECATED(message) [[deprecated(message)]]
#  elif (defined(GNUC) && (GNUC > 4 || (GNUC == 4 && GNUC_MINOR >= 5))) || defined(__clang__)
#    define LIBCPUID_DEPRECATED(message) __attribute__((deprecated(message)))
#  elif defined(__GNUC__) && (__GNUC__ >= 3)
#    define LIBCPUID_DEPRECATED(message) __attribute__((deprecated))
#  elif defined(_MSC_VER)
#    define LIBCPUID_DEPRECATED(message) __declspec(deprecated(message))
#  else
#    pragma message("WARNING: You need to implement LIBCPUID_DEPRECATED for this compiler")
#    define LIBCPUID_DEPRECATED(message)
#  endif
#endif /* LIBCPUID_DEPRECATED */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CPU vendor, as guessed from the Vendor String.
 */
typedef enum {
	VENDOR_INTEL = 0,  /*!< Intel CPU */
	VENDOR_AMD,        /*!< AMD CPU */
	VENDOR_CYRIX,      /*!< Cyrix CPU */
	VENDOR_NEXGEN,     /*!< NexGen CPU */
	VENDOR_TRANSMETA,  /*!< Transmeta CPU */
	VENDOR_UMC,        /*!< x86 CPU by UMC */
	VENDOR_CENTAUR,    /*!< x86 CPU by IDT */
	VENDOR_RISE,       /*!< x86 CPU by Rise Technology */
	VENDOR_SIS,        /*!< x86 CPU by SiS */
	VENDOR_NSC,        /*!< x86 CPU by National Semiconductor */
	VENDOR_HYGON,	   /*!< Hygon CPU */
	VENDOR_VORTEX86,   /*!< DM&P Vortex86 CPU */
	VENDOR_VIA,        /*!< x86 CPU by VIA */
	VENDOR_ZHAOXIN,    /*!< x86 CPU by Zhaoxin */
	VENDOR_ARM,        /*!< ARM CPU */
	VENDOR_BROADCOM,   /*!< Broadcom Corporation CPU */
	VENDOR_CAVIUM,     /*!< Cavium Inc. CPU */
	VENDOR_DEC,        /*!< Digital Equipment Corporation CPU */
	VENDOR_FUJITSU,    /*!< Fujitsu Ltd. CPU */
	VENDOR_HISILICON,  /*!< HiSilicon Technology Co., Ltd. CPU */
	VENDOR_INFINEON,   /*!< Infineon Technologies AG CPU */
	VENDOR_FREESCALE,  /*!< Motorola or Freescale Semiconductor Inc. CPU */
	VENDOR_NVIDIA,     /*!< NVIDIA Corporation CPU */
	VENDOR_APM,        /*!< Applied Micro Circuits Corporation CPU */
	VENDOR_QUALCOMM,   /*!< Qualcomm Inc. CPU */
	VENDOR_SAMSUNG,    /*!< Samsung Group CPU */
	VENDOR_MARVELL,    /*!< Marvell International Ltd. CPU */
	VENDOR_APPLE,      /*!< Apple Inc. CPU */
	VENDOR_FARADAY,    /*!< Faraday Technology CPU */
	VENDOR_MICROSOFT,  /*!< Microsoft Corporation CPU */
	VENDOR_PHYTIUM,    /*!< Phytium Technology Co., Ltd CPU */
	VENDOR_AMPERE,     /*!< Ampere Computing CPU */

	NUM_CPU_VENDORS,   /*!< Valid CPU vendor ids: 0..NUM_CPU_VENDORS - 1 */
	VENDOR_UNKNOWN = -1,
} cpu_vendor_t;
#define NUM_CPU_VENDORS NUM_CPU_VENDORS

/**
 * @brief CPU architecture
 */
typedef enum {
	ARCHITECTURE_X86 = 0,  /*!< x86 CPU */
	ARCHITECTURE_ARM,      /*!< ARM CPU */

	NUM_CPU_ARCHITECTURES, /*!< Valid CPU architecture ids: 0..NUM_CPU_ARCHITECTURES - 1 */
	ARCHITECTURE_UNKNOWN = -1,
} cpu_architecture_t;
#define NUM_CPU_ARCHITECTURES NUM_CPU_ARCHITECTURES

/**
 * @brief CPU feature level
 */
typedef enum {
	/* x86: https://en.wikipedia.org/wiki/X86-64#Microarchitecture_levels */
	FEATURE_LEVEL_I386, /*!< i386 */
	FEATURE_LEVEL_I486, /*!< i486 */
	FEATURE_LEVEL_I586, /*!< i586 */
	FEATURE_LEVEL_I686, /*!< i686 */
	FEATURE_LEVEL_X86_64_V1, /*!< x86-64-v1 */
	FEATURE_LEVEL_X86_64_V2, /*!< x86-64-v2 */
	FEATURE_LEVEL_X86_64_V3, /*!< x86-64-v3 */
	FEATURE_LEVEL_X86_64_V4, /*!< x86-64-v4 */

	/* ARM: https://en.wikipedia.org/wiki/ARM_architecture_family#Cores */
	FEATURE_LEVEL_ARM_V1 = 100, /*!< ARMv1 */
	FEATURE_LEVEL_ARM_V2, /*!< ARMv2 */
	FEATURE_LEVEL_ARM_V3, /*!< ARMv3 */
	FEATURE_LEVEL_ARM_V4, /*!< ARMv4 */
	FEATURE_LEVEL_ARM_V4T, /*!< ARMv4T */
	FEATURE_LEVEL_ARM_V5, /*!< ARMv5 (obsolete) */
	FEATURE_LEVEL_ARM_V5T, /*!< ARMv5T */
	FEATURE_LEVEL_ARM_V5TE, /*!< ARMv5TE */
	FEATURE_LEVEL_ARM_V5TEJ, /*!< ARMv5TEJ */
	FEATURE_LEVEL_ARM_V6, /*!< ARMv6 */
	FEATURE_LEVEL_ARM_V6_M, /*!< ARMv6-M */
	FEATURE_LEVEL_ARM_V7_A, /*!< ARMv7-A */
	FEATURE_LEVEL_ARM_V7_M, /*!< ARMv7-M */
	FEATURE_LEVEL_ARM_V7_R, /*!< ARMv7-R */
	FEATURE_LEVEL_ARM_V7E_M, /*!< ARMv7E-M */
	FEATURE_LEVEL_ARM_V8_0_A, /*!< ARMv8.0-A */
	FEATURE_LEVEL_ARM_V8_0_M, /*!< ARMv8.0-M */
	FEATURE_LEVEL_ARM_V8_0_R, /*!< ARMv8.0-R */
	FEATURE_LEVEL_ARM_V8_1_A, /*!< ARMv8.1-A */
	FEATURE_LEVEL_ARM_V8_1_M, /*!< ARMv8.1-M */
	FEATURE_LEVEL_ARM_V8_2_A, /*!< ARMv8.2-A */
	FEATURE_LEVEL_ARM_V8_3_A, /*!< ARMv8.3-A */
	FEATURE_LEVEL_ARM_V8_4_A, /*!< ARMv8.4-A */
	FEATURE_LEVEL_ARM_V8_5_A, /*!< ARMv8.5-A */
	FEATURE_LEVEL_ARM_V8_6_A, /*!< ARMv8.6-A */
	FEATURE_LEVEL_ARM_V8_7_A, /*!< ARMv8.7-A */
	FEATURE_LEVEL_ARM_V8_8_A, /*!< ARMv8.8-A */
	FEATURE_LEVEL_ARM_V8_9_A, /*!< ARMv8.9-A */
	FEATURE_LEVEL_ARM_V9_0_A, /*!< ARMv9.0-A */
	FEATURE_LEVEL_ARM_V9_1_A, /*!< ARMv9.1-A */
	FEATURE_LEVEL_ARM_V9_2_A, /*!< ARMv9.2-A */
	FEATURE_LEVEL_ARM_V9_3_A, /*!< ARMv9.3-A */
	FEATURE_LEVEL_ARM_V9_4_A, /*!< ARMv9.4-A */

	NUM_FEATURE_LEVELS, /*!< Valid feature level ids: 0..NUM_FEATURE_LEVELS - 1 */
	FEATURE_LEVEL_UNKNOWN = -1,
} cpu_feature_level_t;
#define NUM_FEATURE_LEVELS NUM_FEATURE_LEVELS

/**
 * @brief CPU purpose
 */
typedef enum {
	PURPOSE_GENERAL = 0,   /*!< general purpose CPU */
	PURPOSE_PERFORMANCE,   /*!< performance CPU */
	PURPOSE_EFFICIENCY,    /*!< efficiency CPU */
	PURPOSE_LP_EFFICIENCY, /*!< low-power efficiency CPU */
	PURPOSE_U_PERFORMANCE, /*!< ultimate performance CPU */

	NUM_CPU_PURPOSES,      /*!< Valid CPU purpose ids: 0..NUM_CPU_PURPOSES - 1 */
} cpu_purpose_t;
#define NUM_CPU_PURPOSES NUM_CPU_PURPOSES

/**
 * @brief Hypervisor vendor, as guessed from the CPU_FEATURE_HYPERVISOR flag.
 */
typedef enum {
	HYPERVISOR_NONE = 0,     /*!< no hypervisor */
	HYPERVISOR_ACRN,         /*!< Projext ACRN hypervisor */
	HYPERVISOR_BHYVE,        /*!< FreeBSD bhyve hypervisor */
	HYPERVISOR_HYPERV,       /*!< Microsoft Hyper-V or Windows Virtual PC hypervisor */
	HYPERVISOR_KVM,          /*!< KVM hypervisor */
	HYPERVISOR_PARALLELS,    /*!< Parallels hypervisor */
	HYPERVISOR_QEMU,         /*!< QEMU hypervisor */
	HYPERVISOR_QNX,          /*!< QNX hypervisor */
	HYPERVISOR_VIRTUALBOX,   /*!< VirtualBox hypervisor */
	HYPERVISOR_VMWARE,       /*!< VMware hypervisor */
	HYPERVISOR_XEN,          /*!< Xen hypervisor */
	NUM_HYPERVISOR_VENDORS,  /*!< Valid hypervisor vendor ids: 0..NUM_HYPERVISOR_VENDORS - 1 */
	HYPERVISOR_UNKNOWN = -1,
} hypervisor_vendor_t;
#define NUM_HYPERVISOR_VENDORS NUM_HYPERVISOR_VENDORS

/**
 * @brief Contains just the raw CPUID data.
 *
 * This contains only the most basic CPU data, required to do identification
 * and feature recognition. Every processor should be identifiable using this
 * data only.
 */
struct cpu_raw_data_t {
	/** contains results of CPUID for eax = 0, 1, ...*/
	uint32_t basic_cpuid[MAX_CPUID_LEVEL][NUM_REGS];

	/** contains results of CPUID for eax = 0x80000000, 0x80000001, ...*/
	uint32_t ext_cpuid[MAX_EXT_CPUID_LEVEL][NUM_REGS];

	/** when the CPU is intel and it supports deterministic cache
	    information: this contains the results of CPUID for eax = 4
	    and ecx = 0, 1, ... */
	uint32_t intel_fn4[MAX_INTELFN4_LEVEL][NUM_REGS];

	/** when the CPU is intel and it supports leaf 0Bh (Extended Topology
	    enumeration leaf), this stores the result of CPUID with
	    eax = 11 and ecx = 0, 1, 2... */
	uint32_t intel_fn11[MAX_INTELFN11_LEVEL][NUM_REGS];

	/** when the CPU is intel and supports leaf 12h (SGX enumeration leaf),
	 *  this stores the result of CPUID with eax = 0x12 and
	 *  ecx = 0, 1, 2... */
	uint32_t intel_fn12h[MAX_INTELFN12H_LEVEL][NUM_REGS];

	/** when the CPU is intel and supports leaf 14h (Intel Processor Trace
	 *  capabilities leaf).
	 *  this stores the result of CPUID with eax = 0x12 and
	 *  ecx = 0, 1, 2... */
	uint32_t intel_fn14h[MAX_INTELFN14H_LEVEL][NUM_REGS];

	/** when the CPU is AMD and supports leaf 8000001Dh
	 * (topology information for the DC)
	 * this stores the result of CPUID with eax = 8000001Dh and
	 *  ecx = 0, 1, 2... */
	uint32_t amd_fn8000001dh[MAX_AMDFN8000001DH_LEVEL][NUM_REGS];

	/** when the CPU is AMD and supports leaf 80000026h
	 * (Extended CPU Topology leaf)
	 * this stores the result of CPUID with eax = 80000026h and
	 *  ecx = 0, 1, 2... */
	uint32_t amd_fn80000026h[MAX_AMDFN80000026H_LEVEL][NUM_REGS];

	/** when then CPU is ARM-based and supports MIDR
	 * (Main ID Register) */
	uint64_t arm_midr;

	/** when then CPU is ARM-based and supports MPIDR
	 * (Multiprocessor Affinity Register) */
	uint64_t arm_mpidr;

	/** when then CPU is ARM-based and supports REVIDR
	 * (Revision ID Register) */
	uint64_t arm_revidr;

	/** when then CPU is ARM-based and supports ID_AFR*
	 * (AArch32 Auxiliary Feature Register) */
	uint32_t arm_id_afr[MAX_ARM_ID_AFR_REGS];

	/** when then CPU is ARM-based and supports ID_DFR*
	 * (AArch32 Debug Feature Register) */
	uint32_t arm_id_dfr[MAX_ARM_ID_DFR_REGS];

	/** when then CPU is ARM-based and supports D_ISAR*
	 * (AArch32 Instruction Set Attribute Register) */
	uint32_t arm_id_isar[MAX_ARM_ID_ISAR_REGS];

	/** when then CPU is ARM-based and supports ID_MMFR*
	 * (AArch32 Memory Model Feature Register) */
	uint32_t arm_id_mmfr[MAX_ARM_ID_MMFR_REGS];

	/** when then CPU is ARM-based and supports ID_PFR*
	 * (AArch32 Processor Feature Register) */
	uint32_t arm_id_pfr[MAX_ARM_ID_PFR_REGS];

	/** when then CPU is ARM-based and supports ID_AA64AFR*
	 * (AArch64 Auxiliary Feature Register) */
	uint64_t arm_id_aa64afr[MAX_ARM_ID_AA64AFR_REGS];

	/** when then CPU is ARM-based and supports ID_AA64DFR*
	 * (AArch64 Debug Feature Register) */
	uint64_t arm_id_aa64dfr[MAX_ARM_ID_AA64DFR_REGS];

	/** when then CPU is ARM-based and supports D_AA64ISAR*
	 * (AArch64 Instruction Set Attribute Register) */
	uint64_t arm_id_aa64isar[MAX_ARM_ID_AA64ISAR_REGS];

	/** when then CPU is ARM-based and supports ID_AA64MMFR*
	 * (AArch64 Memory Model Feature Register) */
	uint64_t arm_id_aa64mmfr[MAX_ARM_ID_AA64MMFR_REGS];

	/** when then CPU is ARM-based and supports ID_AA64PFR*
	 * (AArch64 Processor Feature Register) */
	uint64_t arm_id_aa64pfr[MAX_ARM_ID_AA64PFR_REGS];

	/** when then CPU is ARM-based and supports ID_AA64SMFR*
	 * (AArch64 SME Feature ID Register ) */
	uint64_t arm_id_aa64smfr[MAX_ARM_ID_AA64SMFR_REGS];

	/** when then CPU is ARM-based and supports ID_AA64ZFR*
	 * (SVE Feature ID register) */
	uint64_t arm_id_aa64zfr[MAX_ARM_ID_AA64ZFR_REGS];
};

/**
 * @brief Contains an array of raw CPUID data.
 *
 * This contains one \ref cpu_raw_data_t for each logical CPU.
 *
 * @note There is a hard limit of raw array: it is bounded by the logical_cpu_t type.
 * In other words, the limit is 65536 logical CPUs in the system.
 */
struct cpu_raw_data_array_t {
	/** Indicates if \ref raw was obtained by using CPU affinity
	 *  if false, \ref raw contains a single data from an old dump (versions 0.5.1 and below).
	 *  if true, \ref raw contains data from a new dump (versions 0.6.0 and above).
	 *  if true and if \ref num_raw is 1, it indicates only one logical core was detected on the system.
	 */
	bool with_affinity;

	/** \ref raw length */
	logical_cpu_t num_raw;

	/** array of raw CPUID data */
	struct cpu_raw_data_t* raw;
};

/**
 * @brief This contains information about SGX features of the processor
 * Example usage:
 * @code
 * ...
 * struct cpu_raw_data_t raw;
 * struct cpu_id_t id;
 *
 * if (cpuid_get_raw_data(&raw) == 0 && cpu_identify(&raw, &id) == 0 && id.sgx.present) {
 *   printf("SGX is present.\n");
 *   printf("SGX1 instructions: %s.\n", id.sgx.flags[INTEL_SGX1] ? "present" : "absent");
 *   printf("SGX2 instructions: %s.\n", id.sgx.flags[INTEL_SGX2] ? "present" : "absent");
 *   printf("Max 32-bit enclave size: 2^%d bytes.\n", id.sgx.max_enclave_32bit);
 *   printf("Max 64-bit enclave size: 2^%d bytes.\n", id.sgx.max_enclave_64bit);
 *   for (int i = 0; i < id.sgx.num_epc_sections; i++) {
 *     struct cpu_epc_t epc = cpuid_get_epc(i, NULL);
 *     printf("EPC section #%d: address = %x, size = %d bytes.\n", epc.address, epc.size);
 *   }
 * } else {
 *   printf("SGX is not present.\n");
 * }
 * @endcode
 */
struct cpu_sgx_t {
	/** Whether SGX is present (boolean) */
	uint32_t present;

	/** Max enclave size in 32-bit mode. This is a power-of-two value:
	 *  if it is "31", then the max enclave size is 2^31 bytes (2 GiB).
	 */
	uint8_t max_enclave_32bit;

	/** Max enclave size in 64-bit mode. This is a power-of-two value:
	 *  if it is "36", then the max enclave size is 2^36 bytes (64 GiB).
	 */
	uint8_t max_enclave_64bit;

	/**
	 * contains SGX feature flags. See the \ref cpu_sgx_feature_t
	 * "INTEL_SGX*" macros below.
	 */
	uint8_t flags[SGX_FLAGS_MAX];

	/** number of Enclave Page Cache (EPC) sections. Info for each
	 *  section is available through the \ref cpuid_get_epc() function
	 */
	int num_epc_sections;

	/** bit vector of the supported extended  features that can be written
	 *  to the MISC region of the SSA (Save State Area)
	 */
	uint32_t misc_select;

	/** a bit vector of the attributes that can be set to SECS.ATTRIBUTES
	 *  via ECREATE. Corresponds to bits 0-63 (incl.) of SECS.ATTRIBUTES.
	 */
	uint64_t secs_attributes;

	/** a bit vector of the bits that can be set in the XSAVE feature
	 *  request mask; Corresponds to bits 64-127 of SECS.ATTRIBUTES.
	 */
	uint64_t secs_xfrm;
};

/**
 * @brief Contains x86 specific info.
 *
 * @note This is part of \ref cpu_id_t.
 */
struct x86_id_t {
	/** CPU family (BaseFamily[3:0]) */
	int32_t family;

	/** CPU model (BaseModel[3:0]) */
	int32_t model;

	/** CPU stepping */
	int32_t stepping;

	/** CPU display ("true") family (computed as BaseFamily[3:0]+ExtendedFamily[7:0]) */
	int32_t ext_family;

	/**
	 * CPU display ("true") model (computed as (ExtendedModel[3:0]<<4) + BaseModel[3:0])
	 * For detailed discussion about what BaseModel / ExtendedModel / Model are, see Github issue #150.
	 */
	int32_t ext_model;

	/** SSE execution unit size (64 or 128; -1 if N/A) */
	int32_t sse_size;

	/** contains information about SGX features if the processor, if present */
	struct cpu_sgx_t sgx;
};

/**
 * @brief Contains ARM specific info.
 *
 * @note This is part of \ref cpu_id_t.
 */
struct arm_id_t {
	/** CPU implementer code */
	uint8_t implementer;

	/** CPU variant number */
	uint8_t variant;

	/** CPU primary part number */
	uint16_t part_num;

	/** CPU revision number */
	uint8_t revision;
};

/**
 * @brief This contains the recognized CPU features/info
 */
struct cpu_id_t {
	/** index of cpu_types in system_id_t */
	uint8_t index;
	/** contains the CPU architecture ID (e.g. ARCHITECTURE_X86) */
	cpu_architecture_t architecture;

	/**
	 * contains the CPU feature level,
	 * also know as microarchitecture levels (x86)
	 * and architecture version (ARM)
	 */
	cpu_feature_level_t feature_level;

	/** contains the CPU vendor string, e.g. "GenuineIntel" */
	char vendor_str[VENDOR_STR_MAX];

	/** contains the brand string, e.g. "Intel(R) Xeon(TM) CPU 2.40GHz" */
	char brand_str[BRAND_STR_MAX];

	/** contains the recognized CPU vendor */
	cpu_vendor_t vendor;

	/**
	 * contain CPU flags. Used to test for features. See
	 * the \ref cpu_feature_t "CPU_FEATURE_*" macros below.
	 * @see Features
	 */
	uint8_t flags[CPU_FLAGS_MAX];

#ifndef LIBCPUID_DISABLE_DEPRECATED
	/**
	 * CPU family (BaseFamily[3:0])
	 * @deprecated replaced by \ref x86_id_t::family (prefix member with `x86.`, e.g. `id.x86.family`)
	 */
	LIBCPUID_DEPRECATED("replace with '.x86.family' in your code to fix the warning")
	int32_t family;

	/**
	 * CPU model (BaseModel[3:0])
	 * @deprecated replaced by \ref x86_id_t::model (prefix member with `x86.`, e.g. `id.x86.model`)
	 */
	LIBCPUID_DEPRECATED("replace with '.x86.model' in your code to fix the warning")
	int32_t model;

	/**
	 * CPU stepping
	 * @deprecated replaced by \ref x86_id_t::stepping (prefix member with `x86.`, e.g. `id.x86.stepping`)
	 */
	LIBCPUID_DEPRECATED("replace with '.x86.stepping' in your code to fix the warning")
	int32_t stepping;

	/**
	 * CPU display ("true") family (computed as BaseFamily[3:0]+ExtendedFamily[7:0])
	 * @deprecated replaced by \ref x86_id_t::ext_family (prefix member with `x86.`, e.g. `id.x86.ext_family`)
	 */
	LIBCPUID_DEPRECATED("replace with '.x86.ext_family' in your code to fix the warning")
	int32_t ext_family;

	/**
	 * CPU display ("true") model (computed as (ExtendedModel[3:0]<<4) + BaseModel[3:0])
	 * For detailed discussion about what BaseModel / ExtendedModel / Model are, see Github issue #150.
	 * @deprecated replaced by \ref x86_id_t::ext_model (prefix member with `x86.`, e.g. `id.x86.ext_model`)
	 */
	LIBCPUID_DEPRECATED("replace with '.x86.ext_model' in your code to fix the warning")
	int32_t ext_model;
#endif /* LIBCPUID_DISABLE_DEPRECATED */

	/**
	 * contains architecture specific info.
	 * Use \ref cpu_id_t::architecture to know which member is valid.
	 */
	union {
		struct x86_id_t x86;
		struct arm_id_t arm;
	};

	/** Number of CPU cores on the current processor */
	int32_t num_cores;

	/**
	 * Number of logical processors on the current processor.
	 * Could be more than the number of physical cores,
	 * e.g. when the processor has HyperThreading.
	 */
	int32_t num_logical_cpus;

	/**
	 * The total number of logical processors.
	 * The same value is available through \ref cpuid_get_total_cpus.
	 *
	 * This is num_logical_cpus * {total physical processors in the system}
	 * (but only on a real system, under a VM this number may be lower).
	 *
	 * If you're writing a multithreaded program and you want to run it on
	 * all CPUs, this is the number of threads you need.
	 *
	 * @note in a VM, this will exactly match the number of CPUs set in
	 *       the VM's configuration.
	 *
	 */
	int32_t total_logical_cpus;

	/**
	 * L1 data cache size in KB. Could be zero, if the CPU lacks cache.
	 * If the size cannot be determined, it will be -1.
	 */
	int32_t l1_data_cache;

	/**
	 * L1 instruction cache size in KB. Could be zero, if the CPU lacks
	 * cache. If the size cannot be determined, it will be -1.
	 * @note On some Intel CPUs, whose instruction cache is in fact
	 * a trace cache, the size will be expressed in K uOps.
	 */
	int32_t l1_instruction_cache;

	/**
	 * L2 cache size in KB. Could be zero, if the CPU lacks L2 cache.
	 * If the size of the cache could not be determined, it will be -1
	 */
	int32_t l2_cache;

	/** L3 cache size in KB. Zero on most systems */
	int32_t l3_cache;

	/** L4 cache size in KB. Zero on most systems */
	int32_t l4_cache;

#ifndef LIBCPUID_DISABLE_DEPRECATED
	/** Cache associativity for the L1 data cache. -1 if undetermined
	 * @deprecated replaced by \ref cpu_id_t::l1_data_assoc
	 */
	LIBCPUID_DEPRECATED("replace with 'l1_data_assoc' in your code to fix the warning")
	int32_t l1_assoc;
#endif /* LIBCPUID_DISABLE_DEPRECATED */

	/** Cache associativity for the L1 data cache. -1 if undetermined */
	int32_t l1_data_assoc;

	/** Cache associativity for the L1 instruction cache. -1 if undetermined */
	int32_t l1_instruction_assoc;

	/** Cache associativity for the L2 cache. -1 if undetermined */
	int32_t l2_assoc;

	/** Cache associativity for the L3 cache. -1 if undetermined */
	int32_t l3_assoc;

	/** Cache associativity for the L4 cache. -1 if undetermined */
	int32_t l4_assoc;

#ifndef LIBCPUID_DISABLE_DEPRECATED
	/** Cache-line size for L1 data cache. -1 if undetermined
	 * @deprecated replaced by \ref cpu_id_t::l1_data_cacheline
	 */
	LIBCPUID_DEPRECATED("replace with 'l1_data_cacheline' in your code to fix the warning")
	int32_t l1_cacheline;
#endif /* LIBCPUID_DISABLE_DEPRECATED */

	/** Cache-line size for L1 data cache. -1 if undetermined */
	int32_t l1_data_cacheline;

	/** Cache-line size for L1 instruction cache. -1 if undetermined */
	int32_t l1_instruction_cacheline;

	/** Cache-line size for L2 cache. -1 if undetermined */
	int32_t l2_cacheline;

	/** Cache-line size for L3 cache. -1 if undetermined */
	int32_t l3_cacheline;

	/** Cache-line size for L4 cache. -1 if undetermined */
	int32_t l4_cacheline;

	/** Number of L1 data cache instances. -1 if undetermined */
	int32_t l1_data_instances;

	/** Number of L1 instruction cache instances. -1 if undetermined */
	int32_t l1_instruction_instances;

	/** Number of L2 cache instances. -1 if undetermined */
	int32_t l2_instances;

	/** Number of L3 cache instances. -1 if undetermined */
	int32_t l3_instances;

	/** Number of L4 cache instances. -1 if undetermined */
	int32_t l4_instances;

	/**
	 * The brief and human-friendly CPU codename, which was recognized.<br>
	 * Examples:
	 * @code
	 * +--------+--------+-------+-------+-------+---------------------------------------+-----------------------+
	 * | Vendor | Family | Model | Step. | Cache |       Brand String                    | cpu_id_t.cpu_codename |
	 * +--------+--------+-------+-------+-------+---------------------------------------+-----------------------+
	 * | AMD    |      6 |     8 |     0 |   256 | (not available - will be ignored)     | "K6-2"                |
	 * | Intel  |     15 |     2 |     5 |   512 | "Intel(R) Xeon(TM) CPU 2.40GHz"       | "Xeon (Prestonia)"    |
	 * | Intel  |      6 |    15 |    11 |  4096 | "Intel(R) Core(TM)2 Duo CPU E6550..." | "Conroe (Core 2 Duo)" |
	 * | AMD    |     15 |    35 |     2 |  1024 | "Dual Core AMD Opteron(tm) Proces..." | "Opteron (Dual Core)" |
	 * +--------+--------+-------+-------+-------+---------------------------------------+-----------------------+
	 * @endcode
	 */
	char cpu_codename[CODENAME_STR_MAX];

	/**
	 * SSE execution unit size (64 or 128; -1 if N/A)
	 * @deprecated replaced by \ref x86_id_t::sse_size (prefix member with `x86.`, e.g. `id.x86.sse_size`)
	 */
	LIBCPUID_DEPRECATED("replace with '.x86.sse_size' in your code to fix the warning")
	int32_t sse_size;

	/**
	 * contain miscellaneous detection information. Used to test about specifics of
	 * certain detected features. See \ref cpu_hint_t "CPU_HINT_*" macros below.
	 * @see Hints
	 */
	uint8_t detection_hints[CPU_HINTS_MAX];

	/**
	 * contains information about SGX features if the processor, if present
	 * @deprecated replaced by \ref x86_id_t::sgx (prefix member with `x86.`, e.g. `id.x86.sgx`)
	 */
	LIBCPUID_DEPRECATED("replace with '.x86.sgx' in your code to fix the warning")
	struct cpu_sgx_t sgx;

	/** bitmask of the affinity ids this processor type is occupying */
	cpu_affinity_mask_t affinity_mask;

	/** processor type purpose, relevant in case of hybrid CPU (e.g. PURPOSE_PERFORMANCE) */
	cpu_purpose_t purpose;

	/** contains the hypervisor vendor string, e.g. "VBoxVBoxVBox" */
	char hypervisor_str[VENDOR_STR_MAX];

	/** contains the recognized hypervisor vendor */
	hypervisor_vendor_t hypervisor_vendor;
};

/**
 * @brief This contains the recognized features/info for all CPUs on the system
 */
struct system_id_t {
	/** count of different processor types in the system (e.g. performance, efficiency, ...) */
	uint8_t num_cpu_types;

	/** array of recognized CPU features/info for each different processor types in the system */
	struct cpu_id_t* cpu_types;

	/** Number of total L1 data cache instances. -1 if undetermined */
	int32_t l1_data_total_instances;

	/** Number of total L1 instruction cache instances. -1 if undetermined */
	int32_t l1_instruction_total_instances;

	/** Number of total L2 cache instances. -1 if undetermined */
	int32_t l2_total_instances;

	/** Number of total L3 cache instances. -1 if undetermined */
	int32_t l3_total_instances;

	/** Number of total L4 cache instances. -1 if undetermined */
	int32_t l4_total_instances;
};

/**
 * @brief CPU feature identifiers
 *
 * Usage:
 * @code
 * ...
 * struct cpu_raw_data_t raw;
 * struct cpu_id_t id;
 * if (cpuid_get_raw_data(&raw) == 0 && cpu_identify(&raw, &id) == 0) {
 *     if (id.flags[CPU_FEATURE_SSE2]) {
 *         // The CPU has SSE2...
 *         ...
 *     } else {
 *         // no SSE2
 *     }
 * } else {
 *   // processor cannot be determined.
 * }
 * @endcode
 */
typedef enum {
	CPU_FEATURE_FPU = 0,	/*!< Floating point unit */
	CPU_FEATURE_VME,	/*!< Virtual mode extension */
	CPU_FEATURE_DE,		/*!< Debugging extension */
	CPU_FEATURE_PSE,	/*!< Page size extension */
	CPU_FEATURE_TSC,	/*!< Time-stamp counter */
	CPU_FEATURE_MSR,	/*!< Model-specific regsisters, RDMSR/WRMSR supported */
	CPU_FEATURE_PAE,	/*!< Physical address extension */
	CPU_FEATURE_MCE,	/*!< Machine check exception */
	CPU_FEATURE_CX8,	/*!< CMPXCHG8B instruction supported */
	CPU_FEATURE_APIC,	/*!< APIC support */
	CPU_FEATURE_MTRR,	/*!< Memory type range registers */
	CPU_FEATURE_SEP,	/*!< SYSENTER / SYSEXIT instructions supported */
	CPU_FEATURE_PGE,	/*!< Page global enable */
	CPU_FEATURE_MCA,	/*!< Machine check architecture */
	CPU_FEATURE_CMOV,	/*!< CMOVxx instructions supported */
	CPU_FEATURE_PAT,	/*!< Page attribute table */
	CPU_FEATURE_PSE36,	/*!< 36-bit page address extension */
	CPU_FEATURE_PN,		/*!< Processor serial # implemented (Intel P3 only) */
	CPU_FEATURE_CLFLUSH,	/*!< CLFLUSH instruction supported */
	CPU_FEATURE_DTS,	/*!< Debug store supported */
	CPU_FEATURE_ACPI,	/*!< ACPI support (power states) */
	CPU_FEATURE_MMX,	/*!< MMX instruction set supported */
	CPU_FEATURE_FXSR,	/*!< FXSAVE / FXRSTOR supported */
	CPU_FEATURE_SSE,	/*!< Streaming-SIMD Extensions (SSE) supported */
	CPU_FEATURE_SSE2,	/*!< SSE2 instructions supported */
	CPU_FEATURE_SS,		/*!< Self-snoop */
	CPU_FEATURE_HT,		/*!< Hyper-threading supported (but might be disabled) */
	CPU_FEATURE_TM,		/*!< Thermal monitor */
	CPU_FEATURE_IA64,	/*!< IA64 supported (Itanium only) */
	CPU_FEATURE_PBE,	/*!< Pending-break enable */
	CPU_FEATURE_PNI,	/*!< PNI (SSE3) instructions supported */
	CPU_FEATURE_PCLMUL,	/*!< PCLMULQDQ instruction supported */
	CPU_FEATURE_DTS64,	/*!< 64-bit Debug store supported */
	CPU_FEATURE_MONITOR,	/*!< MONITOR / MWAIT supported */
	CPU_FEATURE_DS_CPL,	/*!< CPL Qualified Debug Store */
	CPU_FEATURE_VMX,	/*!< Virtualization technology supported */
	CPU_FEATURE_SMX,	/*!< Safer mode exceptions */
	CPU_FEATURE_EST,	/*!< Enhanced SpeedStep */
	CPU_FEATURE_TM2,	/*!< Thermal monitor 2 */
	CPU_FEATURE_SSSE3,	/*!< SSSE3 instructionss supported (this is different from SSE3!) */
	CPU_FEATURE_CID,	/*!< Context ID supported */
	CPU_FEATURE_CX16,	/*!< CMPXCHG16B instruction supported */
	CPU_FEATURE_XTPR,	/*!< Send Task Priority Messages disable */
	CPU_FEATURE_PDCM,	/*!< Performance capabilities MSR supported */
	CPU_FEATURE_DCA,	/*!< Direct cache access supported */
	CPU_FEATURE_SSE4_1,	/*!< SSE 4.1 instructions supported */
	CPU_FEATURE_SSE4_2,	/*!< SSE 4.2 instructions supported */
	CPU_FEATURE_SYSCALL,	/*!< SYSCALL / SYSRET instructions supported */
	CPU_FEATURE_XD,		/*!< Execute disable bit supported */
	CPU_FEATURE_MOVBE,	/*!< MOVBE instruction supported */
	CPU_FEATURE_POPCNT,	/*!< POPCNT instruction supported */
	CPU_FEATURE_AES,	/*!< AES* instructions supported */
	CPU_FEATURE_XSAVE,	/*!< XSAVE/XRSTOR/etc instructions supported */
	CPU_FEATURE_OSXSAVE,	/*!< non-privileged copy of OSXSAVE supported */
	CPU_FEATURE_AVX,	/*!< Advanced vector extensions supported */
	CPU_FEATURE_MMXEXT,	/*!< AMD MMX-extended instructions supported */
	CPU_FEATURE_3DNOW,	/*!< AMD 3DNow! instructions supported */
	CPU_FEATURE_3DNOWEXT,	/*!< AMD 3DNow! extended instructions supported */
	CPU_FEATURE_NX,		/*!< No-execute bit supported */
	CPU_FEATURE_FXSR_OPT,	/*!< FFXSR: FXSAVE and FXRSTOR optimizations */
	CPU_FEATURE_RDTSCP,	/*!< RDTSCP instruction supported (AMD-only) */
	CPU_FEATURE_LM,		/*!< Long mode (x86_64/EM64T) supported */
	CPU_FEATURE_LAHF_LM,	/*!< LAHF/SAHF supported in 64-bit mode */
	CPU_FEATURE_CMP_LEGACY,	/*!< core multi-processing legacy mode */
	CPU_FEATURE_SVM,	/*!< AMD Secure virtual machine */
	CPU_FEATURE_ABM,	/*!< LZCNT instruction support */
	CPU_FEATURE_MISALIGNSSE,/*!< Misaligned SSE supported */
	CPU_FEATURE_SSE4A,	/*!< SSE 4a from AMD */
	CPU_FEATURE_3DNOWPREFETCH,	/*!< PREFETCH/PREFETCHW support */
	CPU_FEATURE_OSVW,	/*!< OS Visible Workaround (AMD) */
	CPU_FEATURE_IBS,	/*!< Instruction-based sampling */
	CPU_FEATURE_SSE5,	/*!< SSE 5 instructions supported (deprecated, will never be 1) */
	CPU_FEATURE_SKINIT,	/*!< SKINIT / STGI supported */
	CPU_FEATURE_WDT,	/*!< Watchdog timer support */
	CPU_FEATURE_TS,		/*!< Temperature sensor */
	CPU_FEATURE_FID,	/*!< Frequency ID control */
	CPU_FEATURE_VID,	/*!< Voltage ID control */
	CPU_FEATURE_TTP,	/*!< THERMTRIP */
	CPU_FEATURE_TM_AMD,	/*!< AMD-specified hardware thermal control */
	CPU_FEATURE_STC,	/*!< Software thermal control */
	CPU_FEATURE_100MHZSTEPS,/*!< 100 MHz multiplier control */
	CPU_FEATURE_HWPSTATE,	/*!< Hardware P-state control */
	CPU_FEATURE_CONSTANT_TSC,	/*!< TSC ticks at constant rate */
	CPU_FEATURE_XOP,	/*!< The XOP instruction set (same as the old CPU_FEATURE_SSE5) */
	CPU_FEATURE_FMA3,	/*!< The FMA3 instruction set */
	CPU_FEATURE_FMA4,	/*!< The FMA4 instruction set */
	CPU_FEATURE_TBM,	/*!< Trailing bit manipulation instruction support */
	CPU_FEATURE_F16C,	/*!< 16-bit FP convert instruction support */
	CPU_FEATURE_RDRAND,     /*!< RdRand instruction */
	CPU_FEATURE_X2APIC,     /*!< x2APIC, APIC_BASE.EXTD, MSRs 0000_0800h...0000_0BFFh 64-bit ICR (+030h but not +031h), no DFR (+00Eh), SELF_IPI (+040h) also see standard level 0000_000Bh */
	CPU_FEATURE_CPB,	/*!< Core performance boost */
	CPU_FEATURE_APERFMPERF,	/*!< MPERF/APERF MSRs support */
	CPU_FEATURE_PFI,	/*!< Processor Feedback Interface support */
	CPU_FEATURE_PA,		/*!< Processor accumulator */
	CPU_FEATURE_AVX2,	/*!< AVX2 instructions */
	CPU_FEATURE_BMI1,	/*!< BMI1 instructions */
	CPU_FEATURE_BMI2,	/*!< BMI2 instructions */
	CPU_FEATURE_HLE,	/*!< Hardware Lock Elision prefixes */
	CPU_FEATURE_RTM,	/*!< Restricted Transactional Memory instructions */
	CPU_FEATURE_AVX512F,	/*!< AVX-512 Foundation */
	CPU_FEATURE_AVX512DQ,	/*!< AVX-512 Double/Quad granular insns */
	CPU_FEATURE_AVX512PF,	/*!< AVX-512 Prefetch */
	CPU_FEATURE_AVX512ER,	/*!< AVX-512 Exponential/Reciprocal */
	CPU_FEATURE_AVX512CD,	/*!< AVX-512 Conflict detection */
	CPU_FEATURE_SHA_NI,	/*!< SHA-1/SHA-256 instructions */
	CPU_FEATURE_AVX512BW,	/*!< AVX-512 Byte/Word granular insns */
	CPU_FEATURE_AVX512VL,	/*!< AVX-512 128/256 vector length extensions */
	CPU_FEATURE_SGX,	/*!< SGX extensions. Non-autoritative, check cpu_id_t::sgx::present to verify presence */
	CPU_FEATURE_RDSEED,	/*!< RDSEED instruction */
	CPU_FEATURE_ADX,	/*!< ADX extensions (arbitrary precision) */
	CPU_FEATURE_AVX512VNNI, /*!< AVX-512 Vector Neural Network Instructions */
	CPU_FEATURE_AVX512VBMI, /*!< AVX-512 Vector Bit ManipulationInstructions (version 1) */
	CPU_FEATURE_AVX512VBMI2, /*!< AVX-512 Vector Bit ManipulationInstructions (version 2) */
	CPU_FEATURE_HYPERVISOR, /*!< Hypervisor present (always zero on physical CPUs) */
	CPU_FEATURE_INTEL_DTS, /*!< Intel Digital Thermal Sensor */
	CPU_FEATURE_INTEL_PTM, /*!< Intel Package Thermal Management */
	CPU_FEATURE_SWAP, /*!< ARM: Swap instructions in the ARM instruction set */
	CPU_FEATURE_THUMB, /*!< ARM: Thumb instruction set support */
	CPU_FEATURE_ADVMULTU, /*!< ARM: Advanced unsigned Multiply instructions */
	CPU_FEATURE_ADVMULTS, /*!< ARM: Advanced signed Multiply instructions */
	CPU_FEATURE_JAZELLE, /*!< ARM: Jazelle extension support */
	CPU_FEATURE_DEBUGV6, /*!< ARM: Support for v6 Debug architecture */
	CPU_FEATURE_DEBUGV6P1, /*!< ARM: Support for v6.1 Debug architecture */
	CPU_FEATURE_THUMB2, /*!< ARM: Thumb-2, instruction set support */
	CPU_FEATURE_DEBUGV7, /*!< ARM: Support for v7 Debug architecture */
	CPU_FEATURE_DEBUGV7P1, /*!< ARM: Support for v7.1 Debug architecture */
	CPU_FEATURE_THUMBEE, /*!< ARM: ThumbEE instruction set support */
	CPU_FEATURE_DIVIDE, /*!< ARM: Divide instructions */
	CPU_FEATURE_LPAE, /*!< ARM: Large Physical Address Extension */
	CPU_FEATURE_PMUV1, /*!< ARM: PMU extension version 1 */
	CPU_FEATURE_PMUV2, /*!< ARM: PMU extension version 2 */
	CPU_FEATURE_ASID16, /*!< ARM: 16 bit ASID */
	CPU_FEATURE_ADVSIMD, /*!< ARM: Advanced SIMD Extension */
	CPU_FEATURE_CRC32, /*!< ARM: CRC32 instructions */
	CPU_FEATURE_CSV2_1P1, /*!< ARM: Cache Speculation Variant 2 */
	CPU_FEATURE_CSV2_1P2, /*!< ARM: Cache Speculation Variant 2 version 1.2 */
	CPU_FEATURE_CSV2_2, /*!< ARM: Cache Speculation Variant 2 version 2 */
	CPU_FEATURE_CSV2_3, /*!< ARM: Cache Speculation Variant 2 version 3 */
	CPU_FEATURE_DOUBLELOCK, /*!< ARM: Double Lock */
	CPU_FEATURE_ETS2, /*!< ARM: Enhanced Translation Synchronization */
	CPU_FEATURE_FP, /*!< ARM: Floating Point extensions */
	CPU_FEATURE_MIXEDEND, /*!< ARM: Mixed-endian support */
	CPU_FEATURE_MIXEDENDEL0, /*!< ARM: Mixed-endian support at EL0 */
	CPU_FEATURE_PMULL, /*!< ARM: Advanced SIMD PMULL instructions */
	CPU_FEATURE_PMUV3, /*!< ARM: PMU extension version 3 */
	CPU_FEATURE_SHA1, /*!< ARM: Advanced SIMD SHA1 instructions */
	CPU_FEATURE_SHA256, /*!< ARM: Advanced SIMD SHA256 instructions */
	CPU_FEATURE_NTLBPA, /*!< ARM: Intermediate caching of translation table walks */
	CPU_FEATURE_HAFDBS, /*!< ARM: Hardware management of the Access flag and dirty state */
	CPU_FEATURE_HPDS, /*!< ARM: Hierarchical permission disables in translations tables */
	CPU_FEATURE_LOR, /*!< ARM: Limited ordering regions */
	CPU_FEATURE_LSE, /*!< ARM: Large System Extensions */
	CPU_FEATURE_PAN, /*!< ARM: Privileged access never */
	CPU_FEATURE_PMUV3P1, /*!< ARM: Armv8.1 PMU extensions */
	CPU_FEATURE_RDM, /*!< ARM: Advanced SIMD rounding double multiply accumulate instructions */
	CPU_FEATURE_VHE, /*!< ARM: Virtualization Host Extensions */
	CPU_FEATURE_VMID16, /*!< ARM: 16-bit VMID */
	CPU_FEATURE_AA32HPD, /*!< ARM: AArch32 Hierarchical permission disables */
	CPU_FEATURE_AA32I8MM, /*!< ARM: AArch32 Int8 matrix multiplication instructions */
	CPU_FEATURE_DPB, /*!< ARM: DC CVAP instruction */
	CPU_FEATURE_DEBUGV8P2, /*!< ARM: Debug v8.2 */
	CPU_FEATURE_F32MM, /*!< ARM: Single-precision Matrix Multiplication */
	CPU_FEATURE_F64MM, /*!< ARM: Double-precision Matrix Multiplication */
	CPU_FEATURE_FP16, /*!< ARM: Half-precision floating-point data processing */
	CPU_FEATURE_HPDS2, /*!< ARM: Hierarchical permission disables */
	CPU_FEATURE_I8MM, /*!< ARM: AArch64 Int8 matrix multiplication instructions */
	CPU_FEATURE_IESB, /*!< ARM: Implicit Error Synchronization event */
	CPU_FEATURE_LPA, /*!< ARM: Large PA and IPA support */
	CPU_FEATURE_LSMAOC, /*!< ARM: AArch32 Load/Store Multiple instruction atomicity and ordering controls */
	CPU_FEATURE_LVA, /*!< ARM: Large VA support */
	CPU_FEATURE_PAN2, /*!< ARM: AT S1E1R and AT S1E1W instruction variants affected by PSTATE.PAN */
	CPU_FEATURE_RAS, /*!< ARM: Reliability, Availability and Serviceability (RAS) Extension */
	CPU_FEATURE_SHA3, /*!< ARM: Advanced SIMD SHA3 instructions (ARMv8.2 architecture extension) */
	CPU_FEATURE_SHA512, /*!< ARM: Advanced SIMD SHA512 instructions (ARMv8.1 architecture extension) */
	CPU_FEATURE_SM3, /*!< ARM: Advanced SIMD SM3 instructions */
	CPU_FEATURE_SM4, /*!< ARM: Advanced SIMD SM4 instructions */
	CPU_FEATURE_SPE, /*!< ARM: Statistical Profiling Extension */
	CPU_FEATURE_SVE, /*!< ARM: Scalable Vector Extension */
	CPU_FEATURE_TTCNP, /*!< ARM: Translation table Common not private translations */
	CPU_FEATURE_UAO, /*!< ARM: Unprivileged Access Override control */
	CPU_FEATURE_XNX, /*!< ARM: Translation table stage 2 Unprivileged Execute-never */
	CPU_FEATURE_CCIDX, /*!< ARM: Extended cache index */
	CPU_FEATURE_CONSTPACFIELD, /*!< ARM: PAC algorithm enhancement */
	CPU_FEATURE_EPAC, /*!< ARM: Enhanced pointer authentication */
	CPU_FEATURE_FCMA, /*!< ARM: Floating-point complex number instructions */
	CPU_FEATURE_FPAC, /*!< ARM: Faulting on AUT* instructions */
	CPU_FEATURE_FPACCOMBINE, /*!< ARM: Faulting on combined pointer authentication instructions */
	CPU_FEATURE_JSCVT, /*!< ARM: JavaScript conversion instructions */
	CPU_FEATURE_LRCPC, /*!< ARM: Load-Acquire RCpc instructions */
	CPU_FEATURE_PACIMP, /*!< ARM: Pointer authentication - IMPLEMENTATION DEFINED algorithm */
	CPU_FEATURE_PACQARMA3, /*!< ARM: Pointer authentication - QARMA3 algorithm */
	CPU_FEATURE_PACQARMA5, /*!< ARM: Pointer authentication - QARMA5 algorithm */
	CPU_FEATURE_PAUTH, /*!< ARM: Pointer authentication */
	CPU_FEATURE_SPEV1P1, /*!< ARM: Statistical Profiling Extension version 1 */
	CPU_FEATURE_AMUV1, /*!< ARM: Activity Monitors Extension version 1 */
	CPU_FEATURE_BBM, /*!< ARM: Translation table break-before-make levels */
	CPU_FEATURE_DIT, /*!< ARM: Data Independent Timing instructions */
	CPU_FEATURE_DEBUGV8P4, /*!< ARM: Debug v8.4 */
	CPU_FEATURE_DOTPROD, /*!< ARM: Advanced SIMD dot product instructions */
	CPU_FEATURE_DOUBLEFAULT, /*!< ARM: Double Fault Extension */
	CPU_FEATURE_FHM, /*!< ARM: Floating-point half-precision to single-precision multiply-add instructions */
	CPU_FEATURE_FLAGM, /*!< ARM: Condition flag manipulation instructions */
	CPU_FEATURE_IDST, /*!< ARM: ID space trap handling */
	CPU_FEATURE_LRCPC2, /*!< ARM: Load-Acquire RCpc instructions version 2 */
	CPU_FEATURE_LSE2, /*!< ARM: Large System Extensions version 2 */
	CPU_FEATURE_MPAM, /*!< ARM: Memory Partitioning and Monitoring Extension */
	CPU_FEATURE_PMUV3P4, /*!< ARM: Arm8.4 PMU extensions */
	CPU_FEATURE_RASV1P1, /*!< ARM: RAS extension v1.1 */
	CPU_FEATURE_S2FWB, /*!< ARM: Stage 2 forced Write-Back */
	CPU_FEATURE_SEL2, /*!< ARM: Secure EL2 */
	CPU_FEATURE_TLBIOS, /*!< ARM: TLB invalidate instructions in Outer Shareable domain */
	CPU_FEATURE_TLBIRANGE, /*!< ARM: TLB invalidate range instructions */
	CPU_FEATURE_TRF, /*!< ARM: Self-hosted Trace extensions */
	CPU_FEATURE_TTL, /*!< ARM: Translation Table Level */
	CPU_FEATURE_TTST, /*!< ARM: Small translation tables */
	CPU_FEATURE_BTI, /*!< ARM: Branch Target Identification */
	CPU_FEATURE_CSV2, /*!< ARM: Cache Speculation Variant 2 */
	CPU_FEATURE_CSV3, /*!< ARM: Cache Speculation Variant 3 */
	CPU_FEATURE_DPB2, /*!< ARM: DC CVADP instruction */
	CPU_FEATURE_E0PD, /*!< ARM: Preventing EL0 access to halves of address maps */
	CPU_FEATURE_EVT, /*!< ARM: Enhanced Virtualization Traps */
	CPU_FEATURE_EXS, /*!< ARM: Context synchronization and exception handling */
	CPU_FEATURE_FRINTTS, /*!< ARM: Floating-point to integer instructions */
	CPU_FEATURE_FLAGM2, /*!< ARM: Enhancements to flag manipulation instructions */
	CPU_FEATURE_MTE, /*!< ARM: Memory Tagging Extension */
	CPU_FEATURE_MTE2, /*!< ARM: Memory Tagging Extension */
	CPU_FEATURE_PMUV3P5, /*!< ARM: Arm8.5 PMU extensions */
	CPU_FEATURE_RNG, /*!< ARM: Random number generator */
	CPU_FEATURE_RNG_TRAP, /*!< ARM: Trapping support for RNDR/RNDRRS */
	CPU_FEATURE_SB, /*!< ARM: Speculation Barrier */
	CPU_FEATURE_SPECRES, /*!< ARM: Speculation restriction instructions */
	CPU_FEATURE_SSBS, /*!< ARM: Speculative Store Bypass Safe */
	CPU_FEATURE_SSBS2, /*!< ARM: MRS and MSR instructions for SSBS version 2 */
	CPU_FEATURE_AA32BF16, /*!< ARM: AArch32 BFloat16 instructions */
	CPU_FEATURE_AMUV1P1, /*!< ARM: Activity Monitors Extension version 1.1 */
	CPU_FEATURE_BF16, /*!< ARM: AArch64 BFloat16 instructions */
	CPU_FEATURE_DGH, /*!< ARM: Data Gathering Hint */
	CPU_FEATURE_ECV, /*!< ARM: Enhanced Counter Virtualization */
	CPU_FEATURE_FGT, /*!< ARM: Fine Grain Traps */
	CPU_FEATURE_HPMN0, /*!< ARM: Setting of MDCR_EL2.HPMN to zero */
	CPU_FEATURE_MPAMV0P1, /*!< ARM: Memory Partitioning and Monitoring version 0.1 */
	CPU_FEATURE_MPAMV1P1, /*!< ARM: Memory Partitioning and Monitoring version 1.1 */
	CPU_FEATURE_MTPMU, /*!< ARM: Multi-threaded PMU extensions */
	CPU_FEATURE_PAUTH2, /*!< ARM: Enhancements to pointer authentication */
	CPU_FEATURE_TWED, /*!< ARM: Delayed Trapping of WFE */
	CPU_FEATURE_AFP, /*!< ARM: Alternate floating-point behavior */
	CPU_FEATURE_EBF16, /*!< ARM: AArch64 Extended BFloat16 instructions */
	CPU_FEATURE_HCX, /*!< ARM: Support for the HCRX_EL2 register */
	CPU_FEATURE_LPA2, /*!< ARM: Larger physical address for 4KB and 16KB translation granules */
	CPU_FEATURE_LS64, /*!< ARM: Support for 64-byte loads and stores without status */
	CPU_FEATURE_LS64_ACCDATA, /*!< ARM: Support for 64-byte EL0 stores with status */
	CPU_FEATURE_LS64_V, /*!< ARM: Support for 64-byte stores with status */
	CPU_FEATURE_MTE3, /*!< ARM: MTE Asymmetric Fault Handling */
	CPU_FEATURE_MTE_ASYM_FAULT, /*!< ARM: Memory tagging asymmetric faults */
	CPU_FEATURE_PAN3, /*!< ARM: Support for SCTLR_ELx.EPAN */
	CPU_FEATURE_PMUV3P7, /*!< ARM: Armv8.7 PMU extensions */
	CPU_FEATURE_RPRES, /*!< ARM: Increased precision of FRECPE and FRSQRTE */
	CPU_FEATURE_SPEV1P2, /*!< ARM: Statistical Profiling Extensions version 1.2 */
	CPU_FEATURE_WFXT, /*!< ARM: WFE and WFI instructions with timeout */
	CPU_FEATURE_XS, /*!< ARM: XS attribute */
	CPU_FEATURE_CMOW, /*!< ARM: Control for cache maintenance permission */
	CPU_FEATURE_DEBUGV8P8, /*!< ARM: Debug v8.8 */
	CPU_FEATURE_HBC, /*!< ARM: Hinted conditional branches */
	CPU_FEATURE_MOPS, /*!< ARM: Standardization of memory operations */
	CPU_FEATURE_NMI, /*!< ARM: Non-maskable Interrupts */
	CPU_FEATURE_PMUV3P8, /*!< ARM: Armv8.8 PMU extensions */
	CPU_FEATURE_SCTLR2, /*!< ARM: Extension to SCTLR_ELx */
	CPU_FEATURE_SPEV1P3, /*!< ARM: Statistical Profiling Extensions version 1.3 */
	CPU_FEATURE_TCR2, /*!< ARM: Support for TCR2_ELx */
	CPU_FEATURE_TIDCP1, /*!< ARM: EL0 use of IMPLEMENTATION DEFINED functionality */
	CPU_FEATURE_ADERR, /*!< ARM: Asynchronous Device Error Exceptions */
	CPU_FEATURE_AIE, /*!< ARM: Memory Attribute Index Enhancement */
	CPU_FEATURE_ANERR, /*!< ARM: Asynchronous Normal Error Exceptions */
	CPU_FEATURE_ATS1A, /*!< ARM: Address Translation operations that ignore stage 1 permissions */
	CPU_FEATURE_CLRBHB, /*!< ARM: Support for Clear Branch History instruction */
	CPU_FEATURE_CSSC, /*!< ARM: Common Short Sequence Compression instructions */
	CPU_FEATURE_DEBUGV8P9, /*!< ARM: Debug v8.9 */
	CPU_FEATURE_DOUBLEFAULT2, /*!< ARM: Double Fault Extension v2 */
	CPU_FEATURE_ECBHB, /*!< ARM: Exploitative control using branch history information */
	CPU_FEATURE_FGT2, /*!< ARM: Fine-grained traps 2 */
	CPU_FEATURE_HAFT, /*!< ARM: Hardware managed Access Flag for Table descriptors */
	CPU_FEATURE_LRCPC3, /*!< ARM: Load-Acquire RCpc instructions version 3 */
	CPU_FEATURE_MTE4, /*!< ARM: Enhanced Memory Tagging Extension */
	CPU_FEATURE_MTE_ASYNC, /*!< ARM: Asynchronous reporting of Tag Check Fault */
	CPU_FEATURE_MTE_CANONICAL_TAGS, /*!< ARM: Canonical Tag checking for Untagged memory */
	CPU_FEATURE_MTE_NO_ADDRESS_TAGS, /*!< ARM: Memory tagging with Address tagging disabled */
	CPU_FEATURE_MTE_PERM, /*!< ARM: Allocation tag access permission */
	CPU_FEATURE_MTE_STORE_ONLY, /*!< ARM: Store-only Tag Checking */
	CPU_FEATURE_MTE_TAGGED_FAR, /*!< ARM: FAR_ELx on a Tag Check Fault */
	CPU_FEATURE_PFAR, /*!< ARM: Physical Fault Address Register Extension */
	CPU_FEATURE_PMUV3_ICNTR, /*!< ARM: Fixed-function instruction counter */
	CPU_FEATURE_PMUV3_SS, /*!< ARM: PMU Snapshot extension */
	CPU_FEATURE_PMUV3P9, /*!< ARM: Armv8.9 PMU extensions */
	CPU_FEATURE_PRFMSLC, /*!< ARM: SLC target support for PRFM instructions */
	CPU_FEATURE_RASV2, /*!< ARM: RAS Extension v2 */
	CPU_FEATURE_RPRFM, /*!< ARM: Support for Range Prefetch Memory instruction */
	CPU_FEATURE_S1PIE, /*!< ARM: Stage 1 permission indirections */
	CPU_FEATURE_S1POE, /*!< ARM: Stage 1 permission overlays */
	CPU_FEATURE_S2PIE, /*!< ARM: Stage 2 permission indirections */
	CPU_FEATURE_S2POE, /*!< ARM: Stage 1 permission overlays */
	CPU_FEATURE_SPECRES2, /*!< ARM: Enhanced speculation restriction instructions */
	CPU_FEATURE_SPE_DPFZS, /*!< ARM: Disable Cycle Counter on SPE Freeze */
	CPU_FEATURE_SPEV1P4, /*!< ARM: Statistical Profiling Extension version 1.4 */
	CPU_FEATURE_SPMU, /*!< ARM: System Performance Monitors Extension */
	CPU_FEATURE_THE, /*!< ARM: Translation Hardening Extension */
	CPU_FEATURE_SVE2, /*!< ARM: Scalable Vector Extension version 2 */
	CPU_FEATURE_SVE_AES, /*!< ARM: Scalable Vector AES instructions */
	CPU_FEATURE_SVE_BITPERM, /*!< ARM: Scalable Vector Bit Permutes instructions */
	CPU_FEATURE_SVE_PMULL128, /*!< ARM: Scalable Vector PMULL instructions */
	CPU_FEATURE_SVE_SHA3, /*!< ARM: Scalable Vector SHA3 instructions */
	CPU_FEATURE_SVE_SM4, /*!< ARM: Scalable Vector SM4 instructions */
	CPU_FEATURE_TME, /*!< ARM: Transactional Memory Extension */
	CPU_FEATURE_TRBE, /*!< ARM: Trace Buffer Extension */
	CPU_FEATURE_BRBE, /*!< ARM: Branch Record Buffer Extension */
	CPU_FEATURE_RME, /*!< ARM: Realm Management Extension */
	CPU_FEATURE_SME, /*!< ARM: Scalable Matrix Extension */
	CPU_FEATURE_SME_F64F64, /*!< ARM: Double-precision floating-point outer product instructions */
	CPU_FEATURE_SME_FA64, /*!< ARM: Full A64 instruction set support in Streaming SVE mode */
	CPU_FEATURE_SME_I16I64, /*!< ARM: 16-bit to 64-bit integer widening outer product instructions */
	CPU_FEATURE_BRBEV1P1, /*!< ARM: Branch Record Buffer Extension version 1.1 */
	CPU_FEATURE_MEC, /*!< ARM: Memory Encryption Contexts */
	CPU_FEATURE_SME2, /*!< ARM: Scalable Matrix Extensions version 2 */
	CPU_FEATURE_ABLE, /*!< ARM: Address Breakpoint Linking Extension */
	CPU_FEATURE_BWE, /*!< ARM: Breakpoint and watchpoint enhancements */
	CPU_FEATURE_D128, /*!< ARM: 128-bit Translation Tables, 56 bit PA */
	CPU_FEATURE_EBEP, /*!< ARM: Exception-based Event Profiling */
	CPU_FEATURE_GCS, /*!< ARM: Guarded Control Stack Extension */
	CPU_FEATURE_ITE, /*!< ARM: Instrumentation Trace Extension */
	CPU_FEATURE_LSE128, /*!< ARM: 128-bit Atomics */
	CPU_FEATURE_LVA3, /*!< ARM: 56-bit VA */
	CPU_FEATURE_SEBEP, /*!< ARM: Synchronous Exception-based Event Profiling */
	CPU_FEATURE_SME2P1, /*!< ARM: Scalable Matrix Extension version 2.1 */
	CPU_FEATURE_SME_F16F16, /*!< ARM: Non-widening half-precision FP16 to FP16 arithmetic for SME2. */
	CPU_FEATURE_SVE2P1, /*!< ARM: Scalable Vector Extensions version 2.1 */
	CPU_FEATURE_SVE_B16B16, /*!< ARM: Non-widening BFloat16 to BFloat16 arithmetic for SVE2 and SME2. */
	CPU_FEATURE_SYSINSTR128, /*!< ARM: 128-bit System instructions */
	CPU_FEATURE_SYSREG128, /*!< ARM: 128-bit System registers */
	CPU_FEATURE_TRBE_EXT, /*!< ARM: Trace Buffer external mode */
	/* termination: */
	NUM_CPU_FEATURES,
} cpu_feature_t;

/**
 * @brief CPU detection hints identifiers
 *
 * Usage: similar to the flags usage
 */
typedef enum {
	CPU_HINT_SSE_SIZE_AUTH = 0,	/*!< SSE unit size is authoritative (not only a Family/Model guesswork, but based on an actual CPUID bit) */
	/* termination */
	NUM_CPU_HINTS,
} cpu_hint_t;

/**
 * @brief SGX features flags
 * \see cpu_sgx_t
 *
 * Usage:
 * @code
 * ...
 * struct cpu_raw_data_t raw;
 * struct cpu_id_t id;
 * if (cpuid_get_raw_data(&raw) == 0 && cpu_identify(&raw, &id) == 0 && id.sgx.present) {
 *     if (id.sgx.flags[INTEL_SGX1])
 *         // The CPU has SGX1 instructions support...
 *         ...
 *     } else {
 *         // no SGX
 *     }
 * } else {
 *   // processor cannot be determined.
 * }
 * @endcode
 */

typedef enum {
	INTEL_SGX1,		/*!< SGX1 instructions support */
	INTEL_SGX2,		/*!< SGX2 instructions support */

	/* termination: */
	NUM_SGX_FEATURES,
} cpu_sgx_feature_t;

/**
 * @brief Describes common library error codes
 */
typedef enum {
	ERR_OK       =  0,	/*!< No error */
	ERR_NO_CPUID = -1,	/*!< CPUID instruction is not supported */
	ERR_NO_RDTSC = -2,	/*!< RDTSC instruction is not supported */
	ERR_NO_MEM   = -3,	/*!< Memory allocation failed */
	ERR_OPEN     = -4,	/*!< File open operation failed */
	ERR_BADFMT   = -5,	/*!< Bad file format */
	ERR_NOT_IMP  = -6,	/*!< Not implemented */
	ERR_CPU_UNKN = -7,	/*!< Unsupported processor */
	ERR_NO_RDMSR = -8,	/*!< RDMSR instruction is not supported */
	ERR_NO_DRIVER= -9,	/*!< RDMSR driver error (generic) */
	ERR_NO_PERMS = -10,	/*!< No permissions to install RDMSR driver */
	ERR_EXTRACT  = -11,	/*!< Cannot extract RDMSR driver (read only media?) */
	ERR_HANDLE   = -12,	/*!< Bad handle */
	ERR_INVMSR   = -13,	/*!< Invalid MSR */
	ERR_INVCNB   = -14,	/*!< Invalid core number */
	ERR_HANDLE_R = -15,	/*!< Error on handle read */
	ERR_INVRANGE = -16,	/*!< Invalid given range */
	ERR_NOT_FOUND= -17,	/*!< Requested type not found */
	ERR_IOCTL    = -18,	/*!< Error on ioctl */
	ERR_REQUEST  = -19,	/*!< Invalid request */
} cpu_error_t;

/**
 * @brief Internal structure, used in cpu_tsc_mark, cpu_tsc_unmark and
 *        cpu_clock_by_mark
 */
struct cpu_mark_t {
	uint64_t tsc;		/*!< Time-stamp from RDTSC */
	uint64_t sys_clock;	/*!< In microsecond resolution */
};

/**
 * @brief Returns the total number of logical CPU threads (even if CPUID is not present).
 *
 * Under VM, this number (and total_logical_cpus, since they are fetched with the same code)
 * may be nonsensical, i.e. might not equal NumPhysicalCPUs*NumCoresPerCPU*HyperThreading.
 * This is because no matter how many logical threads the host machine has, you may limit them
 * in the VM to any number you like. **This** is the number returned by cpuid_get_total_cpus().
 *
 * @returns Number of logical CPU threads available. Equals the \ref cpu_id_t::total_logical_cpus.
 */
int cpuid_get_total_cpus(void);

/**
 * @brief Checks if the CPUID instruction is supported
 * @retval 1 if CPUID is present
 * @retval 0 the CPU doesn't have CPUID.
 */
int cpuid_present(void);

/**
 * @brief Executes the CPUID instruction
 * @param eax - the value of the EAX register when executing CPUID
 * @param regs - the results will be stored here. regs[0] = EAX, regs[1] = EBX, ...
 * @note CPUID will be executed with EAX set to the given value and EBX, ECX,
 *       EDX set to zero.
 */
void cpu_exec_cpuid(uint32_t eax, uint32_t* regs);

/**
 * @brief Executes the CPUID instruction with the given input registers
 * @note This is just a bit more generic version of cpu_exec_cpuid - it allows
 *       you to control all the registers.
 * @param regs - Input/output. Prior to executing CPUID, EAX, EBX, ECX and
 *               EDX will be set to regs[0], regs[1], regs[2] and regs[3].
 *               After CPUID, this array will contain the results.
 */
void cpu_exec_cpuid_ext(uint32_t* regs);

/**
 * @brief Obtains the raw CPUID data from the current CPU
 * @param data - a pointer to cpu_raw_data_t structure
 * @returns zero if successful, and some negative number on error.
 *          The error message can be obtained by calling \ref cpuid_error.
 *          @see cpu_error_t
 */
int cpuid_get_raw_data(struct cpu_raw_data_t* data);

/**
 * @brief Obtains the raw CPUID data from the specified CPU
 * @param data - a pointer to cpu_raw_data_t structure
 * @param logical_cpu specify the core number.
 *          The first core number is 0.
 *          The last core number is \ref cpuid_get_total_cpus - 1.
 * @returns zero if successful, and some negative number on error.
 *          The error message can be obtained by calling \ref cpuid_error.
 *          @see cpu_error_t
 */
int cpuid_get_raw_data_core(struct cpu_raw_data_t* data, logical_cpu_t logical_cpu);

/**
 * @brief Obtains the raw CPUID data from all CPUs
 * @param data - a pointer to cpu_raw_data_array_t structure
 * @note As the memory is dynamically allocated, be sure to call
 *       cpuid_free_raw_data_array() after you're done with the data
 * @returns zero if successful, and some negative number on error.
 *          The error message can be obtained by calling \ref cpuid_error.
 *          @see cpu_error_t
 */
int cpuid_get_all_raw_data(struct cpu_raw_data_array_t* data);

/**
 * @brief Identifies the CPU
 * @param raw - Input - a pointer to the raw CPUID data, which is obtained
 *              either by cpuid_get_raw_data or cpuid_deserialize_raw_data.
 *              Can also be NULL, in which case the functions calls
 *              cpuid_get_raw_data itself.
 * @param data - Output - the decoded CPU features/info is written here.
 * @note The function will not fail, even if some of the information
 *       cannot be obtained. Even when the CPU is new and thus unknown to
 *       libcpuid, some generic info, such as "AMD K9 family CPU" will be
 *       written to data.cpu_codename, and most other things, such as the
 *       CPU flags, cache sizes, etc. should be detected correctly anyway.
 *       However, the function CAN fail, if the CPU is completely alien to
 *       libcpuid.
 * @note While cpu_identify() and cpuid_get_raw_data() are fast for most
 *       purposes, running them several thousand times per second can hamper
 *       performance significantly. Specifically, avoid writing "cpu feature
 *       checker" wrapping function, which calls cpu_identify and returns the
 *       value of some flag, if that function is going to be called frequently.
 * @returns zero if successful, and some negative number on error.
 *          The error message can be obtained by calling \ref cpuid_error.
 *          @see cpu_error_t
 */
int cpu_identify(struct cpu_raw_data_t* raw, struct cpu_id_t* data);

/**
 * @brief Identifies all the CPUs
 * @param raw_array - Input - a pointer to the array of raw CPUID data, which is obtained
 *              either by cpuid_get_all_raw_data or cpuid_deserialize_all_raw_data.
 *              Can also be NULL, in which case the functions calls
 *              cpuid_get_all_raw_data itself.
 * @param system - Output - the decoded CPU features/info is written here for each CPU type.
 * @note The function is similar to cpu_identify. Refer to cpu_identify notes.
 * @note As the memory is dynamically allocated, be sure to call
 *       cpuid_free_raw_data_array() and cpuid_free_system_id() after you're done with the data
 * @returns zero if successful, and some negative number on error.
 *          The error message can be obtained by calling \ref cpuid_error.
 *          @see cpu_error_t
 */
int cpu_identify_all(struct cpu_raw_data_array_t* raw_array, struct system_id_t* system);

/**
 * @brief Identifies a given CPU type
 * @param purpose - Input - a \ref cpu_purpose_t to request
 * @param raw_array - Optional input - a pointer to the array of raw CPUID data, which is obtained
 *              either by cpuid_get_all_raw_data or cpuid_deserialize_all_raw_data.
 *              Can also be NULL, in which case the functions calls
 *              cpuid_get_all_raw_data itself.
 * @param data - Output - the decoded CPU features/info is written here.
 * @returns zero if successful, and some negative number on error (like ERR_NOT_FOUND if CPU type not found).
 *          The error message can be obtained by calling \ref cpuid_error.
 *          @see cpu_error_t
 */
int cpu_request_core_type(cpu_purpose_t purpose, struct cpu_raw_data_array_t* raw_array, struct cpu_id_t* data);

/**
 * @brief Returns the short textual representation of a CPU architecture
 * @param architecture - the architecture, whose textual representation is wanted.
 * @returns a constant string like "x86", "ARM", etc.
 */
const char* cpu_architecture_str(cpu_architecture_t architecture);

/**
 * @brief Returns the short textual representation of a CPU feature level
 * @param level - the feature level, whose textual representation is wanted.
 * @returns a constant string like "ARMv8.0-A", "ARMv9.4-A", etc.
 */
const char* cpu_feature_level_str(cpu_feature_level_t level);

/**
 * @brief Returns the short textual representation of a CPU purpose
 * @param purpose - the purpose, whose textual representation is wanted.
 * @returns a constant string like "general", "performance", "efficiency", etc.
 */
const char* cpu_purpose_str(cpu_purpose_t purpose);

/**
 * @brief Returns textual representation of a CPU affinity mask (thread-safe)
 * @param affinity_mask - Input - the affinity mask (in hexadecimal), whose textual representation is wanted.
 * @param buffer - Output - an allocated string where to store the textual representation, like "0000FFFF", "00FF0000", etc.
 * @param buffer_len - Input - the size of buffer.
 * @returns a pointer on buffer
 */
char* affinity_mask_str_r(cpu_affinity_mask_t* affinity_mask, char* buffer, uint32_t buffer_len);

/**
 * @brief Returns textual representation of a CPU affinity mask
 * @param affinity_mask - the affinity mask (in hexadecimal), whose textual representation is wanted.
 * @note This function is not thread-safe
 * @returns a string like "0000FFFF", "00FF0000", etc.
 */
char* affinity_mask_str(cpu_affinity_mask_t *affinity_mask);

bool save_cpu_affinity();

bool restore_cpu_affinity();

bool set_cpu_affinity(logical_cpu_t logical_cpu);

/**
 * @brief Returns the short textual representation of a CPU flag
 * @param feature - the feature, whose textual representation is wanted.
 * @returns a constant string like "fpu", "tsc", "sse2", etc.
 * @note the names of the returned flags are compatible with those from
 *       /proc/cpuinfo in Linux, with the exception of `tm_amd'
 */
const char* cpu_feature_str(cpu_feature_t feature);

/**
 * @brief Returns textual description of the last error
 *
 * libcpuid stores an `errno'-style error status, whose description
 * can be obtained with this function.
 * @note This function is not thread-safe
 * @see cpu_error_t
 */
const char* cpuid_error(void);

/**
 * @brief Executes RDTSC
 *
 * The RDTSC (ReaD Time Stamp Counter) instruction gives access to an
 * internal 64-bit counter, which usually increments at each clock cycle.
 * This can be used for various timing routines, and as a very precise
 * clock source. It is set to zero on system startup. Beware that may not
 * increment at the same frequency as the CPU. Consecutive calls of RDTSC
 * are, however, guaranteed to return monotonically-increasing values.
 *
 * @param result - a pointer to a 64-bit unsigned integer, where the TSC value
 *                 will be stored
 *
 * @note  If 100% compatibility is a concern, you must first check if the
 * RDTSC instruction is present (if it is not, your program will crash
 * with "invalid opcode" exception). Only some very old processors (i486,
 * early AMD K5 and some Cyrix CPUs) lack that instruction - they should
 * have become exceedingly rare these days. To verify RDTSC presence,
 * run cpu_identify() and check flags[CPU_FEATURE_TSC].
 *
 * @note The monotonically increasing nature of the TSC may be violated
 * on SMP systems, if their TSC clocks run at different rate. If the OS
 * doesn't account for that, the TSC drift may become arbitrary large.
 */
void cpu_rdtsc(uint64_t* result);

/**
 * @brief Store TSC and timing info
 *
 * This function stores the current TSC value and current
 * time info from a precise OS-specific clock source in the cpu_mark_t
 * structure. The sys_clock field contains time with microsecond resolution.
 * The values can later be used to measure time intervals, number of clocks,
 * FPU frequency, etc.
 * @see cpu_rdtsc
 *
 * @param mark [out] - a pointer to a cpu_mark_t structure
 */
void cpu_tsc_mark(struct cpu_mark_t* mark);

/**
 * @brief Calculate TSC and timing difference
 *
 * @param mark - input/output: a pointer to a cpu_mark_t structure, which has
 *               already been initialized by cpu_tsc_mark. The difference in
 *               TSC and time will be written here.
 *
 * This function calculates the TSC and time difference, by obtaining the
 * current TSC and timing values and subtracting the contents of the `mark'
 * structure from them. Results are written in the same structure.
 *
 * Example:
 * @code
 * ...
 * struct cpu_mark_t mark;
 * cpu_tsc_mark(&mark);
 * foo();
 * cpu_tsc_unmark(&mark);
 * printf("Foo finished. Executed in %llu cycles and %llu usecs\n",
 *        mark.tsc, mark.sys_clock);
 * ...
 * @endcode
 */
void cpu_tsc_unmark(struct cpu_mark_t* mark);

/**
 * @brief Calculates the CPU clock
 *
 * @param mark - pointer to a cpu_mark_t structure, which has been initialized
 *   with cpu_tsc_mark and later `stopped' with cpu_tsc_unmark.
 *
 * @note For reliable results, the marked time interval should be at least about
 * 10 ms.
 *
 * @returns the CPU clock frequency, in MHz. Due to measurement error, it will
 * differ from the true value in a few least-significant bits. Accuracy depends
 * on the timing interval - the more, the better. If the timing interval is
 * insufficient, the result is -1. Also, see the comment on cpu_clock_measure
 * for additional issues and pitfalls in using RDTSC for CPU frequency
 * measurements.
 */
int cpu_clock_by_mark(struct cpu_mark_t* mark);

/**
 * @brief Returns the CPU clock, as reported by the OS
 *
 * This function uses OS-specific functions to obtain the CPU clock. It may
 * differ from the true clock for several reasons:<br><br>
 *
 * i) The CPU might be in some power saving state, while the OS reports its
 *    full-power frequency, or vice-versa.<br>
 * ii) In some cases you can raise or lower the CPU frequency with overclocking
 *     utilities and the OS will not notice.
 *
 * @returns the CPU clock frequency in MHz. If the OS is not (yet) supported
 * or lacks the necessary reporting machinery, the return value is -1
 */
int cpu_clock_by_os(void);

/**
 * @brief Measure the CPU clock frequency
 *
 * @param millis - How much time to waste in the busy-wait cycle. In millisecs.
 *                 Useful values 10 - 1000
 * @param quad_check - Do a more thorough measurement if nonzero
 *                     (see the explanation).
 *
 * The function performs a busy-wait cycle for the given time and calculates
 * the CPU frequency by the difference of the TSC values. The accuracy of the
 * calculation depends on the length of the busy-wait cycle: more is better,
 * but 100ms should be enough for most purposes.
 *
 * While this will calculate the CPU frequency correctly in most cases, there are
 * several reasons why it might be incorrect:<br>
 *
 * i) RDTSC doesn't guarantee it will run at the same clock as the CPU.
 *    Apparently there aren't CPUs at the moment, but still, there's no
 *    guarantee.<br>
 * ii) The CPU might be in a low-frequency power saving mode, and the CPU
 *     might be switched to higher frequency at any time. If this happens
 *     during the measurement, the result can be anywhere between the
 *     low and high frequencies. Also, if you're interested in the
 *     high frequency value only, this function might return the low one
 *     instead.<br>
 * iii) On SMP systems exhibiting TSC drift (see \ref cpu_rdtsc)
 *
 * the quad_check option will run four consecutive measurements and
 * then return the average of the two most-consistent results. The total
 * runtime of the function will still be `millis' - consider using
 * a bit more time for the timing interval.
 *
 * Finally, for benchmarking / CPU intensive applications, the best strategy is
 * to use the cpu_tsc_mark() / cpu_tsc_unmark() / cpu_clock_by_mark() method.
 * Begin by mark()-ing about one second after application startup (allowing the
 * power-saving manager to kick in and rise the frequency during that time),
 * then unmark() just before application finishing. The result will most
 * acurately represent at what frequency your app was running.
 *
 * @returns the CPU clock frequency in MHz (within some measurement error
 * margin). If RDTSC is not supported, the result is -1.
 */
int cpu_clock_measure(int millis, int quad_check);

/**
 * @brief Measure the CPU clock frequency using instruction-counting
 *
 * @param millis - how much time to allocate for each run, in milliseconds
 * @param runs - how many runs to perform
 *
 * The function performs a busy-wait cycle using a known number of "heavy" (SSE)
 * instructions. These instructions run at (more or less guaranteed) 1 IPC rate,
 * so by running a busy loop for a fixed amount of time, and measuring the
 * amount of instructions done, the CPU clock is accurately measured.
 *
 * Of course, this function is still affected by the power-saving schemes, so
 * the warnings as of cpu_clock_measure() still apply. However, this function is
 * immune to problems with detection, related to the Intel Nehalem's "Turbo"
 * mode, where the internal clock is raised, but the RDTSC rate is unaffected.
 *
 * The function will run for about (millis * runs) milliseconds.
 * You can make only a single busy-wait run (runs == 1); however, this can
 * be affected by task scheduling (which will break the counting), so allowing
 * more than one run is recommended. As run length is not imperative for
 * accurate readings (e.g., 50ms is sufficient), you can afford a lot of short
 * runs, e.g. 10 runs of 50ms or 20 runs of 25ms.
 *
 * Recommended values - millis = 50, runs = 4. For more robustness,
 * increase the number of runs.
 *
 * NOTE: on Bulldozer and later CPUs, the busy-wait cycle runs at 1.4 IPC, thus
 * the results are skewed. This is corrected internally by dividing the resulting
 * value by 1.4.
 * However, this only occurs if the thread is executed on a single CMT
 * module - if there are other threads competing for resources, the results are
 * unpredictable. Make sure you run cpu_clock_by_ic() on a CPU that is free from
 * competing threads, or if there are such threads, they shouldn't exceed the
 * number of modules. On a Bulldozer X8, that means 4 threads.
 *
 * @returns the CPU clock frequency in MHz (within some measurement error
 * margin). If SSE is not supported, the result is -1. If the input parameters
 * are incorrect, or some other internal fault is detected, the result is -2.
 */
int cpu_clock_by_ic(int millis, int runs);

/**
 * @brief Measure the CPU clock frequency using TSC frequency from CPUID
 *
 * @param raw - Optional input - a pointer to the raw CPUID data, which is obtained
 *              either by cpuid_get_raw_data or cpuid_deserialize_raw_data.
 *              Can also be NULL, in which case the functions calls
 *              cpuid_get_raw_data itself.
 *
 * The function read Time Stamp Counter and Nominal Core Crystal Clock
 * Information Leaf from CPUID. It determines the processor base frequency.
 *
 * NOTE: only x86 Intel CPUs since Skylake (6th generation of Intel Core
 * processors) are supported. Other vendors do not support this feature.
 *
 * @returns the CPU clock frequency in MHz.
 * If TSC frequency is not supported, the result is -1.
 * If the input parameters are incorrect, or some other internal fault is
 * detected, the result is -2.
 */
int cpu_clock_by_tsc(struct cpu_raw_data_t* raw);

/**
 * @brief Get the CPU clock frequency (all-in-one method)
 *
 * This is an all-in-one method for getting the CPU clock frequency.
 * It tries to use the OS for that. If the OS doesn't have this info, it
 * uses cpu_clock_measure with 200ms time interval and quadruple checking.
 *
 * @returns the CPU clock frequency in MHz. If every possible method fails,
 * the result is -1.
 */
int cpu_clock(void);


/**
 * @brief The return value of cpuid_get_epc().
 * @details
 * Describes an EPC (Enclave Page Cache) layout (physical address and size).
 * A CPU may have one or more EPC areas, and information about each is
 * fetched via \ref cpuid_get_epc.
 */
struct cpu_epc_t {
	uint64_t start_addr;
	uint64_t length;
};

/**
 * @brief Fetches information about an EPC (Enclave Page Cache) area.
 * @param index - zero-based index, valid range [0..cpu_id_t.egx.num_epc_sections)
 * @param raw   - a pointer to fetched raw CPUID data. Needed only for testing,
 *                you can safely pass NULL here (if you pass a real structure,
 *                it will be used for fetching the leaf 12h data if index < 2;
 *                otherwise the real CPUID instruction will be used).
 * @returns the requested data. If the CPU doesn't support SGX, or if
 *          index >= cpu_id_t.egx.num_epc_sections, both fields of the returned
 *          structure will be zeros.
 */
struct cpu_epc_t cpuid_get_epc(int index, const struct cpu_raw_data_t* raw);

/**
 * @brief Returns the libcpuid version
 *
 * @returns the string representation of the libcpuid version, like "0.1.1"
 */
const char* cpuid_lib_version(void);

/**
 * @brief Obtains the CPU vendor from CPUID from the current CPU
 * @note The result is cached.
 * @returns VENDOR_UNKNOWN if failed, otherwise the CPU vendor type.
 *          @see cpu_vendor_t
 */
cpu_vendor_t cpuid_get_vendor(void);

/**
 * @brief Frees a raw array
 *
 * This function deletes all the memory associated with a raw array, as obtained
 * by cpuid_get_all_raw_data(), cpuid_deserialize_all_raw_data() and cpu_identify_all()
 *
 * @param raw_array - the raw array to be free()'d.
 */
void cpuid_free_raw_data_array(struct cpu_raw_data_array_t* raw_array);

/**
 * @brief Frees a system ID type
 *
 * This function deletes all the memory associated with a system ID, as obtained
 * by cpu_identify_all()
 *
 * @param system - the system ID to be free()'d.
 */
void cpuid_free_system_id(struct system_id_t* system);

typedef enum {
	INFO_MPERF,                /*!< Maximum performance frequency clock. This
                                    is a counter, which increments as a
                                    proportion of the actual processor speed. */
	INFO_APERF,                /*!< Actual performance frequency clock. This
                                    accumulates the core clock counts when the
                                    core is active. */
	INFO_MIN_MULTIPLIER,       /*!< Minimum CPU:FSB ratio for this CPU,
                                    multiplied by 100. */
	INFO_CUR_MULTIPLIER,       /*!< Current CPU:FSB ratio, multiplied by 100.
                                    e.g., a CPU:FSB value of 18.5 reads as
                                    "1850". */
	INFO_MAX_MULTIPLIER,       /*!< Maximum CPU:FSB ratio for this CPU,
                                    multiplied by 100. */
	INFO_TEMPERATURE,          /*!< The current core temperature in Celsius. */
	INFO_PKG_TEMPERATURE,      /*!< The current package temperature in Celsius. */
	INFO_THROTTLING,           /*!< 1 if the current logical processor is
                                    throttling. 0 if it is running normally. */
	INFO_VOLTAGE,              /*!< The current core voltage in Volt,
	                            multiplied by 100. */
	INFO_PKG_ENERGY,            /*!< The current package energy consumption in Joules,
	                            multiplied by 100. */
	INFO_PKG_POWER,             /*!< The current package power in Watts,
	                            multiplied by 100. */
	INFO_PKG_PL1,               /*!< The current package power limit #1 in Watts,
	                            multiplied by 100. */
	INFO_PKG_PL2,               /*!< The current package power limit #2 in Watts,
	                            multiplied by 100. */
	INFO_BCLK,                 /*!< See \ref INFO_BUS_CLOCK. */
	INFO_BUS_CLOCK,            /*!< The main bus clock in MHz,
	                            e.g., FSB/QPI/DMI/HT base clock,
	                            multiplied by 100. */
} cpu_msrinfo_request_t;

/**
 * @brief Similar to \ref cpu_rdmsr, but extract a range of bits
 *
 * @param handle - a handle to the MSR reader driver, as created by
 *                 cpu_msr_driver_open
 * @param msr_index - the numeric ID of the MSR you want to read
 * @param highbit - the high bit in range, must be inferior to 64
 * @param lowbit - the low bit in range, must be equal or superior to 0
 * @param result - a pointer to a 64-bit integer, where the MSR value is stored
 *
 * @returns zero if successful, and some negative number on error.
 *          The error message can be obtained by calling \ref cpuid_error.
 *          @see cpu_error_t
 */
int cpu_rdmsr_range(struct wr0_drv_t* handle, uint32_t msr_index, uint8_t highbit,
                    uint8_t lowbit, uint64_t* result);

/**
 * @brief Reads extended CPU information from Model-Specific Registers.
 * @param handle - a handle to an open MSR driver, @see cpu_msr_driver_open
 * @param which - which info field should be returned. A list of
 *                available information entities is listed in the
 *                cpu_msrinfo_request_t enum.
 * @retval - if the requested information is available for the current
 *           processor model, the respective value is returned.
 *           if no information is available, or the CPU doesn't support
 *           the query, the special value CPU_INVALID_VALUE is returned
 * @note This function is not MT-safe. If you intend to call it from multiple
 *       threads, guard it through a mutex or a similar primitive.
 */
int cpu_msrinfo(struct wr0_drv_t* handle, cpu_msrinfo_request_t which);
#define CPU_INVALID_VALUE 0x3fffffff

#ifdef __cplusplus
} /* extern "C" */
#endif


/** @} */

#endif /* __LIBCPUID_H__ */
