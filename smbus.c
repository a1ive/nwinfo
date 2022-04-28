// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "libcpuid.h"
#include "winring0/winring0.h"
#include "nwinfo.h"
#include "spd.h"

#define PCI_CONF_TYPE_NONE 0
#define PCI_CONF_TYPE_1    1
#define PCI_CONF_TYPE_2    2

#define PCI_CLASS_DEVICE      0x0a
#define PCI_CLASS_BRIDGE_HOST 0x0600

static struct msr_driver_t* driver = NULL;
static int smbdev = 0, smbfun = 0;
static unsigned short smbusbase = 0;
static unsigned char* spd_raw = NULL;

#define I2C_WRITE   0
#define I2C_READ    1

#define SMBHSTSTS  smbusbase
#define SMBHSTCNT  smbusbase + 2
#define SMBHSTCMD  smbusbase + 3
#define SMBHSTADD  smbusbase + 4
#define SMBHSTDAT0 smbusbase + 5
#define SMBHSTDAT1 smbusbase + 6 
#define SMBBLKDAT  smbusbase + 7
#define SMBPEC     smbusbase + 8
#define SMBAUXSTS  smbusbase + 12
#define SMBAUXCTL  smbusbase + 13

#define SMBHSTSTS_BYTE_DONE     0x80
#define SMBHSTSTS_INUSE_STS     0x40
#define SMBHSTSTS_SMBALERT_STS  0x20
#define SMBHSTSTS_FAILED        0x10
#define SMBHSTSTS_BUS_ERR       0x08
#define SMBHSTSTS_DEV_ERR       0x04
#define SMBHSTSTS_INTR          0x02
#define SMBHSTSTS_HOST_BUSY     0x01

#define SMBHSTCNT_QUICK             0x00
#define SMBHSTCNT_BYTE              0x04
#define SMBHSTCNT_BYTE_DATA         0x08
#define SMBHSTCNT_WORD_DATA         0x0C
#define SMBHSTCNT_BLOCK_DATA        0x14
#define SMBHSTCNT_I2C_BLOCK_DATA    0x18
#define SMBHSTCNT_LAST_BYTE         0x20
#define SMBHSTCNT_START             0x40

#define SPD5_MR11 11

static unsigned char pci_conf_type = PCI_CONF_TYPE_NONE;

#define PCI_CONF1_ADDRESS(bus, dev, fn, reg) \
	(0x80000000 | (bus << 16) | (dev << 11) | (fn << 8) | (reg & ~3))

#define PCI_CONF2_ADDRESS(dev, reg)	(unsigned short)(0xC000 | (dev << 8) | reg)

#define PCI_CONF3_ADDRESS(bus, dev, fn, reg) \
	(0x80000000 | (((reg >> 8) & 0xF) << 24) | (bus << 16) | ((dev & 0x1F) << 11) | (fn << 8) | (reg & 0xFF))

static void usleep(unsigned int usec)
{
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(10 * (__int64)usec);

	timer = CreateWaitableTimerA(NULL, TRUE, NULL);
	if (!timer)
		return;
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
}

static int
pci_conf_read(unsigned bus, unsigned dev, unsigned fn, unsigned reg, unsigned len, unsigned long* value)
{
	int result;

	if (!value || (bus > 255) || (dev > 31) || (fn > 7) ||
		(reg > 255 && pci_conf_type != PCI_CONF_TYPE_1))
		return -1;
	result = -2;
	switch (pci_conf_type) {
	case PCI_CONF_TYPE_1:
		if (reg < 256) {
			io_outl(driver, 0xCF8, PCI_CONF1_ADDRESS(bus, dev, fn, reg));
		}
		else {
			io_outl(driver, 0xCF8, PCI_CONF3_ADDRESS(bus, dev, fn, reg));
		}
		switch (len) {
		case 1:
			*value = io_inb(driver, 0xCFC + (reg & 3));
			result = 0;
			break;
		case 2:
			*value = io_inw(driver, 0xCFC + (reg & 2));
			result = 0;
			break;
		case 4:
			*value = io_inl(driver, 0xCFC);
			result = 0;
			break;
		}
		break;
	case PCI_CONF_TYPE_2:
		io_outb(driver, 0xCF8, 0xF0 | (fn << 1));
		io_outb(driver, 0xCFA, bus);

		switch (len) {
		case 1:
			*value = io_inb(driver, PCI_CONF2_ADDRESS(dev, reg));
			result = 0;
			break;
		case 2:
			*value = io_inw(driver, PCI_CONF2_ADDRESS(dev, reg));
			result = 0;
			break;
		case 4:
			*value = io_inl(driver, PCI_CONF2_ADDRESS(dev, reg));
			result = 0;
			break;
		}
		io_outb(driver, 0xCF8, 0);
		break;
	}
	return result;
}

