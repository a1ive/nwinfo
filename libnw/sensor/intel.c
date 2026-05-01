// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"
#include "sensors.h"
#include "cpuid.h"
#include "ioctl.h"
#include "mchbar.h"
#include "cpu/rdmsr.h"

// 900 Series
//   NO DATA
// 800 Series
//   D31:F2 - PMC
//     DID 7F21
// 700 Series
//   D31:F2 - PMC
//     DID 7A21
// 600 Series
//   D31:F2 - PMC
//     DID 7AA1
// 500 Series
//   D31:F2 - PMC
//     DID 43A1
// 400 Series
//   D18:F0 - Thermal Subsystem (CLASS 1180)
//     DID 06F9
// 300 Series / C240 Series
//   D18:F0 - Thermal Subsystem (CLASS 1180)
//     DID 9DF9
//     DID A379
// 200 Series / X299 / Z370 Series
//   D20:F2 - Thermal Subsystem (CLASS 1180)
//     DID A2B1
// 100 Series / C230 Series
//   D20:F2 - Thermal Subsystem (CLASS 1180)
//     DID A131
// 9 Series
//   D31:F6 - Thermal Sensor (CLASS 1180)
//     DID 3A32
// 8 Series / C220 Series
//   D31:F6 - Thermal Sensor (CLASS 1180)
//     DID 3A32
// 7 Series / C216 Series
//   D31:F6 - Thermal Sensor (CLASS 1180)
//     DID 1C24
// 6 Series / C200 Series
//   D31:F6 - Thermal Sensor (CLASS 1180)
//     DID 1C24
// 5 Series / 3400 Series
//   D31:F6 - Thermal Sensor (CLASS 1180)
//     DID 3B32h

// PMC (D31:F2)
//   10h-13h PWRMBASE (BAR)
//     Base Address for MMIO Registers.
//     31:13 BAR (BASEADDR): Software programs this register with the base address of the device's memory region
//     12:4  Size Indicator (SIZEINDICATOR)
//      3    Prefetchable (PREFETCHABLE)
//      2:1  Type (TYPE)
//      0    Memory Space Indicator (MESSAGE_SPACE)
//   14h-17h PWRMBASE HIGH (BAR_HIGH)
//     Base Address High for MMIO Registers.
//     31:0  Base Address HIGH (BASEADDR_HIGH): Base Address
//   PWRMBASE+1560h Temperature Sensor Control and Status (TSS0)
//     31    Policy Lock-Down Bit (TSS0LOCK)
//     16    TS MASK for MAXTEMP calculation (TSMASKEN)
//      9    TS Reading Valid (TSRV): This bit indicates if the TS die temperature reported in valid or not.
//      8:0  TS Reading (TSR): The TS die temperature with resolution of 1oC in S9.8.0 2s
//                             complement format 0x001 positive 1oC 0x000 0oC 0x1FF negative 1oC 0x1D8
//                             negative 40oC and so on

// Thermal Subsystem (D18:F0) / (D20:F2)
// Thermal Sensor Registers (D31:F6)
//   10h-13h Thermal Base (TBAR)
//     31:12 Thermal Base Address (TBA): Base address for the Thermal logic memory mapped configuration registers.
//      3    Prefetchable (PREF)
//      2:1  Address Range (ADDRNG)
//      0    Space Type (SPTYP)
//   14h-17h Thermal Base High DWord(TBARH)
//     31:0  Thermal Base Address High (TBAH): TBAR bits 61:32.
//   TBAR+0h Temperature (TEMP)
//      8:0  TS Reading (TSR): The die temperature with resolution of 0.5
//                             degree C and an offset of - 50C.Thus a reading of 0x121 is 94.5C.

static struct
{
	struct cpu_id_t* id;
	ULONGLONG ticks;
	ULONGLONG delta_ticks;
	GROUP_AFFINITY affinity;
	HANDLE thread;
	int core_gen;
	uint32_t pp0_energy;
	uint32_t pp1_energy;
	uint32_t pkg_energy;
	uint32_t dram_energy;
	void (*get_mchbar_sensors)(PNODE node);
	void (*get_pch_sensors)(PNODE node);
} ctx;

#define PMC_MIN_TEMP 0x1500
#define PMC_MAX_TEMP 0x1504
#define PMC_TSAHV 0x1530
#define PMC_TSALV 0x1534
#define PMC_PHLC 0x1540
#define PMC_TSS0 0x1560

