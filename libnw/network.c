// SPDX-License-Identifier: Unlicense

#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <netioapi.h>
#include <wlanapi.h>

#include "libnw.h"
#include "utils.h"
#include "network.h"
#include "stb_ds.h"

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

const char* NWL_BPS_UNITS[6] =
{ "bps", "kbps", "Mbps", "Gbps", "Tbps", "Pbps", };
static const char* BIT_UINTS[6] =
{ "b", "kb", "Mb", "Gb", "Tb", "Pb", };

static BOOL
IpAddrToStrBuf(const PSOCKET_ADDRESS pAddress, CHAR* out, size_t out_len, PBOOL pbIpv6)
{
	if (!out || out_len == 0)
		return FALSE;
	out[0] = '\0';
	if (!pAddress || !pAddress->lpSockaddr
		|| pAddress->iSockaddrLength < sizeof(SOCKADDR_IN)
		|| pAddress->iSockaddrLength > sizeof(SOCKADDR_IN6))
		return FALSE;

	if (pAddress->lpSockaddr->sa_family == AF_INET)
	{
		SOCKADDR_IN* si = (SOCKADDR_IN*)(pAddress->lpSockaddr);
		if (!(NWLC->NetFlags & NW_NET_IPV4))
			return FALSE;
		if (pbIpv6)
			*pbIpv6 = FALSE;
		return inet_ntop(AF_INET, &(si->sin_addr), out, out_len) != NULL;
	}
	if (pAddress->lpSockaddr->sa_family == AF_INET6)
	{
		SOCKADDR_IN6* si = (SOCKADDR_IN6*)(pAddress->lpSockaddr);
		if (!(NWLC->NetFlags & NW_NET_IPV6))
			return FALSE;
		if (pbIpv6)
			*pbIpv6 = TRUE;
		return inet_ntop(AF_INET6, &(si->sin6_addr), out, out_len) != NULL;
	}
	return FALSE;
}

static void
NetAddrAppend(NWLIB_NET_ADDRESS** list, const PSOCKET_ADDRESS address, ULONG prefix, BOOL hasPrefix)
{
	BOOL ipv6 = FALSE;
	CHAR buf[NWL_NET_ADDR_STRLEN] = { 0 };
	NWLIB_NET_ADDRESS entry;

	if (!list || !address)
		return;

	if (!IpAddrToStrBuf(address, buf, sizeof(buf), &ipv6))
		return;

	ZeroMemory(&entry, sizeof(entry));
	snprintf(entry.Address, sizeof(entry.Address), "%s", buf);
	entry.Ipv6 = ipv6;
	entry.HasPrefix = hasPrefix;
	entry.PrefixLength = prefix;

	arrput(*list, entry);
}

static void
FormatMacAddress(const BYTE* addr, ULONG len, CHAR* out, size_t out_len)
{
	size_t pos = 0;

	if (!out || out_len == 0)
		return;
	out[0] = '\0';
	if (!addr || len == 0)
		return;

	for (ULONG i = 0; i < len; i++)
	{
		int written = snprintf(out + pos, out_len - pos, "%.2X%s",
			addr[i], (i + 1 == len) ? "" : "-");
		if (written <= 0)
			break;
		if ((size_t)written >= out_len - pos)
		{
			out[out_len - 1] = '\0';
			break;
		}
		pos += (size_t)written;
	}
}

static DWORD
NetRateFromDiff(DWORD current, DWORD previous, ULONG64 curTicks, ULONG64 prevTicks)
{
	ULONG64 diffBytes;
	ULONG64 diffTicks;
	double rate;

	if (curTicks <= prevTicks)
		return 0;
	if (current < previous)
		return 0;

	diffBytes = (ULONG64)current - (ULONG64)previous;
	diffTicks = curTicks - prevTicks;
	if (diffTicks == 0)
		return 0;

	rate = (double)diffBytes * 1000.0 / (double)diffTicks;
	if (rate <= 0.0)
		return 0;
	if (rate > (double)MAXDWORD)
		return MAXDWORD;
	return (DWORD)(rate + 0.5);
}

static void
NetAdapterFreeArrays(NWLIB_NET_ADAPTER* adapter)
{
	if (!adapter)
		return;
	arrfree(adapter->Unicasts);
	arrfree(adapter->Anycasts);
	arrfree(adapter->Multicasts);
	arrfree(adapter->Gateways);
	arrfree(adapter->DNSServers);
	adapter->Unicasts = NULL;
	adapter->Anycasts = NULL;
	adapter->Multicasts = NULL;
	adapter->Gateways = NULL;
	adapter->DNSServers = NULL;
}

