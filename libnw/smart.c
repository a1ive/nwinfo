// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "disk.h"
#include "utils.h"
#include "smart.h"

#include <versionhelpers.h>

#define SMART_ATTR_FLAG_CRITICAL        0x01
#define SMART_ATTR_FLAG_HIGHER_BETTER   0x02
#define SMART_ATTR_FLAG_LOWER_BETTER    0x04

static LPCSTR
GetSmartAttr(BYTE Id, PDWORD Flag)
{
	*Flag = 0;
	switch (Id)
	{
	case 0x01:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Read Error Rate";
	case 0x02:
		*Flag |= SMART_ATTR_FLAG_HIGHER_BETTER;
		return "Throughput Performance";
	case 0x03:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Spin-Up Time";
	case 0x04: return "Start/Stop Count";
	case 0x05:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER | SMART_ATTR_FLAG_CRITICAL;
		return "Reallocated Sectors Count";
	case 0x06: return "Read Channel Margin";
	case 0x07: return "Seek Error Rate";
	case 0x08:
		*Flag |= SMART_ATTR_FLAG_HIGHER_BETTER;
		return "Seek Time Performance";
	case 0x09: return "Power On Hours";
	case 0x0a:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER | SMART_ATTR_FLAG_CRITICAL;
		return "Spin Retry Count";
	case 0x0b:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Recalibration Retries";
	case 0x0c: return "Power Cycle Count";
	case 0x0d:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Soft Read Error Rate";
	case 0x16:
		*Flag |= SMART_ATTR_FLAG_HIGHER_BETTER;
		return "Current Helium Level";
	case 0xaa: return "Available Reserved Space";
	case 0xab: return "SSD Program Fail Count";
	case 0xac: return "SSD Erase Fail Count";
	case 0xad: return "SSD Wear Leveling Count";
	case 0xae: return "Unexpected Power Loss Count";
	case 0xaf: return "Power Loss Protection Failure";
	case 0xb0: return "Erase Fail Count";
	case 0xb1: return "Wear Range Delta";
	case 0xb2: return "Used Reserved Block Count";
	case 0xb3: return "Used Reserved Block Count Total";
	case 0xb4: return "Unused Reserved Block Count Total";
	case 0xb5:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Program Fail Count Total";
	case 0xb6: return "Erase Fail Count";
	case 0xb7: return "SATA Downshift Error Count";
	case 0xb8:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER | SMART_ATTR_FLAG_CRITICAL;
		return "End-to-End error / IOEDC";
	case 0xb9: return "Head Stability";
	case 0xba: return "Induced Op-Vibration Detection";
	case 0xbb:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER | SMART_ATTR_FLAG_CRITICAL;
		return "Reported Uncorrectable Errors";
	case 0xbc:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER | SMART_ATTR_FLAG_CRITICAL;
		return "Command Timeout";
	case 0xbd:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "High Fly Writes";
	case 0xbe: return "Temperature Difference";
	case 0xbf:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "G-sense Error Rate";
	case 0xc0:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Power-off Retract Count";
	case 0xc1:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Load Cycle Count";
	case 0xc2:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Temperature";
	case 0xc3: return "Hardware ECC Recovered";
	case 0xc4:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER | SMART_ATTR_FLAG_CRITICAL;
		return "Reallocation Event Count";
	case 0xc5:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER | SMART_ATTR_FLAG_CRITICAL;
		return "Current Pending Sector Count";
	case 0xc6:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER | SMART_ATTR_FLAG_CRITICAL;
		return "Uncorrectable Sector Count";
	case 0xc7:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "UltraDMA CRC Error Count";
	case 0xc8:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Multi-Zone Error Rate";
	case 0xc9:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER | SMART_ATTR_FLAG_CRITICAL;
		return "Soft Read Error Rate";
	case 0xca:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Data Address Mark errors";
	case 0xcb:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Run Out Cancel";
	case 0xcc:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Soft ECC Correction";
	case 0xcd:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Thermal Asperity Rate";
	case 0xce: return "Flying Height";
	case 0xcf:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Spin High Current";
	case 0xd0: return "Spin Buzz";
	case 0xd1: return "Offline Seek Performance";
	case 0xd2: return "Vibration During Write";
	case 0xd3: return "Vibration During Write";
	case 0xd4: return "Shock During Write";
	case 0xdc:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Disk Shift";
	case 0xdd:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "G-Sense Error Rate";
	case 0xde: return "Loaded Hours";
	case 0xdf: return "Load/Unload Retry Count";
	case 0xe0:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Load Friction";
	case 0xe1:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Load/Unload Cycle Count";
	case 0xe2: return "Load In-time";
	case 0xe3:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Torque Amplification Count";
	case 0xe4:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Power-Off Retract Cycle";
	case 0xe6: return "HDD GMR Head Amplitude | SSD Drive Life Protection Status";
	case 0xe7: return "SSD Life Left";
	case 0xe8: return "Endurance Remaining";
	case 0xe9: return "SSD Media Wearout Indicator";
	case 0xea: return "Average erase count AND Maximum Erase Count";
	case 0xeb: return "Good Block Count AND System(Free) Block Count";
	case 0xf0: return "Head Flying Hours";
	case 0xf1: return "Total LBAs Written";
	case 0xf2: return "Total LBAs Read";
	case 0xf3: return "Total LBAs Written Expanded";
	case 0xf4: return "Total LBAs Read Expanded";
	case 0xf9: return "NAND Writes (1GiB)";
	case 0xfa:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Read Error Retry Rate";
	case 0xfb: return "Minimum Spares Remaining";
	case 0xfc: return "Newly Added Bad Flash Block";
	case 0xfe:
		*Flag |= SMART_ATTR_FLAG_LOWER_BETTER;
		return "Free Fall Protection";
	}
	return "Unknown";
}

