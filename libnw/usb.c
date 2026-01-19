// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <initguid.h>
#include <usbiodef.h>
#include <usbioctl.h>
#include <devpkey.h>

#include "libnw.h"
#include "utils.h"
#include "devtree.h"

#define USB_STRING_DESCRIPTOR_MAX_LEN 255
#define USB_PIPE_INFO_MAX_COUNT 64
#define USB_LANGID_EN_US 0x0409

static const char*
UsbConnectionStatusToStr(USB_CONNECTION_STATUS status)
{
	switch (status)
	{
	case NoDeviceConnected:
		return "No Device Connected";
	case DeviceConnected:
		return "Connected";
	case DeviceFailedEnumeration:
		return "Failed Enumeration";
	case DeviceGeneralFailure:
		return "General Failure";
	case DeviceCausedOvercurrent:
		return "Overcurrent";
	case DeviceNotEnoughPower:
		return "Not Enough Power";
	case DeviceNotEnoughBandwidth:
		return "Not Enough Bandwidth";
	case DeviceHubNestedTooDeeply:
		return "Hub Nested Too Deeply";
	case DeviceInLegacyHub:
		return "In Legacy Hub";
	default:
		return "Unknown";
	}
}

static inline void
UsbParsePortFromLocationPaths(const CHAR* locPaths, ULONG* port)
{
	const char* cur = locPaths;
	const char* last = NULL;
	while ((cur = strstr(cur, "USB(")) != NULL)
	{
		last = cur;
		cur += 4;
	}
	if (!last)
		return;
	last += 4;
	char* end = NULL;
	unsigned long value = strtoul(last, &end, 10);
	if (end == last || *end != ')' || value == 0)
		return;
	*port = (ULONG)value;
}

