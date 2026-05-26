// SPDX-License-Identifier: Unlicense

#include "drvstore.h"
#include "libnw.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

const DEVPROPKEY DEVPKEY_DriverPackage_DriverInfName =
{ { 0x8163eb01, 0x142c, 0x4f7a, { 0x94, 0xe1, 0xa2, 0x74, 0xcc, 0x47, 0xdb, 0xba } }, 2 };
const DEVPROPKEY DEVPKEY_DriverPackage_OriginalInfName =
{ { 0x8163eb01, 0x142c, 0x4f7a, { 0x94, 0xe1, 0xa2, 0x74, 0xcc, 0x47, 0xdb, 0xba } }, 3 };
const DEVPROPKEY DEVPKEY_DriverPackage_SignerName =
{ { 0x8163eb01, 0x142c, 0x4f7a, { 0x94, 0xe1, 0xa2, 0x74, 0xcc, 0x47, 0xdb, 0xba } }, 7 };
const DEVPROPKEY DEVPKEY_DriverPackage_ProviderName =
{ { 0x8163eb01, 0x142c, 0x4f7a, { 0x94, 0xe1, 0xa2, 0x74, 0xcc, 0x47, 0xdb, 0xba } }, 12 };
const DEVPROPKEY DEVPKEY_DriverPackage_ClassGuid =
{ { 0x8163eb01, 0x142c, 0x4f7a, { 0x94, 0xe1, 0xa2, 0x74, 0xcc, 0x47, 0xdb, 0xba } }, 13 };
const DEVPROPKEY DEVPKEY_DriverPackage_DriverDate =
{ { 0x8163eb01, 0x142c, 0x4f7a, { 0x94, 0xe1, 0xa2, 0x74, 0xcc, 0x47, 0xdb, 0xba } }, 14 };
const DEVPROPKEY DEVPKEY_DriverPackage_DriverVersion =
{ { 0x8163eb01, 0x142c, 0x4f7a, { 0x94, 0xe1, 0xa2, 0x74, 0xcc, 0x47, 0xdb, 0xba } }, 15 };
const DEVPROPKEY DEVPKEY_DriverPackage_ProductName =
{ { 0x8163eb01, 0x142c, 0x4f7a, { 0x94, 0xe1, 0xa2, 0x74, 0xcc, 0x47, 0xdb, 0xba } }, 23 };
const DEVPROPKEY DEVPKEY_DriverPackage_ImportDate =
{ { 0x8163eb01, 0x142c, 0x4f7a, { 0x94, 0xe1, 0xa2, 0x74, 0xcc, 0x47, 0xdb, 0xba } }, 26 };
const DEVPROPKEY DEVPKEY_DriverPackage_DriverPackageId =
{ { 0x8163eb01, 0x142c, 0x4f7a, { 0x94, 0xe1, 0xa2, 0x74, 0xcc, 0x47, 0xdb, 0xba } }, 32 };
const DEVPROPKEY DEVPKEY_DriverPackage_CatalogFile =
{ { 0x8163eb01, 0x142c, 0x4f7a, { 0x94, 0xe1, 0xa2, 0x74, 0xcc, 0x47, 0xdb, 0xba } }, 37 };
const DEVPROPKEY DEVPKEY_DriverPackage_Primitive =
{ { 0x8163eb01, 0x142c, 0x4f7a, { 0x94, 0xe1, 0xa2, 0x74, 0xcc, 0x47, 0xdb, 0xba } }, 40 };
#if 0
const DEVPROPKEY DEVPKEY_DeviceClass_ClassName =
{ { 0x259abffc, 0x50a7, 0x47ce, { 0xaf, 0x08, 0x68, 0xc9, 0xa7, 0xd7, 0x33, 0x66 } }, 3 };
#else
extern const DEVPROPKEY DEVPKEY_DeviceClass_ClassName;
#endif

static HMODULE m_drvstore_dll = NULL;

