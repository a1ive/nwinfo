// SPDX-License-Identifier: Unlicense

#pragma once

#define VC_EXTRALEAN
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct _GNW_AUDIO_DEV
	{
		BOOL is_default;
		float volume;
		WCHAR name[MAX_PATH];
	} GNW_AUDIO_DEV;

	GNW_AUDIO_DEV* gnwinfo_get_audio(UINT* count);

#ifdef __cplusplus
}
#endif
