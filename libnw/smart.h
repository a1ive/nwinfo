// SPDX-License-Identifier: Unlicense
#pragma once

#include <windows.h>

typedef enum
{
	NVME_LOG_PAGE_ERROR_INFO = 0x01,
	NVME_LOG_PAGE_HEALTH_INFO = 0x02,
	NVME_LOG_PAGE_FIRMWARE_SLOT_INFO = 0x03,
	NVME_LOG_PAGE_CHANGED_NAMESPACE_LIST = 0x04,
	NVME_LOG_PAGE_COMMAND_EFFECTS = 0x05,
	NVME_LOG_PAGE_DEVICE_SELF_TEST = 0x06,
	NVME_LOG_PAGE_TELEMETRY_HOST_INITIATED = 0x07,
	NVME_LOG_PAGE_TELEMETRY_CTLR_INITIATED = 0x08,
	NVME_LOG_PAGE_RESERVATION_NOTIFICATION = 0x80,
	NVME_LOG_PAGE_SANITIZE_STATUS = 0x81,
} NVME_LOG_PAGES;

typedef struct
{
	union
	{
		struct
		{
			UCHAR   AvailableSpaceLow : 1;                    // If set to 1, then the available spare space has fallen below the threshold.
			UCHAR   TemperatureThreshold : 1;                   // If set to 1, then a temperature is above an over temperature threshold or below an under temperature threshold.
			UCHAR   ReliabilityDegraded : 1;                    // If set to 1, then the device reliability has been degraded due to significant media related  errors or any internal error that degrades device reliability.
			UCHAR   ReadOnly : 1;                    // If set to 1, then the media has been placed in read only mode
			UCHAR   VolatileMemoryBackupDeviceFailed : 1;    // If set to 1, then the volatile memory backup device has failed. This field is only valid if the controller has a volatile memory backup solution.
			UCHAR   Reserved : 3;
		} DUMMYSTRUCTNAME;
		UCHAR AsUchar;
	} CriticalWarning;    // This field indicates critical warnings for the state of the  controller. Each bit corresponds to a critical warning type; multiple bits may be set.
	UCHAR   Temperature[2];                 // Temperature: Contains the temperature of the overall device (controller and NVM included) in units of Kelvin. If the temperature exceeds the temperature threshold, refer to section 5.12.1.4, then an asynchronous event completion may occur
	UCHAR   AvailableSpare;                 // Available Spare:  Contains a normalized percentage (0 to 100%) of the remaining spare capacity available
	UCHAR   AvailableSpareThreshold;        // Available Spare Threshold:  When the Available Spare falls below the threshold indicated in this field, an asynchronous event  completion may occur. The value is indicated as a normalized percentage (0 to 100%).
	UCHAR   PercentageUsed;                 // Percentage Used
	UCHAR   Reserved0[26];
	UCHAR   DataUnitRead[16];               // Data Units Read:  Contains the number of 512 byte data units the host has read from the controller; this value does not include metadata. This value is reported in thousands (i.e., a value of 1 corresponds to 1000 units of 512 bytes read)  and is rounded up.  When the LBA size is a value other than 512 bytes, the controller shall convert the amount of data read to 512 byte units. For the NVM command set, logical blocks read as part of Compare and Read operations shall be included in this value
	UCHAR   DataUnitWritten[16];            // Data Units Written: Contains the number of 512 byte data units the host has written to the controller; this value does not include metadata. This value is reported in thousands (i.e., a value of 1 corresponds to 1000 units of 512 bytes written)  and is rounded up.  When the LBA size is a value other than 512 bytes, the controller shall convert the amount of data written to 512 byte units. For the NVM command set, logical blocks written as part of Write operations shall be included in this value. Write Uncorrectable commands shall not impact this value.
	UCHAR   HostReadCommands[16];           // Host Read Commands:  Contains the number of read commands  completed by  the controller. For the NVM command set, this is the number of Compare and Read commands. 
	UCHAR   HostWrittenCommands[16];        // Host Write Commands:  Contains the number of write commands  completed by  the controller. For the NVM command set, this is the number of Write commands.
	UCHAR   ControllerBusyTime[16];         // Controller Busy Time:  Contains the amount of time the controller is busy with I/O commands. The controller is busy when there is a command outstanding to an I/O Queue (specifically, a command was issued via an I/O Submission Queue Tail doorbell write and the corresponding  completion queue entry  has not been posted yet to the associated I/O Completion Queue). This value is reported in minutes.
	UCHAR   PowerCycle[16];                 // Power Cycles: Contains the number of power cycles.
	UCHAR   PowerOnHours[16];               // Power On Hours: Contains the number of power-on hours. This does not include time that the controller was powered and in a low power state condition.
	UCHAR   UnsafeShutdowns[16];            // Unsafe Shutdowns: Contains the number of unsafe shutdowns. This count is incremented when a shutdown notification (CC.SHN) is not received prior to loss of power.
	UCHAR   MediaErrors[16];                // Media Errors:  Contains the number of occurrences where the controller detected an unrecovered data integrity error. Errors such as uncorrectable ECC, CRC checksum failure, or LBA tag mismatch are included in this field.
	UCHAR   ErrorInfoLogEntryCount[16];     // Number of Error Information Log Entries:  Contains the number of Error Information log entries over the life of the controller
	ULONG   WarningCompositeTemperatureTime;     // Warning Composite Temperature Time: Contains the amount of time in minutes that the controller is operational and the Composite Temperature is greater than or equal to the Warning Composite Temperature Threshold (WCTEMP) field and less than the Critical Composite Temperature Threshold (CCTEMP) field in the Identify Controller data structure
	ULONG   CriticalCompositeTemperatureTime;    // Critical Composite Temperature Time: Contains the amount of time in minutes that the controller is operational and the Composite Temperature is greater the Critical Composite Temperature Threshold (CCTEMP) field in the Identify Controller data structure
	USHORT  TemperatureSensor1;          // Contains the current temperature reported by temperature sensor 1.
	USHORT  TemperatureSensor2;          // Contains the current temperature reported by temperature sensor 2.
	USHORT  TemperatureSensor3;          // Contains the current temperature reported by temperature sensor 3.
	USHORT  TemperatureSensor4;          // Contains the current temperature reported by temperature sensor 4.
	USHORT  TemperatureSensor5;          // Contains the current temperature reported by temperature sensor 5.
	USHORT  TemperatureSensor6;          // Contains the current temperature reported by temperature sensor 6.
	USHORT  TemperatureSensor7;          // Contains the current temperature reported by temperature sensor 7.
	USHORT  TemperatureSensor8;          // Contains the current temperature reported by temperature sensor 8.
	UCHAR   Reserved1[296];
} NVME_HEALTH_INFO_LOG, * PNVME_HEALTH_INFO_LOG;

#define NVME_MAX_LOG_SIZE 4096

#define DRIVE_HEAD_REG 0xA0

#pragma pack(1)

typedef struct
{
	BYTE bAttrID;
	WORD wStatusFlags;
	BYTE bAttrValue;
	BYTE bWorstValue;
	BYTE bRawValue[6];
	BYTE bReserved;
} ATA_ATTRIBUTE, * PATA_ATTRIBUTE;

typedef struct
{
	BYTE bAttrID;
	BYTE bWarrantyThreshold;
	BYTE bReserved[10];
} ATA_THRESHOLD, * PATA_THRESHOLD;

#pragma pack()
