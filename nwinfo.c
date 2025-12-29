// SPDX-License-Identifier: Unlicense

#include <libnw.h>
#include <version.h>
#include "libcdi/libcdi.h"
#include "sensor/sensors.h"

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include "optparse.h"

static NWLIB_CONTEXT nwContext;

enum
{
	NW_OPT_HELP = 0,
	NW_OPT_HELP_WIN32,
	NW_OPT_FORMAT,
	NW_OPT_OUTPUT,
	NW_OPT_CP,
	NW_OPT_HUMAN,
	NW_OPT_BIN,
	NW_OPT_DEBUG,
	NW_OPT_HIDE_SENSITIVE,
	NW_OPT_SYS,
	NW_OPT_CPU,
	NW_OPT_NET,
	NW_OPT_ACPI,
	NW_OPT_SMBIOS,
	NW_OPT_DISK,
	NW_OPT_SMART,
	NW_OPT_DISPLAY,
	NW_OPT_PCI,
	NW_OPT_USB,
	NW_OPT_SPD,
	NW_OPT_BATTERY,
	NW_OPT_UEFI,
	NW_OPT_SHARES,
	NW_OPT_AUDIO,
	NW_OPT_PUBLIC_IP,
	NW_OPT_PRODUCT_POLICY,
	NW_OPT_GPU,
	NW_OPT_FONT,
	NW_OPT_DEVICE,
	NW_OPT_SENSORS,
};

static struct optparse_option nwOptions[] =
{
	{ "help", 'h', OPTPARSE_NONE },
	{ "?", '?', OPTPARSE_NONE },
	{ "format", 'f', OPTPARSE_REQUIRED},
	{ "output", 'o', OPTPARSE_REQUIRED},
	{ "cp", 'c', OPTPARSE_REQUIRED},
	{ "human", 'u', OPTPARSE_NONE},
	{ "bin", 'b', OPTPARSE_REQUIRED},
	{ "debug", 'd', OPTPARSE_NONE},
	{ "hide-sensitive", 'i', OPTPARSE_NONE},
	{ "sys", 0, OPTPARSE_NONE },
	{ "cpu", 0, OPTPARSE_OPTIONAL },
	{ "net", 0, OPTPARSE_OPTIONAL },
	{ "acpi", 0, OPTPARSE_OPTIONAL },
	{ "smbios", 0, OPTPARSE_OPTIONAL },
	{ "disk", 0, OPTPARSE_OPTIONAL },
	{ "smart", 0, OPTPARSE_REQUIRED },
	{ "display", 0, OPTPARSE_NONE },
	{ "pci", 0, OPTPARSE_OPTIONAL },
	{ "usb", 0, OPTPARSE_NONE },
	{ "spd", 0, OPTPARSE_OPTIONAL },
	{ "battery", 0, OPTPARSE_NONE },
	{ "uefi", 0, OPTPARSE_OPTIONAL },
	{ "shares", 0, OPTPARSE_NONE },
	{ "audio", 0, OPTPARSE_NONE },
	{ "public-ip", 0, OPTPARSE_NONE },
	{ "product-policy", 0, OPTPARSE_OPTIONAL },
	{ "gpu", 0, OPTPARSE_NONE },
	{ "font", 0, OPTPARSE_NONE },
	{ "device", 0, OPTPARSE_OPTIONAL },
	{ "sensors", 0, OPTPARSE_OPTIONAL },
	{ 0, 0, 0 },
};

static void nwinfo_help(void)
{
	printf(NWINFO_CLI " v" NWINFO_VERSION_STR "\n"
		NWINFO_FILEDESC "\n"
		NWINFO_COPYRIGHT "\n"
		"Usage: nwinfo OPTIONS\n"
		"OPTIONS:\n"
		"  --format=FMT     Specify output format.\n"
		"                   FMT can be 'YAML' (default), 'JSON',\n"
		"                   'LUA', 'TREE' or 'HTML'.\n"
		"  --output=FILE    Write to FILE instead of printing to screen.\n"
		"  --cp=CODEPAGE    Set the code page of output text.\n"
		"                   CODEPAGE can be 'ANSI' or 'UTF8'.\n"
		"  --human          Display numbers in human readable format.\n"
		"  --bin=FMT        Specify binary format.\n"
		"                   FMT can be 'NONE' (default), 'BASE64' or 'HEX'.\n"
		"  --debug          Print debug info to stdout.\n"
		"  --hide-sensitive Hide sensitive data (MAC & S/N).\n"
		"  --sys            Print system info.\n"
		"  --cpu[=FILE]     Print CPUID info.\n"
		"    FILE           Specify the file name of the CPUID dump.\n"
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
		"                   'REALTEK', 'MEGARAID', 'VROC', 'ASM1352R' and 'HIDERAID'.\n"
		"                   Use 'DEFAULT' to specify the above features.\n"
		"                   Other features are 'ADVANCED', 'HD204UI',\n"
		"                   'ADATA', 'NOWAKEUP' and 'RTK9220DP'.\n"
		"  --display        Print EDID info.\n"
		"  --pci[=CLASS]    Print PCI info.\n"
		"                   CLASS specifies the class code of PCI devices,\n"
		"                   e.g. '0C05' (SMBus).\n"
		"  --usb            Print USB info.\n"
		"  --spd[=FILE]     Print DIMM SPD info.\n"
		"                   WARNING: This option may damage the hardware.\n"
		"    FILE           Specify the file name of the SPD dump.\n"
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
		"  --font           Print installed fonts.\n"
		"  --device[=TYPE]  Print device tree.\n"
		"                   TYPE specifies the type of the devices,\n"
		"                   e.g. 'ACPI', 'SWD', 'PCI' or 'USB'.\n"
		"  --sensors[=SRC,..]\n"
		"                   Print sensors.\n"
		"                   SRC specifies the provider of sensors.\n"
		"                   Available providers are:\n"
		"                   'LHM', 'GPU', 'CPU', 'DIMM', 'HWINFO' and 'GPU-Z'.\n");
}

