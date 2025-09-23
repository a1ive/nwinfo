// SPDX-License-Identifier: Unlicense
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winioctl.h>
#include <winerror.h>
#include <pathcch.h>
#include <winternl.h>
#include "winring0.h"
#include "winring0_priv.h"

static UINT32 get_driver_version(LPCWSTR name)
{
	DWORD dwHandle = 0;
	DWORD dwVersionInfoSize = 0;
	LPVOID pVersionInfo = NULL;
	UINT32 resultVersion = 0;
	WCHAR wchPath[MAX_PATH] = { 0 };

	GetModuleFileNameW(NULL, wchPath, MAX_PATH);

	PathCchRemoveFileSpec(wchPath, MAX_PATH);
	PathCchAppend(wchPath, MAX_PATH, name);

	dwVersionInfoSize = GetFileVersionInfoSizeW(wchPath, &dwHandle);
	if (dwVersionInfoSize == 0)
		return 0;

	pVersionInfo = HeapAlloc(GetProcessHeap(), 0, dwVersionInfoSize);
	if (pVersionInfo == NULL)
		return 0;

	if (!GetFileVersionInfoW(wchPath, 0, dwVersionInfoSize, pVersionInfo))
		goto out;

	VS_FIXEDFILEINFO* pFileInfo = NULL;
	UINT uiFileInfoSize = 0;
	if (!VerQueryValueW(pVersionInfo, L"\\", (LPVOID*)&pFileInfo, &uiFileInfoSize))
		goto out;
	if (!pFileInfo)
		goto out;

	WORD major = HIWORD(pFileInfo->dwFileVersionMS);
	WORD minor = LOWORD(pFileInfo->dwFileVersionMS);
	WORD build = HIWORD(pFileInfo->dwFileVersionLS);
	WORD revision = LOWORD(pFileInfo->dwFileVersionLS);

	resultVersion = ((major & 0xFF) << 24) |
		((minor & 0xFF) << 16) |
		((build & 0xFF) << 8) |
		(revision & 0xFF);

out:
	HeapFree(GetProcessHeap(), 0, pVersionInfo);
	return resultVersion;
}

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

#define HWRWDRV_MIN_VER 0x01000601

struct wr0_drv_t* WR0_OpenDriver(int debug)
{
	struct wr0_drv_t* drv = NULL;
	DWORD ver = 0;
	if (is_x64())
	{
#ifdef ENABLE_PAWNIO
		drv = open_driver_real(PAWNIO_NAME_X64, PAWNIO_ID, WR0_DRIVER_PAWNIO, PAWNIO_OBJ, debug);
		if (drv)
			return drv;
#endif
		ver = get_driver_version(HWRWDRV_NAME_X64);
		if (ver >= HWRWDRV_MIN_VER)
			drv = open_driver_real(HWRWDRV_NAME_X64, HWRWDRV_ID, WR0_DRIVER_HWRWDRV, HWRWDRV_OBJ, debug);
		else
			drv = open_driver_real(HWRWDRV_NAME_X64, HWRWDRV_ID, WR0_DRIVER_WINRING0, HWRWDRV_OBJ, debug);
		if (drv)
			return drv;
		drv = open_driver_real(WINRING0_NAME_X64, WINRING0_ID, WR0_DRIVER_WINRING0, WINRING0_OBJ, debug);
	}
	else
	{
		ver = get_driver_version(HWRWDRV_NAME);
		if (ver >= HWRWDRV_MIN_VER)
			drv = open_driver_real(HWRWDRV_NAME, HWRWDRV_ID, WR0_DRIVER_HWRWDRV, HWRWDRV_OBJ, debug);
		else
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
	case WR0_DRIVER_HWRWDRV:
		ctlCode = IOCTL_HRD_READ_MSR;
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
	case WR0_DRIVER_HWRWDRV:
		ctlCode = IOCTL_HRD_WRITE_MSR;
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
	case WR0_DRIVER_HWRWDRV:
		ctlCode = IOCTL_HRD_READ_IO_PORT_BYTE;
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
	case WR0_DRIVER_HWRWDRV:
		ctlCode = IOCTL_HRD_READ_IO_PORT_WORD;
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
	case WR0_DRIVER_HWRWDRV:
		ctlCode = IOCTL_HRD_READ_IO_PORT_DWORD;
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
	case WR0_DRIVER_HWRWDRV:
		ctlCode = IOCTL_HRD_WRITE_IO_PORT_BYTE;
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
	case WR0_DRIVER_HWRWDRV:
		ctlCode = IOCTL_HRD_WRITE_IO_PORT_WORD;
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
	case WR0_DRIVER_HWRWDRV:
		ctlCode = IOCTL_HRD_WRITE_IO_PORT_DWORD;
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
	OLS_READ_PCI_CONFIG_INPUT inBuf = { 0 };
	DWORD ctlCode;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE
		|| !value || (size == 2 && (reg & 1) != 0) || (size == 4 && (reg & 3) != 0))
		return -1;