static HANDLE
UsbOpenHubHandle(DEVINST hubInst)
{
	WCHAR instanceId[MAX_DEVICE_ID_LEN];
	ULONG listSize = 0;
	WCHAR* list = NULL;
	HANDLE hubHandle = INVALID_HANDLE_VALUE;

	if (CM_Get_Device_IDW(hubInst, instanceId, MAX_DEVICE_ID_LEN, 0) != CR_SUCCESS)
		return INVALID_HANDLE_VALUE;
	if (CM_Get_Device_Interface_List_SizeW(&listSize, (LPGUID)&GUID_DEVINTERFACE_USB_HUB,
		instanceId, CM_GET_DEVICE_INTERFACE_LIST_PRESENT) != CR_SUCCESS)
		return INVALID_HANDLE_VALUE;
	if (listSize <= 1)
		return INVALID_HANDLE_VALUE;
	list = (WCHAR*)calloc(listSize, sizeof(WCHAR));
	if (!list)
		return INVALID_HANDLE_VALUE;
	if (CM_Get_Device_Interface_ListW((LPGUID)&GUID_DEVINTERFACE_USB_HUB,
		instanceId, list, listSize, CM_GET_DEVICE_INTERFACE_LIST_PRESENT) != CR_SUCCESS)
	{
		free(list);
		return INVALID_HANDLE_VALUE;
	}
	if (list[0] != L'\0')
	{
		hubHandle = CreateFileW(list, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	free(list);
	return hubHandle;
}

static PUSB_NODE_CONNECTION_INFORMATION_EX
UsbGetConnectionInfo(HANDLE hubHandle, ULONG connIndex)
{
	DWORD bytes = 0;
	DWORD size = sizeof(USB_NODE_CONNECTION_INFORMATION_EX) + (sizeof(USB_PIPE_INFO) * USB_PIPE_INFO_MAX_COUNT);
	PUSB_NODE_CONNECTION_INFORMATION_EX info = calloc(1, size);
	if (!info)
		return NULL;
	info->ConnectionIndex = connIndex;
	if (!DeviceIoControl(hubHandle, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
		info, size, info, size, &bytes, NULL))
	{
		free(info);
		return NULL;
	}
	return info;
}

static PUSB_NODE_CONNECTION_INFORMATION_EX_V2
UsbGetConnectionInfo2(HANDLE hubHandle, ULONG connIndex)
{
	// Windows 8
	if (NWLC->NwOsInfo.dwMajorVersion < 6 ||
		(NWLC->NwOsInfo.dwMajorVersion == 6 && NWLC->NwOsInfo.dwMinorVersion < 2))
		return NULL;

	DWORD bytes = 0;
	DWORD size = sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2);
	PUSB_NODE_CONNECTION_INFORMATION_EX_V2 info = calloc(1, size);

	if (!info)
		return NULL;

	info->ConnectionIndex = connIndex;
	info->Length = size;
	info->SupportedUsbProtocols.Usb110 = 1;
	info->SupportedUsbProtocols.Usb200 = 1;
	info->SupportedUsbProtocols.Usb300 = 1;

	if (!DeviceIoControl(hubHandle, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX_V2, info, size, info, size, &bytes, NULL))
	{
		free(info);
		return NULL;
	}
	return info;
}

static LPCSTR
UsbGetStringDescriptor(HANDLE hubHandle, ULONG connIndex, UCHAR descIndex)
{
	DWORD bytes = 0;
	UCHAR reqBuf[sizeof(USB_DESCRIPTOR_REQUEST) + USB_STRING_DESCRIPTOR_MAX_LEN] = { 0 };
	PUSB_DESCRIPTOR_REQUEST request = (PUSB_DESCRIPTOR_REQUEST)reqBuf;
	if (descIndex == 0)
		return NULL;
	request->ConnectionIndex = connIndex;
	request->SetupPacket.bmRequest = 0x80;
	request->SetupPacket.bRequest = USB_REQUEST_GET_DESCRIPTOR;
	request->SetupPacket.wValue = (USB_STRING_DESCRIPTOR_TYPE << 8) | descIndex;
	request->SetupPacket.wIndex = USB_LANGID_EN_US;
	request->SetupPacket.wLength = USB_STRING_DESCRIPTOR_MAX_LEN;
	if (!DeviceIoControl(hubHandle, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
		request, sizeof(reqBuf), request, sizeof(reqBuf), &bytes, NULL))
		return NULL;
	if (request->Data[0] < 2 || request->Data[1] != USB_STRING_DESCRIPTOR_TYPE)
		return NULL;
	size_t charCount = (request->Data[0] - 2) / 2;
	size_t maxChars = (USB_STRING_DESCRIPTOR_MAX_LEN - 4) / 2; // reserve space for null terminator
	if (charCount > maxChars)
		charCount = maxChars;
	((WCHAR*)(request->Data + 2))[charCount] = L'\0';
	return NWL_Ucs2ToUtf8((LPCWSTR)(request->Data + 2));
}

#define USB_BCD_MAJOR(bcd) (((bcd) >> 12) & 0x0F) * 10 + (((bcd) >> 8) & 0x0F)
#define USB_BCD_MINOR(bcd) (((bcd) >> 4) & 0x0F) * 10 + ((bcd) & 0x0F)

static void
UsbPrintConnectionInfo(PNODE node, const USB_NODE_CONNECTION_INFORMATION_EX* info, const USB_NODE_CONNECTION_INFORMATION_EX_V2* info2)
{
	const char* speedStr = NULL;

	if (info2)
	{
		if (info2->Flags.DeviceIsOperatingAtSuperSpeedPlusOrHigher)
			speedStr = "USB 3.x (SuperSpeedPlus)";
		else if (info2->Flags.DeviceIsOperatingAtSuperSpeedOrHigher)
			speedStr = "USB 3.0 (SuperSpeed)";
	}
	if (speedStr == NULL)
	{
		switch (info->Speed)
		{
		case UsbLowSpeed:
			speedStr = "USB 1.1 (Low Speed)";
			break;
		case UsbFullSpeed:
			speedStr = "USB 1.1 (Full Speed)";
			break;
		case UsbHighSpeed:
			speedStr = "USB 2.0 (High Speed)";
			break;
		case UsbSuperSpeed:
			speedStr = "USB 3.0 (SuperSpeed)";
			break;
		default:
			speedStr = "Unknown";
			break;
		}
	}

	NWL_NodeAttrSet(node, "Speed", speedStr, 0);

	NWL_NodeAttrSetf(node, "USB Version", 0, "%u.%02u",
		USB_BCD_MAJOR(info->DeviceDescriptor.bcdUSB), USB_BCD_MINOR(info->DeviceDescriptor.bcdUSB));
	NWL_NodeAttrSetf(node, "Device Version", 0, "%u.%02u",
		USB_BCD_MAJOR(info->DeviceDescriptor.bcdDevice), USB_BCD_MINOR(info->DeviceDescriptor.bcdDevice));

	NWL_NodeAttrSetf(node, "Class Code", 0, "%02X%02X%02X",
		info->DeviceDescriptor.bDeviceClass, info->DeviceDescriptor.bDeviceSubClass, info->DeviceDescriptor.bDeviceProtocol);

	NWL_NodeAttrSet(node, "Connection Status", UsbConnectionStatusToStr(info->ConnectionStatus), 0);
	NWL_NodeAttrSetBool(node, "Is Hub", info->DeviceIsHub, 0);
	NWL_NodeAttrSetf(node, "Configurations", NAFLG_FMT_NUMERIC, "%u", info->DeviceDescriptor.bNumConfigurations);
	NWL_NodeAttrSetf(node, "Current Configuration", NAFLG_FMT_NUMERIC, "%u", info->CurrentConfigurationValue);
	NWL_NodeAttrSetf(node, "Max Packet Size", NAFLG_FMT_NUMERIC, "%u", info->DeviceDescriptor.bMaxPacketSize0);
	NWL_NodeAttrSetf(node, "Device Address", NAFLG_FMT_NUMERIC, "%u", info->DeviceAddress);
	NWL_NodeAttrSetf(node, "Open Pipes", NAFLG_FMT_NUMERIC, "%lu", info->NumberOfOpenPipes);
}

static void
UsbPrintDescriptorStrings(PNODE node, HANDLE hubHandle, ULONG connIndex, const USB_DEVICE_DESCRIPTOR* desc)
{
	LPCSTR u8str = NULL;

	if (desc->iManufacturer &&
		(u8str = UsbGetStringDescriptor(hubHandle, connIndex, desc->iManufacturer)) != NULL)
		NWL_NodeAttrSet(node, "Manufacturer", u8str, 0);

	if (desc->iProduct &&
		(u8str = UsbGetStringDescriptor(hubHandle, connIndex, desc->iProduct)) != NULL)
		NWL_NodeAttrSet(node, "Product", u8str, 0);

	if (desc->iSerialNumber &&
		(u8str = UsbGetStringDescriptor(hubHandle, connIndex, desc->iSerialNumber)) != NULL)
		NWL_NodeAttrSet(node, "Serial Number", u8str, NAFLG_FMT_SENSITIVE);
}

static void
SetUsbConnectionInfo(PNODE node, DEVINST parentDevInst, ULONG ulPort)
{
	HANDLE hubHandle = INVALID_HANDLE_VALUE;
	PUSB_NODE_CONNECTION_INFORMATION_EX info = NULL;
	PUSB_NODE_CONNECTION_INFORMATION_EX_V2 info2 = NULL;

	NWL_NodeAttrSetf(node, "Port", NAFLG_FMT_NUMERIC, "%lu", ulPort);

	if (parentDevInst == 0)
		return;

	hubHandle = UsbOpenHubHandle(parentDevInst);
	if (hubHandle == INVALID_HANDLE_VALUE)
		return;

	info = UsbGetConnectionInfo(hubHandle, ulPort);
	if (!info)
	{
		CloseHandle(hubHandle);
		return;
	}

	info2 = UsbGetConnectionInfo2(hubHandle, ulPort);

	UsbPrintConnectionInfo(node, info, info2);
	UsbPrintDescriptorStrings(node, hubHandle, ulPort, &info->DeviceDescriptor);

	free(info);
	free(info2);
	CloseHandle(hubHandle);
}

static void SetUsbDiskName(PNODE node, DEVINST usbDevInst)
{
	CHAR buf[DEVTREE_MAX_STR_LEN];
	// Check if the service is USBSTOR or UASPStor
	if (!NWL_SetDevPropString(buf, DEVTREE_MAX_STR_LEN, usbDevInst, &DEVPKEY_Device_Service))
		return;

	if (_stricmp(buf, "USBSTOR") != 0 && _stricmp(buf, "UASPStor") != 0)
		return;

	DEVINST childDevice;
	for (CONFIGRET cr = CM_Get_Child(&childDevice, usbDevInst, 0);
		cr == CR_SUCCESS;
		cr = CM_Get_Sibling(&childDevice, childDevice, 0))
	{
		if (NWL_SetDevPropString(buf, DEVTREE_MAX_STR_LEN, childDevice, &DEVPKEY_Device_Class))
		{
			if (_stricmp(buf, "DiskDrive") == 0)
			{
				NWL_SetDevPropString(buf, DEVTREE_MAX_STR_LEN, childDevice, &DEVPKEY_NAME);
				NWL_NodeAttrSet(node, "Disk", buf, 0);
			}
		}
	}
}

static void
ParseHwClass(PNODE nd, PNWLIB_IDS ids, LPCWSTR compId)
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
		NWL_FindClass(nd, ids, u8Class, 1);
	}
}

