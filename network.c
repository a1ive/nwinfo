// SPDX-License-Identifier: Unlicense

#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <netioapi.h>
#include <stdio.h>
#include <stdlib.h>
#include "nwinfo.h"

static PNODE node;

static const char* bps_human_sizes[6] =
{ "bps", "Kbps", "Mbps", "Gbps", "Tbps", "Pbps", };

static void displayAddress(PNODE pNode, const PSOCKET_ADDRESS Address, LPCSTR key)
{
	if (Address->iSockaddrLength < sizeof(SOCKADDR_IN))
	{
		//printf("INVALID\n");
	}
	else if (Address->lpSockaddr->sa_family == AF_INET)
	{
		SOCKADDR_IN* si = (SOCKADDR_IN*)(Address->lpSockaddr);
		char a[INET_ADDRSTRLEN] = { 0 };
		if (NT5InetNtop(AF_INET, &(si->sin_addr), a, sizeof(a)))
			node_att_set(pNode, key ? key : "IPv4", a, 0);
		else
			node_att_set(pNode, key ? key : "IPv4", "", 0);
	}
	else if (Address->lpSockaddr->sa_family == AF_INET6)
	{
		SOCKADDR_IN6* si = (SOCKADDR_IN6*)(Address->lpSockaddr);
		char a[INET6_ADDRSTRLEN] = { 0 };
		if (NT5InetNtop(AF_INET6, &(si->sin6_addr), a, sizeof(a)))
			node_att_set(pNode, key ? key : "IPv6", a, 0);
		else
			node_att_set(pNode, key ? key : "IPv6", "", 0);
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

PNODE nwinfo_network (int active)
{
	DWORD dwRetVal = 0;
	unsigned int i = 0;
	// Set the flags to pass to GetAdaptersAddresses
	ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS;
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	ULONG outBufLen = 0;
	PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
	PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
	PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
	PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
	IP_ADAPTER_DNS_SERVER_ADDRESS* pDnServer = NULL;
	PIP_ADAPTER_GATEWAY_ADDRESS pGateway = NULL;
	MIB_IFTABLE *IfTable = NULL;
	ULONG IfTableSize = 0;

	node = node_alloc("Network", NFLG_TABLE);

	dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, NULL, &outBufLen);

	if (dwRetVal == ERROR_BUFFER_OVERFLOW)
	{
		pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
		if (!pAddresses)
		{
			fprintf (stderr, "Memory allocation failed.\n");
			return node;
		}
	}
	else if (dwRetVal == ERROR_NO_DATA)
	{
		fprintf(stderr, "No addresses were found.\n");
		return node;
	}
	else
	{
		fprintf(stderr, "Error: %d\n", dwRetVal);
		return node;
	}

	dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &outBufLen);

	if (dwRetVal != NO_ERROR)
	{
		fprintf(stderr, "Call to GetAdaptersAddresses failed with error: %d\n", dwRetVal);
		free(pAddresses);
		return node;
	}

	dwRetVal = GetIfTable(NULL, &IfTableSize, FALSE);
	if (dwRetVal == ERROR_INSUFFICIENT_BUFFER)
	{
		IfTable = (MIB_IFTABLE*)malloc(IfTableSize);
		if (IfTable)
			GetIfTable(IfTable, &IfTableSize, TRUE);
	}

	pCurrAddresses = pAddresses;
	while (pCurrAddresses)
	{
		PNODE nic = node_append_new(node, "Interface", NFLG_TABLE_ROW);
		if (active && pCurrAddresses->OperStatus != IfOperStatusUp)
			goto next_addr;
		node_att_set(nic, "Network adapter", pCurrAddresses->AdapterName, 0);
		node_att_set(nic, "Description", NT5WcsToMbs(pCurrAddresses->Description), 0);
		node_att_set(nic, "Type", IfTypeToStr(pCurrAddresses->IfType), 0);
		if (pCurrAddresses->PhysicalAddressLength != 0)
		{
			nwinfo_buffer[0] = '\0';
			for (i = 0; i < pCurrAddresses->PhysicalAddressLength; i++)
			{
				snprintf(nwinfo_buffer, NWINFO_BUFSZ, "%s%.2X%s", nwinfo_buffer,
					pCurrAddresses->PhysicalAddress[i], (i == (pCurrAddresses->PhysicalAddressLength - 1)) ? "" : "-");
			}
			node_att_set(nic, "MAC address", nwinfo_buffer, 0);
		}

		node_att_set(nic, "Status", (pCurrAddresses->OperStatus == IfOperStatusUp) ? "Active" : "Deactive", 0);
		node_att_set_bool(nic, "DHCP Enabled", pCurrAddresses->Dhcpv4Enabled, 0);
		pUnicast = pCurrAddresses->FirstUnicastAddress;
		if (pUnicast != NULL)
		{
			PNODE n_unicast = node_append_new(nic, "Unicasts", NFLG_TABLE);
			for (i = 0; pUnicast != NULL; i++)
			{
				PNODE unicast = node_append_new(n_unicast, "Unicast address", NFLG_TABLE_ROW);
				displayAddress(unicast, &pUnicast->Address, NULL);
				if (pUnicast->Address.lpSockaddr->sa_family == AF_INET)
				{
					ULONG SubnetMask = 0;
					NT5ConvertLengthToIpv4Mask(pUnicast->OnLinkPrefixLength, &SubnetMask);
					node_setf(unicast, "Subnet Mask", 0, "%u.%u.%u.%u",
						SubnetMask & 0xFF, (SubnetMask >> 8) & 0xFF, (SubnetMask >> 16) & 0xFF, (SubnetMask >> 24) & 0xFF);
				}
				pUnicast = pUnicast->Next;
			}
		}

		pAnycast = pCurrAddresses->FirstAnycastAddress;
		if (pAnycast)
		{
			PNODE n_anycast = node_append_new(nic, "Anycasts", NFLG_TABLE);
			for (i = 0; pAnycast != NULL; i++)
			{
				PNODE anycast = node_append_new(n_anycast, "Anycast address", NFLG_TABLE_ROW);
				displayAddress(anycast, &pAnycast->Address, NULL);
				pAnycast = pAnycast->Next;
			}
		}

		pMulticast = pCurrAddresses->FirstMulticastAddress;
		if (pMulticast)
		{
			PNODE n_multicast = node_append_new(nic, "Multicasts", NFLG_TABLE);
			for (i = 0; pMulticast != NULL; i++)
			{
				PNODE multicast = node_append_new(n_multicast, "Multicast address", NFLG_TABLE_ROW);
				displayAddress(multicast, &pMulticast->Address, NULL);
				pMulticast = pMulticast->Next;
			}
		}

		pGateway = pCurrAddresses->FirstGatewayAddress;
		if (pGateway != NULL)
		{
			PNODE n_gateway = node_append_new(nic, "Gateways", NFLG_TABLE);
			for (i = 0; pGateway != NULL; i++)
			{
				PNODE gateway = node_append_new(n_gateway, "Gateway address", NFLG_TABLE_ROW);
				displayAddress(gateway, &pGateway->Address, NULL);
				pGateway = pGateway->Next;
			}
		}

		pDnServer = pCurrAddresses->FirstDnsServerAddress;
		if (pDnServer)
		{
			PNODE n_dns = node_append_new(nic, "DNS Servers", NFLG_TABLE);
			for (i = 0; pDnServer != NULL; i++)
			{
				PNODE dns = node_append_new(n_dns, "DNS Server", NFLG_TABLE_ROW);
				displayAddress(dns, &pDnServer->Address, NULL);
				pDnServer = pDnServer->Next;
			}
		}

		if (pCurrAddresses->Dhcpv4Enabled && pCurrAddresses->Dhcpv4Server.iSockaddrLength >= sizeof(SOCKADDR_IN))
		{
			displayAddress(nic, &pCurrAddresses->Dhcpv4Server, "DHCP Server");
		}

		node_att_set(nic, "Transmit link speed", GetHumanSize(pCurrAddresses->TransmitLinkSpeed, bps_human_sizes, 1000), 0);
		node_att_set(nic, "Receive link speed", GetHumanSize(pCurrAddresses->ReceiveLinkSpeed, bps_human_sizes, 1000), 0);
		node_setf(nic, "MTU", 0, "%lu Byte", pCurrAddresses->Mtu);
		if (IfTable && pCurrAddresses->IfIndex > 0)
		{
			ULONG idx = pCurrAddresses->IfIndex - 1;
			node_setf(nic, "Received", 0, "%u Octets", IfTable->table[idx].dwInOctets);
			node_setf(nic, "Sent", 0, "%u Octets", IfTable->table[idx].dwOutOctets);
		}
next_addr:
		pCurrAddresses = pCurrAddresses->Next;
	}

	free(pAddresses);
	if (IfTable)
		free(IfTable);
	return node;
}