static LPCSTR
GetSmartWarn(BYTE Id, UINT64 Value)
{
	switch (Id)
	{
	case 0x05: if (Value > 0) return "!!"; break;
	case 0xc2: if ((Value & 0xff) > 50) return "!!"; break;
	case 0xc5: if (Value > 0) return "!!"; break;
	case 0xc6: if (Value > 0) return "!!"; break;
	}
	return "OK";
}

static BOOL
GetTrimData(PNODE pNode, HANDLE hDisk)
{
	DWORD dwBytes;
	STORAGE_PROPERTY_QUERY propQuery = { 0 };
	DEVICE_TRIM_DESCRIPTOR descTrim = { 0 };
	if (!IsWindows7OrGreater())
		return FALSE;
	propQuery.PropertyId = StorageDeviceTrimProperty;
	propQuery.QueryType = PropertyStandardQuery;
	if (DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &propQuery, sizeof(propQuery),
		&descTrim, sizeof(descTrim), &dwBytes, NULL))
	{
		NWL_NodeAttrSetBool(pNode, "Trim Enabled", descTrim.TrimEnabled, 0);
		return TRUE;
	}
	return FALSE;
}

#if 0
static BOOL
GetTemperatureData(PNODE pNode, HANDLE hDisk)
{
	DWORD dwBytes;
	STORAGE_PROPERTY_QUERY propQuery = { 0 };
	STORAGE_TEMPERATURE_DATA_DESCRIPTOR descTemp = { 0 };
	if (!IsWindows10OrGreater())
		return FALSE;
	propQuery.PropertyId = StorageDeviceTemperatureProperty;
	propQuery.QueryType = PropertyStandardQuery;
	if (!DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &propQuery, sizeof(propQuery),
		&descTemp, sizeof(descTemp), &dwBytes, NULL))
		return FALSE;
	NWL_NodeAttrSetf(pNode, "Critical Temperature (C)", NAFLG_FMT_NUMERIC, "%u", descTemp.CriticalTemperature);
	NWL_NodeAttrSetf(pNode, "Warning Temperature (C)", NAFLG_FMT_NUMERIC, "%u", descTemp.WarningTemperature);
	if (descTemp.Size >= sizeof(STORAGE_TEMPERATURE_DATA_DESCRIPTOR))
		NWL_NodeAttrSetf(pNode, "Temperature (C)", NAFLG_FMT_NUMERIC, "%u", descTemp.TemperatureInfo[0].Temperature);
	return TRUE;
}
#endif

