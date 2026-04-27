// SPDX-License-Identifier: Unlicense

#include "ryzen_smu.h"
#include "libcpuid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <intrin.h>
#include "ioctl.h"
#include "libnw.h"
#include "pci_ids.h"

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
		NWL_Debug("SMU", __VA_ARGS__); \
	} while (0)

static const char* get_codename_str(ry_codename_t codename)
{
	switch (codename)
	{
	case CODENAME_SUMMITRIDGE: return "Summit Ridge";
	case CODENAME_THREADRIPPER: return "ThreadRipper";
	case CODENAME_NAPLES: return "Naples";
	case CODENAME_RAVENRIDGE: return "Raven Ridge";
	case CODENAME_RAVENRIDGE2: return "Raven Ridge 2";
	case CODENAME_PINNACLERIDGE: return "Pinnacle Ridge";
	case CODENAME_COLFAX: return "Colfax";
	case CODENAME_PICASSO: return "Picasso";
	case CODENAME_FIREFLIGHT: return "Fire Flight";
	case CODENAME_MATISSE: return "Matisse";
	case CODENAME_CASTLEPEAK: return "Castle Peak";
	case CODENAME_ROME: return "Rome";
	case CODENAME_DALI: return "Dali";
	case CODENAME_RENOIR: return "Renoir";
	case CODENAME_VANGOGH: return "VanGogh";
	case CODENAME_VERMEER: return "Vermeer";
	case CODENAME_CHAGALL: return "Chagall";
	case CODENAME_MILAN: return "Milan";
	case CODENAME_CEZANNE: return "Cezanne";
	case CODENAME_REMBRANDT: return "Rembrandt";
	case CODENAME_LUCIENNE: return "Lucienne";
	case CODENAME_RAPHAEL: return "Raphael";
	case CODENAME_PHOENIX: return "Phoenix";
	case CODENAME_PHOENIX2: return "Phoenix 2";
	case CODENAME_MENDOCINO: return "Mendocino";
	case CODENAME_GENOA: return "Genoa";
	case CODENAME_STORMPEAK: return "Storm Peak";
	case CODENAME_DRAGONRANGE: return "Dragon Range";
	case CODENAME_MERO: return "Mero";
	case CODENAME_HAWKPOINT: return "Hawk Point";
	case CODENAME_STRIXPOINT: return "Strix Point";
	case CODENAME_GRANITERIDGE: return "Granite Ridge";
	case CODENAME_KRACKANPOINT: return "Krackan Point";
	case CODENAME_KRACKANPOINT2: return "Krackan Point 2";
	case CODENAME_STRIXHALO: return "Strix Halo";
	case CODENAME_TURIN: return "Turin";
	case CODENAME_TURIND: return "Turin D";
	case CODENAME_SHIMADAPEAK: return "Shimada Peak";
	case CODENAME_BERGAMO: return "Bergamo";
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

ry_codename_t ryzen_smu_get_codename(struct cpu_id_t* id)
{
	ry_codename_t codename = CODENAME_UNKNOWN;

	if (id->vendor != VENDOR_AMD)
	{
		SMU_DEBUG("Not an AMD CPU");
		return codename;
	}

	switch (id->x86.ext_family)
	{
	case 0x17: // Zen, Zen+, Zen2
		switch (id->x86.ext_model)
		{
		case 0x01:
			if (id->x86.pkg_type == 4)
				codename = CODENAME_NAPLES;
			else if (id->x86.pkg_type == 7)
				codename = CODENAME_THREADRIPPER;
			else
				codename = CODENAME_SUMMITRIDGE;
			break;
		case 0x08:
			if (id->x86.pkg_type == 4 || id->x86.pkg_type == 7)
				codename = CODENAME_COLFAX;
			else
				codename = CODENAME_PINNACLERIDGE;
			break;
		case 0x11:
			codename = CODENAME_RAVENRIDGE;
			break;
		case 0x18:
			if (id->x86.pkg_type == 7)
				codename = CODENAME_RAVENRIDGE2;
			else
				codename = CODENAME_PICASSO;
			break;
		case 0x20:
			codename = CODENAME_DALI;
			break;
		case 0x31:
			if (id->x86.pkg_type == 7)
				codename = CODENAME_CASTLEPEAK;
			else
				codename = CODENAME_ROME;
			break;
		case 0x50:
			codename = CODENAME_FIREFLIGHT;
			break;
		case 0x60:
			codename = CODENAME_RENOIR;
			break;
		case 0x68:
			codename = CODENAME_LUCIENNE;
			break;
		case 0x71:
			codename = CODENAME_MATISSE;
			break;
		case 0x90:
		case 0x91:
			codename = CODENAME_VANGOGH;
			break;
		case 0x98:
			codename = CODENAME_MERO;
			break;
		case 0xA0:
			codename = CODENAME_MENDOCINO;
			break;
		}
		break;
	case 0x19: // Zen3, Zen3+, Zen4
		switch (id->x86.ext_model)
		{
		case 0x00:
		case 0x01:
			codename = CODENAME_MILAN;
			break;
		case 0x08:
			codename = CODENAME_CHAGALL;
			break;
		case 0x11:
			codename = CODENAME_GENOA;
			break;
		case 0x18:
			codename = CODENAME_STORMPEAK;
			break;
		case 0x20:
		case 0x21:
			codename = CODENAME_VERMEER;
			break;
		case 0x40:
		case 0x44:
			codename = CODENAME_REMBRANDT;
			break;
		case 0x50:
			codename = CODENAME_CEZANNE;
			break;
		case 0x61:
			if (id->x86.pkg_type == 1)
				codename = CODENAME_DRAGONRANGE;
			else
				codename = CODENAME_RAPHAEL;
			break;
		case 0x74:
		case 0x75:
			codename = CODENAME_PHOENIX;
			break;
		case 0x78:
			codename = CODENAME_PHOENIX2;
			break;
		case 0x7C:
			codename = CODENAME_HAWKPOINT;
			break;
		}
		break;
	case 0x1A: // Zen5
		switch (id->x86.ext_model)
		{
		case 0x02:
			codename = CODENAME_TURIN;
			break;
		case 0x08:
			codename = CODENAME_SHIMADAPEAK;
			break;
		case 0x11:
			codename = CODENAME_TURIND;
			break;
		case 0x20:
		case 0x24:
			codename = CODENAME_STRIXPOINT;
			break;
		case 0x44:
			codename = CODENAME_GRANITERIDGE;
			break;
		case 0x60:
			codename = CODENAME_KRACKANPOINT;
			break;
		case 0x68:
			codename = CODENAME_KRACKANPOINT2;
			break;
		case 0x70:
			codename = CODENAME_STRIXHALO;
			break;
		case 0xA0:
			codename = CODENAME_BERGAMO;
			break;
		}
		break;
	}
	if (codename == CODENAME_UNKNOWN)
		SMU_DEBUG("Unsupported CPU: Family %Xh, Model %Xh, Package %Xh",
			id->x86.ext_family, id->x86.ext_model, id->x86.pkg_type);
	else
		SMU_DEBUG("Detected CPU: %s (Family %Xh, Model %Xh, Package %Xh)",
			get_codename_str(codename), id->x86.ext_family, id->x86.ext_model, id->x86.pkg_type);
	return codename;
}

static ry_err_t get_rsmu_addr(ry_handle_t* handle)
{
	switch (handle->codename)
	{
	case CODENAME_MATISSE:
	case CODENAME_CASTLEPEAK:
	case CODENAME_VERMEER:
	case CODENAME_MILAN:
	case CODENAME_RAPHAEL:
	case CODENAME_GRANITERIDGE:
	case CODENAME_ROME:
	case CODENAME_CHAGALL:
	case CODENAME_GENOA:
	case CODENAME_STORMPEAK:
	case CODENAME_DRAGONRANGE:
	case CODENAME_TURIN:
	case CODENAME_TURIND:
	case CODENAME_BERGAMO:
		handle->rsmu_cmd_addr = 0x3B10524;
		handle->rsmu_rsp_addr = 0x3B10570;
		handle->rsmu_arg_addr = 0x3B10A40;
		break;
	case CODENAME_COLFAX:
	case CODENAME_THREADRIPPER:
	case CODENAME_SUMMITRIDGE:
	case CODENAME_PINNACLERIDGE:
	case CODENAME_NAPLES:
		handle->rsmu_cmd_addr = 0x3B1051C;
		handle->rsmu_rsp_addr = 0x3B10568;
		handle->rsmu_arg_addr = 0x3B10590;
		break;
	case CODENAME_RENOIR:
	case CODENAME_PICASSO:
	case CODENAME_RAVENRIDGE:
	case CODENAME_RAVENRIDGE2:
	case CODENAME_REMBRANDT:
	case CODENAME_VANGOGH:
	case CODENAME_CEZANNE:
	case CODENAME_DALI:
	case CODENAME_FIREFLIGHT:
	case CODENAME_LUCIENNE:
	case CODENAME_PHOENIX:
	case CODENAME_PHOENIX2:
	case CODENAME_MENDOCINO:
	case CODENAME_MERO:
	case CODENAME_HAWKPOINT:
	case CODENAME_STRIXPOINT:
	case CODENAME_STRIXHALO:
	case CODENAME_KRACKANPOINT:
	case CODENAME_KRACKANPOINT2:
		handle->rsmu_cmd_addr = 0x3B10A20;
		handle->rsmu_rsp_addr = 0x3B10A80;
		handle->rsmu_arg_addr = 0x3B10A88;
		break;
	case CODENAME_SHIMADAPEAK:
		handle->rsmu_cmd_addr = 0x3B10924;
		handle->rsmu_rsp_addr = 0x3B10970;
		handle->rsmu_arg_addr = 0x3B10A40;
		break;
	default:
		return RYZEN_SMU_CPU_NOT_SUPPORTED;
	}
	SMU_DEBUG("RSMU(%X,%X,%X)", handle->rsmu_cmd_addr, handle->rsmu_rsp_addr, handle->rsmu_arg_addr);
	return RYZEN_SMU_OK;
}

static ry_err_t send_command(ry_handle_t* handle, uint32_t fn, ry_args_t* args)
{
	if (handle->rsmu_cmd_addr == 0)
		return RYZEN_SMU_UNSUPPORTED;

	SMU_DEBUG("Send RSMU Fn %Xh", fn);

	if (WR0_SendSmuCmd(handle->drv_handle,
		handle->rsmu_cmd_addr, handle->rsmu_rsp_addr, handle->rsmu_arg_addr, fn, args->args) != 0)
		return RYZEN_SMU_DRIVER_ERROR;

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
	ry_err_t rc = send_command(handle, 0x02, &args);
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

	switch (handle->codename)
	{
	case CODENAME_RAPHAEL:
	case CODENAME_GENOA:
	case CODENAME_STORMPEAK:
	case CODENAME_DRAGONRANGE:
	case CODENAME_GRANITERIDGE:
	case CODENAME_BERGAMO:
	case CODENAME_TURIN:
	case CODENAME_TURIND:
	case CODENAME_SHIMADAPEAK:
		fn[0] = 0x04;
		goto base_addr_class_1;
	case CODENAME_MATISSE:
	case CODENAME_VERMEER:
	case CODENAME_CASTLEPEAK:
	case CODENAME_ROME:
	case CODENAME_CHAGALL:
	case CODENAME_MILAN:
		fn[0] = 0x06;
		goto base_addr_class_1;
	case CODENAME_RENOIR:
	case CODENAME_REMBRANDT:
	case CODENAME_CEZANNE:
	case CODENAME_MERO:
	case CODENAME_VANGOGH:
	case CODENAME_PHOENIX:
	case CODENAME_PHOENIX2:
	case CODENAME_HAWKPOINT:
	case CODENAME_MENDOCINO:
	case CODENAME_STRIXHALO:
	case CODENAME_STRIXPOINT:
	case CODENAME_KRACKANPOINT:
	case CODENAME_KRACKANPOINT2:
	case CODENAME_LUCIENNE:
		fn[0] = 0x66;
		goto base_addr_class_1;
	case CODENAME_COLFAX:
	case CODENAME_PINNACLERIDGE:
	case CODENAME_SUMMITRIDGE:
	case CODENAME_NAPLES:
	case CODENAME_THREADRIPPER:
		fn[0] = 0x0b;
		fn[1] = 0x0c;
		goto base_addr_class_2;
	case CODENAME_DALI:
	case CODENAME_PICASSO:
	case CODENAME_RAVENRIDGE:
	case CODENAME_RAVENRIDGE2:
	case CODENAME_FIREFLIGHT:
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
	rc = send_command(handle, fn[0], &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	*addr = ((uint64_t)args.u32.arg1 << 32) | args.u32.arg0;
	SMU_DEBUG("PM Table Base: %llX", (unsigned long long)addr);
	return RYZEN_SMU_OK;

base_addr_class_2:
	init_smu_args(&args, 0);
	rc = send_command(handle, fn[0], &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	init_smu_args(&args, 0);
	rc = send_command(handle, fn[1], &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	*addr = (uint64_t)args.u32.arg0;
	SMU_DEBUG("PM Table Base: %llX", (unsigned long long)addr);
	return RYZEN_SMU_OK;

base_addr_class_3:
	init_smu_args(&args, 3);
	rc = send_command(handle, fn[0], &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	init_smu_args(&args, 3);
	rc = send_command(handle, fn[2], &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	*addr = (uint64_t)args.u32.arg0;
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
	case CODENAME_SUMMITRIDGE:
	case CODENAME_NAPLES:
	case CODENAME_PINNACLERIDGE:
	case CODENAME_COLFAX:
	case CODENAME_THREADRIPPER:
		fn = 0x0d;
		break;
	case CODENAME_DALI:
	case CODENAME_PICASSO:
	case CODENAME_RAVENRIDGE:
	case CODENAME_RAVENRIDGE2:
	case CODENAME_FIREFLIGHT:
		fn = 0x0C;
		break;
	case CODENAME_MATISSE:
	case CODENAME_VERMEER:
	case CODENAME_CASTLEPEAK:
	case CODENAME_ROME:
	case CODENAME_CHAGALL:
	case CODENAME_MILAN:
		fn = 0x08;
		break;
	case CODENAME_RENOIR:
	case CODENAME_REMBRANDT:
	case CODENAME_CEZANNE:
	case CODENAME_MERO:
	case CODENAME_VANGOGH:
	case CODENAME_PHOENIX:
	case CODENAME_PHOENIX2:
	case CODENAME_HAWKPOINT:
	case CODENAME_MENDOCINO:
	case CODENAME_STRIXHALO:
	case CODENAME_STRIXPOINT:
	case CODENAME_KRACKANPOINT:
	case CODENAME_KRACKANPOINT2:
	case CODENAME_LUCIENNE:
		fn = 0x06;
		break;
	case CODENAME_RAPHAEL:
	case CODENAME_GENOA:
	case CODENAME_STORMPEAK:
	case CODENAME_DRAGONRANGE:
	case CODENAME_GRANITERIDGE:
	case CODENAME_BERGAMO:
	case CODENAME_TURIN:
	case CODENAME_TURIND:
	case CODENAME_SHIMADAPEAK:
		fn = 0x05;
		break;
	
	default:
		return RYZEN_SMU_UNSUPPORTED;
	}

	init_smu_args(&args, 0);
	rc = send_command(handle, fn, &args);
	if (rc != RYZEN_SMU_OK)
		return rc;
	*version = args.u32.arg0;
	SMU_DEBUG("PM Table Version: %X", args.u32.arg0);
	return RYZEN_SMU_OK;
}

static size_t get_pm_table_size_from_version(ry_handle_t* handle, uint32_t version)
{
#if 0
	switch (version)
	{
	case 0x190001: return 0x280;
	case 0x190101: return 0x45C;
	case 0x190201: return 0x814;
	case 0x1E0001: return 0x570;
	case 0x1E0002: return 0x580;
	case 0x1E0003:
	case 0x1E0004:
	case 0x1E0005:
	case 0x1E000A:
	case 0x1E0101: return 0x610;
	//case 0x240003: return 0x18AC; // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x240503: return 0xD7C;  // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x240603: return 0xAB0;  // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x240703: return 0x7E4;  // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x240802: return 0x7E0;  // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x240803: return 0x7E4;  // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x240902: return 0x514;  // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x240903: return 0x518;  // CODENAME_CASTLEPEAK, CODENAME_MATISSE
	case 0x260001: return 0x610;  // CODENAME_FIREFLIGHT
	//case 0x2D0008: return 0x1AB0; // CODENAME_MILAN
	case 0x2D0803: return 0x894;  // CODENAME_VERMEER, CODENAME_CHAGALL
	case 0x2D0903: return 0x7E4;  // CODENAME_VERMEER, CODENAME_CHAGALL
	case 0x370000: return 0x79C;  // CODENAME_RENOIR, CODENAME_LUCIENNE
	case 0x370001: return 0x88C;  // CODENAME_RENOIR, CODENAME_LUCIENNE
	case 0x370002: return 0x894;  // CODENAME_RENOIR, CODENAME_LUCIENNE
	case 0x370003:
	case 0x370004: return 0x8B4;  // CODENAME_RENOIR, CODENAME_LUCIENNE
	case 0x370005: return 0x8D0;  // CODENAME_RENOIR, CODENAME_LUCIENNE
	//case 0x380005: return 0x1BB0; // CODENAME_VERMEER, CODENAME_CHAGALL
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
	//case 0x470004: return 0x1E48;
	//case 0x470104: return 0x1A20;
	//case 0x470204: return 0x1554;
	case 0x470304: return 0xDA8;
	case 0x470404: return 0x980;
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
	//case 0x5C0002: return 0x1E3C; // CODENAME_STORMPEAK
	//case 0x5C0003: return 0x1E48; // CODENAME_STORMPEAK
	//case 0x5C0102: return 0x1A14; // CODENAME_STORMPEAK
	//case 0x5C0103: return 0x1A20; // CODENAME_STORMPEAK
	//case 0x5C0202: return 0x15EC; // CODENAME_STORMPEAK
	//case 0x5C0203: return 0x15F8; // CODENAME_STORMPEAK
	case 0x5C0302: return 0xD9C;  // CODENAME_STORMPEAK
	case 0x5C0303: return 0xDA8;  // CODENAME_STORMPEAK
	case 0x5C0402: return 0x974;  // CODENAME_STORMPEAK
	case 0x5C0403: return 0x980;  // CODENAME_STORMPEAK
	case 0x5D0008: return 0xD54;  // CODENAME_STRIXPOINT
	case 0x5D0009: return 0xD58;  // CODENAME_STRIXPOINT
	case 0x5D000A: return 0xD60;
	case 0x5D000B: return 0xD54;
	//case 0x5E0801: return 0x16EC;
	//case 0x5E0802: return 0x289C;
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
	//case 0x640207: return 0x100C; // CODENAME_STRIXHALO
	//case 0x640208: return 0x1010; // CODENAME_STRIXHALO
	case 0x640209:
	//case 0x64020A: return 0x1020; // CODENAME_STRIXHALO
	//case 0x64020C: return 0x1028; // CODENAME_STRIXHALO
	case 0x650004: return 0xB74;  // CODENAME_KRACKANPOINT
	case 0x650005: return 0xB78;  // CODENAME_KRACKANPOINT
	case 0x650006: return 0xB80;
	case 0x650007: return 0xD54;
	//case 0x730A04: return 0x1EBC;
	case 0x730204: return 0xB1C;
	//case 0x730404: return 0x1004;
	//case 0x730604: return 0x14EC;
	//case 0x730804: return 0x19D4;
	//case 0x730C04: return 0x23A4;
	//case 0x730E04: return 0x288C;
	//case 0x731004: return 0x2D74;
	}
#endif
	return SMU_TABLE_MAX_SIZE;
}

static ry_err_t transfer_table_to_dram(ry_handle_t* handle)
{
	ry_args_t args;
	uint32_t fn;

	init_smu_args(&args, 0);
	switch (handle->codename)
	{
	case CODENAME_SUMMITRIDGE:
	case CODENAME_NAPLES:
	case CODENAME_PINNACLERIDGE:
	case CODENAME_COLFAX:
	case CODENAME_THREADRIPPER:
		fn = 0x0a;
		break;
	case CODENAME_RAPHAEL:
	case CODENAME_GENOA:
	case CODENAME_STORMPEAK:
	case CODENAME_DRAGONRANGE:
	case CODENAME_GRANITERIDGE:
	case CODENAME_BERGAMO:
	case CODENAME_TURIN:
	case CODENAME_TURIND:
	case CODENAME_SHIMADAPEAK:
		fn = 0x03;
		break;
	case CODENAME_MATISSE:
	case CODENAME_VERMEER:
	case CODENAME_CASTLEPEAK:
	case CODENAME_ROME:
	case CODENAME_CHAGALL:
	case CODENAME_MILAN:
		fn = 0x05;
		break;
	case CODENAME_RENOIR:
	case CODENAME_REMBRANDT:
	case CODENAME_CEZANNE:
	case CODENAME_MERO:
	case CODENAME_VANGOGH:
	case CODENAME_PHOENIX:
	case CODENAME_PHOENIX2:
	case CODENAME_HAWKPOINT:
	case CODENAME_MENDOCINO:
	case CODENAME_STRIXHALO:
	case CODENAME_STRIXPOINT:
	case CODENAME_KRACKANPOINT:
	case CODENAME_KRACKANPOINT2:
	case CODENAME_LUCIENNE:
		args.u32.arg0 = 3;
		fn = 0x65;
		break;
	case CODENAME_DALI:
	case CODENAME_PICASSO:
	case CODENAME_RAVENRIDGE:
	case CODENAME_RAVENRIDGE2:
	case CODENAME_FIREFLIGHT:
		args.u32.arg0 = 3;
		fn = 0x3d;
		break;
	default:
		return RYZEN_SMU_UNSUPPORTED;
	}
	return send_command(handle, fn, &args);
}

ry_handle_t* ryzen_smu_init(struct wr0_drv_t* drv_handle, struct cpu_id_t* id)
{
	if (!drv_handle)
		return NULL;

	ry_handle_t* handle = (ry_handle_t*)calloc(1, sizeof(ry_handle_t));
	if (!handle)
		return NULL;

	handle->drv_handle = drv_handle;

	if (drv_handle->type == WR0_DRIVER_PAWNIO)
	{
		ULONG64 out[2] = { 0 };
		if (WR0_ExecPawn(drv_handle, &drv_handle->pio_rysmu, "ioctl_get_smu_version", NULL, 0, out, 1, NULL))
			goto fail;
		handle->smu_version = (uint32_t)out[0];
		SMU_DEBUG("SMU Version %Xh", handle->smu_version);
		if (WR0_ExecPawn(drv_handle, &drv_handle->pio_rysmu, "ioctl_resolve_pm_table", NULL, 0, out, 2, NULL))
			goto fail;
		if (out[0] == 0 || out[1] == 0)
			goto fail;
		handle->pm_table_version = (uint32_t)out[0];
		handle->pm_table_base_addr = (uint64_t)out[1];
		SMU_DEBUG("PM Version %X", handle->pm_table_version);
		SMU_DEBUG("PM Table Base: %llX", (unsigned long long)handle->pm_table_base_addr);

		handle->pm_table_size = get_pm_table_size_from_version(handle, handle->pm_table_version);
		SMU_DEBUG("PM Table Size: %zu", handle->pm_table_size);

		ZeroMemory(&handle->pm_table_buffer, sizeof(handle->pm_table_buffer));
		return handle;
	}

	handle->codename = ryzen_smu_get_codename(id);
	if (handle->codename == CODENAME_UNKNOWN)
		goto fail;

	handle->pci_id = WR0_RdPciConf32(handle->drv_handle, 0, 0);
	if ((handle->pci_id & 0xFFFF) != PCI_VID_AMD)
		goto fail;

	if (get_rsmu_addr(handle) != RYZEN_SMU_OK)
		goto fail;

	if (get_smu_version(handle) != RYZEN_SMU_OK)
		goto fail;

	if (handle->rsmu_cmd_addr == 0)
		goto fail;

	uint64_t base_addr = 0;
	if (get_pm_table_base(handle, &base_addr) != RYZEN_SMU_OK)
		goto fail;
	handle->pm_table_base_addr = base_addr;

	ry_err_t rc = get_pm_table_version(handle, &handle->pm_table_version);
	if (rc != RYZEN_SMU_OK && rc != RYZEN_SMU_UNSUPPORTED)
		goto fail;

	handle->pm_table_size = get_pm_table_size_from_version(handle, handle->pm_table_version);
	SMU_DEBUG("PM Table Size: %zu", handle->pm_table_size);

	ZeroMemory(&handle->pm_table_buffer, sizeof(handle->pm_table_buffer));

	return handle;
fail:
	ryzen_smu_free(handle);
	return NULL;
}

void ryzen_smu_free(ry_handle_t* handle)
{
	if (!handle)
		return;
	free(handle);
}

ry_err_t ryzen_smu_update_pm_table(ry_handle_t* handle)
{
	if (!handle->pm_table_base_addr)
		return RYZEN_SMU_UNSUPPORTED;

	if (handle->drv_handle->type == WR0_DRIVER_PAWNIO)
	{
		if (WR0_ExecPawn(handle->drv_handle, &handle->drv_handle->pio_rysmu, "ioctl_update_pm_table", NULL, 0, NULL, 0, NULL))
			return RYZEN_SMU_DRIVER_ERROR;
		if (WR0_ExecPawn(handle->drv_handle, &handle->drv_handle->pio_rysmu, "ioctl_read_pm_table",
			NULL, 0, handle->pm_table_buffer_u64, SMU_TABLE_MAX_SIZE / 8, NULL))
			return RYZEN_SMU_DRIVER_ERROR;
		return RYZEN_SMU_OK;
	}

	ry_err_t rc = transfer_table_to_dram(handle);
	if (rc != RYZEN_SMU_OK)
		return rc;

	return RYZEN_SMU_OK;
}

#define OFFSET_INVALID ((size_t)0xFFFFFFFF)

ry_err_t ryzen_smu_get_pm_table_float(ry_handle_t* handle, size_t offset, float* value)
{
	if (!handle)
		return RYZEN_SMU_NOT_INITIALIZED;
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	if (offset + sizeof(float) > handle->pm_table_size)
		return RYZEN_SMU_INVALID_ARGUMENT;
	if (handle->drv_handle->type == WR0_DRIVER_PAWNIO)
		memcpy(value, handle->pm_table_buffer + offset, sizeof(float));
	else
		WR0_RdMmIo(handle->drv_handle, handle->pm_table_base_addr + offset, value, sizeof(float));
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

ry_err_t ryzen_smu_get_fast_limit(ry_handle_t* handle, float* data)
{
	return ryzen_smu_get_pm_table_float(handle, 0x08, data);
}

ry_err_t ryzen_smu_get_fast_value(ry_handle_t* handle, float* data)
{
	return ryzen_smu_get_pm_table_float(handle, 0x0C, data);
}

ry_err_t ryzen_smu_get_slow_limit(ry_handle_t* handle, float* data)
{
	return ryzen_smu_get_pm_table_float(handle, 0x10, data);
}

ry_err_t ryzen_smu_get_slow_value(ry_handle_t* handle, float* data)
{
	return ryzen_smu_get_pm_table_float(handle, 0x14, data);
}

ry_err_t ryzen_smu_get_apu_slow_limit(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x003F0000:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000B:
	case 0x0064020C:
	case 0x00650005:
	case 0x00650006:
	case 0x00650007:
		offset = 0x18;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_apu_slow_value(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x003F0000:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
	case 0x004C0009:
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000B:
	case 0x0064020C:
	case 0x00650005:
	case 0x00650006:
	case 0x00650007:
		offset = 0x1C;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_vrm_current(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		offset = 0x18;
		break;
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		offset = 0x20;
		break;
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650006:
	case 0x00650007:
		offset = 0x30;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_vrm_current_value(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		offset = 0x1C;
		break;
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		offset = 0x24;
		break;
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650006:
	case 0x00650007:
		offset = 0x34;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_vrmsoc_current(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		offset = 0x20;
		break;
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		offset = 0x28;
		break;
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650006:
	case 0x00650007:
		offset = 0x38;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_vrmsoc_current_value(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		offset = 0x24;
		break;
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		offset = 0x2C;
		break;
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000B:
	case 0x00650005:
	case 0x00650006:
	case 0x00650007:
		offset = 0x3C;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

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
	case 0x4C0006:
		base_offset = 0x840;
		max_cores = 8;
		break;
	case 0x5D0008:
	case 0x5D0009:
	case 0x5D000B:
		base_offset = 0xA38;
		break;
	case 0x64020C:
		base_offset = 0xC10;
		break;
	case 0x620205:
		base_offset = 0x534;
		break;
	}

	if (base_offset == OFFSET_INVALID || core >= max_cores)
		return RYZEN_SMU_UNSUPPORTED;

	return ryzen_smu_get_pm_table_float(handle, base_offset + (core * sizeof(float)), data);
}

ry_err_t ryzen_smu_get_core_volt(ry_handle_t* handle, uint32_t core, float* data)
{
	size_t base_offset = OFFSET_INVALID;
	uint32_t max_cores = SMU_MAX_CORE;

	switch (handle->pm_table_version)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		base_offset = 0x320;
		break;
	case 0x00370005:
		base_offset = 0x33C;
		break;
	case 0x003F0000:
		base_offset = 0x248;
		max_cores = 4;
		break;
	case 0x00400004:
	case 0x00400005:
		base_offset = 0x340;
		break;
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000B:
		base_offset = 0xA08;
		break;
	case 0x0064020C:
		base_offset = 0xBD0;
		break;
	}

	if (base_offset == OFFSET_INVALID || core >= max_cores)
		return RYZEN_SMU_UNSUPPORTED;

	return ryzen_smu_get_pm_table_float(handle, base_offset + (core * sizeof(float)), data);
}

ry_err_t ryzen_smu_get_core_clk(ry_handle_t* handle, uint32_t core, float* data)
{
	size_t base_offset = OFFSET_INVALID;
	uint32_t max_cores = SMU_MAX_CORE;

	switch (handle->pm_table_version)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		base_offset = 0x3A0;
		break;
	case 0x00370005:
		base_offset = 0x3BC;
		break;
	case 0x003F0000:
		base_offset = 0x288;
		max_cores = 4;
		break;
	case 0x00400004:
	case 0x00400005:
		base_offset = 0x3C0;
		break;
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000B:
		base_offset = 0xA68;
		break;
	case 0x0064020C:
		base_offset = 0xC50;
		break;
	}

	if (base_offset == OFFSET_INVALID || core >= max_cores)
		return RYZEN_SMU_UNSUPPORTED;

	return ryzen_smu_get_pm_table_float(handle, base_offset + (core * sizeof(float)), data);
}

ry_err_t ryzen_smu_get_core_power(ry_handle_t* handle, uint32_t core, float* data)
{
	size_t base_offset = OFFSET_INVALID;
	uint32_t max_cores = SMU_MAX_CORE;

	switch (handle->pm_table_version)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		base_offset = 0x300;
		break;
	case 0x00370005:
		base_offset = 0x31C;
		break;
	case 0x003F0000:
		base_offset = 0x238;
		max_cores = 4;
		break;
	case 0x00400001:
		base_offset = 0x304;
		break;
	case 0x00400004:
	case 0x00400005:
		base_offset = 0x320;
		break;
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000B:
		base_offset = 0x9D8;
		break;
	case 0x0064020C:
		base_offset = 0xB90;
		break;
	}

	if (base_offset == OFFSET_INVALID || core >= max_cores)
		return RYZEN_SMU_UNSUPPORTED;

	return ryzen_smu_get_pm_table_float(handle, base_offset + (core * sizeof(float)), data);
}

ry_err_t ryzen_smu_get_gfx_temperature(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;

	switch (handle->pm_table_version)
	{
	case 0x370000:
	case 0x370001:
	case 0x370002:
	case 0x370003:
	case 0x370004:
		offset = 0x5AC;
		break;
	case 0x370005:
		offset = 0x5C8;
		break;
	case 0x400001:
		offset = 0x604;
		break;
	case 0x400002:
		offset = 0x61C;
		break;
	case 0x400003:
		offset = 0x63C;
		break;
	case 0x400004:
	case 0x400005:
		offset = 0x640;
		break;
	case 0x3F0000:
		offset = 0x380;
		break;
	case 0x4C0006:
		offset = 0x358;
		break;
	case 0x5D0008:
	case 0x5D0009:
	case 0x5D000B:
		offset = 0x4C8;
		break;
	case 0x64020C:
		offset = 0x550;
		break;
	}

	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;

	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_gfx_volt(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;

	switch (handle->pm_table_version)
	{
	case 0x370000:
	case 0x370001:
	case 0x370002:
	case 0x370003:
	case 0x370004:
		offset = 0x5A8;
		break;
	case 0x370005:
		offset = 0x5C4;
		break;
	case 0x400001:
		offset = 0x600;
		break;
	case 0x400002:
		offset = 0x618;
		break;
	case 0x400003:
		offset = 0x638;
		break;
	case 0x400004:
	case 0x400005:
		offset = 0x63C;
		break;
	case 0x3F0000:
		offset = 0x37C;
		break;
	case 0x5D0008:
	case 0x5D0009:
	case 0x5D000B:
		offset = 0x4B8;
		break;
	case 0x64020C:
		offset = 0x54C;
		break;
	}

	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;

	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_gfx_clk(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;

	switch (handle->pm_table_version)
	{
	case 0x370000:
	case 0x370001:
	case 0x370002:
	case 0x370003:
	case 0x370004:
		offset = 0x5B4;
		break;
	case 0x370005:
		offset = 0x5D0;
		break;
	case 0x400001:
		offset = 0x60C;
		break;
	case 0x400002:
		offset = 0x624;
		break;
	case 0x400003:
		offset = 0x644;
		break;
	case 0x400004:
	case 0x400005:
		offset = 0x648;
		break;
	case 0x3F0000:
		offset = 0x388;
		break;
	case 0x5D0008:
	case 0x5D0009:
	case 0x5D000B:
		offset = 0x4C0;
		break;
	case 0x64020C:
		offset = 0x588;
		break;
	}

	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;

	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_fclk(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x001E0001:
		offset = 0x460;
		break;
	case 0x001E0002:
		offset = 0x474;
		break;
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		offset = 0x298;
		break;
	case 0x00240003:
		offset = 0xB0;
		break;
	case 0x00240503:
	case 0x00240603:
	case 0x00240703:
		offset = 0xC0;
		break;
	case 0x00240802:
		offset = 0xBC;
		break;
	case 0x00240803:
		offset = 0xC0;
		break;
	case 0x00240902:
		offset = 0xBC;
		break;
	case 0x00240903:
		offset = 0xC0;
		break;
	case 0x00260001:
		offset = 0x28;
		break;
	case 0x002D0008:
	case 0x002D0803:
	case 0x002D0903:
		offset = 0xBC;
		break;
	case 0x00370000:
		offset = 0x4B4;
		break;
	case 0x00370001:
		offset = 0x5A4;
		break;
	case 0x00370002:
		offset = 0x5AC;
		break;
	case 0x00370003:
	case 0x00370004:
		offset = 0x5CC;
		break;
	case 0x00370005:
		offset = 0x5E8;
		break;
	case 0x00380005:
	case 0x00380505:
	case 0x00380605:
	case 0x00380705:
	case 0x00380804:
	case 0x00380805:
	case 0x00380904:
	case 0x00380905:
		offset = 0xC0;
		break;
	case 0x003F0000:
		offset = 0x3C5;
		break;
	case 0x00400001:
		offset = 0x624;
		break;
	case 0x00400002:
		offset = 0x63C;
		break;
	case 0x00400003:
		offset = 0x660;
		break;
	case 0x00400004:
	case 0x00400005:
		offset = 0x664;
		break;
	case 0x00450004:
		offset = 0x664;
		break;
	case 0x00450005:
		offset = 0x6B0;
		break;
	case 0x004C0003:
	case 0x004C0004:
	case 0x004C0005:
	case 0x004C0006:
	case 0x004C0007:
		offset = 0x174;
		break;
	case 0x004C0008:
	case 0x004C0009:
		offset = 0x164;
		break;
	case 0x00540000:
	case 0x00540001:
	case 0x00540002:
	case 0x00540003:
	case 0x00540004:
	case 0x00540005:
	case 0x00540100:
	case 0x00540101:
	case 0x00540102:
	case 0x00540103:
	case 0x00540104:
	case 0x00540105:
	case 0x00540108:
		offset = 0x118;
		break;
	case 0x00540208:
		offset = 0x11C;
		break;
	case 0x005C0002:
	case 0x005C0003:
	case 0x005C0102:
	case 0x005C0103:
	case 0x005C0202:
	case 0x005C0203:
	case 0x005C0302:
		offset = 0x194;
		break;
	case 0x005C0303:
	case 0x005C0402:
	case 0x005C0403:
		offset = 0x19C;
		break;
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000B:
		offset = 0x4E0;
		break;
	case 0x00620105:
	case 0x00620205:
	case 0x00621101:
	case 0x00621102:
	case 0x00621201:
	case 0x00621202:
		offset = 0x11C;
		break;
	case 0x00730204:
	case 0x00730404:
	case 0x00730604:
	case 0x00730804:
	case 0x00730A04:
	case 0x00730C04:
	case 0x00730E04:
	case 0x00731004:
		offset = 0x20C;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_uclk(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x001E0001:
		offset = 0x464;
		break;
	case 0x001E0002:
		offset = 0x478;
		break;
	case 0x001E0003:
	case 0x001E0004:
		offset = 0x29C;
		break;
	case 0x00240003:
		offset = 0xB8;
		break;
	case 0x00240503:
	case 0x00240603:
	case 0x00240703:
		offset = 0xC8;
		break;
	case 0x00240802:
		offset = 0xC4;
		break;
	case 0x00240803:
		offset = 0xC8;
		break;
	case 0x00240902:
		offset = 0xC4;
		break;
	case 0x00240903:
		offset = 0xC8;
		break;
	case 0x00260001:
		offset = 0x2C;
		break;
	case 0x002D0008:
	case 0x002D0803:
	case 0x002D0903:
		offset = 0xC4;
		break;
	case 0x00370000:
		offset = 0x4B8;
		break;
	case 0x00370001:
		offset = 0x5A8;
		break;
	case 0x00370002:
		offset = 0x5B0;
		break;
	case 0x00370003:
	case 0x00370004:
		offset = 0x5D0;
		break;
	case 0x00370005:
		offset = 0x5EC;
		break;
	case 0x00380005:
	case 0x00380505:
	case 0x00380605:
	case 0x00380705:
	case 0x00380804:
	case 0x00380805:
	case 0x00380904:
	case 0x00380905:
		offset = 0xC8;
		break;
	case 0x00400001:
		offset = 0x628;
		break;
	case 0x00400002:
		offset = 0x640;
		break;
	case 0x00400003:
		offset = 0x664;
		break;
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
		offset = 0x668;
		break;
	case 0x00450005:
		offset = 0x6B4;
		break;
	case 0x004C0003:
	case 0x004C0004:
	case 0x004C0005:
	case 0x004C0006:
	case 0x004C0007:
		offset = 0x184;
		break;
	case 0x004C0008:
	case 0x004C0009:
		offset = 0x174;
		break;
	case 0x00540000:
	case 0x00540001:
	case 0x00540002:
	case 0x00540003:
	case 0x00540004:
	case 0x00540005:
	case 0x00540100:
	case 0x00540101:
	case 0x00540102:
	case 0x00540103:
	case 0x00540104:
	case 0x00540105:
	case 0x00540108:
		offset = 0x128;
		break;
	case 0x00540208:
		offset = 0x12C;
		break;
	case 0x005C0002:
	case 0x005C0003:
	case 0x005C0102:
	case 0x005C0103:
	case 0x005C0202:
	case 0x005C0203:
	case 0x005C0302:
		offset = 0x1A8;
		break;
	case 0x005C0303:
	case 0x005C0402:
	case 0x005C0403:
		offset = 0x1B0;
		break;
	case 0x00620105:
	case 0x00620205:
	case 0x00621102:
	case 0x00621202:
		offset = 0x12C;
		break;
	case 0x00730204:
	case 0x00730404:
	case 0x00730604:
	case 0x00730804:
	case 0x00730A04:
	case 0x00730C04:
	case 0x00730E04:
	case 0x00731004:
		offset = 0x21C;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_mclk(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x001E0001:
		offset = 0x468;
		break;
	case 0x001E0002:
		offset = 0x47C;
		break;
	case 0x001E0003:
	case 0x001E0004:
		offset = 0x2A0;
		break;
	case 0x00240003:
		offset = 0xBC;
		break;
	case 0x00240503:
	case 0x00240603:
	case 0x00240703:
		offset = 0xCC;
		break;
	case 0x00240802:
		offset = 0xC8;
		break;
	case 0x00240803:
		offset = 0xCC;
		break;
	case 0x00240902:
		offset = 0xC8;
		break;
	case 0x00240903:
		offset = 0xCC;
		break;
	case 0x00260001:
		offset = 0x30;
		break;
	case 0x002D0008:
	case 0x002D0803:
	case 0x002D0903:
		offset = 0xC8;
		break;
	case 0x00370000:
		offset = 0x4BC;
		break;
	case 0x00370001:
		offset = 0x5AC;
		break;
	case 0x00370002:
		offset = 0x5B4;
		break;
	case 0x00370003:
	case 0x00370004:
		offset = 0x5D4;
		break;
	case 0x00370005:
		offset = 0x5F0;
		break;
	case 0x00380005:
	case 0x00380505:
	case 0x00380605:
	case 0x00380705:
	case 0x00380804:
	case 0x00380805:
	case 0x00380904:
	case 0x00380905:
		offset = 0xCC;
		break;
	case 0x00400001:
		offset = 0x62C;
		break;
	case 0x00400002:
		offset = 0x644;
		break;
	case 0x00400003:
		offset = 0x668;
		break;
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
		offset = 0x66C;
		break;
	case 0x00450005:
		offset = 0x6B8;
		break;
	case 0x004C0003:
	case 0x004C0004:
	case 0x004C0005:
	case 0x004C0006:
	case 0x004C0007:
		offset = 0x194;
		break;
	case 0x004C0008:
	case 0x004C0009:
		offset = 0x184;
		break;
	case 0x00540000:
	case 0x00540001:
	case 0x00540002:
	case 0x00540003:
	case 0x00540004:
	case 0x00540005:
	case 0x00540100:
	case 0x00540101:
	case 0x00540102:
	case 0x00540103:
	case 0x00540104:
	case 0x00540105:
	case 0x00540108:
		offset = 0x138;
		break;
	case 0x00540208:
		offset = 0x13C;
		break;
	case 0x005C0002:
	case 0x005C0003:
	case 0x005C0102:
	case 0x005C0103:
	case 0x005C0202:
	case 0x005C0203:
	case 0x005C0302:
		offset = 0x1BC;
		break;
	case 0x005C0303:
	case 0x005C0402:
	case 0x005C0403:
		offset = 0x1C4;
		break;
	case 0x00620105:
	case 0x00620205:
	case 0x00621102:
	case 0x00621202:
		offset = 0x13C;
		break;
	case 0x00730204:
	case 0x00730404:
	case 0x00730604:
	case 0x00730804:
	case 0x00730A04:
	case 0x00730C04:
	case 0x00730E04:
	case 0x00731004:
		offset = 0x22C;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_soc_volt(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x001E0001:
	case 0x001E0002:
		offset = 0x10C;
		break;
	case 0x001E0003:
	case 0x001E0004:
		offset = 0x104;
		break;
	case 0x00240003:
		offset = 0xA4;
		break;
	case 0x00240503:
	case 0x00240603:
	case 0x00240703:
		offset = 0xB4;
		break;
	case 0x00240802:
		offset = 0xB0;
		break;
	case 0x00240803:
		offset = 0xB4;
		break;
	case 0x00240902:
		offset = 0xB0;
		break;
	case 0x00240903:
		offset = 0xB4;
		break;
	case 0x00260001:
		offset = 0x10;
		break;
	case 0x002D0008:
	case 0x002D0803:
	case 0x002D0903:
		offset = 0xB0;
		break;
	case 0x00370000:
	case 0x00370001:
		offset = 0x190;
		break;
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
		offset = 0x198;
		break;
	case 0x00380005:
	case 0x00380505:
	case 0x00380605:
	case 0x00380705:
	case 0x00380804:
	case 0x00380805:
	case 0x00380904:
	case 0x00380905:
		offset = 0xB4;
		break;
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
		offset = 0x19C;
		break;
	case 0x00450005:
		offset = 0x1C8;
		break;
	case 0x004C0003:
	case 0x004C0004:
	case 0x004C0005:
	case 0x004C0006:
	case 0x004C0007:
		offset = 0x74;
		break;
	case 0x004C0008:
	case 0x004C0009:
		offset = 0x194;
		break;
	case 0x00540000:
	case 0x00540001:
	case 0x00540002:
	case 0x00540003:
	case 0x00540004:
	case 0x00540005:
	case 0x00540100:
	case 0x00540101:
	case 0x00540102:
	case 0x00540103:
	case 0x00540104:
	case 0x00540105:
	case 0x00540108:
		offset = 0xD0;
		break;
	case 0x00540208:
		offset = 0xD4;
		break;
	case 0x005C0002:
	case 0x005C0003:
	case 0x005C0102:
	case 0x005C0103:
	case 0x005C0202:
	case 0x005C0203:
	case 0x005C0302:
		offset = 0x11C;
		break;
	case 0x005C0303:
	case 0x005C0402:
	case 0x005C0403:
		offset = 0x124;
		break;
	case 0x00620105:
	case 0x00620205:
	case 0x00621102:
	case 0x00621202:
		offset = 0x14C;
		break;
	case 0x00730204:
	case 0x00730404:
	case 0x00730604:
	case 0x00730804:
	case 0x00730A04:
	case 0x00730C04:
	case 0x00730E04:
	case 0x00731004:
		offset = 0x23C;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_cldo_vddp(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x001E0001:
	case 0x001E0002:
		offset = 0xF8;
		break;
	case 0x001E0003:
	case 0x001E0004:
		offset = 0xF0;
		break;
	case 0x00240003:
		offset = 0x1E4;
		break;
	case 0x00240503:
	case 0x00240603:
	case 0x00240703:
	case 0x00240803:
	case 0x00240903:
		offset = 0x1F4;
		break;
	case 0x00240802:
	case 0x00240902:
		offset = 0x1F0;
		break;
	case 0x002D0008:
	case 0x002D0803:
	case 0x002D0903:
		offset = 0x220;
		break;
	case 0x00370000:
		offset = 0x72C;
		break;
	case 0x00370001:
		offset = 0x81C;
		break;
	case 0x00370002:
		offset = 0x824;
		break;
	case 0x00370003:
	case 0x00370004:
		offset = 0x844;
		break;
	case 0x00370005:
		offset = 0x86C;
		break;
	case 0x00380005:
	case 0x00380505:
	case 0x00380605:
	case 0x00380705:
	case 0x00380804:
	case 0x00380805:
	case 0x00380904:
	case 0x00380905:
		offset = 0x224;
		break;
	case 0x00400001:
		offset = 0x89C;
		break;
	case 0x00400002:
		offset = 0x8B4;
		break;
	case 0x00400003:
		offset = 0x8D0;
		break;
	case 0x00400004:
	case 0x00400005:
	case 0x00450004:
	case 0x00450005:
		offset = 0x8D4;
		break;
	case 0x004C0003:
	case 0x004C0004:
	case 0x004C0005:
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
		offset = 0x768;
		break;
	case 0x004C0009:
		offset = 0x774;
		break;
	case 0x00540000:
	case 0x00540001:
	case 0x00540002:
	case 0x00540003:
	case 0x00540004:
	case 0x00540005:
	case 0x00540100:
	case 0x00540101:
	case 0x00540102:
	case 0x00540103:
	case 0x00540104:
	case 0x00540105:
	case 0x00540108:
		offset = 0x430;
		break;
	case 0x00540208:
		offset = 0x434;
		break;
	case 0x00620105:
	case 0x00620205:
	case 0x00621102:
	case 0x00621202:
		offset = 0x434;
		break;
	case 0x00730204:
	case 0x00730404:
	case 0x00730604:
	case 0x00730804:
	case 0x00730A04:
	case 0x00730C04:
	case 0x00730E04:
	case 0x00731004:
		offset = 0x5CC;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_psi0_current(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		offset = 0x40;
		break;
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		offset = 0x78;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_psi0soc_current(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x001E0001:
	case 0x001E0002:
	case 0x001E0003:
	case 0x001E0004:
	case 0x001E0005:
	case 0x001E000A:
	case 0x001E0101:
		offset = 0x48;
		break;
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
	case 0x004C0006:
	case 0x004C0007:
	case 0x004C0008:
	case 0x004C0009:
		offset = 0x80;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_l3_clk(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		offset = 0x568;
		break;
	case 0x00370005:
		offset = 0x584;
		break;
	case 0x003F0000:
		offset = 0x35C;
		break;
	case 0x00400004:
	case 0x00400005:
		offset = 0x614;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_l3_vddm(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		offset = 0x548;
		break;
	case 0x00370005:
		offset = 0x564;
		break;
	case 0x003F0000:
		offset = 0x34C;
		break;
	case 0x00400004:
	case 0x00400005:
		offset = 0x604;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_l3_temperature(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
		offset = 0x550;
		break;
	case 0x00370005:
		offset = 0x56C;
		break;
	case 0x003F0000:
		offset = 0x350;
		break;
	case 0x00400004:
	case 0x00400005:
		offset = 0x608;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}

ry_err_t ryzen_smu_get_socket_power(ry_handle_t* handle, float* data)
{
	size_t offset = OFFSET_INVALID;
	switch (handle->pm_table_version)
	{
	case 0x00370000:
	case 0x00370001:
	case 0x00370002:
	case 0x00370003:
	case 0x00370004:
	case 0x00370005:
	case 0x00400001:
	case 0x00400002:
	case 0x00400003:
	case 0x00400004:
	case 0x00400005:
		offset = 0x98;
		break;
	case 0x003F0000:
		offset = 0xA8;
		break;
	case 0x005D0008:
	case 0x005D0009:
	case 0x005D000B:
		offset = 0xD0;
		break;
	}
	if (offset == OFFSET_INVALID)
		return RYZEN_SMU_UNSUPPORTED;
	return ryzen_smu_get_pm_table_float(handle, offset, data);
}
