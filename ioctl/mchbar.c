// SPDX-License-Identifier: Unlicense

#include "cpuid.h"
#include "ioctl.h"
#include "mchbar.h"
#include <libnw.h>

static struct
{
	bool init_done;
	struct cpu_id_t* id;
	uint64_t mchbar_base;
	uint64_t pch_base;
} ctx;

uint64_t
mchbar_get_mmio_reg(void)
{
	return ctx.mchbar_base;
}

uint64_t
pch_get_mmio_reg(void)
{
	return ctx.pch_base;
}

uint32_t
mchbar_read_32(uint64_t offset)
{
	if (!ctx.mchbar_base)
		return 0;

	uint32_t data = 0;
	int ret = WR0_RdMmIo(NWLC->NwDrv, ctx.mchbar_base + offset, &data, sizeof(uint32_t));
	if (ret)
		NWL_Debug("MCHBAR", "MMIO 32 @%llX+%llx failed.", ctx.mchbar_base, offset);
	return data;
}

uint32_t
pch_read_32(uint64_t offset)
{
	if (!ctx.pch_base)
		return 0;

	uint32_t data = 0;
	int ret = WR0_RdMmIo(NWLC->NwDrv, ctx.pch_base + offset, &data, sizeof(uint32_t));
	if (ret)
		NWL_Debug("PCH", "MMIO 32 @%llX+%llx failed.", ctx.pch_base, offset);
	return data;
}

uint64_t
mchbar_read_64(uint64_t offset)
{
	if (!ctx.mchbar_base)
		return 0;

	uint64_t data = 0;
	int ret = WR0_RdMmIo(NWLC->NwDrv, ctx.mchbar_base + offset, &data, sizeof(uint64_t));
	if (ret)
		NWL_Debug("MCHBAR", "MMIO 64 @%llX+%llx failed.", ctx.mchbar_base, offset);
	return data;
}

#define MCHBAR_BASE_REG_LOW     0x48
#define MCHBAR_BASE_REG_HIGH    0x4C
#define PWRMBASE_DEFAULT 0xFE000000

static uint32_t
mchbar_get_base_32(void)
{
	uint32_t mmio_reg = 0;
	mmio_reg = WR0_RdPciConf32(NWLC->NwDrv, 0, MCHBAR_BASE_REG_LOW);
	if (!(mmio_reg & 0x01))
	{
		WR0_WrPciConf32(NWLC->NwDrv, 0, MCHBAR_BASE_REG_LOW, mmio_reg | 0x01);
		mmio_reg = WR0_RdPciConf32(NWLC->NwDrv, 0, MCHBAR_BASE_REG_LOW);
		if (!(mmio_reg & 0x1))
			return 0;
	}
	if (mmio_reg == 0xFFFFFFFF)
		return 0;
	mmio_reg &= 0xFFFFC000;
	return mmio_reg;
}

static uint64_t
mchbar_get_base_64(uint64_t base_mask)
{
	uint64_t mmio_reg = 0;
	mmio_reg = WR0_RdPciConf32(NWLC->NwDrv, 0, MCHBAR_BASE_REG_LOW);
	if (!(mmio_reg & 0x01))
	{
		WR0_WrPciConf32(NWLC->NwDrv, 0, MCHBAR_BASE_REG_LOW, (uint32_t)mmio_reg | 0x01);
		mmio_reg = WR0_RdPciConf32(NWLC->NwDrv, 0, MCHBAR_BASE_REG_LOW);
		if (!(mmio_reg & 0x01))
			return 0;
	}
	if (mmio_reg == 0xFFFFFFFF)
		return 0;

	uint64_t mmio_reg_high = WR0_RdPciConf32(NWLC->NwDrv, 0, MCHBAR_BASE_REG_HIGH);
	if (mmio_reg_high == 0xFFFFFFFF)
		return 0;

	mmio_reg |= mmio_reg_high << 32;
	mmio_reg &= base_mask;
	return mmio_reg;
}

static const uint32_t INTEL_THERMAL_IDS[] =
{
	0x06F98086, // 400 Series
	0x9DF98086, // 300 Series / C240 Series
	0xA3798086, // 300 Series / C240 Series
	0x02F98086, // 300 Series CNL-LP PCH
	0xA2B18086, // 200 Series / X299 / Z370 Series
	0xA1318086, // 100 Series / C230 Series
	0x9D318086, // 100 Series Skylake PCH
	0x3A328086, // 9 Series / 8 Series / C220 Series
	0x9CA48086, // 9 Series Wildcat Point
	0x1C248086, // 7 Series / C216 Series / 6 Series / C200 Series
	0x3B328086, // 5 Series / 3400 Series
	0x9C248086, // Haswell PCH
	0x8C248086, // Haswell PCH
	0xA1B18086, // C620 Series Lewisburg PCH
	0x8D248086, // X99 Wellsburg
};

#define BAR_REG_LOW     0x10
#define BAR_REG_HIGH    0x14

