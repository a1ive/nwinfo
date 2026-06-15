// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <initguid.h>
#include <devpkey.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <hidpi.h>

#include "libnw.h"
#include "utils.h"

typedef void (WINAPI* PFN_HidD_GetHidGuid)(LPGUID);
typedef BOOLEAN (WINAPI* PFN_HidD_GetAttributes)(HANDLE, PHIDD_ATTRIBUTES);
typedef BOOLEAN (WINAPI* PFN_HidD_GetManufacturerString)(HANDLE, PVOID, ULONG);
typedef BOOLEAN (WINAPI* PFN_HidD_GetProductString)(HANDLE, PVOID, ULONG);
typedef BOOLEAN (WINAPI* PFN_HidD_GetSerialNumberString)(HANDLE, PVOID, ULONG);
typedef BOOLEAN (WINAPI* PFN_HidD_GetPreparsedData)(HANDLE, PHIDP_PREPARSED_DATA*);
typedef BOOLEAN (WINAPI* PFN_HidD_FreePreparsedData)(PHIDP_PREPARSED_DATA);
typedef NTSTATUS (WINAPI* PFN_HidP_GetCaps)(PHIDP_PREPARSED_DATA, PHIDP_CAPS);

typedef struct _HID_API
{
	HMODULE hDll;
	PFN_HidD_GetHidGuid pGetHidGuid;
	PFN_HidD_GetAttributes pGetAttributes;
	PFN_HidD_GetManufacturerString pGetManufacturerString;
	PFN_HidD_GetProductString pGetProductString;
	PFN_HidD_GetSerialNumberString pGetSerialNumberString;
	PFN_HidD_GetPreparsedData pGetPreparsedData;
	PFN_HidD_FreePreparsedData pFreePreparsedData;
	PFN_HidP_GetCaps pGetCaps;
} HID_API;

static BOOL
HidLoadApi(HID_API* api)
{
	api->hDll = LoadLibraryW(L"hid.dll");
	if (!api->hDll)
		return FALSE;
	*(FARPROC*)&api->pGetHidGuid = GetProcAddress(api->hDll, "HidD_GetHidGuid");
	*(FARPROC*)&api->pGetAttributes = GetProcAddress(api->hDll, "HidD_GetAttributes");
	*(FARPROC*)&api->pGetManufacturerString = GetProcAddress(api->hDll, "HidD_GetManufacturerString");
	*(FARPROC*)&api->pGetProductString = GetProcAddress(api->hDll, "HidD_GetProductString");
	*(FARPROC*)&api->pGetSerialNumberString = GetProcAddress(api->hDll, "HidD_GetSerialNumberString");
	*(FARPROC*)&api->pGetPreparsedData = GetProcAddress(api->hDll, "HidD_GetPreparsedData");
	*(FARPROC*)&api->pFreePreparsedData = GetProcAddress(api->hDll, "HidD_FreePreparsedData");
	*(FARPROC*)&api->pGetCaps = GetProcAddress(api->hDll, "HidP_GetCaps");
	if (!api->pGetHidGuid || !api->pGetAttributes || !api->pGetPreparsedData ||
		!api->pFreePreparsedData || !api->pGetCaps)
	{
		FreeLibrary(api->hDll);
		api->hDll = NULL;
		return FALSE;
	}
	return TRUE;
}

static void
HidFreeApi(HID_API* api)
{
	if (api->hDll)
	{
		FreeLibrary(api->hDll);
		api->hDll = NULL;
	}
}

static LPCSTR
HidUsagePageToStr(USAGE usagePage)
{
	switch (usagePage)
	{
	case 0x01: return "Generic Desktop";
	case 0x02: return "Simulation";
	case 0x03: return "VR";
	case 0x04: return "Sport";
	case 0x05: return "Game";
	case 0x06: return "Generic Device";
	case 0x07: return "Keyboard/Keypad";
	case 0x08: return "LED";
	case 0x09: return "Button";
	case 0x0A: return "Ordinal";
	case 0x0B: return "Telephony";
	case 0x0C: return "Consumer";
	case 0x0D: return "Digitizer";
	case 0x0E: return "Haptics";
	case 0x0F: return "Physical Input";
	case 0x10: return "Unicode";
	case 0x12: return "Eye and Head Tracker";
	case 0x14: return "Auxiliary Display";
	case 0x20: return "Sensor";
	case 0x40: return "Medical Instrument";
	case 0x41: return "Braille Display";
	case 0x59: return "Lighting and Illumination";
	case 0x80: return "Monitor";
	case 0x81: return "Monitor Enumerated";
	case 0x82: return "VESA Virtual Controls";
	case 0x84: return "Power";
	case 0x85: return "Battery System";
	case 0x8C: return "Bar Code Scanner";
	case 0x8D: return "Scale";
	case 0x8E: return "Magnetic Stripe Reading";
	case 0x90: return "Camera Control";
	case 0x91: return "Arcade";
	case 0x92: return "Gaming Device";
	case 0xF1D0: return "FIDO Alliance";
	default: return NULL;
	}
}

