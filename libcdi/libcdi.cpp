// SPDX-License-Identifier: MIT

#include "stdafx.h"
#include "AtaSmart.h"
#include "NVMeInterpreter.h"
#include "smartids.h"

#define LIBCDI_IMPLEMENTATION
#include "libcdi.h"

#include "UtilityFx.h"

#include "version.h"

extern "C" CONST CHAR* WINAPI
cdi_get_version()
{
	return QUOTE(LIBCDI_MAJOR_VERSION.LIBCDI_MINOR_VERSION.LIBCDI_MICRO_VERSION);
}

#define DEBUG_MODE_NONE		0U
#define DEBUG_MODE_LOG		1U
#define DEBUG_MODE_MESSAGE	2U

#define DEBUG_MODE DEBUG_MODE_NONE

extern "C" CDI_SMART* WINAPI
cdi_create_smart()
{
	SetDebugMode(DEBUG_MODE);
	//(void)CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	return new CDI_SMART;
}

extern "C" VOID WINAPI
cdi_destroy_smart(CDI_SMART * ptr)
{
	delete ptr;
	//CoUninitialize();
}

inline BOOL
check_flag(UINT64 flags, UINT64 mask)
{
	return (flags & mask) ? TRUE : FALSE;
}

extern "C" VOID WINAPI
cdi_init_smart(CDI_SMART * ptr, UINT64 flags)
{
	BOOL change_disk = TRUE;

	ptr->FlagNoWakeUp = check_flag(flags, CDI_FLAG_NO_WAKEUP);
	ptr->SetAtaPassThroughSmart(check_flag(flags, CDI_FLAG_ATA_PASS_THROUGH));

	ptr->FlagNvidiaController = check_flag(flags, CDI_FLAG_ENABLE_NVIDIA);
	ptr->FlagMarvellController = check_flag(flags, CDI_FLAG_ENABLE_MARVELL);
	ptr->FlagUsbSat = check_flag(flags, CDI_FLAG_ENABLE_USB_SAT);
	ptr->FlagUsbSunplus = check_flag(flags, CDI_FLAG_ENABLE_USB_SUNPLUS);
	ptr->FlagUsbIodata = check_flag(flags, CDI_FLAG_ENABLE_USB_IODATA);
	ptr->FlagUsbLogitec = check_flag(flags, CDI_FLAG_ENABLE_USB_LOGITEC);
	ptr->FlagUsbProlific = check_flag(flags, CDI_FLAG_ENABLE_USB_PROLIFIC);
	ptr->FlagUsbJmicron = check_flag(flags, CDI_FLAG_ENABLE_USB_JMICRON);
	ptr->FlagUsbCypress = check_flag(flags, CDI_FLAG_ENABLE_USB_CYPRESS);
	ptr->FlagUsbMemory = check_flag(flags, CDI_FLAG_ENABLE_USB_MEMORY);
	ptr->FlagUsbNVMeJMicron = check_flag(flags, CDI_FLAG_ENABLE_NVME_JMICRON);
	ptr->FlagUsbNVMeASMedia = check_flag(flags, CDI_FLAG_ENABLE_NVME_ASMEDIA);
	ptr->FlagUsbNVMeRealtek = check_flag(flags, CDI_FLAG_ENABLE_NVME_REALTEK);
	ptr->FlagMegaRAID = check_flag(flags, CDI_FLAG_ENABLE_MEGA_RAID);
	ptr->FlagIntelVROC = check_flag(flags, CDI_FLAG_ENABLE_INTEL_VROC);
	ptr->FlagUsbASM1352R = check_flag(flags, CDI_FLAG_ENABLE_ASM1352R);
	ptr->FlagAMD_RC2 = check_flag(flags, CDI_FLAG_ENABLE_AMD_RC2);
	ptr->FlagUsbRealtek9220DP = check_flag(flags, CDI_FLAG_ENABLE_REALTEK_9220DP);
	ptr->CsmiType = (flags >> CDI_CSMI_SHIFT) & 0x07;
	if (ptr->CsmiType >= CAtaSmart::CSMI_TYPE_ENABLE_ALL)
		ptr->CsmiType = CAtaSmart::CSMI_TYPE_ENABLE_AUTO;

	ptr->Init(check_flag(flags, CDI_FLAG_USE_WMI),
		check_flag(flags, CDI_FLAG_ADVANCED_SEARCH),
		&change_disk,
		check_flag(flags, CDI_FLAG_WORKAROUND_HD204UI),
		check_flag(flags, CDI_FLAG_WORKAROUND_ADATA),
		check_flag(flags, CDI_FLAG_HIDE_NO_SMART),
		check_flag(flags, CDI_FLAG_SORT_DRIVE_LETTER),
		check_flag(flags, CDI_FLAG_HIDE_RAID_VOLUME));
	for (INT i = 0; i < ptr->vars.GetCount(); i++)
	{
		if (ptr->vars[i].IsSsd)
			ptr->vars[i].AlarmTemperature = 60;
		else
			ptr->vars[i].AlarmTemperature = 50;
		ptr->vars[i].AlarmHealthStatus = 1;

		ptr->vars[i].Threshold05 = 1;
		ptr->vars[i].ThresholdC5 = 1;
		ptr->vars[i].ThresholdC6 = 1;
		ptr->vars[i].ThresholdFF = 10;

		ptr->vars[i].DiskStatus = ptr->CheckDiskStatus(i);
	}
}

