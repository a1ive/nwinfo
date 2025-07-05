// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <initguid.h>
#include <devpkey.h>

#include "libnw.h"
#include "utils.h"

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
GetDeviceInfo(PNODE nusb, CHAR* ids, DWORD idsSize, DEVINST devInst, LPCWSTR instanceId)
{
	NWL_NodeAttrSet(nusb, "HWID", NWL_Ucs2ToUtf8(instanceId), 0);

	NWL_ParseHwid(nusb, ids, idsSize, instanceId, 1);

	// Parse hardware class if available
	WCHAR* compatibleIds = NWL_GetDevStrProp(devInst, &DEVPKEY_Device_CompatibleIds);
	if (compatibleIds)
	{
		ParseHwClass(nusb, ids, idsSize, compatibleIds);
		free(compatibleIds);
	}

	// Get and print device name using DEVPKEY_NAME
	WCHAR* name = NWL_GetDevStrProp(devInst, &DEVPKEY_NAME);
	if (name)
	{
		NWL_NodeAttrSet(nusb, "Name", NWL_Ucs2ToUtf8(name), 0);
		free(name);
	}

	// Check if it's a Mass Storage Device and get disk name
	WCHAR* diskName = GetUsbDiskName(devInst);
	if (diskName)
	{
		NWL_NodeAttrSet(nusb, "Disk", NWL_Ucs2ToUtf8(diskName), 0);
		free(diskName);
	}
}

static inline PNODE AppendUsbHub(PNODE parent)
{
	if (parent->Flags & NFLG_TABLE)
		return parent; // Already a table, return as is
	PNODE ret = NWL_NodeGetChild(parent, "USB Hub");
	if (ret)
		return ret; // Found existing USB Hub node
	// Create a new USB Hub node
	return NWL_NodeAppendNew(parent, "USB Hub", NFLG_TABLE);
}

static void EnumerateUsbDevices(PNODE parent, CHAR* ids, DWORD idsSize, DEVINST devInst)
{
	PNODE nusb = parent;
	DEVINST childInst;
	WCHAR* instanceId = NWL_GetDevStrProp(devInst, &DEVPKEY_Device_InstanceId);
	if (instanceId)
	{
		if (wcsncmp(instanceId, L"USB\\", 4) == 0)
		{
			nusb = NWL_NodeAppendNew(AppendUsbHub(parent), "Device", NFLG_TABLE_ROW);
			GetDeviceInfo(nusb, ids, idsSize, devInst, instanceId);
		}
		free(instanceId);
	}

	if (CM_Get_Child(&childInst, devInst, 0) == CR_SUCCESS)
	{
		EnumerateUsbDevices(nusb, ids, idsSize, childInst);
		DEVINST siblingInst = childInst;
		while (CM_Get_Sibling(&siblingInst, siblingInst, 0) == CR_SUCCESS)
			EnumerateUsbDevices(nusb, ids, idsSize, siblingInst);
	}
}

PNODE NW_Usb(VOID)
{
	DEVINST devRoot;
	CONFIGRET cr;
	CHAR* ids = NULL;
	DWORD idsSize = 0;
	PNODE node = NWL_NodeAlloc("USB", NFLG_TABLE);
	if (NWLC->UsbInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	ids = NWL_LoadIdsToMemory(L"usb.ids", &idsSize);

	cr = CM_Locate_DevNodeW(&devRoot, NULL, CM_LOCATE_DEVNODE_NORMAL);
	if (cr != CR_SUCCESS)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "CM_Locate_DevNodeW failed");
		goto fail;
	}

	EnumerateUsbDevices(node, ids, idsSize, devRoot);

fail:
	free(ids);
	return node;
}