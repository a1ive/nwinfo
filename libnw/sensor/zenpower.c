// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"
#include "sensors.h"
#include "libcpuid.h"
#include "ioctl.h"
#include "ryzen_smu.h"

// https://github.com/thor2002ro/zenpower
// https://github.com/mattkeenan/zenpower5
// https://github.com/irusanov/ZenStates-Core
// https://github.com/FlyGoat/RyzenAdj

#define F17H_M01H_SVI                       0x0005A000
#define F17H_M02H_SVI                       0x0006F000
#define F1AH_M70H_SVI                       0x00073000

#define F17H_M01H_SVI_TEL_PLANE0            (F17H_M01H_SVI + 0x0C)
#define F17H_M01H_SVI_TEL_PLANE1            (F17H_M01H_SVI + 0x10)

#define F17H_M30H_SVI_TEL_PLANE0            (F17H_M01H_SVI + 0x14)
#define F17H_M30H_SVI_TEL_PLANE1            (F17H_M01H_SVI + 0x10)

#define F17H_M60H_SVI_TEL_PLANE0            (F17H_M02H_SVI + 0x38)
#define F17H_M60H_SVI_TEL_PLANE1            (F17H_M02H_SVI + 0x3C)

#define F17H_M70H_SVI_TEL_PLANE0            (F17H_M01H_SVI + 0x10)
#define F17H_M70H_SVI_TEL_PLANE1            (F17H_M01H_SVI + 0x0C)

#define F19H_M01H_SVI_TEL_PLANE0            (F17H_M01H_SVI + 0x14)
#define F19H_M01H_SVI_TEL_PLANE1            (F17H_M01H_SVI + 0x10)

#define F19H_M21H_SVI_TEL_PLANE0            (F17H_M01H_SVI + 0x10)
#define F19H_M21H_SVI_TEL_PLANE1            (F17H_M01H_SVI + 0x0C)

#define F19H_M50H_SVI_TEL_PLANE0            (F17H_M02H_SVI + 0x38)
#define F19H_M50H_SVI_TEL_PLANE1            (F17H_M02H_SVI + 0x3C)

#define F19H_M61H_SVI_TEL_PLANE0            (F17H_M01H_SVI + 0x10)
#define F19H_M61H_SVI_TEL_PLANE1            (F17H_M01H_SVI + 0x0C)

#define F1AH_M20H_SVI_TEL_PLANE0            (F17H_M01H_SVI + 0x0C)
#define F1AH_M20H_SVI_TEL_PLANE1            (F17H_M01H_SVI + 0x10)

#define F1AH_M70H_SVI_TEL_PLANE0            (F1AH_M70H_SVI + 0x10)
#define F1AH_M70H_SVI_TEL_PLANE1            (F1AH_M70H_SVI + 0x14)

#define ZEN_CCD_TEMP_VALID                  (1 << 11)
#define ZEN_CCD_TEMP_MASK                   0x7FF

#define F17H_M70H_CCD_TEMP_BASE             0x00059954
#define F19H_M11H_CCD_TEMP_BASE             0x00059B00
#define F1AH_M70H_CCD_TEMP_BASE             0x00059B08

#define MAX_CCD_COUNT                       8

#define FUSE1_BASE                          0x5D218
#define FUSE2_BASE                          0x5D21C

#define F17H_M01H_THM_TCON_CUR_TMP          0x00059800
#define F17H_TEMP_OFFSET_FLAG               0x80000

static struct
{
	struct cpu_id_t* id;
	ry_handle_t* smu;
	ry_codename_t codename;
	uint32_t svi_core_addr;
	uint32_t svi_soc_addr;
	uint32_t ccd_temp_base;
	uint32_t num_cores;
	uint8_t num_ccds;
	uint8_t zen_gen;
	double temp_offset;
	float stapm_limit;
	float stapm_value;
	float fast_limit;
	float fast_value;
	float slow_limit;
	float slow_value;
	float apu_slow_limit;
	float apu_slow_value;
	float vrm_current;
	float vrm_current_value;
	float vrmsoc_current;
	float vrmsoc_current_value;
	float gfx_temperature;
	float gfx_volt;
	float gfx_clk;
	float psi0_current;
	float psi0soc_current;
	float fclk;
	float uclk;
	float mclk;
	float soc_volt;
	float cldo_vddp;
	float l3_clk;
	float l3_vddm;
	float l3_temperature;
	float socket_power;
} ctx;

