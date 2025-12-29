// SPDX-License-Identifier: Unlicense

#include "../libnw.h"
#include "../utils.h"
#include "sensors.h"
#include "libcpuid.h"
#include "../cpu/rdmsr.h"

static struct
{
	struct cpu_raw_data_t raw;
	struct cpu_id_t id;
	ULONGLONG ticks;
	double power;
	double vid;
	double multiplier;
	double bus_clk;
	int core_temp;
	int pkg_temp;
	int energy;
} ctx;

static bool cpu_init(void)
{
	cpuid_set_warn_function(NWL_Debug);
	cpuid_get_raw_data(&ctx.raw);
	cpu_identify(&ctx.raw, &ctx.id);
	ctx.ticks = GetTickCount64();

	if (NWLC->NwDrv)
		ctx.energy = cpu_msrinfo(NWLC->NwDrv, 0, INFO_PKG_ENERGY);

	NWL_GetCpuUsage();
	NWL_GetCpuFreq();
	return true;
}

static void cpu_fini(void)
{
	ZeroMemory(&ctx, sizeof(ctx));
}

static void cpu_get(PNODE node)
{
	NWL_NodeAttrSetf(node, "Utilization", NAFLG_FMT_NUMERIC, "%.2f", NWL_GetCpuUsage());
	NWL_NodeAttrSetf(node, "Frequency", NAFLG_FMT_NUMERIC, "%u", NWL_GetCpuFreq());

	ULONGLONG ticks = GetTickCount64();
	if (NWLC->NwDrv)
	{
		int value = cpu_msrinfo(NWLC->NwDrv, 0, INFO_PKG_ENERGY);
		if (value != CPU_INVALID_VALUE && value > 0)
		{
			if (value > ctx.energy && ticks > ctx.ticks)
			{
				ULONGLONG delta_ticks = ticks - ctx.ticks;
				double delta_energy = (double)(value - ctx.energy);
				ctx.power = delta_energy / (double)delta_ticks * 10.0;
			}
			ctx.energy = value;
		}
		value = cpu_msrinfo(NWLC->NwDrv, 0, INFO_PKG_TEMPERATURE);
		if (value != CPU_INVALID_VALUE && value > 0)
			ctx.pkg_temp = value;
		value = cpu_msrinfo(NWLC->NwDrv, 0, INFO_TEMPERATURE);
		if (value != CPU_INVALID_VALUE && value > 0)
			ctx.core_temp = value;
		value = cpu_msrinfo(NWLC->NwDrv, 0, INFO_VOLTAGE);
		if (value != CPU_INVALID_VALUE && value > 0)
			ctx.vid = value / 100.0;
		value = cpu_msrinfo(NWLC->NwDrv, 0, INFO_BUS_CLOCK);
		if (value != CPU_INVALID_VALUE && value > 0)
			ctx.bus_clk = value / 100.0;
		value = cpu_msrinfo(NWLC->NwDrv, 0, INFO_CUR_MULTIPLIER);
		if (value != CPU_INVALID_VALUE && value > 0)
			ctx.multiplier = value / 100.0;
	}
	ctx.ticks = ticks;
	NWL_NodeAttrSetf(node, "Ticks", NAFLG_FMT_NUMERIC, "%llu", ctx.ticks);
	NWL_NodeAttrSetf(node, "Package Temperature", NAFLG_FMT_NUMERIC, "%d", ctx.pkg_temp);
	NWL_NodeAttrSetf(node, "Core Temperature", NAFLG_FMT_NUMERIC, "%d", ctx.core_temp);
	NWL_NodeAttrSetf(node, "Core Voltage", NAFLG_FMT_NUMERIC, "%.2lf", ctx.vid);
	NWL_NodeAttrSetf(node, "Energy", NAFLG_FMT_NUMERIC, "%.2lf", ctx.energy / 100.0);
	NWL_NodeAttrSetf(node, "Power", NAFLG_FMT_NUMERIC, "%.2lf", ctx.power);
	NWL_NodeAttrSetf(node, "Bus Clock", NAFLG_FMT_NUMERIC, "%.2lf", ctx.bus_clk);
	NWL_NodeAttrSetf(node, "Multiplier", NAFLG_FMT_NUMERIC, "%.2lf", ctx.multiplier);
}

sensor_t sensor_cpu =
{
	.name = "CPU",
	.flag = NWL_SENSOR_CPU,
	.init = cpu_init,
	.get = cpu_get,
	.fini = cpu_fini,
};