static const CHAR*
IfTypeToStr(IFTYPE Type)
{
	switch (Type)
	{
	case IF_TYPE_ETHERNET_CSMACD: return "Ethernet";
	case IF_TYPE_ISO88025_TOKENRING: return "Token Ring";
	case IF_TYPE_PPP: return "PPP";
	case IF_TYPE_SOFTWARE_LOOPBACK: return "Software Loopback";
	case IF_TYPE_ATM: return "ATM";
	case IF_TYPE_IEEE80211: return "IEEE 802.11 Wireless";
	case IF_TYPE_TUNNEL: return "Tunnel";
	case IF_TYPE_IEEE1394: return "IEEE 1394 High Performance Serial Bus";
	case IF_TYPE_IEEE80216_WMAN: return "WiMax";
	case IF_TYPE_WWANPP: return "GSM";
	case IF_TYPE_WWANPP2: return "CDMA";
	}
	return "Other";
}

static const CHAR*
WlanStateToStr(WLAN_INTERFACE_STATE state)
{
	switch (state)
	{
	case wlan_interface_state_not_ready: return "Not Ready";
	case wlan_interface_state_connected: return "Connected";
	case wlan_interface_state_ad_hoc_network_formed: return "AS Hoc Network Formed";
	case wlan_interface_state_disconnecting: return "Disconnecting";
	case wlan_interface_state_disconnected: return "Disconnected";
	case wlan_interface_state_associating: return "Associating";
	case wlan_interface_state_discovering: return "Discovering";
	case wlan_interface_state_authenticating: return "Authenticating";
	}
	return "Unknown";
}

static const CHAR*
WlanAuthToStr(DOT11_AUTH_ALGORITHM auth)
{
	switch (auth)
	{
	case DOT11_AUTH_ALGO_80211_OPEN: return "Open System";
	case DOT11_AUTH_ALGO_80211_SHARED_KEY: return "Shared Key";
	case DOT11_AUTH_ALGO_WPA: return "WPA";
	case DOT11_AUTH_ALGO_WPA_PSK: return "WPA PSK";
	case DOT11_AUTH_ALGO_WPA_NONE: return "WPA NONE";
	case DOT11_AUTH_ALGO_RSNA: return "WPA2";
	case DOT11_AUTH_ALGO_RSNA_PSK: return "WPA2 PSK";
	case DOT11_AUTH_ALGO_WPA3_ENT_192: return "WPA3 ENT 192"; // DOT11_AUTH_ALGO_WPA3
	case DOT11_AUTH_ALGO_WPA3_SAE: return "WPA3 SAE";
	case DOT11_AUTH_ALGO_OWE: return "OWE";
	case DOT11_AUTH_ALGO_WPA3_ENT: return "WPA3 ENT";
	}
	return "Unknown";
}

static const CHAR*
WlanCipherToStr(DOT11_CIPHER_ALGORITHM cipher)
{
	switch (cipher)
	{
	case DOT11_CIPHER_ALGO_NONE: return "None";
	case DOT11_CIPHER_ALGO_WEP40: return "WEP 40";
	case DOT11_CIPHER_ALGO_TKIP: return "TKIP";
	case DOT11_CIPHER_ALGO_CCMP: return "CCMP";
	case DOT11_CIPHER_ALGO_WEP104: return "WEP 104";
	}
	return "Unknown";
}

