// SPDX-License-Identifier: Unlicense

#include "../smbus.h"
#include <windows.h>
#include <stdio.h>

#define PCI_VID_INTEL   0x8086

#define SMBHSTSTS       (0U + ctx->base_addr)
#define SMBHSTCNT       (2U + ctx->base_addr)
#define SMBHSTCMD       (3U + ctx->base_addr)
#define SMBHSTADD       (4U + ctx->base_addr)
#define SMBHSTDAT0      (5U + ctx->base_addr)
#define SMBHSTDAT1      (6U + ctx->base_addr)
#define SMBBLKDAT       (7U + ctx->base_addr)
#define SMBPEC          (8U + ctx->base_addr)
#define SMBAUXSTS       (12U + ctx->base_addr)
#define SMBAUXCTL       (13U + ctx->base_addr)
#define SMBSLVSTS       (16U + ctx->base_addr)
#define SMBSLVCMD       (17U + ctx->base_addr)
#define SMBNTFDADD      (20U + ctx->base_addr)

#define PCICMD          0x004
#define SMB_BASE        0x020
#define SMBHSTCFG       0x040

#define PCICMD_IOBIT    0x01

#define SMBHSTCFG_HST_EN        (1 << 0)
#define SMBHSTCFG_SMB_SMI_EN    (1 << 1)
#define SMBHSTCFG_I2C_EN        (1 << 2)
#define SMBHSTCFG_SPD_WD        (1 << 4)

#define SMBAUXSTS_CRCE          (1 << 0)
#define SMBAUXSTS_STCO          (1 << 1)

#define SMBAUXCTL_CRC           (1 << 0)
#define SMBAUXCTL_E32B          (1 << 1)

#define I801_QUICK              0x00
#define I801_BYTE               0x04
#define I801_BYTE_DATA          0x08
#define I801_WORD_DATA          0x0C
#define I801_PROC_CALL          0x10
#define I801_BLOCK_DATA         0x14
#define I801_I2C_BLOCK_DATA     0x18
#define I801_BLOCK_PROC_CALL    0x1C

#define SMBHSTCNT_INTREN        (1 << 0)
#define SMBHSTCNT_KILL          (1 << 1)
#define SMBHSTCNT_LAST_BYTE     (1 << 5)
#define SMBHSTCNT_START         (1 << 6)
#define SMBHSTCNT_PEC_EN        (1 << 7)

#define SMBHSTSTS_HOST_BUSY     (1 << 0)
#define SMBHSTSTS_INTR          (1 << 1)
#define SMBHSTSTS_DEV_ERR       (1 << 2)
#define SMBHSTSTS_BUS_ERR       (1 << 3)
#define SMBHSTSTS_FAILED        (1 << 4)
#define SMBHSTSTS_SMBALERT_STS  (1 << 5)
#define SMBHSTSTS_INUSE_STS     (1 << 6)
#define SMBHSTSTS_BYTE_DONE     (1 << 7)

#define STATUS_ERROR_FLAGS \
	(SMBHSTSTS_FAILED | SMBHSTSTS_BUS_ERR | SMBHSTSTS_DEV_ERR)

#define STATUS_FLAGS \
	(SMBHSTSTS_BYTE_DONE | SMBHSTSTS_INTR | STATUS_ERROR_FLAGS)

#define SMBUS_LEN_SENTINEL (I2C_SMBUS_BLOCK_MAX + 1)

// A 32 byte block process at 10MHz is 62.3ms, 80ms should be plenty
#define MAX_TIMEOUT 80
#define MAX_RETRIES 320

