// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"
#include "sensors.h"
#include "libcpuid.h"
#include "ioctl.h"

#define SLOT_NAME_SIZE 16
#define SLOT_COUNT 8

struct dimm_slot
{
	char name[SLOT_NAME_SIZE];
	uint16_t tCL;
	uint16_t tRCD;
	uint16_t tRP;
	uint16_t tRAS;
	uint16_t tRC;
};

static struct
{
	struct cpu_id_t* id;
	uint64_t mmio_reg;
	uint16_t width;
	uint16_t freq;
	uint16_t ddr;
	struct dimm_slot slot[SLOT_COUNT];
	void (*get) (void);
} ctx;

// https://doc.coreboot.org/northbridge/intel/sandybridge/nri_registers.html
// https://github.com/remittor/pyhwinfo

#define MCHBAR_BASE_REG_LOW     0x48
#define MCHBAR_BASE_REG_HIGH    0x4C

static inline uint32_t
get_mmio_32(uint64_t offset)
{
	uint32_t data = 0;
	int ret = WR0_RdMmIo(NWLC->NwDrv, ctx.mmio_reg + offset, &data, sizeof(uint32_t));
	if (ret)
		NWL_Debug("IMC", "MMIO 32 @%llX+%llx failed.", ctx.mmio_reg, offset);
	return data;
}

static inline uint64_t
get_mmio_64(uint64_t offset)
{
	uint64_t data = 0;
	int ret = WR0_RdMmIo(NWLC->NwDrv, ctx.mmio_reg + offset, &data, sizeof(uint64_t));
	if (ret)
		NWL_Debug("IMC", "MMIO 64 @%llX+%llx failed.", ctx.mmio_reg, offset);
	return data;
}

#define SNB_REG_CH1_OFFSET  0x0400

#define SNB_REG_MAIN_CHAN0  0x5004
#define SNB_REG_MAIN_CHAN1  0x5008
#define SNB_REG_MCH_CFG     0x5E04
#define SNB_REG_TIMING      0x4000

// MCHBAR_0_0_0_PCI 0x48 38:15 Base 32KB
//   CH0 0x4000 CH1 0x4400
// MAD_DIMM_ch0 0x5004 (32)
// MAD_DIMM_ch1 0x5008 (32)
//   15:8  DIMM_B_Size: Size of DIMM B in 256 MB multiples
//    7:0  DIMM_A_SIZE: Size of DIMM A in 256 MB multiples
// 0x4000
//   23:16 tRAS
//   11:8  tCL
//    7:4  tRP
//    3:0  tRCD
static void
get_intel_mchbar_2_3(void)
{
	//FIXME
	ctx.freq = 0;

	ctx.ddr = 3;

	uint32_t ofs_5004 = get_mmio_32(SNB_REG_MAIN_CHAN0);
	uint32_t ofs_5008 = get_mmio_32(SNB_REG_MAIN_CHAN1);
	uint32_t ch_size[2] = { 0 };
	ch_size[0] = ((ofs_5004 >> 8) & 0xFF) + (ofs_5004 & 0xFF);
	ch_size[1] = ((ofs_5008 >> 8) & 0xFF) + (ofs_5008 & 0xFF);

	for (uint64_t ch = 0; ch < 2; ch++)
	{
		if (ch_size[ch] == 0)
			continue;
		uint64_t ch_ofs = ch * SNB_REG_CH1_OFFSET;
		uint32_t ofs_4000 = get_mmio_32(ch_ofs + SNB_REG_TIMING);
		struct dimm_slot* p = &ctx.slot[ch];
		snprintf(p->name, SLOT_NAME_SIZE, "CH%llu", ch);
		p->tCL = (ofs_4000 >> 8) & 0x0F;
		p->tRP = (ofs_4000 >> 4) & 0x0F;
		p->tRAS = (ofs_4000 >> 16) & 0xFF;
		p->tRCD = ofs_4000 & 0x0F;
		p->tRC = p->tRAS + p->tRP;
	}

	ctx.width = (ch_size[0] && ch_size[1]) ? 128 : 64;
}

#define HSW_REG_CH1_OFFSET  0x4000 // ?

#define HSW_REG_MAIN_CHAN0  0x5004
#define HSW_REG_MAIN_CHAN1  0x5008
#define HSW_REG_DRAM_CLOCK  0x5E00
#define HSW_REG_MCH_CFG     0x5E04
#define HSW_REG_TIMING_CAS  0x4014
#define HSW_REG_TIMING_RCD  0x4000

