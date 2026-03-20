// SPDX-License-Identifier: Unlicense

#include "libcdi.h"
#include <pathcch.h>
#include "libnw.h"

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
	BYTE (WINAPI* cdi_get_current_aam)(CDI_SMART* ptr, INT index);
	BYTE (WINAPI* cdi_get_recommend_aam)(CDI_SMART* ptr, INT index);
	BOOL (WINAPI* cdi_enable_aam)(CDI_SMART* ptr, INT index, BYTE value);
	BOOL (WINAPI* cdi_disable_aam)(CDI_SMART* ptr, INT index);
	BYTE (WINAPI* cdi_get_current_apm)(CDI_SMART* ptr, INT index);
	BYTE (WINAPI* cdi_get_recommend_apm)(CDI_SMART* ptr, INT index);
	BOOL (WINAPI* cdi_enable_apm)(CDI_SMART* ptr, INT index, BYTE value);
	BOOL (WINAPI* cdi_disable_apm)(CDI_SMART* ptr, INT index);
} m_cdi;

#ifdef _WIN64
#define LIBCDI_DLL L"libcdi.dll"
#else
#define LIBCDI_DLL L"libcdix86.dll"
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
	*(FARPROC*)&m_cdi.cdi_get_current_aam = GetProcAddress(m_cdi.dll, "cdi_get_current_aam");
	*(FARPROC*)&m_cdi.cdi_get_recommend_aam = GetProcAddress(m_cdi.dll, "cdi_get_recommend_aam");
	*(FARPROC*)&m_cdi.cdi_enable_aam = GetProcAddress(m_cdi.dll, "cdi_enable_aam");
	*(FARPROC*)&m_cdi.cdi_disable_aam = GetProcAddress(m_cdi.dll, "cdi_disable_aam");
	*(FARPROC*)&m_cdi.cdi_get_current_apm = GetProcAddress(m_cdi.dll, "cdi_get_current_apm");
	*(FARPROC*)&m_cdi.cdi_get_recommend_apm = GetProcAddress(m_cdi.dll, "cdi_get_recommend_apm");
	*(FARPROC*)&m_cdi.cdi_enable_apm = GetProcAddress(m_cdi.dll, "cdi_enable_apm");
	*(FARPROC*)&m_cdi.cdi_disable_apm = GetProcAddress(m_cdi.dll, "cdi_disable_apm");
	if (m_cdi.cdi_create_smart == NULL)
		goto fail;
	return m_cdi.cdi_create_smart();
fail:
	ZeroMemory(&m_cdi, sizeof(m_cdi));
	return NULL;
}

CONST CHAR* WINAPI cdi_get_version(VOID)
{
	if (m_cdi.cdi_get_version == NULL)
		return "";
	return m_cdi.cdi_get_version();
}

VOID WINAPI cdi_destroy_smart(CDI_SMART* ptr)
{
	if (m_cdi.cdi_destroy_smart == NULL)
		return;
	m_cdi.cdi_destroy_smart(ptr);
	FreeLibrary(m_cdi.dll);
	ZeroMemory(&m_cdi, sizeof(m_cdi));
}

VOID WINAPI cdi_init_smart(CDI_SMART* ptr, UINT64 flags)
{
	if (m_cdi.cdi_init_smart == NULL)
		return;
	NWL_Debug("SMART", "[%llu] Init %llX", GetTickCount64(), NWLC->NwSmartFlags);
	m_cdi.cdi_init_smart(ptr, flags);
	NWL_Debug("SMART", "[%llu] Init OK", GetTickCount64());
}

DWORD WINAPI cdi_update_smart(CDI_SMART* ptr, INT index)
{
	if (m_cdi.cdi_update_smart == NULL)
		return 0;
	NWL_Debug("SMART", "[%llu] Update %d", GetTickCount64(), index);
	return m_cdi.cdi_update_smart(ptr, index);
}

INT WINAPI cdi_get_disk_count(CDI_SMART* ptr)
{
	if (m_cdi.cdi_get_disk_count == NULL)
		return 0;
	return m_cdi.cdi_get_disk_count(ptr);
}

BOOL WINAPI cdi_get_bool(CDI_SMART* ptr, INT index, enum CDI_ATA_BOOL attr)
{
	if (m_cdi.cdi_get_bool == NULL)
		return FALSE;
	return m_cdi.cdi_get_bool(ptr, index, attr);
}

