//
// Copyright (C) 2017, Olsonet Communications, All rights reserved
//

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

// Bit count table
static const byte ltab [] = {
		0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8 };

// ============================================================================

const static byte pconf [] = { obmic_ck, obmic_rx, BNONE, BNONE };

static obmicrophone_data_t sval;

static void interrupt_function () {

	byte d;

	while (__ssi_ready (obmic_if)) {

		if (__ssi_rx_ready (obmic_if)) {
			d = __ssi_rx (obmic_if);
			sval.level += ltab [d] - 4;
			sval.nsamples++;
		}

		if (__ssi_tx_ready (obmic_if))
			__ssi_tx (obmic_if, 0);
	}
}

// ============================================================================

void obmicrophone_on (word rate) {

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
	sval.nsamples = 0;
	sval.level = 0;
	sti;
}

void obmicrophone_read (word st, const byte *junk, address val) {
	cli;
	memcpy (val, &sval, sizeof (sval));
	sti;
}
