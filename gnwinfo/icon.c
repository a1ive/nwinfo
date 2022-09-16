// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include <acpi.h>

INT GNW_IconFromAcpi(PNODE node, LPCSTR name)
{
	INT icon = IDI_ICON_TVN_DMI;
	UNREFERENCED_PARAMETER(node);
	if (_stricmp(name, RSDP_SIGNATURE) == 0)
		icon = IDI_ICON_TVD_TREE;
	else if (_stricmp(name, "APIC") == 0)
		icon = IDI_ICON_TVD_FW;
	else if (_stricmp(name, "ASF!") == 0)
		icon = IDI_ICON_TVD_INF;
	else if (_stricmp(name, "BGRT") == 0)
		icon = IDI_ICON_TVN_EDID;
	else if (_stricmp(name, "BOOT") == 0)
		icon = IDI_ICON_TVD_REL;
	else if (_stricmp(name, "DBGP") == 0)
		icon = IDI_ICON_TVD_LNK;
	else if (_stricmp(name, "DBG2") == 0)
		icon = IDI_ICON_TVD_LNK;
	else if (_stricmp(name, "DSDT") == 0)
		icon = IDI_ICON_TVN_BAT;
	else if (_stricmp(name, "FACP") == 0)
		icon = IDI_ICON_TVN_ACPI;
	else if (_stricmp(name, "MCFG") == 0)
		icon = IDI_ICON_TVN_SPD;
	else if (_stricmp(name, "MSDM") == 0)
		icon = IDI_ICON_TVN_SYS;
	else if (_stricmp(name, "RSDT") == 0)
		icon = IDI_ICON_TVD_TREE;
	else if (_stricmp(name, "SSDT") == 0)
		icon = IDI_ICON_TVD_TREE;
	else if (_stricmp(name, "TPM2") == 0)
		icon = IDI_ICON_TVD_ENC;
	else if (_stricmp(name, "UEFI") == 0)
		icon = IDI_ICON_TVD_EFI;
	else if (_stricmp(name, "WPBT") == 0)
		icon = IDI_ICON_TVN_SYS;
	else if (_stricmp(name, "WSMT") == 0)
		icon = IDI_ICON_TVD_ENC;
	else if (_stricmp(name, "XSDT") == 0)
		icon = IDI_ICON_TVD_TREE;
	return icon;
}

INT GNW_IconFromCpu(PNODE node, LPCSTR name)
{
	INT icon = IDI_ICON_TVN_CPU;
	UNREFERENCED_PARAMETER(node);
	if (_stricmp(name, "SGX") == 0)
		icon = IDI_ICON_TVD_ENC;
	else if (_stricmp(name, "Multiplier") == 0)
		icon = IDI_ICON_TVN_CPU;
	else if (_stricmp(name, "Cache") == 0)
		icon = IDI_ICON_TVD_FW;
	else if (_stricmp(name, "Features") == 0)
		icon = IDI_ICON_TVD_DOC;
	return icon;
}

INT GNW_IconFromDisk(PNODE node, LPCSTR name)
{
	INT icon = IDI_ICON_TVD_HDD;
	LPSTR rm = NWL_NodeAttrGet(node, "Removable");
	if (rm && _stricmp(rm, "Yes") == 0)
		icon = IDI_ICON_TVD_RMD;
	if (name && _strnicmp(name, "\\\\.\\CdRom", 9) == 0)
		icon = IDI_ICON_TVD_ISO;
	return icon;
}

INT GNW_IconFromNetwork(PNODE node, LPCSTR name)
{
	INT icon = IDI_ICON_TVN_NET;
	UNREFERENCED_PARAMETER(name);
	LPSTR type = NWL_NodeAttrGet(node, "Type");
	if (!type)
		icon = IDI_ICON_TVD_HLP;
	else if (_stricmp(type, "Ethernet") == 0)
		icon = IDI_ICON_TVD_NE;
	else if (_stricmp(type, "IEEE 802.11 Wireless") == 0)
		icon = IDI_ICON_TVD_NW;
	else if (_stricmp(type, "Tunnel") == 0)
		icon = IDI_ICON_TVD_ENC;
	return icon;
}