extern "C" DWORD WINAPI
cdi_update_smart(CDI_SMART * ptr, INT index)
{
	return ptr->UpdateSmartInfo(index);
}

inline WCHAR* cs_to_wcs(const CString& str)
{
	size_t len = str.GetLength() + 1;
	auto ptr = (WCHAR*)CoTaskMemAlloc(len * sizeof(WCHAR));
	if (ptr == nullptr)
		AfxThrowMemoryException();
	wcscpy_s(ptr, len, str.GetString());
	return ptr;
}

extern "C" INT WINAPI
cdi_get_disk_count(CDI_SMART * ptr)
{
	return (INT)ptr->vars.GetCount();
}

extern "C" BOOL WINAPI
cdi_get_bool(CDI_SMART * ptr, INT index, enum CDI_ATA_BOOL attr)
{
	switch (attr)
	{
	case CDI_BOOL_SSD:
		return ptr->vars[index].IsSsd;
	case CDI_BOOL_SSD_NVME:
		return (ptr->vars[index].DiskVendorId == CAtaSmart::SSD_VENDOR_NVME) ? TRUE : FALSE;
	case CDI_BOOL_SMART:
		return ptr->vars[index].IsSmartSupported;
	case CDI_BOOL_LBA48:
		return ptr->vars[index].IsLba48Supported;
	case CDI_BOOL_AAM:
		return ptr->vars[index].IsAamSupported;
	case CDI_BOOL_APM:
		return ptr->vars[index].IsApmSupported;
	case CDI_BOOL_NCQ:
		return ptr->vars[index].IsNcqSupported;
	case CDI_BOOL_NV_CACHE:
		return ptr->vars[index].IsNvCacheSupported;
	case CDI_BOOL_DEVSLP:
		return ptr->vars[index].IsDeviceSleepSupported;
	case CDI_BOOL_STREAMING:
		return ptr->vars[index].IsStreamingSupported;
	case CDI_BOOL_GPL:
		return ptr->vars[index].IsGplSupported;
	case CDI_BOOL_TRIM:
		return ptr->vars[index].IsTrimSupported;
	case CDI_BOOL_VOLATILE_WRITE_CACHE:
		return ptr->vars[index].IsVolatileWriteCachePresent;
	case CDI_BOOL_SMART_ENABLED:
		return ptr->vars[index].IsSmartEnabled;
	case CDI_BOOL_AAM_ENABLED:
		return ptr->vars[index].IsAamEnabled;
	case CDI_BOOL_APM_ENABLED:
		return ptr->vars[index].IsApmEnabled;
	}
	return FALSE;
}

