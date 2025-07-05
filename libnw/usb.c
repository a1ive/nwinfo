// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <initguid.h>
#include <devpkey.h>

#include "libnw.h"
#include "utils.h"
#include "devtree.h"

typedef struct _DEVTREE_CTX
{
	CHAR* ids;
	DWORD idsSize;
} DEVTREE_CTX;

static WCHAR* GetUsbDiskName(DEVINST usbDevInst)
{
	// Check if the service is USBSTOR or UASPStor
	WCHAR* serviceName = NWL_GetDevStrProp(usbDevInst, &DEVPKEY_Device_Service);
	if (!serviceName)
		return NULL;

	if (_wcsicmp(serviceName, L"USBSTOR") != 0 && _wcsicmp(serviceName, L"UASPStor") != 0)
	{
		free(serviceName);
		return NULL;
	}
	free(serviceName);

	DEVINST childDevice;
	for (CONFIGRET cr = CM_Get_Child(&childDevice, usbDevInst, 0);
		cr == CR_SUCCESS;
		cr = CM_Get_Sibling(&childDevice, childDevice, 0))
	{
		WCHAR* className = NWL_GetDevStrProp(childDevice, &DEVPKEY_Device_Class);
		if (className)
		{
			if (_wcsicmp(className, L"DiskDrive") == 0)
			{
				free(className);
				return NWL_GetDevStrProp(childDevice, &DEVPKEY_NAME);
			}
			free(className);
		}
	}
	return NULL;
}

static void
ParseHwClass(PNODE nd, CHAR* ids, DWORD idsSize, LPCWSTR compId)
{
	// USB\Class_XX&SubClass_XX&Prot_XX
	// USB\DevClass_XX&SubClass_XX&Prot_XX
	WCHAR hwClass[7] = { 0 };
	
	for (LPCWSTR curId = compId; *curId; curId += wcslen(curId) + 1)
	{
		if (wcsncmp(curId, L"USB\\Class_", 10) == 0)
		{
			NWL_NodeAttrSet(nd, "Compatible ID", NWL_Ucs2ToUtf8(curId), 0);
			LPCWSTR strClass = wcsstr(curId, L"Class_");
			if (strClass)
				wcsncpy_s(hwClass, 3, strClass + 6, 2);
			strClass = wcsstr(curId, L"SubClass_");
			if (strClass)
				wcsncpy_s(hwClass + 2, 3, strClass + 9, 2);
			strClass = wcsstr(curId, L"Prot_");
			if (strClass)
				wcsncpy_s(hwClass + 4, 3, strClass + 5, 2);
			hwClass[6] = L'\0';
			break;
		}
	}

	if (hwClass[0])
	{
		LPCSTR u8Class = NWL_Ucs2ToUtf8(hwClass);
		NWL_NodeAttrSet(nd, "Class Code", u8Class, 0);
		NWL_FindClass(nd, ids, idsSize, u8Class, 1);
	}
}

static void
GetDeviceInfoUsb(PNODE node, void* data, DEVINST devInst, LPCWSTR instanceId)
{
	DEVTREE_CTX* ctx = (DEVTREE_CTX*)data;
	NWL_NodeAttrSet(node, "HWID", NWL_Ucs2ToUtf8(instanceId), 0);

	NWL_ParseHwid(node, ctx->ids, ctx->idsSize, instanceId, 1);

	// Parse hardware class if available
	WCHAR* compatibleIds = NWL_GetDevStrProp(devInst, &DEVPKEY_Device_CompatibleIds);
	if (compatibleIds)
	{
		ParseHwClass(node, ctx->ids, ctx->idsSize, compatibleIds);
		free(compatibleIds);
	}

	// Get and print device name using DEVPKEY_NAME
	WCHAR* name = NWL_GetDevStrProp(devInst, &DEVPKEY_NAME);
	if (name)
	{
		NWL_NodeAttrSet(node, "Name", NWL_Ucs2ToUtf8(name), 0);
		free(name);
	}

	// Check if it's a Mass Storage Device and get disk name
	WCHAR* diskName = GetUsbDiskName(devInst);
	if (diskName)
	{
		NWL_NodeAttrSet(node, "Disk", NWL_Ucs2ToUtf8(diskName), 0);
		free(diskName);
	}
}

PNODE NW_Usb(VOID)
{
	DEVINST devRoot;
	CONFIGRET cr;
	DEVTREE_CTX data =
	{
		.ids = NULL,
		.idsSize = 0,
	};
	DEVTREE_ENUM_CTX ctx =
	{
		.filter = L"USB\\",
		.filterLen = 4, // Length of "USB\\"
		.data = &data,
		.hub = "USB Hub",
		.GetDeviceInfo = GetDeviceInfoUsb,
	};
	PNODE node = NWL_NodeAlloc("USB", NFLG_TABLE);
	if (NWLC->UsbInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	data.ids = NWL_LoadIdsToMemory(L"usb.ids", &data.idsSize);

	cr = CM_Locate_DevNodeW(&devRoot, NULL, CM_LOCATE_DEVNODE_NORMAL);
	if (cr != CR_SUCCESS)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "CM_Locate_DevNodeW failed");
		goto fail;
	}

	NWL_EnumerateDevices(node, &ctx, devRoot);

fail:
	free(data.ids);
	return node;
}
