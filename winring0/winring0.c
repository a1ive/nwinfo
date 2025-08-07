// SPDX-License-Identifier: Unlicense
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winioctl.h>
#include <winerror.h>
#include <pathcch.h>
#include "winring0.h"
#include "winring0_def.h"

static BOOL load_driver(struct wr0_drv_t* drv)
{
	BOOL bServiceCreated = FALSE;

	drv->scManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (drv->scManager == NULL)
		return FALSE;

	drv->scDriver = CreateServiceW(drv->scManager, drv->driver_id, drv->driver_id,
		SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE,
		drv->driver_path, NULL, NULL, NULL, NULL, NULL);
	if (drv->scDriver == NULL)
	{
		if (GetLastError() == ERROR_SERVICE_EXISTS)
		{
			drv->scDriver = OpenServiceW(drv->scManager, drv->driver_id, SERVICE_ALL_ACCESS);
			if (drv->scDriver == NULL)
			{
				CloseServiceHandle(drv->scManager);
				drv->scManager = NULL;
				return FALSE;
			}
		}
		else
		{
			CloseServiceHandle(drv->scManager);
			drv->scManager = NULL;
			return FALSE;
		}
	}
	else
		bServiceCreated = TRUE;

	if (!StartServiceW(drv->scDriver, 0, NULL))
	{
		if (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
		{
			if (bServiceCreated)
				DeleteService(drv->scDriver);
			CloseServiceHandle(drv->scDriver);
			drv->scDriver = NULL;
			CloseServiceHandle(drv->scManager);
			drv->scManager = NULL;
			return FALSE;
		}
	}

	return TRUE;
}

typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
static BOOL is_x64(void)
{
#ifdef _WIN64
	return TRUE;
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

static BOOL find_driver(struct wr0_drv_t* driver, LPCWSTR name, LPCWSTR id, LPCWSTR obj)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;

	GetModuleFileNameW(NULL, driver->driver_path, MAX_PATH);

	PathCchRemoveFileSpec(driver->driver_path, MAX_PATH);
	PathCchAppend(driver->driver_path, MAX_PATH, name);
	hFile = CreateFileW(driver->driver_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		driver->driver_id = id;
		driver->driver_name = name;
		driver->driver_obj = obj;
		CloseHandle(hFile);
		return TRUE;
	}

	ZeroMemory(driver->driver_path, sizeof(driver->driver_path));
	return FALSE;
}

struct wr0_drv_t* wr0_driver_open_real(LPCWSTR name, LPCWSTR id, LPCWSTR obj)
{
	struct wr0_drv_t* drv;
	BOOL status = FALSE;

	drv = (struct wr0_drv_t*)malloc(sizeof(struct wr0_drv_t));
	if (!drv)
		return NULL;
	ZeroMemory(drv, sizeof(struct wr0_drv_t));

	if (!find_driver(drv, name, id, obj))
		goto fail;
	status = load_driver(drv);
	if (status)
	{
		drv->hhDriver = CreateFileW(drv->driver_obj,
			GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);
		if (drv->hhDriver == INVALID_HANDLE_VALUE)
			status = FALSE;
	}

	if (!status)
		goto fail;
	return drv;
fail:
	free(drv);
	return NULL;
}

struct wr0_drv_t* WR0_OpenDriver(void)
{
	struct wr0_drv_t* drv = NULL;
	if (is_x64())
	{
		drv = wr0_driver_open_real(OLS_DRIVER_NAME_X64, OLS_DRIVER_ID, OLS_DRIVER_OBJ);
		if (drv)
			return drv;
		drv = wr0_driver_open_real(OLS_DRIVER_NAME_WIN7_X64, OLS_DRIVER_ID, OLS_DRIVER_OBJ);
		if (drv)
			return drv;
		drv = wr0_driver_open_real(OLS_ALT_DRIVER_NAME_X64, OLS_ALT_DRIVER_ID, OLS_ALT_DRIVER_OBJ);
	}
	else
	{
		drv = wr0_driver_open_real(OLS_DRIVER_NAME, OLS_DRIVER_ID, OLS_DRIVER_OBJ);
		if (drv)
			return drv;
		drv = wr0_driver_open_real(OLS_ALT_DRIVER_NAME, OLS_ALT_DRIVER_ID, OLS_ALT_DRIVER_OBJ);
	}
	return drv;
}

int WR0_RdMsr(struct wr0_drv_t* driver, uint32_t msr_index, uint64_t* result)
{
	DWORD dwBytesReturned;
	UINT64 MsrData = 0;
	BOOL Res = FALSE;

	if (!driver)
		return -1;
	Res = DeviceIoControl(driver->hhDriver, IOCTL_OLS_READ_MSR,
		&msr_index, sizeof(msr_index), &MsrData, sizeof(MsrData), &dwBytesReturned, NULL);
	if (Res == FALSE)
		return -1;
	*result = MsrData;
	return 0;
}

int WR0_WrMsr(struct wr0_drv_t* driver, uint32_t msr_index, DWORD eax, DWORD edx)
{
	DWORD dwBytesReturned = 0;
	DWORD outBuf;
	OLS_WRITE_MSR_INPUT inBuf;
	BOOL Res = FALSE;

	if (!driver)
		return -1;

	inBuf.Register = msr_index;
	inBuf.Value.HighPart = edx;
	inBuf.Value.LowPart = eax;

	Res = DeviceIoControl(driver->hhDriver, IOCTL_OLS_WRITE_MSR,
		&inBuf, sizeof(inBuf), &outBuf, sizeof(outBuf), &dwBytesReturned, NULL);
	if (Res == FALSE)
		return -1;
	return 0;
}

uint8_t WR0_RdIo8(struct wr0_drv_t* drv, uint16_t port)
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

uint16_t WR0_RdIo16(struct wr0_drv_t* drv, uint16_t port)
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

uint32_t WR0_RdIo32(struct wr0_drv_t* drv, uint16_t port)
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

void WR0_WrIo8(struct wr0_drv_t* drv, uint16_t port, uint8_t value)
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

void WR0_WrIo16(struct wr0_drv_t* drv, uint16_t port, uint16_t value)
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

void WR0_WrIo32(struct wr0_drv_t* drv, uint16_t port, uint32_t value)
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

int WR0_RdPciConf(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, void* value, uint32_t size)
{
	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE
		|| !value || (size == 2 && (reg & 1) != 0) || (size == 4 && (reg & 3) != 0))
		return -1;

	DWORD returnedLength = 0;
	BOOL result = FALSE;
	OLS_READ_PCI_CONFIG_INPUT inBuf = { 0 };

	inBuf.PciAddress = addr;
	inBuf.PciOffset = reg;

	result = DeviceIoControl(drv->hhDriver, IOCTL_OLS_READ_PCI_CONFIG,
		&inBuf, sizeof(inBuf), value, size, &returnedLength, NULL);

	if (result)
		return 0;
	return -2;
}