static const struct
{
	uint16_t did;
	bool pec;
	bool blk;
} i801_devices[] =
{
	{ 0x2413, false, false },   // 82801AA (ICH)
	{ 0x2423, false, false },   // 82801AB (ICH0)
	{ 0x2443, false, false },   // 82801BA (ICH2)
	{ 0x2483, false, false },   // 82801CA (ICH3)
	{ 0x24c3, true , false },   // 82801DB (ICH4)
	{ 0x24d3, true , true  },   // 82801E (ICH5)
	{ 0x25a4, true , true  },   // 6300ESB
	{ 0x266a, true , true  },   // 82801F (ICH6)
	{ 0x269b, true , true  },   // 6310ESB/6320ESB
	{ 0x27da, true , true  },   // 82801G (ICH7)
	{ 0x283e, true , true  },   // 82801H (ICH8)
	{ 0x2930, true , true  },   // 82801I (ICH9)
	{ 0x5032, true , true  },   // EP80579 (Tolapai)
	{ 0x3a30, true , true  },   // ICH10
	{ 0x3a60, true , true  },   // ICH10
	{ 0x3b30, true , true  },   // 5/3400 Series (PCH)
	{ 0x1c22, true , true  },   // 6 Series (PCH)
	{ 0x1d22, true , true  },   // Patsburg (PCH)
	{ 0x1d70, true , true  },   // Patsburg (PCH) IDF
	{ 0x1d71, true , true  },   // Patsburg (PCH) IDF
	{ 0x1d72, true , true  },   // Patsburg (PCH) IDF
	{ 0x2330, true , true  },   // DH89xxCC (PCH)
	{ 0x1e22, true , true  },   // Panther Point (PCH)
	{ 0x8c22, true , true  },   // Lynx Point (PCH)
	{ 0x9c22, true , true  },   // Lynx Point-LP (PCH)
	{ 0x1f3c, true , true  },   // Avoton (SOC)
	{ 0x8d22, true , true  },   // Wellsburg (PCH)
	{ 0x8d7d, true , true  },   // Wellsburg (PCH) MS
	{ 0x8d7e, true , true  },   // Wellsburg (PCH) MS
	{ 0x8d7f, true , true  },   // Wellsburg (PCH) MS
	{ 0x23b0, true , true  },   // Coleto Creek (PCH)
	{ 0x8ca2, true , true  },   // Wildcat Point (PCH)
	{ 0x9ca2, true , true  },   // Wildcat Point-LP (PCH)
	{ 0x0f12, true , true  },   // BayTrail (SOC)
	{ 0x2292, true , true  },   // Braswell (SOC)
	{ 0xa123, true , true  },   // Sunrise Point-H (PCH)
	{ 0x9d23, true , true  },   // Sunrise Point-LP (PCH)
	{ 0x19df, true , true  },   // DNV (SOC)
	{ 0x1bc9, true , true  },   // Emmitsburg (PCH)
	{ 0x5ad4, true , true  },   // Broxton (SOC)
	{ 0xa1a3, true , true  },   // Lewisburg (PCH)
	{ 0xa223, true , true  },   // Lewisburg Supersku (PCH)
	{ 0xa2a3, true , true  },   // Kaby Lake PCH-H (PCH)
	{ 0x31d4, true , true  },   // Gemini Lake (SOC)
	{ 0xa323, true , true  },   // Cannon Lake-H (PCH)
	{ 0x9da3, true , true  },   // Cannon Lake-LP (PCH)
	{ 0x18df, true , true  },   // Cedar Fork (PCH)
	{ 0x34a3, true , true  },   // Ice Lake-LP (PCH)
	{ 0x38a3, true , true  },   // Ice Lake-N (PCH)
	{ 0x02a3, true , true  },   // Comet Lake (PCH)
	{ 0x06a3, true , true  },   // Comet Lake-H (PCH)
	{ 0x4b23, true , true  },   // Elkhart Lake (PCH)
	{ 0xa0a3, true , true  },   // Tiger Lake-LP (PCH)
	{ 0x43a3, true , true  },   // Tiger Lake-H (PCH)
	{ 0x4da3, true , true  },   // Jasper Lake (SOC)
	{ 0xa3a3, true , true  },   // Comet Lake-V (PCH)
	{ 0x7aa3, true , true  },   // Alder Lake-S (PCH)
	{ 0x51a3, true , true  },   // Alder Lake-P (PCH)
	{ 0x54a3, true , true  },   // Alder Lake-M (PCH)
	{ 0x7a23, true , true  },   // Raptor Lake-S (PCH)
	{ 0x7e22, true , true  },   // Meteor Lake-P (SOC)
	{ 0xae22, true , true  },   // Meteor Lake SoC-S (SOC)
	{ 0x7f23, true , true  },   // Meteor Lake PCH-S (PCH)
	{ 0x5796, true , true  },   // Birch Stream (SOC)
	{ 0x7722, true , true  },   // Arrow Lake-H (SOC)
	{ 0xe322, true , true  },   // Panther Lake-H (SOC)
	{ 0xe422, true , true  },   // Panther Lake-P (SOC)
};