// MCHBAR_0_0_0_PCI 0x48 38:15 Base 32KB
//   CH0 0x4000 CH1 0x4400
// MAD_DIMM_CH0_0_0_0_MCHBAR 0x5004 (32)
// MAD_DIMM_CH1_0_0_0_MCHBAR 0x5008 (32)
//   15:8  DIMM_B_Size: Size of DIMM B in 256 MB multiples
//    7:0  DIMM_A_SIZE: Size of DIMM A in 256 MB multiples
// PCU_CR_MC_BIOS_REQ_0_0_0_MCHBAR_PCU 0x5E00 (32)
//    3:0  REQ_DATA: Memory Controller Frequency
//                   These 4 bits are the data for the request. The only possible request type
//                   is MC frequency request.The encoding of this field is the 133 / 266 MHz multiplier for
//                   DCLK / QCLK: Binary Dec DCLK Equation DCLK Freq QCLK Equation QCLK Freq
//                   0000b 0d---------------------------- - MC PLL ¨C shutdown------------------------------
//                   0011b 3d 3 * 133.33 400.00 MHz 3 * 266.67 MHz 800.00 MHz
//                   0100b 4d 4 * 133.33 533.33 MHz 4 * 266.67 MHz 1066.67 MHz
//                   0101b 5d 5 * 133.33 666.67 MHz 5 * 266.67 MHz 1333.33 MHz
//                   0110b 6d 6 * 133.33 800.00 MHz 6 * 266.67 MHz 1600.00 MHz
//                   0111b 7d 7 * 133.33 933.33 MHz 7 * 266.67 MHz 1866.67 MHz
//                   1000b 8d 8 * 133.33 1066.67MHz 8 * 266.67 MHz 2133.33 MHz
// 0x4000
//   15:10 tRAS
//    9:5  tRP
//    4:0  tRCD
// 0x4014
//    4:0  tCL
static void
get_intel_mchbar_4_5(void)
{
	uint32_t ofs_5e00 = get_mmio_32(HSW_REG_DRAM_CLOCK);

	// FIXME
	switch (ofs_5e00 & 0x0F)
	{
	case 0x03:
		ctx.freq = 800;
		break;
	case 0x04:
		ctx.freq = 1067;
		break;
	case 0x05:
		ctx.freq = 1333;
		break;
	case 0x06:
		ctx.freq = 1600;
		break;
	case 0x07:
		ctx.freq = 1867;
		break;
	case 0x08:
		ctx.freq = 2133;
		break;
	default:
		ctx.freq = 0;
		break;
	}

	ctx.ddr = 3;

	uint32_t ofs_5004 = get_mmio_32(HSW_REG_MAIN_CHAN0);
	uint32_t ofs_5008 = get_mmio_32(HSW_REG_MAIN_CHAN1);
	uint32_t ch_size[2] = { 0 };
	ch_size[0] = ((ofs_5004 >> 8) & 0xFF) + (ofs_5004 & 0xFF);
	ch_size[1] = ((ofs_5008 >> 8) & 0xFF) + (ofs_5008 & 0xFF);

	for (uint64_t ch = 0; ch < 2; ch++)
	{
		if (ch_size[ch] == 0)
			continue;
		uint64_t ch_ofs = ch * HSW_REG_CH1_OFFSET;
		uint32_t ofs_4000 = get_mmio_32(ch_ofs + HSW_REG_TIMING_RCD);
		uint32_t ofs_4014 = get_mmio_32(ch_ofs + HSW_REG_TIMING_CAS);
		struct dimm_slot* p = &ctx.slot[ch];
		snprintf(p->name, SLOT_NAME_SIZE, "CH%llu", ch);
		p->tCL = ofs_4014 & 0x1F;
		p->tRP = (ofs_4000 >> 5) & 0x1F;
		p->tRAS = (ofs_4000 >> 10) & 0x3F;
		p->tRCD = ofs_4000 & 0x1F;
		p->tRC = p->tRAS + p->tRP;
	}

	ctx.width = (ch_size[0] && ch_size[1]) ? 128 : 64;
}

#define SKL_MMR_CH1_OFFSET      0x400
#define SKL_MMR_TIMINGS         0x4000
#define SKL_MMR_SCHEDULER_CONF  0x401C
#define SKL_MMR_TIMING_CAS      0x4070
#define SKL_MMR_MAD_CHAN0       0x500C
#define SKL_MMR_MAD_CHAN1       0x5010
#define SKL_MMR_DRAM_CLOCK      0x5E00

// MCHBAR_0_0_0_PCI 0x48 38:15 Base 32KB
//   CH0 0x4000 CH1 0x4400
// MAD_DIMM_CH0_0_0_0_MCHBAR 0x500C (32)
// MAD_DIMM_CH1_0_0_0_MCHBAR 0x5010 (32)
//   21:16 DIMM_S_SIZE: Size of DIMM S in 1GB multiples
//    5:0  DIMM_L_SIZE: Size of DIMM L in 1GB multiples
// PCU_CR_MC_BIOS_REQ_0_0_0_MCHBAR_PCU 0x5E00 (32)
//    3:0  REQ_DATA: Memory Controller Frequency
//                   These 4 bits are the data for the request. The only possible request type
//                   is MC frequency request.The encoding of this field is the 133 / 266 MHz multiplier for
//                   DCLK / QCLK: Binary Dec DCLK Equation DCLK Freq QCLK Equation QCLK Freq
//                   0000b 0d---------------------------- - MC PLL ¨C shutdown------------------------------
//                   0011b 3d 3 * 133.33 400.00 MHz 3 * 266.67 MHz 800.00 MHz
//                   0100b 4d 4 * 133.33 533.33 MHz 4 * 266.67 MHz 1066.67 MHz
//                   0101b 5d 5 * 133.33 666.67 MHz 5 * 266.67 MHz 1333.33 MHz
//                   0110b 6d 6 * 133.33 800.00 MHz 6 * 266.67 MHz 1600.00 MHz
//                   0111b 7d 7 * 133.33 933.33 MHz 7 * 266.67 MHz 1866.67 MHz
//                   1000b 8d 8 * 133.33 1066.67MHz 8 * 266.67 MHz 2133.33 MHz
// MCHBAR_CH0_CR_TC_PRE_0_0_0_MCHBAR 0x4000 (32)
//   14:8  tRAS
//    5:0  tRP tRCD
// MCHBAR_CH0_CR_SC_GS_CFG_0_0_0_MCHBAR 0x401C (32)
//    1:0  DRAM_technology: 00=DDR4, 01=DDR3, 10=LPDDR3
// MCHBAR_CH0_CR_TC_ODT_0_0_0_MCHBAR 0x4070 (32)
//   20:16 tCL

