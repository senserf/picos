/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "kernel.h"
#include "sht_xx.h"

//
// Driver for SHTxx temperature/humidity sensors
//
static byte sht_delcnt, sht_status = 0;

#define	SHTXX_STAT_IDLE 	0
#define	SHTXX_STAT_TEMP		1
#define	SHTXX_STAT_HUMID	2

static Boolean shtxx_cmd (byte cmd) {

	word i;

	// Reset serial interface - just in case
	shtxx_ckdown;
	// Data pin parked input, i.e., high
	for (i = 0; i < 10; i++) {
		shtxx_ckup;
		shtxx_ckdown;
	}

	// Starting sequence for the command
	shtxx_ckup;
	shtxx_dtdown;
	shtxx_ckdown;
	shtxx_ckup;
	shtxx_dtup;
	shtxx_ckdown;

	// The opening sequence of 3 zeros
	shtxx_dtdown;

	for (i = 0; i < 3; i++) {
		shtxx_ckup;
		shtxx_ckdown;
	}

	// Now for the five least significant bits of cmd

	for (i = 5; i != 0; ) {
		i--;
		if ((cmd >> i) & 1)
			shtxx_dtup;
		else
			shtxx_dtdown;
		shtxx_ckup;
		shtxx_ckdown;
	}

	// ACK ?
	shtxx_dtin;
	shtxx_ckup;
	i = shtxx_data;
	shtxx_ckdown;

	return (i != 0);
}

static byte shtxx_get (Boolean ack) {
//
// Read one byte from the sensor
//
	byte i, r;

	for (r = i = 0; i < 8; i++) {
		r <<= 1;
		if (shtxx_data)
			r |= 1;
		shtxx_ckup;
		shtxx_ckdown;
	}

	if (ack) {
		// The ACK bit
		shtxx_dtout;
		shtxx_dtdown;
		shtxx_ckup;
		shtxx_ckdown;
		shtxx_dtin;
	}

	return r;
}
			
static word shtxx_read (word st, word what) {
//
// Read the sensor
//
	if (sht_status == SHTXX_STAT_IDLE) {

		sht_status = (byte) (what ? SHTXX_STAT_HUMID :
			SHTXX_STAT_TEMP);

		if (sht_status == SHTXX_STAT_HUMID) {
			if (shtxx_cmd (SHTXX_CMD_HUMID))
				return 0;
			// Delay for at least 55 msec
			what = 55;
		} else {
			if (shtxx_cmd (SHTXX_CMD_TEMP))
				return 0;
			// Delay for at least 210 msec
			what = 210;
		}

		delay (what, st);
		sht_delcnt = 0;
		release;
	}

	if (shtxx_data) {
		// Still not ready
		if (sht_delcnt == 128) {
			// Something wrong, abort
			shtxx_init ();
			return WNONE;
		}
		sht_delcnt++;
		delay (4, st);
		release;
	}

	sht_status = SHTXX_STAT_IDLE;
	// Get the bytes
	what = shtxx_get (YES);
	what = (what << 8) | shtxx_get (NO);

	return what;
}

void shtxx_temp (word st, const byte *junk, address val) {
	*val = shtxx_read (st, 0);
}

void shtxx_humid (word st, const byte *junk, address val) {
	*val = shtxx_read (st, 1);
}

void shtxx_init (void) {
/*
 * Init and reset
 */
	shtxx_ini_regs;
	shtxx_cmd (SHTXX_CMD_RESET);
	mdelay (12);
	sht_status = SHTXX_STAT_IDLE;
}
