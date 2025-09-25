// SPDX-License-Identifier: Unlicense

#include "ryzen_smu.h"
#include "libcpuid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <intrin.h>
#include <winring0.h>

#define SMU_PCI_ADDR_REG 0xC4
#define SMU_PCI_DATA_REG 0xC8
#define SMU_COMMAND_MAX_RETRIES 8096
#define SMU_MAX_ARGS 6

typedef struct
{
	uint32_t rsmu_cmd;
	uint32_t rsmu_rsp;
	uint32_t rsmu_args;

	uint32_t mp1_cmd;
	uint32_t mp1_rsp;
	uint32_t mp1_args;

	uint32_t hsmp_cmd;
	uint32_t hsmp_rsp;
	uint32_t hsmp_args;
} smu_mailbox_addresses_t;

#define SMU_DEBUG(...) \
	do \
	{ \
		if (handle->debug) \
		{ \
			printf("[SMU] " __VA_ARGS__); \
			puts(""); \
		} \
	} while (0)

static const char* get_codename_str(ry_codename_t codename)
{
	switch (codename)
	{
	case CODENAME_SUMMITRIDGE: return "Summit Ridge";
	case CODENAME_NAPLES: return "Naples";
	case CODENAME_PINNACLERIDGE: return "Pinnacle Ridge";
	case CODENAME_THREADRIPPER: return "ThreadRipper";
	case CODENAME_COLFAX: return "Colfax";
	case CODENAME_RAVENRIDGE: return "Raven Ridge";
	case CODENAME_PICASSO: return "Picasso";
	case CODENAME_RAVENRIDGE2: return "Raven Ridge 2";
	case CODENAME_DALI: return "Dali";
	case CODENAME_MATISSE: return "Matisse";
	case CODENAME_CASTLEPEAK: return "Castle Peak";
	case CODENAME_RENOIR: return "Renoir";
	case CODENAME_LUCIENNE: return "Lucienne";
	case CODENAME_VANGOGH: return "Van Gogh";
	case CODENAME_MENDOCINO: return "Mendocino";
	case CODENAME_VERMEER: return "Vermeer";
	case CODENAME_MILAN: return "Milan";
	case CODENAME_CEZANNE: return "Cezanne";
	case CODENAME_CHAGALL: return "Chagall";
	case CODENAME_REMBRANDT: return "Rembrandt";
	case CODENAME_RAPHAEL: return "Raphael";
	case CODENAME_PHOENIX: return "Phoenix";
	case CODENAME_HAWKPOINT: return "Hawk Point";
	case CODENAME_DRAGONRANGE: return "Dragon Range";
	case CODENAME_GRANITERIDGE: return "Granite Ridge";
	case CODENAME_STRIXPOINT: return "Strix Point";
	case CODENAME_STRIXHALO: return "Strix Halo";
	case CODENAME_FIRERANGE: return "Fire Range";
	case CODENAME_KRACKANPOINT: return "Krackan Point";
	case CODENAME_STORMPEAK: return "Storm Peak";
	default: return "Unknown";
	}
}

static const char* get_ry_err_str(ry_err_t code)
{
	switch (code)
	{
	case RYZEN_SMU_OK: return "OK";
	case RYZEN_SMU_FAILED: return "Failed";
	case RYZEN_SMU_UNKNOWN_CMD: return "Unknown Command";
	case RYZEN_SMU_CMD_REJECTED_PREREQ: return "Command Rejected - Prerequisite Unmet";
	case RYZEN_SMU_CMD_REJECTED_BUSY: return "Command Rejected - Busy";
	case RYZEN_SMU_CMD_TIMEOUT: return "Command Timed Out";
	case RYZEN_SMU_UNSUPPORTED: return "Unsupported Platform or Feature";
	case RYZEN_SMU_DRIVER_ERROR: return "Driver Communication Error";
	case RYZEN_SMU_CPU_NOT_SUPPORTED: return "CPU Not Supported";
	case RYZEN_SMU_NOT_INITIALIZED: return "Library Not Initialized";
	case RYZEN_SMU_INVALID_ARGUMENT: return "Invalid Argument";
	case RYZEN_SMU_MAPPING_ERROR: return "Memory Mapping or Allocation Error";
	case RYZEN_SMU_INSUFFICIENT_BUFFER: return "Insufficient Buffer";
	default: return "Unspecified Error";
	}
}

