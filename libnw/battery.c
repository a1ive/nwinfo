// SPDX-License-Identifier: Unlicense
#define INITGUID
#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <setupapi.h>
#include <batclass.h>
#include <devguid.h>
#include <powrprof.h>

#include "libnw.h"
#include "utils.h"

static DWORD
PwrGetActiveScheme(GUID** ActivePolicyGuid)
{
	DWORD ret = ERROR_SUCCESS;
	DWORD (WINAPI* NT6PowerGetActiveScheme)(HKEY, GUID **) = NULL;
	HMODULE hL = LoadLibraryW(L"powrprof.dll");
	if (!hL)
		goto fail;
	*(FARPROC*)&NT6PowerGetActiveScheme = GetProcAddress(hL, "PowerGetActiveScheme");
	if (!NT6PowerGetActiveScheme)
	{
		FreeLibrary(hL);
		goto fail;
	}

	ret = NT6PowerGetActiveScheme(NULL, ActivePolicyGuid);
	FreeLibrary(hL);
	return ret;

fail:
	LPWSTR lpSchemeName = NULL;
	DWORD dwSize;
	DWORD dwType;

	*ActivePolicyGuid = (GUID*)LocalAlloc(LPTR, sizeof(GUID));
	if (*ActivePolicyGuid == NULL)
		return ERROR_NOT_ENOUGH_MEMORY;

	lpSchemeName = NWL_NtGetRegValue(HKEY_LOCAL_MACHINE,
		L"SYSTEM\\CurrentControlSet\\Control\\Power\\User\\PowerSchemes", L"ActivePowerScheme", &dwSize, &dwType);
	if (!lpSchemeName)
	{
		LocalFree(*ActivePolicyGuid);
		return ERROR_FILE_NOT_FOUND;
	}
	if (dwType != REG_SZ || wcslen(lpSchemeName) != 36)
	{
		LocalFree(*ActivePolicyGuid);
		free(lpSchemeName);
		return ERROR_INVALID_DATA;
	}

	lpSchemeName[8] = 0;
	(*ActivePolicyGuid)->Data1 = (unsigned long)wcstoul(lpSchemeName, NULL, 16);
	lpSchemeName[13] = 0;
	(*ActivePolicyGuid)->Data2 = (unsigned short)wcstoul(lpSchemeName + 9, NULL, 16);
	lpSchemeName[18] = 0;
	(*ActivePolicyGuid)->Data3 = (unsigned short)wcstoul(lpSchemeName + 14, NULL, 16);
	(*ActivePolicyGuid)->Data4[7] = (unsigned char)wcstoul(lpSchemeName + 34, NULL, 16);
	lpSchemeName[34] = 0;
	(*ActivePolicyGuid)->Data4[6] = (unsigned char)wcstoul(lpSchemeName + 32, NULL, 16);
	lpSchemeName[32] = 0;
	(*ActivePolicyGuid)->Data4[5] = (unsigned char)wcstoul(lpSchemeName + 30, NULL, 16);
	lpSchemeName[30] = 0;
	(*ActivePolicyGuid)->Data4[4] = (unsigned char)wcstoul(lpSchemeName + 28, NULL, 16);
	lpSchemeName[28] = 0;
	(*ActivePolicyGuid)->Data4[3] = (unsigned char)wcstoul(lpSchemeName + 26, NULL, 16);
	lpSchemeName[26] = 0;
	(*ActivePolicyGuid)->Data4[2] = (unsigned char)wcstoul(lpSchemeName + 24, NULL, 16);
	lpSchemeName[23] = 0;
	(*ActivePolicyGuid)->Data4[1] = (unsigned char)wcstoul(lpSchemeName + 21, NULL, 16);
	lpSchemeName[21] = 0;
	(*ActivePolicyGuid)->Data4[0] = (unsigned char)wcstoul(lpSchemeName + 19, NULL, 16);

	free(lpSchemeName);
	return ERROR_SUCCESS;
}

