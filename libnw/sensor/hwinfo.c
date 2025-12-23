// SPDX-License-Identifier: Unlicense

#include "../libnw.h"
#include "../utils.h"
#include "sensors.h"
#include "shmem.h"

#define HWiNFO_SENSORS_MAP_FILE_NAME2         L"Global\\HWiNFO_SENS_SM2"

#define HWiNFO_SENSORS_SM2_MUTEX              L"Global\\HWiNFO_SM2_MUTEX"

#define HWiNFO_SENSORS_STRING_LEN2  128
#define HWiNFO_UNIT_STRING_LEN       16

enum SENSOR_READING_TYPE
{
	SENSOR_TYPE_NONE = 0,
	SENSOR_TYPE_TEMP,
	SENSOR_TYPE_VOLT,
	SENSOR_TYPE_FAN,
	SENSOR_TYPE_CURRENT,
	SENSOR_TYPE_POWER,
	SENSOR_TYPE_CLOCK,
	SENSOR_TYPE_USAGE,
	SENSOR_TYPE_OTHER
};

#pragma pack(1)

typedef struct _HWiNFO_SENSORS_READING_ELEMENT
{
	enum SENSOR_READING_TYPE tReading;                 // Type of sensor reading
	DWORD dwSensorIndex;                               // This is the index of sensor in the Sensors[] array to which this reading belongs to
	DWORD dwReadingID;                                 // A unique ID of the reading within a particular sensor
	char szLabelOrig[HWiNFO_SENSORS_STRING_LEN2];      // Original label (e.g. "Chassis2 Fan") in English language [ANSI string]
	char szLabelUser[HWiNFO_SENSORS_STRING_LEN2];      // Label displayed, which might have been renamed by user [ANSI string]
	char szUnit[HWiNFO_UNIT_STRING_LEN];               // e.g. "RPM" [ANSI string]
	double Value;
	double ValueMin;
	double ValueMax;
	double ValueAvg;

	// Version 2+ new:
	BYTE utfLabelUser[HWiNFO_SENSORS_STRING_LEN2];     // Label displayed, which might be translated or renamed by user [UTF-8 string]
	BYTE utfUnit[HWiNFO_UNIT_STRING_LEN];              // e.g. "RPM" [UTF-8 string]
} HWiNFO_SENSORS_READING_ELEMENT, * PHWiNFO_SENSORS_READING_ELEMENT;

typedef struct _HWiNFO_SENSORS_SENSOR_ELEMENT
{
	DWORD dwSensorID;                                  // A unique Sensor ID
	DWORD dwSensorInst;                                // The instance of the sensor (together with dwSensorID forms a unique ID)
	char szSensorNameOrig[HWiNFO_SENSORS_STRING_LEN2]; // Original sensor name in English language [ANSI string]
	char szSensorNameUser[HWiNFO_SENSORS_STRING_LEN2]; // Sensor name displayed, which might have been renamed by user [ANSI string]

	// Version 2+ new:
	BYTE utfSensorNameUser[HWiNFO_SENSORS_STRING_LEN2]; // Sensor name displayed, which might be translated or renamed by user [UTF-8 string]
} HWiNFO_SENSORS_SENSOR_ELEMENT, * PHWiNFO_SENSORS_SENSOR_ELEMENT;

typedef struct _HWiNFO_SENSORS_SHARED_MEM2
{
	DWORD dwSignature;             // "HWiS" if active, 'DEAD' when inactive
	DWORD dwVersion;               // Structure layout version. 1=Initial; 2=Added UTF-8 strings (HWiNFO v7.33+)
	DWORD dwRevision;              // 0: Initial layout (HWiNFO ver <= 6.11)
	// 1: Added (HWiNFO v6.11-3917)
	__time64_t poll_time;          // last polling time

	// descriptors for the Sensors section
	DWORD dwOffsetOfSensorSection; // Offset of the Sensor section from beginning of HWiNFO_SENSORS_SHARED_MEM2
	DWORD dwSizeOfSensorElement;   // Size of each sensor element = sizeof( HWiNFO_SENSORS_SENSOR_ELEMENT )
	DWORD dwNumSensorElements;     // Number of sensor elements

	// descriptors for the Readings section
	DWORD dwOffsetOfReadingSection; // Offset of the Reading section from beginning of HWiNFO_SENSORS_SHARED_MEM2
	DWORD dwSizeOfReadingElement;   // Size of each Reading element = sizeof( HWiNFO_SENSORS_READING_ELEMENT )
	DWORD dwNumReadingElements;     // Number of Reading elements

	DWORD dwPollingPeriod;          // Current sensor polling period in HWiNFO. This variable is present since dwRevision=1 (HWiNFO v6.11) or later
} HWiNFO_SENSORS_SHARED_MEM2, * PHWiNFO_SENSORS_SHARED_MEM2;

#pragma pack()

static struct
{
	struct wr0_shmem_t shmem;
	PHWiNFO_SENSORS_SHARED_MEM2 hdr;
	PHWiNFO_SENSORS_SENSOR_ELEMENT sensors;
	PHWiNFO_SENSORS_READING_ELEMENT readings;
} ctx;

static bool hwinfo_init(void)
{
	if (WR0_OpenShMem(&ctx.shmem, HWiNFO_SENSORS_MAP_FILE_NAME2))
		goto fail;
	if (ctx.shmem.size < sizeof(HWiNFO_SENSORS_SHARED_MEM2))
		goto fail;
	ctx.hdr = ctx.shmem.addr;
	return true;
fail:
	WR0_CloseShMem(&ctx.shmem);
	ZeroMemory(&ctx, sizeof(ctx));
	return false;
}

static void hwinfo_fini(void)
{
	WR0_CloseShMem(&ctx.shmem);
	ZeroMemory(&ctx, sizeof(ctx));
}

static void hwinfo_get(PNODE node)
{
	ctx.sensors = (PHWiNFO_SENSORS_SENSOR_ELEMENT)((PUINT8)ctx.shmem.addr + ctx.hdr->dwOffsetOfSensorSection);
	ctx.readings = (PHWiNFO_SENSORS_READING_ELEMENT)((PUINT8)ctx.shmem.addr + ctx.hdr->dwOffsetOfReadingSection);

	for (DWORD i = 0; i < ctx.hdr->dwNumSensorElements; i++)
	{
		PHWiNFO_SENSORS_SENSOR_ELEMENT sensor = &ctx.sensors[i];
		NWL_NodeAppendNew(node, sensor->szSensorNameOrig, NFLG_ATTGROUP | NAFLG_FMT_KEY_QUOTE);
	}
	for (DWORD i = 0; i < ctx.hdr->dwNumReadingElements; i++)
	{
		PHWiNFO_SENSORS_READING_ELEMENT reading = &ctx.readings[i];
		PHWiNFO_SENSORS_SENSOR_ELEMENT sensor = &ctx.sensors[reading->dwSensorIndex];
		PNODE key = NWL_NodeEnumChild(node, reading->dwSensorIndex);
		if (strcmp(sensor->szSensorNameOrig, key->name) != 0)
			continue;
		NWL_NodeAttrSetf(key, reading->szLabelOrig, NAFLG_FMT_KEY_QUOTE, "%.2f", reading->Value);
	}
}

sensor_t sensor_hwinfo =
{
	.name = "HWiNFO",
	.init = hwinfo_init,
	.get = hwinfo_get,
	.fini = hwinfo_fini,
};
