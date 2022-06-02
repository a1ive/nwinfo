// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "../winring0/winring0.h"

struct msr_driver_t
{
	CHAR driver_path[MAX_PATH + 1];
	SC_HANDLE scManager;
	SC_HANDLE scDriver;
	HANDLE hhDriver;
	int errorcode;
};

static void
PrintDriverVerison(PNODE node, struct msr_driver_t* drv)
{
	DWORD dwLen;
	LPVOID pBlock = NULL;
	UINT uLen;
	VS_FIXEDFILEINFO *pInfo;
	dwLen = GetFileVersionInfoSizeA(drv->driver_path, NULL);
	if (!dwLen)
		return;
	pBlock = malloc(dwLen);
	if (!pBlock)
		return;
	if (!GetFileVersionInfoA(drv->driver_path, 0, dwLen, pBlock))
		goto fail;
	if (!VerQueryValueA(pBlock, "\\", &pInfo, &uLen))
		goto fail;
	NWL_NodeAttrSetf(node, "Driver Version", 0, "%u.%u.%u.%u",
		(pInfo->dwFileVersionMS >> 16) & 0xffff, pInfo->dwFileVersionMS & 0xffff,
		(pInfo->dwFileVersionLS >> 16) & 0xffff, pInfo->dwFileVersionLS & 0xffff);
fail:
	free(pBlock);
	return;
}

PNODE GNW_LibInfo(VOID)
{
	PNODE node = NWL_NodeAlloc("LIBINF", 0);
	if (GNWC.nCtx.NwDrv)
	{
		NWL_NodeAttrSet(node, "Driver", OLS_DRIVER_NAME, 0);
		NWL_NodeAttrSet(node, "Driver Path", GNWC.nCtx.NwDrv->driver_path, 0);
		PrintDriverVerison(node, GNWC.nCtx.NwDrv);
	}
	else
		NWL_NodeAttrSet(node, "Driver", "NOT FOUND", 0);
	NWL_NodeAttrSetf(node, "Language ID", 0, "%u", GNWC.wLang);
	NWL_NodeAttrSet(node, "Homepage", GNWINFO_HOMEPAGE, 0);
	NWL_NodeAttrSet(node, "libcpuid", "https://github.com/anrieff/libcpuid", 0);
	NWL_NodeAttrSet(node, "PCI ID", "https://pci-ids.ucw.cz/", 0);
	NWL_NodeAttrSet(node, "USB ID", "http://www.linux-usb.org/usb-ids.html", 0);
	NWL_NodeAttrSet(node, "PNP ID", "https://uefi.org/pnp_id_list", 0);
	return node;
}