static int
pci_conf_write(unsigned bus, unsigned dev, unsigned fn, unsigned reg, unsigned len, unsigned long value)
{
	int result;

	if (!value || (bus > 255) || (dev > 31) || (fn > 7) ||
		(reg > 255 && pci_conf_type != PCI_CONF_TYPE_1))
		return -1;
	result = -2;
	switch (pci_conf_type)
	{
	case PCI_CONF_TYPE_1:
		if (reg < 256) {
			io_outl(driver, 0xCF8, PCI_CONF1_ADDRESS(bus, dev, fn, reg));
		}
		else {
			io_outl(driver, 0xCF8, PCI_CONF3_ADDRESS(bus, dev, fn, reg));
		}
		switch (len) {
		case 1:
			io_outb(driver, 0xCFC + (reg & 3), (uint8_t)value);
			result = 0;
			break;
		case 2:
			io_outw(driver, 0xCFC + (reg & 2), (uint16_t)value);
			result = 0;
			break;
		case 4:
			io_outl(driver, 0xCFC, (uint32_t)value);
			result = 0;
			break;
		}
		break;
	case PCI_CONF_TYPE_2:
		io_outb(driver, 0xCF8, 0xF0 | (fn << 1));
		io_outb(driver, 0xCFA, bus);

		switch (len) {
		case 1:
			io_outb(driver, PCI_CONF2_ADDRESS(dev, reg), (uint8_t)value);
			result = 0;
			break;
		case 2:
			io_outw(driver, PCI_CONF2_ADDRESS(dev, reg), (uint16_t)value);
			result = 0;
			break;
		case 4:
			io_outl(driver, PCI_CONF2_ADDRESS(dev, reg), (uint32_t)value);
			result = 0;
			break;
		}
		io_outb(driver, 0xCF8, 0);
		break;
	}
	return result;
}

static int
pci_sanity_check(void)
{
	unsigned long value;
	int result;
	result = pci_conf_read(0, 0, 0, PCI_CLASS_DEVICE, 2, &value);
	if (result == 0) {
		result = -1;
		if (value == PCI_CLASS_BRIDGE_HOST)
			result = 0;
	}
	return result;
}

static int
pci_check_direct(void)
{
	unsigned char tmpCFB;
	unsigned int tmpCF8;
	struct cpu_raw_data_t raw = { 0 };
	struct cpu_id_t data = { 0 };

	if (cpuid_get_raw_data(&raw) < 0)
		goto skip_amd;
	if (cpu_identify(&raw, &data) < 0)
		goto skip_amd;
	if (data.vendor_str[0] == 'A' && data.family == 0xF) {
		pci_conf_type = PCI_CONF_TYPE_1;
		return 0;
	}
skip_amd:
	/* Check if configuration type 1 works. */
	pci_conf_type = PCI_CONF_TYPE_1;
	tmpCFB = io_inb(driver, 0xCFB);
	io_outb(driver, 0xCFB, 0x01);
	tmpCF8 = io_inl(driver, 0xCF8);
	io_outl(driver, 0xCF8, 0x80000000);
	if ((io_inl(driver, 0xCF8) == 0x80000000) && (pci_sanity_check() == 0)) {
		io_outl(driver, 0xCF8, tmpCF8);
		io_outb(driver, 0xCFB, tmpCFB);
		return 0;
	}
	io_outl(driver, 0xCF8, tmpCF8);
	/* Check if configuration type 2 works. */
	pci_conf_type = PCI_CONF_TYPE_2;
	io_outb(driver, 0xCFB, 0x00);
	io_outb(driver, 0xCF8, 0x00);
	io_outb(driver, 0xCFA, 0x00);
	if (io_inb(driver, 0xCF8) == 0x00 && io_inb(driver, 0xCFA) == 0x00 && (pci_sanity_check() == 0)) {
		io_outb(driver, 0xCFB, tmpCFB);
		return 0;
	}

	io_outb(driver, 0xCFB, tmpCFB);

	/* Nothing worked return an error */
	pci_conf_type = PCI_CONF_TYPE_NONE;
	return -1;
}

