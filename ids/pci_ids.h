// SPDX-License-Identifier: Unlicense
#pragma once

#define PCI_VID_INTEL       0x8086
#define PCI_VID_AMD         0x1022
#define PCI_VID_ATI         0x1002
#define PCI_VID_HYGON       0x1d94
#define PCI_VID_EFAR        0x1055
#define PCI_VID_NVIDIA      0x10DE
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
#define PCI_DID_AMD_K8_CTRL     0x1103
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

#define PCI_IDS_DEFAULT \
	"1002  AMD/ATI\n" \
	"101e  AMI\n" \
	"1022  AMD\n" \
	"1042  Micron\n" \
	"1043  ASUSTeK\n" \
	"104c  Texas Instruments\n" \
	"1055  Microchip / SMSC\n" \
	"106b  Apple\n" \
	"108e  Oracle/SUN\n" \
	"1099  Samsung\n" \
	"10b5  PLX\n" \
	"10b7  3Com\n" \
	"10b9  ULi\n" \
	"10de  NVIDIA\n" \
	"10ec  Realtek\n" \
	"1106  VIA\n" \
	"1166  Broadcom\n" \
	"1414  Microsoft\n" \
	"144d  Samsung\n" \
	"1458  Gigabyte\n" \
	"1462  MSI\n" \
	"15ad  VMware\n" \
	"1849  ASRock\n" \
	"19e5  Huawei\n" \
	"1ae0  Google\n" \
	"1af4  Red Hat\n" \
	"1b21  ASMedia\n" \
	"1b36  Red Hat\n" \
	"1b96  Western Digital\n" \
	"1bb1  Seagate\n" \
	"1d17  Zhaoxin\n" \
	"1d94  Hygon\n" \
	"8086  Intel\n" \
	"\n" \
	"C 00  Unclassified device\n" \
	"C 01  Mass storage controller\n" \
	"C 02  Network controller\n" \
	"C 03  Display controller\n" \
	"C 04  Multimedia controller\n" \
	"C 05  Memory controller\n" \
	"C 06  Bridge\n" \
	"C 07  Communication controller\n" \
	"C 08  Generic system peripheral\n" \
	"C 09  Input device controller\n" \
	"C 0a  Docking station\n" \
	"C 0b  Processor\n" \
	"C 0c  Serial bus controller\n" \
	"C 0d  Wireless controller\n" \
	"\n"
