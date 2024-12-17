// SPDX-License-Identifier: Unlicense

#include <libnw.h>
#include <version.h>
#include "libcdi/libcdi.h"

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
		"  --net[=FLAG,...] Print network info.\n"
		"    GUID           Specify the GUID of the network interface,\n"
		"                   e.g. '{B16B00B5-CAFE-BEEF-DEAD-001453AD0529}'\n"
		"    FLAGS:\n"
		"      ACTIVE       Exclude inactive network interfaces.\n"
		"      PHYS         Exclude virtual network interfaces.\n"
		"      ETH          Include Ethernet network interfaces.\n"
		"      WLAN         Include IEEE 802.11 wireless addresses.\n"
		"      IPV4         Show IPv4 addresses only.\n"
		"      IPV6         Show IPv6 addresses only.\n"
		"  --acpi[=SGN]     Print ACPI info.\n"
		"                   SGN specifies the signature of the ACPI table,\n"
		"                   e.g. 'FACP' (Fixed ACPI Description Table).\n"
		"  --smbios[=TYPE]  Print SMBIOS info.\n"
		"                   TYPE specifies the type of the SMBIOS table,\n"
		"                   e.g. '2' (Base Board Information).\n"
		"  --disk[=FLAG,..] Print disk info.\n"
		"    PATH           Specify the path of the disk,\n"
		"                   e.g. '\\\\.\\PhysicalDrive0', '\\\\.\\CdRom0'.\n"
		"    FLAGS:\n"
		"      NO-SMART     Don't print disk S.M.A.R.T. info.\n"
		"      PHYS         Exclude virtual drives.\n"
		"      CD           Include CD-ROM devices.\n"
		"      HD           Include hard drives.\n"
		"      NVME         Include NVMe devices.\n"
		"      SATA         Include SATA devices.\n"
		"      SCSI         Include SCSI devices.\n"
		"      SAS          Include SAS devices.\n"
		"      USB          Include USB devices.\n"
		"  --smart=FLAG,... Specify S.M.A.R.T. features.\n"
		"                   Features enabled by default: 'WMI', 'ATA',\n"
		"                   'NVIDIA', 'MARVELL', 'SAT', 'SUNPLUS',\n"
		"                   'IODATA', 'LOGITEC', 'PROLIFIC', 'USBJMICRON',\n"
		"                   'CYPRESS', 'MEMORY', 'JMICRON', 'ASMEDIA',\n"
		"                   'REALTEK', 'MEGARAID', 'VROC' and 'ASM1352R'.\n"
		"                   Use 'DEFAULT' to specify the above features.\n"
		"                   Other features are 'ADVANCED', 'HD204UI',\n"
		"                   'ADATA', 'NOWAKEUP', 'JMICRON3' and 'RTK9220DP'.\n"
		"  --display        Print EDID info.\n"
		"  --pci[=CLASS]    Print PCI info.\n"
		"                   CLASS specifies the class code of pci devices,\n"
		"                   e.g. '0C05' (SMBus).\n"
		"  --usb            Print USB info.\n"
		"  --spd            Print SPD info.\n"
		"  --battery        Print battery info.\n"
		"  --uefi[=FLAG,..] Print UEFI info.\n"
		"    FLAGS:\n"
		"      MENU         Print UEFI boot menus.\n"
		"      VARS         List all UEFI variables.\n"
		"  --shares         Print network mapped drives and shared folders.\n"
		"  --audio          Print audio devices.\n"
		"  --public-ip      Print public IP address.\n"
		"  --product-policy[=NAME]\n"
		"                   Print ProductPolicy.\n"
		"                   NAME specifies the name of the product policy.\n"
		"  --gpu            Print GPU usage.\n"
		"  --font           Print installed fonts.\n");
}

typedef struct _NW_ARG_FILTER
{
	const char* arg;
	UINT64 flag;
} NW_ARG_FILTER;