static void
get_intel_mchbar_6_7_8_9(void)
{
	uint32_t ofs_5e00 = get_mmio_32(SKL_MMR_DRAM_CLOCK);

	// FIXME
	switch (ofs_5e00 & 0x0F)
	{
	case 0x03:
		ctx.freq = 800;
		break;
	case 0x04:
		ctx.freq = 1067;
		break;
	case 0x05:
		ctx.freq = 1333;
		break;
	case 0x06:
		ctx.freq = 1600;
		break;
	case 0x07:
		ctx.freq = 1867;
		break;
	case 0x08:
		ctx.freq = 2133;
		break;
	default:
		ctx.freq = 0;
		break;
	}

	uint32_t ofs_500c = get_mmio_32(SKL_MMR_MAD_CHAN0);
	uint32_t ofs_5010 = get_mmio_32(SKL_MMR_MAD_CHAN1);
	uint32_t ch_size[2] = { 0 };
	ch_size[0] = ((ofs_500c >> 16) & 0x3F) + (ofs_500c & 0x3F);
	ch_size[1] = ((ofs_5010 >> 16) & 0x3F) + (ofs_5010 & 0x3F);

	for (uint64_t ch = 0; ch < 2; ch++)
	{
		if (ch_size[ch] == 0)
			continue;
		uint64_t ch_ofs = ch * SKL_MMR_CH1_OFFSET;
		uint32_t ofs_4070 = get_mmio_32(ch_ofs + SKL_MMR_TIMING_CAS);
		uint32_t ofs_4000 = get_mmio_32(ch_ofs + SKL_MMR_TIMINGS);
		uint32_t ofs_401c = get_mmio_32(ch_ofs + SKL_MMR_SCHEDULER_CONF);
		if ((ofs_401c & 0x03) == 1 || (ofs_401c & 0x03) == 2)
			ctx.ddr = 3;
		struct dimm_slot* p = &ctx.slot[ch];
		snprintf(p->name, SLOT_NAME_SIZE, "CH%llu", ch);
		p->tCL = (ofs_4070 >> 16) & 0x1F;
		p->tRP = ofs_4000 & 0x3F;
		p->tRAS = (ofs_4000 >> 8) & 0x7F;
		p->tRCD = ofs_4000 & 0x3F;
		p->tRC = p->tRAS + p->tRP;
	}

	if (!ctx.ddr)
		ctx.ddr = 4;
	ctx.width = (ch_size[0] && ch_size[1]) ? 128 : 64;
}

#define ICL_MMR_CH1_OFFSET      0x400
#define ICL_MMR_TIMINGS         0x4000
#define ICL_MMR_TIMING_CAS      0x4070
#define ICL_MMR_MAD_CHAN0       0x500C
#define ICL_MMR_MAD_CHAN1       0x5010
#define ICL_MMR_DRAM_CLOCK      0x5E00

#define ICL_MMR_MC_BIOS_REG     0x5E04
#define ICL_MMR_BLCK_REG        0x5F60

// MCHBAR_0_0_0_PCI 0x48 38:16 Base 64KB
//   CH0 0x4000 CH1 0x4400
// MAD_DIMM_CH0_0_0_0_MCHBAR 0x500C (32)
// MAD_DIMM_CH1_0_0_0_MCHBAR 0x5010 (32)
//   22:16 DIMM_S_SIZE: Size of DIMM S in 0.5GB multiples
//    6:0  DIMM_L_SIZE: Size of DIMM L in 0.5GB multiples
// MC_BIOS_DATA_0_0_0_MCHBAR_PCU 0x5E04 (32)
//   16    GEAR_TYPE:
//                    0: Gear1, DDR bus clock is the same as QCLK
//                    1: Gear2, DDR PHY bus clock is double of QCLK
//   11:8  MC_FREQ_TYPE: Reference Clock Type
//                      0: MC frequency request for 133MHz Qclk granularity.
//                      1: MC frequency request for 100MHz Qclk granularity.
//    7:0  MC_FREQ: Memory Controller Frequency
//                  Each bin is 133/100MHz and not 266/200MHz.
//                  This interface replaces the usage of DCLK ratios and Odd Ratio.
//                  QCLK frequency is determined by the MC reference clock(MC_FREQ_TYPE) as well as BCLK.
//                  0 : Memory Controller PLL shutdown
//                  1h - 2h : Reserved
//                  3h - FFh : QCLK ratio in 133.33MHz or 100MHz increments
// BCLK_FREQ_0_0_0_MCHBAR 0x5F60 (64)
//   31:0  BCLK_FREQ: Reported BCLK Frequency in KHz
// TC_PRE_0_0_0_MCHBAR 0x4000 (32)
//   15:9  tRAS
//    5:0  tRP tRCD
// TC_ODT_0_0_0_MCHBAR 0x4070 (64)
//   21:16 tCL