static int I801Detect(smbus_t* ctx)
{
	bool found = false;

	if (ctx->drv->driver_type == WR0_DRIVER_PAWNIO)
	{
		ULONG64 out[3];
		if (WR0_ExecPawn(ctx->drv, &ctx->drv->pio_smi801, "ioctl_identity", NULL, 0, out, 3, NULL))
			return SM_ERR_NO_DEVICE;
		SMBUS_DBG("I/O Base %llx", out[1]);
		if (out[1] == 0)
			return SM_ERR_NO_DEVICE;
		ctx->pci_addr = (uint32_t)out[1];
		return SM_OK;
	}

	if ((ctx->pci_id & 0xFFFF) != PCI_VID_INTEL)
		return SM_ERR_NO_DEVICE;

	for (int i = 0; i < ARRAYSIZE(i801_devices); i++)
	{
		if (((ctx->pci_id >> 16) & 0xFFFF) == i801_devices[i].did)
		{
			found = true;
			ctx->block_read = i801_devices[i].blk;
			ctx->smbus_pec = i801_devices[i].pec;
			break;
		}
	}

	if (!found)
		return SM_ERR_NO_DEVICE;

	SMBUS_DBG("Found i801 SMBus device at PCI %02X:%02X.%X, ID %04X:%04X REV %02X",
		(ctx->pci_addr >> 16) & 0xFF,
		(ctx->pci_addr >> 11) & 0x1F,
		(ctx->pci_addr >> 8) & 0x07,
		ctx->pci_id & 0xFFFF,
		(ctx->pci_id >> 16) & 0xFFFF,
		ctx->rev_id);

	return SM_OK;
}

static int I801Init(smbus_t* ctx)
{
	if (ctx->drv->driver_type == WR0_DRIVER_PAWNIO)
	{
		ULONG64 out;
		if (WR0_ExecPawn(ctx->drv, &ctx->drv->pio_smi801, "ioctl_write_protection", NULL, 0, &out, 1, NULL))
			return SM_ERR_NO_DEVICE;
		ctx->spd_wd = out;
		ctx->block_read = true;
		ctx->smbus_pec = true;
		return SM_OK;
	}

	// Check SMBus is enabled
	uint8_t hostc = WR0_RdPciConf8(ctx->drv, ctx->pci_addr, SMBHSTCFG);
	if (!(hostc & SMBHSTCFG_HST_EN))
	{
		SMBUS_DBG("SMBus is not enabled");
		return SM_ERR_BUS_ERROR;
	}
	ctx->spd_wd = (hostc & SMBHSTCFG_SPD_WD) ? true : false;

	uint32_t base = WR0_RdPciConf32(ctx->drv, ctx->pci_addr, SMB_BASE);
	if (!(base & 0x01))
	{
		SMBUS_DBG("IO mapping not supported");
		return SM_ERR_BUS_ERROR;
	}
	ctx->base_addr = base & 0xffe0;
	return SM_OK;
}

