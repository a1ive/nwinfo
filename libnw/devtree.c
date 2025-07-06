// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <initguid.h>
#include <devpkey.h>

#include "libnw.h"
#include "utils.h"
#include "devtree.h"

WCHAR* NWL_GetDevStrProp(DEVINST devInst, const DEVPROPKEY* pKey)
{
	DEVPROPTYPE propType;
	ULONG propSize = 0;
	CONFIGRET cr = CM_Get_DevNode_PropertyW(devInst, pKey, &propType, NULL, &propSize, 0);

	if (cr != CR_BUFFER_SMALL)
		return NULL;

	BYTE* buffer = (BYTE*)malloc(propSize);
	if (!buffer)
		return NULL;

	cr = CM_Get_DevNode_PropertyW(devInst, pKey, &propType, buffer, &propSize, 0);
	if (cr != CR_SUCCESS)
	{
		free(buffer);
		return NULL;
	}

	// Check if it's a string or list of strings before returning
	if (propType == DEVPROP_TYPE_STRING || propType == DEVPROP_TYPE_STRING_LIST)
		return (WCHAR*)buffer;

	free(buffer);
	return NULL;
}

static void
GetDeviceInfoDefault(PNODE node, void* data, DEVINST devInst, LPCWSTR hwIds)
{
	(void)data;
	NWL_NodeAttrSet(node, "HWID", NWL_Ucs2ToUtf8(hwIds), 0);

	WCHAR* name = NWL_GetDevStrProp(devInst, &DEVPKEY_NAME);
	if (name)
	{
		NWL_NodeAttrSet(node, "Name", NWL_Ucs2ToUtf8(name), 0);
		free(name);
	}
}

static PNODE AppendDevices(PNODE parent, const char* hub)
{
	if (parent->Flags & NFLG_TABLE)
		return parent;
	PNODE ret = NWL_NodeGetChild(parent, hub);
	if (ret)
		return ret;
	return NWL_NodeAppendNew(parent, hub, NFLG_TABLE);
}

void
NWL_EnumerateDevices(PNODE parent, DEVTREE_ENUM_CTX* ctx, DEVINST devInst)
{
	PNODE node = parent;
	DEVINST childInst;
	WCHAR* hwIds = NWL_GetDevStrProp(devInst, &DEVPKEY_Device_HardwareIds);

	if (hwIds)
	{
		if (ctx->filter[0] == L'\0' || _wcsnicmp(hwIds, ctx->filter, ctx->filterLen) == 0)
		{
			node = NWL_NodeAppendNew(AppendDevices(parent, ctx->hub), "Device", NFLG_TABLE_ROW);
			ctx->GetDeviceInfo(node, ctx->data, devInst, hwIds);
		}
		free(hwIds);
	}

	if (CM_Get_Child(&childInst, devInst, 0) == CR_SUCCESS)
	{
		NWL_EnumerateDevices(node, ctx, childInst);
		DEVINST siblingInst = childInst;
		while (CM_Get_Sibling(&siblingInst, siblingInst, 0) == CR_SUCCESS)
			NWL_EnumerateDevices(node, ctx, siblingInst);
	}
}

PNODE NW_DevTree(VOID)
{
	DEVTREE_ENUM_CTX ctx =
	{
		.filter = L"\0",
		.data = NULL,
		.hub = "Devices",
		.GetDeviceInfo = GetDeviceInfoDefault,
	};
	DEVINST devRoot;
	CONFIGRET cr;
	PNODE node = NWL_NodeAlloc("Device Tree", NFLG_TABLE);
	if (NWLC->DevTree)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	if (NWLC->DevTreeFilter)
	{
		wcscpy_s(ctx.filter, 32, NWL_Utf8ToUcs2(NWLC->DevTreeFilter));
		ctx.filterLen = wcslen(ctx.filter);
	}

	cr = CM_Locate_DevNodeW(&devRoot, NULL, CM_LOCATE_DEVNODE_NORMAL);
	if (cr != CR_SUCCESS)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "CM_Locate_DevNodeW failed");
		goto fail;
	}

	NWL_EnumerateDevices(node, &ctx, devRoot);

fail:
	return node;
}
