// SPDX-License-Identifier: Unlicense

#pragma once

#define VC_EXTRALEAN
#include <windows.h>

#include "nwapi.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct _NWLIB_AUDIO_DEV
	{
		BOOL is_default;
		float volume;
		WCHAR name[MAX_PATH];
		WCHAR id[MAX_PATH];
	} NWLIB_AUDIO_DEV;

	LIBNW_API NWLIB_AUDIO_DEV* NWL_GetAudio(UINT* count);

#ifdef __cplusplus
}
#endif
