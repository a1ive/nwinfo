# liblhm

![License](https://img.shields.io/badge/LICENSE-MPL--2.0-orange)

liblhm provides a C/C++ API wrapper for [LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor).

[Download](https://nightly.link/LibreHardwareMonitor/LibreHardwareMonitor/workflows/master/master/LibreHardwareMonitorLib%20%28x64%29.zip) the LibreHardwareMonitorLib package (.NET Framework 4.7.2) and place it in the same folder as the liblhm binaries.

## Constants

```c
#define LHM_STR_MAX 128
```

## Data Structures

### `LhmSensorInfo`
```c
typedef struct
{
	wchar_t hardware[LHM_STR_MAX];
	wchar_t id[LHM_STR_MAX];
	wchar_t name[LHM_STR_MAX];
	wchar_t type[LHM_STR_MAX];
	bool has_value;
	float value;
	bool has_min;
	float min;
	bool has_max;
	float max;
} LhmSensorInfo;
```

Alignment: 8 bytes.

## Functions

### `bool __stdcall LhmInitialize(void);`

Initializes the library and starts the hardware monitor.

- **Returns**: `true` on success; `false` on failure. Call `LhmGetLastError` for details on failure.

### `size_t __stdcall LhmGetToggleCount(void);`

Returns the number of available feature toggles.

- **Returns**: The number of toggles.

### `bool __stdcall LhmGetToggleInfo(size_t index, const wchar_t** out_name, bool* out_enabled);`

Retrieves information about a feature toggle by index.

- **Parameters**:
  - `index`: Zero-based toggle index.
  - `out_name`: Receives a pointer to a wide-string toggle name.
  - `out_enabled`: Receives the current enabled state.
- **Returns**: `true` on success; `false` on failure.

### `bool __stdcall LhmSetToggleEnabled(size_t index, bool enabled);`

Enables or disables a feature toggle by index.

- **Parameters**:
  - `index`: Zero-based toggle index.
  - `enabled`: `true` to enable; `false` to disable.
- **Returns**: `true` on success; `false` on failure.

### `bool __stdcall LhmEnumerateSensors(const LhmSensorInfo** sensors, size_t* out_count);`

Retrieves a pointer to the current list of sensors.

- **Parameters**:
  - `sensors`: Receives a pointer to an array of `LhmSensorInfo`.
  - `out_count`: Receives the number of sensors in the array.
- **Returns**: `true` on success; `false` on failure.

### `void __stdcall LhmShutdown(void);`

Shuts down the library and releases resources.

### `const wchar_t* __stdcall LhmGetLastError(void);`

Returns a human-readable error message for the last failure.

- **Returns**: Pointer to a wide-string error message, or `NULL` if no error is available.
