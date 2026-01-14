// SPDX-License-Identifier: Unlicense
#include <windows.h>
#include <winioctl.h>
#include <pathcch.h>
#include <winternl.h>
#include <shlobj.h>
#include "ioctl.h"
#include "ioctl_priv.h"
#include "../libnw/libnw.h"

BOOL
WR0_CheckPawnIO(void)
{
#ifdef _WIN64
	WCHAR path[MAX_PATH];
	// Program Files\\PawnIO\\PawnIO.sys
	SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, 0, path);
	PathCchAppend(path, MAX_PATH, L"PawnIO\\" PAWNIO_NAME);
	HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		return TRUE;
	}
#endif
	return FALSE;
}

BOOL
WR0_InstallPawnIO(void)
{
#ifdef _WIN64
	WCHAR path[MAX_PATH];
	WCHAR cmdline[] = PAWNIO_SETUP_CMD;
	STARTUPINFOW si = { .cb = sizeof(STARTUPINFOW) };
	PROCESS_INFORMATION pi = { 0 };
	GetModuleFileNameW(NULL, path, MAX_PATH);
	PathCchRemoveFileSpec(path, MAX_PATH);
	PathCchAppend(path, MAX_PATH, PAWNIO_SETUP_EXE);
	if (!CreateProcessW(path, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		return FALSE;
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
#endif
	return WR0_CheckPawnIO();
}

int WR0_ExecPawn(struct wr0_drv_t* drv, struct pio_mod_t* mod, LPCSTR fn,
	const ULONG64* in, SIZE_T in_size,
	PULONG64 out, SIZE_T out_size,
	PSIZE_T return_size)
{
	PIO_EXEC_INPUT* inBuf;
	DWORD inBufSize;
	DWORD returnedLength;
	BOOL bRes;

	if (return_size)
		*return_size = 0;

	if (!mod->hd || mod->hd == INVALID_HANDLE_VALUE)
		return -1;

	inBufSize = (DWORD)(sizeof(PIO_EXEC_INPUT) + in_size * sizeof(ULONG64));

	inBuf = calloc(1, inBufSize);
	if (inBuf == NULL)
		return -1;

	strcpy_s(inBuf->Fn, PIO_FN_NAME_LEN, fn);

	if (in)
		memcpy(inBuf->Params, in, in_size * sizeof(ULONG64));

	bRes = DeviceIoControl(mod->hd,
		IOCTL_PIO_EXECUTE_FN,
		inBuf,
		inBufSize,
		out,
		(DWORD)(out_size * sizeof(*out)),
		&returnedLength,
		NULL);

	free(inBuf);

	NWL_Debug("PIO", "Exec %s %s in@%zu out@%zu ret@%lu", bRes ? "OK" : "FAIL", fn, in_size, out_size, returnedLength);

	if (bRes == FALSE)
		return -1;

	if (return_size)
		*return_size = returnedLength / sizeof(*out);
	return 0;
}