static inline float get_pmc_temperature(uint32_t reg)
{
	int t = reg & 0x1FF;
	if (t & 0x100)
		t -= 0x200;
	return (float)t;
}

static inline float get_ts_temperature(uint32_t reg)
{
	int t = reg & 0x1FF;
	return t / 2.0f - 50.0f;
}

static void get_mchbar_sensors(PNODE node)
{
	uint64_t mchbar_5f60 = mchbar_read_64(0x5F60);
	// 31:0  BCLK_FREQ kHz
	float bclk = (ctx.core_gen >= 10) ? (mchbar_5f60 & 0xFFFFFFFF) / 1000.0f : 100.0f;
	NWL_NodeAttrSetf(node, "BCLK MHz", NAFLG_FMT_NUMERIC, "%.3f", bclk);

	uint64_t mchbar_5918 = mchbar_read_64(0x5918);
	// 55:40 SA_VOLTAGE /8192.0V
	// 31:24 UCLK_RATIO *BCLK
	// 10    QCLK_REFERENCE 0 - 133.34MHz, 1 - 100MHz
	//  9:2  QCLK_RATIO *BCLK*QCLK_REFERENCE
	NWL_NodeAttrSetf(node, "SA Voltage", NAFLG_FMT_NUMERIC, "%.3f", ((mchbar_5918 >> 40) & 0xFFFF) / 8192.0);
	float uclk = ((mchbar_5918 >> 24) & 0xFF) * bclk;
	uint32_t qclk_ref_bit = (ctx.core_gen >= 10) ? (mchbar_5918 & (1 << 10)) : (mchbar_5918 & (1 << 7));
	float qclk_ref = (qclk_ref_bit) ? 1.0f : 4.0f / 3.0f;
	uint32_t qclk_ratio = (ctx.core_gen >= 10) ? ((mchbar_5918 >> 2) & 0xFF) : (mchbar_5918 & 0x7F);
	float qclk = qclk_ratio * bclk * qclk_ref;
	NWL_NodeAttrSetf(node, "UCLK MHz", NAFLG_FMT_NUMERIC, "%.3f", uclk);
	NWL_NodeAttrSetf(node, "QCLK MHz", NAFLG_FMT_NUMERIC, "%.3f", qclk);

#if 0
	uint32_t mchbar_5938 = mchbar_read_32(0x5938);
	// 12:8  ENERGY_UNIT
	//  3:0  PWR_UNIT
	uint32_t energy_unit = (mchbar_5938 >> 8) & 0x1F;
	uint32_t power_unit = mchbar_5938 & 0xF;
#endif

#if 0
	uint32_t mchbar_5928 = mchbar_read_32(0x5928);
	// 31:0  PP0_ENERGY /Power(2, ENERGY_UNIT)
	float pp0_delta_energy = 0.0f;
	if (mchbar_5928 > ctx.pp0_energy)
		pp0_delta_energy = (float)(mchbar_5928 - ctx.pp0_energy) / (1ULL << energy_unit);
	ctx.pp0_energy = mchbar_5928;
	NWL_NodeAttrSetf(node, "PP0 Power", NAFLG_FMT_NUMERIC, "%.3f", pp0_delta_energy * 1000.0f / ctx.delta_ticks);

	uint32_t mchbar_592c = mchbar_read_32(0x592C);
	// 31:0  PP1_ENERGY /Power(2, ENERGY_UNIT)
	float pp1_delta_energy = 0.0f;
	if (mchbar_592c > ctx.pp1_energy)
		pp1_delta_energy = (float)(mchbar_592c - ctx.pp1_energy) / (1ULL << energy_unit);
	ctx.pp1_energy = mchbar_592c;
	NWL_NodeAttrSetf(node, "PP1 Power", NAFLG_FMT_NUMERIC, "%.3f", pp1_delta_energy * 1000.0f / ctx.delta_ticks);

	uint32_t mchbar_593c = mchbar_read_32(0x593C);
	// 31:0  PKG_ENERGY /Power(2, ENERGY_UNIT)
	float pkg_delta_energy = 0.0f;
	if (mchbar_593c > ctx.pkg_energy)
		pkg_delta_energy = (float)(mchbar_593c - ctx.pkg_energy) / (1ULL << energy_unit);
	ctx.pkg_energy = mchbar_593c;
	NWL_NodeAttrSetf(node, "Package Power", NAFLG_FMT_NUMERIC, "%.3f", pkg_delta_energy * 1000.0f / ctx.delta_ticks);
#endif

#if 0
	uint32_t mchbar_594c = mchbar_read_32(0x594C);
	// 7:0 EDRAM_TEMPERATURE
	int edram_temp = mchbar_594c & 0xFF;
	if (edram_temp > 0)
		NWL_NodeAttrSetf(node, "eDRAM Temperature", NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature((float)edram_temp));
#endif

#if 0
	uint32_t mchbar_5978 = mchbar_read_32(0x5978);
	//  7:0 PKG_TEMPERATURE
	int pkg_temp = mchbar_5978 & 0xFF;
	if (pkg_temp > 0)
		NWL_NodeAttrSetf(node, "Package Temperature", NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature((float)pkg_temp));
#endif

	uint32_t mchbar_597c = mchbar_read_32(0x597C);
	//  7:0 PP0_TEMPERATURE
	int pp0_temp = mchbar_597c & 0xFF;
	if (pp0_temp > 0)
		NWL_NodeAttrSetf(node, "PP0 Temperature", NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature((float)pp0_temp));

	uint32_t mchbar_5980 = mchbar_read_32(0x5980);
	//  7:0 PP1_TEMPERATURE
	int pp1_temp = mchbar_5980 & 0xFF;
	if (pp1_temp > 0)
		NWL_NodeAttrSetf(node, "PP1 Temperature", NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature((float)pp1_temp));

	uint32_t mchbar_599c = mchbar_read_32(0x599C);
	// 30:24 TJ_MAX_TCC_OFFSET
	// 23:16 TJMAX
	// 15:8  FAN_TEMP_TARGET_OFST
	int tjmax = (mchbar_599c >> 16) & 0xFF;
	NWL_NodeAttrSetf(node, "Tj Max", NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature((float)tjmax));
	int tcc_offset = (mchbar_599c >> 24) & 0x7F;
	NWL_NodeAttrSetf(node, "Tcc", NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature((float)(tjmax - tcc_offset)));
	int tctrl_offset = (mchbar_599c >> 8) & 0xFF;
	NWL_NodeAttrSetf(node, "T-Control", NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature((float)(tjmax - tctrl_offset)));

#if 0
	uint64_t mchbar_59a0 = mchbar_read_64(0x59A0);
	// 46:32 PKG_PWR_LIM_2 /Power(2, POWER_UNIT)
	// 14:0  PKG_PWR_LIM_1 /Power(2, POWER_UNIT)
	float pkg_pl1 = (float)(mchbar_59a0 & 0x7FFF) / (1ULL << power_unit);
	if (pkg_pl1 > 0.0f)
		NWL_NodeAttrSetf(node, "Package PL1", NAFLG_FMT_NUMERIC, "%.2f", pkg_pl1);
	float pkg_pl2 = (float)((mchbar_59a0 >> 32) & 0x7FFF) / (1ULL << power_unit);
	if (pkg_pl2 > 0.0f)
		NWL_NodeAttrSetf(node, "Package PL2", NAFLG_FMT_NUMERIC, "%.2f", pkg_pl2);
#endif

	uint32_t mchbar_59c0 = mchbar_read_32(0x59C0);
	// 31    VALID
	// 30:27 RESOLUTION
	// 23:16 TEMPERATURE_OFFSET
	if (mchbar_59c0 & (1 << 31))
	{
		int gt_offset = (mchbar_59c0 >> 16) & 0xFF;
		NWL_NodeAttrSetf(node, "GT Temperature", NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature((float)(tjmax - gt_offset)));
	}

	if (ctx.core_gen >= 12)
	{
		uint32_t mchbar_5e04 = mchbar_read_32(0x5E04);
		// 30:27 VDDQ_TX_ICCMAX *0.25A
		// 26:17 VDDQ_TX_VOLTAGE *5mV
		// 13:12 GEAR
		// 11:8  MC_PLL_REF
		//  7:0  MC_PLL_RATIO
		float iccmax = ((mchbar_5e04 >> 27) & 0x0F) * 0.25f;
		NWL_NodeAttrSetf(node, "Request VDDQ TX IccMax", NAFLG_FMT_NUMERIC, "%.2f", iccmax);
		float txvolt = ((mchbar_5e04 >> 17) & 0x3FF) * 0.005f;
		NWL_NodeAttrSetf(node, "Request VDDQ TX Voltage", NAFLG_FMT_NUMERIC, "%.3f", txvolt);
	}

#if 0
	uint32_t mchbar_5f3c = mchbar_read_32(0x5F3C);
	//  7:0  TDP_RATIO *100MHz
	NWL_NodeAttrSetf(node, "cTDP Nominal Ratio", NAFLG_FMT_NUMERIC, "%u", mchbar_5f3c & 0xFF);

	uint64_t mchbar_5f40 = mchbar_read_64(0x5F40);
	// 23:16 TDP_RATIO
	// 14:0  PKG_TDP /Power(2, POWER_UNIT)
	if (mchbar_5f40 != 0ULL)
	{
		NWL_NodeAttrSetf(node, "cTDP Level 1 Ratio", NAFLG_FMT_NUMERIC, "%llu", (mchbar_5f40 >> 16) & 0xFF);
		float ctdp_level_1_power = (float)(mchbar_5f40 & 0x7FFF) / (1ULL << power_unit);
		NWL_NodeAttrSetf(node, "cTDP Level 1 Power Limit", NAFLG_FMT_NUMERIC, "%.3f", ctdp_level_1_power);
	}

	uint64_t mchbar_5f48 = mchbar_read_64(0x5F48);
	// 23:16 TDP_RATIO
	// 14:0  PKG_TDP /Power(2, POWER_UNIT)
	if (mchbar_5f48 != 0ULL)
	{
		NWL_NodeAttrSetf(node, "cTDP Level 2 Ratio", NAFLG_FMT_NUMERIC, "%llu", (mchbar_5f48 >> 16) & 0xFF);
		float ctdp_level_2_power = (float)(mchbar_5f48 & 0x7FFF) / (1ULL << power_unit);
		NWL_NodeAttrSetf(node, "cTDP Level 2 Power Limit", NAFLG_FMT_NUMERIC, "%.3f", ctdp_level_2_power);
	}

	uint32_t mchbar_5f50 = mchbar_read_32(0x5F50);
	//  1:0  TDP_LEVEL 0 - Nominal, 1 - Level 1, 2 - Level 2
	NWL_NodeAttrSetf(node, "cTDP Current Level", NAFLG_FMT_NUMERIC, "%u", mchbar_5f50 & 0x3);
#endif

#if 0
	uint32_t mchbar_6200 = mchbar_read_32(0x6200);
	// 31    VALID
	// 22:16 PKG_THERMAL_DPPM_TEMP
	if (mchbar_6200 & (1 << 31))
	{
		int dppm_temp = (mchbar_6200 >> 16) & 0x7F;
		NWL_NodeAttrSetf(node, "Package DPPM Temperature", NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature((float)(tjmax - dppm_temp)));
	}
#endif
}