static VOID
SetRawNumber(PNODE node, LPCSTR value, PUCHAR data, DWORD size)
{
	CHAR str[15] = { 0 };
	UINT64 raw = 0;
	memcpy(&raw, data, size > 7 ? 7 : size);
	NWL_NodeAttrSetf(node, value, 0, "%014llX", raw);
}

static BOOL
GetNvmeData(PNODE pNode, HANDLE hDisk)
{
	DWORD dwBytes;
	DWORD dwBufferSize;
	PVOID pBuffer = NULL;
	PSTORAGE_PROPERTY_QUERY pQuery = NULL;
	PSTORAGE_PROTOCOL_DATA_DESCRIPTOR pDevData = NULL;
	PSTORAGE_PROTOCOL_SPECIFIC_DATA pProtData = NULL;
	PNVME_HEALTH_INFO_LOG pHealthInfo = NULL;
	dwBufferSize = FIELD_OFFSET(STORAGE_PROPERTY_QUERY, AdditionalParameters)
		+ sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) + NVME_MAX_LOG_SIZE;
	pBuffer = calloc(1, dwBufferSize);
	if (!pBuffer)
		return FALSE;
	pQuery = pBuffer;
	pDevData = pBuffer;
	pProtData = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)pQuery->AdditionalParameters;
	pQuery->PropertyId = StorageDeviceProtocolSpecificProperty;
	pQuery->QueryType = PropertyStandardQuery;
	pProtData->ProtocolType = ProtocolTypeNvme;
	pProtData->DataType = NVMeDataTypeLogPage;
	pProtData->ProtocolDataRequestValue = NVME_LOG_PAGE_HEALTH_INFO;
	pProtData->ProtocolDataRequestSubValue = 0;
	pProtData->ProtocolDataRequestSubValue2 = 0;
	pProtData->ProtocolDataRequestSubValue3 = 0;
	pProtData->ProtocolDataRequestSubValue4 = 0;
	pProtData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
	pProtData->ProtocolDataLength = sizeof(NVME_HEALTH_INFO_LOG);
	if (!DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, pQuery,
		dwBufferSize, pDevData, dwBufferSize, &dwBytes, NULL))
		goto fail;
	if (pDevData->Version < sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR) ||
		pDevData->Size < sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))
		goto fail;
	pProtData = &pDevData->ProtocolSpecificData;
	if (pProtData->ProtocolDataOffset < sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) ||
		pProtData->ProtocolDataLength < sizeof(NVME_HEALTH_INFO_LOG))
		goto fail;
	pHealthInfo = (PNVME_HEALTH_INFO_LOG)((PCHAR)pProtData + pProtData->ProtocolDataOffset);

	{
		USHORT tmp;
		memcpy(&tmp, pHealthInfo->Temperature, sizeof(tmp));
		NWL_NodeAttrSetf(pNode, "Temperature (C)", NAFLG_FMT_NUMERIC, "%u", tmp - 273);
	}

	{
		UINT64 mb = 0;
		LPCSTR unit[6] = { "MB", "GB", "TB", "PB", "EB", "ZB" };
		memcpy(&mb, &pHealthInfo->DataUnitRead[1], sizeof(mb));
		NWL_NodeAttrSet(pNode, "Total Read", NWL_GetHumanSize(mb * 125, unit, 1024), NAFLG_FMT_HUMAN_SIZE);
		memcpy(&mb, &pHealthInfo->DataUnitWritten[1], sizeof(mb));
		NWL_NodeAttrSet(pNode, "Total Written", NWL_GetHumanSize(mb * 125, unit, 1024), NAFLG_FMT_HUMAN_SIZE);
	}

	{
		UINT64 tm = 0;
		memcpy(&tm, pHealthInfo->PowerCycle, sizeof(tm));
		NWL_NodeAttrSetf(pNode, "Power On Count", NAFLG_FMT_NUMERIC, "%llu", tm);
		memcpy(&tm, pHealthInfo->PowerOnHours, sizeof(tm));
		NWL_NodeAttrSetf(pNode, "Power On Time (Hours)", NAFLG_FMT_NUMERIC, "%llu", tm);
	}

	SetRawNumber(pNode, "[01] Critical Warning", &pHealthInfo->CriticalWarning.AsUchar, sizeof(pHealthInfo->CriticalWarning));
	SetRawNumber(pNode, "[02] Composite Temperature", pHealthInfo->Temperature, sizeof(pHealthInfo->Temperature));
	SetRawNumber(pNode, "[03] Available Spare", &pHealthInfo->AvailableSpare, sizeof(pHealthInfo->AvailableSpare));
	SetRawNumber(pNode, "[04] Available Spare Threshold", &pHealthInfo->AvailableSpareThreshold, sizeof(pHealthInfo->AvailableSpareThreshold));
	SetRawNumber(pNode, "[05] Percentage Used", &pHealthInfo->PercentageUsed, sizeof(pHealthInfo->PercentageUsed));
	SetRawNumber(pNode, "[06] Data Units Read", pHealthInfo->DataUnitRead, sizeof(pHealthInfo->DataUnitRead));
	SetRawNumber(pNode, "[07] Data Units Written", pHealthInfo->DataUnitWritten, sizeof(pHealthInfo->DataUnitWritten));
	SetRawNumber(pNode, "[08] Host Read Commands", pHealthInfo->HostReadCommands, sizeof(pHealthInfo->HostReadCommands));
	SetRawNumber(pNode, "[09] Host Written Commands", pHealthInfo->HostWrittenCommands, sizeof(pHealthInfo->HostWrittenCommands));
	SetRawNumber(pNode, "[0A] Controller Busy Time", pHealthInfo->ControllerBusyTime, sizeof(pHealthInfo->ControllerBusyTime));
	SetRawNumber(pNode, "[0B] Power Cycles", pHealthInfo->PowerCycle, sizeof(pHealthInfo->PowerCycle));
	SetRawNumber(pNode, "[0C] Power On Hours", pHealthInfo->PowerOnHours, sizeof(pHealthInfo->PowerOnHours));
	SetRawNumber(pNode, "[0D] Unsafe Shutdowns", pHealthInfo->UnsafeShutdowns, sizeof(pHealthInfo->UnsafeShutdowns));
	SetRawNumber(pNode, "[0E] Media and Data Integrity Errors", pHealthInfo->MediaErrors, sizeof(pHealthInfo->MediaErrors));
	SetRawNumber(pNode, "[0F] Number of Error Information Log Entries", pHealthInfo->ErrorInfoLogEntryCount, sizeof(pHealthInfo->ErrorInfoLogEntryCount));

