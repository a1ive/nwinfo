// SPDX-License-Identifier: Unlicense

#include "../libnw.h"
#include "../utils.h"
#include "sensors.h"
#include "../smbus/smbus.h"

static struct
{
	MEMORYSTATUSEX statex;
	NWLIB_MEM_SENSORS ts;
} ctx;
static const char* dimm_names[8] = { "DIMM 0", "DIMM 1", "DIMM 2", "DIMM 3", "DIMM 4", "DIMM 5", "DIMM 6", "DIMM 7" };

static bool dimm_init(void)
{
	ctx.statex.dwLength = sizeof(MEMORYSTATUSEX);
	if (NWLC->NwSmbus == NULL)
		NWLC->NwSmbus = SM_Init(NWLC->NwDrv);
	return true;
}

static void dimm_fini(void)
{
	ZeroMemory(&ctx, sizeof(ctx));
}

static void dimm_get(PNODE node)
{
	GlobalMemoryStatusEx(&ctx.statex);
	PNODE mem = NWL_NodeEnumChild(node, 0);
	if (mem == NULL)
		mem = NWL_NodeAppendNew(node, "Memory", NFLG_ATTGROUP);
	NWL_NodeAttrSetf(mem, "Load", NAFLG_FMT_NUMERIC, "%u", ctx.statex.dwMemoryLoad);
	NWL_NodeAttrSetf(mem, "Total Physical", NAFLG_FMT_NUMERIC, "%llu", ctx.statex.ullTotalPhys);
	NWL_NodeAttrSetf(mem, "Available Physical", NAFLG_FMT_NUMERIC, "%llu", ctx.statex.ullAvailPhys);
	NWL_NodeAttrSetf(mem, "Total Page File", NAFLG_FMT_NUMERIC, "%llu", ctx.statex.ullTotalPageFile);
	NWL_NodeAttrSetf(mem, "Available Page File", NAFLG_FMT_NUMERIC, "%llu", ctx.statex.ullAvailPageFile);
	NWL_NodeAttrSetf(mem, "Total Virtual", NAFLG_FMT_NUMERIC, "%llu", ctx.statex.ullTotalVirtual);
	NWL_NodeAttrSetf(mem, "Available Virtual", NAFLG_FMT_NUMERIC, "%llu", ctx.statex.ullAvailVirtual);
	NWL_NodeAttrSetf(mem, "Available Extended Virtual", NAFLG_FMT_NUMERIC, "%llu", ctx.statex.ullAvailExtendedVirtual);

	if (!NWLC->NwSmbus)
		return;

	NWL_GetMemSensors(NWLC->NwSmbus, &ctx.ts);
	for (uint32_t i = 0; i < ctx.ts.Count; i++)
	{
		if (!ctx.ts.Sensor[i].Type)
			continue;
		uint8_t dimm_id = ctx.ts.Sensor[i].Addr - SPD_SLABE_ADDR_BASE;
		LPCSTR name = dimm_names[dimm_id];
		PNODE dimm = NWL_NodeGetChild(node, name);
		if (dimm == NULL)
			dimm = NWL_NodeAppendNew(node, name, NFLG_ATTGROUP | NAFLG_FMT_KEY_QUOTE);
		NWL_NodeAttrSetf(dimm, "Temperature", NAFLG_FMT_NUMERIC, "%.2f", ctx.ts.Sensor[i].Temp);
	}
}

sensor_t sensor_dimm =
{
	.name = "DIMM",
	.flag = NWL_SENSOR_DIMM,
	.init = dimm_init,
	.get = dimm_get,
	.fini = dimm_fini,
};
