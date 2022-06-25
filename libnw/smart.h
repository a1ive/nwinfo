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

typedef struct _IDENTIFY_DEVICE_DATA
{
	struct
	{
		USHORT Reserved1 : 1;
		USHORT Retired3 : 1;
		USHORT ResponseIncomplete : 1;
		USHORT Retired2 : 3;
		USHORT FixedDevice : 1;
		USHORT RemovableMedia : 1;
		USHORT Retired1 : 7;
		USHORT DeviceType : 1;
	} GeneralConfiguration;
	USHORT NumCylinders;
	USHORT SpecificConfiguration;
	USHORT NumHeads;
	USHORT Retired1[2];
	USHORT NumSectorsPerTrack;
	USHORT VendorUnique1[3];
	UCHAR  SerialNumber[20];
	USHORT Retired2[2];
	USHORT Obsolete1;
	UCHAR  FirmwareRevision[8];
	UCHAR  ModelNumber[40];
	UCHAR  MaximumBlockTransfer;
	UCHAR  VendorUnique2;
	struct
	{
		USHORT FeatureSupported : 1;
		USHORT Reserved : 15;
	} TrustedComputing;
	struct
	{
		UCHAR  CurrentLongPhysicalSectorAlignment : 2;
		UCHAR  ReservedByte49 : 6;
		UCHAR  DmaSupported : 1;
		UCHAR  LbaSupported : 1;
		UCHAR  IordyDisable : 1;
		UCHAR  IordySupported : 1;
		UCHAR  Reserved1 : 1;
		UCHAR  StandybyTimerSupport : 1;
		UCHAR  Reserved2 : 2;
		USHORT ReservedWord50;
	} Capabilities;
	USHORT ObsoleteWords51[2];
	USHORT TranslationFieldsValid : 3;
	USHORT Reserved3 : 5;
	USHORT FreeFallControlSensitivity : 8;
	USHORT NumberOfCurrentCylinders;
	USHORT NumberOfCurrentHeads;
	USHORT CurrentSectorsPerTrack;
	ULONG  CurrentSectorCapacity;
	UCHAR  CurrentMultiSectorSetting;
	UCHAR  MultiSectorSettingValid : 1;
	UCHAR  ReservedByte59 : 3;
	UCHAR  SanitizeFeatureSupported : 1;
	UCHAR  CryptoScrambleExtCommandSupported : 1;
	UCHAR  OverwriteExtCommandSupported : 1;
	UCHAR  BlockEraseExtCommandSupported : 1;
	ULONG  UserAddressableSectors;
	USHORT ObsoleteWord62;
	USHORT MultiWordDMASupport : 8;
	USHORT MultiWordDMAActive : 8;
	USHORT AdvancedPIOModes : 8;
	USHORT ReservedByte64 : 8;
	USHORT MinimumMWXferCycleTime;
	USHORT RecommendedMWXferCycleTime;
	USHORT MinimumPIOCycleTime;
	USHORT MinimumPIOCycleTimeIORDY;
	struct
	{
		USHORT ZonedCapabilities : 2;
		USHORT NonVolatileWriteCache : 1;
		USHORT ExtendedUserAddressableSectorsSupported : 1;
		USHORT DeviceEncryptsAllUserData : 1;
		USHORT ReadZeroAfterTrimSupported : 1;
		USHORT Optional28BitCommandsSupported : 1;
		USHORT IEEE1667 : 1;
		USHORT DownloadMicrocodeDmaSupported : 1;
		USHORT SetMaxSetPasswordUnlockDmaSupported : 1;
		USHORT WriteBufferDmaSupported : 1;
		USHORT ReadBufferDmaSupported : 1;
		USHORT DeviceConfigIdentifySetDmaSupported : 1;
		USHORT LPSAERCSupported : 1;
		USHORT DeterministicReadAfterTrimSupported : 1;
		USHORT CFastSpecSupported : 1;
	} AdditionalSupported;
	USHORT ReservedWords70[5];
	USHORT QueueDepth : 5;
	USHORT ReservedWord75 : 11;
	struct
	{
		USHORT Reserved0 : 1;
		USHORT SataGen1 : 1;
		USHORT SataGen2 : 1;
		USHORT SataGen3 : 1;
		USHORT Reserved1 : 4;
		USHORT NCQ : 1;
		USHORT HIPM : 1;
		USHORT PhyEvents : 1;
		USHORT NcqUnload : 1;
		USHORT NcqPriority : 1;
		USHORT HostAutoPS : 1;
		USHORT DeviceAutoPS : 1;
		USHORT ReadLogDMA : 1;
		USHORT Reserved2 : 1;
		USHORT CurrentSpeed : 3;
		USHORT NcqStreaming : 1;
		USHORT NcqQueueMgmt : 1;
		USHORT NcqReceiveSend : 1;
		USHORT DEVSLPtoReducedPwrState : 1;
		USHORT Reserved3 : 8;
	} SerialAtaCapabilities;
	struct
	{
		USHORT Reserved0 : 1;
		USHORT NonZeroOffsets : 1;
		USHORT DmaSetupAutoActivate : 1;
		USHORT DIPM : 1;
		USHORT InOrderData : 1;
		USHORT HardwareFeatureControl : 1;
		USHORT SoftwareSettingsPreservation : 1;
		USHORT NCQAutosense : 1;
		USHORT DEVSLP : 1;
		USHORT HybridInformation : 1;
		USHORT Reserved1 : 6;
	} SerialAtaFeaturesSupported;
	struct
	{
		USHORT Reserved0 : 1;
		USHORT NonZeroOffsets : 1;
		USHORT DmaSetupAutoActivate : 1;
		USHORT DIPM : 1;
		USHORT InOrderData : 1;
		USHORT HardwareFeatureControl : 1;
		USHORT SoftwareSettingsPreservation : 1;
		USHORT DeviceAutoPS : 1;
		USHORT DEVSLP : 1;
		USHORT HybridInformation : 1;
		USHORT Reserved1 : 6;
	} SerialAtaFeaturesEnabled;
	USHORT MajorRevision;
	USHORT MinorRevision;
	struct
	{
		USHORT SmartCommands : 1;
		USHORT SecurityMode : 1;
		USHORT RemovableMediaFeature : 1;
		USHORT PowerManagement : 1;
		USHORT Reserved1 : 1;
		USHORT WriteCache : 1;
		USHORT LookAhead : 1;
		USHORT ReleaseInterrupt : 1;
		USHORT ServiceInterrupt : 1;
		USHORT DeviceReset : 1;
		USHORT HostProtectedArea : 1;
		USHORT Obsolete1 : 1;
		USHORT WriteBuffer : 1;
		USHORT ReadBuffer : 1;
		USHORT Nop : 1;
		USHORT Obsolete2 : 1;
		USHORT DownloadMicrocode : 1;
		USHORT DmaQueued : 1;
		USHORT Cfa : 1;
		USHORT AdvancedPm : 1;
		USHORT Msn : 1;
		USHORT PowerUpInStandby : 1;
		USHORT ManualPowerUp : 1;
		USHORT Reserved2 : 1;
		USHORT SetMax : 1;
		USHORT Acoustics : 1;
		USHORT BigLba : 1;
		USHORT DeviceConfigOverlay : 1;
		USHORT FlushCache : 1;
		USHORT FlushCacheExt : 1;
		USHORT WordValid83 : 2;
		USHORT SmartErrorLog : 1;
		USHORT SmartSelfTest : 1;
		USHORT MediaSerialNumber : 1;
		USHORT MediaCardPassThrough : 1;
		USHORT StreamingFeature : 1;
		USHORT GpLogging : 1;
		USHORT WriteFua : 1;
		USHORT WriteQueuedFua : 1;
		USHORT WWN64Bit : 1;
		USHORT URGReadStream : 1;
		USHORT URGWriteStream : 1;
		USHORT ReservedForTechReport : 2;
		USHORT IdleWithUnloadFeature : 1;
		USHORT WordValid : 2;
	} CommandSetSupport;
	struct
	{
		USHORT SmartCommands : 1;
		USHORT SecurityMode : 1;
		USHORT RemovableMediaFeature : 1;
		USHORT PowerManagement : 1;
		USHORT Reserved1 : 1;
		USHORT WriteCache : 1;
		USHORT LookAhead : 1;
		USHORT ReleaseInterrupt : 1;
		USHORT ServiceInterrupt : 1;
		USHORT DeviceReset : 1;
		USHORT HostProtectedArea : 1;
		USHORT Obsolete1 : 1;
		USHORT WriteBuffer : 1;
		USHORT ReadBuffer : 1;
		USHORT Nop : 1;
		USHORT Obsolete2 : 1;
		USHORT DownloadMicrocode : 1;
		USHORT DmaQueued : 1;
		USHORT Cfa : 1;
		USHORT AdvancedPm : 1;
		USHORT Msn : 1;
		USHORT PowerUpInStandby : 1;
		USHORT ManualPowerUp : 1;
		USHORT Reserved2 : 1;
		USHORT SetMax : 1;
		USHORT Acoustics : 1;
		USHORT BigLba : 1;
		USHORT DeviceConfigOverlay : 1;
		USHORT FlushCache : 1;
		USHORT FlushCacheExt : 1;
		USHORT Resrved3 : 1;
		USHORT Words119_120Valid : 1;
		USHORT SmartErrorLog : 1;
		USHORT SmartSelfTest : 1;
		USHORT MediaSerialNumber : 1;
		USHORT MediaCardPassThrough : 1;
		USHORT StreamingFeature : 1;
		USHORT GpLogging : 1;
		USHORT WriteFua : 1;
		USHORT WriteQueuedFua : 1;
		USHORT WWN64Bit : 1;
		USHORT URGReadStream : 1;
		USHORT URGWriteStream : 1;
		USHORT ReservedForTechReport : 2;
		USHORT IdleWithUnloadFeature : 1;
		USHORT Reserved4 : 2;
	} CommandSetActive;
	USHORT UltraDMASupport : 8;
	USHORT UltraDMAActive : 8;
	struct
	{
		USHORT TimeRequired : 15;
		USHORT ExtendedTimeReported : 1;
	} NormalSecurityEraseUnit;
	struct
	{
		USHORT TimeRequired : 15;
		USHORT ExtendedTimeReported : 1;
	} EnhancedSecurityEraseUnit;
	USHORT CurrentAPMLevel : 8;
	USHORT ReservedWord91 : 8;
	USHORT MasterPasswordID;
	USHORT HardwareResetResult;
	USHORT CurrentAcousticValue : 8;
	USHORT RecommendedAcousticValue : 8;
	USHORT StreamMinRequestSize;
	USHORT StreamingTransferTimeDMA;
	USHORT StreamingAccessLatencyDMAPIO;
	ULONG  StreamingPerfGranularity;
	ULONG  Max48BitLBA[2];
	USHORT StreamingTransferTime;
	USHORT DsmCap;
	struct
	{
		USHORT LogicalSectorsPerPhysicalSector : 4;
		USHORT Reserved0 : 8;
		USHORT LogicalSectorLongerThan256Words : 1;
		USHORT MultipleLogicalSectorsPerPhysicalSector : 1;
		USHORT Reserved1 : 2;
	} PhysicalLogicalSectorSize;
	USHORT InterSeekDelay;
	USHORT WorldWideName[4];
	USHORT ReservedForWorldWideName128[4];
	USHORT ReservedForTlcTechnicalReport;
	USHORT WordsPerLogicalSector[2];
	struct
	{
		USHORT ReservedForDrqTechnicalReport : 1;
		USHORT WriteReadVerify : 1;
		USHORT WriteUncorrectableExt : 1;
		USHORT ReadWriteLogDmaExt : 1;
		USHORT DownloadMicrocodeMode3 : 1;
		USHORT FreefallControl : 1;
		USHORT SenseDataReporting : 1;
		USHORT ExtendedPowerConditions : 1;
		USHORT Reserved0 : 6;
		USHORT WordValid : 2;
	} CommandSetSupportExt;
	struct
	{
		USHORT ReservedForDrqTechnicalReport : 1;
		USHORT WriteReadVerify : 1;
		USHORT WriteUncorrectableExt : 1;
		USHORT ReadWriteLogDmaExt : 1;
		USHORT DownloadMicrocodeMode3 : 1;
		USHORT FreefallControl : 1;
		USHORT SenseDataReporting : 1;
		USHORT ExtendedPowerConditions : 1;
		USHORT Reserved0 : 6;
		USHORT Reserved1 : 2;
	} CommandSetActiveExt;
	USHORT ReservedForExpandedSupportandActive[6];
	USHORT MsnSupport : 2;
	USHORT ReservedWord127 : 14;
	struct
	{
		USHORT SecuritySupported : 1;
		USHORT SecurityEnabled : 1;
		USHORT SecurityLocked : 1;
		USHORT SecurityFrozen : 1;
		USHORT SecurityCountExpired : 1;
		USHORT EnhancedSecurityEraseSupported : 1;
		USHORT Reserved0 : 2;
		USHORT SecurityLevel : 1;
		USHORT Reserved1 : 7;
	} SecurityStatus;
	USHORT ReservedWord129[31];
	struct
	{
		USHORT MaximumCurrentInMA : 12;
		USHORT CfaPowerMode1Disabled : 1;
		USHORT CfaPowerMode1Required : 1;
		USHORT Reserved0 : 1;
		USHORT Word160Supported : 1;
	} CfaPowerMode1;
	USHORT ReservedForCfaWord161[7];
	USHORT NominalFormFactor : 4;
	USHORT ReservedWord168 : 12;
	struct
	{
		USHORT SupportsTrim : 1;
		USHORT Reserved0 : 15;
	} DataSetManagementFeature;
	USHORT AdditionalProductID[4];
	USHORT ReservedForCfaWord174[2];
	USHORT CurrentMediaSerialNumber[30];
	struct
	{
		USHORT Supported : 1;
		USHORT Reserved0 : 1;
		USHORT WriteSameSuported : 1;
		USHORT ErrorRecoveryControlSupported : 1;
		USHORT FeatureControlSuported : 1;
		USHORT DataTablesSuported : 1;
		USHORT Reserved1 : 6;
		USHORT VendorSpecific : 4;
	} SCTCommandTransport;
	USHORT ReservedWord207[2];
	struct
	{
		USHORT AlignmentOfLogicalWithinPhysical : 14;
		USHORT Word209Supported : 1;
		USHORT Reserved0 : 1;
	} BlockAlignment;
	USHORT WriteReadVerifySectorCountMode3Only[2];
	USHORT WriteReadVerifySectorCountMode2Only[2];
	struct
	{
		USHORT NVCachePowerModeEnabled : 1;
		USHORT Reserved0 : 3;
		USHORT NVCacheFeatureSetEnabled : 1;
		USHORT Reserved1 : 3;
		USHORT NVCachePowerModeVersion : 4;
		USHORT NVCacheFeatureSetVersion : 4;
	} NVCacheCapabilities;
	USHORT NVCacheSizeLSW;
	USHORT NVCacheSizeMSW;
	USHORT NominalMediaRotationRate;
	USHORT ReservedWord218;
	struct
	{
		UCHAR NVCacheEstimatedTimeToSpinUpInSeconds;
		UCHAR Reserved;
	} NVCacheOptions;
	USHORT WriteReadVerifySectorCountMode : 8;
	USHORT ReservedWord220 : 8;
	USHORT ReservedWord221;
	struct
	{
		USHORT MajorVersion : 12;
		USHORT TransportType : 4;
	} TransportMajorVersion;
	USHORT TransportMinorVersion;
	USHORT ReservedWord224[6];
	ULONG  ExtendedNumberOfUserAddressableSectors[2];
	USHORT MinBlocksPerDownloadMicrocodeMode03;
	USHORT MaxBlocksPerDownloadMicrocodeMode03;
	USHORT ReservedWord236[19];
	USHORT Signature : 8;
	USHORT CheckSum : 8;
} IDENTIFY_DEVICE_DATA, * PIDENTIFY_DEVICE_DATA;

#pragma pack()
