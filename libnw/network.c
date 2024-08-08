// SPDX-License-Identifier: Unlicense

#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <netioapi.h>
#include <wlanapi.h>
#include <pdhmsg.h>

#include "libnw.h"
#include "utils.h"

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

static const char* bps_human_sizes[6] =
{ "bps", "kbps", "Mbps", "Gbps", "Tbps", "Pbps", };

static UINT64 total_recv = 0, total_send = 0;

VOID
NWL_GetNetTraffic(NWLIB_NET_TRAFFIC* info, BOOL bit)
{
	const char* bit_units[6] = { "b", "kb", "Mb", "Gb", "Tb", "Pb", };
	static UINT64 old_recv = 0;
	static UINT64 old_send = 0;
	UINT64 diff_recv = 0;
	UINT64 diff_send = 0;
	if (total_recv >= old_recv)
		diff_recv = total_recv - old_recv;
	if (total_send >= old_send)
		diff_send = total_send - old_send;
	old_recv = total_recv;
	old_send = total_send;

	if (NWLC->PdhNetRecv)
		diff_recv = NWL_GetPdhSum(NWLC->PdhNetRecv, PDH_FMT_LARGE, NULL).largeValue;
	if (NWLC->PdhNetSend)
		diff_send = NWL_GetPdhSum(NWLC->PdhNetSend, PDH_FMT_LARGE, NULL).largeValue;
	if (bit)
	{
		memcpy(info->StrRecv, NWL_GetHumanSize(diff_recv * 8, bit_units, 1000), NWL_STR_SIZE);
		memcpy(info->StrSend, NWL_GetHumanSize(diff_send * 8, bit_units, 1000), NWL_STR_SIZE);
	}
	else
	{
		memcpy(info->StrRecv, NWL_GetHumanSize(diff_recv, NWLC->NwUnits, 1024), NWL_STR_SIZE);
		memcpy(info->StrSend, NWL_GetHumanSize(diff_send, NWLC->NwUnits, 1024), NWL_STR_SIZE);
	}
}

static const CHAR*
IpAddrToStr(const PSOCKET_ADDRESS pAddress, PBOOL pbIpv6)
{
	static CHAR buf[INET6_ADDRSTRLEN] = { 0 };
	if (!pAddress || !pAddress->lpSockaddr
		|| pAddress->iSockaddrLength < sizeof(SOCKADDR_IN)
		|| pAddress->iSockaddrLength > sizeof(SOCKADDR_IN6))
		return NULL;
	if (pAddress->lpSockaddr->sa_family == AF_INET && (NWLC->NetFlags & NW_NET_IPV4))
	{
		SOCKADDR_IN* si = (SOCKADDR_IN*)(pAddress->lpSockaddr);
		*pbIpv6 = FALSE;
		if (inet_ntop(AF_INET, &(si->sin_addr), buf, sizeof(buf)))
			return buf;
	}
	else if (pAddress->lpSockaddr->sa_family == AF_INET6 && (NWLC->NetFlags & NW_NET_IPV6))
	{
		SOCKADDR_IN6* si = (SOCKADDR_IN6*)(pAddress->lpSockaddr);
		*pbIpv6 = TRUE;
		if (inet_ntop(AF_INET6, &(si->sin6_addr), buf, sizeof(buf)))
			return buf;
	}
	return NULL;
}

