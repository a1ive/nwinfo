// SPDX-License-Identifier: Unlicense

#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <netioapi.h>
#include <stdio.h>
#include <stdlib.h>
#include "nwinfo.h"

// Link with Iphlpapi.lib
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

static const char* bps_human_sizes[6] =
{ "bps", "Kbps", "Mbps", "Gbps", "Tbps", "Pbps", };

static void displayAddress(const PSOCKET_ADDRESS Address)
{
	if (Address->lpSockaddr->sa_family == AF_INET)
	{
		SOCKADDR_IN* si = (SOCKADDR_IN*)(Address->lpSockaddr);
		char a[INET_ADDRSTRLEN] = { 0 };
		if (inet_ntop(AF_INET, &(si->sin_addr), a, sizeof(a)))
			printf("(IPv4) %s\n", a);
	}
	else if (Address->lpSockaddr->sa_family == AF_INET6)
	{
		SOCKADDR_IN6* si = (SOCKADDR_IN6*)(Address->lpSockaddr);
		char a[INET6_ADDRSTRLEN] = { 0 };
		if (inet_ntop(AF_INET6, &(si->sin6_addr), a, sizeof(a)))
			printf("(IPv6) %s\n", a);
	}
	else
		printf("NULL\n");
}

void nwinfo_network (int active)
{
	DWORD dwSize = 0;
	DWORD dwRetVal = 0;
	unsigned int i = 0;
	// Set the flags to pass to GetAdaptersAddresses
	ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS;
	char *lpMsgBuf = NULL;
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	ULONG outBufLen = 0;
	ULONG Iterations = 0;
	PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
	PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
	PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
	PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
	IP_ADAPTER_DNS_SERVER_ADDRESS* pDnServer = NULL;
	PIP_ADAPTER_GATEWAY_ADDRESS pGateway = NULL;

	// Allocate a 15 KB buffer to start with.
	outBufLen = WORKING_BUFFER_SIZE;

	do {

		pAddresses = (IP_ADAPTER_ADDRESSES*)MALLOC(outBufLen);
		if (pAddresses == NULL) {
			printf
			("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
			exit(1);
		}

		dwRetVal =
			GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &outBufLen);

		if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
			FREE(pAddresses);
			pAddresses = NULL;
		}
		else {
			break;
		}

		Iterations++;

	} while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));

	if (dwRetVal == NO_ERROR) {
		// If successful, output some information from the data we received
		pCurrAddresses = pAddresses;
		while (pCurrAddresses) {
			if (active && pCurrAddresses->OperStatus != IfOperStatusUp)
				goto next_addr;
			printf("Network adapter: %s\n", pCurrAddresses->AdapterName);
			printf("Description: %wS\n", pCurrAddresses->Description);

			if (pCurrAddresses->PhysicalAddressLength != 0) {
				printf("MAC address: ");
				for (i = 0; i < (int)pCurrAddresses->PhysicalAddressLength;
					i++) {
					if (i == (pCurrAddresses->PhysicalAddressLength - 1))
						printf("%.2X\n",
							(int)pCurrAddresses->PhysicalAddress[i]);
					else
						printf("%.2X-",
							(int)pCurrAddresses->PhysicalAddress[i]);
				}
			}

			if (pCurrAddresses->OperStatus == IfOperStatusUp)
				printf("Status: Active\n");
			else {
				printf("Status: Deactive\n");
			}

			pUnicast = pCurrAddresses->FirstUnicastAddress;
			if (pUnicast != NULL) {
				for (i = 0; pUnicast != NULL; i++) {
					printf("Unicast address %u:", i);
					displayAddress(&pUnicast->Address);
					if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
						ULONG SubnetMask = 0;
						UINT8 bytes[4] = { 0 };
						ConvertLengthToIpv4Mask(pUnicast->OnLinkPrefixLength, &SubnetMask);
						bytes[0] = SubnetMask & 0xFF;
						bytes[1] = (SubnetMask >> 8) & 0xFF;
						bytes[2] = (SubnetMask >> 16) & 0xFF;
						bytes[3] = (SubnetMask >> 24) & 0xFF;
						printf("Subnet Mask: %u.%u.%u.%u\n", bytes[0], bytes[1], bytes[2], bytes[3]);
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

			printf("Transmit link speed: %s\n", GetHumanSize(pCurrAddresses->TransmitLinkSpeed, bps_human_sizes, 1000));
			printf("Receive link speed: %s\n", GetHumanSize(pCurrAddresses->ReceiveLinkSpeed, bps_human_sizes, 1000));

			printf("\n");
next_addr:
			pCurrAddresses = pCurrAddresses->Next;
		}
	}
	else {
		printf("Call to GetAdaptersAddresses failed with error: %d\n",
			dwRetVal);
		if (dwRetVal == ERROR_NO_DATA)
			printf("No addresses were found for the requested parameters\n");
		else
			printf("Error: %u\n", dwRetVal);
	}

	if (pAddresses) {
		FREE(pAddresses);
	}

}