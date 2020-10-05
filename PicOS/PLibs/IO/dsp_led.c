/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "form.h"

procname (__blinker);

extern address __blink_pmem;

#define	len    (*((int*)(__blink_pmem + 0)))
#define	pos    (*((int*)(__blink_pmem + 1)))
#define	chr    (*((int*)(__blink_pmem + 2)))
#define	ntv    (*((int*)(__blink_pmem + 3)))
#define	pattern ((char*)(__blink_pmem + 4))

void dsp_led (const char *m, word bint) {

	int cnt;

	if (__blink_pmem) {
		cnt = running (__blinker);
		if (cnt != 0)
			kill (cnt);
		ufree (__blink_pmem);
		__blink_pmem = NULL;
	}

	if (m == NULL || (cnt = strlen (m)) == 0) {
		/* Erase and do nothing */
		cnt = 0xffff;
		leds (0,0);
		leds (1,0);
		leds (2,0);
		leds (3,0);
		return;
	}
	if ((__blink_pmem = umalloc (4 * 2 + (cnt + 1) / 2)) == NULL)
		/* Quietly ignore if we are out of memory */
		return;
	ntv = bint;
	len = cnt;
	chr = 0;
	for (cnt = 0; cnt < len; cnt++) {
		pos = hexcode (m [cnt]);
		if (pos < 0)
			pos = 0;
		if (cnt & 1)
			pattern [chr++] |= (char) pos;
		else
			pattern [chr  ]  = (char) (pos << 4);
	}
	pos = 0;
	fork (__blinker, NULL);
}
