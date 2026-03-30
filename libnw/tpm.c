// SPDX-License-Identifier: Unlicense

#define VC_EXTRALEAN
#include <windows.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tpm.h"
#include "utils.h"

#define TPM_IDS_IMPL
#include "tpm_ids.h"

typedef struct _NWL_TPM_ALGORITHM_NAME_ENTRY
{
	UINT32 Id;
	LPCSTR Name;
} NWL_TPM_ALGORITHM_NAME_ENTRY;

enum
{
	NWL_TPM_TAG_SIZE = sizeof(UINT16),
	NWL_TPM_UINT32_SIZE = sizeof(UINT32),
	NWL_TPM_COMMAND_HEADER_SIZE = sizeof(UINT16) + sizeof(UINT32) + sizeof(UINT32),
	NWL_TPM12_GET_CAPABILITY_COMMAND_BASE_SIZE = NWL_TPM_COMMAND_HEADER_SIZE + sizeof(UINT32) + sizeof(UINT32),
	NWL_TPM12_GET_CAPABILITY_COMMAND_MAX_SIZE = NWL_TPM12_GET_CAPABILITY_COMMAND_BASE_SIZE + sizeof(UINT32),
	NWL_TPM12_GET_CAPABILITY_RESPONSE_HEADER_SIZE = NWL_TPM_COMMAND_HEADER_SIZE + sizeof(UINT32),
	NWL_TPM12_VERSION_INFO_MIN_SIZE = sizeof(UINT16) + 4U + sizeof(UINT16) + 1U + 4U + sizeof(UINT16),
	NWL_TPM2_GET_CAPABILITY_COMMAND_SIZE = NWL_TPM_COMMAND_HEADER_SIZE + (sizeof(UINT32) * 3U),
	NWL_TPM2_CAPABILITY_HEADER_SIZE = 1U + sizeof(UINT32) + sizeof(UINT32),
	NWL_TPM2_TAGGED_PROPERTY_SIZE = sizeof(UINT32) + sizeof(UINT32),
	NWL_TPM2_ALG_PROPERTY_SIZE = sizeof(UINT16) + sizeof(UINT32),
	NWL_TPM2_ALG_QUERY_BATCH_SIZE = 64U,
};

enum
{
	NWL_TPM12_TAG_RQU_COMMAND = 0x00C1U,
	NWL_TPM12_TAG_RSP_COMMAND = 0x00C4U,
	NWL_TPM12_TAG_CAP_VERSION_INFO = 0x0030U,
	NWL_TPM12_ORD_GET_CAPABILITY = 0x00000065U,
	NWL_TPM12_CAP_ALG = 0x00000002U,
	NWL_TPM12_CAP_PROPERTY = 0x00000005U,
	NWL_TPM12_CAP_VERSION_VAL = 0x0000001AU,
	NWL_TPM12_CAP_PROP_MANUFACTURER = 0x00000103U,
	NWL_TPM12_BOOL_TRUE = 0x01U,
};

enum
{
	NWL_TPM2_ST_NO_SESSIONS = 0x8001U,
	NWL_TPM2_CC_GET_CAPABILITY = 0x0000017AU,
	NWL_TPM2_CAP_ALGS = 0x00000000U,
	NWL_TPM2_CAP_TPM_PROPERTIES = 0x00000006U,
	NWL_TPM2_PT_FAMILY_INDICATOR = 0x00000100U,
	NWL_TPM2_PT_LEVEL = 0x00000101U,
	NWL_TPM2_PT_REVISION = 0x00000102U,
	NWL_TPM2_PT_DAY_OF_YEAR = 0x00000103U,
	NWL_TPM2_PT_YEAR = 0x00000104U,
	NWL_TPM2_PT_MANUFACTURER = 0x00000105U,
	NWL_TPM2_PT_VENDOR_STRING_1 = 0x00000106U,
	NWL_TPM2_PT_VENDOR_STRING_2 = 0x00000107U,
	NWL_TPM2_PT_VENDOR_STRING_3 = 0x00000108U,
	NWL_TPM2_PT_VENDOR_STRING_4 = 0x00000109U,
	NWL_TPM2_PT_FIRMWARE_VERSION_1 = 0x0000010BU,
	NWL_TPM2_PT_FIRMWARE_VERSION_2 = 0x0000010CU,
	NWL_TPM2_FIXED_PROPERTY_COUNT = (NWL_TPM2_PT_FIRMWARE_VERSION_2 - NWL_TPM2_PT_FAMILY_INDICATOR) + 1U,
};

static const NWL_TPM_ALGORITHM_NAME_ENTRY TPM12_ALGORITHMS[] =
{
	{ 0x00000001U, "RSA" },
	{ 0x00000004U, "SHA1" },
	{ 0x00000005U, "HMAC" },
	{ 0x00000006U, "AES128" },
	{ 0x00000007U, "MGF1" },
	{ 0x00000008U, "AES192" },
	{ 0x00000009U, "AES256" },
	{ 0x0000000AU, "XOR" },
};