fail:
	free(pBuffer);
	return FALSE;
}

static BOOL AtaIdentify(HANDLE hDisk, PVOID pData, PDWORD pdwSize)
{
	DWORD dwBytes;
	SENDCMDINPARAMS stCIP = { 0 };
	BYTE rawData[sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE] = { 0 };
	PSENDCMDOUTPARAMS pstCOP = (PSENDCMDOUTPARAMS)rawData;

	stCIP.cBufferSize = IDENTIFY_BUFFER_SIZE;
	stCIP.bDriveNumber = 0;
	stCIP.irDriveRegs.bFeaturesReg = 0;
	stCIP.irDriveRegs.bSectorCountReg = 1;
	stCIP.irDriveRegs.bSectorNumberReg = 1;
	stCIP.irDriveRegs.bCylLowReg = 0;
	stCIP.irDriveRegs.bCylHighReg = 0;
	stCIP.irDriveRegs.bDriveHeadReg = DRIVE_HEAD_REG;
	stCIP.irDriveRegs.bCommandReg = ID_CMD;
	if (!DeviceIoControl(hDisk, SMART_RCV_DRIVE_DATA,
		&stCIP, sizeof(stCIP), rawData, sizeof(rawData), &dwBytes, NULL))
		return FALSE;
	ZeroMemory(pData, READ_ATTRIBUTE_BUFFER_SIZE);
	memcpy(pData, pstCOP->bBuffer, pstCOP->cBufferSize);
	*pdwSize = pstCOP->cBufferSize;
	return TRUE;
}

