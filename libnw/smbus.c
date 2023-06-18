// SPDX-License-Identifier: Unlicense

#include <libcpuid.h>
#include <winring0.h>
#include "libnw.h"
#include "utils.h"
#include "spd.h"
#include "smbios.h"

static unsigned char* spd_raw = NULL;
static uint16_t smbus_vid = 0xFFFF;
static uint16_t smbus_did = 0xFFFF;
static uint32_t smbus_addr = 0xFFFFFFFF;
static uint16_t smbus_base = 0;

static uint8_t get_smbios_memory_type(void)
{
	uint8_t type = 15; // DDR3
	UINT8* p;
	UINT8* end;

	if (!NWLC->NwSmbios)
		NWLC->NwSmbios = NWL_GetSmbios();
	if (!NWLC->NwSmbios)
		return type;
	p = NWLC->NwSmbios->Data;
	end = p + NWLC->NwSmbios->Length;

	while (p < end)
	{
		PSMBIOSHEADER hdr = (PSMBIOSHEADER)p;
		if (hdr->Type == 17 && hdr->Length >= 0x12)
		{
			PMemoryDevice md = (PMemoryDevice)p;
			if (md->MemoryType == 0x1a || md->MemoryType == 0x1e)
				type = 16; // DDR4
			else if (md->MemoryType == 0x22 || md->MemoryType == 0x23)
				type = 18; // DDR5
			break;
		}
		if ((hdr->Type == 127) && (hdr->Length == 4))
			break;
		p += hdr->Length;
		while ((*p++ != 0 || *p++ != 0) && p < end)
			;
	}
	return type;
}

static void usleep(unsigned int usec)
{
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(10 * (__int64)usec);

	timer = CreateWaitableTimerW(NULL, TRUE, NULL);
	if (!timer)
		return;
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
}

static uint16_t piix4_get_smbus_base(uint8_t addr)
{
	uint16_t val = pci_conf_read16(NWLC->NwDrv, smbus_addr, addr);
	if (val == 0xFFFF)
		return 0;
	return (val & 0xFFF0);
}

static uint16_t ichx_get_smbus_base(void)
{
	uint8_t tmp;
	uint16_t val;
	val = pci_conf_read16(NWLC->NwDrv, smbus_addr, 0x04);
	if (!(val & 1))
		pci_conf_write16(NWLC->NwDrv, smbus_addr, 0x04, val | 1);
	val = pci_conf_read16(NWLC->NwDrv, smbus_addr, 0x20);
	if (val == 0xFFFF)
		return 0;
	val &= 0xFFFE;
	tmp = pci_conf_read8(NWLC->NwDrv, smbus_addr, 0x40);
	if ((tmp & 4) == 0)
		pci_conf_write8(NWLC->NwDrv, smbus_addr, 0x40, tmp | 0x04);
	io_outb(NWLC->NwDrv, SMBHSTSTS, io_inb(NWLC->NwDrv, SMBHSTSTS) & 0x1F);
	usleep(1000);
	return val;
}

static uint16_t fch_get_smbus_base(void)
{
	uint16_t pm_reg;
	io_outb(NWLC->NwDrv, AMD_INDEX_IO_PORT, AMD_PM_INDEX + 1);
	pm_reg = io_inb(NWLC->NwDrv, AMD_DATA_IO_PORT) << 8;
	io_outb(NWLC->NwDrv, AMD_INDEX_IO_PORT, AMD_PM_INDEX);
	pm_reg |= io_inb(NWLC->NwDrv, AMD_DATA_IO_PORT);

	if (pm_reg == 0xFFFF)
	{
		if (NWL_ReadMemory(&pm_reg, 0xFED80300, sizeof(pm_reg)) == FALSE)
			return 0;
		return (pm_reg & 0xFF00);
	}
	if ((pm_reg & 0x10) == 0)
		return 0;
	return (pm_reg & 0xFF00);
}

static uint16_t sbx00_get_smbus_base(void)
{
	uint16_t pm_reg;
	io_outb(NWLC->NwDrv, AMD_INDEX_IO_PORT, AMD_SMBUS_BASE_REG + 1);
	pm_reg = io_inb(NWLC->NwDrv, AMD_DATA_IO_PORT) << 8;
	io_outb(NWLC->NwDrv, AMD_INDEX_IO_PORT, AMD_SMBUS_BASE_REG);
	pm_reg |= io_inb(NWLC->NwDrv, AMD_DATA_IO_PORT) & 0xE0;
	if (pm_reg != 0xFFE0)
		return pm_reg;
	return 0;
}

static uint16_t nv_get_smbus_base(void)
{
	uint32_t reg = NV_OLD_SMBUS_ADR_REG;
	if (smbus_did >= 0x0200)
		reg = NV_SMBUS_ADR_REG;
	return (pci_conf_read16(NWLC->NwDrv, smbus_addr, reg) & 0xFFFC);
}