static void get_pch_sensors_pmc(PNODE node)
{
	uint32_t tss0 = pch_read_32(PMC_TSS0);
	if ((tss0 & (1 << 9))) // TS Reading Valid
		NWL_NodeAttrSetf(node, "PCH Temperature", NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature(get_pmc_temperature(tss0)));
}

static void get_pch_sensors_ts(PNODE node)
{
	uint32_t temp = pch_read_32(0);
	NWL_NodeAttrSetf(node, "PCH Temperature", NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature(get_ts_temperature(temp)));
}

static inline int send_oc_mailbox(const OC_MAILBOX_FULL* in, OC_MAILBOX_FULL* out)
{
	int err;
	GROUP_AFFINITY saved_aff;
	SetThreadGroupAffinity(ctx.thread, &ctx.affinity, &saved_aff);
	err = WR0_SendOcMailbox(NWLC->NwDrv, in, out);
	SetThreadGroupAffinity(ctx.thread, &saved_aff, NULL);
	return err;
}

static inline int read_msr(uint32_t msr, uint64_t* value)
{
	int err;
	ULONG64 in = msr;
	ULONG64 out = 0;
	if (NWLC->NwDrv->type == WR0_DRIVER_PAWNIO)
		err = WR0_ExecPawn(NWLC->NwDrv, &NWLC->NwDrv->pio_intel, "ioctl_read_msr", &in, 1, &out, 1, NULL);
	else
		err = WR0_RdMsr(NWLC->NwDrv, msr, &out);
	//NWL_Debug("MSR", "%X -> %llX (%d)", msr, out, err);
	*value = out;
	return err;
}

