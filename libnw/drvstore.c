// SPDX-License-Identifier: Unlicense

#include "drvstore.h"

static HMODULE m_drvstore_dll = NULL;

HDRVSTORE
NWL_DriverStoreOpen(LPCWSTR TargetSystemPath, LPCWSTR TargetBootDrive, DWORD Flags)
{
	if (!m_drvstore_dll)
		m_drvstore_dll = LoadLibraryW(L"drvstore.dll");
	if (!m_drvstore_dll)
		return NULL;
	HDRVSTORE (WINAPI* pfnDriverStoreOpen)(LPCWSTR, LPCWSTR, DWORD, HANDLE) = NULL;

	*(FARPROC*)&pfnDriverStoreOpen = GetProcAddress(m_drvstore_dll, "DriverStoreOpenW");
	if (!pfnDriverStoreOpen)
		return NULL;
	return pfnDriverStoreOpen(TargetSystemPath, TargetBootDrive, Flags, NULL);
}

BOOL
NWL_DriverStoreClose(HDRVSTORE DriverStoreHandle)
{
	BOOL(WINAPI * pfnDriverStoreClose)(HDRVSTORE) = NULL;

	if (DriverStoreHandle == NULL || DriverStoreHandle == INVALID_HANDLE_VALUE)
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