static void ich5_get_smb(void)
{
	unsigned long x = 0;
	unsigned long tmp = 0;
	int res;
	smbusbase = 0;
	res = pci_conf_read(0, smbdev, smbfun, 0x20, 2, &x);
	if (res != 0)
		return;
	smbusbase = (unsigned short)x & 0xFFFE;
	res = pci_conf_read(0, smbdev, smbfun, 0x40, 1, &tmp);
	if (res == 0 && (tmp & 4) == 0)
		res = pci_conf_write(0, smbdev, smbfun, 0x40, 1, tmp | 0x04);
	if (res != 0)
		return;
	io_outb(driver, SMBHSTSTS, io_inb(driver, SMBHSTSTS) & 0x1F);
	usleep(1000);
}

static uint8_t ich5_process(void)
{
	uint8_t status;
	uint16_t timeout = 0;

	status = io_inb(driver, SMBHSTSTS) & 0x1F;

	if (status != 0x00)
	{
		io_outb(driver, SMBHSTSTS, status);
		usleep(500);
		if ((status = (0x1F & io_inb(driver, SMBHSTSTS))) != 0x00)
			return 1;
	}

	io_outb(driver, SMBHSTCNT,
		io_inb(driver, SMBHSTCNT) | SMBHSTCNT_START);

	do
	{
		usleep(500);
		status = io_inb(driver, SMBHSTSTS);
	} while ((status & 0x01) && (timeout++ < 100));

	if (timeout >= 100)
		return 2;

	if (status & 0x1C)
		return status;

	if ((io_inb(driver, SMBHSTSTS) & 0x1F) != 0x00)
		io_outb(driver, SMBHSTSTS, io_inb(driver, SMBHSTSTS));

	return 0;
}

#if 0
static int ich5_smb_check(unsigned char adr)
{
	io_outb(driver, SMBHSTSTS, 0xff);
	while ((io_inb(driver, SMBHSTSTS) & 0x40) != 0x40);
	io_outb(driver, SMBHSTADD, (adr << 1) | 0x01);
	io_outb(driver, SMBHSTCMD, 0x00);
	io_outb(driver, SMBHSTCNT, 0x48);
	while (((io_inb(driver, SMBHSTSTS) & 0x44) != 0x44)
		&& ((io_inb(driver, SMBHSTSTS) & 0x42) != 0x42));
	if ((io_inb(driver, SMBHSTSTS) & 0x44) == 0x44)
		return -1;
	if ((io_inb(driver, SMBHSTSTS) & 0x42) == 0x42)
		return 0;
	return -1;
}
#endif

static unsigned char ich5_smb_read_byte(unsigned char adr, unsigned char cmd)
{
	io_outb(driver, SMBHSTADD, (adr << 1) | I2C_READ);
	io_outb(driver, SMBHSTCMD, cmd);
	io_outb(driver, SMBHSTCNT, SMBHSTCNT_BYTE_DATA);
	if (ich5_process() == 0)
		return io_inb(driver, SMBHSTDAT0);
	else
		return 0xFF;
}

static void ich5_smb_switch_page(unsigned page)
{
	uint8_t value = 0x6c;
	if (page)
		value = 0x6e;
	io_outb(driver, SMBHSTADD, value | I2C_WRITE);
	io_outb(driver, SMBHSTCNT, SMBHSTCNT_BYTE_DATA);
	ich5_process();
}