static const NWL_TPM_ALGORITHM_NAME_ENTRY TPM20_ALGORITHMS[] =
{
	{ 0x00000001U, "RSA" },
	{ 0x00000003U, "TDES" },
	{ 0x00000004U, "SHA1" },
	{ 0x00000005U, "HMAC" },
	{ 0x00000006U, "AES" },
	{ 0x00000007U, "MGF1" },
	{ 0x00000008U, "KEYEDHASH" },
	{ 0x0000000AU, "XOR" },
	{ 0x0000000BU, "SHA256" },
	{ 0x0000000CU, "SHA384" },
	{ 0x0000000DU, "SHA512" },
	{ 0x00000010U, "NULL" },
	{ 0x00000012U, "SM3_256" },
	{ 0x00000013U, "SM4" },
	{ 0x00000014U, "RSASSA" },
	{ 0x00000015U, "RSAES" },
	{ 0x00000016U, "RSAPSS" },
	{ 0x00000017U, "OAEP" },
	{ 0x00000018U, "ECDSA" },
	{ 0x00000019U, "ECDH" },
	{ 0x0000001AU, "ECDAA" },
	{ 0x0000001BU, "SM2" },
	{ 0x0000001CU, "ECSCHNORR" },
	{ 0x0000001DU, "ECMQV" },
	{ 0x0000001FU, "HKDF" },
	{ 0x00000020U, "KDF1_SP800_56A" },
	{ 0x00000021U, "KDF2" },
	{ 0x00000022U, "KDF1_SP800_108" },
	{ 0x00000023U, "ECC" },
	{ 0x00000025U, "SYMCIPHER" },
	{ 0x00000026U, "CAMELLIA" },
	{ 0x00000027U, "SHA3_256" },
	{ 0x00000028U, "SHA3_384" },
	{ 0x00000029U, "SHA3_512" },
	{ 0x0000003FU, "CMAC" },
	{ 0x00000040U, "CTR" },
	{ 0x00000041U, "OFB" },
	{ 0x00000042U, "CBC" },
	{ 0x00000043U, "CFB" },
	{ 0x00000044U, "ECB" },
	{ 0x00000060U, "EDDSA" },
	{ 0x00000061U, "HASH_EDDSA" },
	{ 0x000000A0U, "MLKEM" },
	{ 0x000000A1U, "MLDSA" },
	{ 0x000000A2U, "HASH_MLDSA" },
};

static UINT16
NWL_TPMReadBE16(PCBYTE Buffer)
{
	return (UINT16)(((UINT16)Buffer[0] << 8) | (UINT16)Buffer[1]);
}

static UINT32
NWL_TPMReadBE32(PCBYTE Buffer)
{
	return ((UINT32)Buffer[0] << 24) |
		((UINT32)Buffer[1] << 16) |
		((UINT32)Buffer[2] << 8) |
		(UINT32)Buffer[3];
}

static VOID
NWL_TPMWriteBE16(PBYTE Buffer, UINT16 Value)
{
	Buffer[0] = (BYTE)(Value >> 8);
	Buffer[1] = (BYTE)Value;
}

static VOID
NWL_TPMWriteBE32(PBYTE Buffer, UINT32 Value)
{
	Buffer[0] = (BYTE)(Value >> 24);
	Buffer[1] = (BYTE)(Value >> 16);
	Buffer[2] = (BYTE)(Value >> 8);
	Buffer[3] = (BYTE)Value;
}

static LPCSTR
NWL_TPMFindAlgorithmName(UINT32 TpmVersion, UINT32 AlgorithmId)
{
	const NWL_TPM_ALGORITHM_NAME_ENTRY* table = NULL;
	size_t count = 0;

	if (TpmVersion == TPM_VERSION_12)
	{
		table = TPM12_ALGORITHMS;
		count = ARRAYSIZE(TPM12_ALGORITHMS);
	}
	else if (TpmVersion == TPM_VERSION_20)
	{
		table = TPM20_ALGORITHMS;
		count = ARRAYSIZE(TPM20_ALGORITHMS);
	}
	else
	{
		return NULL;
	}

	for (size_t i = 0; i < count; i++)
	{
		if (table[i].Id == AlgorithmId)
			return table[i].Name;
	}

	return NULL;
}

static VOID
NWL_TPMU32ToAscii(UINT32 Value, CHAR* Text, size_t TextSize)
{
	const BYTE bytes[4] =
	{
		(BYTE)(Value >> 24),
		(BYTE)(Value >> 16),
		(BYTE)(Value >> 8),
		(BYTE)Value
	};
	size_t length = 0;

	if (Text == NULL || TextSize == 0)
		return;

	for (size_t i = 0; i < ARRAYSIZE(bytes) && length + 1 < TextSize; i++)
	{
		if (bytes[i] == 0)
			break;
		if (!isprint(bytes[i]))
			break;
		Text[length++] = (CHAR)bytes[i];
	}

	Text[length] = '\0';
}

