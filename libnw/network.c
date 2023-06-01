// SPDX-License-Identifier: Unlicense

#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <netioapi.h>

#include "libnw.h"
#include "utils.h"

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

static const char* bps_human_sizes[6] =
{ "bps", "Kbps", "Mbps", "Gbps", "Tbps", "Pbps", };

static void displayAddress(PNODE pNode, const PSOCKET_ADDRESS Address, LPCSTR key)
{
	if (!Address || !Address->lpSockaddr
		|| Address->iSockaddrLength < sizeof(SOCKADDR_IN)
		|| Address->iSockaddrLength > sizeof(SOCKADDR_IN6))
	{
		//printf("INVALID\n");
	}
	else if (Address->lpSockaddr->sa_family == AF_INET)
	{
		SOCKADDR_IN* si = (SOCKADDR_IN*)(Address->lpSockaddr);
		char a[INET_ADDRSTRLEN] = { 0 };
		if (inet_ntop(AF_INET, &(si->sin_addr), a, sizeof(a)))
			NWL_NodeAttrSet(pNode, key ? key : "IPv4", a, NAFLG_FMT_IPADDR);
		else
			NWL_NodeAttrSet(pNode, key ? key : "IPv4", "", NAFLG_FMT_IPADDR);
	}
	else if (Address->lpSockaddr->sa_family == AF_INET6)
	{
		SOCKADDR_IN6* si = (SOCKADDR_IN6*)(Address->lpSockaddr);
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

static PNODE CreateNode(PNODE parent, LPCSTR name)
{
	PNODE_LINK link;
	for (link = &parent->Children[0]; link->LinkedNode != NULL; link++)
	{
		if (strcmp(link->LinkedNode->Name, name) == 0)
		{
			NWL_NodeFree(link->LinkedNode, 1);
			link->LinkedNode = NWL_NodeAlloc(name, NFLG_TABLE);
			return link->LinkedNode;
		}
	}
	return NWL_NodeAppendNew(parent, name, NFLG_TABLE);
}

static PNODE CreateIf(PNODE parent, LPCSTR guid)
{
	PNODE node = NULL;
	PNODE_LINK link;
	for (link = &parent->Children[0]; link->LinkedNode != NULL; link++)
	{
		if (strcmp(link->LinkedNode->Name, "Interface") == 0)
		{
			LPCSTR name = NWL_NodeAttrGet(link->LinkedNode, "Network Adapter");
			if (name && strcmp(name, guid) == 0)
				return link->LinkedNode;
		}
	}
	node = NWL_NodeAppendNew(parent, "Interface", NFLG_TABLE_ROW);
	NWL_NodeAttrSet(node, "Network Adapter", guid, NAFLG_FMT_GUID);
	return node;
}

PNODE NW_UpdateNetwork(PNODE node)
{
	DWORD dwRetVal = 0;
	unsigned int i = 0;
	// Set the flags to pass to GetAdaptersAddresses
	ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS;
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	ULONG outBufLen = 0;
	ULONG Iterations = 0;
	PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
	PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
	PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
	PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
	IP_ADAPTER_DNS_SERVER_ADDRESS* pDnServer = NULL;
	PIP_ADAPTER_GATEWAY_ADDRESS pGateway = NULL;
	MIB_IFTABLE *IfTable = NULL;
	ULONG IfTableSize = 0;
	PVOID pMaxAddress = NULL;

	// Allocate a 15 KB buffer to start with.
	outBufLen = WORKING_BUFFER_SIZE;
	do
	{
		pAddresses = (IP_ADAPTER_ADDRESSES*)MALLOC(outBufLen);
		if (!pAddresses)
		{
			NWL_NodeAppendMultiSz(&NWLC->ErrLog, "Memory allocation failed in "__FUNCTION__);
			return node;
		}

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
		return node;
	}

	dwRetVal = GetIfTable(NULL, &IfTableSize, FALSE);
	if (dwRetVal == ERROR_INSUFFICIENT_BUFFER)
	{
		IfTable = (MIB_IFTABLE*)MALLOC(IfTableSize);
		if (IfTable)
			GetIfTable(IfTable, &IfTableSize, TRUE);
	}

	pCurrAddresses = pAddresses;
	pMaxAddress = ((PUCHAR)pAddresses) + outBufLen;
	while (pCurrAddresses && pCurrAddresses < (PIP_ADAPTER_ADDRESSES)pMaxAddress)
	{
		PNODE nic = NULL;
		if (NWLC->ActiveNet && pCurrAddresses->OperStatus != IfOperStatusUp)
			goto next_addr;
		nic = CreateIf(node, pCurrAddresses->AdapterName);
		NWL_NodeAttrSet(nic, "Description", NWL_WcsToMbs(pCurrAddresses->Description), 0);
		NWL_NodeAttrSet(nic, "Type", IfTypeToStr(pCurrAddresses->IfType), 0);
		if (pCurrAddresses->PhysicalAddressLength != 0)
		{
			NWLC->NwBuf[0] = '\0';
			for (i = 0; i < pCurrAddresses->PhysicalAddressLength; i++)
			{
				snprintf(NWLC->NwBuf, NWINFO_BUFSZ, "%s%.2X%s", NWLC->NwBuf,
					pCurrAddresses->PhysicalAddress[i], (i == (pCurrAddresses->PhysicalAddressLength - 1)) ? "" : "-");
			}
			NWL_NodeAttrSet(nic, "MAC Address", NWLC->NwBuf, 0);
		}

		NWL_NodeAttrSet(nic, "Status", (pCurrAddresses->OperStatus == IfOperStatusUp) ? "Active" : "Deactive", 0);
		NWL_NodeAttrSetBool(nic, "DHCP Enabled", pCurrAddresses->Dhcpv4Enabled, 0);
		pUnicast = pCurrAddresses->FirstUnicastAddress;
		if (pUnicast != NULL)
		{
			PNODE n_unicast = CreateNode(nic, "Unicasts");
			for (i = 0; pUnicast != NULL && pUnicast < (PIP_ADAPTER_UNICAST_ADDRESS)pMaxAddress; i++)
			{
				PNODE unicast = NWL_NodeAppendNew(n_unicast, "Unicast Address", NFLG_TABLE_ROW);
				displayAddress(unicast, &pUnicast->Address, NULL);
				if (pUnicast->Address.lpSockaddr->sa_family == AF_INET)
				{
					ULONG SubnetMask = 0;
					NWL_ConvertLengthToIpv4Mask(pUnicast->OnLinkPrefixLength, &SubnetMask);
					NWL_NodeAttrSetf(unicast, "Subnet Mask", NAFLG_FMT_IPADDR, "%u.%u.%u.%u",
						SubnetMask & 0xFF, (SubnetMask >> 8) & 0xFF, (SubnetMask >> 16) & 0xFF, (SubnetMask >> 24) & 0xFF);
				}
				pUnicast = pUnicast->Next;
			}
		}

		pAnycast = pCurrAddresses->FirstAnycastAddress;
		if (pAnycast)
		{
			PNODE n_anycast = CreateNode(nic, "Anycasts");
			for (i = 0; pAnycast != NULL && pAnycast < (PIP_ADAPTER_ANYCAST_ADDRESS)pMaxAddress; i++)
			{
				PNODE anycast = NWL_NodeAppendNew(n_anycast, "Anycast Address", NFLG_TABLE_ROW);
				displayAddress(anycast, &pAnycast->Address, NULL);
				pAnycast = pAnycast->Next;
			}
		}

		pMulticast = pCurrAddresses->FirstMulticastAddress;
		if (pMulticast)
		{
			PNODE n_multicast = CreateNode(nic, "Multicasts");
			for (i = 0; pMulticast != NULL && pMulticast < (PIP_ADAPTER_MULTICAST_ADDRESS)pMaxAddress; i++)
			{
				PNODE multicast = NWL_NodeAppendNew(n_multicast, "Multicast Address", NFLG_TABLE_ROW);
				displayAddress(multicast, &pMulticast->Address, NULL);
				pMulticast = pMulticast->Next;
			}
		}

		pGateway = pCurrAddresses->FirstGatewayAddress;
		if (pGateway != NULL)
		{
			PNODE n_gateway = CreateNode(nic, "Gateways");
			for (i = 0; pGateway != NULL && pGateway < (PIP_ADAPTER_GATEWAY_ADDRESS)pMaxAddress; i++)
			{
				PNODE gateway = NWL_NodeAppendNew(n_gateway, "Gateway", NFLG_TABLE_ROW);
				displayAddress(gateway, &pGateway->Address, NULL);
				pGateway = pGateway->Next;
			}
		}

		pDnServer = pCurrAddresses->FirstDnsServerAddress;
		if (pDnServer)
		{
			PNODE n_dns = CreateNode(nic, "DNS Servers");
			for (i = 0; pDnServer != NULL && pDnServer < (IP_ADAPTER_DNS_SERVER_ADDRESS*)pMaxAddress; i++)
			{
				PNODE dns = NWL_NodeAppendNew(n_dns, "DNS Server", NFLG_TABLE_ROW);
				displayAddress(dns, &pDnServer->Address, NULL);
				pDnServer = pDnServer->Next;
			}
		}

		if (pCurrAddresses->Dhcpv4Enabled && pCurrAddresses->Dhcpv4Server.iSockaddrLength >= sizeof(SOCKADDR_IN))
		{
			displayAddress(nic, &pCurrAddresses->Dhcpv4Server, "DHCP Server");
		}

		NWL_NodeAttrSet(nic, "Transmit Link Speed",
			NWL_GetHumanSize(pCurrAddresses->TransmitLinkSpeed, bps_human_sizes, 1000), NAFLG_FMT_HUMAN_SIZE);
		NWL_NodeAttrSet(nic, "Receive Link Speed",
			NWL_GetHumanSize(pCurrAddresses->ReceiveLinkSpeed, bps_human_sizes, 1000), NAFLG_FMT_HUMAN_SIZE);
		NWL_NodeAttrSetf(nic, "MTU (Byte)", NAFLG_FMT_NUMERIC, "%lu", pCurrAddresses->Mtu);
		if (IfTable && pCurrAddresses->IfIndex > 0 && pCurrAddresses->IfIndex <= IfTable->dwNumEntries)
		{
			IF_INDEX idx = pCurrAddresses->IfIndex - 1;
			NWL_NodeAttrSetf(nic, "Received (Octets)", NAFLG_FMT_NUMERIC, "%lu", IfTable->table[idx].dwInOctets);
			NWL_NodeAttrSetf(nic, "Sent (Octets)", NAFLG_FMT_NUMERIC, "%lu", IfTable->table[idx].dwOutOctets);
		}
next_addr:
		pCurrAddresses = pCurrAddresses->Next;
	}

	FREE(pAddresses);
	if (IfTable)
		FREE(IfTable);
	return node;
}

PNODE NW_Network(VOID)
{
	PNODE node = NWL_NodeAlloc("Network", NFLG_TABLE);
	if (NWLC->NetInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, node);
	return NW_UpdateNetwork(node);
}