extern "C" INT WINAPI
cdi_get_int(CDI_SMART * ptr, INT index, enum CDI_ATA_INT attr)
{
	switch (attr)
	{
	case CDI_INT_DISK_ID:
		return ptr->vars[index].PhysicalDriveId;
	case CDI_INT_DISK_STATUS:
		return (INT)ptr->vars[index].DiskStatus;
	case CDI_INT_SCSI_PORT:
		return ptr->vars[index].ScsiPort;
	case CDI_INT_SCSI_TARGET_ID:
		return ptr->vars[index].ScsiTargetId;
	case CDI_INT_SCSI_BUS:
		return ptr->vars[index].ScsiBus;
	case CDI_INT_POWER_ON_HOURS:
	{
		INT div = 1;
		INT num = ptr->vars[index].MeasuredPowerOnHours;
		if (num > 0)
		{
			if (ptr->vars[index].MeasuredTimeUnitType == CAtaSmart::POWER_ON_MINUTES && ptr->vars[index].IsMaxtorMinute)
				div = 60;
		}
		else
		{
			num = ptr->vars[index].DetectedPowerOnHours;
			if (num > 0 && ptr->vars[index].DetectedTimeUnitType == CAtaSmart::POWER_ON_MINUTES && ptr->vars[index].IsMaxtorMinute)
				div = 60;
		}
		return num / div;
	}
	case CDI_INT_TEMPERATURE:
		return ptr->vars[index].Temperature;
	case CDI_INT_TEMPERATURE_ALARM:
		return ptr->vars[index].AlarmTemperature;
	case CDI_INT_HOST_WRITES:
		return ptr->vars[index].HostWrites;
	case CDI_INT_HOST_READS:
		return ptr->vars[index].HostReads;
	case CDI_INT_NAND_WRITES:
		return ptr->vars[index].NandWrites;
	case CDI_INT_GB_ERASED:
		return ptr->vars[index].GBytesErased;
	case CDI_INT_WEAR_LEVELING_COUNT:
		return ptr->vars[index].WearLevelingCount;
	case CDI_INT_LIFE:
		return ptr->vars[index].Life;
	case CDI_INT_MAX_ATTRIBUTE:
		return CAtaSmart::MAX_ATTRIBUTE;
	case CDI_INT_INTERFACE_TYPE:
	{
		switch (ptr->vars[index].InterfaceType)
		{
		case CAtaSmart::INTERFACE_TYPE_PATA:
			return BusTypeAta;
		case CAtaSmart::INTERFACE_TYPE_SATA:
			return BusTypeSata;
		case CAtaSmart::INTERFACE_TYPE_USB:
			return BusTypeUsb;
		case CAtaSmart::INTERFACE_TYPE_IEEE1394:
			return BusType1394;
		case CAtaSmart::INTERFACE_TYPE_SCSI:
			return BusTypeScsi;
		case CAtaSmart::INTERFACE_TYPE_NVME:
			return BusTypeNvme;
		case CAtaSmart::INTERFACE_TYPE_AMD_RC2:
			return BusTypeRAID;
		}
		return BusTypeUnknown;
	}
	}
	return -1;
}

extern "C" DWORD WINAPI
cdi_get_dword(CDI_SMART * ptr, INT index, enum CDI_ATA_DWORD attr)
{
	switch (attr)
	{
	case CDI_DWORD_DISK_SIZE:
		return ptr->vars[index].TotalDiskSize;
	case CDI_DWORD_LOGICAL_SECTOR_SIZE:
		return ptr->vars[index].LogicalSectorSize;
	case CDI_DWORD_PHYSICAL_SECTOR_SIZE:
		return ptr->vars[index].PhysicalSectorSize;
	case CDI_DWORD_BUFFER_SIZE:
		return ptr->vars[index].BufferSize;
	case CDI_DWORD_ATTR_COUNT:
		return ptr->vars[index].AttributeCount;
	case CDI_DWORD_POWER_ON_COUNT:
		return ptr->vars[index].PowerOnCount;
	case CDI_DWORD_ROTATION_RATE:
		return ptr->vars[index].NominalMediaRotationRate;
	case CDI_DWORD_DRIVE_LETTER:
		return ptr->vars[index].DriveLetterMap;
	case CDI_DWORD_DISK_VENDOR_ID:
		return ptr->vars[index].DiskVendorId;
	}
	return 0;
}