static ry_err_t get_codename(ry_handle_t* handle, struct cpu_id_t* id)
{
	if (id->vendor != VENDOR_AMD)
		return RYZEN_SMU_CPU_NOT_SUPPORTED;

	handle->codename = CODENAME_UNKNOWN;

	switch (id->x86.ext_family)
	{
	case 0x17: // Zen, Zen+, Zen2
		switch (id->x86.ext_model)
		{
		case 0x01:
			if (id->x86.pkg_type == 7)
				handle->codename = CODENAME_THREADRIPPER;
			else if (id->x86.pkg_type == 4)
				handle->codename = CODENAME_NAPLES;
			else
				handle->codename = CODENAME_SUMMITRIDGE;
			break;
		case 0x08:
			if (id->x86.pkg_type == 7 || id->x86.pkg_type == 4)
				handle->codename = CODENAME_COLFAX;
			else
				handle->codename = CODENAME_PINNACLERIDGE;
			break;
		case 0x11:
			handle->codename = CODENAME_RAVENRIDGE;
			break;
		case 0x18:
			if (id->x86.pkg_type == 2)
				handle->codename = CODENAME_RAVENRIDGE2;
			else
				handle->codename = CODENAME_PICASSO;
			break;
		case 0x20:
			handle->codename = CODENAME_DALI;
			break;
		case 0x31:
			handle->codename = CODENAME_CASTLEPEAK;
			break;
		case 0x60:
			handle->codename = CODENAME_RENOIR;
			break;
		case 0x68:
			handle->codename = CODENAME_LUCIENNE;
			break;
		case 0x71:
			handle->codename = CODENAME_MATISSE;
			break;
		case 0x90:
		case 0x91:
			handle->codename = CODENAME_VANGOGH;
			break;
		case 0xA0:
			handle->codename = CODENAME_MENDOCINO;
			break;
		}
		break;
	case 0x19: // Zen3, Zen3+, Zen4
		switch (id->x86.ext_model)
		{
		case 0x01:
			handle->codename = CODENAME_MILAN;
			break;
		case 0x08:
			handle->codename = CODENAME_CHAGALL;
			break;
		case 0x18:
			handle->codename = CODENAME_STORMPEAK;
			break;
		case 0x20:
		case 0x21:
			handle->codename = CODENAME_VERMEER;
			break;
		case 0x40:
		case 0x44:
			handle->codename = CODENAME_REMBRANDT;
			break;
		case 0x50:
			handle->codename = CODENAME_CEZANNE;
			break;
		case 0x61:
			handle->codename = CODENAME_RAPHAEL;
			// CODENAME_DRAGONRANGE ??
			break;
		case 0x74:
		case 0x78:
			handle->codename = CODENAME_PHOENIX;
			break;
		case 0x75:
			handle->codename = CODENAME_HAWKPOINT;
			break;
		}
		break;
	case 0x1A: // Zen5
		switch (id->x86.ext_model)
		{
		case 0x20:
		case 0x24:
			handle->codename = CODENAME_STRIXPOINT;
			break;
		case 0x44:
			handle->codename = CODENAME_GRANITERIDGE;
			// CODENAME_FIRERANGE ??
			break;
		case 0x60:
			handle->codename = CODENAME_KRACKANPOINT;
			break;
		case 0x70:
			handle->codename = CODENAME_STRIXHALO;
			break;
		}
		break;
	}
	if (handle->codename == CODENAME_UNKNOWN)
	{
		SMU_DEBUG("Unsupported CPU: Family %Xh, Model %Xh, Package %Xh",
			id->x86.ext_family, id->x86.ext_model, id->x86.pkg_type);
		return RYZEN_SMU_CPU_NOT_SUPPORTED;
	}
	SMU_DEBUG("Detected CPU: %s (Family %Xh, Model %Xh, Package %Xh)",
		get_codename_str(handle->codename), id->x86.ext_family, id->x86.ext_model, id->x86.pkg_type);
	return RYZEN_SMU_OK;
}

