/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "kernel.h"
#include "ds18b20.h"

#define	LONG_PULL	550	/* us */
#define	SHORT_PULL	3	/* us */
#define	RESPONSE_DELAY	7	/* us */
#define	SLOT_LENGTH	70	/* us */
#define	SLOT_SPACE	2	/* us */
#define	CONV_TIME	800	/* ms */

#define	CMD_CONVERT	0x44
#define	CMD_WRITE	0x4e
#define	CMD_READ	0xbe
#define CMD_SKIP	0xcc

//
// Driver for the DS18B20 temperature sensor
//
static void b_write_0 () {

	// Note: we stop the clock to make the timing more accurate, but
	// we won't be losing any ticks, as these delays are well below
	// 1ms
	cli;
	ds18b20_pull_down;
	udelay (SLOT_LENGTH);
	ds18b20_release;
	sti;
	udelay (SLOT_SPACE);
}

static void b_write_1 () {

	cli;
	ds18b20_pull_down;
	udelay (SHORT_PULL);
	ds18b20_release;
	sti;
	udelay (SLOT_LENGTH+SLOT_SPACE-SHORT_PULL);
}

static byte b_read () {

	byte res;

	cli;
	ds18b20_pull_down;
	udelay (SHORT_PULL);
	ds18b20_release;
	udelay (RESPONSE_DELAY);
	res = ds18b20_value;
	sti;
	udelay (SLOT_LENGTH+SLOT_SPACE-SHORT_PULL-RESPONSE_DELAY);

	return res;
}

static byte s_reset () {

	word i;

	ds18b20_pull_down;
	udelay (LONG_PULL);

	cli;
	ds18b20_release;
	udelay (SHORT_PULL);
	for (i = 0; i < SLOT_LENGTH; i++) {
		if (ds18b20_value == 0) {
			sti;
			goto OK;
		}
	}
	sti;

	// No response
	return 0;
OK:
	for (i = 0; i < LONG_PULL; i++) {
		if (ds18b20_value) {
			// OK, went up again
			udelay (SLOT_SPACE);
			return 1;
		}
	}

	// Something wrong
	return 0;
}

static void write_byte (byte b) {

	word i;

	for (i = 0; i < 8; i++) {
		if (b & 1)
			b_write_1 ();
		else
			b_write_0 ();
		b >>= 1;
	}
}

static byte read_byte () {

	byte b = 0;
	word i;

	for (i = 0; i < 8; i++) {
		b >>= 1;
		if (b_read ())
			b |= 0x80;
	}

	return b;
}

static int s_state = 0;

void ds18b20_read (word st, word junk, address val) {

Again:
	if (s_state <= 0) {
		// First time around or retrying
		if (s_reset () == 0) {
			// Initialization failure, retry up to 5 times
			if (s_state < -5) {
Failure:
				*val = 0;
				s_state = 0;
				return;
			}
			// Try again
			s_state -= 1;
Retry:
			delay (4, st);
			release;
		}
		// Past init
		write_byte (CMD_SKIP);
		write_byte (CMD_CONVERT);
		s_state = 1;
		delay (CONV_TIME, st);
		release;
	}

	// Read the scratchpad
	if (s_reset () == 0) {
		if (s_state > 5)
			goto Failure;
		s_state++;
		goto Retry;
	}

	s_state = 0;
	write_byte (CMD_SKIP);
	write_byte (CMD_READ);

	// Make it unsigned 
	*val = (read_byte () | (((word) read_byte ()) << 8)) + 2048;

	// Stop reading
	s_reset ();

	if (*val > 4096) {
		// Impossible
		*val = 0;
	}

	// Note: the formula to turn this into centigrade is // (v - 2048) / 16
}

void ds18b20_init () {

	// Perhaps this is not needed at all
	ds18b20_ini_regs;
}
