// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

#define MAX_LOADSTRING 100

GNW_CONTEXT GNWC;

static void
GNW_Wait(VOID)
{
	HWND hwndLV = GetDlgItem(GNWC.hWnd, IDC_MAIN_LIST);
	HWND hwndTV = GetDlgItem(GNWC.hWnd, IDC_MAIN_TREE);
	ShowWindow(hwndLV, SW_HIDE);
	ShowWindow(hwndTV, SW_HIDE);
	SetWindowTextA(GNWC.hWnd, GNW_GetText("Loading, please wait ..."));
	GNWC.pnAcpi = NW_Acpi();
	GNWC.pnBattery = NW_Battery();
	GNWC.pnCpuid = NW_Cpuid();
	GNWC.pnDisk = NW_Disk();
	GNWC.pnEdid = NW_Edid();
	GNWC.pnNetwork = NW_Network();
	GNWC.pnPci = NW_Pci();
	GNWC.pnSmbios = NW_Smbios();
	GNWC.pnSystem = NW_System();
	GNWC.pnUsb = NW_Usb();

	SetWindowTextA(GNWC.hWnd, GNWINFO_TITLE);
	ShowWindow(hwndLV, SW_SHOW);
	ShowWindow(hwndTV, SW_SHOW);
}

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
	GNWC.nCtx.BatteryInfo = TRUE;
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

	GNWC.wLang = GetUserDefaultUILanguage();

	GNWC.hImageList = ImageList_Create(24, 24, ILC_COLOR32, 0, IDI_ICON_MAX - IDI_ICON + 1);
	if (!GNWC.hImageList)
		GNW_Exit(1);
	for (i = IDI_ICON; i <= IDI_ICON_MAX; i++)
	{
		ImageList_AddIcon(GNWC.hImageList, LoadIconA(GNWC.hInst, MAKEINTRESOURCEA(i)));
	}

	GNWC.hMenu = GetMenu(GNWC.hWnd);
	if (!GNWC.hMenu)
		GNW_Exit(1);
	GNW_SetMenuText();
	ShowWindow(GNWC.hWnd, nCmdShow);
	UpdateWindow(GNWC.hWnd);

	GNW_Wait();
	GNWC.pnRoot = NW_Libinfo();

	GNW_ListInit();
	GNW_TreeInit();

	SetTimer(GNWC.hWnd, IDT_TIMER1, 1000, (TIMERPROC)NULL);

	GNW_TreeExpand(GNWC.htRoot);
}

VOID
GNW_Reload(VOID)
{
	NWL_NodeFree(GNWC.nCtx.NwRoot, 1);
	GNWC.nCtx.NwRoot = NWL_NodeAlloc("NWinfo", 0);
	
	GNW_Wait();
	GNW_ListClean();
	ZeroMemory(&GNWC.tvCurItem, sizeof(TVITEMA));
	GNW_TreeDelete(TVI_ROOT);
	GNW_TreeInit();

	GNW_TreeExpand(GNWC.htRoot);
}

VOID
GNW_Export(VOID)
{
	OPENFILENAMEA ofn;
	CHAR FilePath[MAX_PATH];
	SYSTEMTIME st;
	GetSystemTime(&st);
	snprintf(FilePath, MAX_PATH, "nwinfo_report_%u%u%u%u%u%u.json",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GNWC.hWnd;
	ofn.lpstrFilter = "JSON (*.json)\0*.json\0YAML (*.yaml)\0*.yaml\0LUA (*.lua)\0*.lua\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = FilePath;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = "txt";
	if (!GetSaveFileNameA(&ofn))
		return;
	if (fopen_s(&GNWC.nCtx.NwFile, FilePath, "w"))
	{
		MessageBoxA(GNWC.hWnd, "Cannot open file!", "ERROR", MB_OK);
		return;
	}
	if (_strnicmp(FilePath + ofn.nFileExtension, "yaml", 3) == 0)
		NWL_NodeToYaml(GNWC.nCtx.NwRoot, GNWC.nCtx.NwFile, 0);
	else if (_strnicmp(FilePath + ofn.nFileExtension, "json", 3) == 0)
		NWL_NodeToJson(GNWC.nCtx.NwRoot, GNWC.nCtx.NwFile, 0);
	else
		NWL_NodeToLua(GNWC.nCtx.NwRoot, GNWC.nCtx.NwFile, 0);
	fclose(GNWC.nCtx.NwFile);
	GNWC.nCtx.NwFile = NULL;
}

VOID __declspec(noreturn)
GNW_Exit(INT nExitCode)
{
	NW_Fini();
	if (GNWC.pnRoot)
		NWL_NodeFree(GNWC.pnRoot, 1);
	ImageList_Destroy(GNWC.hImageList);
	KillTimer(GNWC.hWnd, IDT_TIMER1);
	CloseHandle(GNWC.hMutex);
	exit(nExitCode);
}
