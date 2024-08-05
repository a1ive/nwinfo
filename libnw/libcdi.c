// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "../libcdi/libcdi.h"
#include <pathcch.h>

static struct
{
	HMODULE dll;
	CONST CHAR* (WINAPI* cdi_get_version)(VOID);
	CDI_SMART* (WINAPI* cdi_create_smart)(VOID);
	VOID (WINAPI* cdi_destroy_smart)(CDI_SMART* ptr);
	VOID (WINAPI* cdi_init_smart)(CDI_SMART* ptr, UINT64 flags);
	DWORD (WINAPI* cdi_update_smart)(CDI_SMART* ptr, INT index);
	INT (WINAPI* cdi_get_disk_count)(CDI_SMART* ptr);
	BOOL (WINAPI* cdi_get_bool)(CDI_SMART* ptr, INT index, enum CDI_ATA_BOOL attr);
	INT (WINAPI* cdi_get_int)(CDI_SMART* ptr, INT index, enum CDI_ATA_INT attr);
	DWORD (WINAPI* cdi_get_dword)(CDI_SMART* ptr, INT index, enum CDI_ATA_DWORD attr);
	WCHAR* (WINAPI* cdi_get_string)(CDI_SMART* ptr, INT index, enum CDI_ATA_STRING attr);
	VOID (WINAPI* cdi_free_string)(WCHAR* ptr);
	WCHAR* (WINAPI* cdi_get_smart_format)(CDI_SMART* ptr, INT index);
	BYTE (WINAPI* cdi_get_smart_id)(CDI_SMART* ptr, INT index, INT attr);
	WCHAR* (WINAPI* cdi_get_smart_value)(CDI_SMART* ptr, INT index, INT attr, BOOL hex);
	INT (WINAPI* cdi_get_smart_status)(CDI_SMART* ptr, INT index, INT attr);
	WCHAR* (WINAPI* cdi_get_smart_name)(CDI_SMART* ptr, INT index, BYTE id);
} m_cdi;

#ifdef _WIN64
#define LIBCDI_DLL L"libcdix64.dll"
#else
#define LIBCDI_DLL L"libcdi.dll"
#endif

CDI_SMART* WINAPI cdi_create_smart(VOID)
{
	WCHAR dll_path[MAX_PATH];
	GetModuleFileNameW(NULL, dll_path, MAX_PATH);
	PathCchRemoveFileSpec(dll_path, MAX_PATH);
	PathCchAppend(dll_path, MAX_PATH, LIBCDI_DLL);

	m_cdi.dll = LoadLibraryW(dll_path);
	if (!m_cdi.dll)
		goto fail;
	*(FARPROC*)&m_cdi.cdi_get_version = GetProcAddress(m_cdi.dll, "cdi_get_version");
	*(FARPROC*)&m_cdi.cdi_create_smart = GetProcAddress(m_cdi.dll, "cdi_create_smart");
	*(FARPROC*)&m_cdi.cdi_destroy_smart = GetProcAddress(m_cdi.dll, "cdi_destroy_smart");
	*(FARPROC*)&m_cdi.cdi_init_smart = GetProcAddress(m_cdi.dll, "cdi_init_smart");
	*(FARPROC*)&m_cdi.cdi_update_smart = GetProcAddress(m_cdi.dll, "cdi_update_smart");
	*(FARPROC*)&m_cdi.cdi_get_disk_count = GetProcAddress(m_cdi.dll, "cdi_get_disk_count");
	*(FARPROC*)&m_cdi.cdi_get_bool = GetProcAddress(m_cdi.dll, "cdi_get_bool");
	*(FARPROC*)&m_cdi.cdi_get_int = GetProcAddress(m_cdi.dll, "cdi_get_int");
	*(FARPROC*)&m_cdi.cdi_get_dword = GetProcAddress(m_cdi.dll, "cdi_get_dword");
	*(FARPROC*)&m_cdi.cdi_get_string = GetProcAddress(m_cdi.dll, "cdi_get_string");
	*(FARPROC*)&m_cdi.cdi_free_string = GetProcAddress(m_cdi.dll, "cdi_free_string");
	*(FARPROC*)&m_cdi.cdi_get_smart_format = GetProcAddress(m_cdi.dll, "cdi_get_smart_format");
	*(FARPROC*)&m_cdi.cdi_get_smart_id = GetProcAddress(m_cdi.dll, "cdi_get_smart_id");
	*(FARPROC*)&m_cdi.cdi_get_smart_value = GetProcAddress(m_cdi.dll, "cdi_get_smart_value");
	*(FARPROC*)&m_cdi.cdi_get_smart_status = GetProcAddress(m_cdi.dll, "cdi_get_smart_status");
	*(FARPROC*)&m_cdi.cdi_get_smart_name = GetProcAddress(m_cdi.dll, "cdi_get_smart_name");
	if (m_cdi.cdi_get_version == NULL ||
		m_cdi.cdi_create_smart == NULL ||
		m_cdi.cdi_destroy_smart == NULL ||
		m_cdi.cdi_init_smart == NULL ||
		m_cdi.cdi_update_smart == NULL ||
		m_cdi.cdi_get_disk_count == NULL ||
		m_cdi.cdi_get_bool == NULL ||
		m_cdi.cdi_get_int == NULL ||
		m_cdi.cdi_get_dword == NULL ||
		m_cdi.cdi_get_string == NULL ||
		m_cdi.cdi_free_string == NULL ||
		m_cdi.cdi_get_smart_format == NULL ||
		m_cdi.cdi_get_smart_id == NULL ||
		m_cdi.cdi_get_smart_value == NULL ||
		m_cdi.cdi_get_smart_status == NULL ||
		m_cdi.cdi_get_smart_name == NULL)
	{
		goto fail;
	}
	return m_cdi.cdi_create_smart();
fail:
	ZeroMemory(&m_cdi, sizeof(m_cdi));
	return NULL;
}

