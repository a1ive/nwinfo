// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winioctl.h>

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef struct _IO_STATUS_BLOCK
{
	union
	{
		NTSTATUS Status;
		PVOID Pointer;
	};
	ULONG_PTR Information;
} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;

typedef struct _OBJECT_ATTRIBUTES
{
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

#define InitializeObjectAttributes( p, n, a, r, s ) { \
	(p)->Length = sizeof( OBJECT_ATTRIBUTES ); \
	(p)->RootDirectory = r; \
	(p)->Attributes = a; \
	(p)->ObjectName = n; \
	(p)->SecurityDescriptor = s; \
	(p)->SecurityQualityOfService = NULL; \
}

#define FILE_SUPERSEDE                  0x00000000
#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE                  0x00000004
#define FILE_OVERWRITE_IF               0x00000005
#define FILE_MAXIMUM_DISPOSITION        0x00000005

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef enum _OBJECT_INFORMATION_CLASS
{
	ObjectBasicInformation,
	ObjectNameInformation,
	ObjectTypeInformation,
	ObjectAllInformation,
	ObjectDataInformation
} OBJECT_INFORMATION_CLASS;

typedef struct _OBJECT_NAME_INFORMATION
{
	UNICODE_STRING Name;
	WCHAR NameBuffer[0];
} OBJECT_NAME_INFORMATION, * POBJECT_NAME_INFORMATION;

typedef struct _SYSTEM_BOOT_ENVIRONMENT_INFORMATION
{
	GUID BootIdentifier;
	FIRMWARE_TYPE FirmwareType;
	union
	{
		ULONGLONG BootFlags;
		struct
		{
			ULONGLONG DbgMenuOsSelection : 1;
			ULONGLONG DbgHiberBoot : 1;
			ULONGLONG DbgSoftBoot : 1;
			ULONGLONG DbgMeasuredLaunch : 1;
			ULONGLONG DbgMeasuredLaunchCapable : 1;
			ULONGLONG DbgSystemHiveReplace : 1;
			ULONGLONG DbgMeasuredLaunchSmmProtections : 1;
			ULONGLONG DbgMeasuredLaunchSmmLevel : 7;
		};
	};
} SYSTEM_BOOT_ENVIRONMENT_INFORMATION, * PSYSTEM_BOOT_ENVIRONMENT_INFORMATION;

#define SystemBootEnvironmentInformation 90

#define EFI_VARIABLE_NON_VOLATILE 0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS 0x00000004
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD 0x00000008
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS 0x00000010
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x00000020
#define EFI_VARIABLE_APPEND_WRITE 0x00000040
#define EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS 0x00000080

typedef enum _SYSTEM_ENVIRONMENT_INFORMATION_CLASS
{
	SystemEnvironmentNameInformation = 1, // q: VARIABLE_NAME
	SystemEnvironmentValueInformation = 2, // q: VARIABLE_NAME_AND_VALUE
	MaxSystemEnvironmentInfoClass
} SYSTEM_ENVIRONMENT_INFORMATION_CLASS;

typedef struct _VARIABLE_NAME
{
	ULONG NextEntryOffset;
	GUID VendorGuid;
	WCHAR Name[ANYSIZE_ARRAY];
} VARIABLE_NAME, * PVARIABLE_NAME;

typedef struct _VARIABLE_NAME_AND_VALUE
{
	ULONG NextEntryOffset;
	ULONG ValueOffset;
	ULONG ValueLength;
	ULONG Attributes;
	GUID VendorGuid;
	WCHAR Name[ANYSIZE_ARRAY];
	//BYTE Value[ANYSIZE_ARRAY];
} VARIABLE_NAME_AND_VALUE, * PVARIABLE_NAME_AND_VALUE;

#define EFI_OS_INDICATIONS_BOOT_TO_FW_UI                    0x0000000000000001
#define EFI_OS_INDICATIONS_TIMESTAMP_REVOCATION             0x0000000000000002
#define EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED  0x0000000000000004
#define EFI_OS_INDICATIONS_FMP_CAPSULE_SUPPORTED            0x0000000000000008
#define EFI_OS_INDICATIONS_CAPSULE_RESULT_VAR_SUPPORTED     0x0000000000000010
#define EFI_OS_INDICATIONS_START_OS_RECOVERY                0x0000000000000020
#define EFI_OS_INDICATIONS_START_PLATFORM_RECOVERY          0x0000000000000040
#define EFI_OS_INDICATIONS_JSON_CONFIG_DATA_REFRESH         0x0000000000000080
