// SPDX-License-Identifier: Unlicense
#pragma once

#define VC_EXTRALEAN
#include <windows.h>
#include <cfgmgr32.h>
#include "format.h"

//typedef void* DEVTREE_CTX;

typedef struct _DEVTREE_ENUM_CTX
{
	WCHAR filter[32];
	size_t filterLen;
	const char* hub;
	void* data;
	void (*GetDeviceInfo)(PNODE node, void* data, DEVINST devInst, LPCWSTR instanceId);
} DEVTREE_ENUM_CTX;

WCHAR* NWL_GetDevStrProp(DEVINST devInst, const DEVPROPKEY* pKey);

void
NWL_EnumerateDevices(PNODE parent, DEVTREE_ENUM_CTX* ctx, DEVINST devInst);
