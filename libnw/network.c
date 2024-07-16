// SPDX-License-Identifier: Unlicense

#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <netioapi.h>
#include <wlanapi.h>

#include "libnw.h"
#include "utils.h"

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

static const char* bps_human_sizes[6] =
{ "bps", "kbps", "Mbps", "Gbps", "Tbps", "Pbps", };

static void DisplayAddress(PNODE pNode, const PSOCKET_ADDRESS pAddress, LPCSTR key)
{
	if (!pAddress || !pAddress->lpSockaddr
		|| pAddress->iSockaddrLength < sizeof(SOCKADDR_IN)
		|| pAddress->iSockaddrLength > sizeof(SOCKADDR_IN6))
	{
		//printf("INVALID\n");
	}
	else if (pAddress->lpSockaddr->sa_family == AF_INET)
	{
		SOCKADDR_IN* si = (SOCKADDR_IN*)(pAddress->lpSockaddr);
		char a[INET_ADDRSTRLEN] = { 0 };
		if (inet_ntop(AF_INET, &(si->sin_addr), a, sizeof(a)))
			NWL_NodeAttrSet(pNode, key ? key : "IPv4", a, NAFLG_FMT_IPADDR);
		else
			NWL_NodeAttrSet(pNode, key ? key : "IPv4", "", NAFLG_FMT_IPADDR);
	}
	else if (pAddress->lpSockaddr->sa_family == AF_INET6)
	{
		SOCKADDR_IN6* si = (SOCKADDR_IN6*)(pAddress->lpSockaddr);
		char a[INET6_ADDRSTRLEN] = { 0 };
		if (inet_ntop(AF_INET6, &(si->sin6_addr), a, sizeof(a)))
			NWL_NodeAttrSet(pNode, key ? key : "IPv6", a, NAFLG_FMT_IPADDR);
		else
			NWL_NodeAttrSet(pNode, key ? key : "IPv6", "", NAFLG_FMT_IPADDR);
	}
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
	case DOT11_AUTH_ALGO_RSNA: return "RSNA";
	case DOT11_AUTH_ALGO_RSNA_PSK: return "RSNA PSK";
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
	if (NWLC->NetInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	bLonghornOrLater = (NWLC->NwOsInfo.dwMajorVersion >= 6);

	pAddresses = GetXpAdaptersAddresses(&pMaxAddress);
	if (!pAddresses)
		return node;

	pCurrAddresses = pAddresses;
	while (pCurrAddresses && pCurrAddresses < (PIP_ADAPTER_ADDRESSES_XP)pMaxAddress)
	{
		PNODE nic = NULL;
		LPCSTR desc = NULL;
		if (NWLC->ActiveNet && pCurrAddresses->OperStatus != IfOperStatusUp)
			goto next_addr;
		desc = NWL_Ucs2ToUtf8(pCurrAddresses->Description);
		if (NWLC->SkipVirtualNet)
		{
			if (pCurrAddresses->IfType == IF_TYPE_SOFTWARE_LOOPBACK ||
				pCurrAddresses->IfType == IF_TYPE_TUNNEL)
				goto next_addr;
			if (strncmp(desc, "Microsoft Wi-Fi Direct Virtual Adapter", 38) == 0)
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
				PNODE unicast = NWL_NodeAppendNew(n_unicast, "Unicast Address", NFLG_TABLE_ROW);
				DisplayAddress(unicast, &pUnicast->Address, NULL);
				if (bLonghornOrLater && pUnicast->Address.lpSockaddr->sa_family == AF_INET)
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
				PNODE anycast = NWL_NodeAppendNew(n_anycast, "Anycast Address", NFLG_TABLE_ROW);
				DisplayAddress(anycast, &pAnycast->Address, NULL);
				pAnycast = pAnycast->Next;
			}
		}

		pMulticast = pCurrAddresses->FirstMulticastAddress;
		if (pMulticast)
		{
			PNODE n_multicast = NWL_NodeAppendNew(nic, "Multicasts", NFLG_TABLE);
			for (i = 0; pMulticast != NULL && pMulticast < (PIP_ADAPTER_MULTICAST_ADDRESS_XP)pMaxAddress; i++)
			{
				PNODE multicast = NWL_NodeAppendNew(n_multicast, "Multicast Address", NFLG_TABLE_ROW);
				DisplayAddress(multicast, &pMulticast->Address, NULL);
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
					PNODE gateway = NWL_NodeAppendNew(n_gateway, "Gateway", NFLG_TABLE_ROW);
					DisplayAddress(gateway, &pGateway->Address, NULL);
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
				PNODE dns = NWL_NodeAppendNew(n_dns, "DNS Server", NFLG_TABLE_ROW);
				DisplayAddress(dns, &pDnServer->Address, NULL);
				pDnServer = pDnServer->Next;
			}
		}

		if (bLonghornOrLater &&
			pCurrAddressesLH->Dhcpv4Enabled && pCurrAddressesLH->Dhcpv4Server.iSockaddrLength >= sizeof(SOCKADDR_IN))
		{
			DisplayAddress(nic, &pCurrAddressesLH->Dhcpv4Server, "DHCP Server");
		}

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
			NWL_NodeAttrSetf(nic, "Sent (Octets)", NAFLG_FMT_NUMERIC, "%lu", ifRow.dwOutOctets);
		}
next_addr:
		pCurrAddresses = pCurrAddresses->Next;
	}

	FREE(pAddresses);
	return node;
}
