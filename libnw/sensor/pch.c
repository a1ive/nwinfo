// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"
#include "sensors.h"
#include "libcpuid.h"
#include "ioctl.h"

// 900 Series
//   NO DATA
// 800 Series
//   D31:F2 - PMC
//     DID 7F21
// 700 Series
//   D31:F2 - PMC
//     DID 7A21
// 600 Series
//   D31:F2 - PMC
//     DID 7AA1
// 500 Series
//   D31:F2 - PMC
//     DID 43A1
// 400 Series
//   D18:F0 - Thermal Subsystem (CLASS 1180)
//     DID 06F9
// 300 Series / C240 Series
//   D18:F0 - Thermal Subsystem (CLASS 1180)
//     DID 9DF9
//     DID A379
// 200 Series / X299 / Z370 Series
//   D20:F2 - Thermal Subsystem (CLASS 1180)
//     DID A2B1
// 100 Series / C230 Series
//   D20:F2 - Thermal Subsystem (CLASS 1180)
//     DID A131
// 9 Series
//   D31:F6 - Thermal Sensor (CLASS 1180)
//     DID 3A32
// 8 Series / C220 Series
//   D31:F6 - Thermal Sensor (CLASS 1180)
//     DID 3A32
// 7 Series / C216 Series
//   D31:F6 - Thermal Sensor (CLASS 1180)
//     DID 1C24
// 6 Series / C200 Series
//   D31:F6 - Thermal Sensor (CLASS 1180)
//     DID 1C24
// 5 Series / 3400 Series
//   D31:F6 - Thermal Sensor (CLASS 1180)
//     DID 3B32h

// PMC (D31:F2)
//   10h-13h PWRMBASE (BAR)
//     Base Address for MMIO Registers.
//     31:13 BAR (BASEADDR): Software programs this register with the base address of the device's memory region
//     12:4  Size Indicator (SIZEINDICATOR)
//      3    Prefetchable (PREFETCHABLE)
//      2:1  Type (TYPE)
//      0    Memory Space Indicator (MESSAGE_SPACE)
//   14h-17h PWRMBASE HIGH (BAR_HIGH)
//     Base Address High for MMIO Registers.
//     31:0  Base Address HIGH (BASEADDR_HIGH): Base Address
//   PWRMBASE+1560h Temperature Sensor Control and Status (TSS0)
//     31    Policy Lock-Down Bit (TSS0LOCK)
//     16    TS MASK for MAXTEMP calculation (TSMASKEN)
//      9    TS Reading Valid (TSRV): This bit indicates if the TS die temperature reported in valid or not.
//      8:0  TS Reading (TSR): The TS die temperature with resolution of 1oC in S9.8.0 2s
//                             complement format 0x001 positive 1oC 0x000 0oC 0x1FF negative 1oC 0x1D8
//                             negative 40oC and so on

// Thermal Subsystem (D18:F0) / (D20:F2)
// Thermal Sensor Registers (D31:F6)
//   10h-13h Thermal Base (TBAR)
//     31:12 Thermal Base Address (TBA): Base address for the Thermal logic memory mapped configuration registers.
//      3    Prefetchable (PREF)
//      2:1  Address Range (ADDRNG)
//      0    Space Type (SPTYP)
//   14h-17h Thermal Base High DWord(TBARH)
//     31:0  Thermal Base Address High (TBAH): TBAR bits 61:32.
//   TBAR+0h Temperature (TEMP)
//      8:0  TS Reading (TSR): The die temperature with resolution of 0.5
//                             degree C and an offset of - 50C.Thus a reading of 0x121 is 94.5C.

static struct
{
	uint64_t mmio_reg;
	uint32_t pci_addr;
	uint32_t mmio_lo_mask;
	int (*get) (void);
} ctx;

static inline uint32_t
get_mmio_32(uint64_t offset)
{
	uint32_t data = 0;
	int ret = WR0_RdMmIo(NWLC->NwDrv, ctx.mmio_reg + offset, &data, sizeof(uint32_t));
	if (ret)
		NWL_Debug("PCH", "MMIO 32 @%llX+%llx failed.", ctx.mmio_reg, offset);
	return data;
}

#define BAR_REG_LOW     0x10
#define BAR_REG_HIGH    0x14

static uint64_t
get_mmio_bar(uint32_t addr, uint32_t mask)
{
	uint64_t mmio_reg = 0;
	mmio_reg = WR0_RdPciConf32(NWLC->NwDrv, addr, BAR_REG_LOW);
	if (mmio_reg == 0xFFFFFFFF)
		return 0;
	mmio_reg &= mask;

	uint64_t mmio_reg_high = WR0_RdPciConf32(NWLC->NwDrv, addr, BAR_REG_HIGH);
	if (mmio_reg_high == 0xFFFFFFFF)
		return 0;

	mmio_reg |= mmio_reg_high << 32;
	return mmio_reg;
}

