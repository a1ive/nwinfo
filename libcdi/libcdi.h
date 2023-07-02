#pragma once

#define VC_EXTRALEAN
#include <windows.h>

#define CDI_VERSION "9.0.1"

enum CDI_ATA_BOOL
{
	/* Features */
	CDI_BOOL_SSD = 0,
	CDI_BOOL_SSD_NVME,
	CDI_BOOL_SMART,
	CDI_BOOL_LBA48,
	CDI_BOOL_AAM,
	CDI_BOOL_APM,
	CDI_BOOL_NCQ,
	CDI_BOOL_NV_CACHE,
	CDI_BOOL_DEVSLP,
	CDI_BOOL_STREAMING,
	CDI_BOOL_GPL,
	CDI_BOOL_TRIM,
	CDI_BOOL_VOLATILE_WRITE_CACHE,
	/* SMART */
	CDI_BOOL_SMART_ENABLED,
	CDI_BOOL_AAM_ENABLED,
	CDI_BOOL_APM_ENABLED,
};

enum CDI_ATA_INT
{
	CDI_INT_DISK_ID = 0,
	CDI_INT_SCSI_PORT,
	CDI_INT_SCSI_TARGET_ID,
	CDI_INT_SCSI_BUS,
	/* Info */
	CDI_INT_POWER_ON_HOURS,
	CDI_INT_TEMPERATURE,
	CDI_INT_TEMPERATURE_ALARM,
	CDI_INT_HOST_WRITES,
	CDI_INT_HOST_READS,
	CDI_INT_NAND_WRITES,
	CDI_INT_GB_ERASED,
	CDI_INT_WEAR_LEVELING_COUNT,
	CDI_INT_LIFE,
	CDI_INT_MAX_ATTRIBUTE,
};

enum CDI_ATA_DWORD
{
	/* Size */
	CDI_DWORD_DISK_SIZE = 0,
	CDI_DWORD_LOGICAL_SECTOR_SIZE,
	CDI_DWORD_PHYSICAL_SECTOR_SIZE,
	CDI_DWORD_BUFFER_SIZE,
	/* Other */
	CDI_DWORD_ATTR_COUNT,
	CDI_DWORD_POWER_ON_COUNT,
	CDI_DWORD_ROTATION_RATE,
	CDI_DWORD_DRIVE_LETTER,
	CDI_DWORD_DISK_STATUS,
	CDI_DWORD_DISK_VENDOR_ID, // SSD_VENDOR_NVME = 19
};

enum CDI_ATA_STRING
{
	CDI_STRING_SN,
	CDI_STRING_FIRMWARE,
	CDI_STRING_MODEL,
	CDI_STRING_DRIVE_MAP,
	CDI_STRING_TRANSFER_MODE_MAX,
	CDI_STRING_TRANSFER_MODE_CUR,
	CDI_STRING_INTERFACE,
	CDI_STRING_VERSION_MAJOR,
	CDI_STRING_VERSION_MINOR,
	CDI_STRING_PNP_ID,
	CDI_STRING_DISK_STATUS,
};

#pragma pack(push,1)

typedef	struct _CDI_SMART_ATTRIBUTE
{
	BYTE	Id;
	WORD	StatusFlags;
	BYTE	CurrentValue;
	BYTE	WorstValue;
	BYTE	RawValue[6];
	BYTE	Reserved;
} CDI_SMART_ATTRIBUTE;

typedef	struct _CDI_SMART_THRESHOLD
{
	BYTE	Id;
	BYTE	ThresholdValue;
	BYTE	Reserved[10];
} CDI_SMART_THRESHOLD;

#pragma	pack(pop)

enum CDI_DISK_STATUS
{
	CDI_DISK_STATUS_UNKNOWN = 0,
	CDI_DISK_STATUS_GOOD,
	CDI_DISK_STATUS_CAUTION,
	CDI_DISK_STATUS_BAD
};

typedef struct _CDI_SMART_STATUS
{
	int MaxAttribute;
	DWORD HighestStatus;
	DWORD Status[1];
} CDI_SMART_STATUS;

#ifdef __cplusplus

typedef CAtaSmart CDI_SMART;

#else

typedef struct _CDI_SMART CDI_SMART;

CDI_SMART* cdi_create_smart(void);
void cdi_destroy_smart(CDI_SMART* ptr);
void cdi_init_smart(CDI_SMART* ptr,
	BOOL use_wmi,
	BOOL advanced_disk_search,
	BOOL workaround_hd204ui,
	BOOL workaround_adata_ssd,
	BOOL flag_hide_no_smart_disk,
	BOOL flag_sort_drive_letter);
DWORD cdi_update_smart(CDI_SMART* ptr, int index);

int cdi_get_disk_count(CDI_SMART* ptr);

BOOL cdi_get_bool(CDI_SMART* ptr, int index, enum CDI_ATA_BOOL attr);
INT cdi_get_int(CDI_SMART* ptr, int index, enum CDI_ATA_INT attr);
DWORD cdi_get_dword(CDI_SMART* ptr, int index, enum CDI_ATA_DWORD attr);
char* cdi_get_string(CDI_SMART* ptr, int index, enum CDI_ATA_STRING attr);
void cdi_free_string(char* ptr);

CDI_SMART_ATTRIBUTE* cdi_get_smart_attribute(CDI_SMART* ptr, int index);
CDI_SMART_THRESHOLD* cdi_get_smart_threshold(CDI_SMART* ptr, int index);

char* cdi_get_smart_attribute_name(CDI_SMART * ptr, int index, BYTE id);
char* cdi_get_smart_attribute_format(CDI_SMART* ptr, int index);
char* cdi_get_smart_attribute_value(CDI_SMART* ptr, int index, int attr);

CDI_SMART_STATUS* cdi_get_smart_status(CDI_SMART* ptr, int index);

#endif
