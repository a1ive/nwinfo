// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"
#include "sensors.h"
#include "libcpuid.h"
#include "cpu/rdmsr.h"

static struct
{
	struct system_id_t* id;
	struct msr_info_t* msr;
	ULONGLONG ticks;
} ctx;

static bool cpu_init(void)
{
	if (NWLC->NwDrv == NULL)
		return false;

	if (NWLC->NwCpuRaw->num_raw <= 0)
	{
		if (cpuid_get_all_raw_data(NWLC->NwCpuRaw) != 0)
			return false;
	}

	if (NWLC->NwCpuid->num_cpu_types <= 0)
	{
		if (cpu_identify_all(NWLC->NwCpuRaw, NWLC->NwCpuid) != 0)
			return false;
	}

	if (NWLC->NwCpuid->num_cpu_types <= 0)
		return false;

	if (NWLC->NwMsr == NULL)
	{
		NWLC->NwMsr = calloc(NWLC->NwCpuid->num_cpu_types, sizeof(struct msr_info_t));
		if (NWLC->NwMsr == NULL)
			return false;

		for (uint8_t i = 0; i < NWLC->NwCpuid->num_cpu_types; i++)
			NWL_MsrInit(&NWLC->NwMsr[i], NWLC->NwDrv, &NWLC->NwCpuid->cpu_types[i]);
	}

	ctx.id = NWLC->NwCpuid;
	ctx.msr = NWLC->NwMsr;
	ctx.ticks = GetTickCount64();

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
	NWL_NodeAttrSetf(node, "Ticks", NAFLG_FMT_NUMERIC, "%llu", GetTickCount64());

	if (ctx.msr == NULL)
		return;

	for (uint8_t i = 0; i < ctx.id->num_cpu_types; i++)
	{
		struct msr_info_t* msr = &ctx.msr[i];
		PNODE p = NWL_NodeAppendNew(node, msr->name, NFLG_ATTGROUP);
		int value = NWL_MsrGet(msr, INFO_CUR_MULTIPLIER);
		if (value > 0)
			NWL_NodeAttrSetf(p, "Multiplier", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
		value = NWL_MsrGet(msr, INFO_MIN_MULTIPLIER);
		if (value > 0)
			NWL_NodeAttrSetf(p, "Min Multiplier", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
		value = NWL_MsrGet(msr, INFO_MAX_MULTIPLIER);
		if (value > 0)
			NWL_NodeAttrSetf(p, "Max Multiplier", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
		value = NWL_MsrGet(msr, INFO_TEMPERATURE);
		if (value > 0)
			NWL_NodeAttrSetf(p, "Core Temperature", NAFLG_FMT_NUMERIC, "%d", value);
		value = NWL_MsrGet(msr, INFO_PKG_TEMPERATURE);
		if (value > 0)
			NWL_NodeAttrSetf(p, "Package Temperature", NAFLG_FMT_NUMERIC, "%d", value);
		value = NWL_MsrGet(msr, INFO_VOLTAGE);
		if (value > 0)
			NWL_NodeAttrSetf(p, "Core Voltage", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
		value = NWL_MsrGet(msr, INFO_PKG_POWER);
		if (value > 0)
			NWL_NodeAttrSetf(p, "Package Power", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
		value = NWL_MsrGet(msr, INFO_BUS_CLOCK);
		if (value > 0)
			NWL_NodeAttrSetf(p, "Bus Clock", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
		value = NWL_MsrGet(msr, INFO_PKG_PL1);
		if (value > 0)
			NWL_NodeAttrSetf(p, "PL1", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
		value = NWL_MsrGet(msr, INFO_PKG_PL2);
		if (value > 0)
			NWL_NodeAttrSetf(p, "PL2", NAFLG_FMT_NUMERIC, "%.2lf", value / 100.0);
		value = NWL_MsrGet(msr, INFO_TDP_NOMINAL);
		if (value > 0)
			NWL_NodeAttrSetf(p, "TDP", NAFLG_FMT_NUMERIC, "%d", value);
		value = NWL_MsrGet(msr, INFO_MICROCODE_VER);
		if (value > 0)
			NWL_NodeAttrSetf(p, "Microcode Rev", 0, "0x%X", (UINT32)value);
	}
}

sensor_t sensor_cpu =
{
	.name = "CPU",
	.flag = NWL_SENSOR_CPU,
	.init = cpu_init,
	.get = cpu_get,
	.fini = cpu_fini,
};
