// SPDX-License-Identifier: Unlicense

#include "smbus.h"
#include <windows.h>
#include <stdio.h>
#include "ioctl.h"

#define PCI_VID_INTEL       0x8086
#define PCI_VID_AMD         0x1022
#define PCI_VID_ATI         0x1002
#define PCI_VID_HYGON       0x1d94
#define PCI_VID_EFAR        0x1055
#define PCI_VID_SERVERWORKS 0x1166
#define PCI_VID_VIA         0x1106

#define PCI_DID_INTEL_82371AB_3 0x7113
#define PCI_DID_INTEL_82443MX_3 0x719B
#define PCI_DID_EFAR_SLC90E66_3 0x9463
#define PCI_DID_ATI_IXP200_SM   0x4353
#define PCI_DID_ATI_IXP300_SM   0x4363
#define PCI_DID_ATI_IXP400_SM   0x4372
#define PCI_DID_ATI_SBX00_SM    0x4385
#define PCI_DID_AMD_HUDSON2_SM  0x780B
#define PCI_DID_AMD_KERNCZ_SM   0x790B
#define PCI_DID_SW_OSB4	        0x0200
#define PCI_DID_SW_CSB5	        0x0201
#define PCI_DID_SW_CSB6         0x0203
#define PCI_DID_SW_HT1000SB     0x0205
#define PCI_DID_SW_HT1100LD     0x0408
#define PCI_DID_VIA_82C686_4    0x3057
#define PCI_DID_VIA_8233_0      0x3074
#define PCI_DID_VIA_8233A       0x3147
#define PCI_DID_VIA_8235        0x3177
#define PCI_DID_VIA_8237        0x3227
#define PCI_DID_VIA_8237S       0x3372

#define SMBHSTSTS       (0U + ctx->base_addr)
#define SMBHSLVSTS      (1U + ctx->base_addr)
#define SMBHSTCNT       (2U + ctx->base_addr)
#define SMBHSTCMD       (3U + ctx->base_addr)
#define SMBHSTADD       (4U + ctx->base_addr)
#define SMBHSTDAT0      (5U + ctx->base_addr)
#define SMBHSTDAT1      (6U + ctx->base_addr)
#define SMBBLKDAT       (7U + ctx->base_addr)
#define SMBSLVCNT       (8U + ctx->base_addr)
#define SMBSHDWCMD      (9U + ctx->base_addr)
#define SMBSLVEVT       (10U + ctx->base_addr)
#define SMBSLVDAT       (12U + ctx->base_addr)
#define SMBTIMING       (14U + ctx->base_addr)

#define PCICMD          0x004

#define PCICMD_IOBIT    0x01

#define PIIX4_QUICK              0x00
#define PIIX4_BYTE               0x04
#define PIIX4_BYTE_DATA          0x08
#define PIIX4_WORD_DATA          0x0C
#define PIIX4_PROC_CALL          0x10
#define PIIX4_BLOCK_DATA         0x14
#define PIIX4_BLOCK_PROC_CALL    0x18

#define SB800_PIIX4_PORT_IDX_KERNCZ         0x02
#define SB800_PIIX4_PORT_IDX_MASK_KERNCZ    0x18
#define SB800_PIIX4_PORT_IDX_SHIFT_KERNCZ   3

#define FCH_PM_BASE              0xFED80300

#define AMD_INDEX_IO_PORT   0xCD6
#define AMD_DATA_IO_PORT    0xCD7
#define AMD_SMBUS_BASE_REG  0x2C
#define AMD_PM_INDEX        0x00

#define PIIX4_SMB_BASE_ADR_DEFAULT  0x90
#define PIIX4_SMB_BASE_ADR_VIAPRO   0xD0

#define SMBUS_LEN_SENTINEL (I2C_SMBUS_BLOCK_MAX + 1)

#define MAX_RETRIES 500

#define SMBHSTSTS_HOST_BUSY     (1 << 0)
#define SMBHSTSTS_INTR          (1 << 1)
#define SMBHSTSTS_DEV_ERR       (1 << 2)
#define SMBHSTSTS_BUS_ERR       (1 << 3)
#define SMBHSTSTS_FAILED        (1 << 4)

