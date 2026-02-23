// SPDX-License-Identifier: Unlicense
#pragma once

#include <windows.h>
#include "nwapi.h"

#ifndef NWL_STR_SIZE
#define NWL_STR_SIZE 64
#endif

#define NWL_NET_ADDR_STRLEN 46
#define NWL_NET_DESC_LEN 512
#define NWL_NET_MAC_LEN 32
#define NWL_NET_WLAN_PROFILE_LEN 512

extern const char* NWL_BPS_UNITS[6];

typedef struct _NWLIB_NET_ADDRESS
{
	CHAR Address[NWL_NET_ADDR_STRLEN];
	BOOL Ipv6;
	BOOL HasPrefix;
	ULONG PrefixLength;
} NWLIB_NET_ADDRESS;

typedef struct _NWLIB_NET_ADAPTER
{
	CHAR Description[NWL_NET_DESC_LEN];
	ULONG64 Ticks;
	DWORD IfType;
	CHAR MACAddress[NWL_NET_MAC_LEN];
	DWORD OperStatus;
	ULONG Dhcpv4Enabled;
	NWLIB_NET_ADDRESS* Unicasts;
	NWLIB_NET_ADDRESS* Anycasts;
	NWLIB_NET_ADDRESS* Multicasts;
	NWLIB_NET_ADDRESS* Gateways;
	NWLIB_NET_ADDRESS* DNSServers;
	CHAR DHCPServer[NWL_NET_ADDR_STRLEN];
	ULONG64 TransmitLinkSpeed;
	ULONG64 ReceiveLinkSpeed;
	DWORD Mtu;
	DWORD ReceivedOctets;
	DWORD DiffReceived;
	DWORD SentOctets;
	DWORD DiffSent;

	DWORD WLANState;
	CHAR WLANProfile[NWL_NET_WLAN_PROFILE_LEN];
	DWORD WLANSignalQuality;
	DWORD WLANAuth;
	DWORD WLANCipher;
} NWLIB_NET_ADAPTER;

typedef struct _NWLIB_NET_ADAPTER_MAP
{
	CHAR* key;
	NWLIB_NET_ADAPTER value;
} NWLIB_NET_ADAPTER_MAP;

typedef struct _NWLIB_NET_TRAFFIC
{
	double Recv;
	double Send;
	CHAR StrRecv[NWL_STR_SIZE];
	CHAR StrSend[NWL_STR_SIZE];
} NWLIB_NET_TRAFFIC;

LIBNW_API NWLIB_NET_ADAPTER_MAP* NWL_GetNetAdapters(NWLIB_NET_ADAPTER_MAP* adapters);
LIBNW_API ptrdiff_t NWL_NetAdaptersCount(NWLIB_NET_ADAPTER_MAP* adapters);
LIBNW_API VOID NWL_FreeNetAdapters(NWLIB_NET_ADAPTER_MAP* adapters);
LIBNW_API VOID NWL_GetNetTraffic(NWLIB_NET_TRAFFIC* info, BOOL bit, const NWLIB_NET_ADAPTER_MAP* adapters);
