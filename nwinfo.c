// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <windows.h>
#include "nwinfo.h"
#include "libcpuid.h"

int main(int argc, char** argv)
{
	int debug_level = 0;
	if (IsAdmin() != TRUE
		|| ObtainPrivileges(SE_SYSTEM_ENVIRONMENT_NAME) != ERROR_SUCCESS)
	{
		printf("permission denied\n");
		return 1;
	}
	for (int i = 0; i < argc; i++) {
		cpuid_set_verbosiness_level(debug_level);
		if (i == 0 && argc > 1)
			continue;
		else if (_strnicmp(argv[i], "--debug=", 8) == 0 && argv[i][8]) {
			debug_level = strtol(&argv[i][8], NULL, 0);
		}
		else if (_stricmp(argv[i], "--sys") == 0)
			nwinfo_sys();
		else if (_stricmp(argv[i], "--cpu") == 0)
			nwinfo_cpuid();
		else if (_strnicmp(argv[i], "--net", 5) == 0) {
			int active = 0;
			if (_stricmp(&argv[i][5], "=active") == 0)
				active = 1;
			nwinfo_network(active);
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
		else if (_strnicmp(argv[i], "--display", 9) == 0) {
			int raw = 0;
			if (_stricmp(&argv[i][9], "=raw") == 0)
				raw = 1;
			nwinfo_display(raw);
		}
		else if (_strnicmp(argv[i], "--pci", 5) == 0) {
			const CHAR* PciClass = NULL;
			if (argv[i][5] == '=' && argv[i][6])
				PciClass = &argv[i][6];
			nwinfo_pci(PciClass);
		}
		else if (_stricmp(argv[i], "--usb") == 0)
			nwinfo_usb();
		else if (_stricmp(argv[i], "--beep") == 0) {
			int new_argc = argc - i - 1;
			char** new_argv = argc > 0 ? &argv[i + 1] : NULL;
			nwinfo_beep(new_argc, new_argv);
			return 0;
		}
		else if (_strnicmp(argv[i], "--spd", 5) == 0) {
			int raw = 0;
			if (_stricmp(&argv[i][5], "=raw") == 0)
				raw = 1;
			nwinfo_spd(raw);
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
			printf("  --spd[=raw]    Print [raw] SPD info\n");
		}
	}
	return 0;
}