static VOID
NWL_TPMAppendU32Ascii(CHAR* Text, size_t TextSize, size_t* Length, UINT32 Value)
{
	const BYTE bytes[4] =
	{
		(BYTE)(Value >> 24),
		(BYTE)(Value >> 16),
		(BYTE)(Value >> 8),
		(BYTE)Value
	};
	size_t currentLength = (Length == NULL) ? 0 : *Length;

	if (Text == NULL || TextSize == 0 || Length == NULL || currentLength >= TextSize)
		return;

	for (size_t i = 0; i < ARRAYSIZE(bytes) && currentLength + 1 < TextSize; i++)
	{
		if (bytes[i] == 0)
			break;
		if (!isprint(bytes[i]))
			break;
		Text[currentLength++] = (CHAR)bytes[i];
	}

	Text[currentLength] = '\0';
	*Length = currentLength;
}

static VOID
NWL_TPMSetManufacturerId(PNWL_TPM_INFO Info, UINT32 Value)
{
	Info->ManufacturerId[0] = (CHAR)(BYTE)(Value >> 24);
	Info->ManufacturerId[1] = (CHAR)(BYTE)(Value >> 16);
	Info->ManufacturerId[2] = (CHAR)(BYTE)(Value >> 8);
	Info->ManufacturerId[3] = (CHAR)(BYTE)Value;
	Info->ManufacturerId[4] = '\0';
	Info->ManufacturerName = Info->ManufacturerId;

	for (size_t i = 0; i < ARRAYSIZE(TPM_VENDOR_ID_LIST); i++)
	{
		if (strcmp(TPM_VENDOR_ID_LIST[i].id, Info->ManufacturerId) == 0)
		{
			Info->ManufacturerName = TPM_VENDOR_ID_LIST[i].name;
			break;
		}
	}
}

static UINT32
NWL_TPM12VersionByteToInteger(BYTE Value)
{
	if ((Value & 0xF0U) == 0U)
		return (UINT32)(Value & 0x0FU);
	return ((UINT32)(Value & 0x0FU) * 10U) + (UINT32)((Value >> 4) & 0x0FU);
}

static VOID
NWL_TPMFormatFirmware12(PNWL_TPM_INFO Info)
{
	snprintf(Info->FirmwareVersion, sizeof(Info->FirmwareVersion), "%u.%u",
		Info->FirmwareVersion1, Info->FirmwareVersion2);
}

static VOID
NWL_TPMFormatFirmware20(PNWL_TPM_INFO Info)
{
	snprintf(Info->FirmwareVersion, sizeof(Info->FirmwareVersion), "%u.%u.%u.%u",
		(unsigned)(Info->FirmwareVersion1 >> 16),
		(unsigned)(Info->FirmwareVersion1 & 0xFFFFU),
		(unsigned)(Info->FirmwareVersion2 >> 16),
		(unsigned)(Info->FirmwareVersion2 & 0xFFFFU));
}

static BOOL
NWL_TPMAppendAlgorithm(PNWL_TPM_INFO Info, UINT32 AlgorithmId)
{
	PNWL_TPM_ALGORITHM algorithms = NULL;
	UINT32 newCount = 0;

	for (UINT32 i = 0; i < Info->AlgorithmCount; i++)
	{
		if (Info->Algorithms[i].Id == AlgorithmId)
			return TRUE;
	}

	newCount = Info->AlgorithmCount + 1U;
	algorithms = (PNWL_TPM_ALGORITHM)realloc(Info->Algorithms, newCount * sizeof(*algorithms));
	if (algorithms == NULL)
		return FALSE;

	Info->Algorithms = algorithms;
	Info->Algorithms[Info->AlgorithmCount].Id = AlgorithmId;
	Info->Algorithms[Info->AlgorithmCount].Name[0] = '\0';

	LPCSTR name = NWL_TPMFindAlgorithmName(Info->TpmVersion, AlgorithmId);
	if (name != NULL)
	{
		strncpy_s(Info->Algorithms[Info->AlgorithmCount].Name,
			sizeof(Info->Algorithms[Info->AlgorithmCount].Name), name, _TRUNCATE);
	}
	else
	{
		snprintf(Info->Algorithms[Info->AlgorithmCount].Name,
			sizeof(Info->Algorithms[Info->AlgorithmCount].Name), "0x%08X", AlgorithmId);
	}

	Info->AlgorithmCount = newCount;
	return TRUE;
}

static TBS_RESULT
NWL_TPMOpenContext(PTBS_HCONTEXT Context)
{
#if defined(TBS_CONTEXT_VERSION_TWO)
	TBS_CONTEXT_PARAMS2 params2 = { 0 };
	params2.version = TBS_CONTEXT_VERSION_TWO;
	params2.includeTpm12 = 1;
	params2.includeTpm20 = 1;
	TBS_RESULT rc = NWL_TPMContextCreate((PCTBS_CONTEXT_PARAMS)&params2, Context);
	if (rc == TBS_SUCCESS)
		return rc;
#endif
	TBS_CONTEXT_PARAMS params = { 0 };
	params.version = TBS_CONTEXT_VERSION_ONE;
	return NWL_TPMContextCreate(&params, Context);
}

