// SPDX-License-Identifier: Unlicense
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winioctl.h>
#include <winerror.h>
#include <pathcch.h>
#include <winternl.h>
#include <shlobj.h>
#include "winring0.h"
#include "winring0_priv.h"

static BOOL load_driver(struct wr0_drv_t* drv)
{
	BOOL bServiceCreated = FALSE;

	if (drv->driver_type == WR0_DRIVER_PAWNIO)
		return TRUE;

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

	if (wcscmp(id, PAWNIO_ID) == 0)
	{
		// Program Files\\PawnIO\\PawnIO.sys
		SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, 0, driver->driver_path);
		PathCchAppend(driver->driver_path, MAX_PATH, PAWNIO_ID);
		PathCchAppend(driver->driver_path, MAX_PATH, name);
	}
	else
	{
		GetModuleFileNameW(NULL, driver->driver_path, MAX_PATH);
		PathCchRemoveFileSpec(driver->driver_path, MAX_PATH);
		PathCchAppend(driver->driver_path, MAX_PATH, name);
	}

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

static struct wr0_drv_t*
open_driver_real(LPCWSTR name, LPCWSTR id, enum wr0_driver_type t, LPCWSTR obj, int debug)
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
	drv->driver_type = t;
	drv->debug = debug;
	return drv;
fail:
	free(drv);
	return NULL;
}