#define STATUS_ERROR_FLAGS \
	(SMBHSTSTS_FAILED | SMBHSTSTS_BUS_ERR | SMBHSTSTS_DEV_ERR)

#define SMBHSTCNT_START         (1 << 6)

#define ENABLE_INT9             0

static const struct
{
	uint16_t vid;
	uint16_t did;
} piix4_devices[] =
{
	{ PCI_VID_INTEL, PCI_DID_INTEL_82371AB_3 },
	{ PCI_VID_INTEL, PCI_DID_INTEL_82443MX_3 },
	{ PCI_VID_EFAR, PCI_DID_EFAR_SLC90E66_3 },
	{ PCI_VID_ATI, PCI_DID_ATI_IXP200_SM },
	{ PCI_VID_ATI, PCI_DID_ATI_IXP300_SM },
	{ PCI_VID_ATI, PCI_DID_ATI_IXP400_SM },
	{ PCI_VID_ATI, PCI_DID_ATI_SBX00_SM },
	{ PCI_VID_AMD, PCI_DID_AMD_HUDSON2_SM },
	{ PCI_VID_AMD, PCI_DID_AMD_KERNCZ_SM },
	{ PCI_VID_HYGON, PCI_DID_AMD_KERNCZ_SM },
	{ PCI_VID_SERVERWORKS, PCI_DID_SW_OSB4 },
	{ PCI_VID_SERVERWORKS, PCI_DID_SW_CSB5 },
	{ PCI_VID_SERVERWORKS, PCI_DID_SW_CSB6 },
	{ PCI_VID_SERVERWORKS, PCI_DID_SW_HT1000SB },
	{ PCI_VID_SERVERWORKS, PCI_DID_SW_HT1100LD },
	{ PCI_VID_VIA, PCI_DID_VIA_82C686_4 },
	{ PCI_VID_VIA, PCI_DID_VIA_8233_0 },
	{ PCI_VID_VIA, PCI_DID_VIA_8233A },
	{ PCI_VID_VIA, PCI_DID_VIA_8235 },
	{ PCI_VID_VIA, PCI_DID_VIA_8237 },
	{ PCI_VID_VIA, PCI_DID_VIA_8237S },
};

static struct
{
	uint16_t vid;
	uint16_t did;
	bool notify_imc;
	bool sw_csb5_delay;
	bool use_mmio;
	uint8_t smb_en;
	uint32_t smb_addr;
} piix4_quirks;

static int PIIX4Detect(smbus_t* ctx)
{
	bool found = false;

	if (ctx->drv->type == WR0_DRIVER_PAWNIO)
	{
		ULONG64 in = 0;
		ULONG64 out[3];
		if (WR0_ExecPawn(ctx->drv, &ctx->drv->pio_smpiix4, "ioctl_piix4_port_sel", &in, 1, out, 1, NULL))
		{
			in = 1;
			if (WR0_ExecPawn(ctx->drv, &ctx->drv->pio_smpiix4, "ioctl_piix4_port_sel", &in, 1, out, 1, NULL))
				return SM_ERR_BUS_ERROR;
		}
		SMBUS_DBG("Using port %llu", in);
		if (WR0_ExecPawn(ctx->drv, &ctx->drv->pio_smpiix4, "ioctl_identity", NULL, 0, out, 3, NULL))
			return SM_ERR_NO_DEVICE;
		SMBUS_DBG("I/O Base %llx", out[1]);
		if (out[1] == 0)
			return SM_ERR_NO_DEVICE;
		ctx->pci_addr = (uint32_t)out[1];
		return SM_OK;
	}

	for (int i = 0; i < ARRAYSIZE(piix4_devices); i++)
	{
		uint32_t pci_id = (((uint32_t)piix4_devices[i].did) << 16) | piix4_devices[i].vid;
		if (ctx->pci_id == pci_id)
		{
			found = true;
			break;
		}
	}

	if (!found)
		return SM_ERR_NO_DEVICE;

	SMBUS_DBG("Found PIIX4 SMBus device at PCI %02X:%02X.%X, ID %04X:%04X REV %02X",
		(ctx->pci_addr >> 16) & 0xFF,
		(ctx->pci_addr >> 11) & 0x1F,
		(ctx->pci_addr >> 8) & 0x07,
		ctx->pci_id & 0xFFFF,
		(ctx->pci_id >> 16) & 0xFFFF,
		ctx->rev_id);

	return SM_OK;
}

