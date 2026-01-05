// SPDX-License-Identifier: MIT

#define VC_EXTRALEAN
#include <windows.h>
#include <stdio.h>

#include "../libcdi/libcdi.h"
#include "disklib.h"
#pragma comment(lib, "setupapi.lib")

static INT
GetSmartIndex(CDI_SMART* cdiSmart, INT cdiCount, DWORD dwId)
{
	for (INT i = 0; i < cdiCount; i++)
	{
		if (cdi_get_int(cdiSmart, i, CDI_INT_DISK_ID) == (INT)dwId)
			return i;
	}
	return -1;
}

static VOID
PrintSmartInfo(CDI_SMART* cdiSmart, INT nIndex)
{
	INT d;
	DWORD n;
	WCHAR* str;
	BOOL ssd;
	BYTE id;

	if (nIndex < 0)
	{
		printf("SMART -> UNKNOWN\n");
		return;
	}

	cdi_update_smart(cdiSmart, nIndex);

	printf("SMART -> %d\n", nIndex);

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
	INT cdiCount;
	DWORD dwCount;
	CDI_SMART* cdiSmart;
	PHY_DRIVE_INFO* pdInfo;
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
		CDI_FLAG_ENABLE_REALTEK_9220DP |
		CDI_FLAG_CSMI_AUTO);
	printf("Ticks: %llu\n", GetTickCount64() - ullTick);

	cdiCount = cdi_get_disk_count(cdiSmart);
	printf("SMART DRIVE Count: %d\n", cdiCount);

	dwCount = GetDriveInfoList(FALSE, &pdInfo);
	printf("PHYSICAL DRIVE Count: %u\n", dwCount);

	for (DWORD i = 0; i < dwCount; i++)
	{
		printf("PHYSICAL -> \\\\.\\PhysicalDrive%lu\n", pdInfo[i].Index);
		printf("\tHWID: %s\n", Ucs2ToUtf8(pdInfo[i].HwID));
		printf("\tModel: %s\n", Ucs2ToUtf8(pdInfo[i].HwName));
		printf("\tSize: %s\n", GetHumanSize(pdInfo[i].SizeInBytes, 1024));
		printf("\tRemovable Media: %s\n", pdInfo[i].RemovableMedia ? "Yes" : "No");
		printf("\tVendor Id: %s\n", pdInfo[i].VendorId);
		printf("\tProduct Id: %s\n", pdInfo[i].ProductId);
		printf("\tProduct Rev: %s\n", pdInfo[i].ProductRev);
		printf("\tBus Type: %s\n", GetBusTypeName(pdInfo[i].BusType));

		printf("\tPartition Table: %s\n", GetPartMapName(pdInfo[i].PartMap));
		switch (pdInfo[i].PartMap)
		{
		case PARTITION_STYLE_MBR:
			printf("\tMBR Signature: %02X %02X %02X %02X\n",
				pdInfo[i].MbrSignature[0], pdInfo[i].MbrSignature[1],
				pdInfo[i].MbrSignature[2], pdInfo[i].MbrSignature[3]);
			break;
		case PARTITION_STYLE_GPT:
			printf("\tGPT GUID: {%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
				pdInfo[i].GptGuid[0], pdInfo[i].GptGuid[1], pdInfo[i].GptGuid[2], pdInfo[i].GptGuid[3],
				pdInfo[i].GptGuid[4], pdInfo[i].GptGuid[5], pdInfo[i].GptGuid[6], pdInfo[i].GptGuid[7],
				pdInfo[i].GptGuid[8], pdInfo[i].GptGuid[9], pdInfo[i].GptGuid[10], pdInfo[i].GptGuid[11],
				pdInfo[i].GptGuid[12], pdInfo[i].GptGuid[13], pdInfo[i].GptGuid[14], pdInfo[i].GptGuid[15]);
			break;
		}

		PrintSmartInfo(cdiSmart, GetSmartIndex(cdiSmart, cdiCount, pdInfo[i].Index));
		for (DWORD j = 0; j < pdInfo[i].VolCount; j++)
		{
			DISK_VOL_INFO* p = &pdInfo[i].VolInfo[j];
			printf("\t%s\n", Ucs2ToUtf8(p->VolPath));

			printf("\t\tStarting LBA: %llu\n", p->StartLba);
			printf("\t\tPartition Number: %lu\n", p->PartNum);
			printf("\t\tPartition Type: %s\n", Ucs2ToUtf8(p->PartType));
			printf("\t\tPartition ID: %s\n", Ucs2ToUtf8(p->PartId));
			printf("\t\tBoot Indicator: %s\n", p->BootIndicator ? "Yes" : "No");
			printf("\t\tPartition Flag: %s\n", Ucs2ToUtf8(p->PartFlag));

			printf("\t\tLabel: %s\n", Ucs2ToUtf8(p->VolLabel));
			printf("\t\tFS: %s\n", Ucs2ToUtf8(p->VolFs));
			printf("\t\tFree Space: %s\n", GetHumanSize(p->VolFreeSpace.QuadPart, 1024));
			printf("\t\tTotal Space: %s\n", GetHumanSize(p->VolTotalSpace.QuadPart, 1024));
			printf("\t\tUsage: %.2f%%\n", p->VolUsage);
			printf("\t\tMount Points:\n");
			for (WCHAR* q = p->VolNames; q[0] != L'\0'; q += wcslen(q) + 1)
			{
				printf("\t\t\t%s\n", Ucs2ToUtf8(q));
			}
		}
		printf("\n");
	}

	for (INT i = 0; i < cdiCount; i++)
	{
		INT d = cdi_get_int(cdiSmart, i, CDI_INT_DISK_ID);
		if (d >= 0)
			continue;
		printf("PHYSICAL -> UNKNOWN\n");
		PrintSmartInfo(cdiSmart, i);
		printf("\n");
	}

	cdi_destroy_smart(cdiSmart);
	DestoryDriveInfoList(pdInfo, dwCount);
	CoUninitialize();
	return 0;
}

