// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdint.h>
#include <stdbool.h>

enum intel_cpu_type
{
	INTEL_CPU_TYPE_UNKNOWN = 0,
	INTEL_CPU_TYPE_XEON_PHI,
	INTEL_CPU_TYPE_PENTIUM,
	INTEL_CPU_TYPE_ATOM,
	INTEL_CPU_TYPE_CORE,
};

enum intel_xeon_phi_microarch
{
	INTEL_XEON_PHI_UNKNOWN = 0,
	INTEL_XEON_PHI_KNL, /* Knights Landing */
	INTEL_XEON_PHI_KNM, /* Knights Mill */
};

enum intel_pentium_microarch
{
	INTEL_PENTIUM_UNKNOWN = 0,
	INTEL_PENTIUM_PRO,
	INTEL_PENTIUM_II_KLAMATH,
	INTEL_PENTIUM_III_DESCHUTES,
	INTEL_PENTIUM_III_TUALATIN,
	INTEL_PENTIUM_M_DOTHAN,
	INTEL_P4_WILLAMETTE, /* Also Xeon Foster */
	INTEL_P4_PRESCOTT,
	INTEL_P4_PRESCOTT_2M,
	INTEL_P4_CEDARMILL,
};

enum intel_atom_microarch
{
	INTEL_ATOM_UNKNOWN = 0,
	INTEL_ATOM_BONNELL, /* Diamondville, Pineview */
	INTEL_ATOM_BONNELL_MID, /* Silverthorne, Lincroft */
	INTEL_ATOM_SALTWELL, /* Cedarview */
	INTEL_ATOM_SALTWELL_MID, /* Penwell */
	INTEL_ATOM_SALTWELL_TABLET, /* Cloverview */
	INTEL_ATOM_SILVERMONT, /* Bay Trail, Valleyview */
	INTEL_ATOM_SILVERMONT_D, /* Avaton, Rangely */
	INTEL_ATOM_SILVERMONT_MID, /* Merriefield */
	INTEL_ATOM_SILVERMONT_MID2, /* Anniedale */
	INTEL_ATOM_AIRMONT, /* Cherry Trail, Braswell */
	INTEL_ATOM_AIRMONT_NP, /* Lightning Mountain */
	INTEL_ATOM_GOLDMONT, /* Apollo Lake */
	INTEL_ATOM_GOLDMONT_D, /* Denverton */
	INTEL_ATOM_GOLDMONT_PLUS, /* Gemini Lake */
	INTEL_ATOM_TREMONT_D, /* Jacobsville */
	INTEL_ATOM_TREMONT, /* Elkhart Lake */
	INTEL_ATOM_TREMONT_L, /* Jasper Lake */
	INTEL_ATOM_GRACEMONT, /* Alderlake N */
	INTEL_ATOM_CRESTMONT_X, /* Sierra Forest */
	INTEL_ATOM_CRESTMONT, /* Grand Ridge */
	INTEL_ATOM_DARKMONT_X, /* Clearwater Forest */
};

enum intel_core_microarch
{
	INTEL_CORE_UNKNOWN = 0,
	INTEL_CORE_YONAH,
	INTEL_CORE2_MEROM,
	INTEL_CORE2_MEROM_L,
	INTEL_CORE2_PENRYN,
	INTEL_CORE2_DUNNINGTON,
	INTEL_NEHALEM,
	INTEL_NEHALEM_G,
	INTEL_NEHALEM_EP,
	INTEL_NEHALEM_EX,
	INTEL_WESTMERE,
	INTEL_WESTMERE_EP,
	INTEL_WESTMERE_EX,
	INTEL_SANDYBRIDGE, /* Core 2nd Gen */
	INTEL_SANDYBRIDGE_X,
	INTEL_IVYBRIDGE, /* Core 3rd Gen */
	INTEL_IVYBRIDGE_X,
	INTEL_HASWELL, /* Core 4th Gen */
	INTEL_HASWELL_X,
	INTEL_HASWELL_L,
	INTEL_HASWELL_G,
	INTEL_BROADWELL, /* Core 5th Gen */
	INTEL_BROADWELL_G,
	INTEL_BROADWELL_X,
	INTEL_BROADWELL_D,
	INTEL_SKYLAKE_L, /* Core 6th Gen */
	INTEL_SKYLAKE,
	INTEL_SKYLAKE_X,
	INTEL_KABYLAKE_L,
	INTEL_KABYLAKE,
	INTEL_COMETLAKE, /* Core 10th Gen */
	INTEL_COMETLAKE_L,
	INTEL_CANNONLAKE_L, /* Palm Cove */
	INTEL_ICELAKE_X, /* Sunny Cove */
	INTEL_ICELAKE_D, /* Sunny Cove */
	INTEL_ICELAKE, /* Sunny Cove */
	INTEL_ICELAKE_L, /* Sunny Cove */
	INTEL_ICELAKE_NNPI, /* Sunny Cove */
	INTEL_ROCKETLAKE, /* Core 11th Gen Cypress Cove */
	INTEL_TIGERLAKE_L, /* Willow Cove */
	INTEL_TIGERLAKE, /* Willow Cove */
	INTEL_SAPPHIRERAPIDS_X, /* Golden Cove */
	INTEL_EMERALDRAPIDS_X, /* Raptor Cove */
	INTEL_GRANITERAPIDS_X, /* Redwood Cove */
	INTEL_GRANITERAPIDS_D,
	INTEL_DIAMONDRAPIDS_X,
	INTEL_BARTLETTLAKE, /* Raptor Cove */
	INTEL_LAKEFIELD, /* Sunny Cove / Tremont */
	INTEL_ALDERLAKE, /* Golden Cove / Gracemont */
	INTEL_ALDERLAKE_L, /* Golden Cove / Gracemont */
	INTEL_RAPTORLAKE, /* Raptor Cove / Enhanced Gracemont */
	INTEL_RAPTORLAKE_P,
	INTEL_RAPTORLAKE_S,
	INTEL_METEORLAKE, /* Redwood Cove / Crestmont */
	INTEL_METEORLAKE_L,
	INTEL_ARROWLAKE_H, /* Lion Cove / Skymont */
	INTEL_ARROWLAKE,
	INTEL_ARROWLAKE_U,
	INTEL_LUNARLAKE_M, /* Lion Cove / Skymont */
	INTEL_PANTHERLAKE_L, /* Cougar Cove / Darkmont */
	INTEL_WILDCATLAKE_L,
	INTEL_NOVALAKE, /* Coyote Cove / Arctic Wolf */
	INTEL_NOVALAKE_L, /* Coyote Cove / Arctic Wolf */
};

typedef union
{
	struct
	{
		uint64_t stepping : 4;
		uint64_t model : 16;
		uint64_t family : 16;
		uint64_t microarch : 8;
		uint64_t cpu_type : 8;
		uint64_t reserved : 12;
	};
	uint64_t data;
} intel_microarch_t;

intel_microarch_t
mchbar_get_microarch(void);

uint64_t
mchbar_get_mmio_reg(void);

uint64_t
pch_get_mmio_reg(void);

uint32_t
mchbar_read_32(uint64_t offset);

uint32_t
pch_read_32(uint64_t offset);

uint64_t
mchbar_read_64(uint64_t offset);

bool
mchbar_pch_init(void);
