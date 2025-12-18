// SPDX-License-Identifier: MIT

#define VC_EXTRALEAN
#include <windows.h>
#include <stdio.h>

#include "../libcdi/libcdi.h"

static VOID
PrintSmartInfo(CDI_SMART* cdiSmart, INT nIndex)
{
	INT d;
	DWORD n;
	WCHAR* str;
	BOOL ssd;
	BYTE id;

	cdi_update_smart(cdiSmart, nIndex);

	d = cdi_get_int(cdiSmart, nIndex, CDI_INT_DISK_ID);
	printf("\\\\.\\PhysicalDrive%d\n", d);

	str = cdi_get_string(cdiSmart, nIndex, CDI_STRING_MODEL);
	printf("\tModel: %ls\n", str);
	cdi_free_string(str);

	str = cdi_get_string(cdiSmart, nIndex, CDI_STRING_DRIVE_MAP);
	printf("\tDrive Letters: %ls\n", str);
	cdi_free_string(str);

	ssd = cdi_get_bool(cdiSmart, nIndex, CDI_BOOL_SSD);
	printf("\tSSD: %s\n", ssd ? "Yes" : "No");

	str = cdi_get_string(cdiSmart, nIndex, CDI_STRING_SN);
	printf("\tSerial: %ls\n", str);
	cdi_free_string(str);

	str = cdi_get_string(cdiSmart, nIndex, CDI_STRING_FIRMWARE);
	printf("\tFirmware: %ls\n", str);
	cdi_free_string(str);

	str = cdi_get_string(cdiSmart, nIndex, CDI_STRING_INTERFACE);
	printf("\tInterface: %ls\n", str);
	cdi_free_string(str);

	str = cdi_get_string(cdiSmart, nIndex, CDI_STRING_TRANSFER_MODE_CUR);
	printf("\tCurrent Transfer Mode: %ls\n", str);
	cdi_free_string(str);

	str = cdi_get_string(cdiSmart, nIndex, CDI_STRING_TRANSFER_MODE_MAX);
	printf("\tMax Transfer Mode: %ls\n", str);
	cdi_free_string(str);

	str = cdi_get_string(cdiSmart, nIndex, CDI_STRING_FORM_FACTOR);
	printf("\tForm Factor: %ls\n", str);
	cdi_free_string(str);

	d = cdi_get_int(cdiSmart, nIndex, CDI_INT_LIFE);
	printf("\tHealth Status: %s", cdi_get_health_status(cdi_get_int(cdiSmart, nIndex, CDI_INT_DISK_STATUS)));
	if (d >= 0)
		printf(" (%d%%)\n", d);
	else
		printf("\n");

	printf("\tTemperature: %d (C)\n", cdi_get_int(cdiSmart, nIndex, CDI_INT_TEMPERATURE));

	str = cdi_get_smart_format(cdiSmart, nIndex);
	printf("\tID  Status %-24ls Name\n", str);
	cdi_free_string(str);

	n = cdi_get_dword(cdiSmart, nIndex, CDI_DWORD_ATTR_COUNT);
	for (INT j = 0; j < (INT)n; j++)
	{
		id = cdi_get_smart_id(cdiSmart, nIndex, j);
		if (id == 0x00)
			continue;
		str = cdi_get_smart_value(cdiSmart, nIndex, j, FALSE);
		printf("\t%02X %7s %-24ls",
			id,
			cdi_get_health_status(cdi_get_smart_status(cdiSmart, nIndex, j)),
			str);
		printf(" %ls\n", cdi_get_smart_name(cdiSmart, nIndex, id));
		cdi_free_string(str);
	}
}

int main(int argc, char* argv[])
{
	INT diskCount;
	CDI_SMART* cdiSmart;
	UINT64 ullTick = 0;

	(void)CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	cdiSmart = cdi_create_smart();

	printf("CDI v%s\n", cdi_get_version());

	ullTick = GetTickCount64();
	cdi_init_smart(cdiSmart,
		CDI_FLAG_USE_WMI |
		CDI_FLAG_ADVANCED_SEARCH |
		CDI_FLAG_ATA_PASS_THROUGH |
		CDI_FLAG_ENABLE_NVIDIA |
		CDI_FLAG_ENABLE_MARVELL |
		CDI_FLAG_ENABLE_USB_SAT |
		CDI_FLAG_ENABLE_USB_SUNPLUS |
		CDI_FLAG_ENABLE_USB_IODATA |
		CDI_FLAG_ENABLE_USB_LOGITEC |
		CDI_FLAG_ENABLE_USB_PROLIFIC |
		CDI_FLAG_ENABLE_USB_JMICRON |
		CDI_FLAG_ENABLE_USB_CYPRESS |
		CDI_FLAG_ENABLE_USB_MEMORY |
		CDI_FLAG_ENABLE_NVME_JMICRON |
		CDI_FLAG_ENABLE_NVME_ASMEDIA |
		CDI_FLAG_ENABLE_NVME_REALTEK |
		CDI_FLAG_ENABLE_MEGA_RAID |
		CDI_FLAG_ENABLE_INTEL_VROC |
		CDI_FLAG_ENABLE_ASM1352R |
		CDI_FLAG_ENABLE_AMD_RC2 |
		CDI_FLAG_ENABLE_REALTEK_9220DP);
	printf("Ticks: %llu\n", GetTickCount64() - ullTick);

	diskCount = cdi_get_disk_count(cdiSmart);
	printf("Disk Count: %lu\n", diskCount);

	for (INT i = 0; i < diskCount; i++)
	{
		PrintSmartInfo(cdiSmart, i);
	}

	cdi_destroy_smart(cdiSmart);
	CoUninitialize();
	return 0;
}

