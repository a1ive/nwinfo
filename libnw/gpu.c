// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <devpkey.h>
#include <pdhmsg.h>

#include "libnw.h"
#include "utils.h"
#include "devtree.h"

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
FillGpuInfo(NWLIB_GPU_DEV* info, LPCWSTR devIf)
{
	DEVPROPTYPE devicePropertyType;
	DEVINST deviceInstanceHandle;
	ULONG deviceInstanceIdLength = MAX_DEVICE_ID_LEN;
	WCHAR deviceInstanceId[MAX_DEVICE_ID_LEN];

	if (NWL_CMGetDevIfProp(devIf, &DEVPKEY_Device_InstanceId, &devicePropertyType,
		(PBYTE)deviceInstanceId, &deviceInstanceIdLength, 0) != CR_SUCCESS)
		return;
	if (CM_Locate_DevNodeW(&deviceInstanceHandle, deviceInstanceId, CM_LOCATE_DEVNODE_NORMAL) != CR_SUCCESS)
		return;

	info->driver = TRUE;
	snprintf(info->gpu_if, NWL_STR_SIZE, "%s", NWL_Ucs2ToUtf8(devIf));

	NWL_SetDevPropString(info->gpu_hwid, NWL_STR_SIZE, deviceInstanceHandle, &DEVPKEY_Device_HardwareIds);
	NWL_SetDevPropString(info->gpu_device, NWL_STR_SIZE, deviceInstanceHandle, &DEVPKEY_Device_DeviceDesc);
	NWL_SetDevPropString(info->gpu_vendor, NWL_STR_SIZE, deviceInstanceHandle, &DEVPKEY_Device_Manufacturer);
	NWL_SetDevPropString(info->gpu_driver_date, NWL_STR_SIZE, deviceInstanceHandle, &DEVPKEY_Device_DriverDate);
	NWL_SetDevPropString(info->gpu_driver_ver, NWL_STR_SIZE, deviceInstanceHandle, &DEVPKEY_Device_DriverVersion);
	NWL_SetDevPropString(info->gpu_location, NWL_STR_SIZE, deviceInstanceHandle, &DEVPKEY_Device_LocationInfo);

	info->gpu_mem_size = GetGpuInstalledMemory(deviceInstanceHandle);
}

static inline BOOL
CompareHwid(LPCSTR hwid, NWLIB_GPU_DEV* info, int count)
{
	for (int i = 0; i < count && info[i].driver; i++)
	{
		if (_stricmp(hwid, info[i].gpu_hwid) == 0)
			return TRUE;
	}
	return FALSE;
}

static int
GetGpuDev(NWLIB_GPU_DEV* dev, int count, PUINT64 ram)
{
	int ret = 0;
	PWSTR deviceInterfaceList = NULL;
	PWSTR p;
	ULONG deviceInterfaceListLength = 0;
	GUID guidDisplayDevice = { 0x1CA05180, 0xA699, 0x450A,
		{ 0x9A, 0x0C, 0xDE, 0x4F, 0xBE, 0x3D, 0xDD, 0x89 } };

	*ram = 0;

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
		FillGpuInfo(&dev[ret], p);
		*ram += dev[ret].gpu_mem_size;
		ret++;
	}

	PNODE pciList = NWL_EnumPci(NWL_NodeAlloc("PCI", NFLG_TABLE), "03");
	for (size_t i = 0; ret < count && pciList->Children[i].LinkedNode; i++)
	{
		PNODE pci = pciList->Children[i].LinkedNode;
		LPCSTR hwid = NWL_NodeAttrGet(pci, "HWID");
		if (CompareHwid(hwid, dev, count))
			continue;
		strcpy_s(dev[ret].gpu_hwid, NWL_STR_SIZE, hwid);
		strcpy_s(dev[ret].gpu_device, NWL_STR_SIZE, NWL_NodeAttrGet(pci, "Device"));
		strcpy_s(dev[ret].gpu_vendor, NWL_STR_SIZE, NWL_NodeAttrGet(pci, "Vendor"));
		ret++;
	}

fail:
	if (deviceInterfaceList)
		free(deviceInterfaceList);
	return ret;
}

VOID NWL_GetGpuInfo(PNWLIB_GPU_INFO info)
{
	ZeroMemory(info, sizeof(NWLIB_GPU_INFO));
	info->DeviceCount = GetGpuDev(info->Device, NWL_GPU_MAX_COUNT, &info->DedicatedTotal);
	if (NWLC->PdhGpuUsage)
	{
		info->Usage3D = NWL_GetPdhSum(NWLC->PdhGpuUsage, PDH_FMT_DOUBLE, L"engtype_3D").doubleValue;
		info->UsageCopy = NWL_GetPdhSum(NWLC->PdhGpuUsage, PDH_FMT_DOUBLE, L"engtype_Copy").doubleValue;
		info->UsageCompute0 = NWL_GetPdhSum(NWLC->PdhGpuUsage, PDH_FMT_DOUBLE, L"engtype_Compute 0").doubleValue;
		info->UsageCompute1 = NWL_GetPdhSum(NWLC->PdhGpuUsage, PDH_FMT_DOUBLE, L"engtype_Compute 1").doubleValue;
	}
	if (NWLC->PdhGpuCurMem)
		info->DedicatedInUse = NWL_GetPdhSum(NWLC->PdhGpuCurMem, PDH_FMT_LARGE, NULL).largeValue;
	if (info->DedicatedTotal > 0)
		info->UsageDedicated = (double)info->DedicatedInUse / (double)info->DedicatedTotal * 100.0f;
	if (info->UsageDedicated > 100.0f)
		info->UsageDedicated = 100.0f;
}

PNODE NW_Gpu(VOID)
{
	NWLIB_GPU_INFO info;

	PNODE node = NWL_NodeAlloc("GPU", NFLG_TABLE);
	if (NWLC->GpuInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	NWL_GetGpuInfo(&info);
	for (int i = 0; i < info.DeviceCount; i++)
	{
		PNODE gpu = NWL_NodeAppendNew(node, "Device", NFLG_TABLE_ROW);
		NWL_NodeAttrSet(gpu, "HWID", info.Device[i].gpu_hwid, 0);
		NWL_NodeAttrSet(gpu, "Device", info.Device[i].gpu_device, 0);
		NWL_NodeAttrSet(gpu, "Vendor", info.Device[i].gpu_vendor, 0);
		if (info.Device[i].driver)
		{
			NWL_NodeAttrSet(gpu, "Interface", info.Device[i].gpu_if, 0);
			NWL_NodeAttrSet(gpu, "Driver Date", info.Device[i].gpu_driver_date, 0);
			NWL_NodeAttrSet(gpu, "Driver Version", info.Device[i].gpu_driver_ver, 0);
			NWL_NodeAttrSet(gpu, "Location", info.Device[i].gpu_location, 0);
			NWL_NodeAttrSet(gpu, "Memory Size",
				NWL_GetHumanSize(info.Device[i].gpu_mem_size, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		}
	}

	return node;
}