static LPCSTR
HidUsageToStr(USAGE usagePage, USAGE usage)
{
	if (usagePage != 0x01)
		return NULL;
	switch (usage)
	{
	case 0x01: return "Pointer";
	case 0x02: return "Mouse";
	case 0x04: return "Joystick";
	case 0x05: return "Gamepad";
	case 0x06: return "Keyboard";
	case 0x07: return "Keypad";
	case 0x08: return "Multi-axis Controller";
	case 0x09: return "Tablet PC System Controls";
	case 0x0A: return "Water Cooling Device";
	case 0x0B: return "Computer Chassis Device";
	case 0x0C: return "Wireless Radio Controls";
	case 0x0D: return "Portable Device Control";
	case 0x0E: return "System Multi-axis Controller";
	case 0x0F: return "Spatial Controller";
	case 0x10: return "Assistive Control";
	case 0x11: return "Device Dock";
	case 0x12: return "Dockable Device";
	case 0x13: return "Call State Management Control";
	default: return NULL;
	}
}

static LPCSTR
HidRimTypeToStr(DWORD dwType)
{
	switch (dwType)
	{
	case RIM_TYPEMOUSE:    return "Mouse";
	case RIM_TYPEKEYBOARD: return "Keyboard";
	case RIM_TYPEHID:      return "HID";
	default:               return "Unknown";
	}
}

static void
HidGetDeviceInfo(PNODE nd, HID_API* api, HANDLE hDevice)
{
	PHIDP_PREPARSED_DATA preparsedData = NULL;
	HIDP_CAPS caps;
	HIDD_ATTRIBUTES attr;
	WCHAR wBuf[256];

	attr.Size = sizeof(HIDD_ATTRIBUTES);
	if (api->pGetAttributes(hDevice, &attr))
	{
		NWL_NodeAttrSetf(nd, "Vendor ID", 0, "%04X", attr.VendorID);
		NWL_NodeAttrSetf(nd, "Product ID", 0, "%04X", attr.ProductID);
		NWL_NodeAttrSetf(nd, "Version", 0, "%u.%u",
			(attr.VersionNumber >> 8) & 0xFF, attr.VersionNumber & 0xFF);
	}

	if (api->pGetManufacturerString &&
		api->pGetManufacturerString(hDevice, wBuf, sizeof(wBuf)) && wBuf[0])
		NWL_NodeAttrSet(nd, "Manufacturer", NWL_Ucs2ToUtf8(wBuf), 0);

	if (api->pGetProductString &&
		api->pGetProductString(hDevice, wBuf, sizeof(wBuf)) && wBuf[0])
		NWL_NodeAttrSet(nd, "Product", NWL_Ucs2ToUtf8(wBuf), 0);

	if (api->pGetSerialNumberString &&
		api->pGetSerialNumberString(hDevice, wBuf, sizeof(wBuf)) && wBuf[0])
		NWL_NodeAttrSet(nd, "Serial Number", NWL_Ucs2ToUtf8(wBuf), NAFLG_FMT_SENSITIVE);

	if (!api->pGetPreparsedData(hDevice, &preparsedData))
		return;

	if (api->pGetCaps(preparsedData, &caps) == HIDP_STATUS_SUCCESS)
	{
		LPCSTR pageStr = HidUsagePageToStr(caps.UsagePage);
		LPCSTR usageStr = HidUsageToStr(caps.UsagePage, caps.Usage);
		if (pageStr)
			NWL_NodeAttrSetf(nd, "Usage Page", 0, "%s (0x%02X)", pageStr, caps.UsagePage);
		else
			NWL_NodeAttrSetf(nd, "Usage Page", 0, "0x%04X", caps.UsagePage);
		if (usageStr)
			NWL_NodeAttrSetf(nd, "Usage", 0, "%s (0x%02X)", usageStr, caps.Usage);
		else
			NWL_NodeAttrSetf(nd, "Usage", 0, "0x%04X", caps.Usage);
		NWL_NodeAttrSetf(nd, "Input Report Length", NAFLG_FMT_NUMERIC, "%u", caps.InputReportByteLength);
		NWL_NodeAttrSetf(nd, "Output Report Length", NAFLG_FMT_NUMERIC, "%u", caps.OutputReportByteLength);
		NWL_NodeAttrSetf(nd, "Feature Report Length", NAFLG_FMT_NUMERIC, "%u", caps.FeatureReportByteLength);
		NWL_NodeAttrSetf(nd, "Number of Link Collection Nodes", NAFLG_FMT_NUMERIC, "%u", caps.NumberLinkCollectionNodes);
		NWL_NodeAttrSetf(nd, "Number of Input Button Caps", NAFLG_FMT_NUMERIC, "%u", caps.NumberInputButtonCaps);
		NWL_NodeAttrSetf(nd, "Number of Input Value Caps", NAFLG_FMT_NUMERIC, "%u", caps.NumberInputValueCaps);
		NWL_NodeAttrSetf(nd, "Number of Output Button Caps", NAFLG_FMT_NUMERIC, "%u", caps.NumberOutputButtonCaps);
		NWL_NodeAttrSetf(nd, "Number of Output Value Caps", NAFLG_FMT_NUMERIC, "%u", caps.NumberOutputValueCaps);
		NWL_NodeAttrSetf(nd, "Number of Feature Button Caps", NAFLG_FMT_NUMERIC, "%u", caps.NumberFeatureButtonCaps);
		NWL_NodeAttrSetf(nd, "Number of Feature Value Caps", NAFLG_FMT_NUMERIC, "%u", caps.NumberFeatureValueCaps);
	}

	api->pFreePreparsedData(preparsedData);
}

