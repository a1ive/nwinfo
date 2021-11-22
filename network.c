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

static const char*
inet_ntop4(const unsigned char* src, char* dst, size_t size)
{
	static const char* fmt = "%u.%u.%u.%u";
	char tmp[sizeof "255.255.255.255"];
	size_t len;

	len = snprintf(tmp, sizeof tmp, fmt, src[0], src[1], src[2], src[3]);
	if (len >= size) {
		errno = ENOSPC;
		return (NULL);
	}
	memcpy(dst, tmp, len + 1);

	return (dst);
}

#define NS_INT16SZ   2
#define NS_IN6ADDRSZ  16
static const char*
inet_ntop6(const unsigned char* src, char* dst, size_t size)
{
	char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], * tp;
	struct { int base, len; } best, cur;
	unsigned int words[NS_IN6ADDRSZ / NS_INT16SZ];
	int i, inc;

	memset(words, '\0', sizeof words);
	for (i = 0; i < NS_IN6ADDRSZ; i++)
		words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
	best.base = -1;
	cur.base = -1;
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
		if (words[i] == 0) {
			if (cur.base == -1)
				cur.base = i, cur.len = 1;
			else
				cur.len++;
		}
		else {
			if (cur.base != -1) {
				if (best.base == -1 || cur.len > best.len)
					best = cur;
				cur.base = -1;
			}
		}
	}
	if (cur.base != -1) {
		if (best.base == -1 || cur.len > best.len)
			best = cur;
	}
	if (best.base != -1 && best.len < 2)
		best.base = -1;

	tp = tmp;
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
		if (best.base != -1 && i >= best.base &&
			i < (best.base + best.len)) {
			if (i == best.base)
				*tp++ = ':';
			continue;
		}

		if (i != 0)
			*tp++ = ':';
		if (i == 6 && best.base == 0 &&
			(best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
			if (!inet_ntop4(src + 12, tp, sizeof tmp - (tp - tmp)))
				return (NULL);
			tp += strlen(tp);
			break;
		}
		inc = snprintf(tp, 5, "%x", words[i]);
		tp += inc;
	}

	if (best.base != -1 && (best.base + best.len) ==
		(NS_IN6ADDRSZ / NS_INT16SZ))
		*tp++ = ':';
	*tp++ = '\0';

	if ((size_t)(tp - tmp) > size) {
		errno = ENOSPC;
		return (NULL);
	}
	memcpy(dst, tmp, tp - tmp);
	return (dst);
}

static void displayAddress(const PSOCKET_ADDRESS Address)
{
	if (Address->lpSockaddr->sa_family == AF_INET)
	{
		SOCKADDR_IN* si = (SOCKADDR_IN*)(Address->lpSockaddr);
		char a[INET_ADDRSTRLEN] = { 0 };
		if (inet_ntop4((const PVOID)&(si->sin_addr), a, sizeof(a)))
			printf("(IPv4) %s\n", a);
	}
	else if (Address->lpSockaddr->sa_family == AF_INET6)
	{
		SOCKADDR_IN6* si = (SOCKADDR_IN6*)(Address->lpSockaddr);
		char a[INET6_ADDRSTRLEN] = { 0 };
		if (inet_ntop6((const PVOID)&(si->sin6_addr), a, sizeof(a)))
			printf("(IPv6) %s\n", a);
	}
	else
		printf("NULL\n");
}

static UINT32 nt5_htonl(UINT32 x)
{
	UCHAR* s = (UCHAR*)&x;
	return (UINT32)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
}

static void
NT5ConvertLengthToIpv4Mask(ULONG MaskLength, ULONG* Mask)
{
	if (MaskLength > 32UL)
		*Mask = INADDR_NONE;
	else if (MaskLength == 0)
		*Mask = 0;
	else
		*Mask = nt5_htonl(~0U << (32UL - MaskLength));
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
	DWORD dwSize = 0;
	DWORD dwRetVal = 0;
	unsigned int i = 0;
	// Set the flags to pass to GetAdaptersAddresses
	ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS;
	char *lpMsgBuf = NULL;
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
		printf("Description: %wS\n", pCurrAddresses->Description);
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

		pUnicast = pCurrAddresses->FirstUnicastAddress;
		if (pUnicast != NULL) {
			for (i = 0; pUnicast != NULL; i++) {
				printf("Unicast address %u:", i);
				displayAddress(&pUnicast->Address);
				if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
					ULONG SubnetMask = 0;
					UINT8 bytes[4] = { 0 };
					NT5ConvertLengthToIpv4Mask(pUnicast->OnLinkPrefixLength, &SubnetMask);
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