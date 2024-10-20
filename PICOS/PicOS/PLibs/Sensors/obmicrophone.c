/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

//
// One-bit microphone noise sensor
//

#include "sysio.h"
#include "pins.h"
#include "obmicrophone.h"

#ifndef	obmic_bring_up
#define	obmic_bring_up		CNOP
#endif

#ifndef	obmic_bring_down
#define	obmic_bring_down	CNOP
#endif

#ifndef	obmic_if
#define	obmic_if		0
#endif

#if 0
static const byte ltab [] = {
// Imbalance table (linear)
	4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0,
	3, 2, 2, 1, 2, 1, 1, 0, 2, 1, 1, 0, 1, 0, 0, 1,
	3, 2, 2, 1, 2, 1, 1, 0, 2, 1, 1, 0, 1, 0, 0, 1,
	2, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 2,
	3, 2, 2, 1, 2, 1, 1, 0, 2, 1, 1, 0, 1, 0, 0, 1,
	2, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 2,
	2, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 2,
	1, 0, 0, 1, 0, 1, 1, 2, 0, 1, 1, 2, 1, 2, 2, 3,
	3, 2, 2, 1, 2, 1, 1, 0, 2, 1, 1, 0, 1, 0, 0, 1,
	2, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 2,
	2, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 2,
	1, 0, 0, 1, 0, 1, 1, 2, 0, 1, 1, 2, 1, 2, 2, 3,
	2, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 2,
	1, 0, 0, 1, 0, 1, 1, 2, 0, 1, 1, 2, 1, 2, 2, 3,
	1, 0, 0, 1, 0, 1, 1, 2, 0, 1, 1, 2, 1, 2, 2, 3,
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
#endif

#if 0
static const byte ltab [] = {
// Imbalance table (quadratic, centered)
	16, 9, 9, 4, 9, 4, 4, 1, 9, 4, 4, 1, 4, 1, 1, 0,
 	 9, 4, 4, 1, 4, 1, 1, 0, 4, 1, 1, 0, 1, 0, 0, 1,
 	 9, 4, 4, 1, 4, 1, 1, 0, 4, 1, 1, 0, 1, 0, 0, 1,
 	 4, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 4,
 	 9, 4, 4, 1, 4, 1, 1, 0, 4, 1, 1, 0, 1, 0, 0, 1,
 	 4, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 4,
 	 4, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 4,
 	 1, 0, 0, 1, 0, 1, 1, 4, 0, 1, 1, 4, 1, 4, 4, 9,
 	 9, 4, 4, 1, 4, 1, 1, 0, 4, 1, 1, 0, 1, 0, 0, 1,
 	 4, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 4,
 	 4, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 4,
 	 1, 0, 0, 1, 0, 1, 1, 4, 0, 1, 1, 4, 1, 4, 4, 9,
 	 4, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 4,
 	 1, 0, 0, 1, 0, 1, 1, 4, 0, 1, 1, 4, 1, 4, 4, 9,
 	 1, 0, 0, 1, 0, 1, 1, 4, 0, 1, 1, 4, 1, 4, 4, 9,
 	 0, 1, 1, 4, 1, 4, 4, 9, 1, 4, 4, 9, 4, 9, 9, 16 };
#endif

static const byte ltab [] = {
// Imbalance table (quadratic)
	64, 36, 36, 16, 36, 16, 16,  4, 36, 16, 16,  4, 16,  4,  4,  0,
	36, 16, 16,  4, 16,  4,  4,  0, 16,  4,  4,  0,  4,  0,  0,  4,
	36, 16, 16,  4, 16,  4,  4,  0, 16,  4,  4,  0,  4,  0,  0,  4,
	16,  4,  4,  0,  4,  0,  0,  4,  4,  0,  0,  4,  0,  4,  4, 16,
	36, 16, 16,  4, 16,  4,  4,  0, 16,  4,  4,  0,  4,  0,  0,  4,
	16,  4,  4,  0,  4,  0,  0,  4,  4,  0,  0,  4,  0,  4,  4, 16,
	16,  4,  4,  0,  4,  0,  0,  4,  4,  0,  0,  4,  0,  4,  4, 16,
 	 4,  0,  0,  4,  0,  4,  4, 16,  0,  4,  4, 16,  4, 16, 16, 36,
	36, 16, 16,  4, 16,  4,  4,  0, 16,  4,  4,  0,  4,  0,  0,  4,
	16,  4,  4,  0,  4,  0,  0,  4,  4,  0,  0,  4,  0,  4,  4, 16,
	16,  4,  4,  0,  4,  0,  0,  4,  4,  0,  0,  4,  0,  4,  4, 16,
 	 4,  0,  0,  4,  0,  4,  4, 16,  0,  4,  4, 16,  4, 16, 16, 36,
	16,  4,  4,  0,  4,  0,  0,  4,  4,  0,  0,  4,  0,  4,  4, 16,
 	 4,  0,  0,  4,  0,  4,  4, 16,  0,  4,  4, 16,  4, 16, 16, 36,
 	 4,  0,  0,  4,  0,  4,  4, 16,  0,  4,  4, 16,  4, 16, 16, 36,
 	 0,  4,  4, 16,  4, 16, 16, 36,  4, 16, 16, 36, 16, 36, 36, 64 };

// ============================================================================

const static byte pconf [] = { obmic_ck, obmic_rx, BNONE, BNONE };

static	obmicrophone_data_t	od;
static	lword			runl = 0;
static	int			last;

static inline void do_update (byte d) {

	od.nsamples++;
	od.amplitude += ltab [d];
}

// ============================================================================

static void interrupt_function () {

	while (__ssi_ready (obmic_if)) {

		if (__ssi_rx_ready (obmic_if))
			do_update (__ssi_rx (obmic_if));

		if (__ssi_tx_ready (obmic_if))
			__ssi_tx (obmic_if, 0);
	}
}

// ============================================================================

void obmicrophone_on (word rate) {

	// In case it was on
	obmicrophone_off ();

	if (rate < OBMICROPHONE_MINRATE)
		rate = OBMICROPHONE_MINRATE;
	else if (rate > OBMICROPHONE_MAXRATE)
		rate = OBMICROPHONE_MAXRATE;

	obmic_bring_up;
	__ssi_open (obmic_if, pconf, obmic_mode, rate, interrupt_function);
	obmicrophone_reset ();
	__ssi_irq_enable_rx (obmic_if);
	__ssi_irq_enable_tx (obmic_if);
}

void obmicrophone_off () {

	__ssi_close (obmic_if);
	obmic_bring_down;
}

void obmicrophone_reset () {

	cli;
	od.nsamples = 0;
	od.amplitude = 0;
	runl = 0;
	sti;
}

void obmicrophone_read (word st, const byte *junk, address val) {
	cli;
	memcpy (val, &od, sizeof (od));
	sti;
}
