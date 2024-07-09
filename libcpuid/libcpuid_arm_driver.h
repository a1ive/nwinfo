/*
 * Copyright 2024  Veselin Georgiev,
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

#ifndef __LIBCPUID_ARM_DRIVER_H__
#define __LIBCPUID_ARM_DRIVER_H__

#define AARCH64_REG_MIDR_EL1         "S3_0_C0_C0_0"
#define AARCH64_REG_MPIDR_EL1        "S3_0_C0_C0_5"
#define AARCH64_REG_REVIDR_EL1       "S3_0_C0_C0_6"
#define AARCH64_REG_ID_AFR0          "S3_0_C0_C1_3"
#define AARCH64_REG_ID_DFR0          "S3_0_C0_C1_2"
#define AARCH64_REG_ID_DFR1          "S3_0_C0_C3_5"
#define AARCH64_REG_ID_ISAR0         "S3_0_C0_C2_0"
#define AARCH64_REG_ID_ISAR1         "S3_0_C0_C2_1"
#define AARCH64_REG_ID_ISAR2         "S3_0_C0_C2_2"
#define AARCH64_REG_ID_ISAR3         "S3_0_C0_C2_3"
#define AARCH64_REG_ID_ISAR4         "S3_0_C0_C2_4"
#define AARCH64_REG_ID_ISAR5         "S3_0_C0_C2_5"
#define AARCH64_REG_ID_ISAR6         "S3_0_C0_C2_7"
#define AARCH64_REG_ID_MMFR0         "S3_0_C0_C1_4"
#define AARCH64_REG_ID_MMFR1         "S3_0_C0_C1_5"
#define AARCH64_REG_ID_MMFR2         "S3_0_C0_C1_6"
#define AARCH64_REG_ID_MMFR3         "S3_0_C0_C1_7"
#define AARCH64_REG_ID_MMFR4         "S3_0_C0_C2_6"
#define AARCH64_REG_ID_MMFR5         "S3_0_C0_C3_6"
#define AARCH64_REG_ID_PFR0          "S3_0_C0_C1_0"
#define AARCH64_REG_ID_PFR1          "S3_0_C0_C1_1"
#define AARCH64_REG_ID_PFR2          "S3_0_C0_C3_4"
#define AARCH64_REG_ID_AA64AFR0_EL1  "S3_0_C0_C5_4"
#define AARCH64_REG_ID_AA64AFR1_EL1  "S3_0_C0_C5_5"
#define AARCH64_REG_ID_AA64DFR0_EL1  "S3_0_C0_C5_0"
#define AARCH64_REG_ID_AA64DFR1_EL1  "S3_0_C0_C5_1"
#define AARCH64_REG_ID_AA64ISAR0_EL1 "S3_0_C0_C6_0"
#define AARCH64_REG_ID_AA64ISAR1_EL1 "S3_0_C0_C6_1"
#define AARCH64_REG_ID_AA64ISAR2_EL1 "S3_0_C0_C6_2"
#define AARCH64_REG_ID_AA64MMFR0_EL1 "S3_0_C0_C7_0"
#define AARCH64_REG_ID_AA64MMFR1_EL1 "S3_0_C0_C7_1"
#define AARCH64_REG_ID_AA64MMFR2_EL1 "S3_0_C0_C7_2"
#define AARCH64_REG_ID_AA64MMFR3_EL1 "S3_0_C0_C7_3"
#define AARCH64_REG_ID_AA64MMFR4_EL1 "S3_0_C0_C7_4"
#define AARCH64_REG_ID_AA64PFR0_EL1  "S3_0_C0_C4_0"
#define AARCH64_REG_ID_AA64PFR1_EL1  "S3_0_C0_C4_1"
#define AARCH64_REG_ID_AA64PFR2_EL1  "S3_0_C0_C4_2"
#define AARCH64_REG_ID_AA64SMFR0_EL1 "S3_0_C0_C4_5"
#define AARCH64_REG_ID_AA64ZFR0_EL1  "S3_0_C0_C4_4"

typedef enum {
	REQ_MIDR,
	REQ_MPIDR,
	REQ_REVIDR,
	REQ_ID_AFR0,
	REQ_ID_DFR0,
	REQ_ID_DFR1,
	REQ_ID_ISAR0,
	REQ_ID_ISAR1,
	REQ_ID_ISAR2,
	REQ_ID_ISAR3,
	REQ_ID_ISAR4,
	REQ_ID_ISAR5,
	REQ_ID_ISAR6,
	REQ_ID_MMFR0,
	REQ_ID_MMFR1,
	REQ_ID_MMFR2,
	REQ_ID_MMFR3,
	REQ_ID_MMFR4,
	REQ_ID_MMFR5,
	REQ_ID_PFR0,
	REQ_ID_PFR1,
	REQ_ID_PFR2,
#if defined(__aarch64__)
	REQ_ID_AA64AFR0,
	REQ_ID_AA64AFR1,
	REQ_ID_AA64DFR0,
	REQ_ID_AA64DFR1,
	REQ_ID_AA64ISAR0,
	REQ_ID_AA64ISAR1,
	REQ_ID_AA64ISAR2,
	REQ_ID_AA64MMFR0,
	REQ_ID_AA64MMFR1,
	REQ_ID_AA64MMFR2,
	REQ_ID_AA64MMFR3,
	REQ_ID_AA64MMFR4,
	REQ_ID_AA64PFR0,
	REQ_ID_AA64PFR1,
	REQ_ID_AA64PFR2,
	REQ_ID_AA64SMFR0,
	REQ_ID_AA64ZFR0,
#endif /* __aarch64__ */
	/* termination: */
	NUM_REG_REQUESTS,
	REQ_INVALID = -1
} reg_request_t;

struct read_reg_t {
	reg_request_t request;
	union {
		uint32_t value_32b;
		uint64_t value_64b;
	};
};
typedef struct read_reg_t read_reg_t;

#define ARM_IOC_READ_REG _IOWR('c', 0xB0, read_reg_t)

#endif /* __LIBCPUID_ARM_DRIVER_H__ */
