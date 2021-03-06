/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"

/* =========== */
/* LED blinker */
/* =========== */

#define	BL_NEXT		0
#define	BL_SHOW		1

address __blink_pmem;

thread (__blinker)

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

	leds (0, (chr >> 0) & 1);
	leds (1, (chr >> 1) & 1);
	leds (2, (chr >> 2) & 1);
	leds (3, (chr >> 3) & 1);
	delay (ntv, BL_NEXT);

endthread