static ry_err_t get_mailbox_addr(ry_handle_t* handle)
{
	switch (handle->codename)
	{
	case CODENAME_SUMMITRIDGE:
	case CODENAME_NAPLES:
	case CODENAME_PINNACLERIDGE:
	case CODENAME_THREADRIPPER:
	case CODENAME_COLFAX:
		handle->rsmu_cmd_addr = 0x3B1051C;
		handle->rsmu_rsp_addr = 0x3B10568;
		handle->rsmu_args_addr = 0x3B10590;
		// no hsmp
		handle->mp1_cmd_addr = 0x3B10528;
		handle->mp1_rsp_addr = 0x3B10564;
		handle->mp1_args_addr = 0x3B10598;
		handle->if_version = IF_VERSION_9;
		break;
	case CODENAME_RAVENRIDGE:
	case CODENAME_PICASSO:
	case CODENAME_RAVENRIDGE2:
	case CODENAME_DALI:
		handle->rsmu_cmd_addr = 0x3B10A20;
		handle->rsmu_rsp_addr = 0x3B10A80;
		handle->rsmu_args_addr = 0x3B10A88;
		// no hsmp
		handle->mp1_cmd_addr = 0x3B10528;
		handle->mp1_rsp_addr = 0x3B10564;
		handle->mp1_args_addr = 0x3B10998;
		handle->if_version = IF_VERSION_10;
		break;
	case CODENAME_MATISSE:
	case CODENAME_CASTLEPEAK:
	case CODENAME_VERMEER:
	case CODENAME_MILAN:
	case CODENAME_CHAGALL:
	case CODENAME_RAPHAEL:
	case CODENAME_GRANITERIDGE:
	case CODENAME_STORMPEAK:
	case CODENAME_DRAGONRANGE: // ?
	case CODENAME_FIRERANGE: // ?
		handle->rsmu_cmd_addr = 0x3B10524;
		handle->rsmu_rsp_addr = 0x3B10570;
		handle->rsmu_args_addr = 0x3B10A40;
		handle->hsmp_cmd_addr = 0x3B10534;
		handle->hsmp_rsp_addr = 0x3B10980;
		handle->hsmp_args_addr = 0x3B109E0;
		handle->mp1_cmd_addr = 0x3B10530;
		handle->mp1_rsp_addr = 0x3B1057C;
		handle->mp1_args_addr = 0x3B109C4;
		handle->if_version = IF_VERSION_11;
		break;
	case CODENAME_RENOIR:
	case CODENAME_LUCIENNE:
	case CODENAME_CEZANNE:
		handle->rsmu_cmd_addr = 0x3B10A20;
		handle->rsmu_rsp_addr = 0x3B10A80;
		handle->rsmu_args_addr = 0x3B10A88;
		// no hsmp
		handle->mp1_cmd_addr = 0x3B10528;
		handle->mp1_rsp_addr = 0x3B10564;
		handle->mp1_args_addr = 0x3B10998;
		handle->if_version = IF_VERSION_12;
		break;
	case CODENAME_VANGOGH: // ?
	case CODENAME_REMBRANDT:
	case CODENAME_PHOENIX:
	case CODENAME_HAWKPOINT:
	case CODENAME_MENDOCINO: // ?
		handle->rsmu_cmd_addr = 0x3B10A20;
		handle->rsmu_rsp_addr = 0x3B10A80;
		handle->rsmu_args_addr = 0x3B10A88;
		// no hsmp
		handle->mp1_cmd_addr = 0x3B10528;
		handle->mp1_rsp_addr = 0x3B10578;
		handle->mp1_args_addr = 0x3B10998;
		handle->if_version = IF_VERSION_13;
		break;
	case CODENAME_STRIXPOINT:
	case CODENAME_STRIXHALO:
	case CODENAME_KRACKANPOINT:
		handle->rsmu_cmd_addr = 0x3B10A20;
		handle->rsmu_rsp_addr = 0x3B10A80;
		handle->rsmu_args_addr = 0x3B10A88;
		// no hsmp
		handle->mp1_cmd_addr = 0x3B10928;
		handle->mp1_rsp_addr = 0x3B10978;
		handle->mp1_args_addr = 0x3B10998;
		handle->if_version = IF_VERSION_13;
		break;
	default:
		return RYZEN_SMU_CPU_NOT_SUPPORTED;
	}
	SMU_DEBUG("RSMU(%X,%X,%X), MP1(%X,%X,%X), HSMP(%X,%X,%X)",
		handle->rsmu_cmd_addr, handle->rsmu_rsp_addr, handle->rsmu_args_addr,
		handle->mp1_cmd_addr, handle->mp1_rsp_addr, handle->mp1_args_addr,
		handle->hsmp_cmd_addr, handle->hsmp_rsp_addr, handle->hsmp_args_addr);
	return RYZEN_SMU_OK;
}

static ry_err_t read_smn(ry_handle_t* handle, uint32_t address, uint32_t* value)
{
	WR0_WrPciConf32(handle->drv_handle, 0, SMU_PCI_ADDR_REG, address);
	uint32_t ret = WR0_RdPciConf32(handle->drv_handle, 0, SMU_PCI_DATA_REG);
	if (ret == 0xFFFFFFFF)
	{
		SMU_DEBUG("SMN 0x%X Read ERROR", address);
		return RYZEN_SMU_FAILED;
	}
	*value = ret;
	SMU_DEBUG("SMN 0x%X Read: 0x%X", address, *value);
	return RYZEN_SMU_OK;
}

static ry_err_t write_smn(ry_handle_t* handle, uint32_t address, uint32_t value)
{
	WR0_WrPciConf32(handle->drv_handle, 0, SMU_PCI_ADDR_REG, address);
	WR0_WrPciConf32(handle->drv_handle, 0, SMU_PCI_DATA_REG, value);
	SMU_DEBUG("SMN 0x%X Write: 0x%X", address, value);
	return RYZEN_SMU_OK;
}

