# liblhm

liblhm is a small C API for initializing the hardware monitor, managing feature toggles, and reading sensor data. The public API is defined in `lhm.h` and uses the `__stdcall` calling convention.

## Constants

- `LHM_STR_MAX` (128)
  - Maximum length (in wide characters) for string fields in `LhmSensorInfo`.

## Data Structures

### `LhmSensorInfo`

Represents a single sensor reading and its metadata.

**Alignment (MSVC)**: The structure is built with MSVC default packing (`/Zp8`), meaning members are aligned to their natural alignment up to 8 bytes, with the struct aligned to the largest member alignment (capped at 8). There are no `#pragma pack` directives in the public header, so changing the packing in a consumer (e.g., `/Zp1`) will change the layout and is not ABI-compatible.

| Field | Type | Description |
| --- | --- | --- |
| `hardware` | `wchar_t[LHM_STR_MAX]` | Hardware group/name the sensor belongs to. |
| `id` | `wchar_t[LHM_STR_MAX]` | Stable identifier for the sensor. |
| `name` | `wchar_t[LHM_STR_MAX]` | Human-readable sensor name. |
| `type` | `wchar_t[LHM_STR_MAX]` | Sensor type (e.g., temperature, voltage). |
| `has_value` | `bool` | `true` if `value` is valid. |
| `value` | `float` | Current sensor value. |
| `has_min` | `bool` | `true` if `min` is valid. |
| `min` | `float` | Minimum recorded value. |
| `has_max` | `bool` | `true` if `max` is valid. |
| `max` | `float` | Maximum recorded value. |

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
