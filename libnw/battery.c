// SPDX-License-Identifier: Unlicense
#define INITGUID
#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <setupapi.h>
#include <batclass.h>
#include <devguid.h>

#include "libnw.h"
#include "utils.h"

static void
PrintBatteryTime(PNODE node, LPCSTR key, DWORD value)
{
	if (value != -1)
	{
		UINT32 Hours = value / 3600U;
		UINT32 Minutes = value / 60ULL - Hours * 60ULL;
		UINT32 Seconds = value - Hours * 3600ULL - Minutes * 60ULL;
		NWL_NodeAttrSetf(node, key, 0, "%luh %lum %lus", Hours, Minutes, Seconds);
	}
	else
		NWL_NodeAttrSet(node, key, "UNKNOWN", 0);
}

static BOOL
PrintPowerInfo(PNODE node)
{
	SYSTEM_POWER_STATUS Power;
	if (!GetSystemPowerStatus(&Power))
	{
		Power.ACLineStatus = 255;
		Power.BatteryFlag = 255;
	}

	switch (Power.ACLineStatus)
	{
	case 0:
		NWL_NodeAttrSet(node, "AC Power", "Offline", 0);
		break;
	case 1:
		NWL_NodeAttrSet(node, "AC Power", "Online", 0);
		break;
	default:
		NWL_NodeAttrSet(node, "AC Power", "UNKNOWN", 0);
	}

	if (Power.BatteryFlag == 255U)
	{
		NWL_NodeAttrSet(node, "Battery Status", "UNKNOWN", 0);
		return FALSE;
	}
	else if (Power.BatteryFlag & 128U)
	{
		NWL_NodeAttrSet(node, "Battery Status", "NO BATTERY", 0);
		return FALSE;
	}
	else if (Power.BatteryFlag & 8U)
		NWL_NodeAttrSet(node, "Battery Status", "Charging", 0);
	else
		NWL_NodeAttrSet(node, "Battery Status", "Not Charging", 0);

	if (Power.BatteryLifePercent <= 100U)
		NWL_NodeAttrSetf(node, "Battery Life Percentage", 0, "%u%%", Power.BatteryLifePercent);
	else
		NWL_NodeAttrSet(node, "Battery Life Percentage", "UNKNOWN", 0);

	PrintBatteryTime(node, "Battery Life Remaining", Power.BatteryLifeTime);
	PrintBatteryTime(node, "Battery Life Full", Power.BatteryFullLifeTime);

	return TRUE;
}

static BOOL
PrintBatteryInfo(PNODE pb, HANDLE* hb, BATTERY_QUERY_INFORMATION* pbqi)
{
	DWORD dwOut;
	BOOL bRelative = FALSE;
	BATTERY_INFORMATION bi = { 0 };
	pbqi->InformationLevel = BatteryInformation;
	if (!DeviceIoControl(hb, IOCTL_BATTERY_QUERY_INFORMATION, pbqi, sizeof(BATTERY_QUERY_INFORMATION),
		&bi, sizeof(bi), &dwOut, NULL))
		return FALSE;
	NWL_NodeAttrSetf(pb, "Battery Chemistry", 0, "%c%c%c%c",
		bi.Chemistry[0], bi.Chemistry[1], bi.Chemistry[2], bi.Chemistry[3]);
	if (bi.Capabilities & BATTERY_CAPACITY_RELATIVE)
	{
		bRelative = TRUE;
		NWL_NodeAttrSetf(pb, "Designed Capacity", 0, "%lu", bi.DesignedCapacity);
		NWL_NodeAttrSetf(pb, "Full Charged Capacity", 0, "%lu", bi.FullChargedCapacity);
	}
	else
	{
		NWL_NodeAttrSetf(pb, "Designed Capacity", 0, "%lu mWh", bi.DesignedCapacity);
		NWL_NodeAttrSetf(pb, "Full Charged Capacity", 0, "%lu mWh", bi.FullChargedCapacity);
	}
	NWL_NodeAttrSetf(pb, "Charge cycles", NAFLG_FMT_NUMERIC, "%lu", bi.CycleCount);
	return bRelative;
}