static int SB800Setup(smbus_t* ctx)
{
	switch (piix4_quirks.did)
	{
	case PCI_DID_AMD_HUDSON2_SM:
		if (ctx->rev_id >= 0x41)
			piix4_quirks.smb_en = AMD_PM_INDEX;
		else
			piix4_quirks.smb_en = AMD_SMBUS_BASE_REG;
		break;
	case PCI_DID_AMD_KERNCZ_SM:
		if (ctx->rev_id >= 0x49)
			piix4_quirks.smb_en = AMD_PM_INDEX;
		else
			piix4_quirks.smb_en = AMD_SMBUS_BASE_REG;
		if (ctx->rev_id >= 0x51)
			piix4_quirks.use_mmio = true;
		break;
	default:
		piix4_quirks.smb_en = AMD_SMBUS_BASE_REG;
		break;
	}

	if (piix4_quirks.use_mmio)
	{
		uint32_t pm_data = 0;
		if (WR0_RdMmIo(ctx->drv, FCH_PM_BASE, &pm_data, sizeof(uint32_t)))
		{
			SMBUS_DBG("Failed to read MMIO space");
			return SM_ERR_BUS_ERROR;
		}
		ctx->base_addr = (uint32_t)(pm_data & 0xFFFF);
	}
	else
	{
		WR0_WrIo8(ctx->drv, AMD_INDEX_IO_PORT, piix4_quirks.smb_en + 1);
		ctx->base_addr = ((uint32_t)WR0_RdIo8(ctx->drv, AMD_DATA_IO_PORT)) << 8;
		WR0_WrIo8(ctx->drv, AMD_INDEX_IO_PORT, piix4_quirks.smb_en);
		ctx->base_addr |= WR0_RdIo8(ctx->drv, AMD_DATA_IO_PORT);
	}

	uint32_t smb_en_status = 0;
	if (piix4_quirks.smb_en == AMD_PM_INDEX)
	{
		smb_en_status = ctx->base_addr & 0x10;
		ctx->base_addr &= 0xFF00;
	}
	else
	{
		smb_en_status = ctx->base_addr & 0x01;
		ctx->base_addr &= 0xFFE0;
	}

	if (!smb_en_status)
	{
		SMBUS_DBG("SMBus is not enabled");
		return SM_ERR_BUS_ERROR;
	}

	return SM_OK;
}

static int PIIX4Setup(smbus_t* ctx)
{
	piix4_quirks.smb_addr = PIIX4_SMB_BASE_ADR_DEFAULT;
	switch (piix4_quirks.did)
	{
	case PCI_DID_SW_CSB5:
		piix4_quirks.sw_csb5_delay = true;
		break;
	case PCI_DID_VIA_8233_0:
	case PCI_DID_VIA_8233A:
	case PCI_DID_VIA_8235:
	case PCI_DID_VIA_8237:
	case PCI_DID_VIA_8237S:
		piix4_quirks.smb_addr = PIIX4_SMB_BASE_ADR_VIAPRO;
		break;
	default:
		break;
	}

	ctx->base_addr = WR0_RdPciConf16(ctx->drv, ctx->pci_addr, piix4_quirks.smb_addr);
	ctx->base_addr &= 0xFFF0;

	return SM_OK;
}

