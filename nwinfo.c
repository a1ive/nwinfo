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
		"  --format=XXX     Specify output format. [YAML|JSON|LUA]\n"
		"  --output=FILE    Write to FILE instead of printing to screen.\n"
		"  --human          Display numbers in human readable format.\n"
		"  --sys            Print system info.\n"
		"  --cpu            Print CPUID info.\n"
		"  --net[=active]   Print [active] network info\n"
		"  --acpi[=XXXX]    Print ACPI [table=XXXX] info.\n"
		"  --smbios[=XX]    Print SMBIOS [type=XX] info.\n"
		"  --disk           Print disk info.\n"
		"  --no-smart       Don't print disk S.M.A.R.T. info.\n"
		"  --display        Print EDID info.\n"
		"  --pci[=XX]       Print PCI [class=XX] info.\n"
		"  --usb            Print USB info.\n"
		"  --spd            Print SPD info\n"
		"  --battery        Print battery info.\n"
		"  --uefi           Print UEFI info.\n");
}

int main(int argc, char* argv[])
{
	LPCSTR lpFileName = NULL;
	ZeroMemory(&nwContext, sizeof(NWLIB_CONTEXT));
	nwContext.NwFormat = FORMAT_YAML;
	nwContext.HumanSize = FALSE;
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
		else if (_stricmp(argv[i], "--disk") == 0)
			nwContext.DiskInfo = TRUE;
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
		else
		{
		}
	}
	(void)CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	NW_Print(lpFileName);
	NW_Fini();
	CoUninitialize();
	return 0;
}
