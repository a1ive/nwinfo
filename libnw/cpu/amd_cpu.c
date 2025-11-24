// SPDX-License-Identifier: Unlicense

#include "rdmsr.h"
#include "ryzen_smu.h"

/*
  AMD BIOS and Kernel Developer's Guide (BKDG)
  * AMD Family 10h Processors
  http://support.amd.com/TechDocs/31116.pdf
  * AMD Family 11h Processors
  http://support.amd.com/TechDocs/41256.pdf
  * AMD Family 12h Processors
  http://support.amd.com/TechDocs/41131.pdf
  * AMD Family 14h Processors
  http://support.amd.com/TechDocs/43170_14h_Mod_00h-0Fh_BKDG.pdf
  * AMD Family 15h Processors
  http://support.amd.com/TechDocs/42301_15h_Mod_00h-0Fh_BKDG.pdf
  http://support.amd.com/TechDocs/42300_15h_Mod_10h-1Fh_BKDG.pdf
  http://support.amd.com/TechDocs/49125_15h_Models_30h-3Fh_BKDG.pdf
  http://support.amd.com/TechDocs/50742_15h_Models_60h-6Fh_BKDG.pdf
  http://support.amd.com/TechDocs/55072_AMD_Family_15h_Models_70h-7Fh_BKDG.pdf
  * AMD Family 16h Processors
  http://support.amd.com/TechDocs/48751_16h_bkdg.pdf
  http://support.amd.com/TechDocs/52740_16h_Models_30h-3Fh_BKDG.pdf

  AMD Processor Programming Reference (PPR)
  * AMD Family 17h Processors
  Models 00h-0Fh: https://www.amd.com/system/files/TechDocs/54945-ppr-family-17h-models-00h-0fh-processors.zip
  Models 01h, 08h, Revision B2: https://www.amd.com/system/files/TechDocs/54945_3.03_ppr_ZP_B2_pub.zip
  Model 71h, Revision B0: https://www.amd.com/system/files/TechDocs/56176_ppr_Family_17h_Model_71h_B0_pub_Rev_3.06.zip
  Model 60h, Revision A1: https://www.amd.com/system/files/TechDocs/55922-A1-PUB.zip
  Model 18h, Revision B1: https://www.amd.com/system/files/TechDocs/55570-B1-PUB.zip
  Model 20h, Revision A1: https://www.amd.com/system/files/TechDocs/55772-A1-PUB.zip
  Model 31h, Revision B0 Processors https://www.amd.com/system/files/TechDocs/55803-ppr-family-17h-model-31h-b0-processors.pdf
  Models A0h-AFh, Revision A0: https://www.amd.com/system/files/TechDocs/57243-A0-PUB.zip
  * AMD Family 19h Processors
  Model 01h, Revision B1: https://www.amd.com/system/files/TechDocs/55898_B1_pub_0.50.zip
  Model 21h, Revision B0: https://www.amd.com/system/files/TechDocs/56214-B0-PUB.zip
  Model 51h, Revision A1: https://www.amd.com/system/files/TechDocs/56569-A1-PUB.zip
  Model 11h, Revision B1: https://www.amd.com/system/files/TechDocs/55901_0.25.zip
  Model 61h, Revision B1: https://www.amd.com/system/files/TechDocs/56713-B1-PUB_3.04.zip
  Model 70h, Revision A0: https://www.amd.com/system/files/TechDocs/57019-A0-PUB_3.00.zip
*/

#define MSR_PSTATE_L           0xC0010061
#define MSR_PSTATE_S           0xC0010063
#define MSR_PSTATE_0           0xC0010064
#define MSR_PSTATE_1           0xC0010065
#define MSR_PSTATE_2           0xC0010066
#define MSR_PSTATE_3           0xC0010067
#define MSR_PSTATE_4           0xC0010068
#define MSR_PSTATE_5           0xC0010069
#define MSR_PSTATE_6           0xC001006A
#define MSR_PSTATE_7           0xC001006B
#define MSR_PWR_UNIT           0xC0010299
#define MSR_PKG_ENERGY_STAT    0xC001029B

