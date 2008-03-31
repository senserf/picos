/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "sht_xx.h"

//
// Driver for SHTxx temperature/humidity sensors
//
static byte shtxx_status = 0;

#define	SHTXX_STAT_IDLE 	0
#define	SHTXX_STAT_TEMP		1
#define	SHTXX_STAT_HUMID	2

static void shtxx_cmd (byte cmd) {

	word ec, i;

	ec = 0;

	do {
		// Reset serial interface - just in case
		shtxx_ckdown;
		shtxx_dtout;
		shtxx_dtup;
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

		if (i == 0)
			return;

		udelay (10);
		ec++;

	} while (ec < 8);

	// Ignore absent sensor
	// syserror (EHARDWARE, "shtxx_cmd");
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
	if (shtxx_status == SHTXX_STAT_IDLE) {

		shtxx_status = (byte) (what ? SHTXX_STAT_HUMID :
			SHTXX_STAT_TEMP);

		if (shtxx_status == SHTXX_STAT_HUMID) {
			shtxx_cmd (SHTXX_CMD_HUMID);
			// Delay for at least 55 msec
			what = 55;
		} else {
			shtxx_cmd (SHTXX_CMD_TEMP);
			// Delay for at least 210 msec
			what = 210;
		}

		if (st == NONE) {
			// We have to busy wait on data; this is highly
			// discouraged!!
			while (shtxx_data);
			goto GetItNow;
		}

		delay (what, st);
		release;
	}

	if (shtxx_data) {
		// Still not ready
		delay (4, st);
		release;
	}

GetItNow:
	shtxx_status = SHTXX_STAT_IDLE;
	// Get the bytes
	what = shtxx_get (YES);
	what = (what << 8) | shtxx_get (NO);

	return what;
}

void shtxx_temp (word st, word junk, address val) {
	*val = shtxx_read (st, 0);
}

void shtxx_humid (word st, word junk, address val) {
	*val = shtxx_read (st, 1);
}

void shtxx_init () {
/*
 * Init and reset
 */
	shtxx_ini_regs;
	shtxx_cmd (SHTXX_CMD_RESET);
	mdelay (12);
	shtxx_status = 0;
}
