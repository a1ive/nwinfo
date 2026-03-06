// SPDX-License-Identifier: Unlicense

#include <lpcio.h>
#include <ioctl.h>
#include <superio.h>
#include <libnw.h>
#include <chip_ids.h>

#include "lpc.h"

extern NWLIB_LPC_DRV lpc_fintek_drv;
extern NWLIB_LPC_DRV lpc_winbond_drv;
extern NWLIB_LPC_DRV lpc_nuvoton_drv;
extern NWLIB_LPC_DRV lpc_ite_drv;

static NWLIB_LPC_DRV* lpc_drivers[] =
{
	&lpc_fintek_drv,
	&lpc_winbond_drv,
	&lpc_nuvoton_drv,
	&lpc_ite_drv,
};

PNWLIB_LPC
NWL_InitLpc(struct _NWLIB_MAINBOARD_INFO* board)
{
	if (board == NULL)
		return NULL;
	if (!WR0_WaitIsaBus(100))
		return NULL;

	PNWLIB_LPC lpc = calloc(1, sizeof(NWLIB_LPC));
	if (lpc == NULL)
		goto out;
	lpc->board = board;
	lpc->io.drv = NWLC->NwDrv;

	for (enum LPCIO_CHIP_SLOT i = 0; i < LPCIO_SLOT_MAX; i++)
	{
		NWL_Debug("LPC", "Probing slot %d", i);
		if (!lpcio_select_slot(&lpc->io, i))
		{
			NWL_Debug("LPC", "Failed to select slot %d", i);
			continue;
		}
		for (size_t j = 0; j < ARRAYSIZE(lpc_drivers); j++)
		{
			if (lpc_drivers[j]->detect(&lpc->io, board, &lpc->slots[i]))
			{
				NWL_Debug("LPC", "Driver %s detected the chip", lpc_drivers[j]->name);
				break;
			}
		}
	}

out:
	WR0_ReleaseIsaBus();
	return lpc;
}

VOID
NWL_FreeLpc(PNWLIB_LPC lpc)
{
	if (lpc == NULL)
		return;
	free(lpc);
}