static DWORD
PwrReadFriendlyName(const GUID* SchemeGuid, const GUID* SubGroupOfPowerSettingsGuid,
	const GUID* PowerSettingGuid, PUCHAR Buffer, LPDWORD BufferSize)
{
	DWORD(WINAPI * Nt6PowerReadName)(HKEY, const GUID*, const GUID*, const GUID*, PUCHAR, LPDWORD) = NULL;
	HMODULE hL = LoadLibraryW(L"powrprof.dll");
	if (!hL)
		return ERROR_FILE_NOT_FOUND;
	*(FARPROC*)&Nt6PowerReadName = GetProcAddress(hL, "PowerReadFriendlyName");
	if (!Nt6PowerReadName)
	{
		FreeLibrary(hL);
		return ERROR_PROC_NOT_FOUND;
	}
	DWORD ret = Nt6PowerReadName(NULL, SchemeGuid, SubGroupOfPowerSettingsGuid, PowerSettingGuid, Buffer, BufferSize);
	FreeLibrary(hL);
	return ret;
}

static DWORD
PwrEnumerate(const GUID* SchemeGuid, const GUID* SubGroupOfPowerSettingsGuid,
	POWER_DATA_ACCESSOR AccessFlags, ULONG Index, UCHAR* Buffer, DWORD* BufferSize)
{
	DWORD(WINAPI * Nt6PowerEnumerate)(HKEY, const GUID*, const GUID*, POWER_DATA_ACCESSOR, ULONG, UCHAR*, DWORD*) = NULL;
	HMODULE hL = LoadLibraryW(L"powrprof.dll");
	if (!hL)
		return ERROR_FILE_NOT_FOUND;
	*(FARPROC*)&Nt6PowerEnumerate = GetProcAddress(hL, "PowerEnumerate");
	if (!Nt6PowerEnumerate)
	{
		FreeLibrary(hL);
		return ERROR_PROC_NOT_FOUND;
	}
	DWORD ret = Nt6PowerEnumerate(NULL, SchemeGuid, SubGroupOfPowerSettingsGuid, AccessFlags, Index, Buffer, BufferSize);
	FreeLibrary(hL);
	return ret;
}

#ifdef NWL_SYS_BUTTON_ACTION
static DWORD
PwrReadValue(BOOL Ac, const GUID* SchemeGuid, const GUID* SubGroupOfPowerSettingsGuid,
	const GUID* PowerSettingGuid, PULONG Type, LPBYTE Buffer, LPDWORD BufferSize)
{
	DWORD(WINAPI * Nt6PowerReadValue)(HKEY, const GUID*, const GUID*, const GUID*, PULONG, LPBYTE, LPDWORD) = NULL;
	HMODULE hL = LoadLibraryW(L"powrprof.dll");
	LPCSTR fnName = Ac ? "PowerReadACValue" : "PowerReadDCValue";
	if (!hL)
		return ERROR_FILE_NOT_FOUND;
	*(FARPROC*)&Nt6PowerReadValue = GetProcAddress(hL, fnName);
	if (!Nt6PowerReadValue)
	{
		FreeLibrary(hL);
		return ERROR_PROC_NOT_FOUND;
	}
	DWORD ret = Nt6PowerReadValue(NULL, SchemeGuid, SubGroupOfPowerSettingsGuid, PowerSettingGuid, Type, Buffer, BufferSize);
	FreeLibrary(hL);
	return ret;
}

static PVOID
PwrReadValueAlloc(BOOL Ac, const GUID* SchemeGuid, const GUID* SubGroupOfPowerSettingsGuid,
	const GUID* PowerSettingGuid, PULONG Type, LPDWORD BufferSize)
{
	PVOID pBuffer = NULL;
	DWORD dwSize = 0;
	if (PwrReadValue(Ac, SchemeGuid, SubGroupOfPowerSettingsGuid, PowerSettingGuid, Type, NULL, &dwSize) != ERROR_SUCCESS)
		return NULL;
	if (dwSize == 0)
		return NULL;
	pBuffer = malloc(dwSize);
	if (!pBuffer)
		return NULL;
	if (PwrReadValue(Ac, SchemeGuid, SubGroupOfPowerSettingsGuid, PowerSettingGuid, Type, (LPBYTE)pBuffer, &dwSize) != ERROR_SUCCESS)
	{
		free(pBuffer);
		return NULL;
	}
	if (BufferSize)
		*BufferSize = dwSize;
	return pBuffer;
}
#endif

