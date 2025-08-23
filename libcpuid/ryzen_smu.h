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

	// Zen / Zen+ (Family 17h)
	CODENAME_SUMMITRIDGE,
	CODENAME_NAPLES,
	CODENAME_PINNACLERIDGE,
	CODENAME_THREADRIPPER,
	CODENAME_COLFAX,
	CODENAME_RAVENRIDGE,
	CODENAME_PICASSO,
	CODENAME_RAVENRIDGE2,
	CODENAME_DALI,

	// Zen 2 (Family 17h)
	CODENAME_MATISSE,
	CODENAME_CASTLEPEAK,
	CODENAME_RENOIR,
	CODENAME_LUCIENNE,
	CODENAME_VANGOGH,
	CODENAME_MENDOCINO,

	// Zen 3 / Zen 3+ (Family 19h)
	CODENAME_VERMEER,
	CODENAME_MILAN,
	CODENAME_CEZANNE,
	CODENAME_CHAGALL,
	CODENAME_REMBRANDT,

	// Zen 4 (Family 19h)
	CODENAME_RAPHAEL,
	CODENAME_PHOENIX,
	CODENAME_HAWKPOINT,
	CODENAME_DRAGONRANGE,

	// Zen 5 (Family 1Ah)
	CODENAME_GRANITERIDGE,
	CODENAME_STRIXPOINT,
	CODENAME_STRIXHALO,
	CODENAME_FIRERANGE,
	CODENAME_KRACKANPOINT,

	// Server/HEDT specific
	CODENAME_STORMPEAK,

	CODENAME_COUNT,
} ry_codename_t;

typedef enum
{
	IF_VERSION_9 = 9,
	IF_VERSION_10 = 10,
	IF_VERSION_11 = 11,
	IF_VERSION_12 = 12,
	IF_VERSION_13 = 13,
	IF_VERSION_UNKNOWN,
} ry_ifver_t;

typedef enum
{
	MAILBOX_TYPE_RSMU,
	MAILBOX_TYPE_MP1,
	MAILBOX_TYPE_HSMP,
} ry_mailbox_t;

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

typedef struct
{
	int debug;
	ry_codename_t codename;
	ry_ifver_t if_version;
	uint32_t smu_version;
	uint32_t pci_id;

	uint64_t pm_table_base_addr;
	uint32_t pm_table_version;
	size_t pm_table_size;
	void* pm_table_buffer;

	struct wr0_drv_t* drv_handle;

	uint32_t rsmu_cmd_addr;
	uint32_t rsmu_rsp_addr;
	uint32_t rsmu_args_addr;

	uint32_t mp1_cmd_addr;
	uint32_t mp1_rsp_addr;
	uint32_t mp1_args_addr;

	uint32_t hsmp_cmd_addr;
	uint32_t hsmp_rsp_addr;
	uint32_t hsmp_args_addr;

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
