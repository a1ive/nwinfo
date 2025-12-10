// SPDX-License-Identifier: Unlicense
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "winring0.h"

static HANDLE mutex_pci = NULL;
static HANDLE mutex_smbus = NULL;

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

void WR0_MicroSleep(unsigned int usec)
{
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(10 * (__int64)usec);

	timer = CreateWaitableTimerW(NULL, TRUE, NULL);
	if (!timer)
		return;
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
}

void WR0_OpenMutexes(void)
{
	mutex_pci = open_mutex(L"Global\\Access_PCI");
	mutex_smbus = open_mutex(L"Global\\Access_SMBUS.HTP.Method");
}

void WR0_CloseMutexes(void)
{
	if (mutex_pci != NULL)
		CloseHandle(mutex_pci);
	if (mutex_smbus != NULL)
		CloseHandle(mutex_smbus);
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

BOOL WR0_WaitSmBus(DWORD timeout)
{
	return wait_mutex(mutex_smbus, timeout);
}

void WR0_ReleaseSmBus(void)
{
	if (mutex_smbus != NULL)
		ReleaseMutex(mutex_smbus);
}

typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
BOOL WR0_IsWoW64(void)
{
#ifdef _WIN64
	return FALSE;
#else
	BOOL bIsWow64 = FALSE;
	HMODULE hMod = GetModuleHandleW(L"kernel32");
	LPFN_ISWOW64PROCESS fnIsWow64Process = NULL;
	if (hMod)
		fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(hMod, "IsWow64Process");
	if (fnIsWow64Process)
		fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
	return bIsWow64;
#endif
}

typedef LPCSTR(CDECL* LPFN_WINEGETVER) (VOID);
LPCSTR WR0_GetWineVersion(void)
{
	HMODULE hModule = GetModuleHandleW(L"ntdll");
	LPFN_WINEGETVER fnWineGetVersion = NULL;
	if (hModule)
		fnWineGetVersion = (LPFN_WINEGETVER)GetProcAddress(hModule, "wine_get_version");
	if (fnWineGetVersion)
		return fnWineGetVersion();
	return NULL;
}

typedef VOID(CDECL* LPFN_WINEGETHOST) (const CHAR**, const CHAR**);
void WR0_GetWineHost(const CHAR** sysname, const CHAR** release)
{
	HMODULE hModule = GetModuleHandleW(L"ntdll");
	LPFN_WINEGETHOST fnWineGetHost = NULL;
	*sysname = NULL;
	*release = NULL;
	if (hModule)
		fnWineGetHost = (LPFN_WINEGETHOST)GetProcAddress(hModule, "wine_get_host_version");
	if (fnWineGetHost)
		fnWineGetHost(sysname, release);
}