static TBS_RESULT
NWL_TPM12ParseCapabilityResponse(PCBYTE Response, UINT32 ResponseSize, PCBYTE* Data, PUINT32 DataSize)
{
	UINT16 tag = 0;
	UINT32 totalSize = 0;
	UINT32 returnCode = 0;
	UINT32 payloadSize = 0;

	if (Response == NULL || Data == NULL || DataSize == NULL)
		return TBS_E_BAD_PARAMETER;
	if (ResponseSize < NWL_TPM12_GET_CAPABILITY_RESPONSE_HEADER_SIZE)
		return TBS_E_INTERNAL_ERROR;

	tag = NWL_TPMReadBE16(Response);
	totalSize = NWL_TPMReadBE32(Response + NWL_TPM_TAG_SIZE);
	returnCode = NWL_TPMReadBE32(Response + NWL_TPM_TAG_SIZE + NWL_TPM_UINT32_SIZE);
	if (tag != NWL_TPM12_TAG_RSP_COMMAND || totalSize != ResponseSize || returnCode != 0)
		return TBS_E_INTERNAL_ERROR;

	payloadSize = NWL_TPMReadBE32(Response + NWL_TPM_COMMAND_HEADER_SIZE);
	if (ResponseSize < NWL_TPM12_GET_CAPABILITY_RESPONSE_HEADER_SIZE + payloadSize)
		return TBS_E_INTERNAL_ERROR;

	*Data = Response + NWL_TPM12_GET_CAPABILITY_RESPONSE_HEADER_SIZE;
	*DataSize = payloadSize;
	return TBS_SUCCESS;
}

static TBS_RESULT
NWL_TPM12GetCapability(TBS_HCONTEXT Context, UINT32 Capability, PCBYTE SubCap, UINT32 SubCapSize,
	PBYTE Response, PUINT32 ResponseSize)
{
	BYTE command[NWL_TPM12_GET_CAPABILITY_COMMAND_MAX_SIZE] = { 0 };
	UINT32 commandSize = NWL_TPM12_GET_CAPABILITY_COMMAND_BASE_SIZE + SubCapSize;

	if (SubCapSize > sizeof(UINT32))
		return TBS_E_BAD_PARAMETER;

	NWL_TPMWriteBE16(command, NWL_TPM12_TAG_RQU_COMMAND);
	NWL_TPMWriteBE32(command + NWL_TPM_TAG_SIZE, commandSize);
	NWL_TPMWriteBE32(command + NWL_TPM_TAG_SIZE + NWL_TPM_UINT32_SIZE, NWL_TPM12_ORD_GET_CAPABILITY);
	NWL_TPMWriteBE32(command + NWL_TPM_COMMAND_HEADER_SIZE, Capability);
	NWL_TPMWriteBE32(command + NWL_TPM_COMMAND_HEADER_SIZE + NWL_TPM_UINT32_SIZE, SubCapSize);
	if (SubCapSize != 0 && SubCap != NULL)
		memcpy(command + NWL_TPM12_GET_CAPABILITY_COMMAND_BASE_SIZE, SubCap, SubCapSize);

	return NWL_TPMSubmitCommand(Context, TBS_COMMAND_LOCALITY_ZERO, TBS_COMMAND_PRIORITY_NORMAL,
		command, commandSize, Response, ResponseSize);
}

static TBS_RESULT
NWL_TPM12QueryVersionInfo(TBS_HCONTEXT Context, PBYTE Response, UINT32 ResponseCapacity, PNWL_TPM_INFO Info)
{
	TBS_RESULT rc = 0;
	UINT32 responseSize = ResponseCapacity;
	PCBYTE data = NULL;
	UINT32 dataSize = 0;
	UINT16 tag = 0;
	BYTE major = 0;
	BYTE minor = 0;
	UINT32 vendorSpecificSize = 0;

	rc = NWL_TPM12GetCapability(Context, NWL_TPM12_CAP_VERSION_VAL, NULL, 0, Response, &responseSize);
	if (rc != TBS_SUCCESS)
		return rc;

	rc = NWL_TPM12ParseCapabilityResponse(Response, responseSize, &data, &dataSize);
	if (rc != TBS_SUCCESS)
		return rc;
	if (dataSize < NWL_TPM12_VERSION_INFO_MIN_SIZE)
		return TBS_E_INTERNAL_ERROR;

	tag = NWL_TPMReadBE16(data);
	if (tag != NWL_TPM12_TAG_CAP_VERSION_INFO)
		return TBS_E_INTERNAL_ERROR;

	major = data[2];
	minor = data[3];
	Info->FirmwareVersion1 = (UINT32)data[4];
	Info->FirmwareVersion2 = (UINT32)data[5];
	Info->SpecLevel = (UINT32)NWL_TPMReadBE16(data + 6);
	Info->SpecRevision = (UINT32)data[8];
	NWL_TPMSetManufacturerId(Info, NWL_TPMReadBE32(data + 9));
	vendorSpecificSize = (UINT32)NWL_TPMReadBE16(data + 13);
	if (dataSize < NWL_TPM12_VERSION_INFO_MIN_SIZE + vendorSpecificSize)
		return TBS_E_INTERNAL_ERROR;
	Info->SpecYear = 0;
	Info->SpecDayOfYear = 0;

	snprintf(Info->SpecFamily, sizeof(Info->SpecFamily), "%u.%u",
		(unsigned)NWL_TPM12VersionByteToInteger(major),
		(unsigned)NWL_TPM12VersionByteToInteger(minor));
	NWL_TPMFormatFirmware12(Info);

	return TBS_SUCCESS;
}