static uint8_t ichx_process(void)
{
	uint8_t status;
	uint16_t i = 0;

	status = io_inb(NWLC->NwDrv, SMBHSTSTS) & 0x1F;

	if (status != 0x00)
	{
		io_outb(NWLC->NwDrv, SMBHSTSTS, status);
		usleep(500);
		status = 0x1F & io_inb(NWLC->NwDrv, SMBHSTSTS);
		if (status != 0x00)
			return 1;
	}

	io_outb(NWLC->NwDrv, SMBHSTCNT,
		io_inb(NWLC->NwDrv, SMBHSTCNT) | SMBHSTCNT_START);

	do
	{
		usleep(500);
		status = io_inb(NWLC->NwDrv, SMBHSTSTS);
	} while ((status & 0x01) && (i++ < 100));

	if (i >= 100)
		return 2;

	if (status & 0x1C)
		return status;

	if ((io_inb(NWLC->NwDrv, SMBHSTSTS) & 0x1F) != 0x00)
		io_outb(NWLC->NwDrv, SMBHSTSTS, io_inb(NWLC->NwDrv, SMBHSTSTS));

	return 0;
}

static void ichx_ddr4_set_page(uint8_t page)
{
	if (page > 1)
		return;
	io_outb(NWLC->NwDrv, SMBHSTADD, 0x6C + (page << 1));
	io_outb(NWLC->NwDrv, SMBHSTCNT, SMBHSTCNT_BYTE_DATA);
	ichx_process();
}

static void ichx_ddr5_set_page(uint8_t index, uint8_t page)
{
	if (page >= 8) // offset = 1024 / 128
		return;
	io_outb(NWLC->NwDrv, SMBHSTADD, index << 1);
	io_outb(NWLC->NwDrv, SMBHSTCMD, SPD5_MR11 & 0x7F);
	io_outb(NWLC->NwDrv, SMBHSTDAT0, page);
	io_outb(NWLC->NwDrv, SMBHSTCNT, SMBHSTCNT_BYTE_DATA);
	ichx_process();
}

static uint8_t ichx_spd_read_byte(uint8_t type, uint8_t index, uint16_t offset)
{
	index += 0x50;

	switch (type)
	{
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 15:
		if (offset > 0xFF)
			return 0xFF;
		break;
	case 12: /* DDR4 */
	case 14:
	case 16:
		if (offset > 0x1FF)
			return 0xFF;
		ichx_ddr4_set_page(offset >> 8);
		offset &= 0xFF;
		break;
	case 18: /* DDR5 */
		ichx_ddr5_set_page(index, offset >> 7);
		offset &= 0x7F;
		offset |= 0x80;
		break;
	default:
		if (offset > 0xFF)
			return 0xFF;
		ichx_ddr4_set_page(0);
		break;
	}

	io_outb(NWLC->NwDrv, SMBHSTADD, (index << 1) | 0x01);
	io_outb(NWLC->NwDrv, SMBHSTCMD, (uint8_t) offset);
	io_outb(NWLC->NwDrv, SMBHSTCNT, SMBHSTCNT_BYTE_DATA);

	if (ichx_process() == 0)
		return io_inb(NWLC->NwDrv, SMBHSTDAT0);
	return 0xFF;
}

static uint8_t nv_spd_read_byte(uint8_t index, uint16_t offset)
{
	int i;
	if (offset > 0xFF)
		return 0xFF;
	index += 0x50;
	io_outb(NWLC->NwDrv, NVSMBADD, index << 1);
	io_outb(NWLC->NwDrv, NVSMBCMD, (uint8_t)offset);
	io_outb(NWLC->NwDrv, NVSMBCNT, NVSMBCNT_BYTE_DATA | NVSMBCNT_READ);
	for (i = 500; i > 0; i--) 
	{
		usleep(50);
		if (io_inb(NWLC->NwDrv, NVSMBCNT) == 0)
			break;
	}
	if (i == 0 || io_inb(NWLC->NwDrv, NVSMBSTS) & NVSMBSTS_STATUS)
		return 0xFF;
	return io_inb(NWLC->NwDrv, NVSMBDAT(0));
}

static uint8_t spd_read_byte(uint8_t type, uint8_t index, uint16_t offset)
{
	switch (smbus_vid)
	{
	case 0x10de: /* nVIDIA */
		return nv_spd_read_byte(index, offset);
	default:
		return ichx_spd_read_byte(type, index, offset);
	}
	return 0xFF;
}