static char*
nwinfo_get_opts(char* arg, UINT64* flag, int count, NW_ARG_FILTER* filter, const char* extra)
{
	int i;
	int argc;
	char* p;
	char* ret = NULL;
	char** argv;
	size_t len = 0;

	if (arg[0] != '=')
		return NULL;
	for (p = arg, argc = 0; p; p = strchr(p, ','), argc++)
		p++;
	argv = calloc(argc, sizeof(char*));
	if (!argv)
		return NULL;
	for (p = arg, i = 0; p; p = strchr(p, ','), i++)
	{
		*p = '\0';
		p++;
		argv[i] = p;
	}
	if (extra)
		len = strlen(extra);

	for (i = 0; i < argc; i++)
	{
		if (extra && _strnicmp(argv[i], extra, len) == 0)
		{
			ret = argv[i];
			continue;
		}
		for (int j = 0; j < count; j++)
		{
			if (_stricmp(argv[i], filter[j].arg) == 0)
			{
				*flag |= filter[j].flag;
				break;
			}
		}
	}
	free(argv);
	return ret;
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
			NW_ARG_FILTER filter[] =
			{
				{"active", NW_NET_ACTIVE},
				{"phys", NW_NET_PHYS},
				{"ipv4", NW_NET_IPV4},
				{"ipv6", NW_NET_IPV6},
				{"eth", NW_NET_ETH},
				{"wlan", NW_NET_WLAN},
			};
			nwContext.NetGuid = nwinfo_get_opts(&argv[i][5], &nwContext.NetFlags, ARRAYSIZE(filter), filter, "{");
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
			NW_ARG_FILTER filter[] =
			{
				{"no-smart", NW_DISK_NO_SMART},
				{"phys", NW_DISK_PHYS},
				{"cd", NW_DISK_CD},
				{"hd", NW_DISK_HD},
				{"nvme", NW_DISK_NVME},
				{"sata", NW_DISK_SATA},
				{"scsi", NW_DISK_SCSI},
				{"sas", NW_DISK_SAS},
				{"usb", NW_DISK_USB},
			};
			nwContext.DiskPath = nwinfo_get_opts(&argv[i][6], &nwContext.DiskFlags,
				ARRAYSIZE(filter), filter, "\\\\.\\");
			nwContext.DiskInfo = TRUE;
		}
		else if (_strnicmp(argv[i], "--smart", 7) == 0)
		{
			NW_ARG_FILTER filter[] =
			{
				{"WMI", CDI_FLAG_USE_WMI},
				{"ADVANCED", CDI_FLAG_ADVANCED_SEARCH},
				{"HD204UI", CDI_FLAG_WORKAROUND_HD204UI},
				{"ADATA", CDI_FLAG_WORKAROUND_ADATA},
				{"NOWAKEUP", CDI_FLAG_NO_WAKEUP},
				{"ATA", CDI_FLAG_ATA_PASS_THROUGH},
				{"NVIDIA", CDI_FLAG_ENABLE_NVIDIA	},
				{"MARVELL", CDI_FLAG_ENABLE_MARVELL},
				{"SAT", CDI_FLAG_ENABLE_USB_SAT},
				{"SUNPLUS", CDI_FLAG_ENABLE_USB_SUNPLUS},
				{"IODATA", CDI_FLAG_ENABLE_USB_IODATA},
				{"LOGITEC", CDI_FLAG_ENABLE_USB_LOGITEC},
				{"PROLIFIC", CDI_FLAG_ENABLE_USB_PROLIFIC},
				{"USBJMICRON", CDI_FLAG_ENABLE_USB_JMICRON},
				{"CYPRESS", CDI_FLAG_ENABLE_USB_CYPRESS},
				{"MEMORY", CDI_FLAG_ENABLE_USB_MEMORY},
				{"JMICRON3", CDI_FLAG_ENABLE_NVME_JMICRON3},
				{"JMICRON", CDI_FLAG_ENABLE_NVME_JMICRON},
				{"ASMEDIA", CDI_FLAG_ENABLE_NVME_ASMEDIA},
				{"REALTEK", CDI_FLAG_ENABLE_NVME_REALTEK},
				{"MEGARAID", CDI_FLAG_ENABLE_MEGA_RAID},
				{"VROC", CDI_FLAG_ENABLE_INTEL_VROC},
				{"ASM1352R", CDI_FLAG_ENABLE_ASM1352R},
				{"RTK9220DP", CDI_FLAG_ENABLE_REALTEK_9220DP},
				{"DEFAULT", CDI_FLAG_DEFAULT},
			};
			nwContext.NwSmartFlags = 0;
			nwinfo_get_opts(&argv[i][7], &nwContext.NwSmartFlags,
				ARRAYSIZE(filter), filter, NULL);
		}
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
		else if (_strnicmp(argv[i], "--uefi", 6) == 0)
		{
			NW_ARG_FILTER filter[] =
			{
				{"vars", NW_UEFI_VARS},
				{"menu", NW_UEFI_MENU},
			};
			nwinfo_get_opts(&argv[i][6], &nwContext.UefiFlags, ARRAYSIZE(filter), filter, NULL);
			nwContext.UefiInfo = TRUE;
		}
		else if (_stricmp(argv[i], "--shares") == 0)
			nwContext.ShareInfo = TRUE;
		else if (_stricmp(argv[i], "--audio") == 0)
			nwContext.AudioInfo = TRUE;
		else if (_stricmp(argv[i], "--public-ip") == 0)
			nwContext.PublicIpInfo = TRUE;
		else if (_strnicmp(argv[i], "--product-policy", 16) == 0)
		{
			nwContext.ProductPolicyInfo = TRUE;
			if (argv[i][16] == '=' && argv[i][17])
				nwContext.ProductPolicy = &argv[i][17];
		}
		else if (_stricmp(argv[i], "--gpu") == 0)
			nwContext.GpuInfo = TRUE;
		else if (_stricmp(argv[i], "--font") == 0)
			nwContext.FontInfo = TRUE;
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