extern "C" WCHAR* WINAPI
cdi_get_string(CDI_SMART * ptr, INT index, enum CDI_ATA_STRING attr)
{
	switch (attr)
	{
	case CDI_STRING_SN:
		return cs_to_wcs(ptr->vars[index].SerialNumber);
	case CDI_STRING_FIRMWARE:
		return cs_to_wcs(ptr->vars[index].FirmwareRev);
	case CDI_STRING_MODEL:
		return cs_to_wcs(ptr->vars[index].Model);
	case CDI_STRING_DRIVE_MAP:
		return cs_to_wcs(ptr->vars[index].DriveMap);
	case CDI_STRING_TRANSFER_MODE_MAX:
		return cs_to_wcs(ptr->vars[index].MaxTransferMode);
	case CDI_STRING_TRANSFER_MODE_CUR:
		return cs_to_wcs(ptr->vars[index].CurrentTransferMode);
	case CDI_STRING_INTERFACE:
		return cs_to_wcs(ptr->vars[index].Interface);
	case CDI_STRING_VERSION_MAJOR:
		return cs_to_wcs(ptr->vars[index].MajorVersion);
	case CDI_STRING_VERSION_MINOR:
		return cs_to_wcs(ptr->vars[index].MinorVersion);
	case CDI_STRING_PNP_ID:
		return cs_to_wcs(ptr->vars[index].PnpDeviceId);
	case CDI_STRING_SMART_KEY:
		return cs_to_wcs(ptr->vars[index].SmartKeyName);
	case CDI_STRING_FORM_FACTOR:
		return cs_to_wcs(ptr->vars[index].DeviceNominalFormFactor);
	case CDI_STRING_FIRMWARE_REV:
		return cs_to_wcs(ptr->vars[index].FirmwareRev);
	}
	return nullptr;
}

extern "C" VOID WINAPI
cdi_free_string(WCHAR* ptr)
{
	if (ptr)
		CoTaskMemFree(ptr);
}

extern "C" WCHAR* WINAPI
cdi_get_smart_format(CDI_SMART * ptr, INT index)
{
	CString fmt;
	if (ptr->vars[index].DiskVendorId == CAtaSmart::SSD_VENDOR_SANDFORCE)
	{
		if (ptr->vars[index].IsThresholdCorrect)
			fmt = _T("Cur Wor Thr RawValues(7)");
		else
			fmt = _T("Cur Wor --- RawValues(7)");
	}
	else if (ptr->vars[index].DiskVendorId == CAtaSmart::SSD_VENDOR_JMICRON
		&& ptr->vars[index].IsRawValues8)
		fmt = _T("Cur RawValues(8)");
	else if (ptr->vars[index].DiskVendorId == CAtaSmart::SSD_VENDOR_INDILINX)
		fmt = _T("RawValues(8)");
	else if (ptr->vars[index].DiskVendorId == CAtaSmart::SSD_VENDOR_NVME)
		fmt = _T("RawValues(7)");
	else
	{
		if (ptr->vars[index].IsThresholdCorrect)
			fmt = _T("Cur Wor Thr RawValues(6)");
		else
			fmt = _T("Cur Wor --- RawValues(6)");
	}
		
	return cs_to_wcs(fmt);
}

extern "C" BYTE WINAPI
cdi_get_smart_id(CDI_SMART * ptr, INT index, INT attr)
{
	return ptr->vars[index].Attribute[attr].Id;
}