void
NWL_SpdInit(void)
{
	if (NWLC->NwDrv == NULL)
		return;
	spd_raw = malloc(SPD_DATA_LEN);
	if (!spd_raw)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Memory allocation failed in "__FUNCTION__);
		return;
	}

	smbus_addr = pci_find_by_class(NWLC->NwDrv, 0x0c, 0x05, 0x00, 0);
	if (smbus_addr == 0xFFFFFFFF)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "SMBus not found");
		return;
	}
	smbus_vid = pci_conf_read16(NWLC->NwDrv, smbus_addr, 0);
	smbus_did = pci_conf_read16(NWLC->NwDrv, smbus_addr, 2);
	if (smbus_vid == 0xFFFF || smbus_did == 0xFFFF)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "SMBus id read error");
		return;
	}
	switch (smbus_vid)
	{
	case 0x8086: /* Intel */
		switch (smbus_did)
		{
		case 0x7113: /* PIIX4 */
			smbus_base = piix4_get_smbus_base(PIIX4_SMB_BASE_ADR_DEFAULT);
			break;
		default: /* ICHx */
			smbus_base = ichx_get_smbus_base();
			break;
		}
		break;
	case 0x1022: /* AMD */
		switch (smbus_did)
		{
		case 0x746A: /* AMD-8111 */
			smbus_base = sbx00_get_smbus_base();
			break;
		case 0x780B: /* FCH */
		{
			uint8_t rev_id = pci_conf_read8(NWLC->NwDrv, smbus_addr, 0x08);
			if (rev_id == 0x42)
				smbus_base = fch_get_smbus_base();
			else
				smbus_base = sbx00_get_smbus_base();
		}
			break;
		case 0x790B: /* FCH */
			smbus_base = fch_get_smbus_base();
			break;
		default:
			break;
		}
		break;
	case 0x1002: /* ATI */
		switch (smbus_did)
		{
		case 0x4353: /* SB200 */
		case 0x4363: /* SB300 */
		case 0x4372: /* SB4x0 */
			smbus_base = piix4_get_smbus_base(PIIX4_SMB_BASE_ADR_DEFAULT);
			break;
		case 0x4385: /* SBx00 */
		{
			uint8_t rev_id = pci_conf_read8(NWLC->NwDrv, smbus_addr, 0x08);
			if (rev_id <= 0x3D)
				smbus_base = piix4_get_smbus_base(PIIX4_SMB_BASE_ADR_DEFAULT);
			else
				smbus_base = sbx00_get_smbus_base();
		}
			break;
		}
		break;
	case 0x10de: /* nVIDIA */
		smbus_base = nv_get_smbus_base();
		break;
	}
}

static BOOL bytes_need(uint16_t type, uint16_t offset)
{
	if (offset >= 0 && offset <= 18) return TRUE;
	if (type == 18) // DDR5
	{
		if (offset >= 234 && offset <= 235) return TRUE;
		if (offset >= 512 && offset <= 541) return TRUE;
	}
	else if (type == 16 || type == 14 || type == 12) // DDR4
	{
		if (offset >= 320 && offset <= 349) return TRUE;
	}
	else if (type == 15 || type == 11) // DDR3
	{
		if (offset >= 117 && offset <= 148) return TRUE;
	}
	else if (type <= 10) // DDR2, DDR
	{
		if (offset >= 64 && offset <= 100) return TRUE;
	}
	return FALSE;
}

void*
NWL_SpdGet(uint8_t index)
{
	uint8_t spd_type = 0xFF;
	uint16_t i;
	uint16_t spd_size = SPD_DATA_LEN;
	char progress[] = "Reading Slot N (XXX)";
	char num[] = "0123456789ABCDEF";

	if (!spd_raw || index >= 8
		|| smbus_did == 0xFFFF || smbus_vid == 0xFFFF
		|| smbus_addr == 0xFFFFFFFF || smbus_base == 0)
		return NULL;

	spd_type = get_smbios_memory_type();
	spd_type = spd_read_byte(spd_type, index, 2);
	if (spd_type == 0xFF)
		return NULL;
	
	ZeroMemory(spd_raw, SPD_DATA_LEN);

	if (spd_type == 12 || spd_type == 14 || spd_type == 16)
		spd_size = 512;
	else if (spd_type >= 18)
		spd_size = 1024;
	else
		spd_size = 256;

	for (i = 0; i < spd_size; i++)
	{
		if (!bytes_need(spd_type, i))
			continue;
		if (NWLC->SpdProgress)
		{
			progress[13] = num[index];
			progress[16] = num[i >> 8];
			progress[17] = num[(i >> 4) & 0x0F];
			progress[18] = num[i & 0x0F];
			NWLC->SpdProgress(progress);
		}
		spd_raw[i] = spd_read_byte(spd_type, index, i);
	}
	
	return spd_raw;
}

void
NWL_SpdFini(void)
{
	if (spd_raw)
		free(spd_raw);
	spd_raw = NULL;
}
