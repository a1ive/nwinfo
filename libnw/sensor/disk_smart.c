// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"
#include "sensors.h"
#include "../../libcdi/libcdi.h"

struct disk_info
{
	CHAR name[32];
};

static struct
{
	INT count;
	ULONGLONG ticks;
	struct disk_info* disks;
} ctx;

static bool disk_init(void)
{
	if (NWLC->NwSmart == NULL)
		return false;
	if (NWLC->NwSmartInit == FALSE)
	{
		NWL_Debug("SMART", "Init");
		cdi_init_smart(NWLC->NwSmart, NWLC->NwSmartFlags);
		NWLC->NwSmartInit = TRUE;
	}
	ctx.count = cdi_get_disk_count(NWLC->NwSmart);
	NWL_Debug("SMART", "Found %d disks.", ctx.count);
	if (ctx.count > 0)
		ctx.disks = calloc(ctx.count, sizeof(struct disk_info));
	if (ctx.disks == NULL)
	{
		ZeroMemory(&ctx, sizeof(ctx));
		return false;
	}
	for (INT i = 0; i < ctx.count; i++)
	{
		struct disk_info* d = &ctx.disks[i];
		WCHAR* str = cdi_get_string(NWLC->NwSmart, i, CDI_STRING_MODEL);
		INT n = cdi_get_int(NWLC->NwSmart, i, CDI_INT_DISK_ID);
		if (n < 0)
			n = -i;
		snprintf(d->name, sizeof(d->name), "(%d) %s", n, NWL_Ucs2ToUtf8(str));
		NWL_Debug("SMART", "Add %s", d->name);
		cdi_free_string(str);
	}
	return true;
}

static void disk_fini(void)
{
	free(ctx.disks);
	ZeroMemory(&ctx, sizeof(ctx));
}

static void disk_get(PNODE node)
{
	BOOL need_update = FALSE;
	ULONGLONG ticks = GetTickCount64();
	INT count = cdi_get_disk_count(NWLC->NwSmart);
	if (count != ctx.count || ticks > ctx.ticks + 1000 * 60)
	{
		need_update = TRUE;
		ctx.ticks = ticks;
		ctx.count = count;
	}
	NWL_NodeAttrSetf(node, "Last Update", NAFLG_FMT_NUMERIC, "%llu", ctx.ticks);
	for (INT i = 0; i < ctx.count; i++)
	{
		if (need_update)
			cdi_update_smart(NWLC->NwSmart, i);
		struct disk_info* d = &ctx.disks[i];
		PNODE disk = NWL_NodeAppendNew(node, d->name, NFLG_ATTGROUP | NAFLG_FMT_KEY_QUOTE);
		INT n;
		DWORD dw;
		n = cdi_get_int(NWLC->NwSmart, i, CDI_INT_DISK_STATUS);
		NWL_NodeAttrSet(disk, "Status", cdi_get_health_status(n), 0);
		n = cdi_get_int(NWLC->NwSmart, i, CDI_INT_LIFE);
		if (n >= 0)
			NWL_NodeAttrSetf(disk, "Life", NAFLG_FMT_NUMERIC, "%d", n);
		n = cdi_get_int(NWLC->NwSmart, i, CDI_INT_TEMPERATURE);
		if (n >= 0)
			NWL_NodeAttrSetf(disk, "Temperature", NAFLG_FMT_NUMERIC, "%d", n);
		n = cdi_get_int(NWLC->NwSmart, i, CDI_INT_TEMPERATURE_ALARM);
		if (n >= 0)
			NWL_NodeAttrSetf(disk, "Alarm Temperature", NAFLG_FMT_NUMERIC, "%d", n);
		n = cdi_get_int(NWLC->NwSmart, i, CDI_INT_WEAR_LEVELING_COUNT);
		if (n >= 0)
			NWL_NodeAttrSetf(disk, "Wear Leveling Count", NAFLG_FMT_NUMERIC, "%d", n);
		n = cdi_get_int(NWLC->NwSmart, i, CDI_INT_POWER_ON_HOURS);
		if (n >= 0)
			NWL_NodeAttrSetf(disk, "Power on Hours", NAFLG_FMT_NUMERIC, "%d", n);
		dw = cdi_get_dword(NWLC->NwSmart, i, CDI_DWORD_POWER_ON_COUNT);
		NWL_NodeAttrSetf(disk, "Power on Count", NAFLG_FMT_NUMERIC, "%lu", dw);
		n = cdi_get_int(NWLC->NwSmart, i, CDI_INT_HOST_READS);
		if (n >= 0)
			NWL_NodeAttrSetf(disk, "Total Host Reads GB", NAFLG_FMT_NUMERIC, "%d", n);
		n = cdi_get_int(NWLC->NwSmart, i, CDI_INT_HOST_WRITES);
		if (n >= 0)
			NWL_NodeAttrSetf(disk, "Total Host Writes GB", NAFLG_FMT_NUMERIC, "%d", n);
	}
}

sensor_t sensor_disk_smart =
{
	.name = "SMART",
	.flag = NWL_SENSOR_SMART,
	.init = disk_init,
	.get = disk_get,
	.fini = disk_fini,
};