static TBS_RESULT
NWL_TPM12QueryManufacturerProperty(TBS_HCONTEXT Context, PBYTE Response, UINT32 ResponseCapacity, PNWL_TPM_INFO Info)
{
	BYTE subCap[sizeof(UINT32)] = { 0 };
	TBS_RESULT rc = 0;
	UINT32 responseSize = ResponseCapacity;
	PCBYTE data = NULL;
	UINT32 dataSize = 0;

	NWL_TPMWriteBE32(subCap, NWL_TPM12_CAP_PROP_MANUFACTURER);
	rc = NWL_TPM12GetCapability(Context, NWL_TPM12_CAP_PROPERTY, subCap, sizeof(subCap), Response, &responseSize);
	if (rc != TBS_SUCCESS)
		return rc;

	rc = NWL_TPM12ParseCapabilityResponse(Response, responseSize, &data, &dataSize);
	if (rc != TBS_SUCCESS)
		return rc;
	if (dataSize != sizeof(UINT32))
		return TBS_E_INTERNAL_ERROR;

	NWL_TPMSetManufacturerId(Info, NWL_TPMReadBE32(data));

	return TBS_SUCCESS;
}

static TBS_RESULT
NWL_TPM12QueryAlgorithms(TBS_HCONTEXT Context, PBYTE Response, UINT32 ResponseCapacity, PNWL_TPM_INFO Info)
{
	BYTE subCap[sizeof(UINT32)] = { 0 };

	for (size_t i = 0; i < ARRAYSIZE(TPM12_ALGORITHMS); i++)
	{
		TBS_RESULT rc = 0;
		UINT32 responseSize = ResponseCapacity;
		PCBYTE data = NULL;
		UINT32 dataSize = 0;

		NWL_TPMWriteBE32(subCap, TPM12_ALGORITHMS[i].Id);
		rc = NWL_TPM12GetCapability(Context, NWL_TPM12_CAP_ALG, subCap, sizeof(subCap), Response, &responseSize);
		if (rc != TBS_SUCCESS)
			return rc;

		rc = NWL_TPM12ParseCapabilityResponse(Response, responseSize, &data, &dataSize);
		if (rc != TBS_SUCCESS)
			return rc;
		if (dataSize != sizeof(BYTE))
			return TBS_E_INTERNAL_ERROR;
		if (data[0] == NWL_TPM12_BOOL_TRUE && !NWL_TPMAppendAlgorithm(Info, TPM12_ALGORITHMS[i].Id))
			return TBS_E_INTERNAL_ERROR;
	}

	return TBS_SUCCESS;
}

static TBS_RESULT
NWL_TPM12GetInfo(TBS_HCONTEXT Context, PBYTE Response, UINT32 ResponseCapacity, PNWL_TPM_INFO Info)
{
	TBS_RESULT rc = NWL_TPM12QueryVersionInfo(Context, Response, ResponseCapacity, Info);
	if (rc != TBS_SUCCESS)
		return rc;

	rc = NWL_TPM12QueryManufacturerProperty(Context, Response, ResponseCapacity, Info);
	if (rc != TBS_SUCCESS)
		return rc;

	return NWL_TPM12QueryAlgorithms(Context, Response, ResponseCapacity, Info);
}

static TBS_RESULT
NWL_TPM20ParseCapabilityHeader(PCBYTE Response, UINT32 ResponseSize, PCBYTE* Data, PUINT32 DataSize)
{
	UINT16 tag = 0;
	UINT32 totalSize = 0;
	UINT32 responseCode = 0;

	if (Response == NULL || Data == NULL || DataSize == NULL)
		return TBS_E_BAD_PARAMETER;
	if (ResponseSize < NWL_TPM_COMMAND_HEADER_SIZE)
		return TBS_E_INTERNAL_ERROR;

	tag = NWL_TPMReadBE16(Response);
	totalSize = NWL_TPMReadBE32(Response + NWL_TPM_TAG_SIZE);
	responseCode = NWL_TPMReadBE32(Response + NWL_TPM_TAG_SIZE + NWL_TPM_UINT32_SIZE);
	if (tag != NWL_TPM2_ST_NO_SESSIONS || totalSize != ResponseSize || responseCode != 0)
		return TBS_E_INTERNAL_ERROR;

	*Data = Response + NWL_TPM_COMMAND_HEADER_SIZE;
	*DataSize = ResponseSize - NWL_TPM_COMMAND_HEADER_SIZE;
	return TBS_SUCCESS;
}