static PNODE DisplayAddress(PNODE pParent, LPCSTR name, const PSOCKET_ADDRESS pAddress)
{
	BOOL bIpv6 = FALSE;
	const CHAR* buf = IpAddrToStr(pAddress, &bIpv6);
	if (buf == NULL)
		return NULL;
	PNODE pNode = NWL_NodeAppendNew(pParent, name, NFLG_TABLE_ROW);
	NWL_NodeAttrSet(pNode, bIpv6 ? "IPv6" : "IPv4", buf, NAFLG_FMT_IPADDR);
	return pNode;
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
GetWlanInfo(PNODE node, PIP_ADAPTER_ADDRESSES_XP ipAdapter)
{
	DWORD dwBuf;
	HANDLE hClient = NULL;
	GUID guidIf;
	WLAN_CONNECTION_ATTRIBUTES* wlanAttr = NULL;

	DWORD (WINAPI *OsWlanOpenHandle)(DWORD, PVOID, PDWORD, PHANDLE);
	DWORD (WINAPI *OsWlanQueryInterface)(HANDLE, const GUID*, WLAN_INTF_OPCODE, PVOID, PDWORD, PVOID*, PWLAN_OPCODE_VALUE_TYPE);
	void (WINAPI *OsWlanFreeMemory)(PVOID);
	DWORD (WINAPI *OsWlanCloseHandle)(HANDLE, PVOID);
	HMODULE hL = LoadLibraryW(L"wlanapi.dll");
	if (!hL)
		return;
	*(FARPROC*)&OsWlanOpenHandle = GetProcAddress(hL, "WlanOpenHandle");
	if (!OsWlanOpenHandle)
		return;
	*(FARPROC*)&OsWlanQueryInterface = GetProcAddress(hL, "WlanQueryInterface");
	if (!OsWlanQueryInterface)
		return;
	*(FARPROC*)&OsWlanFreeMemory = GetProcAddress(hL, "WlanFreeMemory");
	if (!OsWlanFreeMemory)
		return;
	*(FARPROC*)&OsWlanCloseHandle = GetProcAddress(hL, "WlanCloseHandle");
	if (!OsWlanCloseHandle)
		return;

	if (NWL_StrToGuid(ipAdapter->AdapterName, &guidIf) != TRUE)
		return;

	if (OsWlanOpenHandle(2, NULL, &dwBuf, &hClient) != ERROR_SUCCESS)
		return;

	if (OsWlanQueryInterface(hClient, &guidIf,
		wlan_intf_opcode_current_connection, NULL, &dwBuf, (PVOID*)&wlanAttr, NULL) != ERROR_SUCCESS)
		goto end;

	NWL_NodeAttrSet(node, "WLAN State", WlanStateToStr(wlanAttr->isState), 0);
	if (wlanAttr->isState != wlan_interface_state_connected)
		goto end;
	NWL_NodeAttrSet(node, "WLAN Profile", NWL_Ucs2ToUtf8(wlanAttr->strProfileName), 0);
	NWL_NodeAttrSetf(node, "WLAN Signal Quality", NAFLG_FMT_NUMERIC,
		"%lu", wlanAttr->wlanAssociationAttributes.wlanSignalQuality);
	NWL_NodeAttrSet(node, "WLAN Auth", WlanAuthToStr(wlanAttr->wlanSecurityAttributes.dot11AuthAlgorithm), 0);
	NWL_NodeAttrSet(node, "WLAN Cipher", WlanCipherToStr(wlanAttr->wlanSecurityAttributes.dot11CipherAlgorithm), 0);

end:
	if (wlanAttr)
		OsWlanFreeMemory(wlanAttr);
	OsWlanCloseHandle(hClient, NULL);
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

PNODE NW_Network(VOID)
{
	unsigned int i = 0;
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
	BOOL bLonghornOrLater;
	PNODE node = NWL_NodeAlloc("Network", NFLG_TABLE);

	total_recv = 0;
	total_send = 0;

	if (NWLC->NetInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	if (!(NWLC->NetFlags & (NW_NET_IPV4 | NW_NET_IPV6)))
		NWLC->NetFlags |= NW_NET_IPV4 | NW_NET_IPV6;

	bLonghornOrLater = (NWLC->NwOsInfo.dwMajorVersion >= 6);

	pAddresses = GetXpAdaptersAddresses(&pMaxAddress);
	if (!pAddresses)
		return node;

	pCurrAddresses = pAddresses;
	while (pCurrAddresses && pCurrAddresses < (PIP_ADAPTER_ADDRESSES_XP)pMaxAddress)
	{
		PNODE nic = NULL;
		LPCSTR desc = NULL;
		BOOL bMatch = FALSE;
		if (NWLC->NetGuid && _stricmp(NWLC->NetGuid, pCurrAddresses->AdapterName) != 0)
			goto next_addr;
		if ((NWLC->NetFlags & NW_NET_ACTIVE) && pCurrAddresses->OperStatus != IfOperStatusUp)
			goto next_addr;
		if (!(NWLC->NetFlags & (NW_NET_ETH | NW_NET_WLAN)))
			bMatch = TRUE;
		if ((NWLC->NetFlags & NW_NET_ETH) && pCurrAddresses->IfType == IF_TYPE_ETHERNET_CSMACD)
			bMatch = TRUE;
		if ((NWLC->NetFlags & NW_NET_WLAN) && pCurrAddresses->IfType == IF_TYPE_IEEE80211)
			bMatch = TRUE;
		if (bMatch == FALSE)
			goto next_addr;
		desc = NWL_Ucs2ToUtf8(pCurrAddresses->Description);
		if (NWLC->NetFlags & NW_NET_PHYS)
		{
			if (pCurrAddresses->IfType == IF_TYPE_SOFTWARE_LOOPBACK ||
				pCurrAddresses->IfType == IF_TYPE_TUNNEL)
				goto next_addr;
			if (strncmp(desc, "Microsoft Wi-Fi Direct Virtual Adapter", 38) == 0)
				goto next_addr;
			if (strncmp(desc, "Bluetooth Device", 16) == 0)
				goto next_addr;
			if (strncmp(desc, "VMware Virtual Ethernet Adapter", 31) == 0)
				goto next_addr;
			if (strncmp(desc, "VirtualBox Host-Only Ethernet Adapter", 37) == 0)
				goto next_addr;
		}
		pCurrAddressesLH = (PIP_ADAPTER_ADDRESSES_LH)pCurrAddresses;
		nic = NWL_NodeAppendNew(node, "Interface", NFLG_TABLE_ROW);
		NWL_NodeAttrSet(nic, "Network Adapter", pCurrAddresses->AdapterName, NAFLG_FMT_GUID);
		NWL_NodeAttrSet(nic, "Description", desc, 0);
		NWL_NodeAttrSet(nic, "Type", IfTypeToStr(pCurrAddresses->IfType), 0);
		if (pCurrAddresses->IfType == IF_TYPE_IEEE80211)
			GetWlanInfo(nic, pCurrAddresses);
		if (pCurrAddresses->PhysicalAddressLength != 0)
		{
			NWLC->NwBuf[0] = '\0';
			for (i = 0; i < pCurrAddresses->PhysicalAddressLength; i++)
			{
				snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s%.2X%s", NWLC->NwBuf,
					pCurrAddresses->PhysicalAddress[i], (i == (pCurrAddresses->PhysicalAddressLength - 1)) ? "" : "-");
			}
			NWL_NodeAttrSet(nic, "MAC Address", NWLC->NwBuf, NAFLG_FMT_SENSITIVE);
		}

		NWL_NodeAttrSet(nic, "Status", (pCurrAddresses->OperStatus == IfOperStatusUp) ? "Active" : "Deactive", 0);
		if (bLonghornOrLater)
			NWL_NodeAttrSetBool(nic, "DHCP Enabled", pCurrAddressesLH->Dhcpv4Enabled, 0);
		pUnicast = pCurrAddresses->FirstUnicastAddress;
		if (pUnicast != NULL)
		{
			PNODE n_unicast = NWL_NodeAppendNew(nic, "Unicasts", NFLG_TABLE);
			for (i = 0; pUnicast != NULL && pUnicast < (PIP_ADAPTER_UNICAST_ADDRESS_XP)pMaxAddress; i++)
			{
				PNODE unicast = DisplayAddress(n_unicast, "Unicast Address", &pUnicast->Address);
				if (unicast && bLonghornOrLater && pUnicast->Address.lpSockaddr->sa_family == AF_INET)
				{
					ULONG SubnetMask = 0;
					PIP_ADAPTER_UNICAST_ADDRESS_LH pUnicastLH = (PIP_ADAPTER_UNICAST_ADDRESS_LH)pUnicast;
					NWL_ConvertLengthToIpv4Mask(pUnicastLH->OnLinkPrefixLength, &SubnetMask);
					NWL_NodeAttrSetf(unicast, "Subnet Mask", NAFLG_FMT_IPADDR, "%u.%u.%u.%u",
						SubnetMask & 0xFF, (SubnetMask >> 8) & 0xFF, (SubnetMask >> 16) & 0xFF, (SubnetMask >> 24) & 0xFF);
				}
				pUnicast = pUnicast->Next;
			}
		}

		pAnycast = pCurrAddresses->FirstAnycastAddress;
		if (pAnycast)
		{
			PNODE n_anycast = NWL_NodeAppendNew(nic, "Anycasts", NFLG_TABLE);
			for (i = 0; pAnycast != NULL && pAnycast < (PIP_ADAPTER_ANYCAST_ADDRESS_XP)pMaxAddress; i++)
			{
				DisplayAddress(n_anycast, "Anycast Address", &pAnycast->Address);
				pAnycast = pAnycast->Next;
			}
		}

		pMulticast = pCurrAddresses->FirstMulticastAddress;
		if (pMulticast)
		{
			PNODE n_multicast = NWL_NodeAppendNew(nic, "Multicasts", NFLG_TABLE);
			for (i = 0; pMulticast != NULL && pMulticast < (PIP_ADAPTER_MULTICAST_ADDRESS_XP)pMaxAddress; i++)
			{
				DisplayAddress(n_multicast, "Multicast Address", &pMulticast->Address);
				pMulticast = pMulticast->Next;
			}
		}

		if (bLonghornOrLater)
		{
			pGateway = pCurrAddressesLH->FirstGatewayAddress;
			if (pGateway != NULL)
			{
				PNODE n_gateway = NWL_NodeAppendNew(nic, "Gateways", NFLG_TABLE);
				for (i = 0; pGateway != NULL && pGateway < (PIP_ADAPTER_GATEWAY_ADDRESS_LH)pMaxAddress; i++)
				{
					DisplayAddress(n_gateway, "Gateway", &pGateway->Address);
					pGateway = pGateway->Next;
				}
			}
		}

		pDnServer = pCurrAddresses->FirstDnsServerAddress;
		if (pDnServer)
		{
			PNODE n_dns = NWL_NodeAppendNew(nic, "DNS Servers", NFLG_TABLE);
			for (i = 0; pDnServer != NULL && pDnServer < (IP_ADAPTER_DNS_SERVER_ADDRESS*)pMaxAddress; i++)
			{
				DisplayAddress(n_dns, "DNS Server", &pDnServer->Address);
				pDnServer = pDnServer->Next;
			}
		}

		if (bLonghornOrLater && pCurrAddressesLH->Dhcpv4Enabled)
		{
			BOOL bIpv6; // unused
			const CHAR* buf = IpAddrToStr(&pCurrAddressesLH->Dhcpv4Server, &bIpv6);
			if (buf)
				NWL_NodeAttrSet(nic, "DHCP Server", buf, NAFLG_FMT_IPADDR);
		}
#if 0
		if (bLonghornOrLater && pCurrAddressesLH->Ipv6Enabled)
		{
			// This structure member is not currently supported and is reserved for future use.
			BOOL bIpv6; // unused
			const CHAR* buf = IpAddrToStr(&pCurrAddressesLH->Dhcpv6Server, &bIpv6);
			if (buf)
				NWL_NodeAttrSet(nic, "DHCPv6 Server", buf, NAFLG_FMT_IPADDR);
		}
#endif
		if (bLonghornOrLater)
		{
			NWL_NodeAttrSet(nic, "Transmit Link Speed",
				NWL_GetHumanSize(pCurrAddressesLH->TransmitLinkSpeed, bps_human_sizes, 1000), NAFLG_FMT_HUMAN_SIZE);
			NWL_NodeAttrSet(nic, "Receive Link Speed",
				NWL_GetHumanSize(pCurrAddressesLH->ReceiveLinkSpeed, bps_human_sizes, 1000), NAFLG_FMT_HUMAN_SIZE);
		}
		NWL_NodeAttrSetf(nic, "MTU (Byte)", NAFLG_FMT_NUMERIC, "%lu", pCurrAddresses->Mtu);

		ZeroMemory(&ifRow, sizeof(ifRow));
		ifRow.dwIndex = pCurrAddresses->IfIndex;
		if (GetIfEntry(&ifRow) == NO_ERROR)
		{
			NWL_NodeAttrSetf(nic, "Received (Octets)", NAFLG_FMT_NUMERIC, "%lu", ifRow.dwInOctets);
			total_recv += ifRow.dwInOctets;
			NWL_NodeAttrSetf(nic, "Sent (Octets)", NAFLG_FMT_NUMERIC, "%lu", ifRow.dwOutOctets);
			total_send += ifRow.dwOutOctets;
		}
next_addr:
		pCurrAddresses = pCurrAddresses->Next;
	}

	FREE(pAddresses);
	return node;
}
