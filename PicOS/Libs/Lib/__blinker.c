/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

/* =========== */
/* LED blinker */
/* =========== */

#define	BL_NEXT		0
#define	BL_SHOW		1

address __blink_pmem;

process (__blinker, void)

	nodata;

#define	len    (*((int*)(__blink_pmem + 0)))
#define	pos    (*((int*)(__blink_pmem + 1)))
#define	chr    (*((int*)(__blink_pmem + 2)))
#define	ntv    (*((int*)(__blink_pmem + 3)))
#define	pattern ((char*)(__blink_pmem + 4))

  entry (BL_NEXT)

	if (pos >= len)
		pos = 0;

	chr = pattern [pos >> 1];
	if ((pos & 1) == 0)
		chr >>= 4;
	chr &= 0xf;
	pos++;

  entry (BL_SHOW)

	io (BL_SHOW, LEDS, CONTROL, ((char*)&chr) + 1, LEDS_CNTRL_SET);
	delay (ntv, BL_NEXT);

endprocess (1)