INT WINAPI cdi_get_int(CDI_SMART* ptr, INT index, enum CDI_ATA_INT attr)
{
	if (m_cdi.cdi_get_int == NULL)
		return 0;
	return m_cdi.cdi_get_int(ptr, index, attr);
}

DWORD WINAPI cdi_get_dword(CDI_SMART* ptr, INT index, enum CDI_ATA_DWORD attr)
{
	if (m_cdi.cdi_get_dword == NULL)
		return 0;
	return m_cdi.cdi_get_dword(ptr, index, attr);
}

WCHAR* WINAPI cdi_get_string(CDI_SMART* ptr, INT index, enum CDI_ATA_STRING attr)
{
	if (m_cdi.cdi_get_string == NULL)
		return NULL;
	return m_cdi.cdi_get_string(ptr, index, attr);
}

VOID WINAPI cdi_free_string(WCHAR* ptr)
{
	if (m_cdi.cdi_free_string == NULL)
		return;
	m_cdi.cdi_free_string(ptr);
}

WCHAR* WINAPI cdi_get_smart_format(CDI_SMART* ptr, INT index)
{
	if (m_cdi.cdi_get_smart_format == NULL)
		return NULL;
	return m_cdi.cdi_get_smart_format(ptr, index);
}

BYTE WINAPI cdi_get_smart_id(CDI_SMART* ptr, INT index, INT attr)
{
	if (m_cdi.cdi_get_smart_id == NULL)
		return 0;
	return m_cdi.cdi_get_smart_id(ptr, index, attr);
}

WCHAR* WINAPI cdi_get_smart_value(CDI_SMART* ptr, INT index, INT attr, BOOL hex)
{
	if (m_cdi.cdi_get_smart_value == NULL)
		return NULL;
	return m_cdi.cdi_get_smart_value(ptr, index, attr, hex);
}

INT WINAPI cdi_get_smart_status(CDI_SMART* ptr, INT index, INT attr)
{
	if (m_cdi.cdi_get_smart_status == NULL)
		return 0;
	return m_cdi.cdi_get_smart_status(ptr, index, attr);
}

WCHAR* WINAPI cdi_get_smart_name(CDI_SMART* ptr, INT index, BYTE id)
{
	if (m_cdi.cdi_get_smart_name == NULL)
		return NULL;
	return m_cdi.cdi_get_smart_name(ptr, index, id);
}

BYTE WINAPI cdi_get_current_aam(CDI_SMART* ptr, INT index)
{
	if (m_cdi.cdi_get_current_aam == NULL)
		return 0;
	return m_cdi.cdi_get_current_aam(ptr, index);
}

BYTE WINAPI cdi_get_recommend_aam(CDI_SMART* ptr, INT index)
{
	if (m_cdi.cdi_get_recommend_aam == NULL)
		return 0;
	return m_cdi.cdi_get_recommend_aam(ptr, index);
}

BOOL WINAPI cdi_enable_aam(CDI_SMART* ptr, INT index, BYTE value)
{
	if (m_cdi.cdi_enable_aam == NULL)
		return FALSE;
	return m_cdi.cdi_enable_aam(ptr, index, value);
}

BOOL WINAPI cdi_disable_aam(CDI_SMART* ptr, INT index)
{
	if (m_cdi.cdi_disable_aam == NULL)
		return FALSE;
	return m_cdi.cdi_disable_aam(ptr, index);
}

BYTE WINAPI cdi_get_current_apm(CDI_SMART* ptr, INT index)
{
	if (m_cdi.cdi_get_current_apm == NULL)
		return 0;
	return m_cdi.cdi_get_current_apm(ptr, index);
}

BYTE WINAPI cdi_get_recommend_apm(CDI_SMART* ptr, INT index)
{
	if (m_cdi.cdi_get_recommend_apm == NULL)
		return 0;
	return m_cdi.cdi_get_recommend_apm(ptr, index);
}

BOOL WINAPI cdi_enable_apm(CDI_SMART* ptr, INT index, BYTE value)
{
	if (m_cdi.cdi_enable_apm == NULL)
		return FALSE;
	return m_cdi.cdi_enable_apm(ptr, index, value);
}

BOOL WINAPI cdi_disable_apm(CDI_SMART* ptr, INT index)
{
	if (m_cdi.cdi_disable_apm == NULL)
		return FALSE;
	return m_cdi.cdi_disable_apm(ptr, index);
}