HDRVSTORE
NWL_DriverStoreOpen(LPCSTR Drive, DWORD Flags)
{
	if (!m_drvstore_dll)
		m_drvstore_dll = LoadLibraryW(L"drvstore.dll");
	if (!m_drvstore_dll)
		return NULL;
	HDRVSTORE (WINAPI* pfnDriverStoreOpen)(LPCWSTR, LPCWSTR, DWORD, HANDLE) = NULL;

	*(FARPROC*)&pfnDriverStoreOpen = GetProcAddress(m_drvstore_dll, "DriverStoreOpenW");
	if (!pfnDriverStoreOpen)
		goto fail;

	LPCWSTR targetSystemPath = NULL;
	LPCWSTR targetBootDrive = NULL;
	WCHAR systemPath[MAX_PATH] = L"C:\\Windows";
	WCHAR bootDrive[MAX_PATH] = L"C:\\";

	if (Drive)
	{
		if (isalpha((unsigned char)Drive[0]) &&
			(Drive[1] == '\0' || (Drive[1] == ':' && Drive[2] == '\0')))
		{
			bootDrive[0] = (WCHAR)Drive[0];
			systemPath[0] = (WCHAR)Drive[0];
		}
		else
		{
			size_t len;
			wcsncpy_s(bootDrive, ARRAYSIZE(bootDrive), NWL_Utf8ToUcs2(Drive), _TRUNCATE);
			len = wcslen(bootDrive);
			while (len > 3 && (bootDrive[len - 1] == L'\\' || bootDrive[len - 1] == L'/'))
				bootDrive[--len] = L'\0';
			wcsncpy_s(systemPath, ARRAYSIZE(systemPath), bootDrive, _TRUNCATE);
			len = wcslen(systemPath);
			if (len && systemPath[len - 1] != L'\\' && systemPath[len - 1] != L'/')
				wcscat_s(systemPath, ARRAYSIZE(systemPath), L"\\");
			wcscat_s(systemPath, ARRAYSIZE(systemPath), L"Windows");
		}
		targetBootDrive = bootDrive;
		targetSystemPath = systemPath;
	}

	HDRVSTORE ret = pfnDriverStoreOpen(targetSystemPath, targetBootDrive, Flags, NULL);
	if (ret == NULL || ret == INVALID_HANDLE_VALUE)
		goto fail;
	return ret;
fail:
	FreeLibrary(m_drvstore_dll);
	m_drvstore_dll = NULL;
	return NULL;
}

BOOL
NWL_DriverStoreClose(HDRVSTORE DriverStoreHandle)
{
	BOOL(WINAPI * pfnDriverStoreClose)(HDRVSTORE) = NULL;

	if (DriverStoreHandle == NULL)
		return FALSE;
	*(FARPROC*)&pfnDriverStoreClose = GetProcAddress(m_drvstore_dll, "DriverStoreClose");
	if (!pfnDriverStoreClose)
		return FALSE;
	BOOL result = pfnDriverStoreClose(DriverStoreHandle);
	FreeLibrary(m_drvstore_dll);
	m_drvstore_dll = NULL;
	return result;
}

