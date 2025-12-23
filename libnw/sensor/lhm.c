// SPDX-License-Identifier: Unlicense

#include "../libnw.h"
#include "../utils.h"
#include "sensors.h"
#include "../../liblhm/lhm.h"
#include <pathcch.h>

static struct
{
	HMODULE dll;
	LhmSensorInfo* lhm;
	size_t count;
	bool (__stdcall * fn_init)(void);
	bool (__stdcall * fn_enum)(const LhmSensorInfo** sensors, size_t* out_count);
	void (__stdcall * fn_fini)(void);
} ctx;

static bool lhm_init(void)
{
	// Check LibreHardwareMonitorLib.dll
	HANDLE hf = INVALID_HANDLE_VALUE;
	WCHAR path[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, path, MAX_PATH);
	PathCchRemoveFileSpec(path, MAX_PATH);
	PathCchAppend(path, MAX_PATH, L"LibreHardwareMonitorLib.dll");
	hf = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hf == INVALID_HANDLE_VALUE)
		return false;
	else
		CloseHandle(hf);

	// Load liblhm.dll
	ctx.dll = LoadLibraryW(L"liblhm.dll");
	if (!ctx.dll)
		goto fail;
	*(FARPROC*)&ctx.fn_init = GetProcAddress(ctx.dll, "LhmInitialize");
	if (!ctx.fn_init)
		goto fail;
	*(FARPROC*)&ctx.fn_enum = GetProcAddress(ctx.dll, "LhmEnumerateSensors");
	if (!ctx.fn_enum)
		goto fail;
	*(FARPROC*)&ctx.fn_fini = GetProcAddress(ctx.dll, "LhmShutdown");
	if (!ctx.fn_fini)
		goto fail;
	NWL_Debug("LHM", "Initializing LHM");
	if (!ctx.fn_init())
	{
		NWL_Debug("LHM", "LhmInitialize FAILED");
		goto fail;
	}

	NWL_Debug("LHM", "LhmInitialize OK");
	return true;
fail:
	if (ctx.dll)
		FreeLibrary(ctx.dll);
	ZeroMemory(&ctx, sizeof(ctx));
	return false;
}

static void lhm_fini(void)
{
	ctx.fn_fini();
	FreeLibrary(ctx.dll);
	ZeroMemory(&ctx, sizeof(ctx));
}

static void lhm_get(PNODE node)
{
	if (!ctx.fn_enum(&ctx.lhm, &ctx.count))
		return;
	NWL_Debug("LHM", "%zu objects", ctx.count);
	for (size_t i = 0; i < ctx.count; i++)
	{
		LhmSensorInfo* p = &ctx.lhm[i];
		LPCSTR hw = NWL_Ucs2ToUtf8(p->hardware);
		PNODE key = NWL_NodeGetChild(node, hw);
		if (key == NULL)
			key = NWL_NodeAppendNew(node, hw, NFLG_ATTGROUP | NAFLG_FMT_KEY_QUOTE);
		NWL_NodeAttrSetf(key, NWL_Ucs2ToUtf8(p->name), NAFLG_FMT_NUMERIC | NAFLG_FMT_KEY_QUOTE, "%.2f", p->value);
	}
}

sensor_t sensor_lhm =
{
	.name = "LibreHardwareMonitor",
	.flag = NWL_SENSOR_LHM,
	.init = lhm_init,
	.get = lhm_get,
	.fini = lhm_fini,
};
