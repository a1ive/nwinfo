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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include "libcpuid.h"
#include "libcpuid_util.h"
#include "libcpuid_internal.h"
#include "recog_arm.h"

struct arm_feature_map_t {
	uint8_t highbit, lowbit;
	uint8_t value;
	cpu_feature_t feature;
	cpu_feature_level_t ver_optional, ver_mandatory;
};

struct arm_arch_feature_t {
	uint8_t optional, mandatory;
};

struct arm_arch_extension_t {
	struct arm_arch_feature_t present[NUM_CPU_FEATURE_LEVELS], total[NUM_CPU_FEATURE_LEVELS];
};

struct arm_id_part {
	const int id;
	const char* name;
	const char* codename;
};

struct arm_hw_impl {
	const int32_t id;
	const cpu_vendor_t vendor;
	const struct arm_id_part *parts;
	const char *name;
};

/* Decoded PartNum for all implementers
   ID taken from lscpu-arm: https://github.com/util-linux/util-linux/blob/master/sys-utils/lscpu-arm.c
*/
/* Codenames: https://en.wikichip.org/wiki/arm_holdings */
static const struct arm_id_part arm_part[] = {
	{ 0x810, "ARM810", "" },
	{ 0x920, "ARM920", "" },
	{ 0x922, "ARM922", "" },
	{ 0x926, "ARM926", "" },
	{ 0x940, "ARM940", "" },
	{ 0x946, "ARM946", "" },
	{ 0x966, "ARM966", "" },
	{ 0xa20, "ARM1020", "" },
	{ 0xa22, "ARM1022", "" },
	{ 0xa26, "ARM1026", "" },
	{ 0xb02, "ARM11 MPCore", "" },
	{ 0xb36, "ARM1136", "" },
	{ 0xb56, "ARM1156", "" },
	{ 0xb76, "ARM1176", "" },
	{ 0xc05, "Cortex-A5", "Sparrow" },
	{ 0xc07, "Cortex-A7", "Kingfisher" },
	{ 0xc08, "Cortex-A8", "Tiger" },
	{ 0xc09, "Cortex-A9", "Falcon" },
	{ 0xc0d, "Cortex-A17", "Owl" },	/* Originally A12 */
	{ 0xc0f, "Cortex-A15", "Eagle" },
	{ 0xc0e, "Cortex-A17", "Owl" },
	{ 0xc14, "Cortex-R4", "" },
	{ 0xc15, "Cortex-R5", "" },
	{ 0xc17, "Cortex-R7", "" },
	{ 0xc18, "Cortex-R8", "" },
	{ 0xc20, "Cortex-M0", "Swift" },
	{ 0xc21, "Cortex-M1", "Proteus" },
	{ 0xc23, "Cortex-M3", "Sandcat" },
	{ 0xc24, "Cortex-M4", "Merlin" },
	{ 0xc27, "Cortex-M7", "Pelican" },
	{ 0xc60, "Cortex-M0+", "Flycatcher" },
	{ 0xd01, "Cortex-A32", "Minerva" },
	{ 0xd02, "Cortex-A34", "Metis" },
	{ 0xd03, "Cortex-A53", "Apollo" },
	{ 0xd04, "Cortex-A35", "Mercury" },
	{ 0xd05, "Cortex-A55", "Ananke" },
	{ 0xd06, "Cortex-A65", "Helios" },
	{ 0xd07, "Cortex-A57", "Atlas" },
	{ 0xd08, "Cortex-A72", "Maia" },
	{ 0xd09, "Cortex-A73", "Artemis" },
	{ 0xd0a, "Cortex-A75", "Prometheus" },
	{ 0xd0b, "Cortex-A76", "Enyo" },
	{ 0xd0c, "Neoverse-N1", "Ares" },
	{ 0xd0d, "Cortex-A77", "Deimos" },
	{ 0xd0e, "Cortex-A76AE", "Enyo-AE" },
	{ 0xd13, "Cortex-R52", "" },
	{ 0xd15, "Cortex-R82", "" },
	{ 0xd16, "Cortex-R52+", "" },
	{ 0xd20, "Cortex-M23", "Grebe" },
	{ 0xd21, "Cortex-M33", "Teal" },
	{ 0xd22, "Cortex-M55", "Yamin" },
	{ 0xd23, "Cortex-M85", "" },
	{ 0xd40, "Neoverse-V1", "Zeus" },
	{ 0xd41, "Cortex-A78", "Hercules" },
	{ 0xd42, "Cortex-A78AE", "Hercules-AE" },
	{ 0xd43, "Cortex-A65AE", "Helios-AE" },
	{ 0xd44, "Cortex-X1", "Hera" },
	{ 0xd46, "Cortex-A510", "Klein" },
	{ 0xd47, "Cortex-A710", "Matterhorn" },
	{ 0xd48, "Cortex-X2", "Matterhorn ELP" },
	{ 0xd49, "Neoverse-N2", "Perseus" },
	{ 0xd4a, "Neoverse-E1", "Helios" },
	{ 0xd4b, "Cortex-A78C", "Hercules-C" },
	{ 0xd4c, "Cortex-X1C", "Hera-C" },
	{ 0xd4d, "Cortex-A715", "Makalu" },
	{ 0xd4e, "Cortex-X3", "Makalu ELP" },
	{ 0xd4f, "Neoverse-V2", "Demeter" },
	{ 0xd80, "Cortex-A520", "Hayes" },
	{ 0xd81, "Cortex-A720", "Hunter" },
	{ 0xd82, "Cortex-X4", "Hunter ELP" },
	{ 0xd84, "Neoverse-V3", "Poseidon" },
	{ 0xd8e, "Neoverse-N3", "Hermes" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part brcm_part[] = {
	{ 0x0f, "Brahma-B15", "" },
	{ 0x100, "Brahma-B53", "" },
	{ 0x516, "ThunderX2", "" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part dec_part[] = {
	{ 0xa10, "SA110", "" },
	{ 0xa11, "SA1100", "" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part cavium_part[] = {
	{ 0x0a0, "ThunderX", "" },
	{ 0x0a1, "ThunderX-88XX", "" },
	{ 0x0a2, "ThunderX-81XX", "" },
	{ 0x0a3, "ThunderX-83XX", "" },
	{ 0x0af, "ThunderX2-99xx", "" },
	{ 0x0b0, "OcteonTX2", "" },
	{ 0x0b1, "OcteonTX2-98XX", "" },
	{ 0x0b2, "OcteonTX2-96XX", "" },
	{ 0x0b3, "OcteonTX2-95XX", "" },
	{ 0x0b4, "OcteonTX2-95XXN", "" },
	{ 0x0b5, "OcteonTX2-95XXMM", "" },
	{ 0x0b6, "OcteonTX2-95XXO", "" },
	{ 0x0b8, "ThunderX3-T110", "" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part apm_part[] = {
	{ 0x000, "X-Gene", "" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part qcom_part[] = {
	{ 0x001, "Oryon", "" },
	{ 0x00f, "Scorpion", "" },
	{ 0x02d, "Scorpion", "" },
	{ 0x04d, "Krait", "" },
	{ 0x06f, "Krait", "" },
	{ 0x201, "Kryo", "" },
	{ 0x205, "Kryo", "" },
	{ 0x211, "Kryo", "" },
	{ 0x800, "Falkor-V1/Kryo", "" },
	{ 0x801, "Kryo-V2", "" },
	{ 0x802, "Kryo-3XX-Gold", "" },
	{ 0x803, "Kryo-3XX-Silver", "" },
	{ 0x804, "Kryo-4XX-Gold", "" },
	{ 0x805, "Kryo-4XX-Silver", "" },
	{ 0xc00, "Falkor", "" },
	{ 0xc01, "Saphira", "" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part samsung_part[] = {
	{ 0x001, "Exynos M1", "" },
	{ 0x002, "Exynos M3", "" },
	{ 0x003, "Exynos M4", "" },
	{ 0x004, "Exynos M5", "" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part nvidia_part[] = {
	{ 0x000, "Denver", "" },
	{ 0x003, "Denver 2", "" },
	{ 0x004, "Carmel", "" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part marvell_part[] = {
	{ 0x131, "Feroceon-88FR131", "" },
	{ 0x581, "PJ4/PJ4b", "" },
	{ 0x584, "PJ4B-MP", "" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part apple_part[] = {
	{ 0x000, "A6", "Swift" },
	{ 0x001, "A7", "Cyclone" },
	{ 0x002, "A8", "Typhoon" },
	{ 0x003, "A8X", "Typhoon" },
	{ 0x004, "A9", "Twister" },
	{ 0x005, "A9X", "Twister" },
	{ 0x006, "A10 Fusion", "Zephyr" },
	{ 0x007, "A10 Fusion", "Hurricane" },
	{ 0x008, "A11 Bionic", "Monsoon" },
	{ 0x009, "A11 Bionic", "Mistral" },
	{ 0x00b, "A12", "Vortex" },
	{ 0x00c, "A12", "Tempest" },
	{ 0x00f, "M9", "Tempest" },
	{ 0x010, "A12X Bionic", "Vortex" },
	{ 0x011, "A12X Bionic", "Tempest" },
	{ 0x012, "A13 Bionic", "Lightning" },
	{ 0x013, "A13 Bionic", "Thunder" },
	{ 0x020, "A14", "Icestorm" },
	{ 0x021, "A14", "Firestorm" },
	{ 0x022, "M1", "Icestorm" },
	{ 0x023, "M1", "Firestorm" },
	{ 0x024, "M1 Pro", "Icestorm" },
	{ 0x025, "M1 Pro", "Firestorm" },
	{ 0x026, "M10", "Thunder" },
	{ 0x028, "M1 Max", "Icestorm" },
	{ 0x029, "M1 Max", "Firestorm" },
	{ 0x030, "A15", "Blizzard" },
	{ 0x031, "A15", "Avalanche" },
	{ 0x032, "M2", "Blizzard" },
	{ 0x033, "M2", "Avalanche" },
	{ 0x034, "M2 Pro", "Blizzard" },
	{ 0x035, "M2 Pro", "Avalanche" },
	{ 0x036, "A16", "Sawtooth" },
	{ 0x037, "A16", "Everest" },
	{ 0x038, "M2 Max", "Blizzard" },
	{ 0x039, "M2 Max", "Avalanche" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part faraday_part[] = {
	{ 0x526, "FA526", "" },
	{ 0x626, "FA626", "" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part intel_part[] = {
	{ 0x200, "i80200", "" },
	{ 0x210, "PXA250A", "" },
	{ 0x212, "PXA210A", "" },
	{ 0x242, "i80321-400", "" },
	{ 0x243, "i80321-600", "" },
	{ 0x290, "PXA250B/PXA26x", "" },
	{ 0x292, "PXA210B", "" },
	{ 0x2c2, "i80321-400-B0", "" },
	{ 0x2c3, "i80321-600-B0", "" },
	{ 0x2d0, "PXA250C/PXA255/PXA26x", "" },
	{ 0x2d2, "PXA210C", "" },
	{ 0x411, "PXA27x", "" },
	{ 0x41c, "IPX425-533", "" },
	{ 0x41d, "IPX425-400", "" },
	{ 0x41f, "IPX425-266", "" },
	{ 0x682, "PXA32x", "" },
	{ 0x683, "PXA930/PXA935", "" },
	{ 0x688, "PXA30x", "" },
	{ 0x689, "PXA31x", "" },
	{ 0xb11, "SA1110", "" },
	{ 0xc12, "IPX1200", "" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part fujitsu_part[] = {
	{ 0x001, "A64FX", "" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part hisi_part[] = {
	{ 0xd01, "TaiShan-v110", "" },	/* used in Kunpeng-920 SoC */
	{ 0xd02, "TaiShan-v120", "" },	/* used in Kirin 990A and 9000S SoCs */
	{ 0xd40, "Cortex-A76", "" },	/* HiSilicon uses this ID though advertises A76 */
	{ 0xd41, "Cortex-A77", "" },	/* HiSilicon uses this ID though advertises A77 */
	{ -1, "unknown", "" },
};

static const struct arm_id_part ampere_part[] = {
	{ 0xac3, "Ampere-1", "" },
	{ 0xac4, "Ampere-1a", "" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part ft_part[] = {
	{ 0x303, "FTC310", "" },
	{ 0x660, "FTC660", "" },
	{ 0x661, "FTC661", "" },
	{ 0x662, "FTC662", "" },
	{ 0x663, "FTC663", "" },
	{ 0x664, "FTC664", "" },
	{ 0x862, "FTC862", "" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part ms_part[] = {
	{ 0xd49, "Azure-Cobalt-100", "" },
	{ -1, "unknown", "" },
};

static const struct arm_id_part unknown_part[] = {
	{ -1, "unknown", "" },
};

/* Implementers list */
static const struct arm_hw_impl hw_implementer[] = {
	{ 0x41, VENDOR_ARM,       arm_part,     "ARM"                },
	{ 0x42, VENDOR_BROADCOM,  brcm_part,    "Broadcom"           },
	{ 0x43, VENDOR_CAVIUM,    cavium_part,  "Cavium"             },
	{ 0x44, VENDOR_DEC,       dec_part,     "DEC"                },
	{ 0x46, VENDOR_FUJITSU,   fujitsu_part, "FUJITSU"            },
	{ 0x48, VENDOR_HISILICON, hisi_part,    "HiSilicon"          },
	{ 0x49, VENDOR_INFINEON,  unknown_part, "Infineon"           },
	{ 0x4d, VENDOR_FREESCALE, unknown_part, "Motorola/Freescale" },
	{ 0x4e, VENDOR_NVIDIA,    nvidia_part,  "NVIDIA"             },
	{ 0x50, VENDOR_APM,       apm_part,     "APM"                },
	{ 0x51, VENDOR_QUALCOMM,  qcom_part,    "Qualcomm"           },
	{ 0x53, VENDOR_SAMSUNG,   samsung_part, "Samsung"            },
	{ 0x56, VENDOR_MARVELL,   marvell_part, "Marvell"            },
	{ 0x61, VENDOR_APPLE,     apple_part,   "Apple"              },
	{ 0x66, VENDOR_FARADAY,   faraday_part, "Faraday"            },
	{ 0x69, VENDOR_INTEL,     intel_part,   "Intel"              },
	{ 0x6d, VENDOR_MICROSOFT, ms_part,      "Microsoft"          },
	{ 0x70, VENDOR_PHYTIUM,   ft_part,      "Phytium"            },
	{ 0xc0, VENDOR_AMPERE,    ampere_part,  "Ampere"             },
	{   -1, VENDOR_UNKNOWN,   unknown_part, "unknown"            },
};

static const struct arm_hw_impl* get_cpu_implementer_from_code(uint8_t implementer)
{
	int i;

	for (i = 0; hw_implementer[i].id >= 0; i++) {
		if (hw_implementer[i].id == implementer)
			break;
	}

	/* This function returns an item with VENDOR_UNKNOWN if implementer is not found in the "hw_implementer" array */
	return &hw_implementer[i];
}

static const struct arm_hw_impl* get_cpu_implementer_from_vendor(cpu_vendor_t vendor)
{
	int i;

	for (i = 0; hw_implementer[i].id >= 0; i++) {
		if (hw_implementer[i].vendor == vendor)
			break;
	}

	/* This function returns an item with VENDOR_UNKNOWN if implementer is not found in the "hw_implementer" array */
	return &hw_implementer[i];
}

static const struct arm_id_part* get_cpu_implementer_parts(const struct arm_hw_impl* hw_impl, uint16_t part_num)
{
	int i;

	for (i = 0; hw_impl->parts[i].id >= 0; i++) {
		if (hw_impl->parts[i].id == part_num)
			break;
	}

	/* This function retuns "unknown" if PartNum is not found in the "parts" array */
	return &hw_impl->parts[i];
}

static void set_feature_status(struct cpu_id_t* data, struct arm_arch_extension_t* ext_status, bool is_present, cpu_feature_t feature, cpu_feature_level_t optional_from, cpu_feature_level_t mandatory_from)
{
	if (optional_from >= 0)
		ext_status->total[optional_from].optional++;
	if (mandatory_from >= 0)
		ext_status->total[mandatory_from].mandatory++;

	if (is_present) {
		data->flags[feature] = 1;
		debugf(3, "feature %s is present", cpu_feature_str(feature));
		if (optional_from >= 0) {
			ext_status->present[optional_from].optional++;
			debugf(3, " (optional from %s", cpu_feature_level_str(optional_from));
			if (mandatory_from >= 0) {
				ext_status->present[mandatory_from].mandatory++;
				debugf(3, ", mandatory from %s", cpu_feature_level_str(mandatory_from));
			}
			debugf(3, ")");
		}
		debugf(3, "\n");
	}
}

static void match_arm_features(const struct arm_feature_map_t* matchtable, const char* reg_name, const int reg_number, uint64_t reg_value, struct cpu_id_t* data, struct arm_arch_extension_t* ext_status)
{
	int i;
	bool feature_is_present;

	for (i = 0; matchtable[i].feature != -1; i++) {
		feature_is_present = (EXTRACTS_BITS(reg_value, matchtable[i].highbit, matchtable[i].lowbit) == matchtable[i].value);
		if (feature_is_present)
			debugf(3, "Register %8s%i (0x%016" PRIX64 "): match value %u for bits [%2u:%2u], ", reg_name, reg_number, reg_value, matchtable[i].value, matchtable[i].highbit, matchtable[i].lowbit);
		set_feature_status(data, ext_status, feature_is_present, matchtable[i].feature, matchtable[i].ver_optional, matchtable[i].ver_mandatory);
	}
}

static void load_arm_feature_pauth(struct cpu_id_t* data, struct arm_arch_extension_t* ext_status)
{
	/* FEAT_PAuth, Pointer authentication
	When FEAT_PAuth is implemented, one of the following must be true:
	- Exactly one of the PAC algorithms is implemented.
	- If the PACGA instruction and other Pointer authentication instructions use different PAC algorithms, exactly two PAC algorithms are implemented.
	The PAC algorithm features are:
	- FEAT_PACQARMA5.
	- FEAT_PACIMP.
	- FEAT_PACQARMA3. */
	set_feature_status(data, ext_status,
		(data->flags[CPU_FEATURE_PACIMP] ||
		 data->flags[CPU_FEATURE_PACQARMA3] ||
		 data->flags[CPU_FEATURE_PACQARMA5]),
		CPU_FEATURE_PAUTH, FEATURE_LEVEL_ARM_V8_2_A, FEATURE_LEVEL_ARM_V8_3_A);
}

static void load_arm_feature_mpam(struct cpu_raw_data_t* raw, struct cpu_id_t* data, struct arm_arch_extension_t* ext_status)
{
	/* FEAT_MPAM, Memory Partitioning and Monitoring Extension
	MPAM Extension version: MPAM | MPAM_frac
	Not implemented: 0b0000 | 0b0000
	v0.1 is implemented: 0b0000 | 0b0001
	v1.0 is implemented: 0b0001 | 0b0000
	v1.1 is implemented: 0b0001 | 0b0001 */
	const uint8_t mpam      = EXTRACTS_BITS(raw->arm_id_aa64pfr[0], 43, 40);
	const uint8_t mpam_frac = EXTRACTS_BITS(raw->arm_id_aa64pfr[1], 19, 16);
	if ((mpam != 0b0000) || (mpam_frac != 0b0000)) {
		debugf(2, "MPAM Extension version is %u.%u\n", mpam, mpam_frac);
		set_feature_status(data, ext_status, true,                                      CPU_FEATURE_MPAM,     FEATURE_LEVEL_ARM_V8_2_A, -1);
		set_feature_status(data, ext_status, (mpam == 0b0000) && (mpam_frac == 0b0001), CPU_FEATURE_MPAMV0P1, FEATURE_LEVEL_ARM_V8_5_A, -1);
		set_feature_status(data, ext_status, (mpam == 0b0001) && (mpam_frac == 0b0001), CPU_FEATURE_MPAMV1P1, FEATURE_LEVEL_ARM_V8_5_A, -1);
	}
}

static void load_arm_feature_mte4(struct cpu_raw_data_t* raw, struct cpu_id_t* data, struct arm_arch_extension_t* ext_status)
{
	/* FEAT_MTE4, Enhanced Memory Tagging Extension */
	const uint8_t mte = EXTRACTS_BITS(raw->arm_id_aa64pfr[1], 11,  8);
	if (mte >= 0b0010) {
		/* These fields are valid only if ID_AA64PFR1_EL1.MTE >= 0b0010 */
		const uint8_t mte_frac = EXTRACTS_BITS(raw->arm_id_aa64pfr[1], 43, 40);
		const uint8_t mtex     = EXTRACTS_BITS(raw->arm_id_aa64pfr[1], 55, 52);
		set_feature_status(data, ext_status, (mte_frac == 0b0000), CPU_FEATURE_MTE_ASYNC,           FEATURE_LEVEL_ARM_V8_5_A, -1);
		set_feature_status(data, ext_status, (mtex == 0b0001),     CPU_FEATURE_MTE_CANONICAL_TAGS,  FEATURE_LEVEL_ARM_V8_7_A, -1);
		set_feature_status(data, ext_status, (mtex == 0b0001),     CPU_FEATURE_MTE_NO_ADDRESS_TAGS, FEATURE_LEVEL_ARM_V8_7_A, -1);
	}
	/* If FEAT_MTE4 is implemented, then FEAT_MTE_CANONICAL_TAGS,
	FEAT_MTE_NO_ADDRESS_TAGS, FEAT_MTE_TAGGED_FAR, and
	FEAT_MTE_STORE_ONLY are implemented. */
	set_feature_status(data, ext_status,
		(data->flags[CPU_FEATURE_MTE_CANONICAL_TAGS] &&
		 data->flags[CPU_FEATURE_MTE_NO_ADDRESS_TAGS] &&
		 data->flags[CPU_FEATURE_MTE_TAGGED_FAR] &&
		 data->flags[CPU_FEATURE_MTE_STORE_ONLY]),
		CPU_FEATURE_MTE4, FEATURE_LEVEL_ARM_V8_7_A, -1);
}

static void decode_arm_architecture_version(struct cpu_raw_data_t* raw, struct cpu_id_t* data, struct arm_arch_extension_t* ext_status)
{
	int i;
	cpu_feature_level_t feature_level = FEATURE_LEVEL_UNKNOWN;
	const cpu_feature_level_t *architecture_arm = NULL;
	const uint8_t architecture = EXTRACTS_BITS(raw->arm_midr, 19, 16);

	const struct { uint8_t raw_value; cpu_feature_level_t enum_value; }
	architecture_matchtable[] = {
		{ 0b0000, FEATURE_LEVEL_UNKNOWN },
		{ 0b0001, FEATURE_LEVEL_ARM_V4 },
		{ 0b0010, FEATURE_LEVEL_ARM_V4T },
		{ 0b0011, FEATURE_LEVEL_ARM_V5 },
		{ 0b0100, FEATURE_LEVEL_ARM_V5T },
		{ 0b0101, FEATURE_LEVEL_ARM_V5TE },
		{ 0b0110, FEATURE_LEVEL_ARM_V5TEJ },
		{ 0b0111, FEATURE_LEVEL_ARM_V6 },
		{ 0b1000, FEATURE_LEVEL_UNKNOWN },
		{ 0b1001, FEATURE_LEVEL_UNKNOWN },
		{ 0b1010, FEATURE_LEVEL_UNKNOWN },
		{ 0b1011, FEATURE_LEVEL_UNKNOWN },
		{ 0b1100, FEATURE_LEVEL_ARM_V6_M },
		{ 0b1101, FEATURE_LEVEL_UNKNOWN },
		{ 0b1110, FEATURE_LEVEL_UNKNOWN }
	};

	/**
	 * The Armv8.0 architecture extension
	 *
	 * The original Armv8-A architecture is called Armv8.0. It contains mandatory and optional architectural features.
	 * Some features must be implemented together. An implementation is Armv8.0 compliant if it includes all of the
	 * Armv8.0 architectural features that are mandatory.
	 *
	 * An Armv8.0 compliant implementation can additionally include:
	 * - Armv8.0 features that are optional.
	 * - Any arbitrary subset of the architectural features of Armv8.1, subject only to those constraints that require that certain features be implemented together.
	 */
	const cpu_feature_level_t architecture_arm_v8a[] = {
		FEATURE_LEVEL_ARM_V8_0_A, /*!< ARMv8.0-A */
		FEATURE_LEVEL_ARM_V8_1_A, /*!< ARMv8.1-A */
		FEATURE_LEVEL_ARM_V8_2_A, /*!< ARMv8.2-A */
		FEATURE_LEVEL_ARM_V8_3_A, /*!< ARMv8.3-A */
		FEATURE_LEVEL_ARM_V8_4_A, /*!< ARMv8.4-A */
		FEATURE_LEVEL_ARM_V8_5_A, /*!< ARMv8.5-A */
		FEATURE_LEVEL_ARM_V8_6_A, /*!< ARMv8.6-A */
		FEATURE_LEVEL_ARM_V8_7_A, /*!< ARMv8.7-A */
		FEATURE_LEVEL_ARM_V8_8_A, /*!< ARMv8.8-A */
		FEATURE_LEVEL_ARM_V8_9_A, /*!< ARMv8.9-A */
		FEATURE_LEVEL_UNKNOWN,
	};

	/**
	 * The Armv9.0 architecture extension
	 *
	 * The Armv9.0 architecture extension adds mandatory and optional architectural features. Some features must be
	 * implemented together. An implementation is Armv9.0 compliant if all of the following apply:
	 * - It is Armv8.5 compliant.
	 * - It includes all of the Armv9.0 architectural features that are mandatory.
	 *
	 * An Armv9.0 compliant implementation can additionally include:
	 * - Armv9.0 features that are optional.
	 * - Any arbitrary subset of the architectural features of Armv9.1, subject only to those constraints that require that certain features be implemented together.
	 */
	const cpu_feature_level_t architecture_arm_v9a[] = {
		FEATURE_LEVEL_ARM_V9_0_A, /*!< ARMv9.0-A */
		FEATURE_LEVEL_ARM_V9_1_A, /*!< ARMv9.1-A */
		FEATURE_LEVEL_ARM_V9_2_A, /*!< ARMv9.2-A */
		FEATURE_LEVEL_ARM_V9_3_A, /*!< ARMv9.3-A */
		FEATURE_LEVEL_ARM_V9_4_A, /*!< ARMv9.4-A */
		FEATURE_LEVEL_UNKNOWN,
	};

	/* Check if architecture level is explicit */
	for (i = 0; i < COUNT_OF(architecture_matchtable); i++) {
		if (architecture == architecture_matchtable[i].raw_value) {
			feature_level = architecture_matchtable[i].enum_value;
			goto found;
		}
	}

	/* When architecture is 0b1111, architectural features are individually identified in the ID_* registers */
	//FIXME: it works only for A-profile architecture, M-profile and R-profile are not supported yet
	if ((ext_status->present[FEATURE_LEVEL_ARM_V9_0_A].optional > 0) || (ext_status->present[FEATURE_LEVEL_ARM_V9_0_A].mandatory > 0))
		architecture_arm = architecture_arm_v9a;
	else if ((ext_status->present[FEATURE_LEVEL_ARM_V8_0_A].optional > 0) || (ext_status->present[FEATURE_LEVEL_ARM_V8_0_A].mandatory > 0))
		architecture_arm = architecture_arm_v8a;
	else
		goto found;

	for (i = 0; architecture_arm[i] != FEATURE_LEVEL_UNKNOWN; i++) {
		debugf(3, "Check if CPU is %s compliant: %2u/%2u optional features detected, %2u/%2u mandatory features required\n",
			cpu_feature_level_str(architecture_arm[i]),
			ext_status->present[ architecture_arm[i] ].optional, ext_status->total[ architecture_arm[i] ].optional,
			ext_status->present[ architecture_arm[i] ].mandatory, ext_status->total[ architecture_arm[i] ].mandatory);
		/* CPU is compliant when it includes all of the architectural features that are mandatory */
		if (ext_status->present[ architecture_arm[i] ].mandatory < ext_status->total[ architecture_arm[i] ].mandatory)
			break;
		/* If there are no mandatory features (like ARMv9.3-A), check that at least one optional feature is implemented */
		else if ((ext_status->total[ architecture_arm[i] ].mandatory == 0) && (ext_status->present[ architecture_arm[i] ].optional == 0))
			break;
		else
			feature_level = architecture_arm[i];
	}

found:
	data->feature_level = feature_level;
	debugf(2, "ARM architecture version is %s\n", cpu_feature_level_str(feature_level));
}

#define MAX_MATCHTABLE_ITEMS 32
#define MATCH_FEATURES_TABLE_WITH_RAW(reg) match_arm_features(matchtable_id_##reg[i], #reg, i, raw->arm_id_##reg[i], data, ext_status)
static void load_arm_features(struct cpu_raw_data_t* raw, struct cpu_id_t* data, struct arm_arch_extension_t* ext_status)
{
	int i;

	const struct arm_feature_map_t matchtable_id_aa64dfr[MAX_ARM_ID_AA64DFR_REGS][MAX_MATCHTABLE_ITEMS] = {
		[0] /* ID_AA64DFR0 */ = {
			{ 59, 56, 0b0001, CPU_FEATURE_TRBE_EXT,   FEATURE_LEVEL_ARM_V9_3_A, -1 },
			{ 55, 52, 0b0001, CPU_FEATURE_BRBE,       FEATURE_LEVEL_ARM_V9_1_A, -1 },
			{ 55, 52, 0b0010, CPU_FEATURE_BRBEV1P1,   FEATURE_LEVEL_ARM_V9_2_A, -1 },
			{ 51, 48, 0b0001, CPU_FEATURE_MTPMU,      FEATURE_LEVEL_ARM_V8_5_A, -1 },
			{ 47, 47, 0b0001, CPU_FEATURE_TRBE,       FEATURE_LEVEL_ARM_V9_0_A, -1 },
			{ 43, 40, 0b0001, CPU_FEATURE_TRF,        FEATURE_LEVEL_ARM_V8_3_A, -1 },
			{ 39, 36, 0b0000, CPU_FEATURE_DOUBLELOCK, FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{ 35, 32, 0b0001, CPU_FEATURE_SPE,        FEATURE_LEVEL_ARM_V8_1_A, -1 },
			{ 35, 32, 0b0010, CPU_FEATURE_SPEV1P1,    FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 35, 32, 0b0011, CPU_FEATURE_SPEV1P2,    FEATURE_LEVEL_ARM_V8_6_A, -1 },
			{ 35, 32, 0b0100, CPU_FEATURE_SPEV1P3,    FEATURE_LEVEL_ARM_V8_7_A, -1 },
			{ 35, 32, 0b0101, CPU_FEATURE_SPEV1P4,    FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 27, 24, 0b0001, CPU_FEATURE_SEBEP,      FEATURE_LEVEL_ARM_V9_3_A, -1 },
			{ 19, 16, 0b0001, CPU_FEATURE_PMUV3_SS,   FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 11,  8, 0b0001, CPU_FEATURE_PMUV3,      FEATURE_LEVEL_ARM_V8_0_A, -1 }, /* Performance Monitors Extension, PMUv3 implemented. */
			{ 11,  8, 0b0100, CPU_FEATURE_PMUV3P1,    FEATURE_LEVEL_ARM_V8_1_A, -1 }, /* PMUv3 for Armv8.1 */
			{ 11,  8, 0b0101, CPU_FEATURE_PMUV3P4,    FEATURE_LEVEL_ARM_V8_3_A, -1 }, /* PMUv3 for Armv8.4 */
			{ 11,  8, 0b0110, CPU_FEATURE_PMUV3P5,    FEATURE_LEVEL_ARM_V8_4_A, -1 }, /* PMUv3 for Armv8.5 */
			{ 11,  8, 0b0111, CPU_FEATURE_PMUV3P7,    FEATURE_LEVEL_ARM_V8_6_A, -1 }, /* PMUv3 for Armv8.7 */
			{ 11,  8, 0b1000, CPU_FEATURE_PMUV3P8,    FEATURE_LEVEL_ARM_V8_7_A, -1 }, /* PMUv3 for Armv8.8 */
			{ 11,  8, 0b1001, CPU_FEATURE_PMUV3P9,    FEATURE_LEVEL_ARM_V8_8_A, -1 }, /* PMUv3 for Armv8.9 */
			{  3,  0, 0b1000, CPU_FEATURE_DEBUGV8P2,  FEATURE_LEVEL_ARM_V8_1_A, FEATURE_LEVEL_ARM_V8_2_A }, /* Armv8.2 debug architecture */
			{  3,  0, 0b1001, CPU_FEATURE_DEBUGV8P4,  FEATURE_LEVEL_ARM_V8_3_A, FEATURE_LEVEL_ARM_V8_4_A }, /* Armv8.4 debug architecture */
			{  3,  0, 0b1010, CPU_FEATURE_DEBUGV8P8,  FEATURE_LEVEL_ARM_V8_7_A, FEATURE_LEVEL_ARM_V8_8_A }, /* Armv8.8 debug architecture */
			{  3,  0, 0b1011, CPU_FEATURE_DEBUGV8P9,  FEATURE_LEVEL_ARM_V8_8_A, FEATURE_LEVEL_ARM_V8_9_A }, /* Armv8.9 debug architecture */
			{ -1, -1,     -1, -1,                     -1,                       -1 }
		},
		[1] /* ID_AA64DFR1 */ = {
			{ 55, 52, 0b0001, CPU_FEATURE_SPE_DPFZS,   FEATURE_LEVEL_ARM_V8_6_A, -1 },
			{ 51, 48, 0b0001, CPU_FEATURE_EBEP,        FEATURE_LEVEL_ARM_V9_3_A, -1 },
			{ 47, 44, 0b0001, CPU_FEATURE_ITE,         FEATURE_LEVEL_ARM_V9_3_A, -1 },
			{ 43, 40, 0b0001, CPU_FEATURE_ABLE,        FEATURE_LEVEL_ARM_V9_3_A, -1 },
			{ 43, 40, 0b0001, CPU_FEATURE_BWE,         FEATURE_LEVEL_ARM_V9_3_A, -1 },
			{ 39, 36, 0b0001, CPU_FEATURE_PMUV3_ICNTR, FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 35, 32, 0b0001, CPU_FEATURE_SPMU,        FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ -1, -1,     -1, -1,                      -1,                       -1 }
		}
	};

	const struct arm_feature_map_t matchtable_id_aa64isar[MAX_ARM_ID_AA64ISAR_REGS][MAX_MATCHTABLE_ITEMS] = {
		[0] /* ID_AA64ISAR0 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_RNG,       FEATURE_LEVEL_ARM_V8_4_A, -1 },
			{ 59, 56, 0b0001, CPU_FEATURE_TLBIOS,    FEATURE_LEVEL_ARM_V8_3_A, FEATURE_LEVEL_ARM_V8_4_A }, /* Outer Shareable and TLB range maintenance instructions are not implemented */
			{ 59, 56, 0b0010, CPU_FEATURE_TLBIOS,    FEATURE_LEVEL_ARM_V8_3_A, FEATURE_LEVEL_ARM_V8_4_A }, /* Outer Shareable TLB maintenance instructions are implemented */
			{ 59, 56, 0b0010, CPU_FEATURE_TLBIRANGE, FEATURE_LEVEL_ARM_V8_3_A, FEATURE_LEVEL_ARM_V8_4_A }, /* Outer Shareable TLB maintenance instructions are implemented */
			{ 55, 52, 0b0001, CPU_FEATURE_FLAGM,     FEATURE_LEVEL_ARM_V8_1_A, FEATURE_LEVEL_ARM_V8_4_A },
			{ 55, 52, 0b0010, CPU_FEATURE_FLAGM2,    FEATURE_LEVEL_ARM_V8_4_A, -1 },
			{ 51, 48, 0b0001, CPU_FEATURE_FHM,       FEATURE_LEVEL_ARM_V8_1_A, -1 },
			{ 47, 44, 0b0001, CPU_FEATURE_DOTPROD,   FEATURE_LEVEL_ARM_V8_1_A, -1 },
			{ 43, 40, 0b0001, CPU_FEATURE_SM4,       FEATURE_LEVEL_ARM_V8_1_A, -1 },
			{ 39, 36, 0b0001, CPU_FEATURE_SM3,       FEATURE_LEVEL_ARM_V8_1_A, -1 },
			{ 35, 32, 0b0001, CPU_FEATURE_SHA3,      FEATURE_LEVEL_ARM_V8_1_A, -1 },
			{ 31, 28, 0b0001, CPU_FEATURE_RDM,       FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{ 27, 24, 0b0001, CPU_FEATURE_TME,       FEATURE_LEVEL_ARM_V9_0_A, -1 },
			{ 23, 20, 0b0010, CPU_FEATURE_LSE,       FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_1_A },
			{ 23, 20, 0b0011, CPU_FEATURE_LSE128,    FEATURE_LEVEL_ARM_V9_3_A, -1 },
			{ 19, 16, 0b0001, CPU_FEATURE_CRC32,     FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_1_A },
			{ 15, 12, 0b0001, CPU_FEATURE_SHA256,    FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{ 15, 12, 0b0010, CPU_FEATURE_SHA512,    FEATURE_LEVEL_ARM_V8_1_A, -1 },
			{ 11,  8, 0b0001, CPU_FEATURE_SHA1,      FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{  7,  4, 0b0001, CPU_FEATURE_AES,       FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{  7,  4, 0b0010, CPU_FEATURE_PMULL,     FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{ -1, -1,     -1, -1,                    -1,                       -1 }
		},
		[1] /* ID_AA64ISAR1 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_LS64,         FEATURE_LEVEL_ARM_V8_6_A, -1 },
			{ 63, 60, 0b0010, CPU_FEATURE_LS64_V,       FEATURE_LEVEL_ARM_V8_6_A, -1 },
			{ 63, 60, 0b0011, CPU_FEATURE_LS64_ACCDATA, FEATURE_LEVEL_ARM_V8_6_A, -1 },
			{ 59, 56, 0b0001, CPU_FEATURE_XS,           FEATURE_LEVEL_ARM_V8_6_A, FEATURE_LEVEL_ARM_V8_7_A },
			{ 55, 52, 0b0001, CPU_FEATURE_I8MM,         FEATURE_LEVEL_ARM_V8_1_A, FEATURE_LEVEL_ARM_V8_6_A },
			{ 51, 48, 0b0001, CPU_FEATURE_DGH,          FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{ 47, 44, 0b0001, CPU_FEATURE_BF16,         FEATURE_LEVEL_ARM_V8_2_A, FEATURE_LEVEL_ARM_V8_6_A },
			{ 47, 44, 0b0010, CPU_FEATURE_EBF16,        FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 43, 40, 0b0001, CPU_FEATURE_SPECRES,      FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_5_A },
			{ 43, 40, 0b0010, CPU_FEATURE_SPECRES2,     FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_9_A },
			{ 39, 36, 0b0001, CPU_FEATURE_SB,           FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_5_A },
			{ 35, 32, 0b0001, CPU_FEATURE_FRINTTS,      FEATURE_LEVEL_ARM_V8_4_A, -1 },
			{ 31, 28, 0b0001, CPU_FEATURE_PACIMP,       FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 27, 24, 0b0001, CPU_FEATURE_PACQARMA5,    FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 23, 20, 0b0001, CPU_FEATURE_LRCPC,        FEATURE_LEVEL_ARM_V8_2_A, FEATURE_LEVEL_ARM_V8_3_A },
			{ 23, 20, 0b0010, CPU_FEATURE_LRCPC2,       FEATURE_LEVEL_ARM_V8_2_A, FEATURE_LEVEL_ARM_V8_4_A },
			{ 23, 20, 0b0011, CPU_FEATURE_LRCPC3,       FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 19, 16, 0b0001, CPU_FEATURE_FCMA,         FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 15, 12, 0b0001, CPU_FEATURE_JSCVT,        FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 11,  8, 0b0001, CPU_FEATURE_PAUTH,        FEATURE_LEVEL_ARM_V8_2_A, FEATURE_LEVEL_ARM_V8_3_A },
			{ 11,  8, 0b0010, CPU_FEATURE_EPAC,         FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 11,  8, 0b0011, CPU_FEATURE_PAUTH2,       FEATURE_LEVEL_ARM_V8_2_A, FEATURE_LEVEL_ARM_V8_6_A },
			{ 11,  8, 0b0100, CPU_FEATURE_FPAC,         FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 11,  8, 0b0101, CPU_FEATURE_FPACCOMBINE,  FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{  7,  4, 0b0001, CPU_FEATURE_PAUTH,        FEATURE_LEVEL_ARM_V8_2_A, FEATURE_LEVEL_ARM_V8_3_A },
			{  7,  4, 0b0010, CPU_FEATURE_EPAC,         FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{  7,  4, 0b0011, CPU_FEATURE_PAUTH2,       FEATURE_LEVEL_ARM_V8_2_A, FEATURE_LEVEL_ARM_V8_6_A },
			{  7,  4, 0b0100, CPU_FEATURE_FPAC,         FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{  7,  4, 0b0101, CPU_FEATURE_FPACCOMBINE,  FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{  3,  0, 0b0001, CPU_FEATURE_DPB,          FEATURE_LEVEL_ARM_V8_1_A, FEATURE_LEVEL_ARM_V8_2_A },
			{  3,  0, 0b0010, CPU_FEATURE_DPB2,         FEATURE_LEVEL_ARM_V8_1_A, FEATURE_LEVEL_ARM_V8_5_A },
			{ -1, -1,     -1, -1,                       -1,                       -1 }
		},
		[2] /* ID_AA64ISAR2 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_ATS1A,         FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 55, 52, 0b0001, CPU_FEATURE_CSSC,          FEATURE_LEVEL_ARM_V8_7_A, -1 },
			{ 51, 48, 0b0001, CPU_FEATURE_RPRFM,         FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{ 43, 40, 0b0001, CPU_FEATURE_PRFMSLC,       FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{ 39, 36, 0b0001, CPU_FEATURE_SYSINSTR128,   FEATURE_LEVEL_ARM_V9_3_A, -1 },
			{ 35, 32, 0b0001, CPU_FEATURE_SYSREG128,     FEATURE_LEVEL_ARM_V9_3_A, -1 },
			{ 31, 28, 0b0001, CPU_FEATURE_CLRBHB,        FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_9_A },
			{ 27, 24, 0b0001, CPU_FEATURE_CONSTPACFIELD, FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 23, 20, 0b0001, CPU_FEATURE_HBC,           FEATURE_LEVEL_ARM_V8_7_A, FEATURE_LEVEL_ARM_V8_8_A },
			{ 19, 16, 0b0001, CPU_FEATURE_MOPS,          FEATURE_LEVEL_ARM_V8_7_A, FEATURE_LEVEL_ARM_V8_8_A },
			{ 15, 12, 0b0001, CPU_FEATURE_PAUTH,         FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 15, 12, 0b0010, CPU_FEATURE_EPAC,          FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 15, 12, 0b0011, CPU_FEATURE_PAUTH2,        FEATURE_LEVEL_ARM_V8_2_A, FEATURE_LEVEL_ARM_V8_6_A },
			{ 15, 12, 0b0100, CPU_FEATURE_FPAC,          FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 15, 12, 0b0101, CPU_FEATURE_FPACCOMBINE,   FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 11,  8, 0b0001, CPU_FEATURE_PACQARMA3,     FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{  7,  4, 0b0001, CPU_FEATURE_RPRES,         FEATURE_LEVEL_ARM_V8_6_A, -1 },
			{  3,  0, 0b0010, CPU_FEATURE_WFXT,          FEATURE_LEVEL_ARM_V8_6_A, FEATURE_LEVEL_ARM_V8_7_A },
			{ -1, -1,     -1, -1,                        -1,                       -1 }
		}
	};

	const struct arm_feature_map_t matchtable_id_aa64mmfr[MAX_ARM_ID_AA64MMFR_REGS][MAX_MATCHTABLE_ITEMS] = {
		[0] /* ID_AA64MMFR0 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_ECV,         FEATURE_LEVEL_ARM_V8_5_A, FEATURE_LEVEL_ARM_V8_6_A },
			{ 63, 60, 0b0010, CPU_FEATURE_ECV,         FEATURE_LEVEL_ARM_V8_5_A, FEATURE_LEVEL_ARM_V8_6_A },
			{ 59, 56, 0b0001, CPU_FEATURE_FGT,         FEATURE_LEVEL_ARM_V8_5_A, -1 },
			{ 59, 56, 0b0010, CPU_FEATURE_FGT2,        FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 47, 44, 0b0001, CPU_FEATURE_EXS,         FEATURE_LEVEL_ARM_V8_4_A, -1 },
			{ 31, 28, 0b0001, CPU_FEATURE_LPA2,        FEATURE_LEVEL_ARM_V8_6_A, -1 },
			{ 23, 20, 0b0001, CPU_FEATURE_LPA2,        FEATURE_LEVEL_ARM_V8_6_A, -1 },
			{ 19, 16, 0b0001, CPU_FEATURE_MIXEDENDEL0, FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{ 11,  8, 0b0001, CPU_FEATURE_MIXEDEND,    FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{  7,  4, 0b0010, CPU_FEATURE_ASID16,      FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{  3,  0, 0b0110, CPU_FEATURE_LPA,         FEATURE_LEVEL_ARM_V8_1_A, -1 },
			{ -1, -1,     -1, -1,                      -1,                       -1 }
		},
		[1] /* ID_AA64MMFR1 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_ECBHB,  FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_9_A },
			{ 59, 56, 0b0001, CPU_FEATURE_CMOW,   FEATURE_LEVEL_ARM_V8_7_A, FEATURE_LEVEL_ARM_V8_8_A },
			{ 55, 52, 0b0001, CPU_FEATURE_TIDCP1, FEATURE_LEVEL_ARM_V8_7_A, FEATURE_LEVEL_ARM_V8_8_A },
			{ 47, 44, 0b0001, CPU_FEATURE_AFP,    FEATURE_LEVEL_ARM_V8_6_A, -1 },
			{ 43, 40, 0b0001, CPU_FEATURE_HCX,    FEATURE_LEVEL_ARM_V8_6_A, -1 },
			{ 39, 36, 0b0010, CPU_FEATURE_ETS2,   FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_8_A },
			{ 35, 32, 0b0001, CPU_FEATURE_TWED,   FEATURE_LEVEL_ARM_V8_5_A, -1 },
			{ 31, 28, 0b0001, CPU_FEATURE_XNX,    FEATURE_LEVEL_ARM_V8_1_A, -1 },
			{ 23, 20, 0b0001, CPU_FEATURE_PAN,    FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_1_A },
			{ 23, 20, 0b0010, CPU_FEATURE_PAN2,   FEATURE_LEVEL_ARM_V8_1_A, FEATURE_LEVEL_ARM_V8_2_A },
			{ 23, 20, 0b0011, CPU_FEATURE_PAN3,   FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_7_A },
			{ 19, 16, 0b0001, CPU_FEATURE_LOR,    FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_1_A },
			{ 15, 12, 0b0001, CPU_FEATURE_HPDS,   FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_1_A },
			{ 15, 12, 0b0010, CPU_FEATURE_HPDS2,  FEATURE_LEVEL_ARM_V8_1_A, -1 },
			{ 11,  8, 0b0001, CPU_FEATURE_VHE,    FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{  7,  4, 0b0010, CPU_FEATURE_VMID16, FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{  3,  0, 0b0001, CPU_FEATURE_HAFDBS, FEATURE_LEVEL_ARM_V8_1_A, -1 },
			{  3,  0, 0b0010, CPU_FEATURE_HAFDBS, FEATURE_LEVEL_ARM_V8_1_A, -1 }, /* as 0b0001, and adds support for hardware update of the Access flag for Block and Page descriptors */
			{  3,  0, 0b0011, CPU_FEATURE_HAFT,   FEATURE_LEVEL_ARM_V8_7_A, -1 },
			{ -1, -1,     -1, -1,                 -1,                       -1 }
		},
		[2] /* ID_AA64MMFR2 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_E0PD,   FEATURE_LEVEL_ARM_V8_4_A, FEATURE_LEVEL_ARM_V8_5_A },
			{ 59, 56, 0b0001, CPU_FEATURE_EVT,    FEATURE_LEVEL_ARM_V8_2_A, -1 }, /* HCR_EL2.{TOCU, TICAB, TID4} traps are supported. HCR_EL2.{TTLBOS,TTLBIS} traps are not supported */
			{ 59, 56, 0b0010, CPU_FEATURE_EVT,    FEATURE_LEVEL_ARM_V8_2_A, -1 }, /* HCR_EL2.{TTLBOS, TTLBIS, TOCU, TICAB, TID4} traps are supported */
			{ 55, 52, 0b0000, CPU_FEATURE_BBM,    FEATURE_LEVEL_ARM_V8_3_A, FEATURE_LEVEL_ARM_V8_4_A }, /* Level 0 support for changing block size is supported */
			{ 55, 52, 0b0001, CPU_FEATURE_BBM,    FEATURE_LEVEL_ARM_V8_3_A, FEATURE_LEVEL_ARM_V8_4_A }, /* Level 1 support for changing block size is supported */
			{ 55, 52, 0b0010, CPU_FEATURE_BBM,    FEATURE_LEVEL_ARM_V8_3_A, FEATURE_LEVEL_ARM_V8_4_A }, /* Level 2 support for changing block size is supported */
			{ 51, 48, 0b0001, CPU_FEATURE_TTL,    FEATURE_LEVEL_ARM_V8_3_A, FEATURE_LEVEL_ARM_V8_4_A },
			{ 43, 40, 0b0001, CPU_FEATURE_S2FWB,  FEATURE_LEVEL_ARM_V8_3_A, -1 },
			{ 39, 36, 0b0001, CPU_FEATURE_IDST,   FEATURE_LEVEL_ARM_V8_3_A, FEATURE_LEVEL_ARM_V8_4_A },
			{ 35, 32, 0b0001, CPU_FEATURE_LSE2,   FEATURE_LEVEL_ARM_V8_2_A, FEATURE_LEVEL_ARM_V8_4_A },
			{ 31, 28, 0b0001, CPU_FEATURE_TTST,   FEATURE_LEVEL_ARM_V8_3_A, -1 },
			{ 23, 20, 0b0001, CPU_FEATURE_CCIDX,  FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 19, 16, 0b0001, CPU_FEATURE_LVA,    FEATURE_LEVEL_ARM_V8_1_A, -1 },
			{ 19, 16, 0b0010, CPU_FEATURE_LVA3,   FEATURE_LEVEL_ARM_V9_3_A, -1 },
			{ 15, 12, 0b0001, CPU_FEATURE_IESB,   FEATURE_LEVEL_ARM_V8_1_A, -1 },
			{ 11,  8, 0b0001, CPU_FEATURE_LSMAOC, FEATURE_LEVEL_ARM_V8_1_A, -1 },
			{  7,  4, 0b0001, CPU_FEATURE_UAO,    FEATURE_LEVEL_ARM_V8_1_A, FEATURE_LEVEL_ARM_V8_2_A },
			{  3,  0, 0b0001, CPU_FEATURE_TTCNP,  FEATURE_LEVEL_ARM_V8_1_A, FEATURE_LEVEL_ARM_V8_2_A },
			{ -1, -1,     -1, -1,                  -1,                       -1 }
		},
		[3] /* ID_AA64MMFR3 */ = {
			{ 59, 56, 0b0010, CPU_FEATURE_ADERR,  FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 55, 52, 0b0010, CPU_FEATURE_ADERR,  FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 47, 44, 0b0010, CPU_FEATURE_ANERR,  FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 43, 40, 0b0010, CPU_FEATURE_ANERR,  FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 35, 32, 0b0001, CPU_FEATURE_D128,   FEATURE_LEVEL_ARM_V9_3_A, -1 },
			{ 31, 28, 0b0001, CPU_FEATURE_MEC,    FEATURE_LEVEL_ARM_V9_2_A, -1 },
			{ 27, 24, 0b0001, CPU_FEATURE_AIE,    FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 23, 20, 0b0001, CPU_FEATURE_S2POE,  FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 19, 16, 0b0001, CPU_FEATURE_S1POE,  FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 15, 12, 0b0001, CPU_FEATURE_S2PIE,  FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 11,  8, 0b0001, CPU_FEATURE_S1PIE,  FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{  7,  4, 0b0001, CPU_FEATURE_SCTLR2, FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_9_A },
			{  3,  0, 0b0001, CPU_FEATURE_TCR2,   FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_9_A },
			{ -1, -1,     -1, -1,                 -1,                       -1 }
		},
		[4] /* ID_AA64MMFR4 */ = {
			{ -1, -1,     -1, -1,                 -1,                       -1 }
		}
	};

	const struct arm_feature_map_t matchtable_id_aa64pfr[MAX_ARM_ID_AA64PFR_REGS][MAX_MATCHTABLE_ITEMS] = {
		[0] /* ID_AA64PFR0 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_CSV3,        FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_5_A },
			{ 59, 56, 0b0001, CPU_FEATURE_CSV2,        FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_5_A },
			{ 59, 56, 0b0010, CPU_FEATURE_CSV2_2,      FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{ 59, 56, 0b0011, CPU_FEATURE_CSV2_3,      FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{ 55, 52, 0b0001, CPU_FEATURE_RME,         FEATURE_LEVEL_ARM_V9_1_A, -1 },
			{ 51, 48, 0b0001, CPU_FEATURE_DIT,         FEATURE_LEVEL_ARM_V8_3_A, FEATURE_LEVEL_ARM_V8_4_A },
			{ 47, 44, 0b0001, CPU_FEATURE_AMUV1,       FEATURE_LEVEL_ARM_V8_3_A, -1 },
			{ 47, 44, 0b0010, CPU_FEATURE_AMUV1P1,     FEATURE_LEVEL_ARM_V8_5_A, -1 },
			{ 39, 36, 0b0001, CPU_FEATURE_SEL2,        FEATURE_LEVEL_ARM_V8_3_A, -1 },
			{ 35, 32, 0b0001, CPU_FEATURE_SVE,         FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 31, 28, 0b0001, CPU_FEATURE_RAS,         FEATURE_LEVEL_ARM_V8_0_A, FEATURE_LEVEL_ARM_V8_2_A },
			{ 31, 28, 0b0010, CPU_FEATURE_DOUBLEFAULT, FEATURE_LEVEL_ARM_V8_4_A, -1 },
			{ 31, 28, 0b0010, CPU_FEATURE_RASV1P1,     FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 31, 28, 0b0011, CPU_FEATURE_RASV2,       FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 23, 20, 0b0000, CPU_FEATURE_ADVSIMD,     FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{ 23, 20, 0b0001, CPU_FEATURE_ADVSIMD,     FEATURE_LEVEL_ARM_V8_0_A, -1 }, /* as for 0b0000, and also includes support for half-precision floating-point arithmetic */
			{ 19, 16, 0b0000, CPU_FEATURE_FP,          FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{ 19, 16, 0b0001, CPU_FEATURE_FP16,        FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ -1, -1,     -1, -1,                      -1,                       -1 }
		},
		[1] /* ID_AA64PFR1 */ = {
			{ 63, 60, 0b0001, CPU_FEATURE_PFAR,           FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 59, 56, 0b0001, CPU_FEATURE_DOUBLEFAULT2,   FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 51, 48, 0b0001, CPU_FEATURE_THE,            FEATURE_LEVEL_ARM_V8_8_A, -1 },
			{ 47, 44, 0b0001, CPU_FEATURE_GCS,            FEATURE_LEVEL_ARM_V9_3_A, -1 },
			{ 39, 36, 0b0001, CPU_FEATURE_NMI,            FEATURE_LEVEL_ARM_V8_7_A, FEATURE_LEVEL_ARM_V8_8_A },
			{ 35, 32, 0b0001, CPU_FEATURE_CSV2_1P1,       FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{ 35, 32, 0b0010, CPU_FEATURE_CSV2_1P2,       FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{ 31, 28, 0b0001, CPU_FEATURE_RNG_TRAP,       FEATURE_LEVEL_ARM_V8_4_A, -1 },
			{ 27, 24, 0b0001, CPU_FEATURE_SME,            FEATURE_LEVEL_ARM_V9_2_A, -1 },
			{ 27, 24, 0b0010, CPU_FEATURE_SME2,           FEATURE_LEVEL_ARM_V9_2_A, -1 },
			{ 11,  8, 0b0001, CPU_FEATURE_MTE,            FEATURE_LEVEL_ARM_V8_4_A, -1 },
			{ 11,  8, 0b0010, CPU_FEATURE_MTE2,           FEATURE_LEVEL_ARM_V8_4_A, -1 },
			{ 11,  8, 0b0011, CPU_FEATURE_MTE3,           FEATURE_LEVEL_ARM_V8_5_A, -1 },
			{ 11,  8, 0b0011, CPU_FEATURE_MTE_ASYM_FAULT, FEATURE_LEVEL_ARM_V8_7_A, -1 }, /* FEAT_MTE3 is implemented if and only if FEAT_MTE_ASYM_FAULT is implemented */
			{  7,  4, 0b0001, CPU_FEATURE_SSBS,           FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{  7,  4, 0b0010, CPU_FEATURE_SSBS2,          FEATURE_LEVEL_ARM_V8_0_A, -1 },
			{  3,  0, 0b0001, CPU_FEATURE_BTI,            FEATURE_LEVEL_ARM_V8_4_A, FEATURE_LEVEL_ARM_V8_5_A },
			{ -1, -1,     -1, -1,                         -1,                       -1 }
		},
		[2] /* ID_AA64PFR2 */ = {
			{ 11,  8, 0b0001, CPU_FEATURE_MTE_TAGGED_FAR, FEATURE_LEVEL_ARM_V8_7_A, -1 },
			{  7,  4, 0b0001, CPU_FEATURE_MTE_STORE_ONLY, FEATURE_LEVEL_ARM_V8_7_A, -1 },
			{  3,  0, 0b0001, CPU_FEATURE_MTE_PERM,       FEATURE_LEVEL_ARM_V8_7_A, -1 },
			{ -1, -1,     -1, -1,                         -1,                       -1 }
		}
	};

	const struct arm_feature_map_t matchtable_id_aa64smfr[MAX_ARM_ID_AA64SMFR_REGS][MAX_MATCHTABLE_ITEMS] = {
		[0] /* ID_AA64SMFR0 */ = {
			{ 63, 63,    0b1, CPU_FEATURE_SME_FA64,   FEATURE_LEVEL_ARM_V9_2_A, -1 },
			{ 59, 56, 0b0001, CPU_FEATURE_SME2,       FEATURE_LEVEL_ARM_V9_2_A, -1 },
			{ 59, 56, 0b0010, CPU_FEATURE_SME2P1,     FEATURE_LEVEL_ARM_V9_2_A, -1 },
			{ 55, 52, 0b1111, CPU_FEATURE_SME_I16I64, FEATURE_LEVEL_ARM_V9_2_A, -1 },
			{ 48, 48,    0b1, CPU_FEATURE_SME_F64F64, FEATURE_LEVEL_ARM_V9_2_A, -1 },
			{ 43, 43,    0b1, CPU_FEATURE_SVE_B16B16, FEATURE_LEVEL_ARM_V9_2_A, -1 },
			{ 42, 42,    0b1, CPU_FEATURE_SME_F16F16, FEATURE_LEVEL_ARM_V9_2_A, -1 },
			{ -1, -1,     -1, -1,                     -1,                       -1 }
		},
	};

	const struct arm_feature_map_t matchtable_id_aa64zfr[MAX_ARM_ID_AA64ZFR_REGS][MAX_MATCHTABLE_ITEMS] = {
		[0] /* ID_AA64ZFR0 */ = {
			{ 59, 56, 0b0001, CPU_FEATURE_F64MM,        FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 55, 52, 0b0001, CPU_FEATURE_F32MM,        FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 47, 44, 0b0001, CPU_FEATURE_I8MM,         FEATURE_LEVEL_ARM_V8_1_A, FEATURE_LEVEL_ARM_V8_6_A },
			{ 43, 40, 0b0001, CPU_FEATURE_SVE_SM4,      FEATURE_LEVEL_ARM_V9_0_A, -1 },
			{ 35, 32, 0b0001, CPU_FEATURE_SVE_SHA3,     FEATURE_LEVEL_ARM_V9_0_A, -1 },
			{ 27, 24, 0b0001, CPU_FEATURE_SVE_B16B16,   FEATURE_LEVEL_ARM_V9_2_A, -1 },
			{ 23, 20, 0b0001, CPU_FEATURE_BF16,         FEATURE_LEVEL_ARM_V8_2_A, FEATURE_LEVEL_ARM_V8_6_A },
			{ 23, 20, 0b0010, CPU_FEATURE_EBF16,        FEATURE_LEVEL_ARM_V8_2_A, -1 },
			{ 19, 16, 0b0001, CPU_FEATURE_SVE_BITPERM,  FEATURE_LEVEL_ARM_V9_0_A, -1 },
			{  7,  4, 0b0001, CPU_FEATURE_SVE_AES,      FEATURE_LEVEL_ARM_V9_0_A, -1 },
			{  7,  4, 0b0010, CPU_FEATURE_SVE_PMULL128, FEATURE_LEVEL_ARM_V9_0_A, -1 },
			{  3,  0, 0b0001, CPU_FEATURE_SVE2,         FEATURE_LEVEL_ARM_V9_0_A, -1 },
			{  3,  0, 0b0001, CPU_FEATURE_SME,          FEATURE_LEVEL_ARM_V9_2_A, -1 },
			{  3,  0, 0b0010, CPU_FEATURE_SME2P1,       FEATURE_LEVEL_ARM_V9_2_A, -1 },
			{  3,  0, 0b0010, CPU_FEATURE_SVE2P1,       FEATURE_LEVEL_ARM_V9_2_A, -1 },
			{ -1, -1,     -1, -1,                       -1,                       -1 }
		},
	};

	for (i = 0; i < MAX_ARM_ID_AA64DFR_REGS; i++)
		MATCH_FEATURES_TABLE_WITH_RAW(aa64dfr);

	for (i = 0; i < MAX_ARM_ID_AA64ISAR_REGS; i++)
		MATCH_FEATURES_TABLE_WITH_RAW(aa64isar);

	for (i = 0; i < MAX_ARM_ID_AA64MMFR_REGS; i++)
		MATCH_FEATURES_TABLE_WITH_RAW(aa64mmfr);

	for (i = 0; i < MAX_ARM_ID_AA64PFR_REGS; i++)
		MATCH_FEATURES_TABLE_WITH_RAW(aa64pfr);

	for (i = 0; i < MAX_ARM_ID_AA64SMFR_REGS; i++)
		MATCH_FEATURES_TABLE_WITH_RAW(aa64smfr);

	for (i = 0; i < MAX_ARM_ID_AA64ZFR_REGS; i++)
		MATCH_FEATURES_TABLE_WITH_RAW(aa64zfr);

	/* Some fields do not follow the standard ID scheme */
	load_arm_feature_pauth(data, ext_status);
	load_arm_feature_mpam(raw, data, ext_status);
	load_arm_feature_mte4(raw, data, ext_status);
}
#undef MAX_MATCHTABLE_ITEMS
#undef MATCH_FEATURES_TABLE_WITH_RAW

int cpuid_identify_arm(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	struct arm_arch_extension_t ext_status;
	memset(&ext_status, 0, sizeof(struct arm_arch_extension_t));

	/* Basic values extraction
	   Based on "Arm Architecture Reference Manual for A-profile architecture", section D23.2.122 "MIDR_EL1, Main ID Register"
	*/
	data->arm.implementer = EXTRACTS_BITS(raw->arm_midr, 31, 24);
	data->arm.variant     = EXTRACTS_BITS(raw->arm_midr, 23, 20);
	data->arm.part_num    = EXTRACTS_BITS(raw->arm_midr, 15,  4);
	data->arm.revision    = EXTRACTS_BITS(raw->arm_midr,  3,  0);
	data->purpose         = cpuid_identify_purpose_arm(raw);

	/* Values decoding */
	const struct arm_hw_impl* hw_impl = get_cpu_implementer_from_code(data->arm.implementer);
	const struct arm_id_part* id_part = get_cpu_implementer_parts(hw_impl, data->arm.part_num);
	data->vendor = hw_impl->vendor;
	strncpy_s(data->vendor_str, VENDOR_STR_MAX, hw_impl->name, VENDOR_STR_MAX);
	strncpy_s(data->brand_str, BRAND_STR_MAX, id_part->name, BRAND_STR_MAX);
	strncpy_s(data->cpu_codename, CODENAME_STR_MAX, id_part->codename, CODENAME_STR_MAX);
	load_arm_features(raw, data, &ext_status);
	decode_arm_architecture_version(raw, data, &ext_status);

	return 0;
}

cpu_purpose_t cpuid_identify_purpose_arm(struct cpu_raw_data_t* raw)
{
	const uint8_t implementer         = EXTRACTS_BITS(raw->arm_midr, 31, 24);
	const uint16_t part_num           = EXTRACTS_BITS(raw->arm_midr, 15,  4);
	const struct arm_hw_impl* hw_impl = get_cpu_implementer_from_code(implementer);
	const struct arm_id_part* id_part = get_cpu_implementer_parts(hw_impl, part_num);

	/* ARM big.LITTLE Typical Processor Combinations: https://www.arm.com/technologies/big-little */
	if (match_pattern(id_part->name, "Cortex-X[012356789]"))
		return PURPOSE_U_PERFORMANCE;
	else if (match_pattern(id_part->name, "Cortex-A[67][012356789]") || match_pattern(id_part->name, "Cortex-A[5][6789]"))
		return PURPOSE_PERFORMANCE;
	else if (match_pattern(id_part->name, "Cortex-A[5][012345]") || match_pattern(id_part->name, "Cortex-A[3][0123456789]"))
		return PURPOSE_EFFICIENCY;

	return PURPOSE_GENERAL;
}
