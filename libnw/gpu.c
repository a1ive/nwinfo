// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>

#include "libnw.h"
#include "utils.h"

#pragma comment(lib, "cfgmgr32.lib")

static UINT64
GetRegValue(HKEY hKey, LPCWSTR lpszName)
{
	UINT64 ullValue = ULLONG_MAX;
	BYTE rawData[sizeof(UINT64)];
	DWORD dwType = REG_BINARY;
	DWORD dwSize = sizeof(rawData);
	RegQueryValueExW(hKey, lpszName, NULL, &dwType, rawData, &dwSize);
	// Intel GPU devices incorrectly create the key with type REG_BINARY.
	if (dwType == REG_DWORD || (dwType == REG_BINARY && dwSize == sizeof(DWORD)))
		ullValue = *(PDWORD)rawData;
	else if (dwType == REG_QWORD || (dwType == REG_BINARY && dwSize == sizeof(UINT64)))
		ullValue = *(PUINT64)rawData;
	return ullValue;
}

static UINT64
GetGpuInstalledMemory(DEVINST devHandle)
{
	UINT64 installedMemory = ULLONG_MAX;
	HKEY keyHandle;

	if (CM_Open_DevInst_Key(devHandle, KEY_READ, 0,
		RegDisposition_OpenExisting, &keyHandle, CM_REGISTRY_SOFTWARE) == CR_SUCCESS)
	{
		installedMemory = GetRegValue(keyHandle, L"HardwareInformation.qwMemorySize");

		if (installedMemory == ULLONG_MAX)
			installedMemory = GetRegValue(keyHandle, L"HardwareInformation.MemorySize");

		if (installedMemory == ULONG_MAX) // HACK
			installedMemory = ULLONG_MAX;

		RegCloseKey(keyHandle);
	}

	return installedMemory;
}

static void
PrintGpuInfo(PNODE pGpu, PWSTR devIf)
{
	DEVPROPTYPE devicePropertyType;
	DEVINST deviceInstanceHandle;
	ULONG deviceInstanceIdLength = MAX_DEVICE_ID_LEN;
	WCHAR deviceInstanceId[MAX_DEVICE_ID_LEN];
	DEVPROPTYPE propertyType;
	ULONG bufferSize = NWINFO_BUFSZ;

	DEVPROPKEY devpkeyInstanceId = { {0x78c34fc8, 0x104a, 0x4aca,
		{0x9e, 0xa4, 0x52, 0x4d, 0x52, 0x99, 0x6e, 0x57} }, 256 };
	DEVPROPKEY devpkeyDesc = { {0xa45c254e, 0xdf1c, 0x4efd,
		{0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0} },2 };

	NWL_NodeAttrSet(pGpu, "Interface", NWL_Ucs2ToUtf8(devIf), 0);

	if (CM_Get_Device_Interface_PropertyW(devIf, &devpkeyInstanceId, &devicePropertyType,
		(PBYTE)deviceInstanceId, &deviceInstanceIdLength, 0) != CR_SUCCESS)
		return;
	if (CM_Locate_DevNodeW(&deviceInstanceHandle, deviceInstanceId, CM_LOCATE_DEVNODE_NORMAL) != CR_SUCCESS)
		return;
	if (CM_Get_DevNode_PropertyW(deviceInstanceHandle, &devpkeyDesc, &propertyType,
		(PBYTE)NWLC->NwBuf, &bufferSize, 0) != CR_SUCCESS)
		return;
	NWL_NodeAttrSet(pGpu, "Description", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);
	NWL_NodeAttrSet(pGpu, "Memory Size",
		NWL_GetHumanSize(GetGpuInstalledMemory(deviceInstanceHandle), NWLC->NwUnits, 1024),
		NAFLG_FMT_HUMAN_SIZE);
}

PNODE NW_Gpu(VOID)
{
	PWSTR deviceInterfaceList = NULL;
	PWSTR p;
	ULONG deviceInterfaceListLength = 0;
	GUID guidDisplayDevice = { 0x1CA05180, 0xA699, 0x450A,
		{ 0x9A, 0x0C, 0xDE, 0x4F, 0xBE, 0x3D, 0xDD, 0x89 } };

	PNODE node = NWL_NodeAlloc("GPU", NFLG_TABLE);
	if (NWLC->GpuInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	if (CM_Get_Device_Interface_List_SizeW(&deviceInterfaceListLength,
		&guidDisplayDevice, NULL,
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT) != CR_SUCCESS)
		goto fail;
	deviceInterfaceList = calloc(deviceInterfaceListLength, sizeof(WCHAR));
	if (deviceInterfaceList == NULL)
		goto fail;
	if (CM_Get_Device_Interface_ListW(&guidDisplayDevice, NULL,
		deviceInterfaceList, deviceInterfaceListLength,
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT) != CR_SUCCESS)
		goto fail;

	for(p = deviceInterfaceList; p < deviceInterfaceList + deviceInterfaceListLength; p += wcslen(p) + 1)
	{
		if (_wcsnicmp(p, L"\\\\?\\PCI#VEN_", 12) != 0)
			continue;
		PNODE pGpu = NWL_NodeAppendNew(node, "Device", NFLG_TABLE_ROW);
		PrintGpuInfo(pGpu, p);
	}

fail:
	if (deviceInterfaceList)
		free(deviceInterfaceList);
	return node;
}
