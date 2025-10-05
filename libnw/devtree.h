// SPDX-License-Identifier: Unlicense
#pragma once

#define VC_EXTRALEAN
#include <windows.h>
#include <cfgmgr32.h>
#include "format.h"

//typedef void* DEVTREE_CTX;

#define DEVTREE_MAX_STR_LEN MAX_PATH

typedef struct _DEVTREE_ENUM_CTX
{
	CHAR filter[DEVTREE_MAX_STR_LEN];
	size_t filterLen;
	const char* hub;
	void* data;
	void (*GetDeviceInfo)(PNODE node, void* data, DEVINST devInst, LPCSTR hwIds);
} DEVTREE_ENUM_CTX;

BOOL NWL_SetDevPropString(CHAR* strBuf, size_t strSize, DEVINST devHandle, const DEVPROPKEY* devProperty);
CONFIGRET NWL_CMGetDevIfProp(LPCWSTR pszDevIf, CONST DEVPROPKEY* propKey, DEVPROPTYPE* propType, PBYTE propBuf, PULONG propBufSize, ULONG ulFlags);

void
NWL_EnumerateDevices(PNODE parent, DEVTREE_ENUM_CTX* ctx, DEVINST devInst);