static inline int
read_amd_msr(struct msr_info_t* info, uint32_t msr_index, uint8_t highbit, uint8_t lowbit, uint64_t* result)
{
	int err = ERR_CPU_UNKN;
	const uint8_t bits = highbit - lowbit + 1;
	ULONG64 in = msr_index;
	ULONG64 out = 0;

	if (highbit > 63 || lowbit > highbit)
		return ERR_INVRANGE;

	if (info->handle->driver_type == WR0_DRIVER_PAWNIO)
	{
		if (info->id->x86.ext_family == 0x0f)
			err = WR0_ExecPawn(info->handle, &info->handle->pio_amd0f, "ioctl_read_msr", &in, 1, &out, 1, NULL);
		else if (info->id->x86.ext_family >= 0x10 && info->id->x86.ext_family <= 0x16)
			err = WR0_ExecPawn(info->handle, &info->handle->pio_amd10, "ioctl_read_msr", &in, 1, &out, 1, NULL);
		else if (info->id->x86.ext_family >= 0x17 && info->id->x86.ext_family <= 0x1A)
			err = WR0_ExecPawn(info->handle, &info->handle->pio_amd17, "ioctl_read_msr", &in, 1, &out, 1, NULL);
	}
	else
		err = WR0_RdMsr(info->handle, msr_index, &out);

	if (!err && bits < 64)
	{
		/* Show only part of register */
		out >>= lowbit;
		out &= (1ULL << bits) - 1;
		*result = out;
	}

	return err;
}