uint8_t WR0_RdPciConf8(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg)
{
	uint8_t ret;
	if (WR0_RdPciConf(drv, addr, reg, &ret, sizeof(ret)) == 0)
		return ret;
	return 0xFF;
}

uint16_t WR0_RdPciConf16(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg)
{
	uint16_t ret;
	if (WR0_RdPciConf(drv, addr, reg, &ret, sizeof(ret)) == 0)
		return ret;
	return 0xFFFF;
}

uint32_t WR0_RdPciConf32(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg)
{
	uint32_t ret;
	if (WR0_RdPciConf(drv, addr, reg, &ret, sizeof(ret)) == 0)
		return ret;
	return 0xFFFFFFFF;
}

int WR0_WrPciConf(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, void* value, uint32_t size)
{
	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE
		|| !value || (size == 2 && (reg & 1) != 0) || (size == 4 && (reg & 3) != 0))
		return -1;

	DWORD returnedLength = 0;
	BOOL result = FALSE;
	int inputSize = 0;
	OLS_WRITE_PCI_CONFIG_INPUT* inBuf;

	inputSize = offsetof(OLS_WRITE_PCI_CONFIG_INPUT, Data) + size;
	inBuf = (OLS_WRITE_PCI_CONFIG_INPUT*)malloc(inputSize);
	if (inBuf == NULL)
		return -1;
	memcpy(inBuf->Data, value, size);
	inBuf->PciAddress = addr;
	inBuf->PciOffset = reg;
	result = DeviceIoControl(drv->hhDriver, IOCTL_OLS_WRITE_PCI_CONFIG,
		inBuf, inputSize, NULL, 0, &returnedLength, NULL);
	free(inBuf);

	if (result)
		return 0;
	return -2;
}

