// SPDX-License-Identifier: MPL-2.0
#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LHM_STR_MAX 128

#pragma pack(push,8)
typedef struct
{
	wchar_t hardware[LHM_STR_MAX];
	wchar_t id[LHM_STR_MAX];
	wchar_t name[LHM_STR_MAX];
	wchar_t type[LHM_STR_MAX];
	bool has_value;
	float value;
	bool has_min;
	float min;
	bool has_max;
	float max;
} LhmSensorInfo;
#pragma pack(pop)

bool __stdcall LhmInitialize(void);
size_t __stdcall LhmGetToggleCount(void);
bool __stdcall LhmGetToggleInfo(size_t index, const wchar_t** out_name, bool* out_enabled);
bool __stdcall LhmSetToggleEnabled(size_t index, bool enabled);
bool __stdcall LhmEnumerateSensors(const LhmSensorInfo** sensors, size_t* out_count);
void __stdcall LhmShutdown(void);
const wchar_t* __stdcall LhmGetLastError(void);

#ifdef __cplusplus
}
#endif