typedef struct _NW_ARG_FILTER
{
	const char* arg;
	UINT64 flag;
} NW_ARG_FILTER;

static char*
nwinfo_get_opts(const char* arg, UINT64* flag, int count, const NW_ARG_FILTER* filter, const char* extra)
{
	char* dup;
	char* token;
	char* str = NULL;
	size_t len = extra ? strlen(extra) : 0;

	if (!arg || !*arg)
		return NULL;

	dup = _strdup(arg);
	if (!dup)
		return NULL;

	for (token = dup; token && *token; )
	{
		char* next = strchr(token, ',');
		if (next)
			*next++ = '\0';

		if (extra && _strnicmp(token, extra, len) == 0)
		{
			free(str);
			str = _strdup(token);
		}
		else
		{
			for (int j = 0; j < count; j++)
			{
				if (_stricmp(token, filter[j].arg) == 0)
				{
					*flag |= filter[j].flag;
					break;
				}
			}
		}
		token = next;
	}
	free(dup);
	return str;
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
	nwContext.BinaryFormat = BIN_FMT_NONE;
	nwContext.NwFile = stdout;
	nwContext.AcpiTable = 0;
	nwContext.SmbiosType = 127;
	nwContext.DiskPath = NULL;

	struct optparse options;
	optparse_init(&options, argv);

	for (;;)
	{
		int option = optparse(&options, nwOptions, NULL);
		if (option == OPTPARSE_DONE)
			break;

		switch (option)
		{
		case OPTPARSE_ERR:
			fprintf(stderr, "Error: %s\n", options.errmsg);
			return 1;
		case NW_OPT_HELP:
		case NW_OPT_HELP_WIN32:
			nwinfo_help();
			return 0;
		case NW_OPT_FORMAT:
			if (_stricmp(options.optarg, "YAML") == 0)
				nwContext.NwFormat = FORMAT_YAML;
			else if (_stricmp(options.optarg, "JSON") == 0)
				nwContext.NwFormat = FORMAT_JSON;
			else if (_stricmp(options.optarg, "LUA") == 0)
				nwContext.NwFormat = FORMAT_LUA;
			else if (_stricmp(options.optarg, "TREE") == 0)
				nwContext.NwFormat = FORMAT_TREE;
			else if (_stricmp(options.optarg, "HTML") == 0)
				nwContext.NwFormat = FORMAT_HTML;
			break;
		case NW_OPT_CP:
			bSetCodePage = TRUE;
			if (_stricmp(options.optarg, "ANSI") == 0)
				nwContext.CodePage = CP_ACP;
			else if (_stricmp(options.optarg, "UTF8") == 0)
				nwContext.CodePage = CP_UTF8;
			break;
		case NW_OPT_OUTPUT:
			lpFileName = options.optarg;
			break;
		case NW_OPT_HUMAN:
			nwContext.HumanSize = TRUE;
			break;
		case NW_OPT_BIN:
			if (_stricmp(options.optarg, "BASE64") == 0)
				nwContext.BinaryFormat = BIN_FMT_BASE64;
			else if (_stricmp(options.optarg, "HEX") == 0)
				nwContext.BinaryFormat = BIN_FMT_HEX;
			break;
		case NW_OPT_SYS:
			nwContext.SysInfo = TRUE;
			break;
		case NW_OPT_CPU:
			if (options.optarg && options.optarg[0])
				nwContext.CpuDump = options.optarg;
			nwContext.CpuInfo = TRUE;
			break;
		case NW_OPT_NET:
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
			nwContext.NetGuid = nwinfo_get_opts(options.optarg, &nwContext.NetFlags, ARRAYSIZE(filter), filter, "{");
			nwContext.NetInfo = TRUE;
			break;
		}
		case NW_OPT_ACPI:
			if (options.optarg && strlen(options.optarg) == 4)
				memcpy(&nwContext.AcpiTable, options.optarg, 4);
			nwContext.AcpiInfo = TRUE;
			break;
		case NW_OPT_SMBIOS:
			if (options.optarg && options.optarg[0])
				nwContext.SmbiosType = (UINT8)strtoul(options.optarg, NULL, 0);
			nwContext.DmiInfo = TRUE;
			break;
		case NW_OPT_DISK:
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
			nwContext.DiskPath = nwinfo_get_opts(options.optarg, &nwContext.DiskFlags,
				ARRAYSIZE(filter), filter, "\\\\.\\");
			nwContext.DiskInfo = TRUE;
			break;
		}
		case NW_OPT_SMART:
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
				{"JMICRON", CDI_FLAG_ENABLE_NVME_JMICRON},
				{"ASMEDIA", CDI_FLAG_ENABLE_NVME_ASMEDIA},
				{"REALTEK", CDI_FLAG_ENABLE_NVME_REALTEK},
				{"MEGARAID", CDI_FLAG_ENABLE_MEGA_RAID},
				{"VROC", CDI_FLAG_ENABLE_INTEL_VROC},
				{"ASM1352R", CDI_FLAG_ENABLE_ASM1352R},
				{"RTK9220DP", CDI_FLAG_ENABLE_REALTEK_9220DP},
				{"HIDERAID", CDI_FLAG_HIDE_RAID_VOLUME},
				{"DEFAULT", CDI_FLAG_DEFAULT},
			};
			nwContext.NwSmartFlags = 0;
			nwinfo_get_opts(options.optarg, &nwContext.NwSmartFlags,
				ARRAYSIZE(filter), filter, NULL);
			break;
		}
		case NW_OPT_DISPLAY:
			nwContext.EdidInfo = TRUE;
			break;
		case NW_OPT_PCI:
			if (options.optarg && options.optarg[0])
				nwContext.PciClass = options.optarg;
			nwContext.PciInfo = TRUE;
			break;
		case NW_OPT_USB:
			nwContext.UsbInfo = TRUE;
			break;
		case NW_OPT_SPD:
			if (options.optarg && options.optarg[0])
				nwContext.SpdDump = options.optarg;
			nwContext.SpdInfo = TRUE;
			break;
		case NW_OPT_BATTERY:
			nwContext.BatteryInfo = TRUE;
			break;
		case NW_OPT_UEFI:
		{
			NW_ARG_FILTER filter[] =
			{
				{"vars", NW_UEFI_VARS},
				{"menu", NW_UEFI_MENU},
			};
			nwinfo_get_opts(options.optarg, &nwContext.UefiFlags, ARRAYSIZE(filter), filter, NULL);
			nwContext.UefiInfo = TRUE;
			break;
		}
		case NW_OPT_SHARES:
			nwContext.ShareInfo = TRUE;
			break;
		case NW_OPT_AUDIO:
			nwContext.AudioInfo = TRUE;
			break;
		case NW_OPT_PUBLIC_IP:
			nwContext.PublicIpInfo = TRUE;
			break;
		case NW_OPT_PRODUCT_POLICY:
			nwContext.ProductPolicyInfo = TRUE;
			if (options.optarg && options.optarg[0])
				nwContext.ProductPolicy = options.optarg;
			break;
		case NW_OPT_GPU:
			nwContext.GpuInfo = TRUE;
			break;
		case NW_OPT_FONT:
			nwContext.FontInfo = TRUE;
			break;
		case NW_OPT_DEVICE:
			if (options.optarg && options.optarg[0])
				nwContext.DevTreeFilter = options.optarg;
			nwContext.DevTree = TRUE;
			break;
		case NW_OPT_SENSORS:
		{
			NW_ARG_FILTER filter[] =
			{
				{"LHM", NWL_SENSOR_LHM},
				{"GPU", NWL_SENSOR_GPU},
				{"DIMM", NWL_SENSOR_DIMM},
				{"HWINFO", NWL_SENSOR_HWINFO},
				{"GPU-Z", NWL_SENSOR_GPUZ},
				{"CPU", NWL_SENSOR_CPU},
			};
			nwinfo_get_opts(options.optarg, &nwContext.NwSensorFlags, ARRAYSIZE(filter), filter, NULL);
			nwContext.Sensors = TRUE;
			break;
		}
		case NW_OPT_DEBUG:
			nwContext.Debug = TRUE;
			break;
		case NW_OPT_HIDE_SENSITIVE:
			nwContext.HideSensitive = TRUE;
			break;
		default:
			break;
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
	NW_Init(&nwContext);
	NW_Print(lpFileName);
	if (nwContext.NetGuid)
		free(nwContext.NetGuid);
	if (nwContext.DiskPath)
		free(nwContext.DiskPath);
	NW_Fini();
	CoUninitialize();
	return 0;
}