static void
get_intel_mchbar_10_11(void)
{
	uint64_t ofs_5f60 = get_mmio_64(ICL_MMR_BLCK_REG);
	uint32_t ofs_5e04 = get_mmio_32(ICL_MMR_MC_BIOS_REG);

	float bclk = (ofs_5f60 & 0xFFFFFFFF) / 1000.0f;
	ctx.freq = (uint16_t)((ofs_5e04 & 0xFF) * bclk);

	if (ofs_5e04 & 0x10000) // [16]
		ctx.freq *= 2;

	if ((ofs_5e04 & 0xF00) == 0) // [11:8]
		ctx.freq = (uint16_t)(133.34f / 100.0f * ctx.freq);

	ctx.ddr = 4;

	uint32_t ofs_500c = get_mmio_32(ICL_MMR_MAD_CHAN0);
	uint32_t ofs_5010 = get_mmio_32(ICL_MMR_MAD_CHAN1);
	uint32_t ch_size[2] = { 0 };
	ch_size[0] = ((ofs_500c >> 16) & 0x7F) + (ofs_500c & 0x7F);
	ch_size[1] = ((ofs_5010 >> 16) & 0x7F) + (ofs_5010 & 0x7F);

	for (uint64_t ch = 0; ch < 2; ch++)
	{
		if (ch_size[ch] == 0)
			continue;
		uint64_t ch_ofs = ch * ICL_MMR_CH1_OFFSET;
		uint64_t ofs_4070 = get_mmio_64(ch_ofs + ICL_MMR_TIMING_CAS);
		uint32_t ofs_4000 = get_mmio_32(ch_ofs + ICL_MMR_TIMINGS);
		struct dimm_slot* p = &ctx.slot[ch];
		snprintf(p->name, SLOT_NAME_SIZE, "CH%llu", ch);
		p->tCL = (ofs_4070 >> 16) & 0x1F;
		p->tRP = ofs_4000 & 0x3F;
		p->tRAS = (ofs_4000 >> 9) & 0x7F;
		p->tRCD = ofs_4000 & 0x3F;
		p->tRC = p->tRAS + p->tRP;
	}

	ctx.width = (ch_size[0] && ch_size[1]) ? 128 : 64;
}

#define ADL_MMR_MC1_OFFSET      0x10000
#define ADL_MMR_CH1_OFFSET      0x800

#define ADL_MMR_IC_DECODE       0xD800
#define ADL_MMR_CH0_DIMM_REG    0xD80C

#define ADL_MMR_MC0_REG         0xE000
#define ADL_MMR_ODT_TCL_REG     0xE070
#define ADL_MMR_MC_INIT_REG     0xE454

#define ADL_MMR_SA_PERF_REG     0x5918
#define ADL_MMR_MC_BIOS_REG     0x5E04
#define ADL_MMR_BLCK_REG        0x5F60

