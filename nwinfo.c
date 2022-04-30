// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <windows.h>
#include "nwinfo.h"
#include "libcpuid.h"
#include "format.h"

enum output_format nwinfo_output_format = FORMAT_YAML;
FILE* nwinfo_output;
UCHAR nwinfo_buffer[NWINFO_BUFSZ];

static void nwinfo_help(void)
{
	printf("Usage: nwinfo OPTIONS\n"
		"OPTIONS:\n"
		"  --format=XXX     Specify output format. [YAML|JSON]\n"
		"  --output=FILE    Write to FILE instead of printing to screen.\n"
		"  --sys            Print system info.\n"
		"  --cpu            Print CPUID info.\n"
		"  --net[=active]   Print [active] network info\n"
		"  --acpi[=XXXX]    Print ACPI [table=XXXX] info.\n"
		"  --smbios[=XX]    Print SMBIOS [type=XX] info.\n"
		"  --disk           Print disk info.\n"
		"  --display        Print EDID info.\n"
		"  --pci[=XX]       Print PCI [class=XX] info.\n"
		"  --usb            Print USB info.\n"
		"  --beep FREQ TIME [FREQ TIME ...]\n"
		"                   Play a tune.\n"
		"  --spd            Print SPD info\n");
}

int main(int argc, char** argv)
{
	int debug_level = 0;
	PNODE nw_root;
	PNODE node;
	nwinfo_output = stdout;
	if (IsAdmin() != TRUE
		|| ObtainPrivileges(SE_SYSTEM_ENVIRONMENT_NAME) != ERROR_SUCCESS)
	{
		fprintf(stderr, "permission denied\n");
		exit(1);
	}
	nw_root = node_alloc("NWinfo", 0);
	for (int i = 0; i < argc; i++)
	{
		cpuid_set_verbosiness_level(debug_level);
		if (i == 0 && argc > 1)
			continue;
		else if (_strnicmp(argv[i], "--debug=", 8) == 0 && argv[i][8])
		{
			debug_level = strtol(&argv[i][8], NULL, 0);
		}
		else if (_strnicmp(argv[i], "--format=", 9) == 0 && argv[i][9])
		{
			if (_stricmp(&argv[i][9], "YAML") == 0)
				nwinfo_output_format = FORMAT_YAML;
			else if (_stricmp(&argv[i][9], "JSON") == 0)
				nwinfo_output_format = FORMAT_JSON;
		}
		else if (_strnicmp(argv[i], "--output=", 9) == 0 && argv[i][9])
		{
			if (fopen_s(&nwinfo_output, &argv[i][9], "w"))
			{
				fprintf(stderr, "cannot open %s.\n", &argv[i][9]);
				exit(1);
			}
		}
		else if (_stricmp(argv[i], "--sys") == 0)
		{
			node = nwinfo_sys();
			node_append_child(nw_root, node);
		}
		else if (_stricmp(argv[i], "--cpu") == 0)
		{
			node = nwinfo_cpuid();
			node_append_child(nw_root, node);
		}
		else if (_strnicmp(argv[i], "--net", 5) == 0)
		{
			node = nwinfo_network(_stricmp(&argv[i][5], "=active") == 0 ? 1 : 0);
			node_append_child(nw_root, node);
		}
		else if (_strnicmp(argv[i], "--acpi", 6) == 0)
		{
			DWORD signature = 0;
			if (argv[i][6] == '=' && strlen(&argv[i][7]) == 4)
				memcpy(&signature, &argv[i][7], 4);
			node = nwinfo_acpi(signature);
			node_append_child(nw_root, node);
		}
		else if (_strnicmp(argv[i], "--smbios", 8) == 0)
		{
			UINT8 Type = 127;
			if (argv[i][8] == '=' && argv[i][9])
				Type = (UINT8) strtoul(&argv[i][9], NULL, 0);
			node = nwinfo_smbios(Type);
			node_append_child(nw_root, node);
		}
		else if (_stricmp(argv[i], "--disk") == 0)
		{
			node = nwinfo_disk();
			node_append_child(nw_root, node);
		}
		else if (_stricmp(argv[i], "--display") == 0)
		{
			node = nwinfo_display();
			node_append_child(nw_root, node);
		}
		else if (_strnicmp(argv[i], "--pci", 5) == 0)
		{
			const CHAR* PciClass = NULL;
			if (argv[i][5] == '=' && argv[i][6])
				PciClass = &argv[i][6];
			node = nwinfo_pci(PciClass);
			node_append_child(nw_root, node);
		}
		else if (_stricmp(argv[i], "--usb") == 0)
		{
			node = nwinfo_usb();
			node_append_child(nw_root, node);
		}
		else if (_stricmp(argv[i], "--beep") == 0)
		{
			int new_argc = argc - i - 1;
			char** new_argv = argc > 0 ? &argv[i + 1] : NULL;
			nwinfo_beep(new_argc, new_argv);
			goto main_out;
		}
		else if (_stricmp(argv[i], "--spd") == 0)
		{
			node = nwinfo_spd();
			node_append_child(nw_root, node);
		}
		else
		{
			nwinfo_help();
			exit(0);
		}
	}

main_out:
	switch (nwinfo_output_format)
	{
	case FORMAT_YAML:
		node_to_yaml(nw_root, nwinfo_output, 0);
		break;
	case FORMAT_JSON:
		node_to_json(nw_root, nwinfo_output, 0);
		break;
	}
	_fcloseall();
	node_free(nw_root, 1);
	return 0;
}