extern "C" WCHAR* WINAPI
cdi_get_smart_value(CDI_SMART * ptr, INT index, INT attr, BOOL hex)
{
	CString cstr;
	UINT64 raw;
	SMART_ATTRIBUTE* data = &ptr->vars[index].Attribute[attr];

	if (ptr->vars[index].DiskVendorId == CAtaSmart::SSD_VENDOR_SANDFORCE)
	{
		raw = ((UINT64)data->Reserved << 48) +
			((UINT64)data->RawValue[5] << 40) +
			((UINT64)data->RawValue[4] << 32) +
			((UINT64)data->RawValue[3] << 24) +
			((UINT64)data->RawValue[2] << 16) +
			((UINT64)data->RawValue[1] << 8) +
			((UINT64)data->RawValue[0]);
		if (ptr->vars[index].IsThresholdCorrect)
			cstr.Format(hex ? _T("%3d %3d %3d %014llX") : _T("%3d %3d %3d %I64u"),
				data->CurrentValue, data->WorstValue, ptr->vars[index].Threshold[attr].ThresholdValue, raw);
		else
			cstr.Format(hex ? _T("%3d %3d --- %014llX") : _T("%3d %3d --- %I64u"),
				data->CurrentValue, data->WorstValue, raw);
	}
	else if (ptr->vars[index].DiskVendorId == CAtaSmart::SSD_VENDOR_JMICRON
		&& ptr->vars[index].IsRawValues8)
	{
		raw = ((UINT64)data->Reserved << 56) +
			((UINT64)data->RawValue[5] << 48) +
			((UINT64)data->RawValue[4] << 40) +
			((UINT64)data->RawValue[3] << 32) +
			((UINT64)data->RawValue[2] << 24) +
			((UINT64)data->RawValue[1] << 16) +
			((UINT64)data->RawValue[0] << 8) +
			((UINT64)data->WorstValue);
		cstr.Format(hex ? _T("%3d %016llX") : _T("%3d %I64u"),
			data->CurrentValue, raw);
	}
	else if (ptr->vars[index].DiskVendorId == CAtaSmart::SSD_VENDOR_INDILINX)
	{
		raw = ((UINT64)data->RawValue[5] << 56) +
			((UINT64)data->RawValue[4] << 48) +
			((UINT64)data->RawValue[3] << 40) +
			((UINT64)data->RawValue[2] << 32) +
			((UINT64)data->RawValue[1] << 24) +
			((UINT64)data->RawValue[0] << 16) +
			((UINT64)data->WorstValue << 8) +
			((UINT64)data->CurrentValue);
		cstr.Format(hex ? _T("%016llX") : _T("%I64u"), raw);
	}
	else if (ptr->vars[index].DiskVendorId == CAtaSmart::SSD_VENDOR_NVME)
	{
		raw = ((UINT64)data->Reserved << 48) +
			((UINT64)data->RawValue[5] << 40) +
			((UINT64)data->RawValue[4] << 32) +
			((UINT64)data->RawValue[3] << 24) +
			((UINT64)data->RawValue[2] << 16) +
			((UINT64)data->RawValue[1] << 8) +
			((UINT64)data->RawValue[0]);
		cstr.Format(hex ? _T("%014llX") : _T("%I64u"), raw);
	}
	else
	{
		raw = ((UINT64)data->RawValue[5] << 40) +
			((UINT64)data->RawValue[4] << 32) +
			((UINT64)data->RawValue[3] << 24) +
			((UINT64)data->RawValue[2] << 16) +
			((UINT64)data->RawValue[1] << 8) +
			((UINT64)data->RawValue[0]);
		if (ptr->vars[index].IsThresholdCorrect)
		{
			cstr.Format(hex ? _T("%3d %3d %3d %012llX") : _T("%3d %3d %3d %I64u"),
				data->CurrentValue, data->WorstValue, ptr->vars[index].Threshold[attr].ThresholdValue, raw);
		}
		else
		{
			cstr.Format(hex ? _T("%3d %3d --- %012llX") : _T("%3d %3d --- %I64u"),
				data->CurrentValue, data->WorstValue, raw);
		}
	}

	return cs_to_wcs(cstr);
}