// The i801 SMBus controller use a fixed clock frequency of 100kHz
static uint64_t I801GetClock(smbus_t* ctx)
{
	return 100000;
}

static int I801SetClock(smbus_t* ctx, uint64_t freq)
{
	return SM_ERR_PARAM;
}

static inline uint8_t I801GetBlockLen(smbus_t* ctx)
{
	uint8_t len = WR0_RdIo8(ctx->drv, SMBHSTDAT0);
	if (len < 1 || len > I2C_SMBUS_BLOCK_MAX)
	{
		SMBUS_DBG("Invalid block size %u", len);
		return 0;
	}
	return len;
}

static inline int I801CheckPre(smbus_t* ctx)
{
	uint8_t hststs = WR0_RdIo8(ctx->drv, SMBHSTSTS);
	if (hststs & SMBHSTSTS_HOST_BUSY)
	{
		SMBUS_DBG("I801 SMBus busy");
		return SM_ERR_TIMEOUT;
	}
	hststs &= STATUS_FLAGS;
	if (hststs)
		WR0_WrIo8(ctx->drv, SMBHSTSTS, hststs);
	return SM_OK;
}

static int I801InUse(smbus_t* ctx, bool in_use)
{
	if (in_use)
	{
		uint8_t hststs = WR0_RdIo8(ctx->drv, SMBHSTSTS);
#if 0
		ULONGLONG deadline = GetTickCount64() + MAX_TIMEOUT;
		while ((hststs & SMBHSTSTS_INUSE_STS) && (GetTickCount64() < deadline))
		{
			WR0_MicroSleep(250);
			hststs = WR0_RdIo8(ctx->drv, SMBHSTSTS);
		}
#else
		for (int i = 0; i < MAX_RETRIES && (hststs & SMBHSTSTS_INUSE_STS); i++)
		{
			WR0_MicroSleep(250);
			hststs = WR0_RdIo8(ctx->drv, SMBHSTSTS);
		}
#endif
		if (hststs & SMBHSTSTS_INUSE_STS)
			return SM_ERR_TIMEOUT;
	}
	else
	{
		WR0_WrIo8(ctx->drv, SMBHSTSTS, SMBHSTSTS_INUSE_STS | STATUS_FLAGS);
	}
	return SM_OK;
}

static inline void I801Kill(smbus_t* ctx)
{
	WR0_WrIo8(ctx->drv, SMBHSTCNT, SMBHSTCNT_KILL);
	WR0_MicroSleep(1000);
	WR0_WrIo8(ctx->drv, SMBHSTCNT, 0);

	uint8_t hststs = WR0_RdIo8(ctx->drv, SMBHSTSTS);
	if ((hststs & SMBHSTSTS_HOST_BUSY) || !(hststs & SMBHSTSTS_FAILED))
		SMBUS_DBG("Failed terminating the transaction");
}

static int I801WaitIntr(smbus_t* ctx, uint8_t* hststs, uint32_t size)
{
#if 0
	ULONGLONG deadline = GetTickCount64() + MAX_TIMEOUT;
	WR0_MicroSleep((10 + (9 * size)) * 10);
	do
	{
		WR0_MicroSleep(10);
		*hststs = WR0_RdIo8(ctx->drv, SMBHSTSTS);
		*hststs &= STATUS_ERROR_FLAGS | SMBHSTSTS_INTR;
		if (*hststs && !(*hststs & SMBHSTSTS_HOST_BUSY))
		{
			*hststs &= STATUS_ERROR_FLAGS;
			return SM_OK;
		}
	} while (((*hststs & SMBHSTSTS_HOST_BUSY) || !(*hststs & (STATUS_ERROR_FLAGS | SMBHSTSTS_INTR))) && (GetTickCount64() < deadline));
#else
	for (int i = 0; i < MAX_RETRIES; i++)
	{
		WR0_MicroSleep(250);
		*hststs = WR0_RdIo8(ctx->drv, SMBHSTSTS);
		*hststs &= STATUS_ERROR_FLAGS | SMBHSTSTS_INTR;
		if (*hststs && !(*hststs & SMBHSTSTS_HOST_BUSY))
		{
			*hststs &= STATUS_ERROR_FLAGS;
			return SM_OK;
		}
	}
#endif
	if ((*hststs & SMBHSTSTS_HOST_BUSY) || !(*hststs & (STATUS_ERROR_FLAGS | SMBHSTSTS_INTR)))
		return SM_ERR_TIMEOUT;
	*hststs &= (STATUS_ERROR_FLAGS | SMBHSTSTS_INTR);
	return SM_OK;
}

