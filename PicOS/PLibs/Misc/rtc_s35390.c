/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "kernel.h"
#include "rtc_s35390.h"

//
// Real Time Clock: S35390A
//

// Note that the bits are reversed as we send the LSB first (stupid data sheet)
#define	CMD_WSTA1	0x06	// Write status reg 1
#define	CMD_WSTA2	0x46	// Write status reg 2
#define	CMD_WTIME	0x26	// Write RTC data
#define	CMD_GTIME	0xA6	// Get RTC data
#define	CMD_WREG	0x76	// Write register
#define	CMD_GREG	0xF6	// Get register

#define	STA1		0x03	// Status 1: reset + 24h
#define	STA2		0x00	// Status 2: interrupts disabled

static void rtc_start () {
//
// Start condition (when we are called, the clock is high, the data is open
// drain)
//
	rtc_open;
	// Transit to low
	rtc_outl;
	rtc_clkl;
}

static void rtc_stop () {
//
// Send stop condition (the clock is low)
//
	rtc_outl;
	rtc_clkh;
	// Transit to high; we could just drop the line, but the resistor
	// pull up may be slower
	rtc_outh;
	rtc_clkl;

	// Both lines open
	rtc_clkh;
	rtc_set_input;
	rtc_close;
}

static word rtc_wbyte (byte cmd) {
//
// Send a byte (LSB first)
//
	word i;

	for (i = 0; i < 8; i++) {
		if ((cmd & 1))
			rtc_outh;
		else
			rtc_outl;
		rtc_clkh;
		rtc_clkl;
		cmd >>= 1;
	}

	// Detect the ACK bit
	rtc_set_input;
	rtc_clkh;
	rtc_clk_delay;

	// Sample the value
	i = rtc_inp;
	rtc_clkl;

	// Nonzero means failure
	return i;
}

static byte rtc_rbyte (byte lack) {
//
// Receive a byte
//
	byte b, i;

	for (b = i = 0; i < 8; i++) {
		rtc_clkh;
		b >>= 1;
		if (rtc_inp)
			b |= 0x80;
		rtc_clkl;
	}

	// Send the ACK
	rtc_clk_delay;
	if (lack)
		rtc_outh;
	else
		rtc_outl;
	rtc_clkh;
	rtc_clkl;

	// Assume this is our default mode
	rtc_set_input;

	return b;
}

static word rtc_reset () {
//
// Reset the status registers
//
	word r;

	rtc_start ();
	if (rtc_wbyte (CMD_WSTA1)) {
Err:
		rtc_stop ();
		return 1;
	}
	if (rtc_wbyte (STA1))
		goto Err;
	rtc_stop ();

	rtc_start ();
	if (rtc_wbyte (CMD_WSTA2))
		goto Err;
	if (rtc_wbyte (STA2))
		goto Err;
	rtc_stop ();
	return 0;
}

word rtc_set (const rtc_time_t *d) {
//
// Set the clock
//
	word i, w;

	if (rtc_reset ())
		return 1;

	rtc_start ();

	if (rtc_wbyte (CMD_WTIME)) {
		// Error
Err:
		rtc_stop ();
		return 1;
	}

	for (i = 0; i < 7; i++) {
		if ((w = ((byte*)d) [i]) > 99)
			// Should we do better checks?
			w = 0;
		w = ((w / 10) << 4) | (w % 10);
		if (rtc_wbyte ((byte) w))
			goto Err;
	}

	rtc_stop ();
	return 0;
}

word rtc_get (rtc_time_t *d) {
//
// Get the date
//
	word i, w;

	rtc_start ();

	if (rtc_wbyte (CMD_GTIME)) {
		// Error
		rtc_stop ();
		return 1;
	}

	rtc_set_input;

	for (i = 0; i < 7; i++) {
		w = rtc_rbyte (i == 6);
		((byte*)d) [i] = (w >> 4) * 10 + (w & 0xf);
	}

	if (d->hour >= 40)
		d->hour -= 40;
		
	rtc_clkh;
	rtc_close;
	return 0;
}

word rtc_getr (byte *r) {

	rtc_start ();

	if (rtc_wbyte (CMD_GREG)) {
		// Error
		rtc_stop ();
		return 1;
	}

	rtc_set_input;

	*r = rtc_rbyte (1);

	rtc_clkh;
	rtc_close;
	return 0;
}

word rtc_setr (byte r) {

	word i, w;

	rtc_start ();

	if (rtc_wbyte (CMD_WREG)) {
		// Error
Err:
		rtc_stop ();
		return 1;
	}

	if (rtc_wbyte (r))
		goto Err;

	rtc_stop ();
	return 0;
}