static void
GetWlanInfo(NWLIB_NET_ADAPTER* adapter, PIP_ADAPTER_ADDRESSES_XP ipAdapter)
{
	DWORD dwBuf;
	HANDLE hClient = NULL;
	GUID guidIf;
	WLAN_CONNECTION_ATTRIBUTES* wlanAttr = NULL;

	DWORD (WINAPI *OsWlanOpenHandle)(DWORD, PVOID, PDWORD, PHANDLE) = NULL;
	DWORD (WINAPI *OsWlanQueryInterface)(HANDLE, const GUID*, WLAN_INTF_OPCODE, PVOID, PDWORD, PVOID*, PWLAN_OPCODE_VALUE_TYPE) = NULL;
	void (WINAPI *OsWlanFreeMemory)(PVOID) = NULL;
	DWORD (WINAPI *OsWlanCloseHandle)(HANDLE, PVOID) = NULL;
	HMODULE hL = LoadLibraryW(L"wlanapi.dll");
	if (!hL)
		return;

	*(FARPROC*)&OsWlanOpenHandle = GetProcAddress(hL, "WlanOpenHandle");
	*(FARPROC*)&OsWlanQueryInterface = GetProcAddress(hL, "WlanQueryInterface");
	*(FARPROC*)&OsWlanFreeMemory = GetProcAddress(hL, "WlanFreeMemory");
	*(FARPROC*)&OsWlanCloseHandle = GetProcAddress(hL, "WlanCloseHandle");
	if (!OsWlanOpenHandle || !OsWlanQueryInterface || !OsWlanFreeMemory || !OsWlanCloseHandle)
		goto end;

	if (NWL_StrToGuid(ipAdapter->AdapterName, &guidIf) != TRUE)
		goto end;

	if (OsWlanOpenHandle(2, NULL, &dwBuf, &hClient) != ERROR_SUCCESS)
		goto end;

	if (OsWlanQueryInterface(hClient, &guidIf,
		wlan_intf_opcode_current_connection, NULL, &dwBuf, (PVOID*)&wlanAttr, NULL) != ERROR_SUCCESS)
		goto end;

	adapter->WLANState = wlanAttr->isState;
	if (adapter->WLANState == wlan_interface_state_connected)
	{
		snprintf(adapter->WLANProfile, sizeof(adapter->WLANProfile), "%s", NWL_Ucs2ToUtf8(wlanAttr->strProfileName));
		adapter->WLANSignalQuality = wlanAttr->wlanAssociationAttributes.wlanSignalQuality;
		adapter->WLANAuth = wlanAttr->wlanSecurityAttributes.dot11AuthAlgorithm;
		adapter->WLANCipher = wlanAttr->wlanSecurityAttributes.dot11CipherAlgorithm;
	}

end:
	if (wlanAttr && OsWlanFreeMemory)
		OsWlanFreeMemory(wlanAttr);
	if (hClient && OsWlanCloseHandle)
		OsWlanCloseHandle(hClient, NULL);
	if (hL)
		FreeLibrary(hL);
}

static PIP_ADAPTER_ADDRESSES_XP
GetXpAdaptersAddresses(PVOID* pEnd)
{
	ULONG outBufLen = 0;
	ULONG Iterations = 0;
	DWORD dwRetVal = 0;
	ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS;
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;

	// Allocate a 15 KB buffer to start with.
	outBufLen = WORKING_BUFFER_SIZE;
	do
	{
		pAddresses = (PIP_ADAPTER_ADDRESSES)MALLOC(outBufLen);
		if (!pAddresses)
		{
			NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Memory allocation failed in "__FUNCTION__);
			return NULL;
		}
		ZeroMemory(pAddresses, outBufLen);
		dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &outBufLen);
		if (dwRetVal == ERROR_BUFFER_OVERFLOW)
		{
			FREE(pAddresses);
			pAddresses = NULL;
		}
		else
			break;
		Iterations++;
	} while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));

	if (dwRetVal != NO_ERROR)
	{
		FREE(pAddresses);
		return NULL;
	}
	*pEnd = ((PUCHAR)pAddresses) + outBufLen;
	return (PIP_ADAPTER_ADDRESSES_XP)pAddresses;
}