static int PIIX4Init(smbus_t* ctx)
{
	int rc = SM_ERR_GENERIC;
	ctx->spd_wd = false;
	ZeroMemory(&piix4_quirks, sizeof(piix4_quirks));
	if (ctx->drv->type == WR0_DRIVER_PAWNIO)
	{
		return SM_OK;
	}

	piix4_quirks.vid = ctx->pci_id & 0xFFFF;
	piix4_quirks.did = (ctx->pci_id >> 16) & 0xFFFF;

	switch (piix4_quirks.vid)
	{
	case PCI_VID_AMD:
	case PCI_VID_HYGON:
		if (piix4_quirks.did == PCI_DID_AMD_KERNCZ_SM)
		{
			uint8_t imc = WR0_RdPciConf8(ctx->drv, PciBusDevFunc(0, 0x14, 3), 0x40);
			if (imc & 0x80)
				piix4_quirks.notify_imc = true;
		}
		rc = SB800Setup(ctx);
		break;
	case PCI_VID_ATI:
		if (piix4_quirks.did == PCI_DID_ATI_SBX00_SM && ctx->rev_id >= 0x40)
			rc = SB800Setup(ctx);
		else
			rc = PIIX4Setup(ctx);
		break;
	case PCI_VID_INTEL:
	case PCI_VID_EFAR:
	case PCI_VID_SERVERWORKS:
		rc = PIIX4Setup(ctx);
		break;
	default:
		SMBUS_DBG("Unsupported PIIX4 SMBus");
		return SM_ERR_NO_DEVICE;
	}

	if (rc != SM_OK)
		return rc;

	if (!ctx->base_addr)
	{
		SMBUS_DBG("Invalid SMBus base address");
		return SM_ERR_BUS_ERROR;
	}

	return SM_OK;
}

#ifdef LIBNW_SMBUS_CLOCK
static uint64_t PIIX4GetClock(smbus_t* ctx)
{
	return (66ULL * 1000000) / (WR0_RdIo8(ctx->drv, SMBTIMING) * 4);
}

static int PIIX4SetClock(smbus_t* ctx, uint64_t freq)
{
	if (freq < 10000 || freq > 1000000)
		return SM_ERR_PARAM;
	WR0_WrIo8(ctx->drv, SMBTIMING, (uint8_t)((66ULL * 1000000) / freq / 4));
	return SM_OK;
}
#endif

static inline int PIIX4CheckBusy(smbus_t* ctx)
{
	uint8_t hststs = WR0_RdIo8(ctx->drv, SMBHSTSTS);
	if (hststs == 0x00)
		return SM_OK;

	WR0_WrIo8(ctx->drv, SMBHSTSTS, hststs);
	hststs = WR0_RdIo8(ctx->drv, SMBHSTSTS);
	if (hststs == 0x00)
		return SM_OK;
	return SM_ERR_TIMEOUT;
}

static int PIIX4Transaction(smbus_t* ctx)
{
	uint8_t hststs = 0;
	uint8_t hstcnt = WR0_RdIo8(ctx->drv, SMBHSTCNT);
	WR0_WrIo8(ctx->drv, SMBHSTCNT, hstcnt | SMBHSTCNT_START);
	if (piix4_quirks.sw_csb5_delay)
		WR0_MicroSleep(2000);
	for (int i = 0; i < MAX_RETRIES; i++)
	{
		hststs = WR0_RdIo8(ctx->drv, SMBHSTSTS);
		if (!(hststs & SMBHSTSTS_HOST_BUSY))
			break;
		WR0_MicroSleep(10);
	}

	if (hststs & SMBHSTSTS_HOST_BUSY)
		return SM_ERR_TIMEOUT;
	if (hststs & STATUS_ERROR_FLAGS)
		return SM_ERR_BUS_ERROR;
	if (!(hststs & SMBHSTSTS_INTR))
		return SM_ERR_NO_DEVICE;

	WR0_WrIo8(ctx->drv, SMBHSTSTS, hststs);
	if (WR0_RdIo8(ctx->drv, SMBHSTSTS) != 0x00)
		SMBUS_DBG("Failed reset at end of transaction");
	return SM_OK;
}

