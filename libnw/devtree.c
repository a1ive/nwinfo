// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <initguid.h>
#include <devpkey.h>
#include <devpropdef.h>

#include "libnw.h"
#include "utils.h"
#include "devtree.h"

static CONFIGRET
GetDevNodeProperty(DEVINST devHandle, const DEVPROPKEY* devProperty, DEVPROPTYPE* propertyType)
{
	ULONG ulRegType;
	ULONG cmDrpProp = (ULONG)-1;
	ULONG bufferSize = NWINFO_BUFSZ;
	CONFIGRET cr;
	HKEY hKey;

	if (NWLC->NwOsInfo.dwMajorVersion >= 6 &&
		CM_Get_DevNode_PropertyW(devHandle, devProperty, propertyType, (PBYTE)NWLC->NwBuf, &bufferSize, 0) == CR_SUCCESS)
		return CR_SUCCESS;

	if (IsEqualDevPropKey(*devProperty, DEVPKEY_Device_HardwareIds))
	{
		cr = CM_Get_DevNode_Registry_PropertyW(devHandle, CM_DRP_HARDWAREID, &ulRegType, (PBYTE)NWLC->NwBuf, &bufferSize, 0);
		*propertyType = DEVPROP_TYPE_STRING_LIST;
	}
	else if (IsEqualDevPropKey(*devProperty, DEVPKEY_Device_CompatibleIds))
	{
		cr = CM_Get_DevNode_Registry_PropertyW(devHandle, CM_DRP_COMPATIBLEIDS, &ulRegType, (PBYTE)NWLC->NwBuf, &bufferSize, 0);
		*propertyType = DEVPROP_TYPE_STRING_LIST;
	}
	else if (IsEqualDevPropKey(*devProperty, DEVPKEY_NAME))
	{
		cr = CM_Get_DevNode_Registry_PropertyW(devHandle, CM_DRP_DEVICEDESC, &ulRegType, (PBYTE)NWLC->NwBuf, &bufferSize, 0);
		*propertyType = DEVPROP_TYPE_STRING;
	}
	else if (IsEqualDevPropKey(*devProperty, DEVPKEY_Device_Class))
	{
		cr = CM_Get_DevNode_Registry_PropertyW(devHandle, CM_DRP_CLASS, &ulRegType, (PBYTE)NWLC->NwBuf, &bufferSize, 0);
		*propertyType = DEVPROP_TYPE_STRING;
	}
	else if (IsEqualDevPropKey(*devProperty, DEVPKEY_Device_Manufacturer))
	{
		cr = CM_Get_DevNode_Registry_PropertyW(devHandle, CM_DRP_MFG, &ulRegType, (PBYTE)NWLC->NwBuf, &bufferSize, 0);
		*propertyType = DEVPROP_TYPE_STRING;
	}
	else if (IsEqualDevPropKey(*devProperty, DEVPKEY_Device_Service))
	{
		cr = CM_Get_DevNode_Registry_PropertyW(devHandle, CM_DRP_SERVICE, &ulRegType, (PBYTE)NWLC->NwBuf, &bufferSize, 0);
		*propertyType = DEVPROP_TYPE_STRING;
	}
	else if (IsEqualDevPropKey(*devProperty, DEVPKEY_Device_LocationInfo))
	{
		cr = CM_Get_DevNode_Registry_PropertyW(devHandle, CM_DRP_LOCATION_INFORMATION, &ulRegType, (PBYTE)NWLC->NwBuf, &bufferSize, 0);
		*propertyType = DEVPROP_TYPE_STRING;
	}
	else if (IsEqualDevPropKey(*devProperty, DEVPKEY_Device_DriverVersion))
	{
		cr = CM_Open_DevNode_Key(devHandle, KEY_READ, 0, RegDisposition_OpenExisting, &hKey, CM_REGISTRY_SOFTWARE);
		if (cr != CR_SUCCESS)
			goto out;
		if (RegQueryValueExW(hKey, L"DriverVersion", NULL, &ulRegType, (PBYTE)NWLC->NwBuf, &bufferSize) != ERROR_SUCCESS)
			cr = CR_FAILURE;
		*propertyType = DEVPROP_TYPE_STRING;
		RegCloseKey(hKey);
	}
	else if (IsEqualDevPropKey(*devProperty, DEVPKEY_Device_DriverDate))
	{
		cr = CM_Open_DevNode_Key(devHandle, KEY_READ, 0, RegDisposition_OpenExisting, &hKey, CM_REGISTRY_SOFTWARE);
		if (cr != CR_SUCCESS)
			goto out;
		if (RegQueryValueExW(hKey, L"DriverDateData", NULL, &ulRegType, (PBYTE)NWLC->NwBuf, &bufferSize) != ERROR_SUCCESS)
			cr = CR_FAILURE;
		*propertyType = DEVPROP_TYPE_FILETIME;
		RegCloseKey(hKey);
	}
	else
		return CR_NO_SUCH_VALUE;

out:
	return cr;
}

