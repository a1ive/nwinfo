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

typedef enum _SYSTEM_ENVIRONMENT_INFORMATION_CLASS
{
	SystemEnvironmentNameInformation = 1, // q: VARIABLE_NAME
	SystemEnvironmentValueInformation = 2, // q: VARIABLE_NAME_AND_VALUE
	MaxSystemEnvironmentInfoClass
} SYSTEM_ENVIRONMENT_INFORMATION_CLASS;