static inline uint32_t count_set_bits(uint32_t v)
{
	uint32_t result = 0;
	while (v > 0)
	{
		if (v & 1)
			result++;
		v >>= 1;
	}
	return result;
}

static void detect_num_ccds(void)
{
	uint32_t fuse1 = FUSE1_BASE;
	uint32_t fuse2 = FUSE2_BASE;
	uint32_t ccds_present = 0, ccds_down = 0;
	uint8_t ext_family = ctx.id->x86.ext_family;
	uint8_t ext_model = ctx.id->x86.ext_model;

	//if (ctx.num_ccds > 0)
		//return;

	switch (ctx.id->x86.ext_family)
	{
	case 0x17:
		switch (ctx.codename)
		{
		case CODENAME_CASTLEPEAK:
		case CODENAME_ROME:
		case CODENAME_MATISSE:
			break;
		default:
			fuse1 += 0x40;
			fuse2 += 0x40;
			break;
		}
		break;
	case 0x19:
		switch (ctx.codename)
		{
		case CODENAME_RAPHAEL:
		case CODENAME_DRAGONRANGE:
			fuse1 += 0x1A4;
			fuse2 += 0x1A4;
			break;
		default:
			break;
		}
		break;
	case 0x1A:
		fuse1 += 0x1A4;
		fuse2 += 0x1A4;
		break;
	}

	WR0_WaitPciBus(500);
	ccds_present = WR0_RdAmdSmn(NWLC->NwDrv, WR0_SMN_AMD17H, fuse1);
	ccds_down = WR0_RdAmdSmn(NWLC->NwDrv, WR0_SMN_AMD17H, fuse2);
	WR0_ReleasePciBus();
	NWL_Debug("SMN", "fuse1=%08Xh fuse2=%08Xh ccds_present=%08Xh ccds_down=%08Xh", fuse1, fuse2, ccds_present, ccds_down);

	if (ccds_present != 0 || ccds_down != 0)
	{
		uint32_t ccd_enable_map = (ccds_present >> 22) & 0x3;
		uint32_t enabled_ccds = count_set_bits(ccd_enable_map);
		ctx.num_ccds = (uint8_t)(enabled_ccds > 0 ? enabled_ccds : 1);
		NWL_Debug("SMN", "Detected %u enabled CCD(s), map=%Xh", ctx.num_ccds, ccd_enable_map);
	}
	else
	{
		if (ctx.num_ccds == 0)
			ctx.num_ccds = 1;
		NWL_Debug("SMN", "Could not detect number of CCDs, defaulting to %u", ctx.num_ccds);
	}

	if (ctx.num_ccds > MAX_CCD_COUNT)
		ctx.num_ccds = MAX_CCD_COUNT;
}

