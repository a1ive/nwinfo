// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "utils.h"
#include "audio.h"

PNODE NW_Audio(VOID)
{
	UINT count, i;
	NWLIB_AUDIO_DEV* dev = NULL;
	PNODE pNode = NWL_NodeAlloc("Audio", 0);
	if (NWLC->AudioInfo)
		NWL_NodeAppendChild(NWLC->NwRoot, pNode);
	dev = NWL_GetAudio(&count);
	if (!dev)
		return pNode;
	for (i = 0; i < count; i++)
	{
		CHAR buf[32];
		PNODE pChild;
		snprintf(buf, 32, "%u", i);
		pChild = NWL_NodeAppendNew(pNode, buf, NFLG_ATTGROUP);
		NWL_NodeAttrSet(pChild, "Name", NWL_Ucs2ToUtf8(dev[i].name), 0);
		NWL_NodeAttrSet(pChild, "ID", NWL_Ucs2ToUtf8(dev[i].id), 0);
		NWL_NodeAttrSetf(pChild, "Volume", 0, "%.0f%%", dev[i].volume);
		if (dev[i].is_default)
			NWL_NodeAttrSetf(pNode, "Default", NAFLG_FMT_NUMERIC, "%u", i);
	}
	free(dev);
	return pNode;
}
