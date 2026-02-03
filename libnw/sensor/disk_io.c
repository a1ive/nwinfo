// SPDX-License-Identifier: Unlicense

#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winioctl.h>
#include "libnw.h"
#include "utils.h"
#include "disk.h"
#include "sensors.h"

struct disk_stats
{
	CHAR name[32];
	DISK_PERFORMANCE perf;
};

static struct
{
	PHY_DRIVE_INFO* drives;
	DWORD count;
	struct disk_stats* stats;
} ctx;

static bool disk_init(void)
{
	ctx.count = NWL_GetDriveInfoList(FALSE, FALSE, &ctx.drives);
	if (ctx.count == 0 || ctx.drives == NULL)
		return false;

	ctx.stats = calloc(ctx.count, sizeof(struct disk_stats));
	if (ctx.stats == NULL)
	{
		NWL_DestoryDriveInfoList(ctx.drives, ctx.count);
		ZeroMemory(&ctx, sizeof(ctx));
		return false;
	}

	qsort(ctx.drives, ctx.count, sizeof(PHY_DRIVE_INFO), NWL_CompareDiskId);

	for (DWORD i = 0; i < ctx.count; i++)
	{
		struct disk_stats* st = &ctx.stats[i];
		PHY_DRIVE_INFO* d = &ctx.drives[i];
		DWORD retsz;

		snprintf(st->name, sizeof(st->name), "(%lu) %s", d->Index, NWL_Ucs2ToUtf8(d->HwName));

		if (d->Handle == INVALID_HANDLE_VALUE)
			continue;

		if (!DeviceIoControl(d->Handle, IOCTL_DISK_PERFORMANCE,
			NULL, 0, &st->perf, sizeof(DISK_PERFORMANCE),
			&retsz, NULL))
			continue;
	}

	return true;
}

static void disk_fini(void)
{
	if (ctx.drives)
		NWL_DestoryDriveInfoList(ctx.drives, ctx.count);
	free(ctx.stats);
	ZeroMemory(&ctx, sizeof(ctx));
}

static void disk_get(PNODE node)
{
	DWORD current = NWL_GetDriveCount(FALSE);
	if (current != ctx.count)
	{
		disk_fini();
		if (!disk_init())
			return;
	}

	for (DWORD i = 0; i < ctx.count; i++)
	{
		struct disk_stats* st = &ctx.stats[i];
		PHY_DRIVE_INFO* d = &ctx.drives[i];
		DISK_PERFORMANCE perf = { 0 };
		DWORD retsz;
		PNODE disk = NWL_NodeAppendNew(node, st->name, NFLG_ATTGROUP | NAFLG_FMT_KEY_QUOTE);

		if (d->Handle == INVALID_HANDLE_VALUE)
			continue;
		if (!DeviceIoControl(d->Handle, IOCTL_DISK_PERFORMANCE,
			NULL, 0, &perf, sizeof(DISK_PERFORMANCE),
			&retsz, NULL))
			continue;

		UINT64 dt_100ns = perf.QueryTime.QuadPart - st->perf.QueryTime.QuadPart; // in 100 ns
		UINT64 drb = perf.BytesRead.QuadPart - st->perf.BytesRead.QuadPart;
		UINT64 dwb = perf.BytesWritten.QuadPart - st->perf.BytesWritten.QuadPart;
		UINT64 drc = perf.ReadCount - st->perf.ReadCount;
		UINT64 dwc = perf.WriteCount - st->perf.WriteCount;
		UINT64 drt_100ns = perf.ReadTime.QuadPart - st->perf.ReadTime.QuadPart;
		UINT64 dwt_100ns = perf.WriteTime.QuadPart - st->perf.WriteTime.QuadPart;
		UINT64 dit_100ns = perf.IdleTime.QuadPart - st->perf.IdleTime.QuadPart;
		UINT64 dsc = perf.SplitCount - st->perf.SplitCount;

		UINT64 read_speed = 0;
		UINT64 write_speed = 0;
		double read_iops = 0;
		double write_iops = 0;
		double read_latency = 0; // in ms
		double write_latency = 0; // in ms
		double percent = 0;
		double split_rate = 0;

		if (dt_100ns > 0)
		{
			read_speed = drb * 10000000ULL / dt_100ns;
			write_speed = dwb * 10000000ULL / dt_100ns;
			read_iops = drc * 10000000.0 / dt_100ns;
			write_iops = dwc * 10000000.0 / dt_100ns;
			if (drc > 0)
				read_latency = drt_100ns / drc / 10000.0;
			if (dwc > 0)
				write_latency = dwt_100ns / dwc / 10000.0;
			if (dt_100ns > dit_100ns)
				percent = (dt_100ns - dit_100ns) * 100.0 / dt_100ns;
			split_rate = (perf.SplitCount - st->perf.SplitCount) * 10000000.0 / dt_100ns;
		}

		NWL_NodeAttrSet(disk, "Read Speed", NWL_GetHumanSize(read_speed, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		NWL_NodeAttrSet(disk, "Write Speed", NWL_GetHumanSize(write_speed, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		NWL_NodeAttrSetf(disk, "Read IOPS", NAFLG_FMT_NUMERIC, "%.2f", read_iops);
		NWL_NodeAttrSetf(disk, "Write IOPS", NAFLG_FMT_NUMERIC, "%.2f", write_iops);
		NWL_NodeAttrSetf(disk, "Read Latency ms", NAFLG_FMT_NUMERIC, "%.2f", read_latency);
		NWL_NodeAttrSetf(disk, "Write Latency ms", NAFLG_FMT_NUMERIC, "%.2f", write_latency);
		NWL_NodeAttrSetf(disk, "Utilization", NAFLG_FMT_NUMERIC, "%.2f", percent);
		NWL_NodeAttrSetf(disk, "Split Rate", NAFLG_FMT_NUMERIC, "%.2f", split_rate);

		NWL_NodeAttrSet(disk, "Bytes Read", NWL_GetHumanSize(perf.BytesRead.QuadPart, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		NWL_NodeAttrSet(disk, "Bytes Written", NWL_GetHumanSize(perf.BytesWritten.QuadPart, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		NWL_NodeAttrSetf(disk, "Queue Depth", NAFLG_FMT_NUMERIC, "%lu", perf.QueueDepth);
		NWL_NodeAttrSetf(disk, "Split Count", NAFLG_FMT_NUMERIC, "%lu", perf.SplitCount);

		memcpy(&st->perf, &perf, sizeof(DISK_PERFORMANCE));
	}
}
sensor_t sensor_disk_io =
{
	.name = "DISKIO",
	.flag = NWL_SENSOR_DISK,
	.init = disk_init,
	.get = disk_get,
	.fini = disk_fini,
};