static bool smn_init(void)
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
	NWL_Debug("SMN", "CPU %s F%02X EF%02X M%02X EM%02X",
		ctx.id->vendor_str, ctx.id->x86.family, ctx.id->x86.ext_family, ctx.id->x86.model, ctx.id->x86.ext_model);

	if (ctx.id->vendor != VENDOR_AMD)
		goto fail;
	
	ctx.codename = ryzen_smu_get_codename(ctx.id);
	if (ctx.codename == CODENAME_UNKNOWN)
		goto fail;

	switch (ctx.id->x86.ext_family)
	{
	case 0x17:
		ctx.ccd_temp_base = F17H_M70H_CCD_TEMP_BASE;
		ctx.svi_core_addr = F17H_M01H_SVI_TEL_PLANE0;
		ctx.svi_soc_addr = F17H_M01H_SVI_TEL_PLANE1;
		if (ctx.id->x86.ext_model >= 0x31)
			ctx.zen_gen = 2;
		else
			ctx.zen_gen = 1;
		break;
	case 0x19:
		ctx.ccd_temp_base = F17H_M70H_CCD_TEMP_BASE;
		ctx.svi_core_addr = F19H_M21H_SVI_TEL_PLANE0;
		ctx.svi_soc_addr = F19H_M21H_SVI_TEL_PLANE1;
		if (ctx.id->x86.ext_model >= 0x61)
			ctx.zen_gen = 4;
		else
			ctx.zen_gen = 3;
		break;
	case 0x1A:
		ctx.ccd_temp_base = F1AH_M70H_CCD_TEMP_BASE;
		ctx.svi_core_addr = F1AH_M20H_SVI_TEL_PLANE0;
		ctx.svi_soc_addr = F1AH_M20H_SVI_TEL_PLANE1;
		ctx.zen_gen = 5;
		break;
	}

	if (strstr(ctx.id->brand_str, "1600X") ||
		strstr(ctx.id->brand_str, "1700X") ||
		strstr(ctx.id->brand_str, "1800X"))
		ctx.temp_offset = -20.0;
	else if (strstr(ctx.id->brand_str, "2700X"))
		ctx.temp_offset = -10.0;
	else if (strstr(ctx.id->brand_str, "Threadripper 19") ||
		strstr(ctx.id->brand_str, "Threadripper 29"))
		ctx.temp_offset = -27.0;

	switch (ctx.codename)
	{
	case CODENAME_SUMMITRIDGE: // 17h 0x01
	case CODENAME_PINNACLERIDGE: // 17h 0x08
	case CODENAME_RAVENRIDGE: // 17h 0x11
	case CODENAME_RAVENRIDGE2: // 17h 0x18
	case CODENAME_PICASSO: // 17h 0x18
	case CODENAME_DALI: // 17h 0x20
	case CODENAME_FIREFLIGHT: // 17h 0x50
		ctx.svi_core_addr = F17H_M01H_SVI_TEL_PLANE0;
		ctx.svi_soc_addr = F17H_M01H_SVI_TEL_PLANE1;
		ctx.zen_gen = 1;
		break;
	case CODENAME_THREADRIPPER: // 17h 0x01
	case CODENAME_NAPLES: // 17h 0x01
	case CODENAME_COLFAX: // 17h 0x08
		ctx.svi_core_addr = F17H_M01H_SVI_TEL_PLANE0;
		ctx.svi_soc_addr = F17H_M01H_SVI_TEL_PLANE1;
		ctx.num_ccds = 4;
		break;
	case CODENAME_CASTLEPEAK: // 17h 0x31
	case CODENAME_ROME: // 17h 0x31
		ctx.svi_core_addr = F17H_M30H_SVI_TEL_PLANE0;
		ctx.svi_soc_addr = F17H_M30H_SVI_TEL_PLANE1;
		ctx.num_ccds = 8;
		break;
	case CODENAME_RENOIR: // 17h 0x60
	case CODENAME_LUCIENNE: // 17h 0x68
	case CODENAME_MENDOCINO: // 17h 0xA0
	case CODENAME_VANGOGH: // 17h 0x90, 0x91
	case CODENAME_MERO: // 17h 0x98
		ctx.svi_core_addr = F17H_M60H_SVI_TEL_PLANE0;
		ctx.svi_soc_addr = F17H_M60H_SVI_TEL_PLANE1;
		break;
	case CODENAME_MATISSE: // 17h 0x71
		ctx.svi_core_addr = F17H_M70H_SVI_TEL_PLANE0;
		ctx.svi_soc_addr = F17H_M70H_SVI_TEL_PLANE1;
		ctx.num_ccds = 8;
		break;
	case CODENAME_MILAN: // 19h 0x00,0x01
	case CODENAME_CHAGALL: // 19h 0x08
		ctx.svi_core_addr = F19H_M01H_SVI_TEL_PLANE1;
		ctx.svi_soc_addr = F19H_M01H_SVI_TEL_PLANE0;
		ctx.num_ccds = 8;
		break;
	case CODENAME_GENOA: // 19h 0x11
		ctx.svi_core_addr = F19H_M61H_SVI_TEL_PLANE1;
		ctx.svi_soc_addr = F19H_M61H_SVI_TEL_PLANE0;
		ctx.ccd_temp_base = F19H_M11H_CCD_TEMP_BASE;
		ctx.zen_gen = 4;
		ctx.num_ccds = 8;
		break;
	case CODENAME_VERMEER: // 19h 0x20, 0x21 AM4
		ctx.svi_core_addr = F19H_M21H_SVI_TEL_PLANE0;
		ctx.svi_soc_addr = F19H_M21H_SVI_TEL_PLANE1;
		ctx.num_ccds = 2;
		break;
	case CODENAME_REMBRANDT: // 19h 0x40, 0x44
	case CODENAME_CEZANNE: // 19h 0x50
	case CODENAME_PHOENIX: // 19h 0x74, 0x75
	case CODENAME_PHOENIX2: // 19h 0x78
	case CODENAME_HAWKPOINT: // 19h 0x7C
		ctx.svi_core_addr = F19H_M50H_SVI_TEL_PLANE0;
		ctx.svi_soc_addr = F19H_M50H_SVI_TEL_PLANE1;
		break;
	case CODENAME_RAPHAEL: // 19h 0x61 AM5
		ctx.svi_core_addr = F19H_M61H_SVI_TEL_PLANE0;
		ctx.svi_soc_addr = F19H_M61H_SVI_TEL_PLANE1;
		ctx.ccd_temp_base = F1AH_M70H_CCD_TEMP_BASE;
		ctx.num_ccds = 2;
		break;
	case CODENAME_STRIXPOINT: // 1Ah 0x20, 0x24
		ctx.svi_core_addr = F1AH_M20H_SVI_TEL_PLANE0;
		ctx.svi_soc_addr = F1AH_M20H_SVI_TEL_PLANE1;
		break;
	case CODENAME_GRANITERIDGE: // 1Ah 0x44
		ctx.svi_core_addr = F1AH_M70H_SVI_TEL_PLANE0;
		ctx.svi_soc_addr = F1AH_M70H_SVI_TEL_PLANE1;
		ctx.num_ccds = 2;
		break;
	case CODENAME_STRIXHALO: // 1Ah 0x70
		ctx.svi_core_addr = F1AH_M70H_SVI_TEL_PLANE0;
		ctx.svi_soc_addr = F1AH_M70H_SVI_TEL_PLANE1;
		ctx.num_ccds = 8;
		break;
	}
	NWL_Debug("SMN", "Zen %u CCD Temp Base %x, SVI Core %x, SVI SoC %x", ctx.zen_gen, ctx.ccd_temp_base, ctx.svi_core_addr, ctx.svi_soc_addr);

	detect_num_ccds();
	if (ctx.id->num_cores > 0)
		ctx.num_cores = (uint32_t)ctx.id->num_cores;
	if (ctx.num_cores > SMU_MAX_CORE)
		ctx.num_cores = SMU_MAX_CORE;

	ctx.smu = ryzen_smu_init(NWLC->NwDrv, ctx.id);
	if (ctx.smu)
		ryzen_smu_update_pm_table(ctx.smu);

	return true;