#define INVALID_USB_PORT 0xFFFFFFFF

static void
GetDeviceInfoUsb(PNODE node, void* data, DEVINST devInst, DEVINST parentDevInst, LPCSTR hwIds)
{
	ULONG port = INVALID_USB_PORT;
	CHAR buf[DEVTREE_MAX_STR_LEN];
	PNWLIB_IDS ids = (PNWLIB_IDS)data;
	NWL_NodeAttrSet(node, "HWID", hwIds, 0);

	NWL_ParseHwid(node, ids, NWL_Utf8ToUcs2(hwIds), 1);

	// Parse hardware class if available
	if (NWL_SetDevPropString(buf, DEVTREE_MAX_STR_LEN, devInst, &DEVPKEY_Device_CompatibleIds))
		ParseHwClass(node, ids, NWL_Utf8ToUcs2(buf));

	// Get and print device name using DEVPKEY_NAME
	if (NWL_SetDevPropString(buf, DEVTREE_MAX_STR_LEN, devInst, &DEVPKEY_NAME))
		NWL_NodeAttrSet(node, "Name", buf, 0);

	if (NWL_SetDevPropString(buf, DEVTREE_MAX_STR_LEN, devInst, &DEVPKEY_Device_LocationInfo))
		NWL_NodeAttrSet(node, "Location", buf, 0);

	if (NWL_SetDevPropString(buf, DEVTREE_MAX_STR_LEN, devInst, &DEVPKEY_Device_LocationPaths))
	{
		NWL_NodeAttrSet(node, "Location Paths", buf, 0);
		UsbParsePortFromLocationPaths(buf, &port);
	}

	if (port != INVALID_USB_PORT)
		SetUsbConnectionInfo(node, parentDevInst, port);

	// Check if it's a Mass Storage Device and get disk name
	SetUsbDiskName(node, devInst);
}

PNODE NW_Usb(BOOL bAppend)
{
	DEVINST devRoot;
	CONFIGRET cr;
	DEVTREE_ENUM_CTX ctx =
	{
		.filter = "USB\\",
		.filterLen = 4, // Length of "USB\\"
		.data = &NWLC->NwUsbIds,
		.hub = "USB Hub",
		.GetDeviceInfo = GetDeviceInfoUsb,
	};
	PNODE node = NWL_NodeAlloc("USB", NFLG_TABLE);
	if (bAppend)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	cr = CM_Locate_DevNodeW(&devRoot, NULL, CM_LOCATE_DEVNODE_NORMAL);
	if (cr != CR_SUCCESS)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "CM_Locate_DevNodeW failed");
		goto fail;
	}

	NWL_EnumerateDevices(node, &ctx, devRoot, 0);

fail:
	return node;
}
