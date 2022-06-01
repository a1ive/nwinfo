// SPDX-License-Identifier: Unlicense
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winioctl.h>
#include <winerror.h>
#include "winring0.h"
#include "libcpuid.h"
#include "asm-bits.h"
#include "libcpuid_util.h"
#include "libcpuid_internal.h"
#include "rdtsc.h"

struct msr_driver_t
{
	CHAR driver_path[MAX_PATH + 1];
	SC_HANDLE scManager;
	SC_HANDLE scDriver;
	HANDLE hhDriver;
	int errorcode;
};

static BOOL LoadDriver(struct msr_driver_t* drv)
{
	BOOL Ret = FALSE;
	DWORD Status = 0;
	BOOL Retry = TRUE;

	drv->scManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (drv->scManager == NULL)
		return FALSE;
retry:
	drv->scDriver = CreateServiceA(drv->scManager, OLS_DRIVER_ID, OLS_DRIVER_ID,
		SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE,
		drv->driver_path, NULL, NULL, NULL, NULL, NULL);
	if (drv->scDriver == NULL)
	{
		drv->scDriver = OpenServiceA(drv->scManager, OLS_DRIVER_ID, SERVICE_ALL_ACCESS);
		if (drv->scDriver == NULL)
		{
			CloseServiceHandle(drv->scManager);
			return FALSE;
		}
	}

	Ret = StartServiceA(drv->scDriver, 0, NULL);
	if (Ret == FALSE)
	{
		Status = GetLastError();
		if (Status == ERROR_SERVICE_ALREADY_RUNNING)
			Ret = TRUE;
		else if (Retry == TRUE)
		{
			Retry = FALSE;
			DeleteService(drv->scDriver);
			CloseServiceHandle(drv->scDriver);
			goto retry;
		}
		else
			Ret = FALSE;
	}

	CloseServiceHandle(drv->scDriver);
	CloseServiceHandle(drv->scManager);

	return Ret;
}

typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
static BOOL is_running_x64(void)
{
#ifdef _WIN64
	return TRUE;
#else
	BOOL bIsWow64 = FALSE;
	HMODULE hMod = GetModuleHandleA("kernel32");
	LPFN_ISWOW64PROCESS fnIsWow64Process = NULL;
	if (hMod)
		fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(hMod, "IsWow64Process");
	if (fnIsWow64Process)
		fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
	return bIsWow64;
#endif
}

static int extract_driver(struct msr_driver_t* driver)
{
	size_t i = 0;
	ZeroMemory(driver->driver_path, sizeof(driver->driver_path));
	if (!GetModuleFileNameA(NULL, driver->driver_path, MAX_PATH)
		|| strlen(driver->driver_path) == 0)
		return 0;
	for (i = strlen(driver->driver_path); i > 0; i--)
	{
		if (driver->driver_path[i] == '\\')
		{
			driver->driver_path[i] = '\0';
			break;
		}
	}
	snprintf(driver->driver_path, MAX_PATH, "%s\\%s%s.sys", driver->driver_path,
		OLS_DRIVER_NAME, is_running_x64() ? "x64" : "");
	return 1;
}

struct msr_driver_t* cpu_msr_driver_open(void)
{
	struct msr_driver_t* drv;
	BOOL status = FALSE;

	drv = (struct msr_driver_t*)malloc(sizeof(struct msr_driver_t));
	if (!drv) {
		set_error(ERR_NO_MEM);
		return NULL;
	}
	memset(drv, 0, sizeof(struct msr_driver_t));

	if (!extract_driver(drv)) {
		free(drv);
		set_error(ERR_EXTRACT);
		return NULL;
	}
	status = LoadDriver(drv);
	if (status) {
		drv->hhDriver = CreateFileA("\\\\.\\"OLS_DRIVER_ID,
			GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);
		if (drv->hhDriver == INVALID_HANDLE_VALUE)
			status = FALSE;
	}

	if (!status) {
		set_error(drv->errorcode ? drv->errorcode : ERR_NO_DRIVER);
		free(drv);
		return NULL;
	}
	return drv;
}

int cpu_rdmsr(struct msr_driver_t* driver, uint32_t msr_index, uint64_t* result)
{
	DWORD dwBytesReturned;
	UINT64 MsrData = 0;
	BOOL Res = FALSE;

	if (!driver)
		return set_error(ERR_HANDLE);
	Res = DeviceIoControl(driver->hhDriver, IOCTL_OLS_READ_MSR,
		&msr_index, sizeof(msr_index), &MsrData, sizeof(MsrData), &dwBytesReturned, NULL);
	if (Res == FALSE)
		return -1;
	*result = MsrData;
	return 0;
}

uint8_t io_inb(struct msr_driver_t* drv, uint16_t port)
{
	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return 0;

	DWORD	returnedLength = 0;
	BOOL	result = FALSE;
	WORD	value = 0;

	result = DeviceIoControl(drv->hhDriver, IOCTL_OLS_READ_IO_PORT_BYTE,
		&port, sizeof(port), &value, sizeof(value), &returnedLength, NULL);

	return (uint8_t)value;
}

