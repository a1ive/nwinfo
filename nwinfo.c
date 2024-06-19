// SPDX-License-Identifier: Unlicense

#include <libnw.h>
#include <version.h>

static NWLIB_CONTEXT nwContext;

static void nwinfo_help(void)
{
	printf(NWINFO_CLI " v" NWINFO_VERSION_STR "\n"
		NWINFO_FILEDESC "\n"
		NWINFO_COPYRIGHT "\n"
		"Usage: nwinfo OPTIONS\n"
		"OPTIONS:\n"
		"  --format=FMT     Specify output format.\n"
		"                   FMT can be 'YAML' (default), 'JSON' and 'LUA'.\n"
		"  --output=FILE    Write to FILE instead of printing to screen.\n"
		"  --cp=CODEPAGE    Set the code page of output text.\n"
		"                   CODEPAGE can be 'ANSI' and 'UTF8'.\n"
		"  --human          Display numbers in human readable format.\n"
		"  --debug          Print debug info to stdout.\n"
		"  --hide-sensitive Hide sensitive data (MAC & S/N).\n"
		"  --sys            Print system info.\n"
		"  --cpu            Print CPUID info.\n"
		"  --net[=FLAG]     Print network info\n"
		"                   FLAG can be 'ACTIVE' (print only the active network).\n"
		"  --acpi[=SGN]     Print ACPI info.\n"
		"                   SGN specifies the signature of the ACPI table,\n"
		"                   e.g. 'FACP' (Fixed ACPI Description Table).\n"
		"  --smbios[=TYPE]  Print SMBIOS info.\n"
		"                   TYPE specifies the type of the SMBIOS table,\n"
		"                   e.g. '2' (Base Board Information).\n"
		"  --disk[=PATH]    Print disk info.\n"
		"                   PATH specifies the path of the disk,\n"
		"                   e.g. '\\\\.\\PhysicalDrive0', '\\\\.\\CdRom0'.\n"
		"  --no-smart       Don't print disk S.M.A.R.T. info.\n"
		"  --display        Print EDID info.\n"
		"  --pci[=CLASS]    Print PCI info.\n"
		"                   CLASS specifies the class code of pci devices,\n"
		"                   e.g. '0C05' (SMBus).\n"
		"  --usb            Print USB info.\n"
		"  --spd            Print SPD info.\n"
		"  --battery        Print battery info.\n"
		"  --uefi           Print UEFI info.\n"
		"  --shares         Print network mapped drives.\n"
		"  --audio          Print audio devices.\n"
		"  --public-ip      Print public IP address.\n"
		"  --product-policy Print ProductPolicy.\n");
}

