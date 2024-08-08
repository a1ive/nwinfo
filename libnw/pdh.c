// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include <pdhmsg.h>

VOID
NWL_PdhFini(VOID)
{
	if (NWLC->Pdh)
		NWLC->PdhCloseQuery(NWLC->Pdh);
	NWLC->Pdh = NULL;
	if (NWLC->PdhDll)
		FreeLibrary(NWLC->PdhDll);
	NWLC->PdhDll = NULL;
}

VOID
NWL_PdhInit(VOID)
{
	if (!NWLC->EnablePdh)
		return;
	NWLC->PdhDll = LoadLibraryW(L"Pdh.dll");
	if (NWLC->PdhDll)
	{
		*(FARPROC*)&NWLC->PdhOpenQueryW = GetProcAddress(NWLC->PdhDll, "PdhOpenQueryW");
		*(FARPROC*)&NWLC->PdhAddCounterW = GetProcAddress(NWLC->PdhDll, "PdhAddCounterW");
		*(FARPROC*)&NWLC->PdhCollectQueryData = GetProcAddress(NWLC->PdhDll, "PdhCollectQueryData");
		*(FARPROC*)&NWLC->PdhGetFormattedCounterValue = GetProcAddress(NWLC->PdhDll, "PdhGetFormattedCounterValue");
		*(FARPROC*)&NWLC->PdhGetFormattedCounterArrayW = GetProcAddress(NWLC->PdhDll, "PdhGetFormattedCounterArrayW");
		*(FARPROC*)&NWLC->PdhCloseQuery = GetProcAddress(NWLC->PdhDll, "PdhCloseQuery");
	}
	if (!NWLC->PdhOpenQueryW || !NWLC->PdhAddCounterW || !NWLC->PdhCollectQueryData
		|| !NWLC->PdhGetFormattedCounterValue || !NWLC->PdhGetFormattedCounterArrayW || !NWLC->PdhCloseQuery)
		goto fail;
	if (NWLC->PdhOpenQueryW(NULL, 0, &NWLC->Pdh) != ERROR_SUCCESS)
		goto fail;

	LPCWSTR cpu = L"\\Processor Information(_Total)\\% Processor Time";
	if (NWLC->NwOsInfo.dwMajorVersion >= 10)
		cpu = L"\\Processor Information(_Total)\\% Processor Utility";
	if (NWLC->PdhAddCounterW(NWLC->Pdh, cpu, 0, &NWLC->PdhCpuUsage) != ERROR_SUCCESS)
		NWLC->PdhCpuUsage = NULL;
	if (NWLC->PdhAddCounterW(NWLC->Pdh, L"\\Processor Information(_Total)\\Processor Frequency", 0, &NWLC->PdhCpuBaseFreq) != ERROR_SUCCESS)
		NWLC->PdhCpuBaseFreq = NULL;
	if (NWLC->PdhAddCounterW(NWLC->Pdh, L"\\Processor Information(_Total)\\% Processor Performance", 0, &NWLC->PdhCpuFreq) != ERROR_SUCCESS)
		NWLC->PdhCpuFreq = NULL;
	if (NWLC->PdhAddCounterW(NWLC->Pdh, L"\\Network Interface(*)\\Bytes Sent/sec", 0, &NWLC->PdhNetSend) != ERROR_SUCCESS)
		NWLC->PdhNetSend = NULL;
	if (NWLC->PdhAddCounterW(NWLC->Pdh, L"\\Network Interface(*)\\Bytes Received/sec", 0, &NWLC->PdhNetRecv) != ERROR_SUCCESS)
		NWLC->PdhNetRecv = NULL;
	if (NWLC->PdhAddCounterW(NWLC->Pdh, L"\\GPU Engine(*)\\Utilization Percentage", 0, &NWLC->PdhGpuUsage) != ERROR_SUCCESS)
		NWLC->PdhGpuUsage = NULL;
	if (NWLC->PdhAddCounterW(NWLC->Pdh, L"\\GPU Adapter Memory(*)\\Dedicated Usage", 0, &NWLC->PdhGpuCurMem) != ERROR_SUCCESS)
		NWLC->PdhGpuCurMem = NULL;
	NWLC->PdhCollectQueryData(NWLC->Pdh);
	return;
fail:
	NWL_PdhFini();
}

VOID
NWL_PdhUpdate(VOID)
{
	if (!NWLC->Pdh)
		return;
	if (NWLC->PdhCollectQueryData(NWLC->Pdh) != ERROR_SUCCESS)
		NWL_PdhFini();
}

static inline BOOL
CompareWcsSuffix(LPCWSTR str, LPCWSTR suffix)
{
	size_t strLen = wcslen(str);
	size_t suffixLen = wcslen(suffix);
	if (suffixLen > strLen)
		return FALSE;
	return (wcscmp(&str[strLen - suffixLen], suffix) == 0);
}

PDH_FMT_COUNTERVALUE
NWL_GetPdhSum(PDH_HCOUNTER counter, DWORD fmt, LPCWSTR suffix)
{
	PDH_STATUS status = ERROR_SUCCESS;
	DWORD dwBufferSize = 0;
	DWORD dwItemCount = 0;
	PDH_FMT_COUNTERVALUE_ITEM* pItems = NULL;
	PDH_FMT_COUNTERVALUE ret = { 0 };

	status = NWLC->PdhGetFormattedCounterArrayW(counter, fmt, &dwBufferSize, &dwItemCount, pItems);
	if (status != PDH_MORE_DATA)
		return ret;
	pItems = malloc(dwBufferSize);
	if (!pItems)
		return ret;
	status = NWLC->PdhGetFormattedCounterArrayW(counter, fmt, &dwBufferSize, &dwItemCount, pItems);
	if (status != ERROR_SUCCESS)
		goto out;
	for (DWORD i = 0; i < dwItemCount; i++)
	{
		if (!suffix || CompareWcsSuffix(pItems[i].szName, suffix))
		{
			switch (fmt)
			{
			case PDH_FMT_DOUBLE:
				ret.doubleValue += pItems[i].FmtValue.doubleValue;
				break;
			case PDH_FMT_LARGE:
				ret.largeValue += pItems[i].FmtValue.largeValue;
				break;
			case PDH_FMT_LONG:
				ret.longValue += pItems[i].FmtValue.longValue;
				break;
			}
		}
	}

out:
	if (pItems)
		free(pItems);
	return ret;
}