struct pci_smbus_controller
{
	unsigned vendor;
	unsigned device;
	char* name;
	void (*get_adr)(void);
};

static struct pci_smbus_controller smbcontrollers[] =
{
	// Intel SMBUS
	{0x8086, 0x18DF, "Intel CDF",			ich5_get_smb},
	{0x8086, 0x9DA3, "Intel Cannon Lake",	ich5_get_smb},
	{0x8086, 0xA323, "Intel Cannon Lake",	ich5_get_smb},
	{0x8086, 0x31D4, "Intel GL",			ich5_get_smb},
	{0x8086, 0xA2A3, "Intel 200/Z370",		ich5_get_smb},
	{0x8086, 0xA223, "Intel Lewisburg",		ich5_get_smb},
	{0x8086, 0xA1A3, "Intel C620",			ich5_get_smb},
	{0x8086, 0x5AD4, "Intel Broxton",		ich5_get_smb},
	{0x8086, 0x19DF, "Intel Atom C3000",	ich5_get_smb},
	{0x8086, 0x9D23, "Intel Sunrise Point",	ich5_get_smb},
	{0x8086, 0x0F12, "Intel BayTrail",		ich5_get_smb},
	{0x8086, 0x9CA2, "Intel Wildcat Point",	ich5_get_smb},
	{0x8086, 0x8CA2, "Intel Wildcat Point",	ich5_get_smb},
	{0x8086, 0x23B0, "Intel DH895XCC",		ich5_get_smb},
	{0x8086, 0x8D22, "Intel C610/X99",		ich5_get_smb},
	{0x8086, 0x8D7D, "Intel C610/X99 B0",	ich5_get_smb},
	{0x8086, 0x8D7E, "Intel C610/X99 B1",	ich5_get_smb},
	{0x8086, 0x8D7F, "Intel C610/X99 B2",	ich5_get_smb},
	{0x8086, 0x1F3C, "Intel Atom C2000",	ich5_get_smb},
	{0x8086, 0x2330, "Intel DH89xxCC",		ich5_get_smb},
	{0x8086, 0x1D22, "Intel C600/X79",		ich5_get_smb},
	{0x8086, 0x1D70, "Intel C600/X79 B0",	ich5_get_smb},
	{0x8086, 0x1D71, "Intel C600/X79 B1",	ich5_get_smb},
	{0x8086, 0x1D72, "Intel C600/X79 B2",	ich5_get_smb},
	{0x8086, 0x2483, "Intel 82801CA/CAM",	ich5_get_smb},
	{0x8086, 0x2443, "Intel 82801BA/BAM",	ich5_get_smb},
	{0x8086, 0x2423, "Intel 82801AB",		ich5_get_smb},
	{0x8086, 0x2413, "Intel 82801AA",		ich5_get_smb},
	{0x8086, 0xA123, "Intel SKY",			ich5_get_smb},
	{0x8086, 0x9C22, "Intel HSW-ULT",		ich5_get_smb},
	{0x8086, 0x8C22, "Intel HSW",			ich5_get_smb},
	{0x8086, 0x1E22, "Intel Z77",			ich5_get_smb},
	{0x8086, 0x1C22, "Intel P67",			ich5_get_smb},
	{0x8086, 0x3B30, "Intel P55",			ich5_get_smb},
	{0x8086, 0x3A60, "Intel ICH10B",		ich5_get_smb},
	{0x8086, 0x3A30, "Intel ICH10R",		ich5_get_smb},
	{0x8086, 0x2930, "Intel ICH9",			ich5_get_smb},
	{0x8086, 0x283E, "Intel ICH8",			ich5_get_smb},
	{0x8086, 0x27DA, "Intel ICH7",			ich5_get_smb},
	{0x8086, 0x266A, "Intel ICH6",			ich5_get_smb},
	{0x8086, 0x24D3, "Intel ICH5",			ich5_get_smb},
	{0x8086, 0x24C3, "Intel ICH4",			ich5_get_smb},
	{0x8086, 0x25A4, "Intel 6300ESB",		ich5_get_smb},
	{0x8086, 0x269B, "Intel ESB2",			ich5_get_smb},
	{0x8086, 0x5032, "Intel EP80579",		ich5_get_smb},
	{0x8086, 0x0f12, "Intel E3800",			ich5_get_smb},
	{0x8086, 0x2292, "Intel Braswell",		ich5_get_smb},
	{0x8086, 0x1BC9, "Intel Emmitsburg",	ich5_get_smb},
	{0x8086, 0x34A3, "Intel Ice Lake-LP",	ich5_get_smb},
	{0x8086, 0x38A3, "Intel Ice Lake-N",	ich5_get_smb},
	{0x8086, 0x02A3, "Intel Comet Lake",	ich5_get_smb},
	{0x8086, 0x06A3, "Intel Comet Lake-H",	ich5_get_smb},
	{0x8086, 0x4B23, "Intel Elkhart Lake",	ich5_get_smb},
	{0x8086, 0xA0A3, "Intel Tiger Lake-LP",	ich5_get_smb},
	{0x8086, 0x43A3, "Intel Tiger Lake-H",	ich5_get_smb},
	{0x8086, 0x4DA3, "Intel Jasper Lake",	ich5_get_smb},
	{0x8086, 0xA3A3, "Intel Comet Lake-V",	ich5_get_smb},
	{0x8086, 0x7AA3, "Intel Alder Lake-S",	ich5_get_smb},
	{0x8086, 0x51A3, "Intel Alder Lake-P",	ich5_get_smb},
	{0x8086, 0x54A3, "Intel Alder Lake-M",	ich5_get_smb},
	{0, 0, "", NULL}
};