static int
PIIX4SimpleTransaction(smbus_t* ctx, uint8_t addr, uint8_t hstcmd, uint8_t read_write, uint8_t protocol, union i2c_smbus_data* data)
{
	uint8_t xact;

	int rc = PIIX4CheckBusy(ctx);
	if (rc != SM_OK)
		return rc;

	switch (protocol)
	{
	case I2C_SMBUS_QUICK:
	{
		WR0_WrIo8(ctx->drv, SMBHSTADD, (addr << 1) | read_write);
		xact = PIIX4_QUICK;
		break;
	}
	case I2C_SMBUS_BYTE:
	{
		WR0_WrIo8(ctx->drv, SMBHSTADD, (addr << 1) | read_write);
		if (read_write == I2C_SMBUS_WRITE)
			WR0_WrIo8(ctx->drv, SMBHSTCMD, hstcmd);
		xact = PIIX4_BYTE;
		break;
	}
	case I2C_SMBUS_BYTE_DATA:
	{
		WR0_WrIo8(ctx->drv, SMBHSTADD, (addr << 1) | read_write);
		if (read_write == I2C_SMBUS_WRITE)
			WR0_WrIo8(ctx->drv, SMBHSTDAT0, data->u8data);
		WR0_WrIo8(ctx->drv, SMBHSTCMD, hstcmd);
		xact = PIIX4_BYTE_DATA;
		break;
	}
	case I2C_SMBUS_WORD_DATA:
	{
		WR0_WrIo8(ctx->drv, SMBHSTADD, (addr << 1) | read_write);
		if (read_write == I2C_SMBUS_WRITE)
		{
			WR0_WrIo8(ctx->drv, SMBHSTDAT0, (uint8_t)(data->u16data & 0xff));
			WR0_WrIo8(ctx->drv, SMBHSTDAT1, (uint8_t)((data->u16data & 0xff00) >> 8));
		}
		WR0_WrIo8(ctx->drv, SMBHSTCMD, hstcmd);
		xact = PIIX4_WORD_DATA;
		break;
	}
	default:
		return SM_ERR_PARAM;
	}

	WR0_WrIo8(ctx->drv, SMBHSTCNT, (xact & 0x1C) + (ENABLE_INT9 & 1));

	rc = PIIX4Transaction(ctx);
	if (rc != SM_OK)
		return rc;

	if ((read_write == I2C_SMBUS_WRITE) || (protocol == I2C_SMBUS_BYTE))
		return SM_OK;

	switch (protocol)
	{
	case I2C_SMBUS_BYTE:
	case I2C_SMBUS_BYTE_DATA:
		data->u8data = WR0_RdIo8(ctx->drv, SMBHSTDAT0);
		break;
	case I2C_SMBUS_WORD_DATA:
	case I2C_SMBUS_PROC_CALL:
		data->u16data = WR0_RdIo8(ctx->drv, SMBHSTDAT0);
		data->u16data |= ((uint16_t)WR0_RdIo8(ctx->drv, SMBHSTDAT1)) << 8;
		break;
	}
	return SM_OK;
}

#ifdef LIBNW_SMBUS_BLOCK
static int
PIIX4BlockTransaction(smbus_t* ctx, uint8_t addr, uint8_t hstcmd, uint8_t read_write, uint8_t protocol, union i2c_smbus_data* data)
{
	if (data->block[0] < 1 || data->block[0] > I2C_SMBUS_BLOCK_MAX)
		return SM_ERR_PARAM;

	uint8_t xact;
	int rc = PIIX4CheckBusy(ctx);
	if (rc != SM_OK)
		return rc;

	switch (protocol)
	{
	case I2C_SMBUS_BLOCK_DATA:
	{
		WR0_WrIo8(ctx->drv, SMBHSTADD, (addr << 1) | read_write);
		WR0_WrIo8(ctx->drv, SMBHSTCMD, hstcmd);
		if (read_write == I2C_SMBUS_WRITE)
		{
			WR0_WrIo8(ctx->drv, SMBHSTDAT0, data->block[0]);
			WR0_RdIo8(ctx->drv, SMBHSTCNT);
			for (uint8_t i = 1; i <= data->block[0]; i++)
				WR0_WrIo8(ctx->drv, SMBBLKDAT, data->block[i]);
		}
		xact = PIIX4_BLOCK_DATA;
	}
		break;
	default:
		return SM_ERR_PARAM;
	}

	WR0_WrIo8(ctx->drv, SMBHSTCNT, (xact & 0x1C) + (ENABLE_INT9 & 1));

	rc = PIIX4Transaction(ctx);
	if (rc != SM_OK)
		return rc;

	if (read_write == I2C_SMBUS_WRITE)
		return SM_OK;

	switch (protocol)
	{
	case PIIX4_BLOCK_DATA:
	{
		data->block[0] = WR0_RdIo8(ctx->drv, SMBHSTDAT0);
		if (data->block[0] < 1 || data->block[0] > I2C_SMBUS_BLOCK_MAX)
			return SM_ERR_PARAM;
		WR0_RdIo8(ctx->drv, SMBHSTCNT);
		for (uint8_t i = 1; i <= data->block[0]; i++)
			data->block[i] = WR0_RdIo8(ctx->drv, SMBBLKDAT);
	}
		break;
	}
	return SM_OK;
}
#endif