NWLIB_NET_ADAPTER_MAP*
NWL_GetNetAdapters(NWLIB_NET_ADAPTER_MAP* adapters)
{
	PIP_ADAPTER_ADDRESSES_XP pAddresses = NULL;
	PIP_ADAPTER_ADDRESSES_XP pCurrAddresses = NULL;
	PIP_ADAPTER_ADDRESSES_LH pCurrAddressesLH = NULL;
	PIP_ADAPTER_UNICAST_ADDRESS_XP pUnicast = NULL;
	PIP_ADAPTER_ANYCAST_ADDRESS_XP pAnycast = NULL;
	PIP_ADAPTER_MULTICAST_ADDRESS_XP pMulticast = NULL;
	PIP_ADAPTER_DNS_SERVER_ADDRESS_XP pDnServer = NULL;
	PIP_ADAPTER_GATEWAY_ADDRESS_LH pGateway = NULL;
	MIB_IFROW ifRow;
	PVOID pMaxAddress = NULL;
	BOOL bLonghornOrLater = (NWLC->NwOsInfo.dwMajorVersion >= 6);
	ULONG64 nowTicks = GetTickCount64();

	if (!(NWLC->NetFlags & (NW_NET_IPV4 | NW_NET_IPV6)))
		NWLC->NetFlags |= NW_NET_IPV4 | NW_NET_IPV6;

	pAddresses = GetXpAdaptersAddresses(&pMaxAddress);
	if (!pAddresses)
	{
		if (adapters)
		{
			ptrdiff_t count = shlen(adapters);
			for (ptrdiff_t i = 0; i < count; i++)
			{
				adapters[i].value.DiffReceived = 0;
				adapters[i].value.DiffSent = 0;
			}
		}
		return adapters;
	}

	pCurrAddresses = pAddresses;
	while (pCurrAddresses && pCurrAddresses < (PIP_ADAPTER_ADDRESSES_XP)pMaxAddress)
	{
		NWLIB_NET_ADAPTER adapter;
		NWLIB_NET_ADAPTER_MAP* prevEntry = NULL;
		const CHAR* desc = NULL;
		const CHAR* adapterName = NULL;
		BOOL hasStats = FALSE;
		DWORD prevReceived = 0;
		DWORD prevSent = 0;
		ULONG64 prevTicks = 0;

		adapterName = pCurrAddresses->AdapterName;
		if (!adapterName)
			goto next_addr;

		if (adapters)
			prevEntry = shgetp_null(adapters, (CHAR*)adapterName);
		if (prevEntry)
		{
			prevReceived = prevEntry->value.ReceivedOctets;
			prevSent = prevEntry->value.SentOctets;
			prevTicks = prevEntry->value.Ticks;
		}

		ZeroMemory(&adapter, sizeof(adapter));
		adapter.Ticks = nowTicks;
		adapter.IfType = pCurrAddresses->IfType;
		adapter.OperStatus = pCurrAddresses->OperStatus;
		desc = NWL_Ucs2ToUtf8(pCurrAddresses->Description);
		if (desc)
			snprintf(adapter.Description, sizeof(adapter.Description), "%s", desc);
		else
			adapter.Description[0] = '\0';

		pCurrAddressesLH = (PIP_ADAPTER_ADDRESSES_LH)pCurrAddresses;
		if (bLonghornOrLater)
			adapter.Dhcpv4Enabled = pCurrAddressesLH->Dhcpv4Enabled;

		if (pCurrAddresses->PhysicalAddressLength != 0)
			FormatMacAddress(pCurrAddresses->PhysicalAddress, pCurrAddresses->PhysicalAddressLength,
				adapter.MACAddress, sizeof(adapter.MACAddress));

		pUnicast = pCurrAddresses->FirstUnicastAddress;
		for (; pUnicast != NULL && pUnicast < (PIP_ADAPTER_UNICAST_ADDRESS_XP)pMaxAddress; pUnicast = pUnicast->Next)
		{
			ULONG prefix = 0;
			BOOL hasPrefix = FALSE;
			if (bLonghornOrLater && pUnicast->Address.lpSockaddr &&
				pUnicast->Address.lpSockaddr->sa_family == AF_INET)
			{
				PIP_ADAPTER_UNICAST_ADDRESS_LH pUnicastLH = (PIP_ADAPTER_UNICAST_ADDRESS_LH)pUnicast;
				prefix = pUnicastLH->OnLinkPrefixLength;
				hasPrefix = TRUE;
			}
			NetAddrAppend(&adapter.Unicasts, &pUnicast->Address, prefix, hasPrefix);
		}

		pAnycast = pCurrAddresses->FirstAnycastAddress;
		for (; pAnycast != NULL && pAnycast < (PIP_ADAPTER_ANYCAST_ADDRESS_XP)pMaxAddress; pAnycast = pAnycast->Next)
			NetAddrAppend(&adapter.Anycasts, &pAnycast->Address, 0, FALSE);

		pMulticast = pCurrAddresses->FirstMulticastAddress;
		for (; pMulticast != NULL && pMulticast < (PIP_ADAPTER_MULTICAST_ADDRESS_XP)pMaxAddress; pMulticast = pMulticast->Next)
			NetAddrAppend(&adapter.Multicasts, &pMulticast->Address, 0, FALSE);

		if (bLonghornOrLater)
		{
			pGateway = pCurrAddressesLH->FirstGatewayAddress;
			for (; pGateway != NULL && pGateway < (PIP_ADAPTER_GATEWAY_ADDRESS_LH)pMaxAddress; pGateway = pGateway->Next)
				NetAddrAppend(&adapter.Gateways, &pGateway->Address, 0, FALSE);
		}

		pDnServer = pCurrAddresses->FirstDnsServerAddress;
		for (; pDnServer != NULL && pDnServer < (IP_ADAPTER_DNS_SERVER_ADDRESS*)pMaxAddress; pDnServer = pDnServer->Next)
			NetAddrAppend(&adapter.DNSServers, &pDnServer->Address, 0, FALSE);

		if (bLonghornOrLater && pCurrAddressesLH->Dhcpv4Enabled)
		{
			BOOL bIpv6;
			CHAR dhcpBuf[NWL_NET_ADDR_STRLEN] = { 0 };
			if (IpAddrToStrBuf(&pCurrAddressesLH->Dhcpv4Server, dhcpBuf, sizeof(dhcpBuf), &bIpv6))
				snprintf(adapter.DHCPServer, sizeof(adapter.DHCPServer), "%s", dhcpBuf);
		}

		if (bLonghornOrLater)
		{
			adapter.TransmitLinkSpeed = pCurrAddressesLH->TransmitLinkSpeed;
			if (adapter.TransmitLinkSpeed == ~0ULL)
				adapter.TransmitLinkSpeed = 0;
			adapter.ReceiveLinkSpeed = pCurrAddressesLH->ReceiveLinkSpeed;
			if (adapter.ReceiveLinkSpeed == ~0ULL)
				adapter.ReceiveLinkSpeed = 0;
		}
		adapter.Mtu = pCurrAddresses->Mtu;

		ZeroMemory(&ifRow, sizeof(ifRow));
		ifRow.dwIndex = pCurrAddresses->IfIndex;
		if (GetIfEntry(&ifRow) == NO_ERROR)
		{
			adapter.ReceivedOctets = ifRow.dwInOctets;
			adapter.SentOctets = ifRow.dwOutOctets;
			hasStats = TRUE;
		}

		if (hasStats && prevEntry)
		{
			adapter.DiffReceived = NetRateFromDiff(adapter.ReceivedOctets, prevReceived,
				adapter.Ticks, prevTicks);
			adapter.DiffSent = NetRateFromDiff(adapter.SentOctets, prevSent,
				adapter.Ticks, prevTicks);
		}

		if (pCurrAddresses->IfType == IF_TYPE_IEEE80211)
			GetWlanInfo(&adapter, pCurrAddresses);

		if (prevEntry)
		{
			NetAdapterFreeArrays(&prevEntry->value);
			// Update value in-place. Using shputs() for an existing key can overwrite
			// the stored key pointer (stb_ds temp_key bookkeeping is not always set
			// on lookups), which later breaks freeing logic for adapter keys.
			prevEntry->value = adapter;
		}
		else
		{
			NWLIB_NET_ADAPTER_MAP item;
			size_t key_len = strlen(adapterName);
			item.key = (CHAR*)MALLOC(key_len + 1);
			if (!item.key)
			{
				NetAdapterFreeArrays(&adapter);
				goto next_addr;
			}
			memcpy(item.key, adapterName, key_len + 1);
			item.value = adapter;
			shputs(adapters, item);
		}

next_addr:
		pCurrAddresses = pCurrAddresses->Next;
	}

	FREE(pAddresses);

	// Remove adapters not seen in this refresh.
	if (adapters)
	{
		for (ptrdiff_t i = 0; i < shlen(adapters); )
		{
			if (adapters[i].value.Ticks != nowTicks)
			{
				CHAR* key = adapters[i].key;
				NetAdapterFreeArrays(&adapters[i].value);
				shdel(adapters, key);
				FREE(key);
				continue;
			}
			i++;
		}
	}
	return adapters;
}