static BOOL AtaEnableSmart(HANDLE hDisk)
{
	DWORD dwBytes = 0;
	SENDCMDINPARAMS stCIP = { 0 };
	SENDCMDOUTPARAMS stCOP = { 0 };

	stCIP.cBufferSize = 0;
	stCIP.bDriveNumber = 0;
	stCIP.irDriveRegs.bFeaturesReg = ENABLE_SMART;
	stCIP.irDriveRegs.bSectorCountReg = 1;
	stCIP.irDriveRegs.bSectorNumberReg = 1;
	stCIP.irDriveRegs.bCylLowReg = SMART_CYL_LOW;
	stCIP.irDriveRegs.bCylHighReg = SMART_CYL_HI;
	stCIP.irDriveRegs.bDriveHeadReg = DRIVE_HEAD_REG;
	stCIP.irDriveRegs.bCommandReg = SMART_CMD;

	return DeviceIoControl(hDisk, SMART_SEND_DRIVE_COMMAND,
		&stCIP, sizeof(stCIP), &stCOP, sizeof(stCOP), &dwBytes, NULL);
}

static BOOL AtaReadSmartAttr(HANDLE hDisk, BOOL Threshold,
	PVOID pData, PDWORD pdwSize)
{
	DWORD dwBytes;
	SENDCMDINPARAMS stCIP = { 0 };
	BYTE rawData[sizeof(SENDCMDOUTPARAMS) + READ_ATTRIBUTE_BUFFER_SIZE] = { 0 };
	PSENDCMDOUTPARAMS pstCOP = (PSENDCMDOUTPARAMS)rawData;

	stCIP.cBufferSize = READ_ATTRIBUTE_BUFFER_SIZE;
	stCIP.bDriveNumber = 0;
	stCIP.irDriveRegs.bFeaturesReg = Threshold ? READ_THRESHOLDS : READ_ATTRIBUTES;
	stCIP.irDriveRegs.bSectorCountReg = 1;
	stCIP.irDriveRegs.bSectorNumberReg = 1;
	stCIP.irDriveRegs.bCylLowReg = SMART_CYL_LOW;
	stCIP.irDriveRegs.bCylHighReg = SMART_CYL_HI;
	stCIP.irDriveRegs.bDriveHeadReg = DRIVE_HEAD_REG;
	stCIP.irDriveRegs.bCommandReg = SMART_CMD;
	if (!DeviceIoControl(hDisk, SMART_RCV_DRIVE_DATA,
		&stCIP, sizeof(stCIP), rawData, sizeof(rawData), &dwBytes, NULL))
		return FALSE;
	ZeroMemory(pData, READ_ATTRIBUTE_BUFFER_SIZE);
	memcpy(pData, pstCOP->bBuffer, pstCOP->cBufferSize);
	*pdwSize = pstCOP->cBufferSize;
	return TRUE;
}

static BOOL
AtaFindSmartAttr(PUCHAR pData, BYTE Id, PUINT64 pRaw)
{
	DWORD i;
	for (i = 0; i < 30; i++)
	{
		PATA_ATTRIBUTE pAtaAttr = (PATA_ATTRIBUTE)&pData[2 + i * sizeof(ATA_ATTRIBUTE)];
		if (Id != pAtaAttr->bAttrID)
			continue;
		*pRaw = 0;
		memcpy(pRaw, pAtaAttr->bRawValue, sizeof(pAtaAttr->bRawValue));
		return TRUE;
	}
	return FALSE;
}

