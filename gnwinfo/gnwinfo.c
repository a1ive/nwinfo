// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

static INT_PTR CALLBACK
AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

static INT_PTR
MainMenuProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	switch (wmId)
	{
	case IDM_ABOUT:
		DialogBoxParamA(GNWC.hInst,
			MAKEINTRESOURCEA(IDD_ABOUT_DIALOG), hWnd, AboutDlgProc, 0);
		break;
	case IDM_EXIT:
		DestroyWindow(hWnd);
		break;
	default:
		return DefWindowProcA(hWnd, message, wParam, lParam);
	}
	return (INT_PTR)TRUE;
}

static VOID
MainDlgResize(HWND hWnd, UINT nWidth, UINT nHeight)
{
	int x, cx;
	HWND hwndTV = GetDlgItem(GNWC.hWnd, IDC_MAIN_TREE);
	HWND hwndLV = GetDlgItem(GNWC.hWnd, IDC_MAIN_LIST);
	if (!hwndTV || !hwndLV)
		return;
	x = nWidth / 4;
	SetWindowPos(hwndTV, NULL, 0, 0, x, nHeight, 0);
	cx = nWidth - x;
	SetWindowPos(hwndLV, NULL, x, 0, cx, nHeight, 0);
	ListView_SetColumnWidth(hwndLV, 0, cx / 4);
	ListView_SetColumnWidth(hwndLV, 1, cx / 4);
	ListView_SetColumnWidth(hwndLV, 2, cx / 2);
}

static INT_PTR CALLBACK
MainDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SendMessageA(hWnd, WM_SETICON, ICON_SMALL,
			(LPARAM)LoadIconA(GNWC.hInst, MAKEINTRESOURCEA(IDI_ICON)));
		return (INT_PTR)TRUE;
	case WM_NOTIFY:
		return GNW_TreeUpdate(hWnd, wParam, lParam);
	case WM_SIZE:
		MainDlgResize(hWnd, LOWORD(lParam), HIWORD(lParam));
		return (INT_PTR)TRUE;
	case WM_SYSCOMMAND:
		if (wParam == SC_CLOSE)
			DestroyWindow(hWnd);
		/* fall through */
	case WM_COMMAND:
		return MainMenuProc(hWnd, message, wParam, lParam);
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return (INT_PTR)FALSE;
	}
	return (INT_PTR)TRUE;
}

int APIENTRY
WinMain(_In_ HINSTANCE hInstance,
		_In_opt_ HINSTANCE hPrevInstance,
		_In_ LPSTR lpCmdLine,
		_In_ int nCmdShow)
{
	MSG msg;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	GNW_Init(hInstance, nCmdShow, MainDlgProc);

	while (GetMessageA(&msg, NULL, 0, 0))
	{
		if (!IsDialogMessageA(GNWC.hWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}

	GNW_Exit(0);
	return 0;
}
