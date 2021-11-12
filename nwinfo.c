// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <windows.h>
#include <VersionHelpers.h>
#include <devguid.h>
#include "nwinfo.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "setupapi.lib")

int __cdecl main(int argc, char** argv)
{
	if (!IsWindows7OrGreater())
	{
		printf("You need at least Windows 7\n");
		return 0;
	}
	for (int i = 0; i < argc; i++) {
		if (i == 0 && argc > 1)
			continue;
		else if (_stricmp(argv[i], "--sys") == 0)
			nwinfo_sys();
		else if (_stricmp(argv[i], "--cpu") == 0)
			nwinfo_cpuid();
		else if (_strnicmp(argv[i], "--net", 5) == 0) {
			if (_stricmp(&argv[i][5], "=active") == 0)
				nwinfo_network(1);
			else
				nwinfo_network(0);
		}
		else if (_stricmp(argv[i], "--acpi") == 0)
			nwinfo_acpi();
		else if (_strnicmp(argv[i], "--smbios", 8) == 0) {
			UINT8 Type = 127;
			if (argv[i][8] == '=' && argv[i][9])
				Type = (UINT8) strtoul(&argv[i][9], NULL, 0);
			nwinfo_smbios(Type);
		}
		else if (_stricmp(argv[i], "--disk") == 0)
			nwinfo_disk();
		else if (_stricmp(argv[i], "--display") == 0)
			nwinfo_display();
		else if (_strnicmp(argv[i], "--pci", 5) == 0) {
			const CHAR* PciClass = NULL;
			PciClass = NULL;
			if (argv[i][5] == '=' && argv[i][6])
				PciClass = &argv[i][6];
			nwinfo_pci(NULL, PciClass);
		}
		else if (_stricmp(argv[i], "--usb") == 0) {
			nwinfo_usb(NULL);
		}
		else {
			printf("Usage: nwinfo OPTIONS\n");
			printf("OPTIONS:\n");
			printf("  --sys          Print system info.\n");
			printf("  --cpu          Print CPUID info.\n");
			printf("  --net[=active] Print [active] network info\n");
			printf("  --acpi         Print ACPI info.\n");
			printf("  --smbios[=XX]  Print SMBIOS info.\n");
			printf("  --disk         Print disk info.\n");
			printf("  --display      Print display info.\n");
			printf("  --pci[=XX]     Print PCI info.\n");
			printf("  --usb          Print USB info.\n");
		}
	}
	return 0;
}
