// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <windows.h>
#include "nwinfo.h"

int main(int argc, char** argv)
{
	int debug_level = 0;
	for (int i = 0; i < argc; i++) {
		if (i == 0 && argc > 1)
			continue;
		else if (_strnicmp(argv[i], "--debug=", 8) == 0 && argv[i][8]) {
			debug_level = strtol(&argv[i][8], NULL, 0);
		}
		else if (_stricmp(argv[i], "--sys") == 0)
			nwinfo_sys();
		else if (_stricmp(argv[i], "--cpu") == 0)
			nwinfo_cpuid(debug_level);
		else if (_strnicmp(argv[i], "--net", 5) == 0) {
			if (_stricmp(&argv[i][5], "=active") == 0)
				nwinfo_network(1);
			else
				nwinfo_network(0);
		}
		else if (_strnicmp(argv[i], "--acpi", 6) == 0) {
			DWORD signature = 0;
			if (argv[i][6] == '=' && strlen(&argv[i][7]) == 4)
				memcpy(&signature, &argv[i][7], 4);
			nwinfo_acpi(signature);
		}
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
		else if (_stricmp(argv[i], "--beep") == 0) {
			int new_argc = argc - i - 1;
			char** new_argv = argc > 0 ? &argv[i + 1] : NULL;
			nwinfo_beep(new_argc, new_argv);
			return 0;
		}
		else {
			printf("Usage: nwinfo OPTIONS\n");
			printf("OPTIONS:\n");
			printf("  --sys          Print system info.\n");
			printf("  --cpu          Print CPUID info.\n");
			printf("  --net[=active] Print [active] network info\n");
			printf("  --acpi[=XXXX]  Print ACPI [table=XXXX] info.\n");
			printf("  --smbios[=XX]  Print SMBIOS [type=XX] info.\n");
			printf("  --disk         Print disk info.\n");
			printf("  --display      Print display info.\n");
			printf("  --pci[=XX]     Print PCI [class=XX] info.\n");
			printf("  --usb          Print USB info.\n");
			printf("  --beep FREQ TIME [FREQ TIME ...]\n");
			printf("                 Play a tune.\n");
		}
	}
	return 0;
}