// MC0 CH0 0x0E000 CH1 0x0E800
// MC1 CH0 0x1E000 CH1 0x1E800
// SA_PERF_STATUS_0_0_0_MCHBAR_PCU 0x5918 (64)
//   10    QCLK_REFERENCE: DDR QCLK Reference
//                         0: 133.34Mhz. In frequency calculations use 400.0MHz/3.0
//                         1: 100.00Mhz
//    9:2  QCLK_RATIO: DDR QCLK Ratio
// MC_BIOS_DATA_0_0_0_MCHBAR_PCU 0x5E04 (32)
//   13:12 GEAR:
//              0: Gear1, DDR bus clock is the same as QCLK
//              1: Gear2, DDR PHY bus clock is double of QCLK
//              2: Gear4, DDR PHY bus clock is quad of QCL
//   11:8  MC_PLL_REF: Reference Clock Type
//                     0: MC frequency request for 133MHz Qclk granularity.
//                     1: MC frequency request for 100MHz Qclk granularity.
// BCLK_FREQ_0_0_0_MCHBAR 0x5F60 (64)
//   63:32 PCIECLK_FREQ: Reported PCIE BCLK Frequency in KHz
//   31:0  BCLK_FREQ: Reported BCLK Frequency in KHz
// MAD_INTER_CHANNEL_0_0_0_MCHBAR 0xD800 (32)
//    2:0  DDR_TYPE 0:DDR4 1:DDR5 2:LPDDR5 3:LPDDR4
// MAD_DIMM_CH0_0_0_0_MCHBAR 0xD80C (32)
//   22:16 DIMM_S_SIZE: Size of DIMM S in 0.5GB multiples
//    4    CH_L_MAP: Channel L mapping to physical channel, 0=Channel 0, 1=Channel 1
//    6:0  DIMM_L_SIZE: Size of DIMM L in 0.5GB multiples
// TC_PRE_0_0_0_MCHBAR 0xE000 (64)
//   58:51 tRCD
//   50:42 tRAS
//    7:0  tRP
// TC_ODT_0_0_0_MCHBAR 0xE070 (64)
//   22:16 tCL
static void
get_intel_mchbar_12_13_14(void)
{
	uint64_t ofs_5f60 = get_mmio_64(ADL_MMR_BLCK_REG);
	uint64_t ofs_5918 = get_mmio_64(ADL_MMR_SA_PERF_REG);
	uint64_t ofs_5e04 = get_mmio_32(ADL_MMR_MC_BIOS_REG);

	float bclk = (ofs_5f60 & 0xFFFFFFFF) / 1000.0f;
	ctx.freq = (uint16_t)(((ofs_5918 >> 2) & 0xFF) * bclk);
	ctx.freq <<= (ofs_5e04 >> 12) & 0x3;
	if ((ofs_5e04 & 0xF00) == 0)
		ctx.freq = (uint16_t)((133.34f / 100.0f) * ctx.freq);

	uint32_t ch_size[2] = { 0 };

	for (uint64_t mc = 0; mc < 2; mc++)
	{
		uint64_t mc_ofs = mc * ADL_MMR_MC1_OFFSET;
		uint32_t ofs_d80c = get_mmio_32(mc_ofs + ADL_MMR_CH0_DIMM_REG);
		uint32_t ofs_d800 = get_mmio_32(ADL_MMR_IC_DECODE);
		ch_size[mc] = ((ofs_d80c >> 16) & 0x7F) + (ofs_d80c & 0x7F);
		if (ch_size[mc] == 0)
			continue;

		if ((ofs_d800 & 0x07) == 1 || (ofs_d800 & 0x07) == 2)
			ctx.ddr = 5;

		for (uint64_t ch = 0; ch < 2; ch++)
		{
			uint64_t ch_ofs = mc_ofs + ch * ADL_MMR_CH1_OFFSET;
			uint64_t ofs_e070 = get_mmio_64(ch_ofs + ADL_MMR_ODT_TCL_REG);
			uint64_t ofs_e000 = get_mmio_64(ch_ofs + ADL_MMR_MC0_REG);
			struct dimm_slot* p = &ctx.slot[mc * 2 + ch];
			snprintf(p->name, SLOT_NAME_SIZE, "MC%lluCH%llu", mc, ch);
			p->tCL = (ofs_e070 >> 16) & 0x7F;
			p->tRP = ofs_e000 & 0xFF;
			p->tRAS = (ofs_e000 >> 42) & 0x1FF;
			p->tRCD = (ofs_e000 >> 51) & 0xFF;
			p->tRC = p->tRAS + p->tRP;
		}
	}

	if (!ctx.ddr)
		ctx.ddr = 4;
	ctx.width = (ch_size[0] && ch_size[1]) ? 64 : 128;
}

#define MTL_MMR_MC1_OFFSET      0x10000
#define MTL_MMR_CH1_OFFSET      0x800

#define MTL_MMR_IC_DECODE       0xD800
#define MTL_MMR_CH0_DIMM_REG    0xD80C

#define MTL_MMR_CH0_PRE_REG     0xE000
#define MTL_MMR_CH0_CAS_REG     0xE070
#define MTL_MMR_CH0_ACT_REG     0xE138

#define MTL_MMR_PTGRAM_REG      0x13D98

// MC0 CH0 0x0E000 CH1 0x0E800
// MC1 CH0 0x1E000 CH1 0x1E800
// MAD_INTER_CHANNEL_0_0_0_MCHBAR 0xD800 (32)
//   28:27 CH_WIDTH: Channel Width, 00b=x16, 01b=x32, 10b=x64
//   19:12 CH_S_SIZE: Channel S size in multiplies of 0.5GB.
//    4    CH_L_MAP: Channel L mapping to physical channel, 0=Channel 0, 1=Channel 1
//    2:0  DDR_TYPE 1:DDR5 2:LPDDR5 3:LPDDR4
// MAD_DIMM_CH0_0_0_0_MCHBAR 0xD80C (32)
//   22:16 DIMM_S_SIZE: Size of DIMM S in 0.5GB multiples
//    6:0  DIMM_L_SIZE: Size of DIMM L in 0.5GB multiples
// TC_PRE_0_0_0_MCHBAR 0xE000 (64)
//   53:45 tRAS
//   17:10 tRPab
//    7:0  tRPpb
// TC_CAS_0_0_0_MCHBAR 0xE070 (32)
//   22:16 tCL
// TC_ACT_0_0_0_MCHBAR 0xE138 (64)
//   29:22 tRCD
static void
get_intel_mchbar_15(void)
{
	uint32_t ofs_13d98 = get_mmio_32(MTL_MMR_PTGRAM_REG);

	switch ((ofs_13d98 >> 20) & 0x0F)
	{
	default:
	case 0x01:
		ctx.freq = 200;
		break;
	case 0x02:
		ctx.freq = 100;
		break;
	case 0x0A:
		ctx.freq = 133;
		break;
	case 0x0B:
		ctx.freq = 66;
		break;
	case 0x0C:
		ctx.freq = 33;
		break;
	}
	ctx.freq *= (ofs_13d98 >> 12) & 0xFF;
	ctx.freq *= (((ofs_13d98 >> 24) & 1) + 1) * 2;

	ctx.ddr = 5;

	uint32_t mc_width[2] = { 0 };

	for (uint64_t mc = 0; mc < 2; mc++)
	{
		uint64_t mc_ofs = mc * MTL_MMR_MC1_OFFSET;
		uint32_t ofs_d800 = get_mmio_32(mc_ofs + MTL_MMR_IC_DECODE);
		uint32_t ofs_d80c = get_mmio_32(mc_ofs + MTL_MMR_CH0_DIMM_REG);
		mc_width[mc] = ~ofs_d80c ? 1 << (((ofs_d800 >> 27) & 3) + 4) : 0;
		if (mc_width[mc] == 0)
			continue;
		for (uint64_t ch = 0; ch < 2; ch++)
		{
			uint64_t ch_ofs = mc_ofs + ch * MTL_MMR_CH1_OFFSET;
			uint32_t ofs_e070 = get_mmio_32(ch_ofs + MTL_MMR_CH0_CAS_REG);
			uint64_t ofs_e000 = get_mmio_64(ch_ofs + MTL_MMR_CH0_PRE_REG);
			uint64_t ofs_e138 = get_mmio_64(ch_ofs + MTL_MMR_CH0_ACT_REG);
			struct dimm_slot* p = &ctx.slot[mc * 2 + ch];
			snprintf(p->name, SLOT_NAME_SIZE, "MC%lluCH%llu", mc, ch);
			p->tCL = (ofs_e070 >> 16) & 0x7F;
			p->tRP = (ofs_e000 >> 10) & 0xFF;
			p->tRAS = (ofs_e000 >> 45) & 0x1FF;
			p->tRCD = (ofs_e138 >> 22) & 0xFF;
			p->tRC = p->tRAS + p->tRP;
		}
	}

	ctx.width = (mc_width[0] + mc_width[1]) * 2;
}

