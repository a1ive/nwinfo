// SPDX-License-Identifier: Unlicense

#include "../libnw.h"
#include "../utils.h"
#include "sensors.h"
#include "../smbus/smbus.h"

static NWLIB_MEM_SENSORS ctx;
static const char* dimm_names[8] = { "DIMM 0", "DIMM 1", "DIMM 2", "DIMM 3", "DIMM 4", "DIMM 5", "DIMM 6", "DIMM 7" };

static bool dimm_init(void)
{
	if (NWLC->NwSmbus == NULL)
		NWLC->NwSmbus = SM_Init(NWLC->NwDrv);
	if (NWLC->NwSmbus == NULL)
		goto fail;
	NWL_GetMemSensors(NWLC->NwSmbus, &ctx);
	return true;
fail:
	ZeroMemory(&ctx, sizeof(ctx));
	return false;
}

static void dimm_fini(void)
{
	ZeroMemory(&ctx, sizeof(ctx));
}

static void dimm_get(PNODE node)
{
	NWL_GetMemSensors(NWLC->NwSmbus, &ctx);
	for (uint32_t i = 0; i < ctx.Count; i++)
	{
		uint8_t dimm_id = ctx.Sensor[i].Addr - SPD_SLABE_ADDR_BASE;
		LPCSTR name = dimm_names[dimm_id];
		PNODE dimm = NWL_NodeGetChild(node, name);
		if (dimm == NULL)
			dimm = NWL_NodeAppendNew(node, name, NFLG_ATTGROUP | NAFLG_FMT_KEY_QUOTE);
		NWL_NodeAttrSetf(dimm, "Temperature", NAFLG_FMT_NUMERIC, "%.2f", ctx.Sensor[i].Temp);
	}
}

sensor_t sensor_dimm =
{
	.name = "DIMM",
	.init = dimm_init,
	.get = dimm_get,
	.fini = dimm_fini,
};
