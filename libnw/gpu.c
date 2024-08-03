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
SetDevicePropertyString(CHAR* strBuf, size_t strSize, DEVINST devHandle, const DEVPROPKEY* devProperty)
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
		strcpy_s(strBuf, strSize, NWL_Ucs2ToUtf8(NWLC->NwBufW));
		break;
	case DEVPROP_TYPE_FILETIME:
	{
		SYSTEMTIME sysTime = { 0 };
		FileTimeToSystemTime((PFILETIME)NWLC->NwBuf, &sysTime);
		snprintf(strBuf, strSize, "%5u-%02u-%02u %02u:%02u%02u",
			sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	}
		break;
	case DEVPROP_TYPE_UINT32:
	{
		UINT32 u;
		memcpy(&u, NWLC->NwBuf, sizeof(UINT32));
		snprintf(strBuf, strSize, "%u", u);
	}
		break;
	case DEVPROP_TYPE_UINT64:
	{
		UINT64 u;
		memcpy(&u, NWLC->NwBuf, sizeof(UINT64));
		snprintf(strBuf, strSize, "%llu", u);
	}
		break;
	}
}

static void
FillGpuInfo(NWLIB_GPU_INFO* info, LPCWSTR devIf)
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

	if (CM_Get_Device_Interface_PropertyW(devIf, &devpkeyInstanceId, &devicePropertyType,
		(PBYTE)deviceInstanceId, &deviceInstanceIdLength, 0) != CR_SUCCESS)
		return;
	if (CM_Locate_DevNodeW(&deviceInstanceHandle, deviceInstanceId, CM_LOCATE_DEVNODE_NORMAL) != CR_SUCCESS)
		return;

	info->driver = TRUE;
	snprintf(info->gpu_if, NWL_STR_SIZE, "%s", NWL_Ucs2ToUtf8(devIf));

	SetDevicePropertyString(info->gpu_hwid, NWL_STR_SIZE, deviceInstanceHandle, &devpKeyHardwareIds);
	SetDevicePropertyString(info->gpu_device, NWL_STR_SIZE, deviceInstanceHandle, &devpkeyDesc);
	SetDevicePropertyString(info->gpu_vendor, NWL_STR_SIZE, deviceInstanceHandle, &devpkeyManufacturer);
	SetDevicePropertyString(info->gpu_driver_date, NWL_STR_SIZE, deviceInstanceHandle, &devpkeyDriverDate);
	SetDevicePropertyString(info->gpu_driver_ver, NWL_STR_SIZE, deviceInstanceHandle, &devpkeyDriverVersion);
	SetDevicePropertyString(info->gpu_location, NWL_STR_SIZE, deviceInstanceHandle, &devpkeyLocationInfo);

	info->gpu_mem_size = GetGpuInstalledMemory(deviceInstanceHandle);
}

static inline BOOL
CompareHwid(LPCSTR hwid, NWLIB_GPU_INFO* info, int count)
{
	for (size_t i = 0; i < count && info[i].driver; i++)
	{
		if (_stricmp(hwid, info[i].gpu_hwid) == 0)
			return TRUE;
	}
	return FALSE;
}

int
NWL_GetGpuInfo(NWLIB_GPU_INFO* info, int count)
{
	int ret = 0;
	PWSTR deviceInterfaceList = NULL;
	PWSTR p;
	ULONG deviceInterfaceListLength = 0;
	GUID guidDisplayDevice = { 0x1CA05180, 0xA699, 0x450A,
		{ 0x9A, 0x0C, 0xDE, 0x4F, 0xBE, 0x3D, 0xDD, 0x89 } };

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

	for (ret = 0, p = deviceInterfaceList;
		ret < count && p < deviceInterfaceList + deviceInterfaceListLength;
		p += wcslen(p) + 1)
	{
		if (_wcsnicmp(p, L"\\\\?\\PCI#VEN_", 12) != 0)
			continue;
		FillGpuInfo(&info[ret], p);
		ret++;
	}

	PNODE pciList = NWL_EnumPci(NWL_NodeAlloc("PCI", NFLG_TABLE), "03");
	for (size_t i = 0; ret < count && pciList->Children[i].LinkedNode; i++)
	{
		PNODE pci = pciList->Children[i].LinkedNode;
		LPCSTR hwid = NWL_NodeAttrGet(pci, "HWID");
		if (CompareHwid(hwid, info, count))
			continue;
		strcpy_s(info[ret].gpu_hwid, NWL_STR_SIZE, hwid);
		strcpy_s(info[ret].gpu_device, NWL_STR_SIZE, NWL_NodeAttrGet(pci, "Device"));
		strcpy_s(info[ret].gpu_vendor, NWL_STR_SIZE, NWL_NodeAttrGet(pci, "Vendor"));
		ret++;
	}

fail:
	if (deviceInterfaceList)
		free(deviceInterfaceList);
	return ret;
}

PNODE NW_Gpu(VOID)
{
	NWLIB_GPU_INFO info[NWL_GPU_MAX_COUNT] = { 0 };
	int count = NWL_GPU_MAX_COUNT;

	PNODE node = NWL_NodeAlloc("GPU", NFLG_TABLE);
	if (NWLC->GpuInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	count = NWL_GetGpuInfo(info, count);
	for (int i = 0; i < count; i++)
	{
		PNODE gpu = NWL_NodeAppendNew(node, "Device", NFLG_TABLE_ROW);
		NWL_NodeAttrSet(gpu, "HWID", info[i].gpu_hwid, 0);
		NWL_NodeAttrSet(gpu, "Device", info[i].gpu_device, 0);
		NWL_NodeAttrSet(gpu, "Vendor", info[i].gpu_vendor, 0);
		if (info[i].driver)
		{
			NWL_NodeAttrSet(gpu, "Interface", info[i].gpu_if, 0);
			NWL_NodeAttrSet(gpu, "Driver Date", info[i].gpu_driver_date, 0);
			NWL_NodeAttrSet(gpu, "Driver Version", info[i].gpu_driver_ver, 0);
			NWL_NodeAttrSet(gpu, "Location", info[i].gpu_location, 0);
			NWL_NodeAttrSet(gpu, "Memory Size",
				NWL_GetHumanSize(info[i].gpu_mem_size, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		}
	}

	return node;
}
