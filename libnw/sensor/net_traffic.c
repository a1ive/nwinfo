// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"
#include "sensors.h"

static struct
{
	PNODE network;
	NWLIB_NET_TRAFFIC traffic;
} ctx;

static bool net_init(void)
{
	ctx.network = NW_Network(FALSE);
	NWL_GetNetTraffic(&ctx.traffic, FALSE);
	return true;
}

static void net_fini(void)
{
	NWL_NodeFree(ctx.network, 1);
	ZeroMemory(&ctx, sizeof(ctx));
}

static void net_get(PNODE node)
{
	NWL_NodeFree(ctx.network, 1);
	ctx.network = NW_Network(FALSE);
	NWL_GetNetTraffic(&ctx.traffic, FALSE);

	NWL_NodeAttrSetf(node, "Upload Speed", NAFLG_FMT_NUMERIC, "%.2f", ctx.traffic.Send);
	NWL_NodeAttrSetf(node, "Download Speed", NAFLG_FMT_NUMERIC, "%.2f", ctx.traffic.Recv);

	INT count = NWL_NodeChildCount(ctx.network);
	for (INT i = 0; i < count; i++)
	{
		PNODE nic = NWL_NodeEnumChild(ctx.network, i);
		if (nic == NULL)
			break;
		LPCSTR str = NWL_NodeAttrGet(nic, "Description");
		PNODE n = NWL_NodeGetChild(node, str);
		if (n == NULL)
			n = NWL_NodeAppendNew(node, str, NFLG_ATTGROUP | NAFLG_FMT_KEY_QUOTE);

		str = NWL_NodeAttrGet(nic, "Received (Octets)");
		if (isdigit(str[0]))
			NWL_NodeAttrSet(n, "Upload", str, NAFLG_FMT_NUMERIC);
		str = NWL_NodeAttrGet(nic, "Sent (Octets)");
		if (isdigit(str[0]))
			NWL_NodeAttrSet(n, "Download", str, NAFLG_FMT_NUMERIC);
		str = NWL_NodeAttrGet(nic, "WLAN Signal Quality");
		if (isdigit(str[0]))
			NWL_NodeAttrSet(n, "Signal Quality", str, NAFLG_FMT_NUMERIC);
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