	switch (drv->driver_type)
	{
	case WR0_DRIVER_WINRING0:
		ctlCode = IOCTL_OLS_READ_PCI_CONFIG;
		break;
	case WR0_DRIVER_HWRWDRV:
		ctlCode = IOCTL_HRD_READ_PCI_CONFIG;
		break;
	default:
		return -1;
	}

	inBuf.PciAddress = addr;
	inBuf.PciOffset = reg;

	result = DeviceIoControl(drv->hhDriver, ctlCode,
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
	case WR0_DRIVER_HWRWDRV:
		ctlCode = IOCTL_HRD_WRITE_PCI_CONFIG;
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

DWORD WR0_RdMem(struct wr0_drv_t* drv,
	DWORD_PTR address, PBYTE buffer, DWORD count, DWORD unitSize)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	DWORD size = 0;
	OLS_READ_MEMORY_INPUT inBuf;
	DWORD ctlCode;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE
		|| !buffer)
		return 0;

	switch (drv->driver_type)
	{
	case WR0_DRIVER_WINRING0:
		ctlCode = IOCTL_OLS_READ_MEMORY;
		break;
	case WR0_DRIVER_HWRWDRV:
		ctlCode = IOCTL_HRD_READ_MEMORY;
		break;
	default:
		return 0;
	}

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

	result = DeviceIoControl(drv->hhDriver, ctlCode,
		&inBuf, sizeof(OLS_READ_MEMORY_INPUT), buffer, size, &returnedLength, NULL);

	if (result && returnedLength == size)
		return count * unitSize;
	return 0;
}

#ifdef ENABLE_PAWNIO

int WR0_LoadPawn(struct wr0_drv_t* drv, PVOID blob, DWORD size)
{
	BOOL bRes = FALSE;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE || drv->driver_type != WR0_DRIVER_PAWNIO)
		return -1;
	bRes = DeviceIoControl(drv->hhDriver, IOCTL_PIO_LOAD_BINARY, blob, size, NULL, 0, NULL, NULL);
	if (bRes == FALSE)
		return -1;
	return 0;
}

int WR0_ExecPawn(struct wr0_drv_t* drv, LPCSTR fn,
	const ULONG64* in, SIZE_T in_size,
	PULONG64 out, SIZE_T out_size,
	PSIZE_T return_size)
{
	PIO_EXEC_INPUT* inBuf;
	DWORD inBufSize;
	DWORD returnedLength;
	BOOL bRes;

	*return_size = 0;

	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE || drv->driver_type != WR0_DRIVER_PAWNIO)
		return -1;

	if (in_size > 0 && in != NULL)
		inBufSize = (DWORD)(sizeof(PIO_EXEC_INPUT) + in_size * sizeof(ULONG64));
	else
		return 0;

	inBuf = calloc(1, inBufSize);
	if (inBuf == NULL)
		return -1;

	strcpy_s(inBuf->Fn, PIO_FN_NAME_LEN, fn);

	memcpy(inBuf->Params, in, in_size * sizeof(ULONG64));

	bRes = DeviceIoControl(drv->hhDriver,
		IOCTL_PIO_EXECUTE_FN,
		inBuf,
		inBufSize,
		out,
		(DWORD)(out_size * sizeof(*out)),
		&returnedLength,
		NULL);

	free(inBuf);

	if (bRes == FALSE)
		return -1;

	*return_size = returnedLength / sizeof(*out);
	return 0;
}

#endif

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
