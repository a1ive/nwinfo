// SPDX-License-Identifier: Unlicense
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "winring0.h"

static HANDLE mutex_pci = NULL;

static HANDLE open_mutex(LPCWSTR name)
{
	HANDLE mutex = CreateMutexW(NULL, FALSE, name);

	if (mutex == NULL && GetLastError() == ERROR_ACCESS_DENIED)
		mutex = OpenMutexW(SYNCHRONIZE, FALSE, name);

	return mutex;
}

static BOOL wait_mutex(HANDLE mutex, DWORD timeout)
{
	if (mutex == NULL)
		return TRUE;

	DWORD dwWaitResult = WaitForSingleObject(mutex, timeout);

	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		return TRUE;
	case WAIT_ABANDONED:
		return TRUE;

	case WAIT_TIMEOUT:
	case WAIT_FAILED:
	default:
		return FALSE;
	}
}

void WR0_OpenMutexes(void)
{
	mutex_pci = open_mutex(L"Global\\Access_PCI");
}

void WR0_CloseMutexes(void)
{
	if (mutex_pci != NULL)
		CloseHandle(mutex_pci);
}

BOOL WR0_WaitPciBus(DWORD timeout)
{
	return wait_mutex(mutex_pci, timeout);
}

void WR0_ReleasePciBus(void)
{
	if (mutex_pci != NULL)
		ReleaseMutex(mutex_pci);
}
