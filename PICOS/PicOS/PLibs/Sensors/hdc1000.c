/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

//
// HDC1000 humidity/temperature combo
//

#include "sysio.h"
#include "hdc1000.h"

#ifndef	HDC1000_ADDR
#define	HDC1000_ADDR		0x40
#endif

#ifndef	hdc1000_bring_up
#define	hdc1000_bring_up	CNOP
#endif

#ifndef	hdc1000_bring_down
#define	hdc1000_bring_down	CNOP
#endif

#if I2C_INTERFACE
// Use I2C interface commands; for now, this only works for CC1350; MSP430
// implementation (raw pin) will be trivial, if we ever need it

#define	sbus	__i2c_open (hdc1000_scl, hdc1000_sda, hdc1000_rate)

void hdc1000_wreg (byte reg, word val) {
//
// Write word to reg
//
	byte msg [3];

	msg [0] = reg;
	msg [1] = (byte)(val >> 8);
	msg [2] = (byte)(val     );

	sbus;
	while (__i2c_op (HDC1000_ADDR, msg, 3, NULL, 0));
}

word hdc1000_rreg (byte reg) {
//
// Read a 16-bit value from the indicated register
//
	byte msg [2];

	sbus;
	while (__i2c_op (HDC1000_ADDR, &reg, 1, msg, 2));

	return ((word) msg [0]) << 8 | msg [1];
}

static void start_measurement (byte reg) {
//
// Start the measurement by writing to pointer reg
//
	sbus;
	while (__i2c_op (HDC1000_ADDR, &reg, 1, NULL, 0));
}

static Boolean get_data (word *dat, word n) {
//
// Read n words of data
//
	byte msg [4];	// n is 1 or 2
	word i;

	if (__i2c_op (HDC1000_ADDR, NULL, 0, msg, n << 1))
		// Busy
		return YES;

	for (i = 0; i < n; i++)
		dat [i] = ((word) msg [i + i]) << 8 | msg [i + i + 1];

	return NO;
}

#else
#error "S: hdc1000 only works with I2C_INTERFACE!!!"
#endif

// ============================================================================

static byte hstatus;

void hdc1000_on (word wmode) {
//
// Configuration
//
// TEMP: 	bits 0-15  T = (T16 / 2^16) * 165 - 40 deg C
// HUMID:	bits 0-15  H = (H16 / 2^16) * 100 %
// CONF:	15	RST (self clears)
//		13	heater on
//		12	mode: 0 T or H, 1 T and H, T first
//		11	battery status: 1 -> < 2.8V
//		10	temp res
//		8-9	humid res
//		0-7	
//
// #define	HDC1000_MODE_HEATER	0x2000
// #define	HDC1000_MODE_BOTH	0x1000
// #define	HDC1000_MODE_TR14	0x0000
// #define	HDC1000_MODE_TR11	0x0400
// #define	HDC1000_MODE_HR14	0x0000
// #define	HDC1000_MODE_HR11	0x0100
// #define	HDC1000_MODE_HR8	0x0200
//
	if (hstatus == 0)
		// We are powered down
		hdc1000_bring_up;

	if ((hstatus = (wmode & (HDC1000_STATUS_HUMID | HDC1000_STATUS_TEMP))) == 0)
		// None selected, humidity selected by default; note that hstatus is
		// never zero after 'on', so it can be used as a flag to tell we are on
		hstatus = HDC1000_STATUS_HUMID;

	wmode &= 0x2700;
	if (hstatus == (HDC1000_STATUS_HUMID | HDC1000_STATUS_TEMP))
		// Both
		wmode |= HDC1000_MODE_BOTH;

	hdc1000_wreg (HDC1000_REG_CONFIG, wmode);
}

void hdc1000_off () {

	if (hstatus) {
		hstatus = 0;
		// Reset
		hdc1000_wreg (HDC1000_REG_CONFIG, 0x8000);
		hdc1000_bring_down;
	}
}

void hdc1000_read (word st, const byte *junk, address val) {

	byte select;
	sint nw;

	if ((select = (hstatus & (HDC1000_STATUS_HUMID | HDC1000_STATUS_TEMP))) ==
		0)
		// Off
		return;

	// Data length 1 or 2
	nw = (select == (HDC1000_STATUS_HUMID | HDC1000_STATUS_TEMP)) ? 2 : 1;
	if ((hstatus & HDC1000_STATUS_PENDING) == 0) {
		// Have to start it
		start_measurement ((select == HDC1000_STATUS_HUMID) ?
			HDC1000_REG_HUMID : HDC1000_REG_TEMP);
		hstatus |= HDC1000_STATUS_PENDING;
		if (st != WNONE) {
			// Safe conversion time
			delay (7, st);
			release;
		}
		mdelay (7);
	}

	// Try it
	while (get_data (val, nw)) { 
		// Still busy
		if (st != WNONE) {
			delay (1, st);
			release;
		}
	}

	hstatus &= ~HDC1000_STATUS_PENDING;

	// Return the value
	nw--;
	if (select & 1)
		// Convert humid to tenths of percent
		val [nw] = (word)(((lint) (val [nw]) * 1000) / 0x10000);
	if (select & 2) {
		// Convert temp to tenths of degrees
		val [0] = (word)((((lint) (val [0]) * 1650) / 0x10000) - 400);
	}
}
