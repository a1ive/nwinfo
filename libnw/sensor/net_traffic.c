// SPDX-License-Identifier: Unlicense

#include <winsock2.h>
#include <netioapi.h>
#include <wlanapi.h>

#include "libnw.h"
#include "utils.h"
#include "sensors.h"
#include "network.h"

static struct
{
	NWLIB_NET_TRAFFIC traffic;
} ctx;

static bool net_init(void)
{
	NWLC->NwNetAdapters = NWL_GetNetAdapters(NWLC->NwNetAdapters);
	if (NWLC->NwNetAdapters == NULL)
		return false;
	NWL_GetNetTraffic(&ctx.traffic, FALSE, NWLC->NwNetAdapters);
	return true;
}

static void net_fini(void)
{
	ZeroMemory(&ctx, sizeof(ctx));
}

static void net_get(PNODE node)
{
	NWLC->NwNetAdapters = NWL_GetNetAdapters(NWLC->NwNetAdapters);
	NWL_GetNetTraffic(&ctx.traffic, FALSE, NWLC->NwNetAdapters);

	NWL_NodeAttrSet(node, "Upload Speed", ctx.traffic.StrSend, NAFLG_FMT_HUMAN_SIZE);
	NWL_NodeAttrSet(node, "Download Speed", ctx.traffic.StrRecv, NAFLG_FMT_HUMAN_SIZE);

	BOOL bLonghornOrLater = (NWLC->NwOsInfo.dwMajorVersion >= 6);
	ptrdiff_t count = NWL_NetAdaptersCount(NWLC->NwNetAdapters);
	for (ptrdiff_t i = 0; i < count; i++)
	{
		const NWLIB_NET_ADAPTER* adapter = &NWLC->NwNetAdapters[i].value;
		LPCSTR name = adapter->Description[0] ? adapter->Description : NWLC->NwNetAdapters[i].key;
		PNODE n = NWL_NodeGetChild(node, name);
		if (n == NULL)
			n = NWL_NodeAppendNew(node, name, NFLG_ATTGROUP | NAFLG_FMT_KEY_QUOTE);

		NWL_NodeAttrSet(n, "Upload Speed", NWL_GetHumanSize(adapter->DiffSent, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		NWL_NodeAttrSet(n, "Download Speed", NWL_GetHumanSize(adapter->DiffReceived, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);

		NWL_NodeAttrSet(n, "Upload", NWL_GetHumanSize(adapter->SentOctets, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);
		NWL_NodeAttrSet(n, "Download", NWL_GetHumanSize(adapter->ReceivedOctets, NWLC->NwUnits, 1024), NAFLG_FMT_HUMAN_SIZE);

		if (adapter->WLANState == wlan_interface_state_connected)
			NWL_NodeAttrSetf(n, "Signal Quality", NAFLG_FMT_NUMERIC, "%lu", adapter->WLANSignalQuality);

		if (bLonghornOrLater)
		{
			NWL_NodeAttrSet(n, "Transmit Link Speed",
				NWL_GetHumanSize(adapter->TransmitLinkSpeed, NWL_BPS_UNITS, 1000), NAFLG_FMT_HUMAN_SIZE);
			NWL_NodeAttrSet(n, "Receive Link Speed",
				NWL_GetHumanSize(adapter->ReceiveLinkSpeed, NWL_BPS_UNITS, 1000), NAFLG_FMT_HUMAN_SIZE);
		}
		NWL_NodeAttrSetf(n, "MTU", NAFLG_FMT_HUMAN_SIZE, "%lu", adapter->Mtu);
	}
}

sensor_t sensor_net =
{
	.name = "Network",
	.flag = NWL_SENSOR_NET,
	.init = net_init,
	.get = net_get,
	.fini = net_fini,
};