CONST CHAR* WINAPI cdi_get_version(VOID)
{
	return m_cdi.cdi_get_version();
}

VOID WINAPI cdi_destroy_smart(CDI_SMART* ptr)
{
	m_cdi.cdi_destroy_smart(ptr);
	FreeLibrary(m_cdi.dll);
	ZeroMemory(&m_cdi, sizeof(m_cdi));
}

VOID WINAPI cdi_init_smart(CDI_SMART* ptr, UINT64 flags)
{
	m_cdi.cdi_init_smart(ptr, flags);
}

DWORD WINAPI cdi_update_smart(CDI_SMART* ptr, INT index)
{
	return m_cdi.cdi_update_smart(ptr, index);
}

INT WINAPI cdi_get_disk_count(CDI_SMART* ptr)
{
	return m_cdi.cdi_get_disk_count(ptr);
}

BOOL WINAPI cdi_get_bool(CDI_SMART* ptr, INT index, enum CDI_ATA_BOOL attr)
{
	return m_cdi.cdi_get_bool(ptr, index, attr);
}

INT WINAPI cdi_get_int(CDI_SMART* ptr, INT index, enum CDI_ATA_INT attr)
{
	return m_cdi.cdi_get_int(ptr, index, attr);
}

DWORD WINAPI cdi_get_dword(CDI_SMART* ptr, INT index, enum CDI_ATA_DWORD attr)
{
	return m_cdi.cdi_get_dword(ptr, index, attr);
}

WCHAR* WINAPI cdi_get_string(CDI_SMART* ptr, INT index, enum CDI_ATA_STRING attr)
{
	return m_cdi.cdi_get_string(ptr, index, attr);
}

VOID WINAPI cdi_free_string(WCHAR* ptr)
{
	m_cdi.cdi_free_string(ptr);
}

WCHAR* WINAPI cdi_get_smart_format(CDI_SMART* ptr, INT index)
{
	return m_cdi.cdi_get_smart_format(ptr, index);
}

BYTE WINAPI cdi_get_smart_id(CDI_SMART* ptr, INT index, INT attr)
{
	return m_cdi.cdi_get_smart_id(ptr, index, attr);
}

WCHAR* WINAPI cdi_get_smart_value(CDI_SMART* ptr, INT index, INT attr, BOOL hex)
{
	return m_cdi.cdi_get_smart_value(ptr, index, attr, hex);
}

INT WINAPI cdi_get_smart_status(CDI_SMART* ptr, INT index, INT attr)
{
	return m_cdi.cdi_get_smart_status(ptr, index, attr);
}

WCHAR* WINAPI cdi_get_smart_name(CDI_SMART* ptr, INT index, BYTE id)
{
	return m_cdi.cdi_get_smart_name(ptr, index, id);
}