int main(int argc, char* argv[])
{
	BOOL bSetCodePage = FALSE;
	LPCSTR lpFileName = NULL;
	ZeroMemory(&nwContext, sizeof(NWLIB_CONTEXT));
	nwContext.NwFormat = FORMAT_YAML;
	nwContext.HumanSize = FALSE;
	nwContext.Debug = FALSE;
	nwContext.HideSensitive = FALSE;
	NW_Init(&nwContext);
	
	for (int i = 0; i < argc; i++)
	{
		if (i == 0 && argc > 1)
			continue;
		else if (_stricmp(argv[i], "--help") == 0
			|| _stricmp(argv[i], "-h") == 0
			|| _stricmp(argv[i], "/h") == 0
			|| _stricmp(argv[i], "/?") == 0)
		{
			nwinfo_help();
			exit(0);
		}
		else if (_strnicmp(argv[i], "--format=", 9) == 0 && argv[i][9])
		{
			if (_stricmp(&argv[i][9], "YAML") == 0)
				nwContext.NwFormat = FORMAT_YAML;
			else if (_stricmp(&argv[i][9], "JSON") == 0)
				nwContext.NwFormat = FORMAT_JSON;
			else if (_stricmp(&argv[i][9], "LUA") == 0)
				nwContext.NwFormat = FORMAT_LUA;
		}
		else if (_strnicmp(argv[i], "--cp=", 5) == 0 && argv[i][5])
		{
			bSetCodePage = TRUE;
			if (_stricmp(&argv[i][5], "ANSI") == 0)
				nwContext.CodePage = CP_ACP;
			else if (_stricmp(&argv[i][5], "UTF8") == 0)
				nwContext.CodePage = CP_UTF8;
		}
		else if (_strnicmp(argv[i], "--output=", 9) == 0 && argv[i][9])
			lpFileName = &argv[i][9];
		else if (_stricmp(argv[i], "--human") == 0)
			nwContext.HumanSize = TRUE;
		else if (_stricmp(argv[i], "--sys") == 0)
			nwContext.SysInfo = TRUE;
		else if (_stricmp(argv[i], "--cpu") == 0)
			nwContext.CpuInfo = TRUE;
		else if (_strnicmp(argv[i], "--net", 5) == 0)
		{
			nwContext.ActiveNet = _stricmp(&argv[i][5], "=active") == 0 ? TRUE : FALSE;
			nwContext.NetInfo = TRUE;
		}
		else if (_strnicmp(argv[i], "--acpi", 6) == 0)
		{
			if (argv[i][6] == '=' && strlen(&argv[i][7]) == 4)
				memcpy(&nwContext.AcpiTable, &argv[i][7], 4);
			nwContext.AcpiInfo = TRUE;
		}
		else if (_strnicmp(argv[i], "--smbios", 8) == 0)
		{
			if (argv[i][8] == '=' && argv[i][9])
				nwContext.SmbiosType = (UINT8)strtoul(&argv[i][9], NULL, 0);
			nwContext.DmiInfo = TRUE;
		}
		else if (_strnicmp(argv[i], "--disk", 6) == 0)
		{
			if (argv[i][6] == '=' && argv[i][7])
				nwContext.DiskPath = &argv[i][7];
			nwContext.DiskInfo = TRUE;
		}
		else if (_stricmp(argv[i], "--no-smart") == 0)
			nwContext.DisableSmart = TRUE;
		else if (_stricmp(argv[i], "--display") == 0)
			nwContext.EdidInfo = TRUE;
		else if (_strnicmp(argv[i], "--pci", 5) == 0)
		{
			if (argv[i][5] == '=' && argv[i][6])
				nwContext.PciClass = &argv[i][6];
			nwContext.PciInfo = TRUE;
		}
		else if (_stricmp(argv[i], "--usb") == 0)
			nwContext.UsbInfo = TRUE;
		else if (_stricmp(argv[i], "--spd") == 0)
			nwContext.SpdInfo = TRUE;
		else if (_stricmp(argv[i], "--battery") == 0)
			nwContext.BatteryInfo = TRUE;
		else if (_stricmp(argv[i], "--uefi") == 0)
			nwContext.UefiInfo = TRUE;
		else if (_stricmp(argv[i], "--shares") == 0)
			nwContext.ShareInfo = TRUE;
		else if (_stricmp(argv[i], "--audio") == 0)
			nwContext.AudioInfo = TRUE;
		else if (_stricmp(argv[i], "--public-ip") == 0)
			nwContext.PublicIpInfo = TRUE;
		else if (_stricmp(argv[i], "--product-policy") == 0)
			nwContext.ProductPolicyInfo = TRUE;
		else if (_stricmp(argv[i], "--debug") == 0)
			nwContext.Debug = TRUE;
		else if (_stricmp(argv[i], "--hide-sensitive") == 0)
			nwContext.HideSensitive = TRUE;
		else
		{
		}
	}
	if (bSetCodePage == FALSE)
	{
		if (lpFileName)
			nwContext.CodePage = CP_UTF8;
		else
			nwContext.CodePage = CP_ACP;
	}
	(void)CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	NW_Print(lpFileName);
	NW_Fini();
	CoUninitialize();
	return 0;
}
