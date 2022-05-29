// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

#define MAX_LOADSTRING 100

GNW_CONTEXT GNWC;

VOID
GNW_Init(HINSTANCE hInstance, INT nCmdShow, DLGPROC lpDialogFunc)
{
	INT i;

	ZeroMemory(&GNWC, sizeof(GNW_CONTEXT));
	GNWC.hMutex = CreateMutexA(NULL, TRUE, "NWinfo{e25f6e37-d51b-4950-8949-510dfc86d913}");
	if (GetLastError() == ERROR_ALREADY_EXISTS || !GNWC.hMutex)
		exit(1);
	GNWC.hInst = hInstance;
	GNWC.nCtx.NwFormat = FORMAT_JSON;
	GNWC.nCtx.HumanSize = TRUE;
	GNWC.nCtx.AcpiInfo = TRUE;
	GNWC.nCtx.CpuInfo = TRUE;
	GNWC.nCtx.DiskInfo = TRUE;
	GNWC.nCtx.EdidInfo = TRUE;
	GNWC.nCtx.NetInfo = TRUE;
	GNWC.nCtx.PciInfo = TRUE;
	GNWC.nCtx.DmiInfo = TRUE;
	GNWC.nCtx.SysInfo = TRUE;
	GNWC.nCtx.UsbInfo = TRUE;
	if (NW_Init(&GNWC.nCtx) == FALSE)
		GNW_Exit(1);
	GNWC.hWnd = CreateDialogParamA(hInstance,
		MAKEINTRESOURCEA(IDD_MAIN_DIALOG), NULL, lpDialogFunc, 0);
	if (!GNWC.hWnd)
		GNW_Exit(1);
	ShowWindow(GNWC.hWnd, nCmdShow);
	UpdateWindow(GNWC.hWnd);

	GNWC.hImageList = ImageList_Create(24, 24, ILC_COLOR32, 0, IDI_ICON_MAX - IDI_ICON + 1);
	if (!GNWC.hImageList)
		GNW_Exit(1);
	for (i = IDI_ICON; i <= IDI_ICON_MAX; i++)
	{
		ImageList_AddIcon(GNWC.hImageList, LoadIconA(GNWC.hInst, MAKEINTRESOURCEA(i)));
	}

	GNWC.pnAcpi = NW_Acpi();
	GNWC.pnCpuid = NW_Cpuid();
	GNWC.pnDisk = NW_Disk();
	GNWC.pnEdid = NW_Edid();
	GNWC.pnNetwork = NW_Network();
	GNWC.pnPci = NW_Pci();
	GNWC.pnSmbios = NW_Smbios();
	GNWC.pnSystem = NW_System();
	GNWC.pnUsb = NW_Usb();

	GNW_ListInit();
	GNW_TreeInit();

	GNW_TreeExpand(GNWC.htRoot);
}

VOID __declspec(noreturn)
GNW_Exit(INT nExitCode)
{
	NW_Fini();
	ImageList_Destroy(GNWC.hImageList);
	CloseHandle(GNWC.hMutex);
	exit(nExitCode);
}