static void get_msr_sensors(PNODE node)
{
	uint64_t value;
	GROUP_AFFINITY saved_aff;
	SetThreadGroupAffinity(ctx.thread, &ctx.affinity, &saved_aff);

	int tjmax = 100;
	if (read_msr(0x1a2, &value) == 0)
	{
		tjmax = (value >> 16) & 0xFF;
		NWL_NodeAttrSetf(node, "Tj Max", NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature((float)tjmax));
		int tcc_offset = (value >> 24) & 0x7F;
		NWL_NodeAttrSetf(node, "Tcc", NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature((float)(tjmax - tcc_offset)));
		int tctrl_offset = (value >> 8) & 0xFF;
		NWL_NodeAttrSetf(node, "T-Control", NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature((float)(tjmax - tctrl_offset)));
	}

	if (read_msr(0x1b1, &value) == 0)
	{
		int delta = (value >> 16) & 0x7F;
		NWL_NodeAttrSetf(node, "Package Temperature", NAFLG_FMT_NUMERIC, "%.0f", NWL_GetTemperature((float)(tjmax - delta)));
	}

	uint32_t energy_unit = 0x10; // 10000b
	uint32_t power_unit = 0x03; // 0011b
	uint32_t time_unit = 0x0A; // 1010b

	if (read_msr(0x606, &value) == 0)
	{
		energy_unit = (value >> 8) & 0x1F;
		power_unit = value & 0xF;
		time_unit = (value >> 16) & 0xF;
	}

	// Package RAPL
	// MSR_PKG_POWER_LIMIT 0x610
	// MSR_PKG_ENERGY_STATUS 0x611
	// RESERVED 0x612
	// MSR_PKG_PERF_STATUS 0x613
	// MSR_PKG_POWER_INFO 0x614
	if (read_msr(0x610, &value) == 0)
	{
		if (value & (1ULL << 15))
		{
			float pl1 = (float)(value & 0x7FFF) / (1ULL << power_unit);
			NWL_NodeAttrSetf(node, "Package PL1", NAFLG_FMT_NUMERIC, "%.0f", pl1);
			float pl1_time = (float)(1ULL << ((value >> 17) & 0x1F)) * (1.0f + ((value >> 22) & 0x3) / 4.0f) / (1ULL << time_unit);
			NWL_NodeAttrSetf(node, "Package PL1 Time", NAFLG_FMT_NUMERIC, "%.3f", pl1_time);
		}
		if (value & (1ULL << 47))
		{
			float pl2 = (float)((value >> 32) & 0x7FFF) / (1ULL << power_unit);
			NWL_NodeAttrSetf(node, "Package PL2", NAFLG_FMT_NUMERIC, "%.0f", pl2);
			float pl2_time = (float)(1ULL << ((value >> 49) & 0x1F)) * (1.0f + ((value >> 54) & 0x3) / 4.0f) / (1ULL << time_unit);
			NWL_NodeAttrSetf(node, "Package PL2 Time", NAFLG_FMT_NUMERIC, "%.3f", pl2_time);
		}
	}
	if (read_msr(0x611, &value) == 0)
	{
		float delta_energy = 0.0f;
		value &= 0xFFFFFFFF;
		if (value > ctx.pkg_energy)
			delta_energy = (float)(value - ctx.pkg_energy) / (1ULL << energy_unit);
		ctx.pkg_energy = (uint32_t)value;
		NWL_NodeAttrSetf(node, "Package Power", NAFLG_FMT_NUMERIC, "%.3f", delta_energy * 1000.0f / ctx.delta_ticks);
	}

	// PP0 RAPL
#if 0
	if (read_msr(0x638, &value) == 0)
	{
		if (value & (1ULL << 15))
		{
			float pl1 = (float)(value & 0x7FFF) / (1ULL << power_unit);
			NWL_NodeAttrSetf(node, "PP0 PL1", NAFLG_FMT_NUMERIC, "%.0f", pl1);
			float pl1_time = (float)(1ULL << ((value >> 17) & 0x1F)) * (1.0f + ((value >> 22) & 0x3) / 4.0f) / (1ULL << time_unit);
			NWL_NodeAttrSetf(node, "PP0 PL1 Time", NAFLG_FMT_NUMERIC, "%.3f", pl1_time);
		}
	}
#endif
	if (read_msr(0x639, &value) == 0)
	{
		float delta_energy = 0.0f;
		value &= 0xFFFFFFFF;
		if (value > ctx.pp0_energy)
			delta_energy = (float)(value - ctx.pp0_energy) / (1ULL << energy_unit);
		ctx.pp0_energy = (uint32_t)value;
		NWL_NodeAttrSetf(node, "PP0 Power", NAFLG_FMT_NUMERIC, "%.3f", delta_energy * 1000.0f / ctx.delta_ticks);
	}

	// PP1 RAPL
#if 0
	if (read_msr(0x640, &value) == 0)
	{
		if (value & (1ULL << 15))
		{
			float pl1 = (float)(value & 0x7FFF) / (1ULL << power_unit);
			NWL_NodeAttrSetf(node, "PP1 PL1", NAFLG_FMT_NUMERIC, "%.0f", pl1);
			float pl1_time = (float)(1ULL << ((value >> 17) & 0x1F)) * (1.0f + ((value >> 22) & 0x3) / 4.0f) / (1ULL << time_unit);
			NWL_NodeAttrSetf(node, "PP1 PL1 Time", NAFLG_FMT_NUMERIC, "%.3f", pl1_time);
		}
	}
#endif
	if (read_msr(0x641, &value) == 0)
	{
		float delta_energy = 0.0f;
		value &= 0xFFFFFFFF;
		if (value > ctx.pp1_energy)
			delta_energy = (float)(value - ctx.pp1_energy) / (1ULL << energy_unit);
		ctx.pp1_energy = (uint32_t)value;
		NWL_NodeAttrSetf(node, "PP1 Power", NAFLG_FMT_NUMERIC, "%.3f", delta_energy * 1000.0f / ctx.delta_ticks);
	}

	// DRAM RAPL
#if 0
	if (read_msr(0x618, &value) == 0)
	{
		if (value & (1ULL << 15))
		{
			float pl1 = (float)(value & 0x7FFF) / (1ULL << power_unit);
			NWL_NodeAttrSetf(node, "DRAM PL1", NAFLG_FMT_NUMERIC, "%.0f", pl1);
			float pl1_time = (float)(1ULL << ((value >> 17) & 0x1F)) * (1.0f + ((value >> 22) & 0x3) / 4.0f) / (1ULL << time_unit);
			NWL_NodeAttrSetf(node, "DRAM PL1 Time", NAFLG_FMT_NUMERIC, "%.3f", pl1_time);
		}
	}
#endif
	if (read_msr(0x619, &value) == 0)
	{
		float delta_energy = 0.0f;
		value &= 0xFFFFFFFF;
		if (value > ctx.dram_energy)
			delta_energy = (float)(value - ctx.dram_energy) / (1ULL << energy_unit);
		ctx.dram_energy = (uint32_t)value;
		NWL_NodeAttrSetf(node, "DRAM Power", NAFLG_FMT_NUMERIC, "%.3f", delta_energy * 1000.0f / ctx.delta_ticks);
	}

	if (read_msr(0x620, &value) == 0)
	{
		// 14:8 Uncore Min Ratio
		//  6:0 Uncore Max Ratio
		uint32_t min_ratio = (value >> 8) & 0x7F;
		uint32_t max_ratio = value & 0x7F;
		NWL_NodeAttrSetf(node, "Uncore Min Ratio", NAFLG_FMT_NUMERIC, "%u", min_ratio);
		NWL_NodeAttrSetf(node, "Uncore Max Ratio", NAFLG_FMT_NUMERIC, "%u", max_ratio);
	}
	if (read_msr(0x621, &value) == 0)
	{
		//  6:0 Uncore Ratio
		uint32_t ratio = value & 0x7F;
		NWL_NodeAttrSetf(node, "Uncore Frequency MHz", NAFLG_FMT_NUMERIC, "%u", ratio * 100);
	}

	SetThreadGroupAffinity(ctx.thread, &saved_aff, NULL);
}

