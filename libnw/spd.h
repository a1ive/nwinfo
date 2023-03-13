// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdint.h>

void NWL_SpdInit(void);
void* NWL_SpdGet(uint8_t index);
void NWL_SpdFini(void);

#define SPD_DATA_LEN 1024

#define SPD5_MR11 11

#define SMBHSTSTS   smbus_base
#define SMBHSTCNT   smbus_base + 2
#define SMBHSTCMD   smbus_base + 3
#define SMBHSTADD   smbus_base + 4
#define SMBHSTDAT0  smbus_base + 5
#define SMBHSTDAT1  smbus_base + 6
#define SMBBLKDAT   smbus_base + 7
#define SMBPEC      smbus_base + 8
#define SMBAUXSTS   smbus_base + 12
#define SMBAUXCTL   smbus_base + 13

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

#define AMD_INDEX_IO_PORT   0xCD6
#define AMD_DATA_IO_PORT    0xCD7
#define AMD_SMBUS_BASE_REG  0x2C
#define AMD_PM_INDEX        0x00

#define NV_SMBUS_ADR_REG        0x20
#define NV_OLD_SMBUS_ADR_REG    0x50

#define NVSMBCNT    smbus_base + 0
#define NVSMBSTS    smbus_base + 1
#define NVSMBADD    smbus_base + 2
#define NVSMBCMD    smbus_base + 3
#define NVSMBDAT(x) (smbus_base + 4 + (x))

#define NVSMBCNT_WRITE      0x00
#define NVSMBCNT_READ       0x01
#define NVSMBCNT_QUICK      0x02
#define NVSMBCNT_BYTE       0x04
#define NVSMBCNT_BYTE_DATA  0x06
#define NVSMBCNT_WORD_DATA  0x08

#define NVSMBSTS_DONE       0x80
#define NVSMBSTS_ALRM       0x40
#define NVSMBSTS_RES        0x20
#define NVSMBSTS_STATUS     0x1f

#define PIIX4_SMB_BASE_ADR_DEFAULT  0x90
