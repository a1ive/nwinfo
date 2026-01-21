// SPDX-License-Identifier: Unlicense
#include "ioctl.h"
#include "ioctl_priv.h"
#include "libnw.h"
#include "utils.h"
#include <pathcch.h>
#include <shlobj.h>

#define WR0_ENABLE_CPUZ_161     0
#define WR0_ENABLE_CPUZ_162     1
#define WR0_ENABLE_NWHWIO       1
#define WR0_ENABLE_HWRWDRV      0
#define WR0_ENABLE_WINRING0_120 0

static BOOL load_wr0(struct wr0_drv_t* drv)
{
	drv->handle = CreateFileW(drv->obj, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (drv->handle == INVALID_HANDLE_VALUE)
		return FALSE;
	return TRUE;
}

static BOOL install_wr0(struct wr0_drv_t* drv)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	GetModuleFileNameW(NULL, drv->path, MAX_PATH);
	PathCchRemoveFileSpec(drv->path, MAX_PATH);
	PathCchAppend(drv->path, MAX_PATH, drv->name);
	hFile = CreateFileW(drv->path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	else
		CloseHandle(hFile);

	drv->scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (drv->scm == NULL)
		return FALSE;

	drv->sch = CreateServiceW(drv->scm, drv->id, drv->id,
		SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE,
		drv->path, NULL, NULL, NULL, NULL, NULL);
	if (drv->sch == NULL)
	{
		if (GetLastError() == ERROR_SERVICE_EXISTS)
		{
			drv->sch = OpenServiceW(drv->scm, drv->id, SERVICE_ALL_ACCESS);
			if (drv->sch == NULL)
			{
				CloseServiceHandle(drv->scm);
				drv->scm = NULL;
				return FALSE;
			}
		}
		else
		{
			CloseServiceHandle(drv->scm);
			drv->scm = NULL;
			return FALSE;
		}
	}
	else
		drv->installed = TRUE;

	if (!StartServiceW(drv->sch, 0, NULL))
	{
		if (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
		{
			if (drv->installed)
				DeleteService(drv->sch);
			drv->installed = FALSE;
			CloseServiceHandle(drv->sch);
			drv->sch = NULL;
			CloseServiceHandle(drv->scm);
			drv->scm = NULL;
			return FALSE;
		}
	}

	return TRUE;
}

static void uninstall_wr0(struct wr0_drv_t* drv)
{
	SERVICE_STATUS srvStatus = { 0 };

	if (drv->handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(drv->handle);
		drv->handle = INVALID_HANDLE_VALUE;
	}

	if (drv->sch)
	{
		if (drv->installed)
		{
			ControlService(drv->sch, SERVICE_CONTROL_STOP, &srvStatus);
			DeleteService(drv->sch);
			drv->installed = FALSE;
		}
		CloseServiceHandle(drv->sch);
		drv->sch = NULL;
	}
	if (drv->scm)
	{
		CloseServiceHandle(drv->scm);
		drv->scm = NULL;
	}
}

#if WR0_ENABLE_CPUZ_161
static struct wr0_drv_t drv_cpuz161 =
{
	.name = CPUZ161DRV_NAME,
	.type = WR0_DRIVER_CPUZ161,
	.id = CPUZ161DRV_ID,
	.obj = CPUZ161DRV_OBJ,
	.handle = INVALID_HANDLE_VALUE,

	.load = load_wr0,
	.install = install_wr0,
	.uninstall = uninstall_wr0,
};
#endif

#if WR0_ENABLE_CPUZ_162
static struct wr0_drv_t drv_cpuz162 =
{
	.name = CPUZ162DRV_NAME,
	.type = WR0_DRIVER_CPUZ162,
	.id = CPUZ162DRV_ID,
	.obj = CPUZ162DRV_OBJ,
	.handle = INVALID_HANDLE_VALUE,

	.load = load_wr0,
	.install = install_wr0,
	.uninstall = uninstall_wr0,
};
#endif

#if WR0_ENABLE_NWHWIO
static struct wr0_drv_t drv_nwhwio =
{
	.name = HWIODRV_NAME,
	.type = WR0_DRIVER_HWIO,
	.id = HWIODRV_ID,
	.obj = HWIODRV_OBJ,
	.handle = INVALID_HANDLE_VALUE,

	.load = load_wr0,
	.install = install_wr0,
	.uninstall = uninstall_wr0,
};
#endif

#if WR0_ENABLE_HWRWDRV
static struct wr0_drv_t drv_hwrwdrv =
{
	.name = HWRWDRV_NAME,
	.type = WR0_DRIVER_WINRING0,
	.id = HWRWDRV_ID,
	.obj = HWRWDRV_OBJ,
	.handle = INVALID_HANDLE_VALUE,

	.load = load_wr0,
	.install = install_wr0,
	.uninstall = uninstall_wr0,
};
#endif

#if WR0_ENABLE_WINRING0_120
static struct wr0_drv_t drv_winring0 =
{
	.name = WINRING0_NAME,
	.type = WR0_DRIVER_WINRING0,
	.id = WINRING0_ID,
	.obj = WINRING0_OBJ,
	.handle = INVALID_HANDLE_VALUE,

	.load = load_wr0,
	.install = install_wr0,
	.uninstall = uninstall_wr0,
};
#endif

static int load_pio_mod(struct pio_mod_t* mod, LPCWSTR name)
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

	NWL_Debug("PIO", "Load file OK %s", NWL_Ucs2ToUtf8(name));

	mod->hd = CreateFileW(PAWNIO_OBJ,
		GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, NULL);
	if (mod->hd == INVALID_HANDLE_VALUE)
		goto fail;
	if (!DeviceIoControl(mod->hd, IOCTL_PIO_LOAD_BINARY, mod->blob, mod->size, NULL, 0, NULL, NULL))
		goto fail;

	CloseHandle(hFile);

	NWL_Debug("PIO", "Load blob OK %s size=%lu", NWL_Ucs2ToUtf8(name), mod->size);
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
	NWL_Debug("PIO", "Load blob FAIL %s", NWL_Ucs2ToUtf8(name));
	return -1;
}

static void unload_pio_mod(struct pio_mod_t* mod)
{
	if (mod->blob)
		free(mod->blob);
	if (mod->hd && mod->hd != INVALID_HANDLE_VALUE)
		CloseHandle(mod->hd);
	mod->blob = NULL;
	mod->size = 0;
	mod->hd = INVALID_HANDLE_VALUE;
}

static BOOL load_pio(struct wr0_drv_t* drv)
{
#ifdef _WIN64
	drv->handle = CreateFileW(drv->obj, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (drv->handle == INVALID_HANDLE_VALUE)
		return FALSE;
	load_pio_mod(&drv->pio_amd0f, L"AMDFamily0F.bin");
	load_pio_mod(&drv->pio_amd10, L"AMDFamily10.bin");
	load_pio_mod(&drv->pio_amd17, L"AMDFamily17.bin");
	load_pio_mod(&drv->pio_intel, L"IntelMSR.bin");
	load_pio_mod(&drv->pio_rysmu, L"RyzenSMU.bin");
	load_pio_mod(&drv->pio_smi801, L"SmbusI801.bin");
	load_pio_mod(&drv->pio_smpiix4, L"SmbusPIIX4.bin");
	return TRUE;
#else
	return FALSE;
#endif
}

static BOOL install_pio(struct wr0_drv_t* drv)
{
#ifdef _WIN64
	if (!WR0_CheckPawnIO())
	{
		if (!WR0_InstallPawnIO())
			return FALSE;
		drv->installed = TRUE;
	}

	SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, 0, drv->path);
	PathCchAppend(drv->path, MAX_PATH, PAWNIO_ID);
	PathCchAppend(drv->path, MAX_PATH, drv->name);
	return TRUE;
#else
	return FALSE;
#endif
}

static void uninstall_pio(struct wr0_drv_t* drv)
{
#ifdef _WIN64
	unload_pio_mod(&drv->pio_amd0f);
	unload_pio_mod(&drv->pio_amd10);
	unload_pio_mod(&drv->pio_amd17);
	unload_pio_mod(&drv->pio_intel);
	unload_pio_mod(&drv->pio_rysmu);
	unload_pio_mod(&drv->pio_smi801);
	unload_pio_mod(&drv->pio_smpiix4);
	if (drv->handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(drv->handle);
		drv->handle = INVALID_HANDLE_VALUE;
	}
	if (!drv->installed)
		return;
	drv->installed = FALSE;

	WCHAR path[MAX_PATH];
	WCHAR cmdline[] = PAWNIO_DEL_CMD;
	STARTUPINFOW si = { .cb = sizeof(STARTUPINFOW) };
	PROCESS_INFORMATION pi = { 0 };
	GetModuleFileNameW(NULL, path, MAX_PATH);
	PathCchRemoveFileSpec(path, MAX_PATH);
	PathCchAppend(path, MAX_PATH, PAWNIO_SETUP_EXE);
	if (!CreateProcessW(path, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		return;
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
#endif
}

static struct wr0_drv_t drv_pawnio =
{
	.name = PAWNIO_NAME,
	.type = WR0_DRIVER_PAWNIO,
	.id = PAWNIO_ID,
	.obj = PAWNIO_OBJ,
	.handle = INVALID_HANDLE_VALUE,

	.load = load_pio,
	.install = install_pio,
	.uninstall = uninstall_pio,
};

static struct wr0_drv_t* drv_list[] =
{
#if WR0_ENABLE_CPUZ_161
	&drv_cpuz161,
#endif
#if WR0_ENABLE_CPUZ_162
	&drv_cpuz162,
#endif
#if WR0_ENABLE_NWHWIO
	&drv_nwhwio,
#endif
#if WR0_ENABLE_HWRWDRV
	&drv_hwrwdrv,
#endif
#if WR0_ENABLE_WINRING0_120
	&drv_winring0,
#endif
	&drv_pawnio,
};

struct wr0_drv_t* WR0_OpenDriver(void)
{
	if (WR0_GetWineVersion())
		return NULL;
	if (WR0_IsWoW64())
		return NULL;
	for (size_t i = 0; i < ARRAYSIZE(drv_list); i++)
	{
		struct wr0_drv_t* drv = drv_list[i];
		ZeroMemory(drv->path, MAX_PATH);
		if (drv->load(drv))
		{
			NWL_Debug("DRV", "%s loaded.", NWL_Ucs2ToUtf8(drv->name));
			return drv;
		}
		if (!drv->install(drv))
		{
			NWL_Debug("DRV", "cannot install %s.", NWL_Ucs2ToUtf8(drv->name));
			ZeroMemory(drv->path, MAX_PATH);
			continue;
		}
		NWL_Debug("DRV", "%s installed.", NWL_Ucs2ToUtf8(drv->name));
		if (drv->load(drv))
		{
			NWL_Debug("DRV", "%s loaded.", NWL_Ucs2ToUtf8(drv->name));
			return drv;
		}
		NWL_Debug("DRV", "cannot load %s.", NWL_Ucs2ToUtf8(drv->name));
		ZeroMemory(drv->path, MAX_PATH);
	}
	return NULL;
}

void WR0_CloseDriver(struct wr0_drv_t* drv)
{
	if (drv)
		drv->uninstall(drv);
}

int WR0_RdMsr(struct wr0_drv_t* drv, uint32_t msr_index, uint64_t* result)
{
	DWORD dwBytesReturned;
	UINT64 msrData = 0;
	BOOL bRes = FALSE;
	DWORD ctlCode;

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE)
		return -1;

	switch (drv->type)
	{
	case WR0_DRIVER_WINRING0:
		ctlCode = IOCTL_OLS_READ_MSR;
		break;
	case WR0_DRIVER_CPUZ161:
	case WR0_DRIVER_CPUZ162:
		ctlCode = IOCTL_CPUZ_READ_MSR;
		break;
	case WR0_DRIVER_HWIO:
		ctlCode = IOCTL_HIO_READ_MSR;
		break;
	default:
		return -1;
	}

	bRes = DeviceIoControl(drv->handle, ctlCode,
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

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE)
		return -1;

	switch (drv->type)
	{
	case WR0_DRIVER_WINRING0:
		ctlCode = IOCTL_OLS_WRITE_MSR;
		break;
	case WR0_DRIVER_CPUZ161:
	case WR0_DRIVER_CPUZ162:
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

	bRes = DeviceIoControl(drv->handle, ctlCode,
		&inBuf, sizeof(inBuf), &outBuf, sizeof(outBuf), &dwBytesReturned, NULL);
	if (bRes == FALSE)
		return -1;
	return 0;
}

uint8_t WR0_RdIo8(struct wr0_drv_t* drv, uint16_t port)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	uint8_t value = 0;

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE)
		return 0;

	switch (drv->type)
	{
	case WR0_DRIVER_WINRING0:
	{
		WORD outBuf = 0;
		result = DeviceIoControl(drv->handle, IOCTL_OLS_READ_IO_PORT_BYTE,
			&port, sizeof(port), &outBuf, sizeof(outBuf), &returnedLength, NULL);
		value = (uint8_t)outBuf;
	}
		break;
	case WR0_DRIVER_CPUZ161:
	case WR0_DRIVER_CPUZ162:
	{
		UINT64 inBuf = port;
		DWORD outBuf[2] = { 0 };
		result = DeviceIoControl(drv->handle, IOCTL_CPUZ_READ_IO_PORT_BYTE,
			&inBuf, sizeof(inBuf), outBuf, sizeof(outBuf), &returnedLength, NULL);
		value = (uint8_t)outBuf[0];
	}
		break;
	case WR0_DRIVER_HWIO:
	{
		WORD outBuf = 0;
		result = DeviceIoControl(drv->handle, IOCTL_HIO_READ_IO_PORT,
			&port, sizeof(port), &outBuf, sizeof(outBuf), &returnedLength, NULL);
		value = (uint8_t)outBuf;
	}
		break;
	default:
		return 0;
	}
	if (!result)
		NWL_Debug("IO", "Read 8 @%u failed", port);
	return value;
}

uint16_t WR0_RdIo16(struct wr0_drv_t* drv, uint16_t port)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	uint16_t value = 0;

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE)
		return 0;

	switch (drv->type)
	{
	case WR0_DRIVER_WINRING0:
	{
		result = DeviceIoControl(drv->handle, IOCTL_OLS_READ_IO_PORT_WORD,
			&port, sizeof(port), &value, sizeof(value), &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_CPUZ161:
	case WR0_DRIVER_CPUZ162:
	{
		UINT64 inBuf = port;
		DWORD outBuf[2] = { 0 };
		result = DeviceIoControl(drv->handle, IOCTL_CPUZ_READ_IO_PORT_WORD,
			&inBuf, sizeof(inBuf), outBuf, sizeof(outBuf), &returnedLength, NULL);
		value = (uint16_t)outBuf[0];
	}
		break;
	case WR0_DRIVER_HWIO:
	{
		result = DeviceIoControl(drv->handle, IOCTL_HIO_READ_IO_PORT,
			&port, sizeof(port), &value, sizeof(value), &returnedLength, NULL);
	}
		break;
	default:
		return 0;
	}
	if (!result)
		NWL_Debug("IO", "Read 16 @%u failed", port);
	return value;
}

uint32_t WR0_RdIo32(struct wr0_drv_t* drv, uint16_t port)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	uint32_t value = 0;

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE)
		return 0;

	switch (drv->type)
	{
	case WR0_DRIVER_WINRING0:
	{
		DWORD inBuf = port;
		result = DeviceIoControl(drv->handle, IOCTL_OLS_READ_IO_PORT_DWORD,
			&inBuf, sizeof(inBuf), &value, sizeof(value), &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_CPUZ161:
	case WR0_DRIVER_CPUZ162:
	{
		UINT64 inBuf = port;
		DWORD outBuf[2] = { 0 };
		result = DeviceIoControl(drv->handle, IOCTL_CPUZ_READ_IO_PORT_DWORD,
			&inBuf, sizeof(inBuf), outBuf, sizeof(outBuf), &returnedLength, NULL);
		value = (uint32_t)outBuf[0];
	}
		break;
	case WR0_DRIVER_HWIO:
	{
		DWORD inBuf = port;
		result = DeviceIoControl(drv->handle, IOCTL_HIO_READ_IO_PORT,
			&inBuf, sizeof(inBuf), &value, sizeof(value), &returnedLength, NULL);
	}
		break;
	default:
		return 0;
	}
	if (!result)
		NWL_Debug("IO", "Read 32 @%u failed", port);
	return value;
}

void WR0_WrIo8(struct wr0_drv_t* drv, uint16_t port, uint8_t value)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	DWORD length = 0;

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE)
		return;

	switch (drv->type)
	{
	case WR0_DRIVER_WINRING0:
	{
		OLS_WRITE_IO_PORT_INPUT inBuf = { 0 };
		inBuf.CharData = value;
		inBuf.PortNumber = port;
		length = offsetof(OLS_WRITE_IO_PORT_INPUT, CharData) + sizeof(inBuf.CharData);
		result = DeviceIoControl(drv->handle, IOCTL_OLS_WRITE_IO_PORT_BYTE,
			&inBuf, length, NULL, 0, &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_CPUZ161:
	case WR0_DRIVER_CPUZ162:
	{
		OLS_WRITE_IO_PORT_INPUT inBuf = { 0 };
		UINT64 outBuf;
		inBuf.CharData = value;
		inBuf.PortNumber = port;
		length = sizeof(inBuf);
		result = DeviceIoControl(drv->handle, IOCTL_CPUZ_WRITE_IO_PORT_BYTE,
			&inBuf, length, &outBuf, sizeof(outBuf), &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_HWIO:
	{
		OLS_WRITE_IO_PORT_INPUT inBuf = { 0 };
		inBuf.CharData = value;
		inBuf.PortNumber = port;
		length = offsetof(OLS_WRITE_IO_PORT_INPUT, CharData) + sizeof(inBuf.CharData);
		result = DeviceIoControl(drv->handle, IOCTL_HIO_WRITE_IO_PORT,
			&inBuf, length, NULL, 0, &returnedLength, NULL);
	}
		break;
	default:
		return;
	}
	if (!result)
		NWL_Debug("IO", "Write 8 @%u->%02X failed", port, value);
}

void WR0_WrIo16(struct wr0_drv_t* drv, uint16_t port, uint16_t value)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	DWORD length = 0;

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE)
		return;

	switch (drv->type)
	{
	case WR0_DRIVER_WINRING0:
	{
		OLS_WRITE_IO_PORT_INPUT inBuf = { 0 };
		inBuf.ShortData = value;
		inBuf.PortNumber = port;
		length = offsetof(OLS_WRITE_IO_PORT_INPUT, ShortData) + sizeof(inBuf.ShortData);
		result = DeviceIoControl(drv->handle, IOCTL_OLS_WRITE_IO_PORT_WORD,
			&inBuf, length, NULL, 0, &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_CPUZ161:
	case WR0_DRIVER_CPUZ162:
	{
		OLS_WRITE_IO_PORT_INPUT inBuf = { 0 };
		UINT64 outBuf;
		inBuf.ShortData = value;
		inBuf.PortNumber = port;
		length = sizeof(inBuf);
		result = DeviceIoControl(drv->handle, IOCTL_CPUZ_WRITE_IO_PORT_WORD,
			&inBuf, length, &outBuf, sizeof(outBuf), &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_HWIO:
	{
		OLS_WRITE_IO_PORT_INPUT inBuf = { 0 };
		inBuf.ShortData = value;
		inBuf.PortNumber = port;
		length = offsetof(OLS_WRITE_IO_PORT_INPUT, ShortData) + sizeof(inBuf.ShortData);
		result = DeviceIoControl(drv->handle, IOCTL_HIO_WRITE_IO_PORT,
			&inBuf, length, NULL, 0, &returnedLength, NULL);
	}
		break;
	default:
		return;
	}
	if (!result)
		NWL_Debug("IO", "Write 16 @%u->%04X failed", port, value);
}

void WR0_WrIo32(struct wr0_drv_t* drv, uint16_t port, uint32_t value)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	DWORD length = 0;

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE)
		return;

	switch (drv->type)
	{
	case WR0_DRIVER_WINRING0:
	{
		OLS_WRITE_IO_PORT_INPUT inBuf = { 0 };
		inBuf.LongData = value;
		inBuf.PortNumber = port;
		length = offsetof(OLS_WRITE_IO_PORT_INPUT, LongData) + sizeof(inBuf.LongData);
		result = DeviceIoControl(drv->handle, IOCTL_OLS_WRITE_IO_PORT_DWORD,
			&inBuf, length, NULL, 0, &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_CPUZ161:
	case WR0_DRIVER_CPUZ162:
	{
		OLS_WRITE_IO_PORT_INPUT inBuf = { 0 };
		UINT64 outBuf;
		inBuf.LongData = value;
		inBuf.PortNumber = port;
		length = sizeof(inBuf);
		result = DeviceIoControl(drv->handle, IOCTL_CPUZ_WRITE_IO_PORT_DWORD,
			&inBuf, length, &outBuf, sizeof(outBuf), &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_HWIO:
	{
		OLS_WRITE_IO_PORT_INPUT inBuf = { 0 };
		inBuf.LongData = value;
		inBuf.PortNumber = port;
		length = offsetof(OLS_WRITE_IO_PORT_INPUT, LongData) + sizeof(inBuf.LongData);
		result = DeviceIoControl(drv->handle, IOCTL_HIO_WRITE_IO_PORT,
			&inBuf, length, NULL, 0, &returnedLength, NULL);
	}
		break;
	default:
		return;
	}
	if (!result)
		NWL_Debug("IO", "Write 32 @%u->%08X failed", port, value);
}

int WR0_RdPciConf(struct wr0_drv_t* drv, uint32_t addr, uint32_t reg, void* value, uint32_t size)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE || addr == 0xFFFFFFFF
		|| !value || (size == 2 && (reg & 1) != 0) || (size == 4 && (reg & 3) != 0))
		return -1;

	switch (drv->type)
	{
	case WR0_DRIVER_HWIO:
	{
		OLS_READ_PCI_CONFIG_INPUT inBuf = { 0 };
		inBuf.PciAddress = (PciGetBus(addr)) | (PciGetDev(addr) << 8) | (PciGetFunc(addr) << 16);
		inBuf.PciOffset = reg;
		result = DeviceIoControl(drv->handle, IOCTL_HIO_READ_PCI_CONFIG,
			&inBuf, sizeof(inBuf), value, size, &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_WINRING0:
	{
		OLS_READ_PCI_CONFIG_INPUT inBuf = { 0 };
		inBuf.PciAddress = addr;
		inBuf.PciOffset = reg;
		result = DeviceIoControl(drv->handle, IOCTL_OLS_READ_PCI_CONFIG,
			&inBuf, sizeof(inBuf), value, size, &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_CPUZ161:
	case WR0_DRIVER_CPUZ162:
	{
		CPUZ_READ_PCI_CONFIG_INPUT inBuf = { 0 };
		inBuf.Bus = PciGetBus(addr);
		inBuf.Device = PciGetDev(addr);
		inBuf.Function = PciGetFunc(addr);
		inBuf.Offset = reg;
		inBuf.Length = size;
		result = DeviceIoControl(drv->handle, IOCTL_CPUZ_READ_PCI_CONFIG,
			&inBuf, sizeof(inBuf), &inBuf, sizeof(DWORD) + size, &returnedLength, NULL);
		memcpy(value, inBuf.RetData, size);
	}
		break;
	default:
		return -1;
	}
	if (!result)
	{
		//NWL_Debug("IO", "Read PCI @%X(%X+%u) failed", addr, reg, size);
		return -2;
	}
	return 0;
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

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE || addr == 0xFFFFFFFF
		|| !value || (size == 2 && (reg & 1) != 0) || (size == 4 && (reg & 3) != 0))
		return -1;

	switch (drv->type)
	{
	case WR0_DRIVER_WINRING0:
	{
		DWORD inSize = offsetof(OLS_WRITE_PCI_CONFIG_INPUT, Data) + size;
		OLS_WRITE_PCI_CONFIG_INPUT inBuf = { 0 };
		inBuf.PciAddress = addr;
		inBuf.PciOffset = reg;
		memcpy(inBuf.Data, value, size);
		result = DeviceIoControl(drv->handle, IOCTL_OLS_WRITE_PCI_CONFIG,
			&inBuf, inSize, NULL, 0, &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_HWIO:
	{
		DWORD inSize = offsetof(OLS_WRITE_PCI_CONFIG_INPUT, Data) + size;
		OLS_WRITE_PCI_CONFIG_INPUT inBuf = { 0 };
		inBuf.PciAddress = (PciGetBus(addr)) | (PciGetDev(addr) << 8) | (PciGetFunc(addr) << 16);
		inBuf.PciOffset = reg;
		memcpy(inBuf.Data, value, size);
		result = DeviceIoControl(drv->handle, IOCTL_HIO_WRITE_PCI_CONFIG,
			&inBuf, inSize, NULL, 0, &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_CPUZ161:
	case WR0_DRIVER_CPUZ162:
	{
		CPUZ_WRITE_PCI_CONFIG_INPUT inBuf = { 0 };
		inBuf.Bus = PciGetBus(addr);
		inBuf.Device = PciGetDev(addr);
		inBuf.Function = PciGetFunc(addr);
		inBuf.Offset = reg;
		uint32_t ioAddr = 0x80000000U | (inBuf.Bus << 16) | (inBuf.Device << 11) | (inBuf.Function << 8) | (reg & 0xFC);
		if (size == sizeof(DWORD))
		{
#if 0
			// Use port IO
			WR0_WrIo32(drv, 0xCF8, ioAddr);
			WR0_WrIo32(drv, 0xCFC + (reg & 3), *(uint32_t*)value);
#else
			memcpy(&inBuf.Value, value, sizeof(DWORD));
			result = DeviceIoControl(drv->handle, IOCTL_CPUZ_WRITE_PCI_CONFIG,
				&inBuf, sizeof(inBuf), &inBuf, sizeof(inBuf), &returnedLength, NULL);
#endif
		}
		else if (size == sizeof(uint16_t))
		{
			// Use port IO
			WR0_WrIo32(drv, 0xCF8, ioAddr);
			WR0_WrIo16(drv, 0xCFC + (reg & 2), *(uint16_t*)value);
			result = TRUE;
		}
		else if (size == sizeof(uint8_t))
		{
			// Use port IO
			WR0_WrIo32(drv, 0xCF8, ioAddr);
			WR0_WrIo8(drv, 0xCFC + (reg & 3), *(uint8_t*)value);
			result = TRUE;
		}
	}
	default:
		return -1;
	}
	if (!result)
	{
		NWL_Debug("IO", "Write PCI @%X(%X+%u) failed", addr, reg, size);
		return -2;
	}
	return 0;
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

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE || drv->type == WR0_DRIVER_PAWNIO
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

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE || drv->type == WR0_DRIVER_PAWNIO)
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

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE)
		return -1;

	switch (drv->type)
	{
	case WR0_DRIVER_HWIO:
		result = DeviceIoControl(drv->handle, IOCTL_HIO_READ_MMIO,
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

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE
		|| !buffer || count == 0)
		return 0;

	switch (drv->type)
	{
	case WR0_DRIVER_WINRING0:
	{
		OLS_READ_MEMORY_INPUT inBuf;
		inBuf.Address.QuadPart = address;
		inBuf.UnitSize = unitSize;
		inBuf.Count = count;
		result = DeviceIoControl(drv->handle, IOCTL_OLS_READ_MEMORY,
			&inBuf, sizeof(OLS_READ_MEMORY_INPUT), buffer, size, &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_CPUZ161:
	case WR0_DRIVER_CPUZ162:
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
		result = DeviceIoControl(drv->handle, IOCTL_CPUZ_READ_MEMORY,
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
#define NB_PCI_REG_ADDR_ADDR 0xB8
#define NB_PCI_REG_DATA_ADDR 0xBC
// smn=2 -> 0xC4/0xC8 Zen SMU
#define SMU_PCI_ADDR_REG 0xC4
#define SMU_PCI_DATA_REG 0xC8
// smn=3 -> 0x60/0x64 Zen Temperature
#define FAMILY_17H_PCI_CONTROL_REGISTER     0x60
#define FAMILY_17H_PCI_DATA_REGISTER        0x64

static inline BOOL
read_smn(struct wr0_drv_t* drv, uint32_t port[2], uint32_t address, uint32_t* value)
{
	WR0_WrPciConf32(drv, 0, port[0], address);
	uint32_t ret = WR0_RdPciConf32(drv, 0, port[1]);
	if (ret == 0xFFFFFFFF)
	{
		NWL_Debug("SMN", "Address 0x%X Read ERROR", address);
		return FALSE;
	}
	*value = ret;
	return TRUE;
}

static inline void
write_smn(struct wr0_drv_t* drv, uint32_t port[2], uint32_t address, uint32_t value)
{
	WR0_WrPciConf32(drv, 0, port[0], address);
	WR0_WrPciConf32(drv, 0, port[1], value);
	NWL_Debug("SMN", "Address 0x%X Write: 0x%X", address, value);
}

DWORD WR0_RdAmdSmn(struct wr0_drv_t* drv, enum wr0_smn_type smn, DWORD reg)
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	DWORD value = 0;
	uint32_t addr[2];

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE)
		return 0;
	switch (smn)
	{
	case WR0_SMN_AMD15H:
		addr[0] = NB_PCI_REG_ADDR_ADDR;
		addr[1] = NB_PCI_REG_DATA_ADDR;
		break;
	case WR0_SMN_ZENSMU:
		addr[0] = SMU_PCI_ADDR_REG;
		addr[1] = SMU_PCI_DATA_REG;
		break;
	case WR0_SMN_AMD17H:
		addr[0] = FAMILY_17H_PCI_CONTROL_REGISTER;
		addr[1] = FAMILY_17H_PCI_DATA_REGISTER;
		break;
	default:
		return 0;
	}

	switch (drv->type)
	{
	case WR0_DRIVER_CPUZ161:
	case WR0_DRIVER_CPUZ162:
	{
		DWORD inBuf[3];
		inBuf[0] = 0; // BDF, (bus << 16) | (dev << 11) | (fn << 8)
		inBuf[1] = smn;
		inBuf[2] = reg;
		result = DeviceIoControl(drv->handle, IOCTL_CPUZ_READ_AMD_SMN,
			inBuf, sizeof(inBuf), &value, sizeof(value), &returnedLength, NULL);
	}
		break;
	case WR0_DRIVER_HWIO:
	case WR0_DRIVER_WINRING0:
		result = read_smn(drv, addr, reg, &value);
		break;
	default:
		return 0;
	}

	NWL_Debug("SMN", "Read AMD SMN reg=%08xh (%X,%X) %d value=%08xh", reg, addr[0], addr[1], result, value);

	return value;
}

#define SMU_COMMAND_MAX_RETRIES 8096

int
WR0_SendSmuCmd(struct wr0_drv_t* drv, uint32_t cmd, uint32_t rsp, uint32_t arg, uint32_t fn, uint32_t args[SMU_MAX_ARGS])
{
	DWORD returnedLength = 0;
	BOOL result = FALSE;
	uint32_t inBuf[17] = { 0 };
	uint32_t outBuf[7] = { 0 };

	if (!drv || !drv->handle || drv->handle == INVALID_HANDLE_VALUE)
		return -1;
	switch (drv->type)
	{
	case WR0_DRIVER_CPUZ161:
	case WR0_DRIVER_CPUZ162:
	{
		inBuf[0] = 0; // BDF, (bus << 16) | (dev << 11) | (fn << 8)
		inBuf[1] = WR0_SMN_ZENSMU; // Zen SMU
		inBuf[2] = fn;
		inBuf[3] = cmd;
		inBuf[4] = rsp;
		for (uint32_t i = 0; i < SMU_MAX_ARGS; i++)
		{
			inBuf[5 + i] = arg + i * 4;
			inBuf[11 + i] = args[i];
		}
		result = DeviceIoControl(drv->handle, IOCTL_CPUZ_SEND_SMN_CMD,
			inBuf, sizeof(inBuf), outBuf, sizeof(outBuf), &returnedLength, NULL);
		NWL_Debug("SMU", "Send SMU fn=%08xh %d -> %u", fn, result, outBuf[0]);
		memcpy(args, &outBuf[1], 6 * sizeof(uint32_t));
		if (result && outBuf[0] == 1)
			return 0;
		return -2;
	}
	case WR0_DRIVER_HWIO:
	case WR0_DRIVER_WINRING0:
	{
		uint32_t i;
		uint32_t response;
		uint32_t port[2] = { SMU_PCI_ADDR_REG , SMU_PCI_DATA_REG };

		// Wait until the RSP register is non-zero.
		for (i = 0, response = 0; i < SMU_COMMAND_MAX_RETRIES; i++)
		{
			if (!read_smn(drv, port, rsp, &response))
			{
				NWL_Debug("SMU", "Failed to read RSP register");
				return -2;
			}
			if (response != 0)
				break;
		}

		if (response == 0)
			return -3;

		// Write zero to the RSP register
		write_smn(drv, port, rsp, 0);

		// Write the arguments into the argument registers
		for (i = 0; i < SMU_MAX_ARGS; i++)
		{
			write_smn(drv, port, arg + (i * 4), args[i]);
		}

		// Write the message Id into the Message ID register
		write_smn(drv, port, cmd, fn);

		// Wait until the Response register is non-zero.
		for (i = 0, response = 0; i < SMU_COMMAND_MAX_RETRIES; i++)
		{
			if (!read_smn(drv, port, rsp, &response))
				return -2;
			if (response != 0)
				break;
		}

		if (response == 0)
			return -3;

		// Read the response value
		for (i = 0; i < SMU_MAX_ARGS; i++)
		{
			if (!read_smn(drv, port, arg + (i * 4), &args[i]))
				return -2;
		}

		return 0;
	}
	default:
		return -1;
	}
}