static void
PrintBatteryName(PNODE pb, HANDLE* hb, BATTERY_QUERY_INFORMATION* pbqi)
{
	DWORD dwOut;
	WCHAR wName[64];
	pbqi->InformationLevel = BatteryDeviceName;
	if (DeviceIoControl(hb, IOCTL_BATTERY_QUERY_INFORMATION, pbqi, sizeof(BATTERY_QUERY_INFORMATION),
		wName, sizeof(wName), &dwOut, NULL))
		NWL_NodeAttrSet(pb, "Name", NWL_Ucs2ToUtf8(wName), 0);
	pbqi->InformationLevel = BatteryUniqueID;
	if (DeviceIoControl(hb, IOCTL_BATTERY_QUERY_INFORMATION, pbqi, sizeof(BATTERY_QUERY_INFORMATION),
		wName, sizeof(wName), &dwOut, NULL))
		NWL_NodeAttrSet(pb, "ID", NWL_Ucs2ToUtf8(wName), 0);
}

static void
PrintBatteryEstimatedTime(PNODE pb, HANDLE* hb, BATTERY_QUERY_INFORMATION* pbqi)
{
	DWORD dwOut;
	ULONG ulEstimatedTime = 0;
	pbqi->InformationLevel = BatteryEstimatedTime;
	if (!DeviceIoControl(hb, IOCTL_BATTERY_QUERY_INFORMATION, pbqi, sizeof(BATTERY_QUERY_INFORMATION),
		&ulEstimatedTime, sizeof(ulEstimatedTime), &dwOut, NULL))
		return;
	PrintBatteryTime(pb, "Estimated Run Time", ulEstimatedTime);
}

static void
PrintBatteryManufacture(PNODE pb, HANDLE* hb, BATTERY_QUERY_INFORMATION* pbqi)
{
	DWORD dwOut;
	WCHAR wName[64];
	BATTERY_MANUFACTURE_DATE bmDate = { 0 };
	pbqi->InformationLevel = BatteryManufactureName;
	if (DeviceIoControl(hb, IOCTL_BATTERY_QUERY_INFORMATION, pbqi, sizeof(BATTERY_QUERY_INFORMATION),
		wName, sizeof(wName), &dwOut, NULL))
		NWL_NodeAttrSet(pb, "Manufacturer", NWL_Ucs2ToUtf8(wName), 0);
	pbqi->InformationLevel = BatteryManufactureDate;
	if (DeviceIoControl(hb, IOCTL_BATTERY_QUERY_INFORMATION, pbqi, sizeof(BATTERY_QUERY_INFORMATION),
		&bmDate, sizeof(bmDate), &dwOut, NULL))
		NWL_NodeAttrSetf(pb, "Date", 0, "%u-%02u-%02u", bmDate.Year, bmDate.Month, bmDate.Day);
	pbqi->InformationLevel = BatterySerialNumber;
	if (DeviceIoControl(hb, IOCTL_BATTERY_QUERY_INFORMATION, pbqi, sizeof(BATTERY_QUERY_INFORMATION),
		wName, sizeof(wName), &dwOut, NULL))
		NWL_NodeAttrSet(pb, "Serial Number", NWL_Ucs2ToUtf8(wName), 0);
}

static void
PrintBatteryTemperature(PNODE pb, HANDLE* hb, BATTERY_QUERY_INFORMATION* pbqi)
{
	DWORD dwOut;
	ULONG ulTemperature = 0;
	pbqi->InformationLevel = BatteryTemperature;
	if (!DeviceIoControl(hb, IOCTL_BATTERY_QUERY_INFORMATION, pbqi, sizeof(BATTERY_QUERY_INFORMATION),
		&ulTemperature, sizeof(ulTemperature), &dwOut, NULL))
		return;
	NWL_NodeAttrSetf(pb, "Temperature", 0, "%lu K", ulTemperature / 10);
}

