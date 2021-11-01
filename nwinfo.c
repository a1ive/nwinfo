// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include "nwinfo.h"

int __cdecl main(int argc, char** argv)
{
	for (int i = 0; i < argc; i++) {
		if (i == 0 && argc > 1)
			continue;
		else if (_stricmp(argv[i], "--sys") == 0)
			nwinfo_sys();
		else if (_stricmp(argv[i], "--cpu") == 0)
			nwinfo_cpuid();
		else if (_stricmp(argv[i], "--net") == 0)
			nwinfo_network();
		else if (_stricmp(argv[i], "--acpi") == 0)
			nwinfo_acpi();
		else if (_stricmp(argv[i], "--smbios") == 0)
			nwinfo_smbios();
		else {
			printf("Usage: nwinfo OPTIONS\n");
			printf("OPTIONS:\n");
			printf("  --sys      Print system info.\n");
			printf("  --cpu      Print CPUID info.\n");
			printf("  --net      Print network info.\n");
			printf("  --acpi     Print ACPI info.\n");
			printf("  --smbios   Print SMBIOS info.\n");
		}
	}
	return 0;
}