static TBS_RESULT
NWL_TPM20GetCapability(TBS_HCONTEXT Context, UINT32 Capability, UINT32 Property, UINT32 PropertyCount,
	PBYTE Response, PUINT32 ResponseSize)
{
	BYTE command[NWL_TPM2_GET_CAPABILITY_COMMAND_SIZE] = { 0 };

	NWL_TPMWriteBE16(command, NWL_TPM2_ST_NO_SESSIONS);
	NWL_TPMWriteBE32(command + NWL_TPM_TAG_SIZE, sizeof(command));
	NWL_TPMWriteBE32(command + NWL_TPM_TAG_SIZE + NWL_TPM_UINT32_SIZE, NWL_TPM2_CC_GET_CAPABILITY);
	NWL_TPMWriteBE32(command + NWL_TPM_COMMAND_HEADER_SIZE, Capability);
	NWL_TPMWriteBE32(command + NWL_TPM_COMMAND_HEADER_SIZE + NWL_TPM_UINT32_SIZE, Property);
	NWL_TPMWriteBE32(command + NWL_TPM_COMMAND_HEADER_SIZE + (NWL_TPM_UINT32_SIZE * 2U), PropertyCount);

	return NWL_TPMSubmitCommand(Context, TBS_COMMAND_LOCALITY_ZERO, TBS_COMMAND_PRIORITY_NORMAL,
		command, sizeof(command), Response, ResponseSize);
}

static TBS_RESULT
NWL_TPM20QueryProperties(TBS_HCONTEXT Context, PBYTE Response, UINT32 ResponseCapacity, PNWL_TPM_INFO Info)
{
	TBS_RESULT rc = 0;
	UINT32 responseSize = ResponseCapacity;
	PCBYTE data = NULL;
	UINT32 dataSize = 0;
	UINT32 count = 0;
	PCBYTE propertyData = NULL;
	size_t vendorStringLength = 0;

	rc = NWL_TPM20GetCapability(Context, NWL_TPM2_CAP_TPM_PROPERTIES,
		NWL_TPM2_PT_FAMILY_INDICATOR, NWL_TPM2_FIXED_PROPERTY_COUNT, Response, &responseSize);
	if (rc != TBS_SUCCESS)
		return rc;

	rc = NWL_TPM20ParseCapabilityHeader(Response, responseSize, &data, &dataSize);
	if (rc != TBS_SUCCESS)
		return rc;
	if (dataSize < NWL_TPM2_CAPABILITY_HEADER_SIZE)
		return TBS_E_INTERNAL_ERROR;
	if (NWL_TPMReadBE32(data + 1) != NWL_TPM2_CAP_TPM_PROPERTIES)
		return TBS_E_INTERNAL_ERROR;

	count = NWL_TPMReadBE32(data + 5);
	propertyData = data + NWL_TPM2_CAPABILITY_HEADER_SIZE;
	if (dataSize < NWL_TPM2_CAPABILITY_HEADER_SIZE + (count * NWL_TPM2_TAGGED_PROPERTY_SIZE))
		return TBS_E_INTERNAL_ERROR;

	for (UINT32 i = 0; i < count; i++)
	{
		UINT32 property = NWL_TPMReadBE32(propertyData);
		UINT32 value = NWL_TPMReadBE32(propertyData + sizeof(UINT32));

		switch (property)
		{
		case NWL_TPM2_PT_FAMILY_INDICATOR:
			NWL_TPMU32ToAscii(value, Info->SpecFamily, sizeof(Info->SpecFamily));
			break;
		case NWL_TPM2_PT_LEVEL:
			Info->SpecLevel = value;
			break;
		case NWL_TPM2_PT_REVISION:
			Info->SpecRevision = value;
			break;
		case NWL_TPM2_PT_DAY_OF_YEAR:
			Info->SpecDayOfYear = value;
			break;
		case NWL_TPM2_PT_YEAR:
			Info->SpecYear = value;
			break;
		case NWL_TPM2_PT_MANUFACTURER:
			NWL_TPMSetManufacturerId(Info, value);
			break;
		case NWL_TPM2_PT_VENDOR_STRING_1:
		case NWL_TPM2_PT_VENDOR_STRING_2:
		case NWL_TPM2_PT_VENDOR_STRING_3:
		case NWL_TPM2_PT_VENDOR_STRING_4:
			NWL_TPMAppendU32Ascii(Info->VendorString, sizeof(Info->VendorString), &vendorStringLength, value);
			break;
		case NWL_TPM2_PT_FIRMWARE_VERSION_1:
			Info->FirmwareVersion1 = value;
			break;
		case NWL_TPM2_PT_FIRMWARE_VERSION_2:
			Info->FirmwareVersion2 = value;
			break;
		}

		propertyData += NWL_TPM2_TAGGED_PROPERTY_SIZE;
	}

	NWL_TPMFormatFirmware20(Info);

	return TBS_SUCCESS;
}