static int I801Transaction(smbus_t* ctx, uint8_t xact, uint8_t* hststs, uint32_t size)
{
	uint8_t old_hstcnt = WR0_RdIo8(ctx->drv, SMBHSTCNT);
	WR0_WrIo8(ctx->drv, SMBHSTCNT, old_hstcnt & (~SMBHSTCNT_INTREN));
	WR0_WrIo8(ctx->drv, SMBHSTCNT, xact | SMBHSTCNT_START);
	int rc = I801WaitIntr(ctx, hststs, size);
	WR0_WrIo8(ctx->drv, SMBHSTCNT, old_hstcnt);
	return rc;
}

static inline void I801SetHstadd(smbus_t* ctx, uint8_t addr, uint8_t read_write)
{
	WR0_WrIo8(ctx->drv, SMBHSTADD, ((addr & 0x7f) << 1) | (read_write & 0x01));
}

static int
I801SimpleTransaction(smbus_t* ctx, uint8_t addr, uint8_t hstcmd, uint8_t read_write, uint8_t protocol, union i2c_smbus_data* data, uint8_t* hststs)
{
	uint8_t xact;
	uint32_t size = protocol;
	switch (protocol)
	{
	case I2C_SMBUS_QUICK:
	{
		I801SetHstadd(ctx, addr, read_write);
		xact = I801_QUICK;
		break;
	}
	case I2C_SMBUS_BYTE:
	{
		I801SetHstadd(ctx, addr, read_write);
		if (read_write == I2C_SMBUS_WRITE)
			WR0_WrIo8(ctx->drv, SMBHSTCMD, hstcmd);
		xact = I801_BYTE;
		break;
	}
	case I2C_SMBUS_BYTE_DATA:
	{
		I801SetHstadd(ctx, addr, read_write);
		if (read_write == I2C_SMBUS_WRITE)
			WR0_WrIo8(ctx->drv, SMBHSTDAT0, data->u8data);
		WR0_WrIo8(ctx->drv, SMBHSTCMD, hstcmd);
		xact = I801_BYTE_DATA;
		size += read_write;
		break;
	}
	case I2C_SMBUS_WORD_DATA:
	{
		I801SetHstadd(ctx, addr, read_write);
		if (read_write == I2C_SMBUS_WRITE)
		{
			WR0_WrIo8(ctx->drv, SMBHSTDAT0, (uint8_t)(data->u16data & 0xff));
			WR0_WrIo8(ctx->drv, SMBHSTDAT1, (uint8_t)((data->u16data & 0xff00) >> 8));
		}
		WR0_WrIo8(ctx->drv, SMBHSTCMD, hstcmd);
		xact = I801_WORD_DATA;
		size += read_write;
		break;
	}
	case I2C_SMBUS_PROC_CALL:
	{
		I801SetHstadd(ctx, addr, read_write);
		WR0_WrIo8(ctx->drv, SMBHSTDAT0, (uint8_t)(data->u16data & 0xff));
		WR0_WrIo8(ctx->drv, SMBHSTDAT1, (uint8_t)((data->u16data & 0xff00) >> 8));
		WR0_WrIo8(ctx->drv, SMBHSTCMD, hstcmd);
		read_write = I2C_SMBUS_READ;
		xact = I801_PROC_CALL;
		break;
	}
	default:
		return SM_ERR_PARAM;
	}

	int rc = I801Transaction(ctx, xact, hststs, size);
	if (rc != SM_OK)
		return rc;
	if (!*hststs && read_write != I2C_SMBUS_WRITE)
	{
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
	}
	return SM_OK;
}