// DiskInfoDlgUpdate.cpp
// BOOL CDiskInfoDlg::UpdateListCtrl(DWORD i)
extern "C" INT WINAPI
cdi_get_smart_status(CDI_SMART * ptr, INT index, INT attr)
{
	INT stat = CDI_DISK_STATUS_UNKNOWN;
	const BYTE attr_id = ptr->vars[index].Attribute[attr].Id;
	const DWORD ven_id = ptr->vars[index].DiskVendorId;

	if (attr_id == 0x00 || attr_id == 0xFF)
		return stat;

	if (ptr->vars[index].IsNVMe)
	{
		stat = CDI_DISK_STATUS_GOOD;
		if (ptr->vars[index].Model.Find(_T("Parallels")) == 0
			|| ptr->vars[index].Model.Find(_T("VMware")) == 0
			|| ptr->vars[index].Model.Find(_T("QEMU")) == 0)  stat = CDI_DISK_STATUS_UNKNOWN;
		else
		{
			switch (attr_id)
			{
				case 0x01:
					if (ptr->vars[index].Attribute[attr].RawValue[0])  stat = CDI_DISK_STATUS_BAD;
					break;
				case 0x02:
				{
					const int temperature = (int)(MAKEWORD(ptr->vars[index].Attribute[1].RawValue[0], ptr->vars[index].Attribute[1].RawValue[1])) - 273;
					if (temperature >= ptr->vars[index].AlarmTemperature)  stat = CDI_DISK_STATUS_BAD;
					else if (temperature == ptr->vars[index].AlarmTemperature)  stat = CDI_DISK_STATUS_CAUTION;
				}
				break;
				case 0x03:
					// 2022/10/02 Workaround for no support Available Spare Threshold devices
					// https://github.com/hiyohiyo/CrystalDiskInfo/issues/186
					if (ptr->vars[index].Attribute[3].RawValue[0] > 100)
						stat = CDI_DISK_STATUS_GOOD;
				// 2022/03/26 Workaround for WD_BLACK AN1500 (No support Available Spare/Available Spare Threshold)
				else if (ptr->vars[index].Attribute[2].RawValue[0] == 0 && ptr->vars[index].Attribute[3].RawValue[0] == 0)
					stat = CDI_DISK_STATUS_GOOD;
				else if (ptr->vars[index].Attribute[2].RawValue[0] < ptr->vars[index].Attribute[3].RawValue[0])
					stat = CDI_DISK_STATUS_BAD;
				else if (ptr->vars[index].Attribute[2].RawValue[0] == ptr->vars[index].Attribute[3].RawValue[0] && ptr->vars[index].Attribute[3].RawValue[0] != 100)
					stat = CDI_DISK_STATUS_CAUTION;
				break;
				case 0x05:
					if ((100 - ptr->vars[index].Attribute[attr].RawValue[0]) <= ptr->vars[index].ThresholdFF)
						stat = CDI_DISK_STATUS_CAUTION;
				break;
			}
		}
	}
	else if (ptr->vars[index].IsSmartCorrect && ptr->vars[index].IsThresholdCorrect && !ptr->vars[index].IsThresholdBug)
	{
		if (!ptr->vars[index].IsSsd &&
			(attr_id == 0x05 // Reallocated Sectors Count
			|| attr_id == 0xC5 // Current Pending Sector Count
			|| attr_id == 0xC6 // Off-Line Scan Uncorrectable Sector Count
			))
		{
			if (ptr->vars[index].Threshold[attr].ThresholdValue
				&& ptr->vars[index].Attribute[attr].CurrentValue < ptr->vars[index].Threshold[attr].ThresholdValue)
				stat = CDI_DISK_STATUS_BAD;
			else
			{
				WORD threshold = 0;
				switch (attr_id)
				{
					case 0x05:
						threshold = ptr->vars[index].Threshold05;
						break;
					case 0xC5:
						threshold = ptr->vars[index].ThresholdC5;
						break;
					case 0xC6:
						threshold = ptr->vars[index].ThresholdC6;
						break;
				}
				if (threshold && (MAKEWORD(ptr->vars[index].Attribute[attr].RawValue[0], ptr->vars[index].Attribute[attr].RawValue[1])) >= threshold)
					stat = CDI_DISK_STATUS_CAUTION;
				else
					stat = CDI_DISK_STATUS_GOOD;
			}
		}
		// [2021/12/15] Workaround for SanDisk USB Memory
		else if (attr_id == 0xE8 && ptr->vars[index].FlagLifeSanDiskUsbMemory)
			stat = CDI_DISK_STATUS_GOOD;
		// Temperature
		else if (attr_id == 0xC2)
			stat = CDI_DISK_STATUS_GOOD;
		// End-to-End Error
		// https://crystalmark.info/bbs/c-board.cgi?cmd=one;no=1090;id=diskinfo#1090
		// http://h20000.www2.hp.com/bc/docs/support/SupportManual/c01159621/c01159621.pdf
		else if (attr_id == 0xB8)
			stat = CDI_DISK_STATUS_GOOD;
		// Life for WDC/SanDisk
		else if (attr_id == 0xE6 && (ptr->vars[index].DiskVendorId == ptr->SSD_VENDOR_WDC || ptr->vars[index].DiskVendorId == ptr->SSD_VENDOR_SANDISK))
		{
			int life = -1;

			if (ptr->vars[index].FlagLifeSanDiskUsbMemory)
				life = -1;
			else if (ptr->vars[index].FlagLifeSanDisk0_1)
				life = 100 - (ptr->vars[index].Attribute[attr].RawValue[1] * 256 + ptr->vars[index].Attribute[attr].RawValue[0]) / 100;
			else if (ptr->vars[index].FlagLifeSanDisk1)
				life = 100 - ptr->vars[index].Attribute[attr].RawValue[1];
			else if (ptr->vars[index].FlagLifeSanDiskLenovo)
				life = ptr->vars[index].Attribute[attr].CurrentValue;
			else
				life = 100 - ptr->vars[index].Attribute[attr].RawValue[1];

			if (life < 0)
				life = 0;

			if (ptr->vars[index].FlagLifeSanDiskUsbMemory)
				stat = CDI_DISK_STATUS_GOOD;
			else if (life == 0)
				stat = CDI_DISK_STATUS_BAD;
			else if (life <= ptr->vars[index].ThresholdFF)
				stat = CDI_DISK_STATUS_CAUTION;
			else
				stat = CDI_DISK_STATUS_GOOD;
		}
		// Life
		else if (
			(attr_id == 0xA9 && (ven_id == ptr->SSD_VENDOR_REALTEK || (ven_id == ptr->SSD_VENDOR_KINGSTON && ptr->vars[index].HostReadsWritesUnit == ptr->HOST_READS_WRITES_32MB) || ven_id == ptr->SSD_VENDOR_SILICONMOTION))
			|| (attr_id == 0xAD && (ven_id == ptr->SSD_VENDOR_TOSHIBA || ven_id == ptr->SSD_VENDOR_KIOXIA))
			|| (attr_id == 0xB1 && ven_id == ptr->SSD_VENDOR_SAMSUNG)
			|| (attr_id == 0xBB && ven_id == ptr->SSD_VENDOR_MTRON)
			|| (attr_id == 0xCA && (ven_id == ptr->SSD_VENDOR_MICRON || ven_id == ptr->SSD_VENDOR_MICRON_MU03 || ven_id == ptr->SSD_VENDOR_INTEL_DC || ven_id == ptr->SSD_VENDOR_SILICONMOTION_CVC))
			|| (attr_id == 0xD1 && ven_id == ptr->SSD_VENDOR_INDILINX)
			|| (attr_id == 0xE7 && (ven_id == ptr->SSD_VENDOR_SANDFORCE || ven_id == ptr->SSD_VENDOR_CORSAIR || ven_id == ptr->SSD_VENDOR_KINGSTON || ven_id == ptr->SSD_VENDOR_SKHYNIX
			|| ven_id == ptr->SSD_VENDOR_REALTEK || ven_id == ptr->SSD_VENDOR_SANDISK || ven_id == ptr->SSD_VENDOR_SSSTC || ven_id == ptr->SSD_VENDOR_APACER
			|| ven_id == ptr->SSD_VENDOR_JMICRON || ven_id == ptr->SSD_VENDOR_SEAGATE || ven_id == ptr->SSD_VENDOR_MAXIOTEK
			|| ven_id == ptr->SSD_VENDOR_YMTC || ven_id == ptr->SSD_VENDOR_SCY || ven_id == ptr->SSD_VENDOR_RECADATA || ven_id == ptr->SSD_VENDOR_ADATA_INDUSTRIAL))
			|| (attr_id == 0xE8 && ven_id == ptr->SSD_VENDOR_PLEXTOR)
			|| (attr_id == 0xE9 && (ven_id == ptr->SSD_VENDOR_INTEL || ven_id == ptr->SSD_VENDOR_OCZ || ven_id == ptr->SSD_VENDOR_OCZ_VECTOR || ven_id == ptr->SSD_VENDOR_SKHYNIX))
			|| (attr_id == 0xE9 && ven_id == ptr->SSD_VENDOR_SANDISK_LENOVO_HELEN_VENUS)
			|| (attr_id == 0xE6 && (ven_id == ptr->SSD_VENDOR_SANDISK_LENOVO || ven_id == ptr->SSD_VENDOR_SANDISK_DELL))
			|| (attr_id == 0xC9 && (ven_id == ptr->SSD_VENDOR_SANDISK_HP || ven_id == ptr->SSD_VENDOR_SANDISK_HP_VENUS))
		)
		{
			if (ptr->vars[index].FlagLifeRawValue || ptr->vars[index].FlagLifeRawValueIncrement)
				stat = CDI_DISK_STATUS_GOOD;
			else if (ptr->vars[index].Attribute[attr].CurrentValue == 0
				|| ptr->vars[index].Attribute[attr].CurrentValue < ptr->vars[index].Threshold[attr].ThresholdValue)
				stat = CDI_DISK_STATUS_BAD;
			else if (ptr->vars[index].Attribute[attr].CurrentValue <= ptr->vars[index].ThresholdFF)
				stat = CDI_DISK_STATUS_CAUTION;
			else
				stat = CDI_DISK_STATUS_GOOD;
		}
		else if (attr_id == 0x01 // Read Error Rate for SandForce Bug
			&& ven_id == ptr->SSD_VENDOR_SANDFORCE)
		{
			if (ptr->vars[index].Attribute[attr].CurrentValue == 0
				&& ptr->vars[index].Attribute[attr].RawValue[0] == 0
				&& ptr->vars[index].Attribute[attr].RawValue[1] == 0)
				stat = CDI_DISK_STATUS_GOOD;
			else if (ptr->vars[index].Threshold[attr].ThresholdValue
				&& ptr->vars[index].Attribute[attr].CurrentValue < ptr->vars[index].Threshold[attr].ThresholdValue)
				stat = CDI_DISK_STATUS_BAD;
			else
				stat = CDI_DISK_STATUS_GOOD;
		}
		else if ((ptr->vars[index].IsSsd && !ptr->vars[index].IsRawValues8)
			|| ((0x01 <= attr_id && attr_id <= 0x0D)
			|| attr_id == 0x16
			|| (0xBB <= attr_id && attr_id <= 0xC1)
			|| (0xC3 <= attr_id && attr_id <= 0xD1)
			|| (0xD3 <= attr_id && attr_id <= 0xD4)
			|| (0xDC <= attr_id && attr_id <= 0xE4)
			|| (0xE6 <= attr_id && attr_id <= 0xE7)
			|| attr_id == 0xF0
			|| attr_id == 0xFA
			|| attr_id == 0xFE
			))
		{
			if (ptr->vars[index].Threshold[attr].ThresholdValue
				&& ptr->vars[index].Attribute[attr].CurrentValue < ptr->vars[index].Threshold[attr].ThresholdValue)
				stat = CDI_DISK_STATUS_BAD;
			else
				stat = CDI_DISK_STATUS_GOOD;
		}
		else
			stat = CDI_DISK_STATUS_GOOD;
	}

	return stat;
}

extern "C" WCHAR * WINAPI
cdi_get_smart_name(CDI_SMART * ptr, INT index, BYTE id)
{
	for (size_t i = 0; i < ARRAYSIZE(SMART_NAMES); i++)
	{
		if (ptr->vars[index].SmartKeyName.CompareNoCase(SMART_NAMES[i].group) == 0)
		{
			for (size_t j = 0; j < SMART_NAMES[i].count; j++)
			{
				if (id == SMART_NAMES[i].name[j].id)
					return cs_to_wcs(SMART_NAMES[i].name[j].name);
			}
			break;
		}
	}
	return cs_to_wcs(_T("Vendor Specific"));
}
