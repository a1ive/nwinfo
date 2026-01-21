// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"
#include "sensors.h"
#include "gpu/gpu.h"

static bool gpu_init(void)
{
	if (NWLC->NwGpu == NULL)
		NWLC->NwGpu = NWL_InitGpu();
	if (NWLC->NwGpu == NULL)
		goto fail;

	return true;
fail:
	return false;
}

static void gpu_fini(void)
{
}

static void gpu_get(PNODE node)
{
	NWL_GetGpuInfo(NWLC->NwGpu);
	for (uint32_t i = 0; i < NWLC->NwGpu->DeviceCount; i++)
	{
		NWLIB_GPU_DEV* dev = &NWLC->NwGpu->Device[i];
		PNODE gpu = NWL_NodeGetChild(node, dev->Name);
		if (gpu == NULL)
			gpu = NWL_NodeAppendNew(node, dev->Name, NFLG_ATTGROUP | NAFLG_FMT_KEY_QUOTE);

		NWL_NodeAttrSetf(gpu, "Utilization", NAFLG_FMT_NUMERIC, "%.2f", dev->UsagePercent);
		NWL_NodeAttrSetf(gpu, "Temperature", NAFLG_FMT_NUMERIC, "%.2f", dev->Temperature);
		NWL_NodeAttrSetf(gpu, "Total Dedicated Memory", NAFLG_FMT_NUMERIC, "%llu", dev->TotalMemory);
		NWL_NodeAttrSetf(gpu, "Free Dedicated Memory", NAFLG_FMT_NUMERIC, "%llu", dev->FreeMemory);
		NWL_NodeAttrSetf(gpu, "Memory Usage", NAFLG_FMT_NUMERIC, "%llu", dev->MemoryPercent);
		NWL_NodeAttrSetf(gpu, "Power", NAFLG_FMT_NUMERIC, "%.2f", dev->Power);
		NWL_NodeAttrSetf(gpu, "Frequency", NAFLG_FMT_NUMERIC, "%.2f", dev->Frequency);
		NWL_NodeAttrSetf(gpu, "Voltage", NAFLG_FMT_NUMERIC, "%.2f", dev->Voltage);
		NWL_NodeAttrSetf(gpu, "Fan Speed", NAFLG_FMT_NUMERIC, "%llu", dev->FanSpeed);
	}
}

sensor_t sensor_gpu =
{
	.name = "GPU",
	.flag = NWL_SENSOR_GPU,
	.init = gpu_init,
	.get = gpu_get,
	.fini = gpu_fini,
};