static ry_err_t send_command(ry_handle_t* handle, ry_mailbox_t mailbox, uint32_t fn, ry_args_t* args)
{
	int i;
	ry_err_t rc;
	uint32_t cmd_addr, rsp_addr, args_addr;
	uint32_t response;
	const char* mb_type_str = "ERR";

	switch (mailbox)
	{
	case MAILBOX_TYPE_RSMU:
		cmd_addr = handle->rsmu_cmd_addr;
		rsp_addr = handle->rsmu_rsp_addr;
		args_addr = handle->rsmu_args_addr;
		mb_type_str = "RSMU";
		break;
	case MAILBOX_TYPE_MP1:
		cmd_addr = handle->mp1_cmd_addr;
		rsp_addr = handle->mp1_rsp_addr;
		args_addr = handle->mp1_args_addr;
		mb_type_str = "MP1";
		break;
	case MAILBOX_TYPE_HSMP:
		cmd_addr = handle->hsmp_cmd_addr;
		rsp_addr = handle->hsmp_rsp_addr;
		args_addr = handle->hsmp_args_addr;
		mb_type_str = "HSMP";
		break;
	default:
		return RYZEN_SMU_INVALID_ARGUMENT;
	}

	if (cmd_addr == 0)
		return RYZEN_SMU_UNSUPPORTED;

	SMU_DEBUG("Send %s Fn %Xh", mb_type_str, fn);

	// Wait until the RSP register is non-zero.
	for (i = 0, response = 0; i < SMU_COMMAND_MAX_RETRIES; i++)
	{
		rc = read_smn(handle, rsp_addr, &response);
		if (rc != RYZEN_SMU_OK)
		{
			SMU_DEBUG("Failed to read RSP register: %s", get_ry_err_str(rc));
			return rc;
		}
		if (response != 0)
			break;
	}

	if (response == 0)
		return RYZEN_SMU_CMD_TIMEOUT;

	// Write zero to the RSP register
	write_smn(handle, rsp_addr, 0);

	// Write the arguments into the argument registers
	for (int i = 0; i < SMU_MAX_ARGS; i++)
	{
		rc = write_smn(handle, args_addr + (i * 4), args->args[i]);
		if (rc != RYZEN_SMU_OK)
			return rc;
	}

	// Write the message Id into the Message ID register
	write_smn(handle, cmd_addr, fn);

	// Wait until the Response register is non-zero.
	for (i = 0, response = 0; i < SMU_COMMAND_MAX_RETRIES; i++)
	{
		rc = read_smn(handle, rsp_addr, &response);
		if (rc != RYZEN_SMU_OK)
			return rc;
		if (response != 0)
			break;
	}

	if (response == 0)
		return RYZEN_SMU_CMD_TIMEOUT;

	// Read the response value
	for (int i = 0; i < SMU_MAX_ARGS; i++)
	{
		rc = read_smn(handle, args_addr + (i * 4), &args->args[i]);
		if (rc != RYZEN_SMU_OK)
			return rc;
	}

	return RYZEN_SMU_OK;
}

static void init_smu_args(ry_args_t* args, uint32_t value)
{
	uint32_t i;
	args->args[0] = value;
	for (i = 1; i < SMU_MAX_ARGS; i++)
		args->args[i] = 0;
}