static bool detect_pch_thermal(void)
{
	uint32_t pci_addr = WR0_FindPciByClass(NWLC->NwDrv, 0x11, 0x80, 0, 0);
	if (pci_addr == 0xFFFFFFFF)
		return false;
	uint32_t id = WR0_RdPciConf32(NWLC->NwDrv, pci_addr, 0);
	for (size_t i = 0; i < ARRAYSIZE(INTEL_THERMAL_IDS); i++)
	{
		if (id == INTEL_THERMAL_IDS[i])
		{
			NWL_Debug("PCH", "Found Thermal Sensor with ID %08X", id);

			uint64_t mmio_reg = 0;
			mmio_reg = WR0_RdPciConf32(NWLC->NwDrv, pci_addr, BAR_REG_LOW);
			if (mmio_reg == 0xFFFFFFFF)
				return false;
			mmio_reg &= 0xFFFFF000;

			uint64_t mmio_reg_high = WR0_RdPciConf32(NWLC->NwDrv, pci_addr, BAR_REG_HIGH);
			if (mmio_reg_high == 0xFFFFFFFF)
				return false;
			mmio_reg |= mmio_reg_high << 32;
			ctx.pch_base = mmio_reg;
			return true;
		}
	}
	return false;
}

bool
mchbar_pch_init(void)
{
	struct system_id_t* id = NWL_GetCpuid();

	if (ctx.init_done)
		return true;

	if (!NWLC->NwDrv || NWLC->NwDrv->type == WR0_DRIVER_PAWNIO)
		goto fail;

	if (!id)
		goto fail;

	ctx.id = &id->cpu_types[0];
	if (ctx.id->vendor != VENDOR_INTEL)
		goto fail;

	if (ctx.id->x86.family != 0x06)
		goto fail;
	switch (ctx.id->x86.ext_model)
	{
	case 0x2A: // Core 2nd Gen (Sandy Bridge)
	case 0x3A: // Core 3rd Gen (Ivy Bridge)
		ctx.mchbar_base = mchbar_get_base_32();
		detect_pch_thermal();
		break;
	case 0x3C: // Core 4th Gen (Haswell)
	case 0x3F: // Core 4th Gen (Haswell-EP)
	case 0x45: // Core 4th Gen (Haswell)
	case 0x46: // Core 4th Gen (Haswell)
	case 0x47: // Core 5th Gen (Broadwell)
	case 0x4E: // Core 6th Gen (Sky Lake)
	case 0x5E: // Core 6th Gen (Sky Lake)
	case 0x8E: // Core 7/8/9th Gen (Kaby/Coffee Lake)
	case 0x9E: // Core 7/8/9th Gen (Kaby/Coffee Lake)
		ctx.mchbar_base = mchbar_get_base_64(0x7FFFFF8000); // 38:15
		detect_pch_thermal();
		break;
	case 0x7E: // Core 10th Gen (Ice Lake) // ???
	case 0xA5: // Core 10th Gen (Comet Lake)
	case 0xA6: // Core 10th Gen (Comet Lake)
	case 0xA7: // Core 11th Gen (Rocket Lake)
	case 0x8A: // Core 11th Gen (Lakefield) // ???
	case 0x9C: // Core 11th Gen (Jasper Lake) // ???
		ctx.mchbar_base = mchbar_get_base_64(0x7FFFFF0000); // 38:16
		detect_pch_thermal();
		break;
	case 0x8C: // Core 11th Gen (Tiger Lake-U)
	case 0x8D: // Core 11th Gen (Tiger Lake-H)
		ctx.mchbar_base = mchbar_get_base_64(0x7FFFFE0000); // 38:17
		ctx.pch_base = PWRMBASE_DEFAULT;
		break;
	case 0x97: // Core 12th Gen (Alder Lake)
	case 0x9A: // Core 12th Gen (Alder Lake)
	case 0xB7: // Core 13th/14th Gen (Raptor Lake)
	case 0xBA: // Core 13th/14th Gen (Raptor Lake)
	case 0xBE: // Core 13th/14th Gen (Raptor Lake)
	case 0xBF: // Core 13th/14th Gen (Raptor Lake)
	case 0xAA: // Ultra (Meteor Lake)
	case 0xAB: // Ultra (Meteor Lake)
	case 0xAC: // Ultra (Meteor Lake)
	case 0xB5: // Ultra (Arrow Lake-U)
	case 0xBD: // Ultra (Lunar Lake-V)
	case 0xC5: // Ultra (Arrow Lake-H)
	case 0xC6: // Ultra (Arrow Lake-S)
	case 0xCC: // Ultra (Panther Lake)
		ctx.mchbar_base = mchbar_get_base_64(0x3FFFFFE0000); // 41:17
		ctx.pch_base = PWRMBASE_DEFAULT;
		break;
	}
	NWL_Debug("MCHBAR", "MMIO REG %llX", ctx.mchbar_base);
	NWL_Debug("PCH", "MMIO REG %llX", ctx.pch_base);

	ctx.init_done = true;
	return true;

fail:
	ZeroMemory(&ctx, sizeof(ctx));
	return false;
}
