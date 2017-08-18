/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "sht_xx_a.h"

//
// Driver for SHTxx temperature/humidity sensors, the array version
//
static byte sht_delcnt, sht_status = 0;

#define	SHTXX_STAT_IDLE 	0
#define	SHTXX_STAT_TEMP		1
#define	SHTXX_STAT_HUMID	2

#ifdef	SHTXX_CLOCK_DELAY
#define	cdel	udelay (SHTXX_CLOCK_DELAY)
#else
#define	cdel	CNOP
#endif

static void ckup () { cdel; shtxx_a_ckup; }
static void ckdown () { cdel; shtxx_a_ckdown; }
static void ckck () { ckup (); ckdown (); }
static word dv () { word a; cdel; shtxx_a_data (a); return a; }
static void du () { cdel; shtxx_a_du; }
static void dd () { cdel; shtxx_a_dd; }

// ============================================================================

static void shtxx_cmd (byte cmd) {

	word i;

	// Reset serial interface - just in case
	// ckdown ();
	// Data line is always parked input, i.e., up

	for (i = 0; i < 10; i++)
		ckck ();

	// Starting sequence for the command
	ckup (); dd (); ckdown (); ckup (); du (); ckdown ();

	// The opening sequence of 3 zeros
	dd ();
	for (i = 0; i < 3; i++)
		ckck ();

	// Now for the five least significant bits of cmd

	for (i = 5; i != 0; ) {
		i--;
		if ((cmd >> i) & 1)
			du ();
		else
			dd ();
		ckck ();
	}

	du ();		// Select data input
	ckck ();	// Retrieve (and ignore) the ack bit
}

static void shtxx_get (Boolean ack, byte *res) {
//
// Read one byte from each sensor, res points to the current byte in an
// array of words, i.e., every second byte belongs to another sensor
//
	byte i, r, *rp;
	word v;

	for (i = 0; i < SHTXX_A_SIZE*2; i += 2)
		// Zero out the bytes
		res [i] = 0;

	for (r = 0; r < 8; r++) {
		v = dv ();
		ckck ();
		for (rp = res, i = 0; i < SHTXX_A_SIZE; i++) {
			*rp <<= 1;
			if (v & (1 << i))
				*rp |= 1;
			rp += 2;
		}
	}

	if (ack) {
		// The ACK bit
		dd ();
		ckck ();
		du ();
	}
}
			
static void shtxx_read (word st, word what, word *res) {
//
// Read the sensor
//
	if (sht_status == SHTXX_STAT_IDLE) {

		sht_status = (byte) (what ? SHTXX_STAT_HUMID :
			SHTXX_STAT_TEMP);

		if (sht_status == SHTXX_STAT_HUMID) {
			shtxx_cmd (SHTXX_CMD_HUMID);
			// Delay for at least 55 msec, make it more as we don't
			// check for status in the array version
			what = 75;
		} else {
			shtxx_cmd (SHTXX_CMD_TEMP);
			// Delay for at least 210 msec
			what = 300;
		}

		delay (what, st);
		sht_delcnt = 0;
		release;
	}

	sht_status = SHTXX_STAT_IDLE;
	// Get the bytes

#if LITTLE_ENDIAN
	shtxx_get (YES, ((byte*)res) + 1);
	shtxx_get (NO , ((byte*)res)    );
#else
	shtxx_get (YES, ((byte*)res)    );
	shtxx_get (NO , ((byte*)res) + 1);
#endif
}

void shtxx_a_temp (word st, const byte *junk, address val) {
	shtxx_read (st, 0, val);
}

void shtxx_a_humid (word st, const byte *junk, address val) {
	shtxx_read (st, 1, val);
}

void shtxx_a_init (void) {
/*
 * Init and reset
 */
	shtxx_a_ini_regs;
	shtxx_cmd (SHTXX_CMD_RESET);
	mdelay (12);
	sht_status = SHTXX_STAT_IDLE;
}