void WR0_WrPciConf8(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, uint8_t value)
{
	WR0_WrPciConf(drv, addr, reg, &value, sizeof(value));
}

void WR0_WrPciConf16(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, uint16_t value)
{
	WR0_WrPciConf(drv, addr, reg, &value, sizeof(value));
}

void WR0_WrPciConf32(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, uint32_t value)
{
	WR0_WrPciConf(drv, addr, reg, &value, sizeof(value));
}

uint32_t WR0_FindPciById(struct wr0_drv_t* drv, uint16_t vid, uint16_t did, uint8_t index)
{
	uint32_t addr = 0xFFFFFFFF;
	uint64_t id = 0;
	BOOL multi_func_flag = FALSE;
	uint8_t type = 0;
	uint8_t count = 0;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE || vid == 0xFFFF)
		return addr;

	for (uint8_t bus = 0; bus <= 7; bus++)
	{
		for (uint8_t dev = 0; dev < 32; dev++)
		{
			multi_func_flag = FALSE;
			for (uint8_t func = 0; func < 8; func++)
			{
				if (!multi_func_flag && func > 0)
					break;
				addr = PciBusDevFunc(bus, dev, func);
				if (WR0_RdPciConf(drv, addr, 0, &id, sizeof(id)) == 0)
				{
					// Is Multi Function Device
					if (func == 0
						&& WR0_RdPciConf(drv, addr, 0x0E, &type, sizeof(type)) == 0)
					{
						if (type & 0x80)
							multi_func_flag = TRUE;
					}
					if (id == (vid | (((uint32_t)did) << 16)))
					{
						if (count == index)
							return addr;
						count++;
						continue;
					}
				}
			}
		}
	}
	addr = 0xFFFFFFFF;
	return addr;
}

uint32_t WR0_FindPciByClass(struct wr0_drv_t* drv, uint8_t base, uint8_t sub, uint8_t prog, uint8_t index)
{
	uint32_t bus = 0, dev = 0, func = 0;
	uint32_t count = 0;
	uint32_t addr = 0xFFFFFFFF;
	uint32_t conf[3] = { 0 };
	BOOL multi_func_flag = FALSE;
	uint8_t type = 0;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return addr;

	for (bus = 0; bus <= 7; bus++)
	{
		for (dev = 0; dev < 32; dev++)
		{
			multi_func_flag = FALSE;
			for (func = 0; func < 8; func++)
			{
				if (multi_func_flag == FALSE && func > 0)
					break;
				addr = PciBusDevFunc(bus, dev, func);
				if (WR0_RdPciConf(drv, addr, 0, conf, sizeof(conf)) == 0)
				{
					// Is Multi Function Device
					if (func == 0
						&& WR0_RdPciConf(drv, addr, 0x0E, &type, sizeof(type)) == 0)
					{
						if (type & 0x80)
							multi_func_flag = TRUE;
					}
					if ((conf[2] & 0xFFFFFF00) ==
						(((DWORD)base << 24) | ((DWORD)sub << 16) | ((DWORD)prog << 8)))
					{
						if (count == index)
							return addr;
						count++;
						continue;
					}
				}
			}
		}
	}

	return 0xFFFFFFFF;
}

DWORD WR0_RdMem(struct wr0_drv_t* drv,
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

int WR0_CloseDriver(struct wr0_drv_t* drv)
{
	SERVICE_STATUS srvStatus = { 0 };
	if (drv == NULL)
		return 0;
	if (drv->hhDriver && drv->hhDriver != INVALID_HANDLE_VALUE)
	{
		CloseHandle(drv->hhDriver);
		drv->hhDriver = NULL;
	}
	if (drv->scDriver)
	{
		ControlService(drv->scDriver, SERVICE_CONTROL_STOP, &srvStatus);
		DeleteService(drv->scDriver);
		CloseServiceHandle(drv->scDriver);
		drv->scDriver = NULL;
	}
	if (drv->scManager)
	{
		CloseServiceHandle(drv->scManager);
		drv->scManager = NULL;
	}
	free(drv);
	return 0;
}
