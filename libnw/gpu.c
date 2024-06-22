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
	UINT64 ullValue = 0;
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
	UINT64 installedMemory = 0;
	HKEY keyHandle;

	if (CM_Open_DevInst_Key(devHandle, KEY_READ, 0,
		RegDisposition_OpenExisting, &keyHandle, CM_REGISTRY_SOFTWARE) == CR_SUCCESS)
	{
		installedMemory = GetRegValue(keyHandle, L"HardwareInformation.qwMemorySize");
		if (installedMemory == 0)
			installedMemory = GetRegValue(keyHandle, L"HardwareInformation.MemorySize");
		RegCloseKey(keyHandle);
	}

	return installedMemory;
}

static void
SetDevicePropertyString(PNODE pGpu, LPCSTR lpszKey, DEVINST devHandle, const DEVPROPKEY* devProperty)
{
	ULONG bufferSize = NWINFO_BUFSZ;
	DEVPROPTYPE propertyType;

	propertyType = DEVPROP_TYPE_EMPTY;

	if (CM_Get_DevNode_PropertyW(devHandle, devProperty, &propertyType,
		(PBYTE)NWLC->NwBuf, &bufferSize, 0) != CR_SUCCESS)
		return;

	switch (propertyType)
	{
	case DEVPROP_TYPE_STRING:
	case DEVPROP_TYPE_STRING_LIST: // TODO: add multi sz support
		NWL_NodeAttrSet(pGpu, lpszKey, NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);
		break;
	case DEVPROP_TYPE_FILETIME:
	{
		SYSTEMTIME sysTime = { 0 };
		FileTimeToSystemTime((PFILETIME)NWLC->NwBuf, &sysTime);
		NWL_NodeAttrSetf(pGpu, lpszKey, 0, "%5u-%02u-%02u %02u:%02u%02u",
			sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	}
		break;
	case DEVPROP_TYPE_UINT32:
	{
		UINT32 u;
		memcpy(&u, NWLC->NwBuf, sizeof(UINT32));
		NWL_NodeAttrSetf(pGpu, lpszKey, NAFLG_FMT_NUMERIC, "%u", u);
	}
		break;
	case DEVPROP_TYPE_UINT64:
	{
		UINT64 u;
		memcpy(&u, NWLC->NwBuf, sizeof(UINT64));
		NWL_NodeAttrSetf(pGpu, lpszKey, NAFLG_FMT_NUMERIC, "%llu", u);
	}
		break;
	}
}

static void
PrintGpuInfo(PNODE pGpu, PWSTR devIf)
{
	DEVPROPTYPE devicePropertyType;
	DEVINST deviceInstanceHandle;
	ULONG deviceInstanceIdLength = MAX_DEVICE_ID_LEN;
	WCHAR deviceInstanceId[MAX_DEVICE_ID_LEN];

	DEVPROPKEY devpkeyInstanceId = { {0x78c34fc8, 0x104a, 0x4aca,
		{0x9e, 0xa4, 0x52, 0x4d, 0x52, 0x99, 0x6e, 0x57} }, 256 };
	DEVPROPKEY devpKeyHardwareIds = { {0xa45c254e, 0xdf1c, 0x4efd,
		{0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0} }, 3 };
	DEVPROPKEY devpkeyDesc = { {0xa45c254e, 0xdf1c, 0x4efd,
		{0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0} },2 };
	DEVPROPKEY devpkeyManufacturer = { {0xa45c254e, 0xdf1c, 0x4efd,
		{0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0} }, 13 };
	DEVPROPKEY devpkeyDriverDate = { { 0xa8b865dd, 0x2e3d, 0x4094,
		{0xad, 0x97, 0xe5, 0x93, 0xa7, 0x0c, 0x75, 0xd6} }, 2 };
	DEVPROPKEY devpkeyDriverVersion = { { 0xa8b865dd, 0x2e3d, 0x4094,
		{0xad, 0x97, 0xe5, 0x93, 0xa7, 0x0c, 0x75, 0xd6} }, 3 };
	DEVPROPKEY devpkeyLocationInfo = { { 0xa45c254e, 0xdf1c, 0x4efd,
		{0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0} }, 15 };

	NWL_NodeAttrSet(pGpu, "Interface", NWL_Ucs2ToUtf8(devIf), 0);

	if (CM_Get_Device_Interface_PropertyW(devIf, &devpkeyInstanceId, &devicePropertyType,
		(PBYTE)deviceInstanceId, &deviceInstanceIdLength, 0) != CR_SUCCESS)
		return;
	if (CM_Locate_DevNodeW(&deviceInstanceHandle, deviceInstanceId, CM_LOCATE_DEVNODE_NORMAL) != CR_SUCCESS)
		return;

	SetDevicePropertyString(pGpu, "HWID", deviceInstanceHandle, &devpKeyHardwareIds);
	SetDevicePropertyString(pGpu, "Description", deviceInstanceHandle, &devpkeyDesc);
	SetDevicePropertyString(pGpu, "Manufacturer", deviceInstanceHandle, &devpkeyManufacturer);
	SetDevicePropertyString(pGpu, "Driver Date", deviceInstanceHandle, &devpkeyDriverDate);
	SetDevicePropertyString(pGpu, "Driver Version", deviceInstanceHandle, &devpkeyDriverVersion);
	SetDevicePropertyString(pGpu, "Location Info", deviceInstanceHandle, &devpkeyLocationInfo);

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
