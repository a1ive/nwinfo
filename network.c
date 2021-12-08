// SPDX-License-Identifier: Unlicense

#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <netioapi.h>
#include <stdio.h>
#include <stdlib.h>
#include "nwinfo.h"

static const char* bps_human_sizes[6] =
{ "bps", "Kbps", "Mbps", "Gbps", "Tbps", "Pbps", };

static void displayAddress(const PSOCKET_ADDRESS Address)
{
	if (Address->iSockaddrLength < sizeof(SOCKADDR_IN))
	{
		printf("INVALID\n");
	}
	else if (Address->lpSockaddr->sa_family == AF_INET)
	{
		SOCKADDR_IN* si = (SOCKADDR_IN*)(Address->lpSockaddr);
		char a[INET_ADDRSTRLEN] = { 0 };
		if (NT5InetNtop(AF_INET, &(si->sin_addr), a, sizeof(a)))
			printf("(IPv4) %s\n", a);
		else
			printf("(IPv4) NULL\n");
	}
	else if (Address->lpSockaddr->sa_family == AF_INET6)
	{
		SOCKADDR_IN6* si = (SOCKADDR_IN6*)(Address->lpSockaddr);
		char a[INET6_ADDRSTRLEN] = { 0 };
		if (NT5InetNtop(AF_INET6, &(si->sin6_addr), a, sizeof(a)))
			printf("(IPv6) %s\n", a);
		else
			printf("(IPv6) NULL\n");
	}
	else
		printf("NULL\n");
}

static const CHAR*
IfTypeToStr(IFTYPE Type) {
	switch (Type) {
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

void nwinfo_network (int active)
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

	dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, NULL, &outBufLen);

	if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
		pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
		if (!pAddresses) {
			printf ("Memory allocation failed.\n");
			return;
		}
	}
	else if (dwRetVal == ERROR_NO_DATA) {
		printf("No addresses were found.\n");
		return;
	}
	else {
		printf("Error: %d\n", dwRetVal);
		return;
	}

	dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &outBufLen);

	if (dwRetVal != NO_ERROR) {
		printf("Call to GetAdaptersAddresses failed with error: %d\n", dwRetVal);
		free(pAddresses);
		return;
	}

	dwRetVal = GetIfTable(NULL, &IfTableSize, FALSE);
	if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) {
		IfTable = (MIB_IFTABLE*)malloc(IfTableSize);
		if (IfTable)
			GetIfTable(IfTable, &IfTableSize, TRUE);
	}

	pCurrAddresses = pAddresses;
	while (pCurrAddresses) {
		if (active && pCurrAddresses->OperStatus != IfOperStatusUp)
			goto next_addr;
		printf("Network adapter: %s\n", pCurrAddresses->AdapterName);
		printf("Description: %s\n", NT5WcsToMbs(pCurrAddresses->Description));
		printf("Type: %s\n", IfTypeToStr(pCurrAddresses->IfType));
		if (pCurrAddresses->PhysicalAddressLength != 0) {
			printf("MAC address: ");
			for (i = 0; i < (int)pCurrAddresses->PhysicalAddressLength; i++) {
				if (i == (pCurrAddresses->PhysicalAddressLength - 1))
					printf("%.2X\n", pCurrAddresses->PhysicalAddress[i]);
				else
					printf("%.2X-", pCurrAddresses->PhysicalAddress[i]);
			}
		}

		if (pCurrAddresses->OperStatus == IfOperStatusUp)
			printf("Status: Active\n");
		else
			printf("Status: Deactive\n");
		printf("DHCP Enabled: %s\n", pCurrAddresses->Dhcpv4Enabled ? "YES" : "NO");
		pUnicast = pCurrAddresses->FirstUnicastAddress;
		if (pUnicast != NULL) {
			for (i = 0; pUnicast != NULL; i++) {
				printf("Unicast address %u:", i);
				displayAddress(&pUnicast->Address);
				if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
					ULONG SubnetMask = 0;
					NT5ConvertLengthToIpv4Mask(pUnicast->OnLinkPrefixLength, &SubnetMask);
					printf("Subnet Mask: %u.%u.%u.%u\n",
						SubnetMask & 0xFF, (SubnetMask >> 8) & 0xFF, (SubnetMask >> 16) & 0xFF, (SubnetMask >> 24) & 0xFF);
				}
				pUnicast = pUnicast->Next;
			}
		}
		else
			printf("No Unicast addresses\n");

		pAnycast = pCurrAddresses->FirstAnycastAddress;
		if (pAnycast) {
			for (i = 0; pAnycast != NULL; i++) {
				printf("Anycast address %u:", i);
				displayAddress(&pAnycast->Address);
				pAnycast = pAnycast->Next;
			}
		}
		else
			printf("No Anycast addresses\n");

		pMulticast = pCurrAddresses->FirstMulticastAddress;
		if (pMulticast) {
			for (i = 0; pMulticast != NULL; i++) {
				printf("Multicast address %u:", i);
				displayAddress(&pMulticast->Address);
				pMulticast = pMulticast->Next;
			}
		}
		else
			printf("No Multicast addresses\n");

		pGateway = pCurrAddresses->FirstGatewayAddress;
		if (pGateway != NULL) {
			for (i = 0; pGateway != NULL; i++) {
				printf("Gateway address %u:", i);
				displayAddress(&pGateway->Address);
				pGateway = pGateway->Next;
			}
		}
		else
			printf("No Gateway addresses\n");

		pDnServer = pCurrAddresses->FirstDnsServerAddress;
		if (pDnServer) {
			for (i = 0; pDnServer != NULL; i++) {
				printf("DNS Server address %u:", i);
				displayAddress(&pDnServer->Address);
				pDnServer = pDnServer->Next;
			}
		}
		else
			printf("No DNS Server addresses\n");

		if (pCurrAddresses->Dhcpv4Enabled && pCurrAddresses->Dhcpv4Server.iSockaddrLength >= sizeof(SOCKADDR_IN)) {
			printf("DHCP Server:");
			displayAddress(&pCurrAddresses->Dhcpv4Server);
		}

		printf("Transmit link speed: %s\n", GetHumanSize(pCurrAddresses->TransmitLinkSpeed, bps_human_sizes, 1000));
		printf("Receive link speed: %s\n", GetHumanSize(pCurrAddresses->ReceiveLinkSpeed, bps_human_sizes, 1000));
		printf("MTU: %lu Byte\n", pCurrAddresses->Mtu);
		if (IfTable && pCurrAddresses->IfIndex > 0) {
			ULONG idx = pCurrAddresses->IfIndex - 1;
			printf("Received: %u Octets\n", IfTable->table[idx].dwInOctets);
			printf("Sent: %u Octets\n", IfTable->table[idx].dwOutOctets);
		}
		printf("\n");
next_addr:
		pCurrAddresses = pCurrAddresses->Next;
	}

	free(pAddresses);
	if (IfTable)
		free(IfTable);
}