INT GNW_IconFromPci(PNODE node, LPCSTR name)
{
	INT icon = IDI_ICON_TVN_PCI;
	UNREFERENCED_PARAMETER(name);
	LPSTR hwclass = NWL_NodeAttrGet(node, "Class");
	if (!hwclass || _stricmp(hwclass, "Unclassified device") == 0)
		icon = IDI_ICON_TVD_HLP;
	else if (_stricmp(hwclass, "Mass storage controller") == 0)
		icon = IDI_ICON_TVD_HDD;
	else if (_stricmp(hwclass, "Network controller") == 0)
		icon = IDI_ICON_TVD_NE;
	else if (_stricmp(hwclass, "Display controller") == 0)
		icon = IDI_ICON_TVN_EDID;
	else if (_stricmp(hwclass, "Multimedia controller") == 0)
		icon = IDI_ICON_TVD_MM;
	else if (_stricmp(hwclass, "Memory controller") == 0)
		icon = IDI_ICON_TVN_SPD;
	else if (_stricmp(hwclass, "Bridge") == 0)
		icon = IDI_ICON_TVD_FW;
	else if (_stricmp(hwclass, "Communication controller") == 0)
		icon = IDI_ICON_TVD_LNK;
	else if (_stricmp(hwclass, "Generic system peripheral") == 0)
		icon = IDI_ICON_TVD_INF;
	else if (_stricmp(hwclass, "Processor") == 0)
		icon = IDI_ICON_TVN_CPU;
	else if (_stricmp(hwclass, "Wireless controller") == 0)
		icon = IDI_ICON_TVD_NW;
	else if (_stricmp(hwclass, "Encryption controller") == 0)
		icon = IDI_ICON_TVD_ENC;
	else if (_stricmp(hwclass, "Signal processing controller") == 0)
		icon = IDI_ICON_TVD_LNK;
	else if (_stricmp(hwclass, "Unassigned class") == 0)
		icon = IDI_ICON_TVD_HLP;
	return icon;
}

INT GNW_IconFromSmbios(PNODE node, LPCSTR name)
{
	UINT type = strtoul(name, NULL, 10);
	UNREFERENCED_PARAMETER(node);
	switch (type)
	{
	case 0:
		return IDI_ICON_TVD_EFI;
	case 1:
	case 3:
		return IDI_ICON_TVD_PC;
	case 2:
	case 9:
		return IDI_ICON_TVD_FW;
	case 10:
		return IDI_ICON_TVN_PCI;
	case 4:
	case 7:
		return IDI_ICON_TVN_CPU;
	case 5:
	case 6:
	case 16:
	case 17:
	case 19:
	case 20:
		return IDI_ICON_TVN_SPD;
	case 8:
		return IDI_ICON_TVD_LNK;
	case 11:
	case 12:
		return IDI_ICON_TVD_DOC;
	case 13:
		return IDI_ICON_TVD_INF;
	case 14:
		return IDI_ICON_TVD_TREE;
	case 22:
		return IDI_ICON_TVN_BAT;
	case 32:
		return IDI_ICON_TVD_REL;
	case 43:
		return IDI_ICON_TVD_ENC;
	case 127:
		return IDI_ICON_TVD_EXIT;
	case 130:
	case 131:
		// Intel ME
		return IDI_ICON_TVN_CPU;
	case 221:
		// Firmware Version
		return IDI_ICON_TVD_FW;
	default:
		if (type > 127)
			return IDI_ICON_TVD_HLP;
	}
	return IDI_ICON_TVN_DMI;
}

INT GNW_IconFromUsb(PNODE node, LPCSTR name)
{
	INT icon = IDI_ICON_TVD_USB;
	UNREFERENCED_PARAMETER(name);
	LPSTR hwclass = NWL_NodeAttrGet(node, "Class");
	if (!hwclass || _stricmp(hwclass, "(Defined at Interface level)") == 0)
		icon = IDI_ICON_TVD_INF;
	else if (_stricmp(hwclass, "Audio") == 0)
		icon = IDI_ICON_TVD_MM;
	else if (_stricmp(hwclass, "Communications") == 0)
		icon = IDI_ICON_TVD_LNK;
	else if (_stricmp(hwclass, "Human Interface Device") == 0)
		icon = IDI_ICON_TVD_HID;
	else if (_stricmp(hwclass, "Physical Interface Device") == 0)
		icon = IDI_ICON_TVD_HID;
	else if (_stricmp(hwclass, "Printer") == 0)
		icon = IDI_ICON_TVD_DOC;
	else if (_stricmp(hwclass, "Mass Storage") == 0)
		icon = IDI_ICON_TVD_RMD;
	else if (_stricmp(hwclass, "Hub") == 0)
		icon = IDI_ICON_TVD_TREE;
	else if (_stricmp(hwclass, "Chip/SmartCard") == 0)
		icon = IDI_ICON_TVD_FW;
	else if (_stricmp(hwclass, "Content Security") == 0)
		icon = IDI_ICON_TVD_ENC;
	else if (_stricmp(hwclass, "Video") == 0)
		icon = IDI_ICON_TVD_MM;
	else if (_stricmp(hwclass, "Wireless") == 0)
		icon = IDI_ICON_TVD_NW;
	return icon;
}