static void
PrintBatteryPower(PNODE pb, HANDLE* hb, BATTERY_QUERY_INFORMATION* pbqi, BOOL bcr)
{
	DWORD dwOut;
	BATTERY_WAIT_STATUS bws = { 0 };
	BATTERY_STATUS bs;
	bws.BatteryTag = pbqi->BatteryTag;
	if (!DeviceIoControl(hb, IOCTL_BATTERY_QUERY_STATUS, &bws, sizeof(bws),
		&bs, sizeof(bs), &dwOut, NULL))
		return;
	if (bs.Capacity == BATTERY_UNKNOWN_CAPACITY)
		NWL_NodeAttrSet(pb, "Capacity", "UNKNOWN", 0);
	else
		NWL_NodeAttrSetf(pb, "Capacity", 0, "%lu mWh", bs.Capacity);
	if (bs.Voltage == BATTERY_UNKNOWN_VOLTAGE)
		NWL_NodeAttrSet(pb, "Voltage", "UNKNOWN", 0);
	else
		NWL_NodeAttrSetf(pb, "Voltage", 0, "%lu mV", bs.Voltage);
	if (bs.Rate == BATTERY_UNKNOWN_RATE)
		NWL_NodeAttrSet(pb, "Charge/Discharge Rate", "UNKNOWN", 0);
	else
		NWL_NodeAttrSetf(pb, "Charge/Discharge Rate", 0, "%ld%s", bs.Rate, bcr ? "" : " mW");
}

static void
PrintBatteryState(PNODE node)
{
	DWORD i;
	HDEVINFO hdev = SetupDiGetClassDevsW(&GUID_DEVCLASS_BATTERY, 0, 0,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	SP_DEVICE_INTERFACE_DATA did = { .cbSize = sizeof(SP_DEVINFO_DATA) };
	if (hdev == INVALID_HANDLE_VALUE)
		return;
	for (i = 0; i < 32 && SetupDiEnumDeviceInterfaces(hdev, 0, &GUID_DEVCLASS_BATTERY, i, &did); i++)
	{
		CHAR batName[] = "BAT32";
		BATTERY_QUERY_INFORMATION bqi = { 0 };
		DWORD dwWait = 0;
		DWORD dwOut;
		DWORD cbRequired = 0;
		HANDLE hBattery = INVALID_HANDLE_VALUE;
		PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd = NULL;
		snprintf(batName, sizeof(batName), "BAT%u", i);
		SetupDiGetDeviceInterfaceDetailW(hdev, &did, 0, 0, &cbRequired, 0);
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			goto fail;
		pdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, cbRequired);
		if (!pdidd)
			goto fail;
		pdidd->cbSize = sizeof(*pdidd);
		if (!SetupDiGetDeviceInterfaceDetailW(hdev, &did, pdidd, cbRequired, &cbRequired, 0))
			goto fail;
		hBattery = CreateFileW(pdidd->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hBattery == INVALID_HANDLE_VALUE)
			goto fail;
		if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_TAG, &dwWait, sizeof(dwWait),
			&bqi.BatteryTag, sizeof(bqi.BatteryTag), &dwOut, NULL))
		{
			BOOL bcr = FALSE;
			PNODE pb = NWL_NodeAppendNew(node, batName, 0);
			NWL_NodeAttrSet(pb, "Path", NWL_Ucs2ToUtf8(pdidd->DevicePath), 0);
			//NWL_NodeAttrSetf(pb, "Battery Tag", NAFLG_FMT_NUMERIC, "%lu", bqi.BatteryTag);
			PrintBatteryName(pb, hBattery, &bqi);
			bcr = PrintBatteryInfo(pb, hBattery, &bqi);
			PrintBatteryEstimatedTime(pb, hBattery, &bqi);
			PrintBatteryTemperature(pb, hBattery, &bqi);
			PrintBatteryManufacture(pb, hBattery, &bqi);
			PrintBatteryPower(pb, hBattery, &bqi, bcr);
		}
		CloseHandle(hBattery);
	fail:
		if (pdidd)
			LocalFree(pdidd);
	}
	SetupDiDestroyDeviceInfoList(hdev);
}

PNODE NW_Battery(VOID)
{
	PNODE node = NWL_NodeAlloc("Battery", 0);
	if (NWLC->BatteryInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	if (!PrintPowerInfo(node))
		goto fail;
	PrintBatteryState(node);
fail:
	return node;
}
