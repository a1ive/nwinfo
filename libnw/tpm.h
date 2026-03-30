// SPDX-License-Identifier: Unlicense
#pragma once

#include <tbs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NWL_TPM_VENDOR_STRING_LEN 64U
#define NWL_TPM_FIRMWARE_STRING_LEN 64U
#define NWL_TPM_SPEC_FAMILY_LEN 16U
#define NWL_TPM_ALGORITHM_NAME_LEN 32U
#define NWL_TPM_MANUFACTURER_ID_LEN 5U

typedef struct _NWL_TPM_ALGORITHM
{
	UINT32 Id;
	CHAR Name[NWL_TPM_ALGORITHM_NAME_LEN];
} NWL_TPM_ALGORITHM, *PNWL_TPM_ALGORITHM;

typedef struct _NWL_TPM_INFO
{
	UINT32 TpmVersion;
	CHAR ManufacturerId[NWL_TPM_MANUFACTURER_ID_LEN];
	LPCSTR ManufacturerName;
	CHAR VendorString[NWL_TPM_VENDOR_STRING_LEN];
	UINT32 FirmwareVersion1;
	UINT32 FirmwareVersion2;
	CHAR FirmwareVersion[NWL_TPM_FIRMWARE_STRING_LEN];
	CHAR SpecFamily[NWL_TPM_SPEC_FAMILY_LEN];
	UINT32 SpecLevel;
	UINT32 SpecRevision;
	UINT32 SpecYear;
	UINT32 SpecDayOfYear;
	PNWL_TPM_ALGORITHM Algorithms;
	UINT32 AlgorithmCount;
} NWL_TPM_INFO, *PNWL_TPM_INFO;

LPCSTR NWL_TPMGetVersionStr(UINT32 Version);
TBS_RESULT NWL_TPMContextCreate(PCTBS_CONTEXT_PARAMS pContextParams, PTBS_HCONTEXT phContext);
TBS_RESULT NWL_TPMGetDeviceInfo(UINT32 Size, PVOID Info);
TBS_RESULT NWL_TPMContextClose(TBS_HCONTEXT hContext);
TBS_RESULT NWL_TPMSubmitCommand(TBS_HCONTEXT hContext,
	TBS_COMMAND_LOCALITY Locality, TBS_COMMAND_PRIORITY Priority, PCBYTE pabCommand, UINT32 cbCommand, PBYTE pabResult, PUINT32 pcbResult);
VOID NWL_TPMFreeInfo(PNWL_TPM_INFO Info);
TBS_RESULT NWL_TPMGetInfo(PNWL_TPM_INFO Info);

#ifdef __cplusplus
} /* extern "C" */
#endif
