
# libcdi

![License](https://img.shields.io/badge/LICENSE-MIT-green)


Dynamic library for accessing S.M.A.R.T. information, based on [CrystalDiskInfo](https://github.com/hiyohiyo/CrystalDiskInfo).

## Supported Platforms
- Windows XP and later
- x64, x86 architectures

## Credits

- [CrystalDiskInfo](https://github.com/hiyohiyo/CrystalDiskInfo)
- [CrystalDiskInfoEmbedded](https://github.com/iTXTech/CrystalDiskInfoEmbedded)

## API Reference

> [!WARNING]
>
> The host application that loads and calls this DLL must be run with administrator privileges.

### `CONST CHAR* WINAPI cdi_get_version(VOID)`
Gets the CDI version.

- **Parameters:** None.
- **Return Value:** The CDI version string, e.g., `"9.0.0"`.

---
### `CDI_SMART* WINAPI cdi_create_smart(VOID)`
Creates a SMART data structure.

- **Note:**
    - `CoInitializeEx` must be called before this function after loading the DLL.
    - You should call this function first to create a `CDI_SMART` structure before using other functions except `cdi_get_version`.
    - Call `cdi_destroy_smart` to free the memory when done.
- **Parameters:** None.
- **Return Value:** A pointer to a `CDI_SMART` structure for use with other functions.
- **Reference:**
    - [Microsoft Docs - CoInitializeEx](https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-coinitializeex)
    - [Microsoft Docs - CoUninitialize](https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-couninitialize)

---
### `VOID WINAPI cdi_destroy_smart(CDI_SMART* ptr)`
Frees the SMART data structure.

- **Parameters:**
    - `ptr`: A pointer to the `CDI_SMART` structure obtained from `cdi_create_smart`.
- **Return Value:** None.

---
### `VOID WINAPI cdi_init_smart(CDI_SMART* ptr, UINT64 flags)`
Initializes the SMART data.

- **Note:**
    - This function should be called after `cdi_create_smart` to initialize the SMART data.
    - It scans all disks and collects S.M.A.R.T. information, which may take some time.
- **Parameters:**
    - `ptr`: A pointer to the `CDI_SMART` structure.
    - `flags`: Various options for disk processing. See the **Disk Processing Options** section below. The recommended value is `0x01FBFF81`.
- **Return Value:** None.

---
### `DWORD WINAPI cdi_update_smart(CDI_SMART* ptr, INT index)`
Updates the S.M.A.R.T. information for a specific disk.

- **Parameters:**
    - `ptr`: A pointer to the `CDI_SMART` structure.
    - `index`: The disk index, starting from 0. The maximum value is `cdi_get_disk_count() - 1`.
- **Return Value:** The DWORD value indicating the update status.
    - `0` if the disk's S.M.A.R.T. information has not changed.
    - Non-zero if the S.M.A.R.T. information has changed.

---
### `INT WINAPI cdi_get_disk_count(CDI_SMART* ptr)`
Gets the number of detected disks.

- **Parameters:**
    - `ptr`: A pointer to the `CDI_SMART` structure.
- **Return Value:** The total count of disks.

---
### `BOOL WINAPI cdi_get_bool(CDI_SMART* ptr, INT index, enum CDI_ATA_BOOL attr)`
Gets a boolean disk attribute.

- **Parameters:**
    - `ptr`: A pointer to the `CDI_SMART` structure.
    - `index`: The disk index.
    - `attr`: The attribute to retrieve. See the **Disk Attributes (BOOL)** section.
- **Return Value:** The boolean value of the specified attribute.

---
### `INT WINAPI cdi_get_int(CDI_SMART* ptr, INT index, enum CDI_ATA_INT attr)`
Gets an integer disk attribute.

- **Parameters:**
    - `ptr`: A pointer to the `CDI_SMART` structure.
    - `index`: The disk index.
    - `attr`: The attribute to retrieve. See the **Disk Attributes (INT)** section.
- **Return Value:** The integer value of the specified attribute.
    - A negative value indicates that the attribute is not available.

---
### `DWORD WINAPI cdi_get_dword(CDI_SMART* ptr, INT index, enum CDI_ATA_DWORD attr)`
Gets a DWORD disk attribute.

- **Parameters:**
    - `ptr`: A pointer to the `CDI_SMART` structure.
    - `index`: The disk index.
    - `attr`: The attribute to retrieve. See the **Disk Attributes (DWORD)** section.
- **Return Value:** The DWORD value of the specified attribute.

---
### `WCHAR* WINAPI cdi_get_string(CDI_SMART* ptr, INT index, enum CDI_ATA_STRING attr)`
Gets a string disk attribute.

- **Parameters:**
    - `ptr`: A pointer to the `CDI_SMART` structure.
    - `index`: The disk index.
    - `attr`: The attribute to retrieve. See the **Disk Attributes (WCHAR*)** section.
- **Return Value:** The string value of the specified attribute. Use `cdi_free_string` to release the memory.

---
### `VOID WINAPI cdi_free_string(WCHAR* ptr)`
Frees the memory allocated for a string.

- **Parameters:**
    - `ptr`: The string pointer to be freed.
- **Return Value:** None.

---
### `WCHAR* WINAPI cdi_get_smart_format(CDI_SMART* ptr, INT index)`
Gets the S.M.A.R.T. data format string.

- **Parameters:**
    - `ptr`: A pointer to the `CDI_SMART` structure.
    - `index`: The disk index.
- **Return Value:** A string representing the S.M.A.R.T. data format. Use `cdi_free_string` to release the memory.
    - Possible values are `RawValues(7)`, `RawValues(8)`, `Cur RawValues(8)`, `Cur Wor --- RawValues(6)`, `Cur Wor Thr RawValues(6)`, `Cur Wor --- RawValues(7)` and `Cur Wor Thr RawValues(7)`.

---
### `BYTE WINAPI cdi_get_smart_id(CDI_SMART* ptr, INT index, INT attr)`
Gets the ID of a specific S.M.A.R.T. attribute.

- **Parameters:**
    - `ptr`: A pointer to the `CDI_SMART` structure.
    - `index`: The disk index.
    - `attr`: The S.M.A.R.T. attribute index, from `0` to `CDI_DWORD_ATTR_COUNT - 1`.
- **Return Value:** The ID of the S.M.A.R.T. attribute.

---
### `WCHAR* WINAPI cdi_get_smart_value(CDI_SMART* ptr, INT index, INT attr, BOOL hex)`
Gets the formatted value of a specific S.M.A.R.T. attribute.

- **Parameters:**
    - `ptr`: A pointer to the `CDI_SMART` structure.
    - `index`: The disk index.
    - `attr`: The S.M.A.R.T. attribute index, from `0` to `CDI_DWORD_ATTR_COUNT - 1`.
    - `hex`: If `TRUE`, the raw value will be formatted as hexadecimal.
- **Return Value:** A string representing the S.M.A.R.T. attribute data. Use `cdi_free_string` to release the memory.

---
### `INT WINAPI cdi_get_smart_status(CDI_SMART* ptr, INT index, INT attr)`
Gets the status of a specific S.M.A.R.T. attribute.

- **Parameters:**
    - `ptr`: A pointer to the `CDI_SMART` structure.
    - `index`: The disk index.
    - `attr`: The S.M.A.R.T. attribute index, from `0` to `CDI_DWORD_ATTR_COUNT - 1`.
- **Return Value:** The status of the S.M.A.R.T. attribute. See the **Disk Health Status** section.

---
### `WCHAR* WINAPI *cdi_get_smart_name(CDI_SMART* ptr, INT index, BYTE id)`
Gets the name of a S.M.A.R.T. attribute by its ID.

- **Parameters:**
    - `ptr`: A pointer to the `CDI_SMART` structure.
    - `index`: The disk index.
    - `id`: The ID of the S.M.A.R.T. attribute, obtained from `cdi_get_smart_id`.
- **Return Value:** The name of the S.M.A.R.T. attribute. Use `cdi_free_string` to release the memory.


## Disk Processing Options
```c
#define CDI_FLAG_USE_WMI                (1ULL << 0)  // TRUE
#define CDI_FLAG_ADVANCED_SEARCH        (1ULL << 1)  // FALSE
#define CDI_FLAG_WORKAROUND_HD204UI     (1ULL << 2)  // FALSE
#define CDI_FLAG_WORKAROUND_ADATA       (1ULL << 3)  // FALSE
#define CDI_FLAG_HIDE_NO_SMART          (1ULL << 4)  // FALSE
#define CDI_FLAG_SORT_DRIVE_LETTER      (1ULL << 5)  // FALSE
#define CDI_FLAG_NO_WAKEUP              (1ULL << 6)  // FALSE
#define CDI_FLAG_ATA_PASS_THROUGH       (1ULL << 7)  // TRUE
#define CDI_FLAG_ENABLE_NVIDIA          (1ULL << 8)  // TRUE
#define CDI_FLAG_ENABLE_MARVELL         (1ULL << 9)  // TRUE
#define CDI_FLAG_ENABLE_USB_SAT         (1ULL << 10) // TRUE
#define CDI_FLAG_ENABLE_USB_SUNPLUS     (1ULL << 11) // TRUE
#define CDI_FLAG_ENABLE_USB_IODATA      (1ULL << 12) // TRUE
#define CDI_FLAG_ENABLE_USB_LOGITEC     (1ULL << 13) // TRUE
#define CDI_FLAG_ENABLE_USB_PROLIFIC    (1ULL << 14) // TRUE
#define CDI_FLAG_ENABLE_USB_JMICRON     (1ULL << 15) // TRUE
#define CDI_FLAG_ENABLE_USB_CYPRESS     (1ULL << 16) // TRUE
#define CDI_FLAG_ENABLE_USB_MEMORY      (1ULL << 17) // TRUE
#define CDI_FLAG_ENABLE_NVME_JMICRON    (1ULL << 19) // TRUE
#define CDI_FLAG_ENABLE_NVME_ASMEDIA    (1ULL << 20) // TRUE
#define CDI_FLAG_ENABLE_NVME_REALTEK    (1ULL << 21) // TRUE
#define CDI_FLAG_ENABLE_MEGA_RAID       (1ULL << 22) // TRUE
#define CDI_FLAG_ENABLE_INTEL_VROC      (1ULL << 23) // TRUE
#define CDI_FLAG_ENABLE_ASM1352R        (1ULL << 24) // TRUE
#define CDI_FLAG_ENABLE_AMD_RC2         (1ULL << 25) // FALSE
#define CDI_FLAG_ENABLE_REALTEK_9220DP  (1ULL << 26) // FALSE
#define CDI_FLAG_HIDE_RAID_VOLUME       (1ULL << 27) // TRUE
// CDI_FLAG[30:28]
#define CDI_CSMI_SHIFT                  28
#define CDI_FLAG_CSMI_DISABLE           (0ULL << CDI_CSMI_SHIFT) // FALSE
#define CDI_FLAG_CSMI_AUTO              (1ULL << CDI_CSMI_SHIFT) // TRUE
#define CDI_FLAG_CSMI_RAID              (2ULL << CDI_CSMI_SHIFT) // FALSE
```

## Enumerations

### Disk Attributes (BOOL)
`enum CDI_ATA_BOOL`
- `CDI_BOOL_SSD`: Is Solid State Drive
- `CDI_BOOL_SSD_NVME`: Is NVMe SSD (Win10+)
- `CDI_BOOL_SMART`: S.M.A.R.T. supported
- `CDI_BOOL_LBA48`: LBA48 supported
- `CDI_BOOL_AAM`: AAM supported
- `CDI_BOOL_APM`: APM supported
- `CDI_BOOL_NCQ`: NCQ supported
- `CDI_BOOL_NV_CACHE`: NV Cache supported
- `CDI_BOOL_DEVSLP`: DEVSLP supported
- `CDI_BOOL_STREAMING`: Streaming supported
- `CDI_BOOL_GPL`: GPL supported
- `CDI_BOOL_TRIM`: TRIM supported
- `CDI_BOOL_VOLATILE_WRITE_CACHE`: Volatile Write Cache supported
- `CDI_BOOL_SMART_ENABLED`: S.M.A.R.T. is enabled
- `CDI_BOOL_AAM_ENABLED`: AAM is enabled
- `CDI_BOOL_APM_ENABLED`: APM is enabled

### Disk Attributes (INT)
`enum CDI_ATA_INT`
- `CDI_INT_DISK_ID`: Disk ID (e.g., of `\\.\PhysicalDriveX`)
- `CDI_INT_DISK_STATUS`: Health status (See **Disk Health Status**)
- `CDI_INT_SCSI_PORT`: SCSI Port
- `CDI_INT_SCSI_TARGET_ID`: SCSI Target ID
- `CDI_INT_SCSI_BUS`: SCSI Bus
- `CDI_INT_POWER_ON_HOURS`: Power on hours
- `CDI_INT_TEMPERATURE`: Temperature (°C)
- `CDI_INT_TEMPERATURE_ALARM`: Temperature alarm threshold (°C)
- `CDI_INT_HOST_WRITES`: Total host writes (GB)
- `CDI_INT_HOST_READS`: Total host reads (GB)
- `CDI_INT_NAND_WRITES`: Total NAND writes (GB)
- `CDI_INT_GB_ERASED`: Gigabytes erased (GB)
- `CDI_INT_WEAR_LEVELING_COUNT`: Wear Leveling Count
- `CDI_INT_LIFE`: Health percentage (0% - 100%)
- `CDI_INT_MAX_ATTRIBUTE`: Max attribute count
- `CDI_INT_INTERFACE_TYPE`: Bus type (See [STORAGE_BUS_TYPE](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ne-winioctl-storage_bus_type))

### Disk Attributes (DWORD)
`enum CDI_ATA_DWORD`
- `CDI_DWORD_DISK_SIZE`: Disk size (MB)
- `CDI_DWORD_LOGICAL_SECTOR_SIZE`: Logical sector size
- `CDI_DWORD_PHYSICAL_SECTOR_SIZE`: Physical sector size
- `CDI_DWORD_BUFFER_SIZE`: Buffer size (Bytes)
- `CDI_DWORD_ATTR_COUNT`: Total number of S.M.A.R.T. attributes
- `CDI_DWORD_POWER_ON_COUNT`: Power on count
- `CDI_DWORD_ROTATION_RATE`: Rotation rate (RPM)
- `CDI_DWORD_DRIVE_LETTER`: Drive letter map
- `CDI_DWORD_DISK_VENDOR_ID`: Disk vendor ID

### Disk Attributes (WCHAR*)
`enum CDI_ATA_STRING`
- `CDI_STRING_SN`: Serial Number
- `CDI_STRING_FIRMWARE`: Firmware Version
- `CDI_STRING_MODEL`: Model Name
- `CDI_STRING_DRIVE_MAP`: Drive letter list (e.g., "C: D:")
- `CDI_STRING_TRANSFER_MODE_MAX`: Maximum transfer mode
- `CDI_STRING_TRANSFER_MODE_CUR`: Current transfer mode
- `CDI_STRING_INTERFACE`: Interface type (e.g., "NVM Express")
- `CDI_STRING_VERSION_MAJOR`: Major version
- `CDI_STRING_VERSION_MINOR`: Minor version
- `CDI_STRING_PNP_ID`: Plug and Play ID
- `CDI_STRING_SMART_KEY`: S.M.A.R.T. Key
- `CDI_STRING_FORM_FACTOR`: Form factor (inch)
- `CDI_STRING_FIRMWARE_REV`: Firmware Revision

### Disk Health Status
`enum CDI_DISK_STATUS`
- `CDI_DISK_STATUS_UNKNOWN`: Unknown
- `CDI_DISK_STATUS_GOOD`: Good
- `CDI_DISK_STATUS_CAUTION`: Caution
- `CDI_DISK_STATUS_BAD`: Bad
