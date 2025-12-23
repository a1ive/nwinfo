// SPDX-License-Identifier: Unlicense

#include "../libnw.h"
#include "../utils.h"
#include "sensors.h"
#include "shmem.h"

#define GPUZ_SHMEM_NAME L"GPUZShMem"
#define GPUZ_MAX_RECORDS 128
#define GPUZ_STR_LEN 256
#define GPUZ_UNIT_LEN 8

#pragma pack(push, 1)
typedef struct
{
	WCHAR key[GPUZ_STR_LEN];
	WCHAR value[GPUZ_STR_LEN];
} GPUZ_RECORD;

typedef struct
{
	WCHAR name[GPUZ_STR_LEN];
	WCHAR unit[GPUZ_UNIT_LEN];
	UINT32 digits;
	double value;
} GPUZ_SENSOR_RECORD;

typedef struct
{
	UINT32 version; // 1
	volatile LONG busy; // Is data being accessed
	UINT32 lastUpdate; // GetTickCount() of last update
	GPUZ_RECORD data[GPUZ_MAX_RECORDS];
	GPUZ_SENSOR_RECORD sensors[GPUZ_MAX_RECORDS];
} GPUZ_SH_MEM;
#pragma pack(pop)

#define GPUZSHM "GPUZ"

static struct
{
	struct wr0_shmem_t shmem;
	GPUZ_SH_MEM* data;
	CHAR key[GPUZ_STR_LEN];
} ctx;

static bool gpuz_init(void)
{
	if (WR0_OpenShMem(&ctx.shmem, GPUZ_SHMEM_NAME))
		goto fail;
	if (ctx.shmem.size < sizeof(GPUZ_SH_MEM))
		goto fail;
	ctx.data = ctx.shmem.addr;
	return true;
fail:
	WR0_CloseShMem(&ctx.shmem);
	ZeroMemory(&ctx, sizeof(ctx));
	return false;
}

static void gpuz_fini(void)
{
	WR0_CloseShMem(&ctx.shmem);
	ZeroMemory(&ctx, sizeof(ctx));
}

static void gpuz_get(PNODE node)
{
	for (size_t i = 0; i < GPUZ_MAX_RECORDS; i++)
	{
		GPUZ_RECORD* r = &ctx.data->data[i];
		if (r->key[0] == L'\0')
			continue;
		strcpy_s(ctx.key, GPUZ_STR_LEN, NWL_Ucs2ToUtf8(r->key));
		NWL_NodeAttrSet(node, ctx.key, NWL_Ucs2ToUtf8(r->value), NAFLG_FMT_KEY_QUOTE);
	}
	for (size_t i = 0; i < GPUZ_MAX_RECORDS; i++)
	{
		GPUZ_SENSOR_RECORD* r = &ctx.data->sensors[i];
		if (r->name[0] == L'\0')
			continue;
		strcpy_s(ctx.key, GPUZ_STR_LEN, NWL_Ucs2ToUtf8(r->name));
		NWL_NodeAttrSetf(node, ctx.key, NAFLG_FMT_KEY_QUOTE | NAFLG_FMT_NUMERIC, "%.2f", r->value);
	}
}

sensor_t sensor_gpuz =
{
	.name = "GPUZ",
	.flag = NWL_SENSOR_GPUZ,
	.init = gpuz_init,
	.get = gpuz_get,
	.fini = gpuz_fini,
};