static const uint32_t INTEL_PMC_IDS[] =
{
	0x7F218086, // 800 Series
	0x7A218086, // 700 Series
	0x7AA18086, // 600 Series
	0x43A18086, // 500 Series
};

static const uint32_t INTEL_SMBUS_IDS[] =
{
	0x7F238086, // 800 Series
	0x7A238086, // 700 Series
	0x7AA38086, // 600 Series
	0x43A38086, // 500 Series
};

#define PMC_TSS0 0x1560
#define PWRMBASE_DEFAULT 0xFE000000

static bool detect_pmc(void)
{
	uint32_t pci_addr = PciBusDevFunc(0, 31, 2);
	uint32_t id = WR0_RdPciConf32(NWLC->NwDrv, pci_addr, 0);
	NWL_Debug("PCH", "Found device at D31:F2 with ID %08X", id);
	for (size_t i = 0; i < ARRAYSIZE(INTEL_PMC_IDS); i++)
	{
		if (id == INTEL_PMC_IDS[i])
		{
			NWL_Debug("PCH", "Found PMC with ID %08X", id);
			ctx.pci_addr = pci_addr;
			ctx.mmio_lo_mask = 0xFFFFE000;
			ctx.mmio_reg = get_mmio_bar(pci_addr, ctx.mmio_lo_mask);
			return true;
		}
	}

	pci_addr = PciBusDevFunc(0, 31, 4);
	id = WR0_RdPciConf32(NWLC->NwDrv, pci_addr, 0);
	NWL_Debug("PCH", "Found SMBus at D31:F4 with ID %08X", id);
	for (size_t i = 0; i < ARRAYSIZE(INTEL_SMBUS_IDS); i++)
	{
		if (id == INTEL_SMBUS_IDS[i])
		{
			ctx.mmio_reg = PWRMBASE_DEFAULT;
			break;
		}
	}
	if (!ctx.mmio_reg)
	{
		NWL_Debug("PCH", "PMC not found.");
		return false;
	}

	NWL_Debug("PCH", "PCI access failed, try fallback address");
	uint32_t data = get_mmio_32(PMC_TSS0);
	NWL_Debug("PCH", "Try reading TSS0: %08X", data);
	if (data == 0xFFFFFFFF || !(data & (1 << 9)))
	{
		ctx.mmio_reg = 0;
		return false;
	}
	return true;
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

static bool detect_thermal(void)
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
			ctx.pci_addr = pci_addr;
			ctx.mmio_lo_mask = 0xFFFFF000;
			ctx.mmio_reg = get_mmio_bar(pci_addr, ctx.mmio_lo_mask);
			return true;
		}
	}
	return false;
}

static int get_pmc_temperature(void)
{
	if (!ctx.mmio_reg)
		return 0;
	uint32_t tss0 = get_mmio_32(PMC_TSS0);
	if (!(tss0 & (1 << 9))) // TS Reading Valid
		return 0;
	// TSR (bits 8:0)
	int tsr = tss0 & 0x1FF;
	if (tsr & 0x100)
		tsr -= 0x200;
	return tsr;
}

static int get_ts_temperature(void)
{
	if (!ctx.mmio_reg)
		return 0;
	uint32_t temp = get_mmio_32(0x0);
	// TSR (bits 8:0): resolution 0.5C, offset -50C
	int tsr = temp & 0x1FF;
	return tsr / 2 - 50;
}

static bool pch_init(void)
{
	if (!NWLC->NwDrv)
		goto fail;

	if (detect_pmc())
	{
		ctx.get = get_pmc_temperature;
	}
	else if (detect_thermal())
	{
		ctx.get = get_ts_temperature;
	}
	else
	{
		NWL_Debug("PCH", "No supported device found.");
		goto fail;
	}

	NWL_Debug("PCH", "MMIO REG %llX", ctx.mmio_reg);
	if (!ctx.mmio_reg || !ctx.get)
		goto fail;

	return true;
fail:
	ZeroMemory(&ctx, sizeof(ctx));
	return false;
}

static void pch_fini(void)
{
	ZeroMemory(&ctx, sizeof(ctx));
}

static void pch_get(PNODE node)
{
	NWL_NodeAttrSetf(node, "Temperature", NAFLG_FMT_NUMERIC, "%d", ctx.get());
}

sensor_t sensor_pch =
{
	.name = "PCH",
	.flag = NWL_SENSOR_PCH,
	.init = pch_init,
	.get = pch_get,
	.fini = pch_fini,
};