BOOL
NWL_SetDevPropString(CHAR* strBuf, size_t strSize, DEVINST devHandle, const DEVPROPKEY* devProperty)
{
	ULONG bufferSize = NWINFO_BUFSZ;
	DEVPROPTYPE propertyType;

	propertyType = DEVPROP_TYPE_EMPTY;

	if (GetDevNodeProperty(devHandle, devProperty, &propertyType) != CR_SUCCESS)
		return FALSE;

	ZeroMemory(strBuf, strSize);

	switch (propertyType)
	{
	case DEVPROP_TYPE_STRING:
	case DEVPROP_TYPE_STRING_LIST: // TODO: add multi sz support
		strcpy_s(strBuf, strSize, NWL_Ucs2ToUtf8(NWLC->NwBufW));
		break;
	case DEVPROP_TYPE_FILETIME:
	{
		SYSTEMTIME sysTime = { 0 };
		FileTimeToSystemTime((PFILETIME)NWLC->NwBuf, &sysTime);
		snprintf(strBuf, strSize, "%5u-%02u-%02u %02u:%02u%02u",
			sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	}
		break;
	case DEVPROP_TYPE_UINT32:
	{
		UINT32 u;
		memcpy(&u, NWLC->NwBuf, sizeof(UINT32));
		snprintf(strBuf, strSize, "%u", u);
	}
		break;
	case DEVPROP_TYPE_UINT64:
	{
		UINT64 u;
		memcpy(&u, NWLC->NwBuf, sizeof(UINT64));
		snprintf(strBuf, strSize, "%llu", u);
	}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

CONFIGRET
NWL_CMGetDevIfProp(LPCWSTR pszDevIf, CONST DEVPROPKEY* propKey, DEVPROPTYPE* propType, PBYTE propBuf, PULONG propBufSize, ULONG ulFlags)
{
	CONFIGRET rc = CR_FAILURE;
	CONFIGRET(WINAPI * OsCMGetDeviceInterfaceProperty) (LPCWSTR, CONST DEVPROPKEY*, DEVPROPTYPE*, PBYTE, PULONG, ULONG) = NULL;
	HMODULE hDll = LoadLibraryW(L"cfgmgr32.dll");
	if (hDll == NULL)
		return CR_FAILURE;
	*(FARPROC*)&OsCMGetDeviceInterfaceProperty = GetProcAddress(hDll, "CM_Get_Device_Interface_PropertyW");
	if (OsCMGetDeviceInterfaceProperty == NULL)
		goto fail;
	rc = OsCMGetDeviceInterfaceProperty(pszDevIf, propKey, propType, propBuf, propBufSize, ulFlags);
fail:
	if (hDll)
		FreeLibrary(hDll);
	return rc;
}

static void
GetDeviceInfoDefault(PNODE node, void* data, DEVINST devInst, LPCSTR hwIds)
{
	(void)data;
	NWL_NodeAttrSet(node, "HWID", hwIds, 0);

	CHAR buf[DEVTREE_MAX_STR_LEN];

	if (NWL_SetDevPropString(buf, DEVTREE_MAX_STR_LEN, devInst, &DEVPKEY_NAME))
		NWL_NodeAttrSet(node, "Name", buf, 0);

	if (NWL_SetDevPropString(buf, DEVTREE_MAX_STR_LEN, devInst, &DEVPKEY_Device_Class))
		NWL_NodeAttrSet(node, "Device Class", buf, 0);

	if (NWL_SetDevPropString(buf, DEVTREE_MAX_STR_LEN, devInst, &DEVPKEY_Device_Manufacturer))
		NWL_NodeAttrSet(node, "Manufacturer", buf, 0);

	if (NWL_SetDevPropString(buf, DEVTREE_MAX_STR_LEN, devInst, &DEVPKEY_Device_Service))
		NWL_NodeAttrSet(node, "Service Name", buf, 0);

	if (NWL_SetDevPropString(buf, DEVTREE_MAX_STR_LEN, devInst, &DEVPKEY_Device_DriverDate))
		NWL_NodeAttrSet(node, "Driver Date", buf, 0);

	if (NWL_SetDevPropString(buf, DEVTREE_MAX_STR_LEN, devInst, &DEVPKEY_Device_DriverVersion))
		NWL_NodeAttrSet(node, "Driver Version", buf, 0);

	if (NWL_SetDevPropString(buf, DEVTREE_MAX_STR_LEN, devInst, &DEVPKEY_Device_LocationInfo))
		NWL_NodeAttrSet(node, "Location", buf, 0);

}

static PNODE AppendDevices(PNODE parent, const char* hub)
{
	if (parent->Flags & NFLG_TABLE)
		return parent;
	PNODE ret = NWL_NodeGetChild(parent, hub);
	if (ret)
		return ret;
	return NWL_NodeAppendNew(parent, hub, NFLG_TABLE);
}

void
NWL_EnumerateDevices(PNODE parent, DEVTREE_ENUM_CTX* ctx, DEVINST devInst)
{
	PNODE node = parent;
	DEVINST childInst;
	CHAR hwIds[DEVTREE_MAX_STR_LEN];

	if (NWL_SetDevPropString(hwIds, DEVTREE_MAX_STR_LEN, devInst, &DEVPKEY_Device_HardwareIds))
	{
		if (ctx->filter[0] == L'\0' || _strnicmp(hwIds, ctx->filter, ctx->filterLen) == 0)
		{
			node = NWL_NodeAppendNew(AppendDevices(parent, ctx->hub), "Device", NFLG_TABLE_ROW);
			ctx->GetDeviceInfo(node, ctx->data, devInst, hwIds);
		}
	}

	if (CM_Get_Child(&childInst, devInst, 0) == CR_SUCCESS)
	{
		NWL_EnumerateDevices(node, ctx, childInst);
		DEVINST siblingInst = childInst;
		while (CM_Get_Sibling(&siblingInst, siblingInst, 0) == CR_SUCCESS)
			NWL_EnumerateDevices(node, ctx, siblingInst);
	}
}

PNODE NW_DevTree(VOID)
{
	DEVTREE_ENUM_CTX ctx =
	{
		.filter = "\0",
		.data = NULL,
		.hub = "Devices",
		.GetDeviceInfo = GetDeviceInfoDefault,
	};
	DEVINST devRoot;
	CONFIGRET cr;
	PNODE node = NWL_NodeAlloc("Device Tree", NFLG_TABLE);
	if (NWLC->DevTree)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	if (NWLC->DevTreeFilter)
	{
		strcpy_s(ctx.filter, DEVTREE_MAX_STR_LEN, NWLC->DevTreeFilter);
		ctx.filterLen = strlen(ctx.filter);
	}

	cr = CM_Locate_DevNodeW(&devRoot, NULL, CM_LOCATE_DEVNODE_NORMAL);
	if (cr != CR_SUCCESS)
	{
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "CM_Locate_DevNodeW failed");
		goto fail;
	}

	NWL_EnumerateDevices(node, &ctx, devRoot);

fail:
	return node;
}