fail:
	ZeroMemory(&ctx, sizeof(ctx));
	return false;
}

static void smn_fini(void)
{
	ryzen_smu_free(ctx.smu);
	ZeroMemory(&ctx, sizeof(ctx));
}

static inline double svi_plane_to_vcc(uint32_t plane)
{
	uint32_t vdd = 0;
	int32_t vcc = 0;
	if (ctx.zen_gen <= 3)
	{
		vdd = (plane >> 16) & 0xff;
		vcc = 1550 - (int32_t)((625 * vdd) / 100);
	}
	else
	{
		vdd = (plane >> 8) & 0xff;
		vcc = 1550 - (int32_t)((625 * vdd) / 100);
	}
	return (vcc > 0) ? (vcc / 1000.0) : 0.0;
}

static inline double svi_plane_to_core_idd(uint32_t plane)
{
	if (ctx.zen_gen <= 3)
	{
		uint32_t idd = plane & 0xff;
		uint32_t fc = (ctx.zen_gen >= 2) ? 658823 : 1039211;
		return (fc * idd) / 1000000.0;
	}
	else
	{
		uint32_t idd = plane & 0x7f;
		return (1000.0 * idd) / 4000.0;
	}
}

static inline double svi_plane_to_soc_idd(uint32_t plane)
{
	if (ctx.zen_gen <= 3)
	{
		uint32_t idd = plane & 0xff;
		uint32_t fc = (ctx.zen_gen >= 2) ? 294300 : 360772;
		return (fc * idd) / 1000000.0;
	}
	else
	{
		uint32_t idd = plane & 0x7f;
		return (350.0 * idd) / 4000.0;
	}
}