static int get_amd_multipliers(struct msr_info_t* info, uint32_t pstate, double* multiplier)
{
	int i, err;
	uint64_t cpu_fid, cpu_did, cpu_did_lsd;

	/* Constant values needed for 12h family */
	const struct
	{
		uint64_t did;
		double divisor;
	} divisor_t[] =
	{
		{ 0x0, 1 },
		{ 0x1, 1.5 },
		{ 0x2, 2 },
		{ 0x3, 3 },
		{ 0x4, 4 },
		{ 0x5, 6 },
		{ 0x6, 8 },
		{ 0x7, 12 },
		{ 0x8, 16 },
	};
	const int num_dids = (int)COUNT_OF(divisor_t);

	/* Constant values for common families */
	const int magic_constant = (info->id->x86.ext_family == 0x11) ? 0x8 : 0x10;
	const int is_apu = (strstr(info->id->brand_str, "APU") != NULL) || (strstr(info->id->brand_str, "Radeon ") != NULL);
	const double divisor = is_apu ? 1.0 : 2.0;

	/* Check if P-state is valid */
	if (pstate < MSR_PSTATE_0 || MSR_PSTATE_7 < pstate)
		return 1;

	/* Overview of AMD CPU microarchitectures: https://en.wikipedia.org/wiki/List_of_AMD_CPU_microarchitectures#Nomenclature */
	switch (info->id->x86.ext_family)
	{
	case 0x12: /* K10 (Llano) / K12 */
		/* BKDG 12h, page 469
		MSRC001_00[6B:64][8:4] is cpu_fid
		MSRC001_00[6B:64][3:0] is cpu_did
		CPU COF is (100MHz * (cpu_fid + 10h) / (divisor specified by cpu_did))
		Note: This family contains only APUs */
		err = read_amd_msr(info, pstate, 8, 4, &cpu_fid);
		err += read_amd_msr(info, pstate, 3, 0, &cpu_did);
		i = 0;
		while (i < num_dids && divisor_t[i].did != cpu_did)
			i++;
		if (i < num_dids)
			*multiplier = (double)((cpu_fid + magic_constant) / divisor_t[i].divisor);
		else
			err++;
		break;
	case 0x14: /* Bobcat */
		/* BKDG 14h, page 430
		MSRC001_00[6B:64][8:4] is CpuDidMSD
		MSRC001_00[6B:64][3:0] is cpu_did_lsd
		PLL COF is (100 MHz * (D18F3xD4[MainPllOpFreqId] + 10h))
		Divisor is (CpuDidMSD + (cpu_did_lsd * 0.25) + 1)
		CPU COF is (main PLL frequency specified by D18F3xD4[MainPllOpFreqId]) / (core clock divisor specified by CpuDidMSD and cpu_did_lsd)
		Note: This family contains only APUs */
		err = read_amd_msr(info, pstate, 8, 4, &cpu_did);
		err += read_amd_msr(info, pstate, 3, 0, &cpu_did_lsd);
		*multiplier = (double)(((info->cpu_clock + 5LL) / 100 + magic_constant) / (cpu_did + cpu_did_lsd * 0.25 + 1));
		break;
	case 0x10: /* K10 */
		/* BKDG 10h, page 429
		MSRC001_00[6B:64][8:6] is cpu_did
		MSRC001_00[6B:64][5:0] is cpu_fid
		CPU COF is (100 MHz * (cpu_fid + 10h) / (2^cpu_did))
		Note: This family contains only CPUs */
		/* Fall through */
	case 0x11: /* K8 & K10 "hybrid" */
		/* BKDG 11h, page 236
		MSRC001_00[6B:64][8:6] is cpu_did
		MSRC001_00[6B:64][5:0] is cpu_fid
		CPU COF is ((100 MHz * (cpu_fid + 08h)) / (2^cpu_did))
		Note: This family contains only CPUs */
		/* Fall through */
	case 0x15: /* Bulldozer / Piledriver / Steamroller / Excavator */
		/* BKDG 15h, page 570/580/635/692 (00h-0Fh/10h-1Fh/30h-3Fh/60h-6Fh)
		MSRC001_00[6B:64][8:6] is cpu_did
		MSRC001_00[6B:64][5:0] is cpu_fid
		CoreCOF is (100 * (MSRC001_00[6B:64][cpu_fid] + 10h) / (2^MSRC001_00[6B:64][cpu_did]))
		Note: This family contains BOTH CPUs and APUs */
		/* Fall through */
	case 0x16: /* Jaguar / Puma */
		/* BKDG 16h, page 549/611 (00h-0Fh/30h-3Fh)
		MSRC001_00[6B:64][8:6] is cpu_did
		MSRC001_00[6B:64][5:0] is cpu_fid
		CoreCOF is (100 * (MSRC001_00[6B:64][cpu_fid] + 10h) / (2^MSRC001_00[6B:64][cpu_did]))
		Note: This family contains only APUs */
		err = read_amd_msr(info, pstate, 8, 6, &cpu_did);
		err += read_amd_msr(info, pstate, 5, 0, &cpu_fid);
		*multiplier = ((double)(cpu_fid + magic_constant) / (1ull << cpu_did)) / divisor;
		break;
	case 0x17: /* Zen / Zen+ / Zen 2 */
		/* PPR 17h, pages 30 and 138-139
		MSRC001_00[6B:64][13:8] is CpuDfsId
		MSRC001_00[6B:64][7:0]  is cpu_fid
		CoreCOF is (Core::X86::Msr::PStateDef[cpu_fid[7:0]] / Core::X86::Msr::PStateDef[CpuDfsId]) * 200 */
		/* Fall through */
	case 0x18: /* Hygon Dhyana */
		/* Note: Dhyana is "mostly a re-branded Zen CPU for the Chinese server market"
		https://www.phoronix.com/news/Hygon-Dhyana-AMD-China-CPUs */
		/* Fall through */
	case 0x19: /* Zen 3 / Zen 3+ / Zen 4 */
		/* PPR for AMD Family 19h Model 70h A0, pages 37 and 206-207
		MSRC001_006[4...B][13:8] is CpuDfsId
		MSRC001_006[4...B][7:0]  is cpu_fid
		CoreCOF is (Core::X86::Msr::PStateDef[cpu_fid[7:0]]/Core::X86::Msr::PStateDef[CpuDfsId]) *200 */
		err = read_amd_msr(info, pstate, 13, 8, &cpu_did);
		err += read_amd_msr(info, pstate, 7, 0, &cpu_fid);
		*multiplier = ((double)cpu_fid / cpu_did) * 2;
		break;
	case 0x1A: /* Zen 5 */
		/* PPR for AMD Family 1Ah Model 02h C1, pages 235
		MSRC001_006[4...B][11:0]  is cpu_fid
		CoreCOF is Core::X86::Msr::PStateDef[CpuFid[11:0]] *5 */
		err = read_amd_msr(info, pstate, 11, 0, &cpu_fid);
		*multiplier = ((double)cpu_fid) * 0.05;
		break;
	default:
		warnf("get_amd_multipliers(): unsupported CPU extended family: %xh\n", info->id->x86.ext_family);
		err = 1;
		break;
	}

	return err;
}