static int I801BlockTransactionByBlock(smbus_t* ctx, uint8_t read_write, uint8_t protocol, union i2c_smbus_data* data, uint8_t* hststs)
{
	uint8_t xact, len, auxctl;
	uint32_t size = 2 + read_write;
	*hststs = 0;

	switch (protocol)
	{
	case I2C_SMBUS_BLOCK_PROC_CALL:
		xact = I801_BLOCK_PROC_CALL;
		break;
	case I2C_SMBUS_BLOCK_DATA:
		xact = I801_BLOCK_DATA;
		break;
	default:
		return SM_ERR_PARAM;
	}

	auxctl = WR0_RdIo8(ctx->drv, SMBAUXCTL);
	WR0_WrIo8(ctx->drv, SMBAUXCTL, auxctl | SMBAUXCTL_E32B);
	if (read_write == I2C_SMBUS_WRITE)
	{
		len = data->block[0];
		size += len;
		WR0_WrIo8(ctx->drv, SMBHSTDAT0, len);
		WR0_RdIo8(ctx->drv, SMBHSTCNT);
		for (uint8_t i = 1; i <= len; i++)
			WR0_WrIo8(ctx->drv, SMBBLKDAT, data->block[i]);
	}

	int rc = I801Transaction(ctx, xact, hststs, size);
	if (rc != SM_OK)
		goto out;
	if (*hststs)
	{
		rc = SM_OK;
		goto out;
	}

	if (read_write == I2C_SMBUS_READ || protocol == I2C_SMBUS_BLOCK_PROC_CALL)
	{
		len = I801GetBlockLen(ctx);
		if (len == 0)
		{
			rc = SM_ERR_PARAM;
			goto out;
		}

		data->block[0] = len;
		WR0_RdIo8(ctx->drv, SMBHSTCNT);
		for (uint8_t i = 1; i <= len; i++)
			data->block[i] = WR0_RdIo8(ctx->drv, SMBBLKDAT);
	}

out:
	auxctl = WR0_RdIo8(ctx->drv, SMBAUXCTL);
	WR0_WrIo8(ctx->drv, SMBAUXCTL, auxctl & (~SMBAUXCTL_E32B));
	return rc;
}

static int
I801BlockTransaction(smbus_t* ctx, uint8_t addr, uint8_t hstcmd, uint8_t read_write, uint8_t protocol, union i2c_smbus_data* data, uint8_t* hststs)
{
	if (data->block[0] < 1 || data->block[0] > I2C_SMBUS_BLOCK_MAX)
		return SM_ERR_PARAM;

	if (protocol == I2C_SMBUS_BLOCK_PROC_CALL)
		/* Needs to be flagged as write transaction */
		I801SetHstadd(ctx, addr, I2C_SMBUS_WRITE);
	else
		I801SetHstadd(ctx, addr, read_write);
	WR0_WrIo8(ctx->drv, SMBHSTCMD, hstcmd);

	return I801BlockTransactionByBlock(ctx, read_write, protocol, data, hststs);
}