static TBS_RESULT
NWL_TPM20QueryAlgorithms(TBS_HCONTEXT Context, PBYTE Response, UINT32 ResponseCapacity, PNWL_TPM_INFO Info)
{
	UINT32 nextAlgorithm = 0;

	for (;;)
	{
		TBS_RESULT rc = 0;
		UINT32 responseSize = ResponseCapacity;
		PCBYTE data = NULL;
		UINT32 dataSize = 0;
		UINT32 count = 0;
		PCBYTE algorithmData = NULL;
		BOOL moreData = FALSE;

		rc = NWL_TPM20GetCapability(Context, NWL_TPM2_CAP_ALGS,
			nextAlgorithm, NWL_TPM2_ALG_QUERY_BATCH_SIZE, Response, &responseSize);
		if (rc != TBS_SUCCESS)
			return rc;

		rc = NWL_TPM20ParseCapabilityHeader(Response, responseSize, &data, &dataSize);
		if (rc != TBS_SUCCESS)
			return rc;
		if (dataSize < NWL_TPM2_CAPABILITY_HEADER_SIZE)
			return TBS_E_INTERNAL_ERROR;
		if (NWL_TPMReadBE32(data + 1) != NWL_TPM2_CAP_ALGS)
			return TBS_E_INTERNAL_ERROR;

		moreData = data[0] != 0;
		count = NWL_TPMReadBE32(data + 5);
		algorithmData = data + NWL_TPM2_CAPABILITY_HEADER_SIZE;
		if (dataSize < NWL_TPM2_CAPABILITY_HEADER_SIZE + (count * NWL_TPM2_ALG_PROPERTY_SIZE))
			return TBS_E_INTERNAL_ERROR;
		if (count == 0 && moreData)
			return TBS_E_INTERNAL_ERROR;

		for (UINT32 i = 0; i < count; i++)
		{
			UINT32 algorithmId = (UINT32)NWL_TPMReadBE16(algorithmData);
			if (!NWL_TPMAppendAlgorithm(Info, algorithmId))
				return TBS_E_INTERNAL_ERROR;
			nextAlgorithm = algorithmId + 1U;
			algorithmData += NWL_TPM2_ALG_PROPERTY_SIZE;
		}

		if (!moreData)
			break;
	}

	return TBS_SUCCESS;
}

static TBS_RESULT
NWL_TPM20GetInfo(TBS_HCONTEXT Context, PBYTE Response, UINT32 ResponseCapacity, PNWL_TPM_INFO Info)
{
	TBS_RESULT rc = NWL_TPM20QueryProperties(Context, Response, ResponseCapacity, Info);
	if (rc != TBS_SUCCESS)
		return rc;
	return NWL_TPM20QueryAlgorithms(Context, Response, ResponseCapacity, Info);
}

LPCSTR NWL_TPMGetVersionStr(UINT32 Version)
{
	switch (Version)
	{
	case TPM_VERSION_12: return "v1.2";
	case TPM_VERSION_20: return "v2.0";
	default: return "UNKNOWN";
	}
}

TBS_RESULT NWL_TPMContextCreate(PCTBS_CONTEXT_PARAMS pContextParams, PTBS_HCONTEXT phContext)
{
	TBS_RESULT(WINAPI *TPM_Context_Create)(PCTBS_CONTEXT_PARAMS, PTBS_HCONTEXT) = NULL;
	TBS_RESULT rc = TBS_E_INTERNAL_ERROR;
	HMODULE hL = LoadLibraryW(L"tbs.dll");
	if (hL != NULL)
		*(FARPROC*)&TPM_Context_Create = GetProcAddress(hL, "Tbsi_Context_Create");
	if (TPM_Context_Create != NULL)
		rc = TPM_Context_Create(pContextParams, phContext);
	if (hL != NULL)
		FreeLibrary(hL);
	return rc;
}

