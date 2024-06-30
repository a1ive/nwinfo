// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"
#include <libcpuid.h>
#include <winring0.h>
#include "../libcdi/libcdi.h"
#include "version.h"

static void
PrintDriverVerison(PNODE node, struct wr0_drv_t* drv)
{
	DWORD dwLen;
	LPVOID pBlock = NULL;
	UINT uLen;
	VS_FIXEDFILEINFO *pInfo;
	dwLen = GetFileVersionInfoSizeW(drv->driver_path, NULL);
	if (!dwLen)
		return;
	pBlock = malloc(dwLen);
	if (!pBlock)
		return;
	if (!GetFileVersionInfoW(drv->driver_path, 0, dwLen, pBlock))
		goto fail;
	if (!VerQueryValueW(pBlock, L"\\", &pInfo, &uLen))
		goto fail;
	NWL_NodeAttrSetf(node, "Driver Version", 0, "%u.%u.%u.%u",
		(pInfo->dwFileVersionMS >> 16) & 0xffff, pInfo->dwFileVersionMS & 0xffff,
		(pInfo->dwFileVersionLS >> 16) & 0xffff, pInfo->dwFileVersionLS & 0xffff);
fail:
	free(pBlock);
	return;
}

PNODE NW_Libinfo(VOID)
{
	PNODE pNode = NWLC->NwRoot;
	NWL_NodeAttrSet(pNode, "Build Time", __DATE__ " " __TIME__, 0);
	NWL_NodeAttrSet(pNode, "libnw", "v" NWINFO_VERSION_STR, 0);
	NWL_NodeAttrSetf(pNode, "MSVC Version", 0, "%u", _MSC_FULL_VER);
	NWL_NodeAttrSetf(pNode, "NT Version", 0, "%lu.%lu.%lu",
		NWLC->NwOsInfo.dwMajorVersion, NWLC->NwOsInfo.dwMinorVersion, NWLC->NwOsInfo.dwBuildNumber);
	if (NWLC->NwDrv)
	{
		NWL_NodeAttrSet(pNode, "Driver", NWL_Ucs2ToUtf8(NWLC->NwDrv->driver_id), 0);
		NWL_NodeAttrSet(pNode, "Driver Path", NWL_Ucs2ToUtf8(NWLC->NwDrv->driver_path), 0);
		PrintDriverVerison(pNode, NWLC->NwDrv);
	}
	else
		NWL_NodeAttrSet(pNode, "Driver", "NOT FOUND", 0);
	NWL_NodeAttrSetf(pNode, "Language ID", 0, "%u", GetUserDefaultUILanguage());
	NWL_NodeAttrSet(pNode, "libcpuid", cpuid_lib_version(), 0);
	NWL_NodeAttrSet(pNode, "CrystalDiskInfo", cdi_get_version(), 0);
	NWL_NodeAttrSet(pNode, "PCI ID", NWL_GetIdsDate(L"pci.ids"), 0);
	NWL_NodeAttrSet(pNode, "USB ID", NWL_GetIdsDate(L"usb.ids"), 0);
	NWL_NodeAttrSet(pNode, "PNP ID", NWL_GetIdsDate(L"pnp.ids"), 0);
	NWL_NodeAttrSet(pNode, "JEP106 ID", NWL_GetIdsDate(L"jep106.ids"), 0);
	NWL_NodeAttrSetMulti(pNode, "Error", NWLC->ErrLog, 0);
	return pNode;
}
