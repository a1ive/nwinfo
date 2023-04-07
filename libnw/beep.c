// SPDX-License-Identifier: Unlicense

#include <stdlib.h>
#include <string.h>

#include "libnw.h"
#include <libcpuid.h>
#include <winring0.h>

static void
speaker_play(struct wr0_drv_t* drv, unsigned int hz, unsigned long ms)
{
	unsigned int div = 0;
	if (hz == 0)
	{
		io_outb(drv, 0x61, io_inb(drv, 0x61) & 0xfc);
		Sleep (ms);
		return;
	}
	if (hz < 20)
		hz = 20;
	if (hz > 20000)
		hz = 20000;
	div = 1193180 / hz;
	/* speaker freq */
	io_outb(drv, 0x43, 0xb6);
	io_outb(drv, 0x42, (unsigned char)div);
	io_outb(drv, 0x42, (unsigned char)(div >> 8));
	/* speaker on */
	io_outb(drv, 0x61, io_inb(drv, 0x61) | 0x3);
	/* sleep */
	Sleep(ms);
	/* speaker off */
	io_outb(drv, 0x61, io_inb(drv, 0x61) & 0xfc);
}

#include "tokyo.h"

static void
play_default(struct wr0_drv_t* drv)
{
	unsigned i = 0;
	for (i = 0; i < TOKYO_HOT_COUNT - 1; i+=2)
		speaker_play(drv, TOKYO_HOT[i], TOKYO_HOT[i+1]);
}

VOID
NW_Beep(int argc, char* argv[])
{
	int i = 0;
	if (NWLC->NwDrv == NULL)
		return;
	if (argc < 2)
	{
		play_default(NWLC->NwDrv);
		goto fail;
	}
	for (i = 0; i < argc - 1; i+=2)
	{
		unsigned int hz = 0;
		unsigned long time = 0;
		hz = strtoul(argv[i], NULL, 0);
		time = strtoul(argv[i + 1], NULL, 0);
		speaker_play(NWLC->NwDrv, hz, time);
	}
fail:
	speaker_play(NWLC->NwDrv, 0, 0);
}
