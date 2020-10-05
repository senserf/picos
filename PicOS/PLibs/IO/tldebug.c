/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "tldebug.h"

static 	byte leds_n, leds_s = 0;
Boolean overflow = NO;
static 	word interval;
static 	address buffer;
static 	word in = 0, out = 0, limit;

thread (__tld)

	entry (0)

		// Check if the buffer is empty
		while (in != out) {
			diag (overflow ? "TLV: %x" : "TLD: %x", buffer [out]);
			overflow = NO;
			if (++out == limit)
				out = 0;
		}

		delay (interval, 0);
endthread

// ============================================================================

static void showleds () {

	word i;

	for (i = 0; i < leds_n; i++)
		leds (i, (leds_s >> i) & 1);
}

void tld_put (word v) {

	word nin;

	if (limit == 0) {
		overflow = YES;
		return;
	}

	if ((nin = in + 1) == limit)
		nin = 0;

	if (nin == out) {
		// Overflow
		overflow = YES;
	} else {
		buffer [in] = v;
		in = nin;
	}
}

void tld_lval (word v) {

	word i;

	leds_s = (byte) v;
	showleds ();
}

void tld_incr () {

	leds_s++;
	showleds ();
}

void tld_init (word nw, word nl, word de) {

	if (buffer != NULL)
		return;

	if ((limit = nw + 1) < 5)
		limit = 5;

	if ((buffer = umalloc (limit * 2)) == NULL)
		syserror (EMALLOC, "tld0");

	if ((leds_n = nl) > 8)
		leds_n = 8;

	if ((interval = de) == 0)
		interval = 1;

	if (runthread (__tld) == 0)
		syserror (ERESOURCE, "tld1");

	showleds ();
}