static int
PIIX4Xfer(smbus_t* ctx, uint8_t addr, uint8_t read_write, uint8_t command, uint8_t protocol, union i2c_smbus_data* data)
{
	if (ctx->drv->type == WR0_DRIVER_PAWNIO)
	{
		ULONG64 in[9] = { 0 };
		ULONG64 out[5] = { 0 };

		in[0] = addr;
		in[1] = read_write;
		in[2] = command;
		in[3] = protocol;

		switch (protocol)
		{
		case I2C_SMBUS_QUICK:
			if (WR0_ExecPawn(ctx->drv, &ctx->drv->pio_smpiix4, "ioctl_smbus_xfer", in, 9, out, 5, NULL))
				return SM_ERR_NO_DEVICE;
			break;
		case I2C_SMBUS_BYTE:
		case I2C_SMBUS_BYTE_DATA:
			in[4] = data->u8data;
			if (WR0_ExecPawn(ctx->drv, &ctx->drv->pio_smpiix4, "ioctl_smbus_xfer", in, 9, out, 5, NULL))
				return SM_ERR_NO_DEVICE;
			data->u8data = (uint8_t)out[0];
			break;
		case I2C_SMBUS_WORD_DATA:
		case I2C_SMBUS_PROC_CALL:
			in[4] = data->u16data;
			if (WR0_ExecPawn(ctx->drv, &ctx->drv->pio_smpiix4, "ioctl_smbus_xfer", in, 9, out, 5, NULL))
				return SM_ERR_NO_DEVICE;
			data->u16data = (uint8_t)out[0];
			break;
#ifdef LIBNW_SMBUS_BLOCK
		case I2C_SMBUS_BLOCK_DATA:
		case I2C_SMBUS_BLOCK_PROC_CALL:
			memcpy(&in[4], data->block, I2C_SMBUS_BLOCK_MAX + 1);
			if (WR0_ExecPawn(ctx->drv, &ctx->drv->pio_smpiix4, "ioctl_smbus_xfer", in, 9, out, 5, NULL))
				return SM_ERR_NO_DEVICE;
			memcpy(data->block, out, I2C_SMBUS_BLOCK_MAX + 1);
			break;
#endif
		default:
			return SM_ERR_PARAM;
		}

		return SM_OK;
	}

	int rc;
	uint16_t pci_cmd = WR0_RdPciConf16(ctx->drv, ctx->pci_addr, PCICMD);
	if (!(pci_cmd & PCICMD_IOBIT))
		WR0_WrPciConf16(ctx->drv, ctx->pci_addr, PCICMD, pci_cmd | PCICMD_IOBIT);

	switch (protocol)
	{
	case I2C_SMBUS_QUICK:
	case I2C_SMBUS_BYTE:
	case I2C_SMBUS_BYTE_DATA:
	case I2C_SMBUS_WORD_DATA:
	case I2C_SMBUS_PROC_CALL:
		rc = PIIX4SimpleTransaction(ctx, addr, command, read_write, protocol, data);
		break;
#ifdef LIBNW_SMBUS_BLOCK
	case I2C_SMBUS_BLOCK_DATA:
	case I2C_SMBUS_BLOCK_PROC_CALL:
		rc = PIIX4BlockTransaction(ctx, addr, command, read_write, protocol, data);
		break;
#endif
	default:
		rc = SM_ERR_PARAM;
		break;
	}

	return rc;
}

const smctrl_t piix4_controller =
{
	.name = "piix4",
	.detect = PIIX4Detect,
	.init = PIIX4Init,
#ifdef LIBNW_SMBUS_CLOCK
	.get_clock = PIIX4GetClock,
	.set_clock = PIIX4SetClock,
#endif
	.xfer = PIIX4Xfer,
};
