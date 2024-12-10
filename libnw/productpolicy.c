// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "libnw.h"
#include "utils.h"


#pragma pack(1)
typedef struct _PRODUCT_POLICY_HEADER
{
	DWORD dwSize; // total size, including this header
	DWORD dwArraySize; // size of values array that follows this header
	DWORD dwEndSize; // size of end marker that follows the values array
	DWORD unused;
	DWORD dwVersion; // must be 1
} PRODUCT_POLICY_HEADER, * PPRODUCT_POLICY_HEADER;

typedef struct _PRODUCT_POLICY_VALUE
{
	UINT16 wSize; // total size, including this header
	UINT16 wNameSize; // size of name
	// type of data
	// 1 - REG_SZ, 2- REG_EXPAND_SZ, 3 - REG_BINARY, 4 - REG_BINARY
	UINT16 wType;
	UINT16 wDataSize; // size of data
	DWORD dwFlags;
	DWORD unused;
} PRODUCT_POLICY_VALUE, * PPRODUCT_POLICY_VALUE;

#define PRODUCT_POLICY_END_MARK 0x45ul // dword
#pragma pack()

static void
PrintProductPolicyEntry(PNODE node, PPRODUCT_POLICY_VALUE ppValue)
{
	CHAR hex[] = "0123456789ABCDEF";
	PNODE pp;
	PUINT8 pValueData = (PUINT8)ppValue + sizeof(PRODUCT_POLICY_VALUE) + ppValue->wNameSize;
	LPCSTR pName;

	wcsncpy_s(NWLC->NwBufW, NWINFO_BUFSZW,
		(WCHAR*)((PUINT8)ppValue + sizeof(PRODUCT_POLICY_VALUE)), ppValue->wNameSize / sizeof(WCHAR));
	pName = NWL_Ucs2ToUtf8(NWLC->NwBufW);
	if (NWLC->ProductPolicy && _stricmp(pName, NWLC->ProductPolicy) != 0)
		return;
	pp = NWL_NodeAppendNew(node, "Entry", NFLG_TABLE_ROW);
	NWL_NodeAttrSet(pp, "Name", pName, 0);
	//NWL_NodeAttrSetf(pp, "Flags", NAFLG_FMT_NUMERIC, "%lu", ppValue->dwFlags);

	switch (ppValue->wType)
	{
		case REG_SZ:
		case REG_EXPAND_SZ:
			NWL_NodeAttrSet(pp, "Type", "String", 0);
			NWL_NodeAttrSet(pp, "Data", NWL_Ucs2ToUtf8((LPCWSTR)pValueData), 0);
			break;
		case REG_DWORD:
			NWL_NodeAttrSet(pp, "Type", "DWORD", 0);
			NWL_NodeAttrSetf(pp, "Data", NAFLG_FMT_NUMERIC, "%lu", *(PDWORD)pValueData);
			break;
		case REG_QWORD:
			NWL_NodeAttrSet(pp, "Type", "QWORD", 0);
			NWL_NodeAttrSetf(pp, "Data", NAFLG_FMT_NUMERIC, "%llu", *(PUINT64)pValueData);
			break;
		default:
		{
			CHAR* tmp = calloc(ppValue->wDataSize, 3);
			for (UINT i = 0; i < ppValue->wDataSize; i++)
			{
				tmp[i * 3] = hex[(pValueData[i] & 0xF0) >> 4];
				tmp[i * 3 + 1] = hex[pValueData[i] & 0x0F];
				if (i < ppValue->wDataSize - 1U)
					tmp[i * 3 + 2] = ' ';
			}
			NWL_NodeAttrSet(pp, "Type", "Binary", 0);
			NWL_NodeAttrSet(pp, "Data", tmp, 0);
			free(tmp);
		}
	}
}

PNODE NW_ProductPolicy(VOID)
{
	CHAR hex[] = "0123456789ABCDEF";
	PNODE node = NWL_NodeAlloc("Product Policy", NFLG_TABLE);
	if (NWLC->ProductPolicyInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	DWORD dwType;
	DWORD ppSize;
	PPRODUCT_POLICY_HEADER ppHeader;
	PPRODUCT_POLICY_VALUE ppValue;
	PUINT8 ppData = NWL_NtGetRegValue(HKEY_LOCAL_MACHINE,
		L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
		L"ProductPolicy", &ppSize, &dwType);
	if (ppData == NULL || dwType != REG_BINARY || ppSize <= sizeof(PRODUCT_POLICY_HEADER))
		goto end;
	ppHeader = (PPRODUCT_POLICY_HEADER)ppData;
	if (ppSize != ppHeader->dwSize || ppHeader->dwVersion != 1)
		goto end;
	if (ppHeader->dwArraySize + ppHeader->dwEndSize + sizeof(PRODUCT_POLICY_HEADER) > ppSize)
		goto end;
	for (ppValue = (PPRODUCT_POLICY_VALUE)(ppData + sizeof(PRODUCT_POLICY_HEADER));
		(PUINT8)ppValue < ppData + sizeof(PRODUCT_POLICY_HEADER) + ppHeader->dwArraySize;
		ppValue = (PPRODUCT_POLICY_VALUE)((PUINT8)ppValue + ppValue->wSize))
	{
		PrintProductPolicyEntry(node, ppValue);
	}
end:
	free(ppData);
	return node;
}