static bool intel_init(void)
{
	struct system_id_t* id = NWL_GetCpuid();

	if (!NWLC->NwDrv)
		goto fail;

	if (!id)
		goto fail;
	ctx.id = &id->cpu_types[0];
	if (ctx.id->vendor != VENDOR_INTEL)
		goto fail;

	ctx.thread = GetCurrentThread();
	NWL_GetGroupAffinity(&ctx.id->affinity_mask, &ctx.affinity);

	if (!mchbar_pch_init())
		goto ok;

	switch (ctx.id->x86.ext_model)
	{
	case 0x3C: // Core 4th Gen (Haswell)
	case 0x3F: // Core 4th Gen (Haswell-EP)
	case 0x45: // Core 4th Gen (Haswell)
	case 0x46: // Core 4th Gen (Haswell)
	case 0x3D: // Core 5th Gen (Broadwell) ?
	case 0x47: // Core 5th Gen (Broadwell)
	case 0x4F: // Core 5th Gen (Broadwell) ?
	case 0x56: // Core 5th Gen (Broadwell) ?
	case 0x4E: // Core 6th Gen (Sky Lake)
	case 0x55: // Core 6th Gen (Sky Lake) ?
	case 0x5E: // Core 6th Gen (Sky Lake)
	case 0x8E: // Core 7/8/9th Gen (Kaby/Coffee Lake)
	case 0x9E: // Core 7/8/9th Gen (Kaby/Coffee Lake)
		ctx.core_gen = 4;
		if (pch_get_mmio_reg() != 0)
			ctx.get_pch_sensors = get_pch_sensors_ts;
		break;
	case 0x6A: // Core 10th Gen (Ice Lake) ?
	case 0x6C: // Core 10th Gen (Ice Lake) ?
	case 0x7D: // Core 10th Gen (Ice Lake) ?
	case 0x7E: // Core 10th Gen (Ice Lake) ?
	case 0xA5: // Core 10th Gen (Comet Lake)
	case 0xA6: // Core 10th Gen (Comet Lake)
	case 0xA7: // Core 11th Gen (Rocket Lake)
	case 0x8A: // Core 11th Gen (Lakefield) ?
	case 0x9C: // Core 11th Gen (Jasper Lake) ?
		ctx.core_gen = 10;
		if (pch_get_mmio_reg() != 0)
			ctx.get_pch_sensors = get_pch_sensors_ts;
		break;
	case 0x8C: // Core 11th Gen (Tiger Lake-U) // ???
	case 0x8D: // Core 11th Gen (Tiger Lake-H)
	case 0x97: // Core 12th Gen (Alder Lake)
	case 0x9A: // Core 12th Gen (Alder Lake)
	case 0xB7: // Core 13th/14th Gen (Raptor Lake)
	case 0xBA: // Core 13th/14th Gen (Raptor Lake)
	case 0xBE: // Core 13th/14th Gen (Raptor Lake)
	case 0xBF: // Core 13th/14th Gen (Raptor Lake)
	case 0xAA: // Ultra (Meteor Lake)
	case 0xAB: // Ultra (Meteor Lake)
	case 0xAC: // Ultra (Meteor Lake)
	case 0xB5: // Ultra (Arrow Lake-U)
	case 0xBD: // Ultra (Lunar Lake-V)
	case 0xC5: // Ultra (Arrow Lake-H)
	case 0xC6: // Ultra (Arrow Lake-S)
	case 0xCC: // Ultra (Panther Lake)
		ctx.core_gen = 12;
		if (pch_get_mmio_reg() != 0)
			ctx.get_pch_sensors = get_pch_sensors_pmc;
		break;
	}
	if (mchbar_get_mmio_reg() != 0)
		ctx.get_mchbar_sensors = get_mchbar_sensors;

ok:
	return true;
fail:
	ZeroMemory(&ctx, sizeof(ctx));
	return false;
}

static void intel_fini(void)
{
	ZeroMemory(&ctx, sizeof(ctx));
}

static void intel_get(PNODE node)
{
	ULONGLONG ticks = GetTickCount64();
	if (ticks > ctx.ticks)
		ctx.delta_ticks = ticks - ctx.ticks;
	else
		ctx.delta_ticks = 1;
	if (ctx.get_mchbar_sensors)
		ctx.get_mchbar_sensors(node);
	if (ctx.get_pch_sensors)
		ctx.get_pch_sensors(node);
	get_msr_sensors(node);
	ctx.ticks = GetTickCount64();
}

sensor_t sensor_intel =
{
	.name = "INTEL",
	.flag = NWL_SENSOR_INTEL,
	.init = intel_init,
	.get = intel_get,
	.fini = intel_fini,
};