BOOL
NWL_DriverStoreEnum(HDRVSTORE DriverStoreHandle, DWORD Flags, PDRIVERSTORE_ENUM_PACKAGE_CALLBACK CallbackRoutine, LPARAM Context)
{
	BOOL(WINAPI * pfnDriverStoreEnum)(HDRVSTORE, DWORD, PDRIVERSTORE_ENUM_PACKAGE_CALLBACK, LPARAM) = NULL;

	if (DriverStoreHandle == NULL || DriverStoreHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	*(FARPROC*)&pfnDriverStoreEnum = GetProcAddress(m_drvstore_dll, "DriverStoreEnumW");
	if (!pfnDriverStoreEnum)
		return FALSE;
	return pfnDriverStoreEnum(DriverStoreHandle, Flags, CallbackRoutine, Context);
}

DWORD
NWL_DriverStoreImport(HDRVSTORE DriverStoreHandle, LPCWSTR DriverPackageFileName,
	USHORT ProcessorArchitecture, LPCWSTR LocaleName, DWORD Flags, LPWSTR DriverStoreFileName, DWORD DriverStoreFileNameSize)
{
	DWORD(WINAPI * pfnDriverStoreImport)(HDRVSTORE, LPCWSTR, USHORT, LPCWSTR, DWORD, LPWSTR, DWORD) = NULL;

	if (DriverStoreHandle == NULL || DriverStoreHandle == INVALID_HANDLE_VALUE)
		return ERROR_INVALID_HANDLE;
	*(FARPROC*)&pfnDriverStoreImport = GetProcAddress(m_drvstore_dll, "DriverStoreImportW");
	if (!pfnDriverStoreImport)
		return ERROR_PROC_NOT_FOUND;
	return pfnDriverStoreImport(DriverStoreHandle, DriverPackageFileName, ProcessorArchitecture,
		LocaleName, Flags, DriverStoreFileName, DriverStoreFileNameSize);
}

DWORD
NWL_DriverStoreOfflineAddDriverPackage(LPCWSTR DriverPackageInfPath, DWORD Flags, HANDLE Reserved,
	USHORT ProcessorArchitecture, LPCWSTR LocaleName, LPWSTR DestInfPath, PDWORD CchDestInfPath,
	LPCWSTR TargetSystemRoot, LPCWSTR TargetSystemDrive)
{
	DWORD(WINAPI * pfnDriverStoreOfflineAddDriverPackage)(LPCWSTR, DWORD, HANDLE, USHORT,
		LPCWSTR, LPWSTR, PDWORD, LPCWSTR, LPCWSTR) = NULL;

	if (!m_drvstore_dll)
		return ERROR_PROC_NOT_FOUND;
	*(FARPROC*)&pfnDriverStoreOfflineAddDriverPackage = GetProcAddress(m_drvstore_dll, "DriverStoreOfflineAddDriverPackageW");
	if (!pfnDriverStoreOfflineAddDriverPackage)
		return ERROR_PROC_NOT_FOUND;
	return pfnDriverStoreOfflineAddDriverPackage(DriverPackageInfPath, Flags, Reserved,
		ProcessorArchitecture, LocaleName, DestInfPath, CchDestInfPath, TargetSystemRoot, TargetSystemDrive);
}

DWORD
NWL_DriverStoreUpdateDevices(HDRVSTORE DriverStoreHandle, LPCWSTR DriverStoreFilename,
	DWORD Flags, LPCWSTR SourceFilter, LPCWSTR TargetFilter)
{
	DWORD(WINAPI * pfnDriverStoreUpdateDevices)(HDRVSTORE, LPCWSTR, DWORD, LPCWSTR, LPCWSTR) = NULL;

	if (DriverStoreHandle == NULL || DriverStoreHandle == INVALID_HANDLE_VALUE)
		return ERROR_INVALID_HANDLE;
	*(FARPROC*)&pfnDriverStoreUpdateDevices = GetProcAddress(m_drvstore_dll, "DriverStoreUpdateDevicesW");
	if (!pfnDriverStoreUpdateDevices)
		return ERROR_PROC_NOT_FOUND;
	return pfnDriverStoreUpdateDevices(DriverStoreHandle, DriverStoreFilename, Flags, SourceFilter, TargetFilter);
}

DWORD
NWL_DriverStoreDelete(HDRVSTORE DriverStoreHandle, LPCWSTR DriverStoreFilename, DWORD Flags)
{
	DWORD(WINAPI * pfnDriverStoreDelete)(HDRVSTORE, LPCWSTR, DWORD) = NULL;

	if (DriverStoreHandle == NULL || DriverStoreHandle == INVALID_HANDLE_VALUE)
		return ERROR_INVALID_HANDLE;
	*(FARPROC*)&pfnDriverStoreDelete = GetProcAddress(m_drvstore_dll, "DriverStoreDeleteW");
	if (!pfnDriverStoreDelete)
		return ERROR_PROC_NOT_FOUND;
	return pfnDriverStoreDelete(DriverStoreHandle, DriverStoreFilename, Flags);
}

DWORD
NWL_DriverStoreConfigure(HDRVSTORE DriverStoreHandle, LPCWSTR DriverStoreFilename,
	DWORD Flags, LPCWSTR SourceFilter, LPCWSTR TargetFilter)
{
	DWORD(WINAPI * pfnDriverStoreConfigure)(HDRVSTORE, LPCWSTR, DWORD, LPCWSTR, LPCWSTR) = NULL;

	if (DriverStoreHandle == NULL || DriverStoreHandle == INVALID_HANDLE_VALUE)
		return ERROR_INVALID_HANDLE;
	*(FARPROC*)&pfnDriverStoreConfigure = GetProcAddress(m_drvstore_dll, "DriverStoreConfigureW");
	if (!pfnDriverStoreConfigure)
		return ERROR_PROC_NOT_FOUND;
	return pfnDriverStoreConfigure(DriverStoreHandle, DriverStoreFilename, Flags, SourceFilter, TargetFilter);
}

DWORD
NWL_DriverStoreReflectCritical(HDRVSTORE DriverStoreHandle, LPCWSTR DriverStoreFilename, DWORD Flags, LPCWSTR FilterDeviceId)
{
	DWORD(WINAPI * pfnDriverStoreReflectCritical)(HDRVSTORE, LPCWSTR, DWORD, LPCWSTR) = NULL;

	if (DriverStoreHandle == NULL || DriverStoreHandle == INVALID_HANDLE_VALUE)
		return ERROR_INVALID_HANDLE;
	*(FARPROC*)&pfnDriverStoreReflectCritical = GetProcAddress(m_drvstore_dll, "DriverStoreReflectCriticalW");
	if (!pfnDriverStoreReflectCritical)
		return ERROR_PROC_NOT_FOUND;
	return pfnDriverStoreReflectCritical(DriverStoreHandle, DriverStoreFilename, Flags, FilterDeviceId);
}

DWORD
NWL_DriverStoreReflect(HDRVSTORE DriverStoreHandle, LPCWSTR DriverStoreFilename, DWORD Flags, LPCWSTR FilterSectionNames)
{
	DWORD(WINAPI * pfnDriverStoreReflect)(HDRVSTORE, LPCWSTR, DWORD, LPCWSTR) = NULL;

	if (DriverStoreHandle == NULL || DriverStoreHandle == INVALID_HANDLE_VALUE)
		return ERROR_INVALID_HANDLE;
	*(FARPROC*)&pfnDriverStoreReflect = GetProcAddress(m_drvstore_dll, "DriverStoreReflectW");
	if (!pfnDriverStoreReflect)
		return ERROR_PROC_NOT_FOUND;
	return pfnDriverStoreReflect(DriverStoreHandle, DriverStoreFilename, Flags, FilterSectionNames);
}

DWORD
NWL_DriverStorePublish(HDRVSTORE DriverStoreHandle, LPCWSTR DriverStoreFilename,
	DWORD Flags, LPWSTR PublishedFileName, DWORD PublishedFileNameSize, PBOOL IsPublishedFileNameChanged)
{
	DWORD(WINAPI * pfnDriverStorePublish)(HDRVSTORE, LPCWSTR, DWORD, LPWSTR, DWORD, PBOOL) = NULL;

	if (DriverStoreHandle == NULL || DriverStoreHandle == INVALID_HANDLE_VALUE)
		return ERROR_INVALID_HANDLE;
	*(FARPROC*)&pfnDriverStorePublish = GetProcAddress(m_drvstore_dll, "DriverStorePublishW");
	if (!pfnDriverStorePublish)
		return ERROR_PROC_NOT_FOUND;
	return pfnDriverStorePublish(DriverStoreHandle, DriverStoreFilename, Flags,
		PublishedFileName, PublishedFileNameSize, IsPublishedFileNameChanged);
}

BOOL
NWL_DriverStoreEnumObjects(HDRVSTORE DriverStoreHandle, DWORD ObjectType, DWORD Flags,
	PDRIVERSTORE_ENUM_OBJECTS_CALLBACK CallbackRoutine, LPARAM Context)
{
	BOOL(WINAPI * pfnDriverStoreEnumObjects)(HDRVSTORE, DWORD, DWORD, PDRIVERSTORE_ENUM_OBJECTS_CALLBACK, LPARAM) = NULL;

	if (DriverStoreHandle == NULL || DriverStoreHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	*(FARPROC*)&pfnDriverStoreEnumObjects = GetProcAddress(m_drvstore_dll, "DriverStoreEnumObjectsW");
	if (!pfnDriverStoreEnumObjects)
		return FALSE;
	return pfnDriverStoreEnumObjects(DriverStoreHandle, ObjectType, Flags, CallbackRoutine, Context);
}

BOOL
NWL_DriverStoreSetObjectProperty(HDRVSTORE DriverStoreHandle, DWORD ObjectType, LPCWSTR ObjectName,
	CONST DEVPROPKEY* PropertyKey, DEVPROPTYPE PropertyType, LPCVOID PropertyBuffer, DWORD PropertySize, DWORD Flags)
{
	BOOL(WINAPI * pfnDriverStoreSetObjectProperty)(HDRVSTORE, DWORD, LPCWSTR,
		CONST DEVPROPKEY*, DEVPROPTYPE, LPCVOID, DWORD, DWORD) = NULL;

	if (DriverStoreHandle == NULL || DriverStoreHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	*(FARPROC*)&pfnDriverStoreSetObjectProperty = GetProcAddress(m_drvstore_dll, "DriverStoreSetObjectPropertyW");
	if (!pfnDriverStoreSetObjectProperty)
		return FALSE;
	return pfnDriverStoreSetObjectProperty(DriverStoreHandle, ObjectType, ObjectName,
		PropertyKey, PropertyType, PropertyBuffer, PropertySize, Flags);
}

BOOL
NWL_DriverStoreGetObjectProperty(HDRVSTORE DriverStoreHandle, DWORD ObjectType, LPCWSTR ObjectName,
	CONST DEVPROPKEY* PropertyKey, DEVPROPTYPE* PropertyType, PBYTE PropertyBuffer, DWORD BufferSize,
	PDWORD PropertySize, DWORD Flags)
{
	BOOL(WINAPI * pfnDriverStoreGetObjectProperty)(HDRVSTORE, DWORD, LPCWSTR,
		CONST DEVPROPKEY*, DEVPROPTYPE*, PBYTE, DWORD, PDWORD, DWORD) = NULL;

	if (DriverStoreHandle == NULL || DriverStoreHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	*(FARPROC*)&pfnDriverStoreGetObjectProperty = GetProcAddress(m_drvstore_dll, "DriverStoreGetObjectPropertyW");
	if (!pfnDriverStoreGetObjectProperty)
		return FALSE;
	return pfnDriverStoreGetObjectProperty(DriverStoreHandle, ObjectType, ObjectName,
		PropertyKey, PropertyType, PropertyBuffer, BufferSize, PropertySize, Flags);
}

HDRIVERPACKAGE
NWL_DriverPackageOpen(LPCWSTR DriverPackageFilename, USHORT ProcessorArchitecture,
	LPCWSTR LocaleName, DWORD Flags, HANDLE ResolveContext)
{
	HDRIVERPACKAGE(WINAPI * pfnDriverPackageOpen)(LPCWSTR, USHORT, LPCWSTR, DWORD, HANDLE) = NULL;

	if (!m_drvstore_dll)
		return NULL;
	*(FARPROC*)&pfnDriverPackageOpen = GetProcAddress(m_drvstore_dll, "DriverPackageOpenW");
	if (!pfnDriverPackageOpen)
		return NULL;
	return pfnDriverPackageOpen(DriverPackageFilename, ProcessorArchitecture, LocaleName, Flags, ResolveContext);
}

BOOL
NWL_DriverPackageEnumFiles(HDRIVERPACKAGE DriverPackageHandle, PVOID EnumContext,
	DWORD Flags, PDRIVER_PACKAGE_ENUM_FILES_CALLBACK CallbackRoutine, LPARAM Context)
{
	BOOL(WINAPI * pfnDriverPackageEnumFiles)(HDRIVERPACKAGE, PVOID, DWORD,
		PDRIVER_PACKAGE_ENUM_FILES_CALLBACK, LPARAM) = NULL;

	if (DriverPackageHandle == NULL || DriverPackageHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	*(FARPROC*)&pfnDriverPackageEnumFiles = GetProcAddress(m_drvstore_dll, "DriverPackageEnumFilesW");
	if (!pfnDriverPackageEnumFiles)
		return FALSE;
	return pfnDriverPackageEnumFiles(DriverPackageHandle, EnumContext, Flags, CallbackRoutine, Context);
}

BOOL
NWL_DriverPackageGetVersionInfo(HDRIVERPACKAGE DriverPackageHandle, PDRIVER_PACKAGE_VERSION_INFO VersionInfo)
{
	BOOL(WINAPI * pfnDriverPackageGetVersionInfo)(HDRIVERPACKAGE, PDRIVER_PACKAGE_VERSION_INFO) = NULL;

	if (DriverPackageHandle == NULL || DriverPackageHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	*(FARPROC*)&pfnDriverPackageGetVersionInfo = GetProcAddress(m_drvstore_dll, "DriverPackageGetVersionInfoW");
	if (!pfnDriverPackageGetVersionInfo)
		return FALSE;
	return pfnDriverPackageGetVersionInfo(DriverPackageHandle, VersionInfo);
}

BOOL
NWL_DriverPackageGetProperty(HDRIVERPACKAGE DriverPackageHandle, PVOID EnumContext,
	LPCWSTR SectionName, CONST DEVPROPKEY* PropertyKey, DEVPROPTYPE* PropertyType, PVOID PropertyBuffer,
	DWORD BufferSize, PDWORD PropertySize, DWORD Flags)
{
	BOOL(WINAPI * pfnDriverPackageGetProperty)(HDRIVERPACKAGE, PVOID, LPCWSTR,
		CONST DEVPROPKEY*, DEVPROPTYPE*, PVOID, DWORD, PDWORD, DWORD) = NULL;

	if (DriverPackageHandle == NULL || DriverPackageHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	*(FARPROC*)&pfnDriverPackageGetProperty = GetProcAddress(m_drvstore_dll, "DriverPackageGetPropertyW");
	if (!pfnDriverPackageGetProperty)
		return FALSE;
	return pfnDriverPackageGetProperty(DriverPackageHandle, EnumContext, SectionName,
		PropertyKey, PropertyType, PropertyBuffer, BufferSize, PropertySize, Flags);
}

VOID
NWL_DriverPackageClose(HDRIVERPACKAGE DriverPackageHandle)
{
	VOID(WINAPI * pfnDriverPackageClose)(HDRIVERPACKAGE) = NULL;

	if (DriverPackageHandle == NULL || DriverPackageHandle == INVALID_HANDLE_VALUE)
		return;
	*(FARPROC*)&pfnDriverPackageClose = GetProcAddress(m_drvstore_dll, "DriverPackageClose");
	if (!pfnDriverPackageClose)
		return;
	pfnDriverPackageClose(DriverPackageHandle);
}

DWORD
NWL_DriverStoreCopy(HDRVSTORE DriverStoreHandle, LPCWSTR DriverPackageFilename,
	USHORT ProcessorArchitecture, LPCWSTR LocaleName, DWORD Flags, LPCWSTR DestinationPath)
{
	DWORD(WINAPI * pfnDriverStoreCopy)(HDRVSTORE, LPCWSTR, USHORT, LPCWSTR, DWORD, LPCWSTR) = NULL;

	if (DriverStoreHandle == NULL || DriverStoreHandle == INVALID_HANDLE_VALUE)
		return ERROR_INVALID_HANDLE;
	*(FARPROC*)&pfnDriverStoreCopy = GetProcAddress(m_drvstore_dll, "DriverStoreCopyW");
	if (!pfnDriverStoreCopy)
		return ERROR_PROC_NOT_FOUND;
	return pfnDriverStoreCopy(DriverStoreHandle, DriverPackageFilename,
		ProcessorArchitecture, LocaleName, Flags, DestinationPath);
}

static LPCWSTR
DrvStoreGetFileName(LPCWSTR path)
{
	LPCWSTR p = wcsrchr(path, L'\\');
	return p ? p + 1 : path;
}

PNODE_ATT
NWL_DrvStoreSetProperty(PNODE node, HDRVSTORE hDrvStore, DWORD objType, LPCWSTR objName, LPCSTR name, const DEVPROPKEY* propKey)
{
	DEVPROPTYPE propType = DEVPROP_TYPE_EMPTY;
	DWORD propSize = 0;
	PNODE_ATT ret = NULL;

	NWL_DriverStoreGetObjectProperty(hDrvStore, objType, objName, propKey, &propType, NULL, 0, &propSize, 0);
	if (propSize == 0)
		return NULL;

	switch (propType)
	{
	case DEVPROP_TYPE_STRING:
	case DEVPROP_TYPE_STRING_INDIRECT:
		ZeroMemory(NWLC->NwBuf, NWINFO_BUFSZ);
		if (NWL_DriverStoreGetObjectProperty(hDrvStore, objType, objName,
			propKey, &propType, (PBYTE)NWLC->NwBuf, NWINFO_BUFSZ, &propSize, 0))
			ret = NWL_NodeAttrSet(node, name, NWL_Ucs2ToUtf8(NWLC->NwBufW), 0);
		break;
	case DEVPROP_TYPE_STRING_LIST:
		ZeroMemory(NWLC->NwBuf, NWINFO_BUFSZ);
		if (NWL_DriverStoreGetObjectProperty(hDrvStore, objType, objName,
			propKey, &propType, (PBYTE)NWLC->NwBuf, NWINFO_BUFSZ, &propSize, 0))
		{
			LPSTR ms = NULL;
			for (LPCWSTR p = NWLC->NwBufW; *p; p += wcslen(p) + 1)
				NWL_NodeAppendMultiSz(&ms, NWL_Ucs2ToUtf8(p));
			if (ms)
			{
				ret = NWL_NodeAttrSetMulti(node, name, ms, 0);
				free(ms);
			}
		}
		break;
	case DEVPROP_TYPE_GUID:
	{
		GUID guid;
		if (NWL_DriverStoreGetObjectProperty(hDrvStore, objType, objName,
			propKey, &propType, (PBYTE)&guid, sizeof(GUID), &propSize, 0))
			ret = NWL_NodeAttrSet(node, name, NWL_WinGuidToStr(TRUE, &guid), NAFLG_FMT_GUID);
		break;
	}
	case DEVPROP_TYPE_BOOLEAN:
	{
		DEVPROP_BOOLEAN value;
		if (NWL_DriverStoreGetObjectProperty(hDrvStore, objType, objName,
			propKey, &propType, (PBYTE)&value, sizeof(DEVPROP_BOOLEAN), &propSize, 0))
			ret = NWL_NodeAttrSetBool(node, name, value, 0);
		break;
	}
	case DEVPROP_TYPE_UINT16:
	{
		UINT16 value;
		if (NWL_DriverStoreGetObjectProperty(hDrvStore, objType, objName,
			propKey, &propType, (PBYTE)&value, sizeof(UINT16), &propSize, 0))
			ret = NWL_NodeAttrSetf(node, name, NAFLG_FMT_NUMERIC, "%u", value);
		break;
	}
	case DEVPROP_TYPE_UINT32:
	{
		UINT32 value;
		if (NWL_DriverStoreGetObjectProperty(hDrvStore, objType, objName,
			propKey, &propType, (PBYTE)&value, sizeof(UINT32), &propSize, 0))
			ret = NWL_NodeAttrSetf(node, name, NAFLG_FMT_NUMERIC, "%u", value);
		break;
	}
	case DEVPROP_TYPE_UINT64:
	{
		UINT64 value;
		if (NWL_DriverStoreGetObjectProperty(hDrvStore, objType, objName,
			propKey, &propType, (PBYTE)&value, sizeof(UINT64), &propSize, 0))
		{
			if (memcmp(propKey, &DEVPKEY_DriverPackage_DriverVersion, sizeof(DEVPROPKEY)) == 0)
			{
				ret = NWL_NodeAttrSetf(node, name, 0, "%u.%u.%u.%u",
					(UINT)((value >> 48) & 0xffff),
					(UINT)((value >> 32) & 0xffff),
					(UINT)((value >> 16) & 0xffff),
					(UINT)(value & 0xffff));
			}
			else
				ret = NWL_NodeAttrSetf(node, name, NAFLG_FMT_NUMERIC, "%llu", value);
		}
		break;
	}
	case DEVPROP_TYPE_FILETIME:
	{
		FILETIME value;
		if (NWL_DriverStoreGetObjectProperty(hDrvStore, objType, objName,
			propKey, &propType, (PBYTE)&value, sizeof(FILETIME), &propSize, 0))
		{
			SYSTEMTIME sysTime;
			if (FileTimeToSystemTime(&value, &sysTime))
				ret = NWL_NodeAttrSetf(node, name, 0, "%04u-%02u-%02u", sysTime.wYear, sysTime.wMonth, sysTime.wDay);
		}
		break;
	}
	default:
		break;
	}
	return ret;
}

static BOOL CALLBACK
DrvStoreEnumPackages(HDRVSTORE hDrvStore, LPCWSTR drvStorePath,
	PDRIVER_PACKAGE_INFO pkgInfo, LPARAM context)
{
	PNODE root = (PNODE)context;
	PNODE node = NWL_NodeAlloc("Driver", NFLG_TABLE_ROW);
	PNODE_ATT attr;

	attr = NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Name", &DEVPKEY_DriverPackage_ProductName);
	if (attr == NULL)
		attr = NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Name", &DEVPKEY_DriverPackage_DriverPackageId);
	if (attr == NULL)
	{
		WCHAR name[MAX_PATH];
		wcsncpy_s(name, ARRAYSIZE(name), DrvStoreGetFileName(drvStorePath), _TRUNCATE);
		LPWSTR ext = wcsrchr(name, L'.');
		if (ext && _wcsicmp(ext, L".inf") == 0)
			*ext = L'\0';
		NWL_NodeAttrSet(node, "Name", NWL_Ucs2ToUtf8(name), 0);
	}

	attr = NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Inf Name", &DEVPKEY_DriverPackage_DriverInfName);
	if (attr == NULL)
		NWL_NodeAttrSet(node, "Inf Name", NWL_Ucs2ToUtf8(DrvStoreGetFileName(drvStorePath)), 0);

	NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Original Inf Name", &DEVPKEY_DriverPackage_OriginalInfName);
	if (pkgInfo->PublishedInfName[0])
		NWL_NodeAttrSet(node, "Published Inf Name", NWL_Ucs2ToUtf8(pkgInfo->PublishedInfName), 0);

	NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Provider", &DEVPKEY_DriverPackage_ProviderName);
	NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Version", &DEVPKEY_DriverPackage_DriverVersion);
	NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Date", &DEVPKEY_DriverPackage_DriverDate);

	LPCSTR group_name = "Unknown";

	attr = NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Class GUID", &DEVPKEY_DriverPackage_ClassGuid);
	if (attr)
	{
		group_name = attr->value;
		attr = NWL_DrvStoreSetProperty(node, hDrvStore, DeviceSetupClass, NWL_Utf8ToUcs2(attr->value), "Type", &DEVPKEY_DeviceClass_ClassName);
		if (attr)
			group_name = attr->value;
	}

	PNODE group = NWL_NodeGetChild(root, group_name);
	if (group == NULL)
		group = NWL_NodeAppendNew(root, group_name, NFLG_TABLE);
	NWL_NodeAppendChild(group, node);

	NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Signer", &DEVPKEY_DriverPackage_SignerName);
	NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Package ID", &DEVPKEY_DriverPackage_DriverPackageId);
	NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Catalog", &DEVPKEY_DriverPackage_CatalogFile);
	NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Import Date", &DEVPKEY_DriverPackage_ImportDate);
	NWL_NodeAttrSetBool(node, "Inbox", pkgInfo->Flags & DRIVER_PACKAGE_INBOX, 0);
	NWL_NodeAttrSetBool(node, "OEM", pkgInfo->Flags & DRIVER_PACKAGE_OEM, 0);
	NWL_NodeAttrSetBool(node, "Published", pkgInfo->Flags & DRIVER_PACKAGE_PUBLISHED, 0);
	NWL_NodeAttrSetBool(node, "F6", pkgInfo->Flags & DRIVER_PACKAGE_F6, 0);
	NWL_NodeAttrSetBool(node, "Base Version", pkgInfo->Flags & DRIVER_PACKAGE_BASEVERSION, 0);
	NWL_DrvStoreSetProperty(node, hDrvStore, DriverPackage, drvStorePath, "Primitive", &DEVPKEY_DriverPackage_Primitive);
	NWL_NodeAttrSet(node, "Path", NWL_Ucs2ToUtf8(drvStorePath), 0);
	return TRUE;
}

PNODE NW_DrvStore(BOOL bAppend)
{
	PNODE node = NWL_NodeAlloc("DrvStore", 0);

	if (bAppend)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	HDRVSTORE hDrvStore = NWL_DriverStoreOpen(NWLC->DrvStoreDrive, DRIVERSTORE_OPEN_NONE);
	if (!hDrvStore)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "DriverStoreOpen failed");
		return node;
	}

	if (!NWL_DriverStoreEnum(hDrvStore, DRIVERSTORE_ENUM_NONE, DrvStoreEnumPackages, (LPARAM)node))
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "DriverStoreEnum failed");

	NWL_DriverStoreClose(hDrvStore);
	return node;
}