static inline double thm_to_tctl(uint32_t thm)
{
	double temp = 0.001 * ((thm >> 21) * 125);
	if (thm & F17H_TEMP_OFFSET_FLAG)
		temp += -49.0;
	return temp;
}

static inline double thm_to_tccd(uint32_t thm)
{
	if (!(thm & ZEN_CCD_TEMP_VALID))
		return 0.0;
	return (0.125 * (thm & ZEN_CCD_TEMP_MASK) - 49.0);
}

static inline void
get_smu_value(PNODE node, const char* name, ry_err_t(*func)(ry_handle_t*, float*), float* value)
{
	float data = 0.0f;
	ry_err_t err = func(ctx.smu, &data);
	if (err == RYZEN_SMU_OK)
		value = &data;
	else if (*value == 0.0f)
		return;
	NWL_NodeAttrSetf(node, name, NAFLG_FMT_NUMERIC, "%.3f", *value);
}

static inline void
get_smu_value_core(PNODE node, const char* name, ry_err_t(*func)(ry_handle_t*, uint32_t, float*))
{
	for (uint32_t i = 0; i < ctx.num_cores; i++)
	{
		float data = 0.0f;
		ry_err_t err = func(ctx.smu, i, &data);
		if (err == RYZEN_SMU_OK)
		{
			char buf[64];
			snprintf(buf, sizeof(buf), "%s %u", name, i);
			NWL_NodeAttrSetf(node, buf, NAFLG_FMT_NUMERIC, "%.3f", data);
		}
	}
}