ptrdiff_t
NWL_NetAdaptersCount(NWLIB_NET_ADAPTER_MAP* adapters)
{
	return shlen(adapters);
}

VOID
NWL_FreeNetAdapters(NWLIB_NET_ADAPTER_MAP* adapters)
{
	ptrdiff_t count = shlen(adapters);
	for (ptrdiff_t i = 0; i < count; i++)
	{
		NetAdapterFreeArrays(&adapters[i].value);
		if (adapters[i].key)
			FREE(adapters[i].key);
	}
	shfree(adapters);
}

VOID
NWL_GetNetTraffic(NWLIB_NET_TRAFFIC* info, BOOL bit, const NWLIB_NET_ADAPTER_MAP* adapters)
{
	double recv = 0.0;
	double send = 0.0;

	info->Recv = 0;
	info->Send = 0;

	if (adapters)
	{
		ptrdiff_t count = shlen((NWLIB_NET_ADAPTER_MAP*)adapters);
		for (ptrdiff_t i = 0; i < count; i++)
		{
			recv += adapters[i].value.DiffReceived;
			send += adapters[i].value.DiffSent;
		}
	}

	info->Recv = recv;
	info->Send = send;

	if (bit)
	{
		memcpy(info->StrRecv, NWL_GetHumanSize((UINT64)(info->Recv * 8.0), BIT_UINTS, 1000), NWL_STR_SIZE);
		memcpy(info->StrSend, NWL_GetHumanSize((UINT64)(info->Send * 8.0), BIT_UINTS, 1000), NWL_STR_SIZE);
	}
	else
	{
		memcpy(info->StrRecv, NWL_GetHumanSize((UINT64)info->Recv, NWLC->NwUnits, 1024), NWL_STR_SIZE);
		memcpy(info->StrSend, NWL_GetHumanSize((UINT64)info->Send, NWLC->NwUnits, 1024), NWL_STR_SIZE);
	}
}

