// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdint.h>
#include <stddef.h>

typedef enum
{
	RYZEN_SMU_OK = 0x01,
	RYZEN_SMU_FAILED = 0xFF,
	RYZEN_SMU_UNKNOWN_CMD = 0xFE,
	RYZEN_SMU_CMD_REJECTED_PREREQ = 0xFD,
	RYZEN_SMU_CMD_REJECTED_BUSY = 0xFC,
	RYZEN_SMU_CMD_TIMEOUT = 0xFB,

	RYZEN_SMU_UNSUPPORTED = 0xF9,
	RYZEN_SMU_DRIVER_ERROR = 0xF6,
	RYZEN_SMU_CPU_NOT_SUPPORTED = 0xF5,
	RYZEN_SMU_NOT_INITIALIZED = 0xF4,
	RYZEN_SMU_INVALID_ARGUMENT = 0xF3,
	RYZEN_SMU_MAPPING_ERROR = 0xF2,
	RYZEN_SMU_INSUFFICIENT_BUFFER = 0xF1,
} ry_err_t;

typedef enum
{
	CODENAME_UNKNOWN = 0,
	CODENAME_SUMMITRIDGE,
	CODENAME_THREADRIPPER,
	CODENAME_NAPLES,
	CODENAME_RAVENRIDGE,
	CODENAME_RAVENRIDGE2, // DALI
	CODENAME_PINNACLERIDGE,
	CODENAME_COLFAX,
	CODENAME_PICASSO,
	CODENAME_FIREFLIGHT,
	CODENAME_MATISSE,
	CODENAME_CASTLEPEAK,
	CODENAME_ROME,
	CODENAME_DALI,
	CODENAME_RENOIR,
	CODENAME_VANGOGH,
	CODENAME_VERMEER,
	CODENAME_CHAGALL,
	CODENAME_MILAN,
	CODENAME_CEZANNE,
	CODENAME_REMBRANDT,
	CODENAME_LUCIENNE,
	CODENAME_RAPHAEL,
	CODENAME_PHOENIX,
	CODENAME_PHOENIX2,
	CODENAME_MENDOCINO,
	CODENAME_GENOA,
	CODENAME_STORMPEAK,
	CODENAME_DRAGONRANGE,
	CODENAME_MERO,
	CODENAME_HAWKPOINT,
	CODENAME_STRIXPOINT,
	CODENAME_GRANITERIDGE,
	CODENAME_KRACKANPOINT,
	CODENAME_KRACKANPOINT2,
	CODENAME_STRIXHALO,
	CODENAME_TURIN,
	CODENAME_SHIMADAPEAK,
	CODENAME_TURIND,
	CODENAME_BERGAMO,
	CODENAME_COUNT,
} ry_codename_t;

typedef union
{
	struct
	{
		uint32_t arg0;
		uint32_t arg1;
		uint32_t arg2;
		uint32_t arg3;
		uint32_t arg4;
		uint32_t arg5;
	} u32;
	uint32_t args[6];
} ry_args_t;

struct wr0_drv_t;

#define SMU_TABLE_MAX_SIZE 0x1000

typedef struct
{
	ry_codename_t codename;
	uint32_t smu_version;
	uint32_t pci_id;

	uint64_t pm_table_base_addr;
	uint32_t pm_table_version;
	size_t pm_table_size;
	union
	{
		uint8_t pm_table_buffer[SMU_TABLE_MAX_SIZE];
		uint64_t pm_table_buffer_u64[SMU_TABLE_MAX_SIZE / 8];
	};

	struct wr0_drv_t* drv_handle;

	uint32_t rsmu_cmd_addr;
	uint32_t rsmu_rsp_addr;
	uint32_t rsmu_arg_addr;
} ry_handle_t;

ry_handle_t* ryzen_smu_init(struct wr0_drv_t* drv_handle, struct cpu_id_t* id);

void ryzen_smu_free(ry_handle_t* handle);

ry_err_t ryzen_smu_init_pm_table(ry_handle_t* handle);

ry_err_t ryzen_smu_update_pm_table(ry_handle_t* handle);

ry_err_t ryzen_smu_get_pm_table_float(ry_handle_t* handle, size_t offset, float* value);

ry_err_t ryzen_smu_get_stapm_limit(ry_handle_t* handle, float* data);

ry_err_t ryzen_smu_get_stapm_value(ry_handle_t* handle, float* data);

ry_err_t ryzen_smu_get_fast_limit(ry_handle_t* handle, float* data);

ry_err_t ryzen_smu_get_fast_value(ry_handle_t* handle, float* data);

ry_err_t ryzen_smu_get_slow_limit(ry_handle_t* handle, float* data);

ry_err_t ryzen_smu_get_slow_value(ry_handle_t* handle, float* data);

ry_err_t ryzen_smu_get_core_temperature(ry_handle_t* handle, uint32_t core, float* data);

ry_err_t ryzen_smu_get_apu_temperature(ry_handle_t* handle, float* data);