static int load_pawnio(struct pio_mod_t* mod, LPCWSTR name, int debug)
{
	WCHAR path[MAX_PATH] = { 0 };
	HANDLE hFile = INVALID_HANDLE_VALUE;

	mod->blob = NULL;
	mod->size = 0;
	mod->hd = INVALID_HANDLE_VALUE;

	GetModuleFileNameW(NULL, path, MAX_PATH);
	PathCchRemoveFileSpec(path, MAX_PATH);
	PathCchAppend(path, MAX_PATH, name);

	hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		goto fail;
	mod->size = GetFileSize(hFile, NULL);
	if (mod->size == INVALID_FILE_SIZE || mod->size == 0)
		goto fail;
	mod->blob = malloc(mod->size);
	if (mod->blob == NULL)
		goto fail;
	if (!ReadFile(hFile, mod->blob, mod->size, &mod->size, NULL) || mod->size == 0)
		goto fail;

	if (debug)
		printf("[PIO] Load file OK %ls\n", name);

	mod->hd = CreateFileW(PAWNIO_OBJ,
		GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, NULL);
	if (mod->hd == INVALID_HANDLE_VALUE)
		goto fail;
	if (!DeviceIoControl(mod->hd, IOCTL_PIO_LOAD_BINARY, mod->blob, mod->size, NULL, 0, NULL, NULL))
		goto fail;

	CloseHandle(hFile);

	if (debug)
		printf("[PIO] Load blob OK %ls size=%lu\n", name, mod->size);
	return 0;

fail:
	if (hFile && hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if (mod->blob)
		free(mod->blob);
	if (mod->hd && mod->hd != INVALID_HANDLE_VALUE)
		CloseHandle(mod->hd);
	mod->blob = NULL;
	mod->size = 0;
	mod->hd = INVALID_HANDLE_VALUE;
	if (debug)
		printf("[PIO] Load blob FAIL %ls\n", name);
	return -1;
}

#define HWRWDRV_MIN_VER 0x01000601

struct wr0_drv_t* WR0_OpenDriver(int debug)
{
	struct wr0_drv_t* drv = NULL;
	DWORD ver = 0;
	if (is_x64())
	{
		drv = open_driver_real(CPUZDRV_NAME_X64, CPUZDRV_ID, WR0_DRIVER_CPUZ161, CPUZDRV_OBJ, debug);
		if (drv)
			return drv;
		drv = open_driver_real(HWIODRV_NAME_X64, HWIODRV_ID, WR0_DRIVER_HWIO, HWIODRV_OBJ, debug);
		if (drv)
			return drv;
		drv = open_driver_real(HWRWDRV_NAME_X64, HWRWDRV_ID, WR0_DRIVER_WINRING0, HWRWDRV_OBJ, debug);
		if (drv)
			return drv;
		drv = open_driver_real(WINRING0_NAME_X64, WINRING0_ID, WR0_DRIVER_WINRING0, WINRING0_OBJ, debug);
		if (drv)
			return drv;
		drv = open_driver_real(PAWNIO_NAME, PAWNIO_ID, WR0_DRIVER_PAWNIO, PAWNIO_OBJ, debug);
		if (drv)
		{
			load_pawnio(&drv->pio_amd0f, L"AMDFamily0F.bin", debug);
			load_pawnio(&drv->pio_amd10, L"AMDFamily10.bin", debug);
			load_pawnio(&drv->pio_amd17, L"AMDFamily17.bin", debug);
			load_pawnio(&drv->pio_intel, L"IntelMSR.bin", debug);
			load_pawnio(&drv->pio_rysmu, L"RyzenSMU.bin", debug);
			return drv;
		}
	}
	else
	{
		drv = open_driver_real(CPUZDRV_NAME, CPUZDRV_ID, WR0_DRIVER_CPUZ161, CPUZDRV_OBJ, debug);
		if (drv)
			return drv;
		drv = open_driver_real(HWIODRV_NAME, HWIODRV_ID, WR0_DRIVER_HWIO, HWIODRV_OBJ, debug);
		if (drv)
			return drv;
		drv = open_driver_real(HWRWDRV_NAME, HWRWDRV_ID, WR0_DRIVER_WINRING0, HWRWDRV_OBJ, debug);
		if (drv)
			return drv;
		drv = open_driver_real(WINRING0_NAME, WINRING0_ID, WR0_DRIVER_WINRING0, WINRING0_OBJ, debug);
	}
	return drv;
}

int WR0_RdMsr(struct wr0_drv_t* drv, uint32_t msr_index, uint64_t* result)
{
	DWORD dwBytesReturned;
	UINT64 msrData = 0;
	BOOL bRes = FALSE;
	DWORD ctlCode;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return -1;

	switch (drv->driver_type)
	{
	case WR0_DRIVER_WINRING0:
		ctlCode = IOCTL_OLS_READ_MSR;
		break;
	case WR0_DRIVER_CPUZ161:
		ctlCode = IOCTL_CPUZ_READ_MSR;
		break;
	case WR0_DRIVER_HWIO:
		ctlCode = IOCTL_HIO_READ_MSR;
		break;
	default:
		return -1;
	}

	bRes = DeviceIoControl(drv->hhDriver, ctlCode,
		&msr_index, sizeof(msr_index), &msrData, sizeof(msrData), &dwBytesReturned, NULL);
	if (bRes == FALSE)
		return -1;
	*result = msrData;
	return 0;
}

int WR0_WrMsr(struct wr0_drv_t* drv, uint32_t msr_index, DWORD eax, DWORD edx)
{
	DWORD dwBytesReturned = 0;
	DWORD outBuf;
	OLS_WRITE_MSR_INPUT inBuf;
	BOOL bRes = FALSE;
	DWORD ctlCode;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return -1;

	switch (drv->driver_type)
	{
	case WR0_DRIVER_WINRING0:
		ctlCode = IOCTL_OLS_WRITE_MSR;
		break;
	case WR0_DRIVER_CPUZ161:
		ctlCode = IOCTL_CPUZ_WRITE_MSR;
		break;
	case WR0_DRIVER_HWIO:
		ctlCode = IOCTL_HIO_WRITE_MSR;
		break;
	default:
		return -1;
	}

	inBuf.Register = msr_index;
	inBuf.Value.HighPart = edx;
	inBuf.Value.LowPart = eax;

	bRes = DeviceIoControl(drv->hhDriver, ctlCode,
		&inBuf, sizeof(inBuf), &outBuf, sizeof(outBuf), &dwBytesReturned, NULL);
	if (bRes == FALSE)
		return -1;
	return 0;
}

uint8_t WR0_RdIo8(struct wr0_drv_t* drv, uint16_t port)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	WORD value = 0;
	DWORD ctlCode;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return 0;

	switch (drv->driver_type)
	{
	case WR0_DRIVER_WINRING0:
		ctlCode = IOCTL_OLS_READ_IO_PORT_BYTE;
		break;
	case WR0_DRIVER_CPUZ161:
		ctlCode = IOCTL_CPUZ_READ_IO_PORT_BYTE;
		break;
	case WR0_DRIVER_HWIO:
		ctlCode = IOCTL_HIO_READ_IO_PORT;
		break;
	default:
		return 0;
	}

	result = DeviceIoControl(drv->hhDriver, ctlCode,
		&port, sizeof(port), &value, sizeof(value), &returnedLength, NULL);

	return (uint8_t)value;
}