uint16_t io_inw(struct msr_driver_t* drv, uint16_t port)
{
	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return 0;

	DWORD	returnedLength = 0;
	BOOL	result = FALSE;
	WORD	value = 0;

	result = DeviceIoControl(drv->hhDriver, IOCTL_OLS_READ_IO_PORT_WORD,
		&port, sizeof(port), &value, sizeof(value), &returnedLength, NULL);

	return value;
}

uint32_t io_inl(struct msr_driver_t* drv, uint16_t port)
{
	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return 0;

	DWORD	returnedLength = 0;
	BOOL	result = FALSE;
	DWORD	port4 = port;
	DWORD	value = 0;

	result = DeviceIoControl(drv->hhDriver, IOCTL_OLS_READ_IO_PORT_DWORD,
		&port4, sizeof(port4), &value, sizeof(value), &returnedLength, NULL);

	return value;
}

void io_outb(struct msr_driver_t* drv, uint16_t port, uint8_t value)
{
	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return;

	DWORD	returnedLength = 0;
	BOOL	result = FALSE;
	DWORD   length = 0;
	OLS_WRITE_IO_PORT_INPUT inBuf = { 0 };

	inBuf.CharData = value;
	inBuf.PortNumber = port;
	length = offsetof(OLS_WRITE_IO_PORT_INPUT, CharData) + sizeof(inBuf.CharData);

	result = DeviceIoControl(drv->hhDriver, IOCTL_OLS_WRITE_IO_PORT_BYTE,
		&inBuf, length, NULL, 0, &returnedLength, NULL);
}

void io_outw(struct msr_driver_t* drv, uint16_t port, uint16_t value)
{
	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return;

	DWORD	returnedLength = 0;
	BOOL	result = FALSE;
	DWORD   length = 0;
	OLS_WRITE_IO_PORT_INPUT inBuf = { 0 };

	inBuf.ShortData = value;
	inBuf.PortNumber = port;
	length = offsetof(OLS_WRITE_IO_PORT_INPUT, CharData) + sizeof(inBuf.ShortData);

	result = DeviceIoControl(drv->hhDriver, IOCTL_OLS_WRITE_IO_PORT_WORD,
		&inBuf, length, NULL, 0, &returnedLength, NULL);
}

void io_outl(struct msr_driver_t* drv, uint16_t port, uint32_t value)
{
	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return;

	DWORD	returnedLength = 0;
	BOOL	result = FALSE;
	DWORD   length = 0;
	OLS_WRITE_IO_PORT_INPUT inBuf = { 0 };

	inBuf.LongData = value;
	inBuf.PortNumber = port;
	length = offsetof(OLS_WRITE_IO_PORT_INPUT, CharData) + sizeof(inBuf.LongData);

	result = DeviceIoControl(drv->hhDriver, IOCTL_OLS_WRITE_IO_PORT_DWORD,
		&inBuf, length, NULL, 0, &returnedLength, NULL);
}

DWORD phymem_read(struct msr_driver_t* drv,
	DWORD_PTR address, PBYTE buffer, DWORD count, DWORD unitSize)
{
	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE || !buffer)
		return 0;

	DWORD	returnedLength = 0;
	BOOL	result = FALSE;
	DWORD	size = 0;
	OLS_READ_MEMORY_INPUT inBuf;

	if (sizeof(DWORD_PTR) == 4)
	{
		inBuf.Address.HighPart = 0;
		inBuf.Address.LowPart = (DWORD)address;
	}
	else
	{
		inBuf.Address.QuadPart = address;
	}

	inBuf.UnitSize = unitSize;
	inBuf.Count = count;
	size = inBuf.UnitSize * inBuf.Count;

	result = DeviceIoControl(drv->hhDriver, IOCTL_OLS_READ_MEMORY,
		&inBuf, sizeof(OLS_READ_MEMORY_INPUT), buffer, size, &returnedLength, NULL);

	if (result && returnedLength == size)
		return count * unitSize;
	return 0;
}

int cpu_msr_driver_close(struct msr_driver_t* drv)
{
	SERVICE_STATUS srvStatus = { 0 };
	if (drv == NULL)
		return 0;
	if (drv->hhDriver && drv->hhDriver != INVALID_HANDLE_VALUE) {
		CloseHandle(drv->hhDriver);
		drv->hhDriver = NULL;
		drv->scManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (drv->scManager)
			drv->scDriver = OpenServiceA(drv->scManager, OLS_DRIVER_ID, SERVICE_ALL_ACCESS);
	}
	if (drv->scDriver) {
		ControlService(drv->scDriver, SERVICE_CONTROL_STOP, &srvStatus);
		DeleteService(drv->scDriver);
		CloseServiceHandle(drv->scDriver);
		drv->scDriver = NULL;
	}
	if (drv->scManager) {
		CloseServiceHandle(drv->scManager);
		drv->scManager = NULL;
	}
	free(drv);
	return 0;
}