static DWORD
PwrReadValueIndex(BOOL Ac, const GUID* SchemeGuid, const GUID* SubGroupOfPowerSettingsGuid,
	const GUID* PowerSettingGuid, LPDWORD ValueIndex)
{
	DWORD(WINAPI * Nt6PowerReadValueIndex)(HKEY, const GUID*, const GUID*, const GUID*, LPDWORD) = NULL;
	HMODULE hL = LoadLibraryW(L"powrprof.dll");
	LPCSTR fnName = Ac ? "PowerReadACValueIndex" : "PowerReadDCValueIndex";
	if (!hL)
		return ERROR_FILE_NOT_FOUND;
	*(FARPROC*)&Nt6PowerReadValueIndex = GetProcAddress(hL, fnName);
	if (!Nt6PowerReadValueIndex)
	{
		FreeLibrary(hL);
		return ERROR_PROC_NOT_FOUND;
	}
	DWORD ret = Nt6PowerReadValueIndex(NULL, SchemeGuid, SubGroupOfPowerSettingsGuid, PowerSettingGuid, ValueIndex);
	FreeLibrary(hL);
	return ret;
}

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
		NWL_NodeAttrSet(pb, "Serial Number", NWL_Ucs2ToUtf8(wName), NAFLG_FMT_SENSITIVE);
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

#ifdef NWL_SYS_BUTTON_ACTION
static LPCSTR
GetPowerActionStr(DWORD a)
{
	switch (a)
	{
	case POWERBUTTON_ACTION_VALUE_NOTHING:
		return "Nothing";
	case POWERBUTTON_ACTION_VALUE_SLEEP:
		return "Sleep";
	case POWERBUTTON_ACTION_VALUE_HIBERNATE:
		return "Hibernate";
	case POWERBUTTON_ACTION_VALUE_SHUTDOWN:
		return "Shutdown";
	case POWERBUTTON_ACTION_VALUE_TURN_OFF_THE_DISPLAY:
		return "Display Off";
	default:
		return "Unknown";
	}
}
#endif