static void
AppendAddressTable(PNODE parent, LPCSTR title, LPCSTR row, const NWLIB_NET_ADDRESS* addresses, BOOL withMask)
{
	if (!addresses || arrlen(addresses) == 0)
		return;

	PNODE table = NWL_NodeAppendNew(parent, title, NFLG_TABLE);
	for (ptrdiff_t i = 0; i < arrlen(addresses); i++)
	{
		const NWLIB_NET_ADDRESS* addr = &addresses[i];
		PNODE entry = NULL;
		ULONG mask = 0;

		if (!addr->Address[0])
			continue;

		entry = NWL_NodeAppendNew(table, row, NFLG_TABLE_ROW);
		NWL_NodeAttrSet(entry, addr->Ipv6 ? "IPv6" : "IPv4", addr->Address, NAFLG_FMT_IPADDR);
		if (withMask && addr->HasPrefix && !addr->Ipv6)
		{
			NWL_ConvertLengthToIpv4Mask(addr->PrefixLength, &mask);
			NWL_NodeAttrSetf(entry, "Subnet Mask", NAFLG_FMT_IPADDR, "%u.%u.%u.%u",
				mask & 0xFF, (mask >> 8) & 0xFF, (mask >> 16) & 0xFF, (mask >> 24) & 0xFF);
		}
	}
}