static ry_err_t get_smu_version(ry_handle_t* handle)
{
	ry_args_t args;
	init_smu_args(&args, 1);
	// OP 0x02 is consistent with all platforms
	ry_err_t rc = send_command(handle, MAILBOX_TYPE_MP1, 0x02, &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	handle->smu_version = args.u32.arg0;
	SMU_DEBUG("SMU Version %Xh", handle->smu_version);
	return RYZEN_SMU_OK;
}

static ry_err_t get_pm_table_base(ry_handle_t* handle, uint64_t* addr)
{
	ry_args_t args;
	ry_err_t rc;
	uint32_t fn[3];
	uint32_t low_part, high_part;

	switch (handle->codename)
	{
	case CODENAME_NAPLES:
	case CODENAME_SUMMITRIDGE:
	case CODENAME_THREADRIPPER:
		fn[0] = 0xa;
		goto base_addr_class_1;
	case CODENAME_VERMEER:
	case CODENAME_MATISSE:
	case CODENAME_CASTLEPEAK:
	case CODENAME_MILAN:
	case CODENAME_CHAGALL:
		fn[0] = 0x06;
		goto base_addr_class_1;
	case CODENAME_RAPHAEL:
	case CODENAME_GRANITERIDGE:
	case CODENAME_STORMPEAK:
	case CODENAME_DRAGONRANGE: // ?
	case CODENAME_FIRERANGE: // ?
		fn[0] = 0x04;
		goto base_addr_class_1;
	case CODENAME_RENOIR:
	case CODENAME_LUCIENNE:
	case CODENAME_CEZANNE:
	case CODENAME_REMBRANDT:
	case CODENAME_PHOENIX:
	case CODENAME_HAWKPOINT:
	case CODENAME_STRIXPOINT:
	case CODENAME_STRIXHALO:
	case CODENAME_KRACKANPOINT:
	case CODENAME_VANGOGH: // ?
	case CODENAME_MENDOCINO: // ?
		fn[0] = 0x66;
		goto base_addr_class_1;

	case CODENAME_COLFAX:
	case CODENAME_PINNACLERIDGE:
		fn[0] = 0x0b;
		fn[1] = 0x0c;
		goto base_addr_class_2;

	case CODENAME_DALI:
	case CODENAME_PICASSO:
	case CODENAME_RAVENRIDGE:
	case CODENAME_RAVENRIDGE2:
		fn[0] = 0x0a;
		fn[1] = 0x3d;
		fn[2] = 0x0b;
		goto base_addr_class_3;

	default:
		return RYZEN_SMU_UNSUPPORTED;
	}

base_addr_class_1:
	init_smu_args(&args, 1);
	args.u32.arg1 = 1;
	rc = send_command(handle, MAILBOX_TYPE_RSMU, fn[0], &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	*addr = ((uint64_t)args.u32.arg1 << 32) | args.u32.arg0;
	SMU_DEBUG("PM Table Base: %llX", (unsigned long long)addr);
	return RYZEN_SMU_OK;

base_addr_class_2:
	init_smu_args(&args, 0);
	rc = send_command(handle, MAILBOX_TYPE_RSMU, fn[0], &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	init_smu_args(&args, 0);
	rc = send_command(handle, MAILBOX_TYPE_RSMU, fn[1], &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	*addr = (uint64_t)args.u32.arg0;
	SMU_DEBUG("PM Table Base: %llX", (unsigned long long)addr);
	return RYZEN_SMU_OK;

base_addr_class_3:
	init_smu_args(&args, 3);
	rc = send_command(handle, MAILBOX_TYPE_RSMU, fn[0], &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	init_smu_args(&args, 3);
	rc = send_command(handle, MAILBOX_TYPE_RSMU, fn[2], &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	low_part = args.u32.arg0;

	init_smu_args(&args, 3);
	rc = send_command(handle, MAILBOX_TYPE_RSMU, fn[1], &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	init_smu_args(&args, 5);
	rc = send_command(handle, MAILBOX_TYPE_RSMU, fn[0], &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	init_smu_args(&args, 5);
	rc = send_command(handle, MAILBOX_TYPE_RSMU, fn[2], &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	high_part = args.u32.arg0;

	*addr = ((uint64_t)high_part << 32) | low_part;
	SMU_DEBUG("PM Table Base: %llX", (unsigned long long)addr);
	return RYZEN_SMU_OK;
}

static ry_err_t get_pm_table_version(ry_handle_t* handle, uint32_t* version)
{
	ry_err_t rc;
	ry_args_t args;
	uint32_t fn;

	switch (handle->codename)
	{
	case CODENAME_DALI: // ?
	case CODENAME_PICASSO:
	case CODENAME_RAVENRIDGE:
	case CODENAME_RAVENRIDGE2: // ?
		fn = 0x0C;
		break;
	case CODENAME_VERMEER:
	case CODENAME_MATISSE:
	case CODENAME_CASTLEPEAK:
	case CODENAME_MILAN:
	case CODENAME_CHAGALL:
		fn = 0x08;
		break;
	case CODENAME_RAPHAEL:
	case CODENAME_GRANITERIDGE:
	case CODENAME_STORMPEAK:
	case CODENAME_DRAGONRANGE: // ?
	case CODENAME_FIRERANGE: // ?
		fn = 0x05;
		break;
	case CODENAME_RENOIR:
	case CODENAME_LUCIENNE:
	case CODENAME_CEZANNE:
	case CODENAME_REMBRANDT:
	case CODENAME_PHOENIX:
	case CODENAME_HAWKPOINT:
	case CODENAME_STRIXPOINT:
	case CODENAME_STRIXHALO:
	case CODENAME_KRACKANPOINT:
	case CODENAME_VANGOGH: // ?
	case CODENAME_MENDOCINO: // ?
		fn = 0x06;
		break;
	default:
		return RYZEN_SMU_UNSUPPORTED;
	}

	init_smu_args(&args, 0);
	rc = send_command(handle, MAILBOX_TYPE_RSMU, fn, &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	*version = args.u32.arg0;
	SMU_DEBUG("PM Table Version: %X", args.u32.arg0);
	return RYZEN_SMU_OK;
}

static size_t get_pm_table_size_from_version(ry_handle_t* handle, uint32_t version)
{
	switch (handle->codename)
	{
	case CODENAME_PICASSO:
	case CODENAME_RAVENRIDGE:
	case CODENAME_RAVENRIDGE2:
		// These codenames have two PM tables, a larger (primary) one and a smaller
		// one. The size is always fixed to 0x608 and 0xA4 bytes each.
		return 0x608 + 0xA4;
	}
	switch (version)
	{
	case 0x000400: return 0x948;  // CODENAME_RAPHAEL ??
	case 0x1E0001: return 0x568;
	case 0x1E0002: return 0x580;
	case 0x1E0003: return 0x578;
	case 0x1E0004:
	case 0x1E0005:
	case 0x1E000A:
	case 0x1E0101: return 0x608;
	case 0x240003: return 0x18AC; // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x240503: return 0xD7C;  // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x240603: return 0xAB0;  // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x240703: return 0x7E4;  // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x240802: return 0x7E0;  // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x240803: return 0x7E4;  // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x240902: return 0x514;  // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x240903: return 0x518;  // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x2D0008: return 0x1AB0; // CODENAME_MILAN
	case 0x2D0803: return 0x894;  // CODENAME_VERMEER, CODENAME_CHAGALL
	case 0x2D0903: return 0x594;  // CODENAME_VERMEER, CODENAME_CHAGALL
	case 0x370000: return 0x794;  // CODENAME_RENOIR, CODENAME_LUCIENNE
	case 0x370001: return 0x884;  // CODENAME_RENOIR, CODENAME_LUCIENNE
	case 0x370002: return 0x88C;  // CODENAME_RENOIR, CODENAME_LUCIENNE
	case 0x370003:
	case 0x370004: return 0x8AC;  // CODENAME_RENOIR, CODENAME_LUCIENNE
	case 0x370005: return 0x8C8;  // CODENAME_RENOIR, CODENAME_LUCIENNE
	case 0x380005: return 0x1BB0; // CODENAME_VERMEER, CODENAME_CHAGALL
	case 0x380505: return 0xF30;  // CODENAME_VERMEER, CODENAME_CHAGALL
	case 0x380605: return 0xC10;  // CODENAME_VERMEER, CODENAME_CHAGALL
	case 0x380705: return 0x8F0;  // CODENAME_VERMEER, CODENAME_CHAGALL
	case 0x380804: return 0x8A4;  // CODENAME_VERMEER, CODENAME_CHAGALL
	case 0x380805: return 0x8F0;  // CODENAME_VERMEER, CODENAME_CHAGALL
	case 0x380904: return 0x5A4;  // CODENAME_VERMEER, CODENAME_CHAGALL
	case 0x380905: return 0x5D0;  // CODENAME_VERMEER, CODENAME_CHAGALL
	case 0x3F0000: return 0x7AC;
	case 0x400001: return 0x910;
	case 0x400002: return 0x928;
	case 0x400003: return 0x94C;
	case 0x400004:
	case 0x400005: return 0x944;  // CODENAME_CEZANNE
	case 0x450004: return 0xAA4;  // CODENAME_REMBRANDT
	case 0x450005: return 0xAB0;  // CODENAME_REMBRANDT
	case 0x4C0003: return 0xB18;  // CODENAME_PHOENIX, CODENAME_HAWKPOINT
	case 0x4C0004: return 0xB1C;  // CODENAME_PHOENIX, CODENAME_HAWKPOINT
	case 0x4C0005: return 0xAF8;  // CODENAME_PHOENIX, CODENAME_HAWKPOINT
	case 0x4C0006: return 0xAFC;  // CODENAME_PHOENIX, CODENAME_HAWKPOINT
	case 0x4C0007: return 0xB00;  // CODENAME_PHOENIX, CODENAME_HAWKPOINT
	case 0x4C0008: return 0xAF0;  // CODENAME_PHOENIX, CODENAME_HAWKPOINT
	case 0x4C0009: return 0xB00;  // CODENAME_PHOENIX, CODENAME_HAWKPOINT
	case 0x540000: return 0x828;  // CODENAME_RAPHAEL
	case 0x540001: return 0x82C;  // CODENAME_RAPHAEL
	case 0x540002: return 0x87C;  // CODENAME_RAPHAEL
	case 0x540003: return 0x89C;  // CODENAME_RAPHAEL
	case 0x540004: return 0x8BC;  // CODENAME_RAPHAEL
	case 0x540005: return 0x8C8;  // CODENAME_RAPHAEL
	case 0x540100: return 0x618;  // CODENAME_RAPHAEL
	case 0x540101: return 0x61C;  // CODENAME_RAPHAEL
	case 0x540102: return 0x66C;  // CODENAME_RAPHAEL
	case 0x540103: return 0x68C;  // CODENAME_RAPHAEL
	case 0x540104: return 0x6A8;  // CODENAME_RAPHAEL
	case 0x540105: return 0x6B4;  // CODENAME_RAPHAEL
	case 0x540108: return 0x6BC;  // CODENAME_RAPHAEL
	case 0x540208: return 0x8D0;  // CODENAME_RAPHAEL
	case 0x5C0002: return 0x1E3C; // CODENAME_STORMPEAK
	case 0x5C0003: return 0x1E48; // CODENAME_STORMPEAK
	case 0x5C0102: return 0x1A14; // CODENAME_STORMPEAK
	case 0x5C0103: return 0x1A20; // CODENAME_STORMPEAK
	case 0x5C0202: return 0x15EC; // CODENAME_STORMPEAK
	case 0x5C0203: return 0x15F8; // CODENAME_STORMPEAK
	case 0x5C0302: return 0xD9C;  // CODENAME_STORMPEAK
	case 0x5C0303: return 0xDA8;  // CODENAME_STORMPEAK
	case 0x5C0402: return 0x974;  // CODENAME_STORMPEAK
	case 0x5C0403: return 0x980;  // CODENAME_STORMPEAK
	case 0x5D0008: return 0xD54;  // CODENAME_STRIXPOINT
	case 0x5D0009: return 0xD58;  // CODENAME_STRIXPOINT
	case 0x620105: return 0x724;  // CODENAME_GRANITERIDGE
	case 0x620205: return 0x994;  // CODENAME_GRANITERIDGE
	case 0x621101:
	case 0x621102: return 0x724;  // CODENAME_GRANITERIDGE
	case 0x621201:
	case 0x621202: return 0x994;  // CODENAME_GRANITERIDGE
	case 0x640107: return 0xDC0;  // CODENAME_STRIXHALO
	case 0x640108: return 0xDC4;  // CODENAME_STRIXHALO
	case 0x640109:
	case 0x64010A: return 0xDD4;  // CODENAME_STRIXHALO
	case 0x64010C: return 0xDDC;  // CODENAME_STRIXHALO
	case 0x640207: return 0x100C; // CODENAME_STRIXHALO
	case 0x640208: return 0x1010; // CODENAME_STRIXHALO
	case 0x640209:
	case 0x64020A: return 0x1020; // CODENAME_STRIXHALO
	case 0x64020C: return 0x1028; // CODENAME_STRIXHALO
	case 0x650004: return 0xB74;  // CODENAME_KRACKANPOINT
	case 0x650005: return 0xB78;  // CODENAME_KRACKANPOINT
	}
	return 0x2000;
}

static ry_err_t transfer_table_to_dram(ry_handle_t* handle)
{
	ry_args_t args;
	uint32_t fn;

	init_smu_args(&args, 0);
	switch (handle->codename)
	{
	case CODENAME_RAPHAEL:
	case CODENAME_GRANITERIDGE:
	case CODENAME_STORMPEAK:
	case CODENAME_DRAGONRANGE: // ?
	case CODENAME_FIRERANGE: // ?
		fn = 0x03;
		break;
	case CODENAME_VERMEER:
	case CODENAME_MATISSE:
	case CODENAME_CASTLEPEAK:
	case CODENAME_MILAN:
	case CODENAME_CHAGALL:
		fn = 0x05;
		break;
	case CODENAME_CEZANNE:
		fn = 0x65;
		break;
	case CODENAME_RENOIR:
	case CODENAME_LUCIENNE:
	case CODENAME_REMBRANDT:
	case CODENAME_PHOENIX:
	case CODENAME_HAWKPOINT:
	case CODENAME_STRIXPOINT:
	case CODENAME_STRIXHALO:
	case CODENAME_KRACKANPOINT:
	case CODENAME_VANGOGH: // ?
	case CODENAME_MENDOCINO: // ?
		args.u32.arg0 = 3;
		fn = 0x65;
		break;
	case CODENAME_SUMMITRIDGE:
	case CODENAME_THREADRIPPER:
	case CODENAME_NAPLES:
		fn = 0x0a;
		break;
	case CODENAME_COLFAX:
	case CODENAME_PINNACLERIDGE:
	case CODENAME_PICASSO:
	case CODENAME_RAVENRIDGE:
	case CODENAME_RAVENRIDGE2:
	case CODENAME_DALI: // ?
		args.u32.arg0 = 3;
		fn = 0x3d;
		break;
	default:
		return RYZEN_SMU_UNSUPPORTED;
	}
	return send_command(handle, MAILBOX_TYPE_RSMU, fn, &args);
}

ry_handle_t* ryzen_smu_init(struct wr0_drv_t* drv_handle, struct cpu_id_t* id)
{
	if (!drv_handle)
		return NULL;

	ry_handle_t* handle = (ry_handle_t*)calloc(1, sizeof(ry_handle_t));
	if (!handle)
		return NULL;

	handle->drv_handle = drv_handle;
	handle->debug = drv_handle->debug;

	if (get_codename(handle, id) != RYZEN_SMU_OK)
		goto fail;

	handle->pci_id = WR0_RdPciConf32(handle->drv_handle, 0, 0);
	if ((handle->pci_id & 0xFFFF) != 0x1022)
		goto fail;

	if (get_mailbox_addr(handle) != RYZEN_SMU_OK)
		goto fail;

	if (get_smu_version(handle) != RYZEN_SMU_OK)
		goto fail;

	return handle;
fail:
	ryzen_smu_free(handle);
	return NULL;
}

void ryzen_smu_free(ry_handle_t* handle) 
{
	if (!handle)
		return;
	if (handle->pm_table_buffer)
		free(handle->pm_table_buffer);
	free(handle);
}

ry_err_t ryzen_smu_init_pm_table(ry_handle_t* handle)
{
	if (handle->rsmu_cmd_addr == 0)
		return RYZEN_SMU_UNSUPPORTED;

	uint64_t base_addr = 0;
	ry_err_t rc;

	rc = get_pm_table_base(handle, &base_addr);
	if (rc != RYZEN_SMU_OK)
		return rc;
	handle->pm_table_base_addr = base_addr;

	rc = get_pm_table_version(handle, &handle->pm_table_version);
	if (rc != RYZEN_SMU_OK && rc != RYZEN_SMU_UNSUPPORTED)
		return rc;

	handle->pm_table_size = get_pm_table_size_from_version(handle, handle->pm_table_version);
	SMU_DEBUG("PM Table Size: %zu", handle->pm_table_size);

	if (handle->pm_table_buffer)
		free(handle->pm_table_buffer);

	handle->pm_table_buffer = calloc(1, handle->pm_table_size);
	if (!handle->pm_table_buffer)
		return RYZEN_SMU_MAPPING_ERROR;

	return RYZEN_SMU_OK;
}

ry_err_t ryzen_smu_update_pm_table(ry_handle_t* handle)
{
	if (!handle->pm_table_base_addr || !handle->pm_table_buffer)
		return RYZEN_SMU_UNSUPPORTED;

	ry_err_t rc = transfer_table_to_dram(handle);
	if (rc != RYZEN_SMU_OK)
		return rc;

	if (WR0_RdMem(handle->drv_handle, (DWORD_PTR)handle->pm_table_base_addr, handle->pm_table_buffer, (DWORD)handle->pm_table_size, 1)
		!= handle->pm_table_size)
		return RYZEN_SMU_DRIVER_ERROR;

	return RYZEN_SMU_OK;
}

#define OFFSET_INVALID ((size_t)0xFFFFFFFF)

ry_err_t ryzen_smu_get_pm_table_float(ry_handle_t* handle, size_t offset, float* value)
{
	if (!handle || !handle->pm_table_buffer)
		return RYZEN_SMU_NOT_INITIALIZED;
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	if (offset + sizeof(float) > handle->pm_table_size)
		return RYZEN_SMU_INVALID_ARGUMENT;
	memcpy(value, (uint8_t*)handle->pm_table_buffer + offset, sizeof(float));
	SMU_DEBUG("Get PM Table Float at offset 0x%zX, value %.2f", offset, *value);
	return RYZEN_SMU_OK;
}

ry_err_t ryzen_smu_get_stapm_limit(ry_handle_t* handle, float* data)
{
	return ryzen_smu_get_pm_table_float(handle, 0x00, data);
}

ry_err_t ryzen_smu_get_stapm_value(ry_handle_t* handle, float* data)
{
	return ryzen_smu_get_pm_table_float(handle, 0x04, data);
}

// PL2
ry_err_t ryzen_smu_get_fast_limit(ry_handle_t* handle, float* data)
{
	return ryzen_smu_get_pm_table_float(handle, 0x08, data);
}

ry_err_t ryzen_smu_get_fast_value(ry_handle_t* handle, float* data)
{
	return ryzen_smu_get_pm_table_float(handle, 0x0C, data);
}

// PL1
ry_err_t ryzen_smu_get_slow_limit(ry_handle_t* handle, float* data)
{
	return ryzen_smu_get_pm_table_float(handle, 0x10, data);
}

ry_err_t ryzen_smu_get_slow_value(ry_handle_t* handle, float* data)
{
	return ryzen_smu_get_pm_table_float(handle, 0x14, data);
}

#define SMU_MAX_CORE 16

ry_err_t ryzen_smu_get_core_temperature(ry_handle_t* handle, uint32_t core, float* data)
{
	size_t base_offset = OFFSET_INVALID;
	uint32_t max_cores = SMU_MAX_CORE;

	switch (handle->pm_table_version)
	{
	case 0x240803:
		base_offset = 0x2CC;
		break;
	case 0x240903:
		base_offset = 0x28C;
		max_cores = 8;
		break;
	case 0x370000:
	case 0x370001:
	case 0x370002:
	case 0x370003:
	case 0x370004:
		base_offset = 0x340;
		break;
	case 0x370005:
		base_offset = 0x35C;
		break;
	case 0x380804:
		base_offset = 0x324;
		break;
	case 0x380805:
		base_offset = 0x330;
		break;
	case 0x380904:
		base_offset = 0x2E4;
		max_cores = 8;
		break;
	case 0x380905:
		base_offset = 0x2F0;
		max_cores = 8;
		break;
	case 0x3F0000:
		base_offset = 0x258;
		max_cores = 4;
		break;
	case 0x400004:
	case 0x400005:
		base_offset = 0x360;
		break;
	case 0x5D0008:
		base_offset = 0xA38;
		break;
	case 0x64020C:
		base_offset = 0xC10;
		break;
	}

	if (base_offset == OFFSET_INVALID || core >= max_cores)
		return RYZEN_SMU_UNSUPPORTED;

	return ryzen_smu_get_pm_table_float(handle, base_offset + (core * sizeof(float)), data);
}

ry_err_t ryzen_smu_get_apu_temperature(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;

	switch (handle->pm_table_version)
	{
	case 0x370000:
	case 0x370001:
	case 0x370002:
	case 0x370003:
	case 0x370004:
	case 0x370005:
	case 0x3F0000:
	case 0x400001:
	case 0x400002:
	case 0x400003:
	case 0x400004:
	case 0x400005:
	case 0x450004:
	case 0x450005:
	case 0x4C0006:
	case 0x4C0007:
	case 0x4C0008:
	case 0x4C0009:
	case 0x5D0008:
	case 0x64020c:
		offset = 0x5C;
		break;
	}

	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;

	return ryzen_smu_get_pm_table_float(handle, offset, data);
}