static void
HidEnumDevices(PNODE node, HID_API* api)
{
	UINT numDevices = 0;
	PRAWINPUTDEVICELIST pDeviceList = NULL;
	UINT i;

	if (GetRawInputDeviceList(NULL, &numDevices, sizeof(RAWINPUTDEVICELIST)) != 0)
		return;
	if (numDevices == 0)
		return;

	pDeviceList = (PRAWINPUTDEVICELIST)calloc(numDevices, sizeof(RAWINPUTDEVICELIST));
	if (!pDeviceList)
		return;

	if (GetRawInputDeviceList(pDeviceList, &numDevices, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1)
	{
		free(pDeviceList);
		return;
	}

	for (i = 0; i < numDevices; i++)
	{
		UINT nameSize = 0;
		WCHAR* deviceName = NULL;
		HANDLE hDevice = INVALID_HANDLE_VALUE;
		PNODE child = NWL_NodeAppendNew(node, "Device", NFLG_TABLE_ROW);

		NWL_NodeAttrSet(child, "Type", HidRimTypeToStr(pDeviceList[i].dwType), 0);

		if (GetRawInputDeviceInfoW(pDeviceList[i].hDevice, RIDI_DEVICENAME, NULL, &nameSize) != 0)
			continue;
		if (nameSize == 0)
			continue;

		deviceName = (WCHAR*)calloc(nameSize + 1, sizeof(WCHAR));
		if (!deviceName)
			continue;

		if (GetRawInputDeviceInfoW(pDeviceList[i].hDevice, RIDI_DEVICENAME, deviceName, &nameSize) == (UINT)-1)
		{
			free(deviceName);
			continue;
		}

		NWL_NodeAttrSet(child, "Device Path", NWL_Ucs2ToUtf8(deviceName), 0);

		hDevice = CreateFileW(deviceName, 0,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, 0, NULL);
		free(deviceName);

		if (hDevice == INVALID_HANDLE_VALUE)
			continue;

		HidGetDeviceInfo(child, api, hDevice);
		CloseHandle(hDevice);
	}

	free(pDeviceList);
}

PNODE NW_Hid(BOOL bAppend)
{
	HID_API api = { 0 };
	PNODE node = NWL_NodeAlloc("HID", NFLG_TABLE);
	if (bAppend)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	if (HidLoadApi(&api))
	{
		HidEnumDevices(node, &api);
		HidFreeApi(&api);
	}

	return node;
}