static uint32_t get_amd_last_pstate_addr(struct msr_info_t* info)
{
	/* Refer links above
		MSRC001_00[6B:64][63] is PstateEn
		PstateEn indicates if the rest of the P-state information in the register is valid after a reset
	*/
	uint64_t reg = 0x0;
	uint32_t last_addr = MSR_PSTATE_7 + 1;
	while ((reg == 0x0) && (last_addr > MSR_PSTATE_0))
	{
		last_addr--;
		read_amd_msr(info, last_addr, 63, 63, &reg);
	}
	return last_addr;
}

static double get_min_multiplier(struct msr_info_t* info)
{
	/* N.B.: Find the last P-state
		get_amd_last_pstate_addr() returns the last P-state, MSR_PSTATE_0 <= addr <= MSR_PSTATE_7
	*/
	double mult;
	uint32_t addr = get_amd_last_pstate_addr(info);
	if (get_amd_multipliers(info, addr, &mult))
		goto fail;
	return mult;

fail:
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_cur_multiplier(struct msr_info_t* info)
{
	double mult;
	uint64_t reg;
	/* Refer links above
		MSRC001_0063[2:0] is CurPstate
	*/
	if (read_amd_msr(info, MSR_PSTATE_S, 2, 0, &reg))
		goto fail;
	if (get_amd_multipliers(info, MSR_PSTATE_0 + (uint32_t)reg, &mult))
		goto fail;
	return mult;

fail:
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_max_multiplier(struct msr_info_t* info)
{
	double mult;
	/* Refer links above
		MSRC001_0064 is Pb0
		Pb0 is the highest-performance boosted P-state
	*/
	if (get_amd_multipliers(info, MSR_PSTATE_0, &mult))
		goto fail;
	return mult;

fail:
	return (double)CPU_INVALID_VALUE / 100;
}

static int get_temperature(struct msr_info_t* info)
{
	float value = 0.0f;
	WR0_WaitPciBus(500);
	ry_handle_t* ry = ryzen_smu_init(info->handle, info->id);
	if (ry == NULL)
		goto fail;
	if (ryzen_smu_init_pm_table(ry) != RYZEN_SMU_OK)
		goto fail;
	if (ryzen_smu_update_pm_table(ry) != RYZEN_SMU_OK)
		goto fail;
	if (ryzen_smu_get_core_temperature(ry, 0, &value) != RYZEN_SMU_OK)
		goto fail;
	ryzen_smu_free(ry);
	WR0_ReleasePciBus();
	return (int)value;
fail:
	if (ry != NULL)
		ryzen_smu_free(ry);
	WR0_ReleasePciBus();
	return CPU_INVALID_VALUE;
}

#define THERMTRIP_STATUS_REGISTER       0xE4
#define AMD_PCI_VENDOR_ID               0x1022
#define AMD_PCI_CONTROL_DEVICE_ID       0x1103

static int amd_k8_temperature(struct msr_info_t* info)
{
	uint32_t value;
	uint32_t addr;
	int offset = -49;
	if (info->id->x86.ext_model >= 0x69 &&
		info->id->x86.ext_model != 0xc1 &&
		info->id->x86.ext_model != 0x6c &&
		info->id->x86.ext_model != 0x7c)
		offset += 21;

	if (info->handle->driver_type != WR0_DRIVER_PAWNIO)
	{
		addr = WR0_FindPciById(info->handle, AMD_PCI_VENDOR_ID, AMD_PCI_CONTROL_DEVICE_ID, info->id->index);

		if (addr == 0xFFFFFFFF)
			return CPU_INVALID_VALUE;

		WR0_WrPciConf32(info->handle, addr, THERMTRIP_STATUS_REGISTER, 0);
		value = WR0_RdPciConf32(info->handle, addr, THERMTRIP_STATUS_REGISTER);
	}
	else
	{
		struct pio_mod_t* pio = &info->handle->pio_amd0f;
		ULONG64 in[2] = { 0, 0 }; // cpu index, core index
		ULONG64 out;
		if (WR0_ExecPawn(info->handle, pio, "ioctl_get_thermtrip", in, 2, &out, 1, NULL))
			return CPU_INVALID_VALUE;
		value = (uint32_t)out;
	}
	
	return (int)((value >> 16) & 0xFF) + offset;
}

#define SMU_REPORTED_TEMP_CTRL_OFFSET              0xD8200CA4

#define FAMILY_10H_MISCELLANEOUS_CONTROL_DEVICE_ID 0x1203
#define FAMILY_11H_MISCELLANEOUS_CONTROL_DEVICE_ID 0x1303
#define FAMILY_12H_MISCELLANEOUS_CONTROL_DEVICE_ID 0x1703
#define FAMILY_14H_MISCELLANEOUS_CONTROL_DEVICE_ID 0x1703
#define FAMILY_15H_MODEL_00_MISC_CONTROL_DEVICE_ID 0x1603
#define FAMILY_15H_MODEL_10_MISC_CONTROL_DEVICE_ID 0x1403
#define FAMILY_15H_MODEL_30_MISC_CONTROL_DEVICE_ID 0x141D
#define FAMILY_15H_MODEL_60_MISC_CONTROL_DEVICE_ID 0x1573
#define FAMILY_15H_MODEL_70_MISC_CONTROL_DEVICE_ID 0x15B3
#define FAMILY_16H_MODEL_00_MISC_CONTROL_DEVICE_ID 0x1533
#define FAMILY_16H_MODEL_30_MISC_CONTROL_DEVICE_ID 0x1583

#define NB_PCI_REG_ADDR_ADDR 0xB8
#define NB_PCI_REG_DATA_ADDR 0xBC

static int amd_k10_temperature(struct msr_info_t* info)
{
	uint32_t value = 0;
	uint32_t addr;
	uint16_t did = 0;
	bool smu = false;
	switch (info->id->x86.ext_family)
	{
	case 0x10:
		did = FAMILY_10H_MISCELLANEOUS_CONTROL_DEVICE_ID;
		break;
	case 0x11:
		did = FAMILY_11H_MISCELLANEOUS_CONTROL_DEVICE_ID;
		break;
	case 0x12:
		did = FAMILY_12H_MISCELLANEOUS_CONTROL_DEVICE_ID;
		break;
	case 0x14:
		did = FAMILY_14H_MISCELLANEOUS_CONTROL_DEVICE_ID;
		break;
	case 0x15:
		switch (info->id->x86.ext_model & 0xF0)
		{
		case 0x00:
			did = FAMILY_15H_MODEL_00_MISC_CONTROL_DEVICE_ID;
			break;
		case 0x10:
			did = FAMILY_15H_MODEL_10_MISC_CONTROL_DEVICE_ID;
			break;
		case 0x30:
			did = FAMILY_15H_MODEL_30_MISC_CONTROL_DEVICE_ID;
			break;
		case 0x60:
			did = FAMILY_15H_MODEL_60_MISC_CONTROL_DEVICE_ID;
			smu = true;
			break;
		case 0x70:
			did = FAMILY_15H_MODEL_70_MISC_CONTROL_DEVICE_ID;
			smu = true;
			break;
		}
		break;
	case 0x16:
		switch (info->id->x86.ext_model & 0xF0)
		{
		case 0x00:
			did = FAMILY_16H_MODEL_00_MISC_CONTROL_DEVICE_ID;
			break;
		case 0x30:
			did = FAMILY_16H_MODEL_30_MISC_CONTROL_DEVICE_ID;
			break;
		};
		break;
	}

	if (smu)
	{
		if (info->handle->driver_type == WR0_DRIVER_CPUZ161)
			value = WR0_RdAmdSmn(info->handle, 0, 1, SMU_REPORTED_TEMP_CTRL_OFFSET);
		else if (info->handle->driver_type == WR0_DRIVER_PAWNIO)
		{
			struct pio_mod_t* pio = &info->handle->pio_amd10;
			ULONG64 in = SMU_REPORTED_TEMP_CTRL_OFFSET;
			ULONG64 out;
			if (WR0_ExecPawn(info->handle, pio, "ioctl_read_smu", &in, 1, &out, 1, NULL) == 0)
				value = (uint32_t)out;
		}
		else
		{
			WR0_WrPciConf32(info->handle, 0, NB_PCI_REG_ADDR_ADDR, SMU_REPORTED_TEMP_CTRL_OFFSET);
			value = WR0_RdPciConf32(info->handle, 0, NB_PCI_REG_DATA_ADDR);
		}
	}
	else
	{
		if (info->handle->driver_type == WR0_DRIVER_PAWNIO)
		{
			struct pio_mod_t* pio = &info->handle->pio_amd10;
			ULONG64 in[2] = { 0, 0xA4 }; // cpu index, offset
			ULONG64 out;
			if (WR0_ExecPawn(info->handle, pio, "ioctl_read_miscctl", in, 2, &out, 1, NULL) == 0)
				value = (uint32_t)out;
		}
		else
		{
			addr = WR0_FindPciById(info->handle, AMD_PCI_VENDOR_ID, did, info->id->index);
			if (addr == 0xFFFFFFFF)
				return CPU_INVALID_VALUE;
			value = WR0_RdPciConf32(info->handle, addr, 0xA4);
		}
	}
	if ((info->id->x86.ext_family == 0x15 ||
		info->id->x86.ext_family == 0x16)
		&& (value & 0x30000) == 0x3000)
	{
		if (info->id->x86.ext_family == 0x15 && (info->id->x86.ext_model & 0xF0) == 0x00)
			return (int)(((value >> 21) & 0x7FC) / 8.0f) - 49;
		return (int)(((value >> 21) & 0x7FF) / 8.0f) - 49;
	}
	return (int)(((value >> 21) & 0x7FF) / 8.0f);
}

#define F17H_M01H_THM_TCON_CUR_TMP          0x00059800
#define F17H_TEMP_OFFSET_FLAG               0x80000
#define FAMILY_17H_PCI_CONTROL_REGISTER     0x60
#define FAMILY_17H_PCI_DATA_REGISTER        0x64

static float amd_17h_temperature(struct msr_info_t* info)
{
	uint32_t temperature;
	float offset = 0.0f;

	if (info->handle->driver_type == WR0_DRIVER_CPUZ161)
	{
		temperature = WR0_RdAmdSmn(info->handle, 0, 3, F17H_M01H_THM_TCON_CUR_TMP);
	}
	else if (info->handle->driver_type == WR0_DRIVER_PAWNIO)
	{
		ULONG64 in = F17H_M01H_THM_TCON_CUR_TMP;
		ULONG64 out = 0;
		WR0_ExecPawn(info->handle, &info->handle->pio_amd17, "ioctl_read_smn", &in, 1, &out, 1, NULL);
		temperature = (uint32_t)out;
	}
	else
	{
		WR0_WrPciConf32(info->handle, 0, FAMILY_17H_PCI_CONTROL_REGISTER, F17H_M01H_THM_TCON_CUR_TMP);
		temperature = WR0_RdPciConf32(info->handle, 0, FAMILY_17H_PCI_DATA_REGISTER);
	}

	if (strstr(info->id->brand_str, "1600X") ||
		strstr(info->id->brand_str, "1700X") ||
		strstr(info->id->brand_str, "1800X"))
		offset = -20.0f;
	else if (strstr(info->id->brand_str, "2700X"))
		offset = -10.0f;
	else if (strstr(info->id->brand_str, "Threadripper 19") ||
		strstr(info->id->brand_str, "Threadripper 29"))
		offset = -27.0f;

	if ((temperature & F17H_TEMP_OFFSET_FLAG))
		offset += -49.0f;

	return 0.001f * ((temperature >> 21) * 125) + offset;
}

static int get_pkg_temperature(struct msr_info_t* info)
{
	int ret = CPU_INVALID_VALUE;
	WR0_WaitPciBus(10);
	if (info->id->x86.ext_family >= 0x17)
		ret = (int)amd_17h_temperature(info);
	else if (info->id->x86.ext_family > 0x0F)
		ret = amd_k10_temperature(info);
	else if (info->id->x86.ext_family == 0x0F)
		ret = amd_k8_temperature(info);
	WR0_ReleasePciBus();
	return ret;
}

static double get_pkg_energy(struct msr_info_t* info)
{
	uint64_t total_energy, energy_units;
	// 17h: Zen / Zen+ / Zen 2
	// 18h: Hygon Dhyana
	// 19h: Zen 3 / Zen 3+ / Zen 4
	if (info->id->x86.ext_family < 0x17)
		goto fail;
	if (read_amd_msr(info, MSR_PKG_ENERGY_STAT, 31, 0, &total_energy))
		goto fail;
	if (read_amd_msr(info, MSR_PWR_UNIT, 12, 8, &energy_units))
		goto fail;
	return (double)total_energy / (1ULL << energy_units);
fail:
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_pkg_pl1(struct msr_info_t* info)
{
	float value = 0.0f;
	WR0_WaitPciBus(500);
	ry_handle_t* ry = ryzen_smu_init(info->handle, info->id);
	if (ry == NULL)
		goto fail;
	if (ryzen_smu_init_pm_table(ry) != RYZEN_SMU_OK)
		goto fail;
	if (ryzen_smu_update_pm_table(ry) != RYZEN_SMU_OK)
		goto fail;
	if (ryzen_smu_get_slow_limit(ry, &value) != RYZEN_SMU_OK)
		goto fail;
	ryzen_smu_free(ry);
	WR0_ReleasePciBus();
	return (double)value;
fail:
	if (ry != NULL)
		ryzen_smu_free(ry);
	WR0_ReleasePciBus();
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_pkg_pl2(struct msr_info_t* info)
{
	float value = 0.0f;
	WR0_WaitPciBus(500);
	ry_handle_t* ry = ryzen_smu_init(info->handle, info->id);
	if (ry == NULL)
		goto fail;
	if (ryzen_smu_init_pm_table(ry) != RYZEN_SMU_OK)
		goto fail;
	if (ryzen_smu_update_pm_table(ry) != RYZEN_SMU_OK)
		goto fail;
	if (ryzen_smu_get_fast_limit(ry, &value) != RYZEN_SMU_OK)
		goto fail;
	ryzen_smu_free(ry);
	WR0_ReleasePciBus();
	return (double)value;
fail:
	if (ry != NULL)
		ryzen_smu_free(ry);
	WR0_ReleasePciBus();
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_voltage(struct msr_info_t* info)
{
	/* Refer links above
		MSRC001_00[6B:64][15:9]  is vid (Jaguar and before)
		MSRC001_00[6B:64][21:14] is vid (Zen)
		MSRC001_0063[2:0] is P-state Status
		BKDG 10h, page 49: voltage = 1.550V - 0.0125V * SviVid (SVI1)
		BKDG 15h, page 50: Voltage = 1.5500 - 0.00625 * Vid[7:0] (SVI2)
		SVI2 since Piledriver (Family 15h, 2nd-gen): Models 10h-1Fh Processors
	*/
	double vid_step;
	uint64_t reg, vid;
	uint8_t range_h, range_l;

	if (read_amd_msr(info, MSR_PSTATE_S, 2, 0, &reg))
		goto fail;
	if (info->id->x86.ext_family < 0x15
		|| ((info->id->x86.ext_family == 0x15) && (info->id->x86.ext_model < 0x10)))
		vid_step = 0.0125;
	else
		vid_step = 0.00625;

	if (info->id->x86.ext_family < 0x17)
	{
		range_h = 15;
		range_l = 9;
	}
	else
	{
		range_h = 21;
		range_l = 14;
	}
	if (read_amd_msr(info, MSR_PSTATE_0 + (uint32_t)reg, range_h, range_l, &vid))
		goto fail;
	if (MSR_PSTATE_0 + (uint32_t)reg <= MSR_PSTATE_7)
		return 1.550 - vid_step * vid;
fail:
	return (double)CPU_INVALID_VALUE / 100;
}

static double get_bus_clock(struct msr_info_t* info)
{
	/* Refer links above
		MSRC001_0061[6:4] is PstateMaxVal
		PstateMaxVal is the the lowest-performance non-boosted P-state
	*/
	double mult;
	uint64_t reg;
	uint32_t addr = get_amd_last_pstate_addr(info);
	if (read_amd_msr(info, MSR_PSTATE_L, 6, 4, &reg))
		goto fail;
	if (get_amd_multipliers(info, addr - (uint32_t)reg, &mult))
		goto fail;
	return (double)info->cpu_clock / mult;
fail:
	return (double)CPU_INVALID_VALUE / 100;
}

static int get_igpu_temperature(struct msr_info_t* info)
{
	float value = 0.0f;
	WR0_WaitPciBus(500);
	ry_handle_t* ry = ryzen_smu_init(info->handle, info->id);
	if (ry == NULL)
		goto fail;
	if (ryzen_smu_init_pm_table(ry) != RYZEN_SMU_OK)
		goto fail;
	if (ryzen_smu_update_pm_table(ry) != RYZEN_SMU_OK)
		goto fail;
	if (ryzen_smu_get_apu_temperature(ry, &value) != RYZEN_SMU_OK)
		goto fail;
	ryzen_smu_free(ry);
	WR0_ReleasePciBus();
	return (int)value;
fail:
	if (ry != NULL)
		ryzen_smu_free(ry);
	WR0_ReleasePciBus();
	return CPU_INVALID_VALUE;
}

static double get_igpu_energy(struct msr_info_t* info)
{
	return (double)CPU_INVALID_VALUE / 100;
}

struct msr_fn_t msr_fn_amd =
{
	.get_min_multiplier = get_min_multiplier,
	.get_cur_multiplier = get_cur_multiplier,
	.get_max_multiplier = get_max_multiplier,
	.get_temperature = get_temperature,
	.get_pkg_temperature = get_pkg_temperature,
	.get_pkg_energy = get_pkg_energy,
	.get_pkg_pl1 = get_pkg_pl1,
	.get_pkg_pl2 = get_pkg_pl2,
	.get_voltage = get_voltage,
	.get_bus_clock = get_bus_clock,
	.get_igpu_temperature = get_igpu_temperature,
	.get_igpu_energy = get_igpu_energy,
};