static uint32_t
get_intel_reg_32(void)
{
	uint32_t mmio_reg = 0;
	mmio_reg = WR0_RdPciConf32(NWLC->NwDrv, 0, MCHBAR_BASE_REG_LOW);
	if (!(mmio_reg & 0x01))
	{
		WR0_WrPciConf32(NWLC->NwDrv, 0, MCHBAR_BASE_REG_LOW, mmio_reg | 0x01);
		mmio_reg = WR0_RdPciConf32(NWLC->NwDrv, 0, MCHBAR_BASE_REG_LOW);
		if (!(mmio_reg & 0x1))
			return 0;
	}
	if (mmio_reg == 0xFFFFFFFF)
		return 0;
	mmio_reg &= 0xFFFFC000;
	return mmio_reg;
}

static uint64_t
get_intel_reg_64(uint64_t base_mask)
{
	uint64_t mmio_reg = 0;
	mmio_reg = WR0_RdPciConf32(NWLC->NwDrv, 0, MCHBAR_BASE_REG_LOW);
	if (!(mmio_reg & 0x01))
	{
		WR0_WrPciConf32(NWLC->NwDrv, 0, MCHBAR_BASE_REG_LOW, (uint32_t)mmio_reg | 0x01);
		mmio_reg = WR0_RdPciConf32(NWLC->NwDrv, 0, MCHBAR_BASE_REG_LOW);
		if (!(mmio_reg & 0x01))
			return 0;
	}
	if (mmio_reg == 0xFFFFFFFF)
		return 0;

	uint64_t mmio_reg_high = WR0_RdPciConf32(NWLC->NwDrv, 0, MCHBAR_BASE_REG_HIGH);
	if (mmio_reg_high == 0xFFFFFFFF)
		return 0;

	mmio_reg |= mmio_reg_high << 32;
	mmio_reg &= base_mask;
	return mmio_reg;
}

static void
detect_intel_mchbar(void)
{
	if (ctx.id->x86.family != 0x06)
		return;
	switch (ctx.id->x86.ext_model)
	{
	case 0x2A: // Core 2nd Gen (Sandy Bridge)
	case 0x3A: // Core 3rd Gen (Ivy Bridge)
		ctx.mmio_reg = get_intel_reg_32();
		ctx.get = get_intel_mchbar_2_3;
		break;
	case 0x3C: // Core 4th Gen (Haswell)
	case 0x45: // Core 4th Gen (Haswell)
	case 0x46: // Core 4th Gen (Haswell)
	case 0x47: // Core 5th Gen (Broadwell)
		ctx.mmio_reg = get_intel_reg_32();
		ctx.get = get_intel_mchbar_4_5;
		break;
	case 0x4E: // Core 6th Gen (Sky Lake)
	case 0x5E: // Core 6th Gen (Sky Lake)
	case 0x8E: // Core 7/8/9th Gen (Kaby/Coffee Lake)
	case 0x9E: // Core 7/8/9th Gen (Kaby/Coffee Lake)
		ctx.mmio_reg = get_intel_reg_64(0x7FFFFF8000); // 38:15
		ctx.get = get_intel_mchbar_6_7_8_9;
		break;
	case 0xA5: // Core 10th Gen (Comet Lake)
	case 0xA6: // Core 10th Gen (Comet Lake)
	case 0xA7: // Core 11th Gen (Rocket Lake)
		ctx.mmio_reg = get_intel_reg_64(0x7FFFFF0000); // 38:16
		ctx.get = get_intel_mchbar_10_11;
		break;
	case 0x97: // Core 12th Gen (Alder Lake)
	case 0x9A: // Core 12th Gen (Alder Lake)
	case 0xB7: // Core 13th/14th Gen (Raptor Lake)
	case 0xBA: // Core 13th/14th Gen (Raptor Lake)
	case 0xBE: // Core 13th/14th Gen (Raptor Lake)
	case 0xBF: // Core 13th/14th Gen (Raptor Lake)
		ctx.mmio_reg = get_intel_reg_64(0x3FFFFFE0000); // 41:17
		ctx.get = get_intel_mchbar_12_13_14;
		break;
	case 0xAA: // Ultra (Meteor Lake)
	case 0xAB: // Ultra (Meteor Lake)
	case 0xAC: // Ultra (Meteor Lake)
	case 0xB5: // Ultra (Arrow Lake)
	case 0xC5: // Ultra (Arrow Lake)
	case 0xC6: // Ultra (Arrow Lake)
		ctx.mmio_reg = get_intel_reg_64(0x3FFFFFE0000); // 41:17
		ctx.get = get_intel_mchbar_15;
		break;
	}
}

