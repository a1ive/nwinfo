// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "libnw.h"
#include "utils.h"
#include "sensor/sensors.h"

extern sensor_t sensor_lhm;
extern sensor_t sensor_hwinfo;
extern sensor_t sensor_gpuz;
extern sensor_t sensor_cpu;
extern sensor_t sensor_dimm;
extern sensor_t sensor_gpu;
extern sensor_t sensor_disk_smart;
extern sensor_t sensor_disk_io;
extern sensor_t sensor_net;
extern sensor_t sensor_imc;

static sensor_t* sensor_list[] =
{
	&sensor_lhm,
	&sensor_hwinfo,
	&sensor_gpuz,
	&sensor_cpu,
	&sensor_dimm,
	&sensor_gpu,
	&sensor_disk_smart,
	&sensor_disk_io,
	&sensor_net,
	&sensor_imc,
};

static bool sensor_initialized = false;

void NWL_InitSensors(uint64_t flags)
{
	if (sensor_initialized)
		return;
	for (size_t i = 0; i < ARRAYSIZE(sensor_list); i++)
	{
		sensor_t* s = sensor_list[i];
		if (s == NULL)
			continue;
		if (flags == 0 || (flags & s->flag))
			s->enabled = s->init();
	}
	sensor_initialized = true;
}

void NWL_FreeSensors(void)
{
	if (!sensor_initialized)
		return;
	for (size_t i = 0; i < ARRAYSIZE(sensor_list); i++)
	{
		sensor_t* s = sensor_list[i];
		if (s == NULL)
			continue;
		if (s->enabled)
			s->fini();
	}
	sensor_initialized = false;
}

PNODE NWL_GetSensors(PNODE parent)
{
	if (!sensor_initialized)
		goto out;
	for (size_t i = 0; i < ARRAYSIZE(sensor_list); i++)
	{
		sensor_t* s = sensor_list[i];
		if (s == NULL)
			continue;
		if (!s->enabled)
			continue;
		PNODE node = NWL_NodeAppendNew(parent, s->name, NFLG_ATTGROUP);
		NWL_Debug("SENSOR", "Read sensors from %s", s->name);
		s->get(node);
	}
out:
	return parent;
}

PNODE NW_Sensors(BOOL bAppend)
{
	PNODE node = NWL_NodeAlloc("Sensors", NFLG_ATTGROUP);
	if (bAppend)
		NWL_NodeAppendChild(NWLC->NwRoot, node);

	NWL_InitSensors(NWLC->NwSensorFlags);
	NWL_GetSensors(node);
	return node;
}