static BOOL
GetAtaData(PNODE pNode, HANDLE hDisk)
{
	UINT64 ullRaw;
	DWORD dwBytes, i;
	GETVERSIONINPARAMS gvParam = { 0 };
	UCHAR curAttr[READ_ATTRIBUTE_BUFFER_SIZE];
	UCHAR trsAttr[READ_THRESHOLD_BUFFER_SIZE];
	IDENTIFY_DEVICE_DATA idAta = { 0 };

	ZeroMemory(curAttr, sizeof(curAttr));
	ZeroMemory(trsAttr, sizeof(trsAttr));

	if (AtaIdentify(hDisk, &idAta, &dwBytes))
	{
		NWL_NodeAttrSetf(pNode, "Rotation Rate (RPM)", NAFLG_FMT_NUMERIC, "%u", idAta.NominalMediaRotationRate);
	}

	if (!DeviceIoControl(hDisk, SMART_GET_VERSION,
		NULL, 0, &gvParam, sizeof(GETVERSIONINPARAMS), &dwBytes, NULL))
		return FALSE;

	NWL_NodeAttrSetBool(pNode, "S.M.A.R.T. Support", (gvParam.fCapabilities & CAP_SMART_CMD), 0);
	if (!(gvParam.fCapabilities & CAP_SMART_CMD))
		return TRUE;

	// Enable SMART
	if (!AtaEnableSmart(hDisk))
		return FALSE;

	// Read SMART Attributes
	if (!AtaReadSmartAttr(hDisk, FALSE, curAttr, &dwBytes))
		return FALSE;
	if (!AtaReadSmartAttr(hDisk, TRUE, trsAttr, &dwBytes))
		return FALSE;

	if (AtaFindSmartAttr(curAttr, 0xc2, &ullRaw))
		NWL_NodeAttrSetf(pNode, "Temperature (C)", NAFLG_FMT_NUMERIC, "%llu", ullRaw & 0xFF);
	if (AtaFindSmartAttr(curAttr, 0x0c, &ullRaw))
		NWL_NodeAttrSetf(pNode, "Power On Count", NAFLG_FMT_NUMERIC, "%llu", ullRaw);
	if (AtaFindSmartAttr(curAttr, 0x09, &ullRaw))
		NWL_NodeAttrSetf(pNode, "Power On Time (Hours)", NAFLG_FMT_NUMERIC, "%llu", ullRaw);

	for (i = 0; i < 30; i++)
	{
		CHAR tmp[64];
		DWORD dwFlag;
		PATA_ATTRIBUTE pAtaAttr = (PATA_ATTRIBUTE)&curAttr[2 + i * sizeof(ATA_ATTRIBUTE)];
		PATA_THRESHOLD pAtaThrs = (PATA_THRESHOLD)&trsAttr[2 + i * sizeof(ATA_THRESHOLD)];
		if (!pAtaAttr->bAttrID)
			continue;
		snprintf(tmp, 64, "[%02X] %s", pAtaAttr->bAttrID, GetSmartAttr(pAtaAttr->bAttrID, &dwFlag));
		ullRaw = 0;
		memcpy(&ullRaw, pAtaAttr->bRawValue, sizeof(pAtaAttr->bRawValue));
		NWL_NodeAttrSetf(pNode, tmp, 0, "[%s] %012llX (Current=%u Worst=%u Threshold=%u)",
			GetSmartWarn(pAtaAttr->bAttrID, ullRaw), ullRaw,
			pAtaAttr->bAttrValue, pAtaAttr->bWorstValue, pAtaThrs->bWarrantyThreshold);
	}

	return TRUE;
}

VOID NWL_GetDiskProtocolSpecificInfo(PNODE pNode, DWORD dwIndex, STORAGE_BUS_TYPE busType)
{
	HANDLE hDisk = NWL_GetDiskHandleById(FALSE, TRUE, dwIndex);
	if (!hDisk || hDisk == INVALID_HANDLE_VALUE)
		return;
	switch (busType)
	{
	case BusTypeNvme:
		GetTrimData(pNode, hDisk);
		GetNvmeData(pNode, hDisk);
		break;
	case BusTypeAtapi:
	case BusTypeAta:
	case BusTypeSata:
		GetTrimData(pNode, hDisk);
		GetAtaData(pNode, hDisk);
		break;
	case BusTypeScsi:
	default:
		break;
	}
	CloseHandle(hDisk);
}