// https://github.com/memtest86plus/memtest86plus/blob/main/system/imc/x86/amd_zen.c
// https://github.com/cyring/CoreFreq/blob/master/x86_64/amd_reg.h

#define AMD_SMN_UMC_BAR             0x050000

static inline uint32_t
get_smn_umc_32(uint32_t offset)
{
	NWL_Debug("UMC", "Read %08X", AMD_SMN_UMC_BAR + offset);
	return WR0_RdAmdSmn(NWLC->NwDrv, WR0_SMN_AMD17H, AMD_SMN_UMC_BAR + offset);
}

#define AMD_SMN_UMC_CHA(_cha)       (_cha << 20) // 0x100000
#define AMD_SMN_UMC_NUM(_num)       (_num << 8)  // 0x100

#define AMD_SMN_UMC_DRAM_CONFIG     0x100
#define AMD_SMN_UMC_DRAM_MISC       0x200
#define AMD_SMN_UMC_DRAM_TIMINGS1   0x204
#define AMD_SMN_UMC_DRAM_TIMINGS2   0x208

// 0x{0,1,2,3,4,5,6,7}50100 AMD_ZEN_UMC_CONFIG
//  31    DramReady
//  12    ECC_Support
//  11:10 BurstCtrl
//   9:8  BurstLength
//   3:0  DdrType: 1=DDR5
// 0x{0,1,2,3,4,5,6,7}50{200,300,400,500} AMD_ZEN_UMC_CFG_MISC
// DDR4
//  11    GearDownMode
//  10:9  CMD_Rate: 0b10=2T, 0b00=1T
//   8    BankGroup: 1=Enable
//   5:0  MEMCLK: UMC=((Value * 100) / 3) MHz
// DDR5
//  18    GearDownMode
//  17:16 CMD_Rate
//  15:0  MEMCLK: DRAM=(Value * 2) MT/s
// 0x{0,1,2,3,4,5,6,7}50{204,304,404,504} AMD_17_UMC_TIMING_DTR1
//  29:24 tRCD_WR
//  21:16 tRCD_RD
//  14:8  tRAS
//   5:0  tCL
// 0x{0,1,2,3,4,5,6,7}50{208,308,405,508} AMD_17_UMC_TIMING_DTR2
//  29:24 tRPPB
//  21:16 tRP
//  15:8  tRCPB
//   7:0  tRC
// 0x{0,1,2,3,4,5,6,7}50{20c,30c,40c,50c} AMD_17_UMC_TIMING_DTR3
//  28:24 tRTP
//  20:16 tRRDDLR
//  12:8  tRRDL
//   4:0  tRRDS
// 0x{0,1,2,3,4,5,6,7}50{210,310,410,510} AMD_17_UMC_TIMING_DTR4
//  30:25 tFAWDLR
//  23:18 tFAWSLR
//   6:0  tFAW
// 0x{0,1,2,3,4,5,6,7}50{214,314,414,514} AMD_17_UMC_TIMING_DTR5
//  22:16 tWTRL
//  12:8  tWTRS
//   5:0  tCWL
// 0x{0,1,2,3,4,5,6,7}50{218,318,418,518} AMD_17_UMC_TIMING_DTR6
//   6:0  tWR