static void smn_get(PNODE node)
{
	WR0_WaitPciBus(500);

	uint32_t thm = WR0_RdAmdSmn(NWLC->NwDrv, WR0_SMN_AMD17H, F17H_M01H_THM_TCON_CUR_TMP);
	double tctl = thm_to_tctl(thm);
	NWL_NodeAttrSetf(node, "Tctl", NAFLG_FMT_NUMERIC, "%.3f", tctl);
	double tdie = tctl - ctx.temp_offset;
	NWL_NodeAttrSetf(node, "Tdie", NAFLG_FMT_NUMERIC, "%.3f", tdie);

	for (uint8_t i = 0; i < ctx.num_ccds; i++)
	{
		char buf[] = "TccdXXX";
		snprintf(buf, sizeof(buf), "Tccd%u", i + 1);
		uint32_t thm_data = WR0_RdAmdSmn(NWLC->NwDrv, WR0_SMN_AMD17H, ctx.ccd_temp_base + (i * 4));
		double tccd = thm_to_tccd(thm_data);
		NWL_NodeAttrSetf(node, buf, NAFLG_FMT_NUMERIC, "%.3f", tccd);
	}

	uint32_t plane0 = WR0_RdAmdSmn(NWLC->NwDrv, WR0_SMN_AMD17H, ctx.svi_core_addr);
	double vcc_core = svi_plane_to_vcc(plane0);
	NWL_NodeAttrSetf(node, "SVI Core Vcc", NAFLG_FMT_NUMERIC, "%.3f", vcc_core);
	double idd_core = svi_plane_to_core_idd(plane0);
	NWL_NodeAttrSetf(node, "SVI Core Idd", NAFLG_FMT_NUMERIC, "%.3f", idd_core);

	uint32_t plane1 = WR0_RdAmdSmn(NWLC->NwDrv, WR0_SMN_AMD17H, ctx.svi_soc_addr);
	double vcc_soc = svi_plane_to_vcc(plane1);
	NWL_NodeAttrSetf(node, "SVI SoC Vcc", NAFLG_FMT_NUMERIC, "%.3f", vcc_soc);
	double idd_soc = svi_plane_to_soc_idd(plane1);
	NWL_NodeAttrSetf(node, "SVI SoC Idd", NAFLG_FMT_NUMERIC, "%.3f", idd_soc);

	if (ctx.smu)
	{
		ryzen_smu_update_pm_table(ctx.smu);

		get_smu_value(node, "STAPM Limit", ryzen_smu_get_stapm_limit, &ctx.stapm_limit);
		get_smu_value(node, "STAPM Value", ryzen_smu_get_stapm_value, &ctx.stapm_value);
		get_smu_value(node, "Fast Limit", ryzen_smu_get_fast_limit, &ctx.fast_limit);
		get_smu_value(node, "Fast Value", ryzen_smu_get_fast_value, &ctx.fast_value);
		get_smu_value(node, "Slow Limit", ryzen_smu_get_slow_limit, &ctx.slow_limit);
		get_smu_value(node, "Slow Value", ryzen_smu_get_slow_value, &ctx.slow_value);
		get_smu_value(node, "APU Slow Limit", ryzen_smu_get_apu_slow_limit, &ctx.apu_slow_limit);
		get_smu_value(node, "APU Slow Value", ryzen_smu_get_apu_slow_value, &ctx.apu_slow_value);
		get_smu_value(node, "VRM Current", ryzen_smu_get_vrm_current, &ctx.vrm_current);
		get_smu_value(node, "VRM Current Value", ryzen_smu_get_vrm_current_value, &ctx.vrm_current_value);
		get_smu_value(node, "VRM SoC Current", ryzen_smu_get_vrmsoc_current, &ctx.vrmsoc_current);
		get_smu_value(node, "VRM SoC Current Value", ryzen_smu_get_vrmsoc_current_value, &ctx.vrmsoc_current_value);
		get_smu_value(node, "GFX Temperature", ryzen_smu_get_gfx_temperature, &ctx.gfx_temperature);
		get_smu_value(node, "GFX Voltage", ryzen_smu_get_gfx_volt, &ctx.gfx_volt);
		get_smu_value(node, "GFX Clock", ryzen_smu_get_gfx_clk, &ctx.gfx_clk);
		get_smu_value(node, "PSI0 Current", ryzen_smu_get_psi0_current, &ctx.psi0_current);
		get_smu_value(node, "PSI0 SoC Current", ryzen_smu_get_psi0soc_current, &ctx.psi0soc_current);
		get_smu_value(node, "Fabric Clock", ryzen_smu_get_fclk, &ctx.fclk);
		get_smu_value(node, "Uncore Clock", ryzen_smu_get_uclk, &ctx.uclk);
		get_smu_value(node, "Memory Clock", ryzen_smu_get_mclk, &ctx.mclk);
		get_smu_value(node, "SoC Voltage", ryzen_smu_get_soc_volt, &ctx.soc_volt);
		get_smu_value(node, "CLDO VDDP", ryzen_smu_get_cldo_vddp, &ctx.cldo_vddp);
		get_smu_value(node, "L3 Clock", ryzen_smu_get_l3_clk, &ctx.l3_clk);
		get_smu_value(node, "L3 VDDM", ryzen_smu_get_l3_vddm, &ctx.l3_vddm);
		get_smu_value(node, "L3 Temperature", ryzen_smu_get_l3_temperature, &ctx.l3_temperature);
		get_smu_value(node, "Socket Power", ryzen_smu_get_socket_power, &ctx.socket_power);
		get_smu_value_core(node, "Core Temperature", ryzen_smu_get_core_temperature);
		get_smu_value_core(node, "Core Power", ryzen_smu_get_core_power);
		get_smu_value_core(node, "Core Voltage", ryzen_smu_get_core_volt);
		get_smu_value_core(node, "Core Clock", ryzen_smu_get_core_clk);
	}

	WR0_ReleasePciBus();
}

sensor_t sensor_zen =
{
	.name = "ZEN",
	.flag = NWL_SENSOR_ZEN,
	.init = smn_init,
	.get = smn_get,
	.fini = smn_fini,
};