uint16_t WR0_RdIo16(struct wr0_drv_t* drv, uint16_t port)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	WORD value = 0;
	DWORD ctlCode;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return 0;

	switch (drv->driver_type)
	{
	case WR0_DRIVER_WINRING0:
		ctlCode = IOCTL_OLS_READ_IO_PORT_WORD;
		break;
	case WR0_DRIVER_CPUZ161:
		ctlCode = IOCTL_CPUZ_READ_IO_PORT_WORD;
		break;
	case WR0_DRIVER_HWIO:
		ctlCode = IOCTL_HIO_READ_IO_PORT;
		break;
	default:
		return 0;
	}

	result = DeviceIoControl(drv->hhDriver, ctlCode,
		&port, sizeof(port), &value, sizeof(value), &returnedLength, NULL);

	return value;
}

uint32_t WR0_RdIo32(struct wr0_drv_t* drv, uint16_t port)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	DWORD port4 = port;
	DWORD value = 0;
	DWORD ctlCode;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return 0;

	switch (drv->driver_type)
	{
	case WR0_DRIVER_WINRING0:
		ctlCode = IOCTL_OLS_READ_IO_PORT_DWORD;
		break;
	case WR0_DRIVER_CPUZ161:
		ctlCode = IOCTL_CPUZ_READ_IO_PORT_DWORD;
		break;
	case WR0_DRIVER_HWIO:
		ctlCode = IOCTL_HIO_READ_IO_PORT;
		break;
	default:
		return 0;
	}

	result = DeviceIoControl(drv->hhDriver, ctlCode,
		&port4, sizeof(port4), &value, sizeof(value), &returnedLength, NULL);

	return value;
}

void WR0_WrIo8(struct wr0_drv_t* drv, uint16_t port, uint8_t value)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	DWORD length = 0;
	OLS_WRITE_IO_PORT_INPUT inBuf = { 0 };
	DWORD ctlCode;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return;

	switch (drv->driver_type)
	{
	case WR0_DRIVER_WINRING0:
		ctlCode = IOCTL_OLS_WRITE_IO_PORT_BYTE;
		break;
	case WR0_DRIVER_CPUZ161:
		ctlCode = IOCTL_CPUZ_WRITE_IO_PORT_BYTE;
		break;
	case WR0_DRIVER_HWIO:
		ctlCode = IOCTL_HIO_WRITE_IO_PORT;
		break;
	default:
		return;
	}

	inBuf.CharData = value;
	inBuf.PortNumber = port;
	length = offsetof(OLS_WRITE_IO_PORT_INPUT, CharData) + sizeof(inBuf.CharData);

	result = DeviceIoControl(drv->hhDriver, ctlCode,
		&inBuf, length, NULL, 0, &returnedLength, NULL);
}

void WR0_WrIo16(struct wr0_drv_t* drv, uint16_t port, uint16_t value)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	DWORD length = 0;
	OLS_WRITE_IO_PORT_INPUT inBuf = { 0 };
	DWORD ctlCode;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return;

	switch (drv->driver_type)
	{
	case WR0_DRIVER_WINRING0:
		ctlCode = IOCTL_OLS_WRITE_IO_PORT_WORD;
		break;
	case WR0_DRIVER_CPUZ161:
		ctlCode = IOCTL_CPUZ_WRITE_IO_PORT_WORD;
		break;
	case WR0_DRIVER_HWIO:
		ctlCode = IOCTL_HIO_WRITE_IO_PORT;
		break;
	default:
		return;
	}

	inBuf.ShortData = value;
	inBuf.PortNumber = port;
	length = offsetof(OLS_WRITE_IO_PORT_INPUT, CharData) + sizeof(inBuf.ShortData);

	result = DeviceIoControl(drv->hhDriver, ctlCode,
		&inBuf, length, NULL, 0, &returnedLength, NULL);
}