static int find_smb_controller(void)
{
	int i = 0;
	int result = 0;
	unsigned long valuev, valued;

	for (smbdev = 0; smbdev < 32; smbdev++) {
		for (smbfun = 0; smbfun < 8; smbfun++) {
			result = pci_conf_read(0, smbdev, smbfun, 0, 2, &valuev);
			if (result != 0 || valuev == 0xFFFF)
				continue;
			result = pci_conf_read(0, smbdev, smbfun, 2, 2, &valued);
			if (result != 0)
				continue;
			//printf("PCI %04X %04X\n", valuev, valued);
			for (i = 0; smbcontrollers[i].vendor > 0; i++) {
				if (valuev == smbcontrollers[i].vendor && valued == smbcontrollers[i].device)
					return i;
			}
		}
	}
	return -1;
}

static int smbus_index = -1;

void
SpdInit(void)
{
	if ((driver = cpu_msr_driver_open()) == NULL) {
		return;
	}
	if (pci_check_direct() != 0) {
		printf("pci check failed\n");
		return;
	}
	smbus_index = find_smb_controller();
	if (smbus_index == -1) {
		printf("unsupported smbus controller\n");
		return;
	}
	spd_raw = malloc(SPD_DATA_LEN);
	if (!spd_raw) {
		printf("out of memory\n");
		return;
	}
	smbcontrollers[smbus_index].get_adr();
}

void*
SpdGet(int dimmadr)
{
	unsigned short x;
	if (smbus_index < 0 || dimmadr < 0)
		return NULL;
	ZeroMemory(spd_raw, SPD_DATA_LEN);
	// switch page 0
	ich5_smb_switch_page(0);
	for (x = 0; x < 256; x++)
	{
		spd_raw[x] = ich5_smb_read_byte(0x50 + dimmadr, (unsigned char)x);
		if (x == 1 && spd_raw[0] == 0xFF && spd_raw[1] == 0xFF)
			return NULL;
	}
	if (spd_raw[2] < 12) // DDR4
		return spd_raw;
	// switch page 1
	ich5_smb_switch_page(1);
	for (x = 0; x < 256; x++)
		spd_raw[x + 256] = ich5_smb_read_byte(0x50 + dimmadr, (unsigned char)x);
	return spd_raw;
}

void
SpdFini(void)
{
	free(spd_raw);
	cpu_msr_driver_close(driver);
}