TBS_RESULT NWL_TPMGetDeviceInfo(UINT32 Size, PVOID Info)
{
	TBS_RESULT(WINAPI *TPM_GetDeviceInfo) (UINT32, VOID*) = NULL;
	HMODULE hL = LoadLibraryW(L"tbs.dll");
	if (hL != NULL)
		*(FARPROC*)&TPM_GetDeviceInfo = GetProcAddress(hL, "Tbsi_GetDeviceInfo");
	if (TPM_GetDeviceInfo != NULL)
	{
		TBS_RESULT rc = TPM_GetDeviceInfo(Size, Info);
		FreeLibrary(hL);
		return rc;
	}
	if (hL != NULL)
		FreeLibrary(hL);

	UINT32 tpmVersion = TPM_VERSION_UNKNOWN;
	PVOID acpiTable = NWL_GetSysAcpi('2MPT');
	if (acpiTable)
		tpmVersion = TPM_VERSION_20;
	else
	{
		acpiTable = NWL_GetSysAcpi('APCT');
		if (acpiTable)
			tpmVersion = TPM_VERSION_12;
	}
	free(acpiTable);
	if (tpmVersion == TPM_VERSION_UNKNOWN)
		return TBS_E_TPM_NOT_FOUND;

	if (Size < sizeof(TPM_DEVICE_INFO) || Info == NULL)
		return TBS_E_BAD_PARAMETER;

	PTPM_DEVICE_INFO tpmInfo = (PTPM_DEVICE_INFO)Info;
	tpmInfo->structVersion = TPM_VERSION_20;
	tpmInfo->tpmVersion = tpmVersion;
	tpmInfo->tpmInterfaceType = TPM_IFTYPE_UNKNOWN;
	tpmInfo->tpmImpRevision = 0;

	return TBS_SUCCESS;
}

TBS_RESULT NWL_TPMContextClose(TBS_HCONTEXT hContext)
{
	TBS_RESULT(WINAPI *TPM_Context_Close)(TBS_HCONTEXT) = NULL;
	TBS_RESULT rc = TBS_E_INTERNAL_ERROR;
	HMODULE hL = LoadLibraryW(L"tbs.dll");
	if (hL != NULL)
		*(FARPROC*)&TPM_Context_Close = GetProcAddress(hL, "Tbsip_Context_Close");
	if (TPM_Context_Close != NULL)
		rc = TPM_Context_Close(hContext);
	if (hL != NULL)
		FreeLibrary(hL);
	return rc;
}

TBS_RESULT NWL_TPMSubmitCommand(TBS_HCONTEXT hContext,
	TBS_COMMAND_LOCALITY Locality, TBS_COMMAND_PRIORITY Priority, PCBYTE pabCommand, UINT32 cbCommand, PBYTE pabResult, PUINT32 pcbResult)
{
	TBS_RESULT(WINAPI *TPM_Submit_Command)(TBS_HCONTEXT, TBS_COMMAND_LOCALITY, TBS_COMMAND_PRIORITY, PCBYTE, UINT32, PBYTE, PUINT32) = NULL;
	TBS_RESULT rc = TBS_E_INTERNAL_ERROR;
	HMODULE hL = LoadLibraryW(L"tbs.dll");
	if (hL != NULL)
		*(FARPROC*)&TPM_Submit_Command = GetProcAddress(hL, "Tbsip_Submit_Command");
	if (TPM_Submit_Command != NULL)
		rc = TPM_Submit_Command(hContext, Locality, Priority, pabCommand, cbCommand, pabResult, pcbResult);
	if (hL != NULL)
		FreeLibrary(hL);
	return rc;
}

VOID NWL_TPMFreeInfo(PNWL_TPM_INFO Info)
{
	if (Info == NULL)
		return;

	free(Info->Algorithms);
	ZeroMemory(Info, sizeof(*Info));
}

TBS_RESULT NWL_TPMGetInfo(PNWL_TPM_INFO Info)
{
	TBS_RESULT rc = TBS_E_INTERNAL_ERROR;
	TPM_DEVICE_INFO deviceInfo = { 0 };
	NWL_TPM_INFO tempInfo = { 0 };
	TBS_HCONTEXT context = 0;
	PBYTE response = NULL;

	if (Info == NULL)
		return TBS_E_BAD_PARAMETER;

	deviceInfo.structVersion = TPM_VERSION_20;
	deviceInfo.tpmVersion = TPM_VERSION_UNKNOWN;
	rc = NWL_TPMGetDeviceInfo(sizeof(deviceInfo), &deviceInfo);
	if (rc != TBS_SUCCESS)
		return rc;

	tempInfo.TpmVersion = deviceInfo.tpmVersion;
	response = (PBYTE)malloc(TBS_IN_OUT_BUF_SIZE_MAX);
	if (response == NULL)
		return TBS_E_INTERNAL_ERROR;

	rc = NWL_TPMOpenContext(&context);
	if (rc != TBS_SUCCESS)
		goto cleanup;

	switch (tempInfo.TpmVersion)
	{
	case TPM_VERSION_12:
		rc = NWL_TPM12GetInfo(context, response, TBS_IN_OUT_BUF_SIZE_MAX, &tempInfo);
		break;
	case TPM_VERSION_20:
		rc = NWL_TPM20GetInfo(context, response, TBS_IN_OUT_BUF_SIZE_MAX, &tempInfo);
		break;
	default:
		rc = TBS_E_TPM_NOT_FOUND;
		break;
	}

cleanup:
	if (context != 0)
		NWL_TPMContextClose(context);
	free(response);

	if (rc != TBS_SUCCESS)
	{
		NWL_TPMFreeInfo(&tempInfo);
		return rc;
	}

	*Info = tempInfo;
	return TBS_SUCCESS;
}