static int
I801Xfer(smbus_t* ctx, uint8_t addr, uint8_t read_write, uint8_t command, uint8_t protocol, union i2c_smbus_data* data)
{
	if (ctx->drv->driver_type == WR0_DRIVER_PAWNIO)
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
			if (WR0_ExecPawn(ctx->drv, &ctx->drv->pio_smi801, "ioctl_smbus_xfer", in, 9, out, 5, NULL))
				return SM_ERR_NO_DEVICE;
			break;
		case I2C_SMBUS_BYTE:
		case I2C_SMBUS_BYTE_DATA:
			in[4] = data->u8data;
			if (WR0_ExecPawn(ctx->drv, &ctx->drv->pio_smi801, "ioctl_smbus_xfer", in, 9, out, 5, NULL))
				return SM_ERR_NO_DEVICE;
			data->u8data = (uint8_t)out[0];
			break;
		case I2C_SMBUS_WORD_DATA:
		case I2C_SMBUS_PROC_CALL:
			in[4] = data->u16data;
			if (WR0_ExecPawn(ctx->drv, &ctx->drv->pio_smi801, "ioctl_smbus_xfer", in, 9, out, 5, NULL))
				return SM_ERR_NO_DEVICE;
			data->u16data = (uint8_t)out[0];
			break;
		case I2C_SMBUS_BLOCK_DATA:
		case I2C_SMBUS_BLOCK_PROC_CALL:
			memcpy(&in[4], data->block, I2C_SMBUS_BLOCK_MAX + 1);
			if (WR0_ExecPawn(ctx->drv, &ctx->drv->pio_smi801, "ioctl_smbus_xfer", in, 9, out, 5, NULL))
				return SM_ERR_NO_DEVICE;
			memcpy(data->block, out, I2C_SMBUS_BLOCK_MAX + 1);
			break;
		default:
			return SM_ERR_PARAM;
		}

		return SM_OK;
	}

	uint16_t pci_cmd = WR0_RdPciConf16(ctx->drv, ctx->pci_addr, PCICMD);
	if (!(pci_cmd & PCICMD_IOBIT))
		WR0_WrPciConf16(ctx->drv, ctx->pci_addr, PCICMD, pci_cmd | PCICMD_IOBIT);
	uint8_t hostc = WR0_RdPciConf8(ctx->drv, ctx->pci_addr, SMBHSTCFG);
	if (!(hostc & SMBHSTCFG_I2C_EN))
		WR0_WrPciConf8(ctx->drv, ctx->pci_addr, SMBHSTCFG, hostc | SMBHSTCFG_I2C_EN);

	uint8_t hststs = 0;
	int rc = I801InUse(ctx, true);
	if (rc != SM_OK)
		goto unlock;
	rc = I801CheckPre(ctx);
	if (rc != SM_OK)
		goto unlock;

	uint8_t auxctl = WR0_RdIo8(ctx->drv, SMBAUXCTL);
	WR0_WrIo8(ctx->drv, SMBAUXCTL, auxctl & (~SMBAUXCTL_CRC));

	switch (protocol)
	{
	case I2C_SMBUS_QUICK:
	case I2C_SMBUS_BYTE:
	case I2C_SMBUS_BYTE_DATA:
	case I2C_SMBUS_WORD_DATA:
	case I2C_SMBUS_PROC_CALL:
		rc = I801SimpleTransaction(ctx, addr, command, read_write, protocol, data, &hststs);
		break;
	case I2C_SMBUS_BLOCK_DATA:
	case I2C_SMBUS_BLOCK_PROC_CALL:
		rc = I801BlockTransaction(ctx, addr, command, read_write, protocol, data, &hststs);
		break;
	default:
		rc = SM_ERR_PARAM;
		goto unlock;
	}

	if (rc != SM_OK)
	{
		I801Kill(ctx);
		goto unlock;
	}

	if (hststs & STATUS_ERROR_FLAGS)
	{
		rc = SM_ERR_GENERIC;
		SMBUS_DBG("HSTSTS ERROR %x", hststs);
	}

unlock:
	I801InUse(ctx, false);
	return rc;
}

const smctrl_t i801_controller =
{
	.name = "i801",
	.detect = I801Detect,
	.init = I801Init,
	.get_clock = I801GetClock,
	.set_clock = I801SetClock,
	.xfer = I801Xfer,
};