PNODE NW_Network(BOOL bAppend)
{
	NWLIB_NET_ADAPTER_MAP* adapters;
	PNODE node = NWL_NodeAlloc("Network", NFLG_TABLE);
	BOOL bLonghornOrLater = (NWLC->NwOsInfo.dwMajorVersion >= 6);

	if (bAppend)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	NWLC->NwNetAdapters = NWL_GetNetAdapters(NWLC->NwNetAdapters);
	adapters = NWLC->NwNetAdapters;
	if (!adapters)
		return node;

	ptrdiff_t count = NWL_NetAdaptersCount(adapters);
	for (ptrdiff_t i = 0; i < count; i++)
	{
		NWLIB_NET_ADAPTER* adapter = &adapters[i].value;
		BOOL bMatch = FALSE;

		if (NWLC->NetGuid && _stricmp(NWLC->NetGuid, adapters[i].key) != 0)
			continue;
		if ((NWLC->NetFlags & NW_NET_ACTIVE) && adapter->OperStatus != IfOperStatusUp)
			continue;
		if (!(NWLC->NetFlags & (NW_NET_ETH | NW_NET_WLAN)))
			bMatch = TRUE;
		if ((NWLC->NetFlags & NW_NET_ETH) && adapter->IfType == IF_TYPE_ETHERNET_CSMACD)
			bMatch = TRUE;
		if ((NWLC->NetFlags & NW_NET_WLAN) && adapter->IfType == IF_TYPE_IEEE80211)
			bMatch = TRUE;
		if (bMatch == FALSE)
			continue;

		if (NWLC->NetFlags & NW_NET_PHYS)
		{
			const CHAR* desc = adapter->Description;
			if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK ||
				adapter->IfType == IF_TYPE_TUNNEL)
				continue;
			if (desc && desc[0])
			{
				if (strncmp(desc, "Microsoft Wi-Fi Direct Virtual Adapter", 38) == 0)
					continue;
				if (strncmp(desc, "Bluetooth Device", 16) == 0)
					continue;
				if (strncmp(desc, "VMware Virtual Ethernet Adapter", 31) == 0)
					continue;
				if (strncmp(desc, "VirtualBox Host-Only Ethernet Adapter", 37) == 0)
					continue;
				if (strncmp(desc, "Hyper-V Virtual Ethernet Adapter", 32) == 0)
					continue;
			}
		}

		PNODE nic = NWL_NodeAppendNew(node, "Interface", NFLG_TABLE_ROW);
		NWL_NodeAttrSet(nic, "Network Adapter", adapters[i].key, NAFLG_FMT_GUID);
		if (adapter->Description[0])
			NWL_NodeAttrSet(nic, "Description", adapter->Description, 0);
		NWL_NodeAttrSet(nic, "Type", IfTypeToStr(adapter->IfType), 0);
		if (adapter->IfType == IF_TYPE_IEEE80211)
		{
			NWL_NodeAttrSet(nic, "WLAN State", WlanStateToStr(adapter->WLANState), 0);
			if (adapter->WLANState == wlan_interface_state_connected)
			{
				if (adapter->WLANProfile[0])
					NWL_NodeAttrSet(nic, "WLAN Profile", adapter->WLANProfile, 0);
				NWL_NodeAttrSetf(nic, "WLAN Signal Quality", NAFLG_FMT_NUMERIC, "%lu", adapter->WLANSignalQuality);
				NWL_NodeAttrSet(nic, "WLAN Auth", WlanAuthToStr(adapter->WLANAuth), 0);
				NWL_NodeAttrSet(nic, "WLAN Cipher", WlanCipherToStr(adapter->WLANCipher), 0);
			}
		}
		if (adapter->MACAddress[0])
			NWL_NodeAttrSet(nic, "MAC Address", adapter->MACAddress, NAFLG_FMT_SENSITIVE);

		NWL_NodeAttrSet(nic, "Status", (adapter->OperStatus == IfOperStatusUp) ? "Active" : "Deactive", 0);
		if (bLonghornOrLater)
			NWL_NodeAttrSetBool(nic, "DHCP Enabled", adapter->Dhcpv4Enabled, 0);

		AppendAddressTable(nic, "Unicasts", "Unicast Address", adapter->Unicasts, TRUE);
		AppendAddressTable(nic, "Anycasts", "Anycast Address", adapter->Anycasts, FALSE);
		AppendAddressTable(nic, "Multicasts", "Multicast Address", adapter->Multicasts, FALSE);
		AppendAddressTable(nic, "Gateways", "Gateway", adapter->Gateways, FALSE);
		AppendAddressTable(nic, "DNS Servers", "DNS Server", adapter->DNSServers, FALSE);

		if (bLonghornOrLater && adapter->Dhcpv4Enabled && adapter->DHCPServer[0])
			NWL_NodeAttrSet(nic, "DHCP Server", adapter->DHCPServer, NAFLG_FMT_IPADDR);

		if (bLonghornOrLater)
		{
			NWL_NodeAttrSet(nic, "Transmit Link Speed",
				NWL_GetHumanSize(adapter->TransmitLinkSpeed, NWL_BPS_UNITS, 1000), NAFLG_FMT_HUMAN_SIZE);
			NWL_NodeAttrSet(nic, "Receive Link Speed",
				NWL_GetHumanSize(adapter->ReceiveLinkSpeed, NWL_BPS_UNITS, 1000), NAFLG_FMT_HUMAN_SIZE);
		}
		NWL_NodeAttrSetf(nic, "MTU (Byte)", NAFLG_FMT_NUMERIC, "%lu", adapter->Mtu);
		NWL_NodeAttrSet(nic, "Received (Octets)",
			NWL_GetHumanSize(adapter->ReceivedOctets, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		NWL_NodeAttrSet(nic, "Sent (Octets)",
			NWL_GetHumanSize(adapter->SentOctets, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
	}

	return node;
}