static void
get_amd_umc_zen(void)
{
	switch (ctx.id->x86.ext_family)
	{
	case 0x18: // Hygon Dhyana
		ctx.ddr = 4;
		break;
	case 0x1A: // Zen 5
		ctx.ddr = 5;
		break;
	}

	uint32_t memclk[8] = { 0 };
	for (uint32_t ch = 0; ch < 8; ch++)
	{
		uint32_t ch_ofs = AMD_SMN_UMC_CHA(ch);
		uint32_t ofs_100 = get_smn_umc_32(ch_ofs + AMD_SMN_UMC_DRAM_CONFIG);

		if (ofs_100 == 0 || ofs_100 == 0xFFFFFFFF)
			continue;

		NWL_Debug("UMC", "100: %08X %u %u %u %u %u", ofs_100,
			(ofs_100 >> 31) & 0x01, (ofs_100 >> 12) & 0x01, (ofs_100 >> 10) & 0x03, (ofs_100 >> 8) & 0x03, ofs_100 & 0x0F);
		if (!((ofs_100 >> 31) & 0x01)) // !DramReady
			continue;

		if (!ctx.ddr)
			ctx.ddr = (ofs_100 & 0x0F) ? 5 : 4;

		uint32_t ofs_200 = get_smn_umc_32(ch_ofs + AMD_SMN_UMC_DRAM_MISC);
		if (ofs_200 == 0 || ofs_200 == 0xFFFFFFFF)
			continue;

		if (ctx.ddr == 4)
		{
			memclk[ch] = ofs_200 & 0x3F;
			NWL_Debug("UMC", "200: D4 %08X %u %u %u", ofs_200,
				(ofs_200 >> 11) & 0x01, (ofs_200 >> 9) & 0x03, memclk[ch]);
			if (memclk[ch] == 0)
				continue;
			ctx.freq = (uint16_t)(memclk[ch] * 66.67f);
		}
		else
		{
			memclk[ch] = ofs_200 & 0xFFFF;
			NWL_Debug("UMC", "200: D5 %08X %u %u %u", ofs_200,
				(ofs_200 >> 18) & 0x01, (ofs_200 >> 16) & 0x03, memclk[ch]);
			if (memclk[ch] == 0)
				continue;
			ctx.freq = (uint16_t)(memclk[ch] << ((ofs_200 >> 18) & 0x01));
		}

		struct dimm_slot* p = &ctx.slot[ch];
		snprintf(p->name, SLOT_NAME_SIZE, "CH%u", ch);
		uint32_t ofs_204 = get_smn_umc_32(ch_ofs + AMD_SMN_UMC_DRAM_TIMINGS1);
		uint32_t ofs_208 = get_smn_umc_32(ch_ofs + AMD_SMN_UMC_DRAM_TIMINGS2);
		p->tCL = ofs_204 & 0x3F;
		p->tRAS = (ofs_204 >> 8) & 0x7F;
		p->tRCD = (ofs_204 >> 16) & 0x3F;
		p->tRP = (ofs_208 >> 16) & 0x3F;
		p->tRC = ofs_208 & 0xFF;
	}
	ctx.width = (memclk[0] && memclk[1]) ? 128 : 64;
}

static void
detect_amd_umc(void)
{
	switch (ctx.id->x86.ext_family)
	{
	case 0x17: // Zen / Zen + / Zen 2
	case 0x18: // Hygon Dhyana
	case 0x19: // Zen 3 / Zen 3+ / Zen 4
	case 0x1A: // Zen 5
		ctx.mmio_reg = AMD_SMN_UMC_BAR;
		ctx.get = get_amd_umc_zen;
		break;
	}
}

static bool imc_init(void)
{
	struct cpu_raw_data_array_t* raw = NWLC->NwCpuRaw;
	struct system_id_t* id = NWLC->NwCpuid;

	if (!NWLC->NwDrv)
		goto fail;

	if (raw->num_raw <= 0)
	{
		if (cpuid_get_all_raw_data(raw) != 0)
			goto fail;
	}

	if (id->num_cpu_types <= 0)
	{
		if (cpu_identify_all(raw, id) != 0)
			goto fail;
	}

	if (id->num_cpu_types < 1)
		goto fail;

	ctx.id = &id->cpu_types[0];
	NWL_Debug("IMC", "CPU %s F%02X EF%02X M%02X EM%02X",
		ctx.id->vendor_str, ctx.id->x86.family, ctx.id->x86.ext_family, ctx.id->x86.model, ctx.id->x86.ext_model);

	switch (ctx.id->vendor)
	{
	case VENDOR_INTEL:
		detect_intel_mchbar();
		break;
	case VENDOR_AMD:
	case VENDOR_HYGON:
		detect_amd_umc();
		break;
	}

	NWL_Debug("IMC", "MMIO REG %llX", ctx.mmio_reg);
	if (!ctx.mmio_reg || !ctx.get)
		goto fail;

	return true;
fail:
	ZeroMemory(&ctx, sizeof(ctx));
	return false;
}

static void imc_fini(void)
{
	ZeroMemory(&ctx, sizeof(ctx));
}

static void imc_get(PNODE node)
{
	ctx.get();
	NWL_NodeAttrSetf(node, "DDR", NAFLG_FMT_NUMERIC, "%u", ctx.ddr);
	NWL_NodeAttrSetf(node, "Frequency", NAFLG_FMT_NUMERIC, "%u", ctx.freq);
	NWL_NodeAttrSetf(node, "Width", NAFLG_FMT_NUMERIC, "%u", ctx.width);
	for (int i = 0; i < SLOT_COUNT; i++)
	{
		struct dimm_slot* p = &ctx.slot[i];
		if (p->name[0] == '\0')
			continue;
		PNODE n = NWL_NodeAppendNew(node, p->name, NFLG_ATTGROUP);
		NWL_NodeAttrSetf(n, "tCL", NAFLG_FMT_NUMERIC, "%u", p->tCL);
		NWL_NodeAttrSetf(n, "tRCD", NAFLG_FMT_NUMERIC, "%u", p->tRCD);
		NWL_NodeAttrSetf(n, "tRP", NAFLG_FMT_NUMERIC, "%u", p->tRP);
		NWL_NodeAttrSetf(n, "tRAS", NAFLG_FMT_NUMERIC, "%u", p->tRAS);
		NWL_NodeAttrSetf(n, "tRC", NAFLG_FMT_NUMERIC, "%u", p->tRC);
	}
}

sensor_t sensor_imc =
{
	.name = "IMC",
	.flag = NWL_SENSOR_IMC,
	.init = imc_init,
	.get = imc_get,
	.fini = imc_fini,
};
