// SPDX-License-Identifier: Unlicense

#include "../libnw.h"
#include "../utils.h"
#include "sensors.h"

static struct
{
	PNODE network;
	NWLIB_NET_TRAFFIC traffic;
} ctx;

static bool net_init(void)
{
	
	return true;
}

static void net_fini(void)
{
	ZeroMemory(&ctx, sizeof(ctx));
}

static void net_get(PNODE node)
{
	
}

sensor_t sensor_net =
{
	.name = "NET",
	.flag = NWL_SENSOR_NET,
	.init = net_init,
	.get = net_get,
	.fini = net_fini,
};
