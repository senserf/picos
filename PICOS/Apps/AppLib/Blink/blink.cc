/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "blink.h"

typedef struct {

	word	count, on, off, space;
	address next;

} blinkrq_t;

#define	MAX_BLINK_REQUESTS	4
#define	MAX_LEDS		4

static blinkrq_t	*RList [MAX_LEDS];

fsm __blinker (word led) {

	state LOOP:

		if (RList [led] == NULL)
			finish;

	state RUN:

		if (RList [led]->count == 0) {
			// Done
			delay (RList [led]->space, DONE);
			release;
		}

		RList [led]->count--;

		leds (led, 1);
		delay (RList [led]->on, GOFF);
		release;

	state GOFF:

		leds (led, 0);
		delay (RList [led]->off, RUN);
		release;

	state DONE:

		blinkrq_t *p;

		p = RList [led];
		RList [led] = (blinkrq_t*)(p->next);
		ufree (p);
		sameas LOOP;
}

void blink (byte led, byte times, word on, word off, word space) {

	word rc;
	blinkrq_t *p, *q;

	for (rc = 0, p = NULL, q = RList [led]; q != NULL;
		p = q, q = (blinkrq_t*)(q->next), rc++);

	if (rc >= MAX_BLINK_REQUESTS)
		return;

	if ((q = (blinkrq_t*) umalloc (sizeof (blinkrq_t))) == NULL)
		return;

	q->count = times;
	q->on = on;
	q->off = off;
	q->space = space;
	q->next = NULL;

	if (p == NULL) {
		RList [led] = q;
		runfsm __blinker (led);
	} else
		p->next = (address) q;
}