void WR0_WrIo32(struct wr0_drv_t* drv, uint16_t port, uint32_t value)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	DWORD length = 0;
	OLS_WRITE_IO_PORT_INPUT inBuf = { 0 };
	DWORD ctlCode;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return;

	switch (drv->driver_type)
	{
	case WR0_DRIVER_WINRING0:
		ctlCode = IOCTL_OLS_WRITE_IO_PORT_DWORD;
		break;
	case WR0_DRIVER_CPUZ161:
		ctlCode = IOCTL_CPUZ_WRITE_IO_PORT_DWORD;
		break;
	case WR0_DRIVER_HWIO:
		ctlCode = IOCTL_HIO_WRITE_IO_PORT;
		break;
	default:
		return;
	}

	inBuf.LongData = value;
	inBuf.PortNumber = port;
	length = offsetof(OLS_WRITE_IO_PORT_INPUT, CharData) + sizeof(inBuf.LongData);

	result = DeviceIoControl(drv->hhDriver, ctlCode,
		&inBuf, length, NULL, 0, &returnedLength, NULL);
}

int WR0_RdPciConf(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, void* value, uint32_t size)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE
		|| !value || (size == 2 && (reg & 1) != 0) || (size == 4 && (reg & 3) != 0))
		return -1;

	switch (drv->driver_type)
	{
	case WR0_DRIVER_HWIO:
	{
		OLS_READ_PCI_CONFIG_INPUT inBuf = { 0 };
		inBuf.PciAddress = addr;
		inBuf.PciOffset = reg;
		result = DeviceIoControl(drv->hhDriver, IOCTL_HIO_READ_PCI_CONFIG,
			&inBuf, sizeof(inBuf), value, size, &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_WINRING0:
	{
		OLS_READ_PCI_CONFIG_INPUT inBuf = { 0 };
		inBuf.PciAddress = addr;
		inBuf.PciOffset = reg;
		result = DeviceIoControl(drv->hhDriver, IOCTL_OLS_READ_PCI_CONFIG,
			&inBuf, sizeof(inBuf), value, size, &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_CPUZ161:
	{
		CPUZ_READ_PCI_CONFIG_INPUT inBuf = { 0 };
		CPUZ_READ_PCI_CONFIG_OUTPUT outBuf = { 0 };
		inBuf.Bus = PciGetBus(addr);
		inBuf.Device = PciGetDev(addr);
		inBuf.Function = PciGetFunc(addr);
		inBuf.Offset = reg;
		inBuf.Length = size;
		result = DeviceIoControl(drv->hhDriver, IOCTL_CPUZ_READ_PCI_CONFIG,
			&inBuf, sizeof(inBuf), &outBuf, sizeof(outBuf), &returnedLength, NULL);
		if (result)
			memcpy(value, outBuf.DataByte, size);
	}
		break;
	default:
		return -1;
	}

	return result ? 0 : -2;
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
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	int inputSize = 0;
	OLS_WRITE_PCI_CONFIG_INPUT* inBuf;
	DWORD ctlCode;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE
		|| !value || (size == 2 && (reg & 1) != 0) || (size == 4 && (reg & 3) != 0))
		return -1;

	switch (drv->driver_type)
	{
	case WR0_DRIVER_WINRING0:
		ctlCode = IOCTL_OLS_WRITE_PCI_CONFIG;
		break;
	case WR0_DRIVER_HWIO:
		ctlCode = IOCTL_HIO_WRITE_PCI_CONFIG;
		break;
	default:
		return -1;
	}

	inputSize = offsetof(OLS_WRITE_PCI_CONFIG_INPUT, Data) + size;
	inBuf = (OLS_WRITE_PCI_CONFIG_INPUT*)malloc(inputSize);
	if (inBuf == NULL)
		return -1;
	memcpy(inBuf->Data, value, size);
	inBuf->PciAddress = addr;
	inBuf->PciOffset = reg;
	result = DeviceIoControl(drv->hhDriver, ctlCode,
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
	BOOL mfFlag = FALSE;
	uint8_t type = 0;
	uint8_t count = 0;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE || drv->driver_type == WR0_DRIVER_PAWNIO
		|| vid == 0xFFFF)
		return addr;

	for (uint8_t bus = 0; bus <= 7; bus++)
	{
		for (uint8_t dev = 0; dev < 32; dev++)
		{
			mfFlag = FALSE;
			for (uint8_t func = 0; func < 8; func++)
			{
				if (!mfFlag && func > 0)
					break;
				addr = PciBusDevFunc(bus, dev, func);
				if (WR0_RdPciConf(drv, addr, 0, &id, sizeof(id)) == 0)
				{
					// Is Multi Function Device
					if (func == 0
						&& WR0_RdPciConf(drv, addr, 0x0E, &type, sizeof(type)) == 0)
					{
						if (type & 0x80)
							mfFlag = TRUE;
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
	BOOL mfFlag = FALSE;
	uint8_t type = 0;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE || drv->driver_type == WR0_DRIVER_PAWNIO)
		return addr;

	for (bus = 0; bus <= 7; bus++)
	{
		for (dev = 0; dev < 32; dev++)
		{
			mfFlag = FALSE;
			for (func = 0; func < 8; func++)
			{
				if (mfFlag == FALSE && func > 0)
					break;
				addr = PciBusDevFunc(bus, dev, func);
				if (WR0_RdPciConf(drv, addr, 0, conf, sizeof(conf)) == 0)
				{
					// Is Multi Function Device
					if (func == 0
						&& WR0_RdPciConf(drv, addr, 0x0E, &type, sizeof(type)) == 0)
					{
						if (type & 0x80)
							mfFlag = TRUE;
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

int WR0_RdMmIo(struct wr0_drv_t* drv, uint64_t addr, void* value, uint32_t size)
{
	DWORD returnedLength = 0;
	BOOL result = TRUE;
	DWORD length = 0;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE)
		return -1;

	switch (drv->driver_type)
	{
	case WR0_DRIVER_HWIO:
		result = DeviceIoControl(drv->hhDriver, IOCTL_HIO_READ_MMIO,
			&addr, sizeof(uint64_t), value, size, &returnedLength, NULL);
		break;
	case WR0_DRIVER_WINRING0:
	default:
		returnedLength = WR0_RdMem(drv, (DWORD_PTR)addr, (PBYTE)value, 1, size);
		break;
	}

	if (result && returnedLength == size)
		return 0;
	return -2;
}

DWORD WR0_RdMem(struct wr0_drv_t* drv,
	DWORD_PTR address, PBYTE buffer, DWORD count, DWORD unitSize)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	DWORD size = count * unitSize;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE
		|| !buffer || count == 0)
		return 0;

	switch (drv->driver_type)
	{
	case WR0_DRIVER_WINRING0:
	{
		OLS_READ_MEMORY_INPUT inBuf;
		inBuf.Address.QuadPart = address;
		inBuf.UnitSize = unitSize;
		inBuf.Count = count;
		result = DeviceIoControl(drv->hhDriver, IOCTL_OLS_READ_MEMORY,
			&inBuf, sizeof(OLS_READ_MEMORY_INPUT), buffer, size, &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_CPUZ161:
	{
		DWORD inBuf[3];
		CPUZ_READ_MEMORY_OUTPUT* outBuf = malloc(sizeof(CPUZ_READ_MEMORY_OUTPUT) + size);
		if (!outBuf)
			return 0;
#ifdef _WIN64
		inBuf[0] = (DWORD)(address >> 32U);
#else
		inBuf[0] = 0;
#endif
		inBuf[1] = (DWORD)(address & 0xFFFFFFFF);
		inBuf[2] = size;
		result = DeviceIoControl(drv->hhDriver, IOCTL_CPUZ_READ_MEMORY,
			inBuf, sizeof(inBuf), outBuf, sizeof(CPUZ_READ_MEMORY_OUTPUT) + size, &returnedLength, NULL);
		memcpy(buffer, outBuf->Data, size);
		free(outBuf);
		returnedLength = size;
	}
		break;
	default:
		return 0;
	}

	if (result && returnedLength == size)
		return count * unitSize;
	return 0;
}


// smn=1 -> 0xB8/0xBC AMD 15h Temperature
// smn=2 -> 0xC4/0xC8 Zen SMU
// smn=3 -> 0x60/0x64 Zen Temperature
DWORD WR0_RdAmdSmn(struct wr0_drv_t* drv, DWORD bdf, DWORD smn, DWORD reg)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	DWORD inBuf[3];
	DWORD value = 0;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE || drv->driver_type != WR0_DRIVER_CPUZ161)
		return 0;

	inBuf[0] = bdf;
	inBuf[1] = smn;
	inBuf[2] = reg;

	result = DeviceIoControl(drv->hhDriver, IOCTL_CPUZ_READ_AMD_SMN,
		inBuf, sizeof(inBuf), &value, sizeof(value), &returnedLength, NULL);
	if (drv->debug)
		printf("[SMN] Read AMD SMN reg=%08xh %d value=%08xh\n", reg, result, value);

	return value;
}

int
WR0_SendSmuCmd(struct wr0_drv_t* drv, uint32_t cmd, uint32_t rsp, uint32_t arg, uint32_t fn, uint32_t args[6])
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	uint32_t inBuf[17] = { 0 };
	uint32_t outBuf[7] = { 0 };

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE || drv->driver_type != WR0_DRIVER_CPUZ161)
		return -1;

	inBuf[0] = 0; // BDF
	inBuf[1] = 2; // Zen SMU
	inBuf[2] = fn;
	inBuf[3] = cmd;
	inBuf[4] = rsp;
	for (uint32_t i = 0; i < 6; i++)
	{
		inBuf[5 + i] = arg + i * 4;
		inBuf[11 + i] = args[i];
	}
	result = DeviceIoControl(drv->hhDriver, IOCTL_CPUZ_SEND_SMN_CMD,
		inBuf, sizeof(inBuf), outBuf, sizeof(outBuf), &returnedLength, NULL);
	if (drv->debug)
		printf("[SMU] Send SMU fn=%08xh %d -> %u\n", fn, result, outBuf[0]);
	memcpy(args, &outBuf[1], 6 * sizeof(uint32_t));
	if (result && outBuf[0] == 1)
		return 0;
	return -2;
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

	if (drv->debug)
	{
		printf("[PIO] Exec %s %s", bRes ? "OK" : "FAIL", fn);
		for (SIZE_T i = 0; in && i < min(in_size, 4); i++)
			printf(" in[%zu]=%llxh", i, in[i]);
		for (SIZE_T i = 0; out && i < min(out_size,4); i++)
			printf(" out[%zu]=%llxh", i, out[i]);
		printf(" ret=%lu\n", returnedLength);
	}

	if (bRes == FALSE)
		return -1;

	if (return_size)
		*return_size = returnedLength / sizeof(*out);
	return 0;
}

static void unload_pawnio(struct pio_mod_t* mod)
{
	if (mod->blob)
		free(mod->blob);
	if (mod->hd && mod->hd != INVALID_HANDLE_VALUE)
		CloseHandle(mod->hd);
	mod->blob = NULL;
	mod->size = 0;
	mod->hd = INVALID_HANDLE_VALUE;
}

int WR0_CloseDriver(struct wr0_drv_t* drv)
{
	SERVICE_STATUS srvStatus = { 0 };
	if (drv == NULL)
		return 0;

	if (drv->driver_type == WR0_DRIVER_PAWNIO)
	{
		unload_pawnio(&drv->pio_amd0f);
		unload_pawnio(&drv->pio_amd10);
		unload_pawnio(&drv->pio_amd17);
		unload_pawnio(&drv->pio_intel);
		unload_pawnio(&drv->pio_rysmu);
		free(drv);
		return 0;
	}

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
