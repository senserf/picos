/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"
#include "hold.h"
#include "ser.h"
#include "serf.h"
#include "form.h"
#include "hold.h"

#define	N_HOLDERS	32
#define	MIN_HOLD_DELAY	0
#define	MAX_HOLD_DELAY	600

#define	N_DISTURBERS	32
#define	MIN_DIST_DELAY	1
#define	MAX_DIST_DELAY	2048

typedef struct {
		sint ID;
		lword target_sec;
} hold_t;
		
lword	disturber_wait_count = 0, holder_wait_count = 0;

fsm disturber (sint num) {

	state DS_WAIT:

		word del = (rnd () % (MAX_DIST_DELAY - MIN_DIST_DELAY)) +
			MIN_DIST_DELAY;
		disturber_wait_count++;
		delay (del, DS_WAIT);
}

fsm holder (hold_t *h) {

	state DS_START:

		h->target_sec = seconds () +
			(lrnd () % (MAX_HOLD_DELAY - MIN_HOLD_DELAY)) +
				MIN_HOLD_DELAY;

	state DS_HOLD:

		hold (DS_HOLD, h->target_sec);
		if (holder_wait_count++ & 1)
			leds (0, 0);
		else
			leds (0, 1);

	state DS_SHOW:

		lint d = (lint)(seconds () - h->target_sec);

		ser_outf (DS_SHOW, "H %d: %lu -> %lu <%ld> [%lu %lu]\r\n",
			h->ID, h->target_sec, seconds (), d,
			disturber_wait_count, holder_wait_count);
		proceed DS_START;
}

fsm root {

	state RS_INIT:

		sint i;
		hold_t *h;

		// Create the disturbers
		for (i = 0; i < N_DISTURBERS; i++)
			runfsm disturber (i);
		// Create the holders
		for (i = 0; i < N_HOLDERS; i++) {
			h = (hold_t*) umalloc (sizeof (hold_t));
			h -> ID = i;
			runfsm holder (h);
		}

		finish;
}