static VOID
PrintSchemeInfo(PNODE node, GUID* guid, BOOL ac)
{
	DWORD dwValue;
	PNODE ni = NWL_NodeAppendNew(node, ac ? "AC Power Settings" : "DC Power Settings", NFLG_ATTGROUP);
	if (PwrReadValueIndex(ac, guid, &GUID_DISK_SUBGROUP, &GUID_DISK_POWERDOWN_TIMEOUT, &dwValue) == ERROR_SUCCESS)
		NWL_NodeAttrSet(ni, "Disk Poweroff Timeout", NWL_GetHumanTime(dwValue), 0);
	if (PwrReadValueIndex(ac, guid, &GUID_VIDEO_SUBGROUP, &GUID_VIDEO_POWERDOWN_TIMEOUT, &dwValue) == ERROR_SUCCESS)
		NWL_NodeAttrSet(ni, "Screen Timeout", NWL_GetHumanTime(dwValue), 0);
	if (PwrReadValueIndex(ac, guid, &GUID_VIDEO_SUBGROUP, &GUID_VIDEO_DIM_TIMEOUT, &dwValue) == ERROR_SUCCESS)
		NWL_NodeAttrSet(ni, "Dim Timeout", NWL_GetHumanTime(dwValue), 0);
	if (PwrReadValueIndex(ac, guid, &GUID_SLEEP_SUBGROUP, &GUID_STANDBY_TIMEOUT, &dwValue) == ERROR_SUCCESS)
		NWL_NodeAttrSet(ni, "Standby Timeout", NWL_GetHumanTime(dwValue), 0);
	if (PwrReadValueIndex(ac, guid, &GUID_SLEEP_SUBGROUP, &GUID_UNATTEND_SLEEP_TIMEOUT, &dwValue) == ERROR_SUCCESS)
		NWL_NodeAttrSet(ni, "Unattend Sleep Timeout", NWL_GetHumanTime(dwValue), 0);
	if (PwrReadValueIndex(ac, guid, &GUID_SLEEP_SUBGROUP, &GUID_HIBERNATE_TIMEOUT, &dwValue) == ERROR_SUCCESS)
		NWL_NodeAttrSet(ni, "Hibernate Timeout", NWL_GetHumanTime(dwValue), 0);
#ifdef NWL_SYS_BUTTON_ACTION
	DWORD dwSize = sizeof(DWORD);
	if (PwrReadValue(ac, guid, &GUID_SYSTEM_BUTTON_SUBGROUP, &GUID_POWERBUTTON_ACTION, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
		NWL_NodeAttrSet(ni, "Power Button Action", GetPowerActionStr(dwValue), 0);
	dwSize = sizeof(DWORD);
	if (PwrReadValue(ac, guid, &GUID_SYSTEM_BUTTON_SUBGROUP, &GUID_SLEEPBUTTON_ACTION, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
		NWL_NodeAttrSet(ni, "Sleep Button Action", GetPowerActionStr(dwValue), 0);
#endif
	if (PwrReadValueIndex(ac, guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &GUID_PROCESSOR_THROTTLE_MINIMUM, &dwValue) == ERROR_SUCCESS)
		NWL_NodeAttrSetf(ni, "Processor Throttle Min", NAFLG_FMT_NUMERIC, "%lu", dwValue);
	if (PwrReadValueIndex(ac, guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &GUID_PROCESSOR_THROTTLE_MAXIMUM, &dwValue) == ERROR_SUCCESS)
		NWL_NodeAttrSetf(ni, "Processor Throttle Max", NAFLG_FMT_NUMERIC, "%lu", dwValue);
}

#define MAX_POWER_SCHEMES 64

static VOID
PrintPowerScheme(PNODE node)
{
	GUID* activeSchemeGuid = NULL;

	if (PwrGetActiveScheme(&activeSchemeGuid) == ERROR_SUCCESS && activeSchemeGuid)
	{
		DWORD dwSize = NWINFO_BUFSZB;
		NWL_NodeAttrSet(node, "Active Power Scheme", NWL_WinGuidToStr(TRUE, activeSchemeGuid), NAFLG_FMT_GUID);
		if (PwrReadFriendlyName(activeSchemeGuid, NULL, NULL, (PUCHAR)NWLC->NwBufW, &dwSize) == ERROR_SUCCESS)
			NWL_NodeAttrSet(node, "Active Power Scheme Name", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);
	}

	PNODE nps = NWL_NodeAppendNew(node, "Power Schemes", NFLG_TABLE);
	for (ULONG index = 0; index < MAX_POWER_SCHEMES; ++index)
	{
		GUID curGuid;
		DWORD guidSize = sizeof(GUID);

		if (PwrEnumerate(NULL, NULL, ACCESS_SCHEME, index, (UCHAR*)&curGuid, &guidSize) == ERROR_SUCCESS)
		{
			DWORD dwSize = NWINFO_BUFSZB;
			PNODE nc = NWL_NodeAppendNew(nps, "Power Scheme", NFLG_TABLE_ROW);
			NWL_NodeAttrSet(nc, "GUID", NWL_WinGuidToStr(TRUE, &curGuid), NAFLG_FMT_GUID);
			NWL_NodeAttrSetBool(nc, "Active", (activeSchemeGuid && IsEqualGUID(&curGuid, activeSchemeGuid)), 0);
			if (PwrReadFriendlyName(&curGuid, NULL, NULL, (PUCHAR)NWLC->NwBufW, &dwSize) == ERROR_SUCCESS)
				NWL_NodeAttrSet(nc, "Name", NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);
			PrintSchemeInfo(nc, &curGuid, TRUE); // AC Power Settings
			PrintSchemeInfo(nc, &curGuid, FALSE); // DC Power Settings
		}
		else
		{
			// ERROR_NO_MORE_ITEMS is the expected end of enumeration
			break;
		}
	}

	if (activeSchemeGuid)
		LocalFree(activeSchemeGuid);
}

PNODE NW_Battery(VOID)
{
	PNODE node = NWL_NodeAlloc("Battery", 0);
	if (NWLC->BatteryInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	PrintPowerScheme(node);
	if (!PrintPowerInfo(node))
		goto fail;
	PrintBatteryState(node);
fail:
	return node;
}
