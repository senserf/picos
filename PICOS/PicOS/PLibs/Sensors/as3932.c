/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "as3932.h"

#ifndef	as3932_delay
#define	as3932_delay		CNOP
#endif

#ifndef	as3932_bring_up
#define	as3932_bring_up		CNOP
#endif

#ifndef	as3932_bring_down
#define	as3932_bring_down	CNOP
#endif

#ifdef	as3932_spi_read
// ============================================================================
// SPI access =================================================================
// ============================================================================

as3932_static byte as3932_rreg (byte reg) {

	volatile byte res;

	// Select the chip
	as3932_csel;
	as3932_delay;

	// Remove SPI interrupt condition
	res = as3932_spi_read;

	// Write the address + the read bit
	as3932_spi_write (reg | AS3932_MODE_READ);

	// Wait until accepted
	while (as3932_busy);
	res = as3932_spi_read;

	// Send dummy data
	as3932_spi_write (0);

	// Wait until accepted
	while (as3932_busy);
	res = as3932_spi_read;

	as3932_cunsel;
	as3932_delay;

	return res;
}

as3932_static void as3932_wreg (byte reg, byte val) {

	volatile byte res;

	// Select the chip
	as3932_csel;
	as3932_delay;

	// Remove SPI interrupt condition
	res = as3932_spi_read;

	// Write the address (straight, read bit == 0)
	as3932_spi_write (reg);

	// Wait until accepted
	while (as3932_busy);
	res = as3932_spi_read;

	// Send the data
	as3932_spi_write (val);

	// Wait until accepted
	while (as3932_busy);
	res = as3932_spi_read;

	as3932_cunsel;
	as3932_delay;
}

as3932_static void as3932_wcmd (byte cmd) {

	// Select the chip
	as3932_csel;
	as3932_delay;

	// Remove SPI interrupt condition
	res = as3932_spi_read;

	// Write the command
	as3932_spi_write (cmd);

	// Wait until accepted
	while (as3932_busy);
	res = as3932_spi_read;

	as3932_cunsel;
	as3932_delay;
}

#else
// ============================================================================
// Raw pin access =============================================================
// ============================================================================

static byte get_byte () {

	register int i;
	register byte b;

	for (b = 0, i = 0; i < 8; i++) {
		b <<= 1;
		as3932_clkh;
		as3932_delay;
		if (as3932_inp)
			b |= 1;
		as3932_clkl;
		as3932_delay;
	}

	return b;
}

static void put_byte (byte b) {

	register int i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			as3932_outh;
		else
			as3932_outl;
		as3932_clkh;
		as3932_delay;
		as3932_clkl;
		as3932_delay;
		b <<= 1;
	}
}

as3932_static void as3932_wreg (byte reg, byte val) {

	// Select the chip
	as3932_csel;

	// Address
	put_byte (reg);

	// The data
	put_byte (val);

	// Unselect
	as3932_cunsel;
	as3932_delay;
}

as3932_static void as3932_wcmd (byte cmd) {

	// Select the chip
	as3932_csel;

	put_byte (cmd);

	as3932_cunsel;
	as3932_delay;
}

as3932_static byte as3932_rreg (byte reg) {

	byte res;

	// Select the chip
	as3932_csel;

	// Address + read
	put_byte (reg | AS3932_MODE_READ);

	// Get the data
	res = get_byte ();

	// Unselect
	as3932_cunsel;

	return res;
}

// ============================================================================
#endif

byte as3932_status = 0, as3932_bytes [AS3932_DATASIZE];

static byte bitnum;

// ============================================================================

void as3932_clearall (byte full) {

	as3932_stop_timer;
	as3932_tim_cli;

	_BIC (as3932_status, AS3932_STATUS_RUNNING);

	as3932_disable_d;
	as3932_disable_w;

	bitnum = 0;

	if (full) {
	        _BIC (as3932_status, AS3932_STATUS_EVENT |
			AS3932_STATUS_BOUNDARY | AS3932_STATUS_DATA);
	        // Clear the wake event
	        as3932_wcmd (AS3932_CMD_CWAKE);
	        as3932_enable_w;
	}
}

void as3932_init () {

	// Put the chip into power down, off by default
	as3932_bring_up;
	// Power down
	as3932_wreg (0, 1);

#ifdef	as3932_detect_absent
	if (as3932_rreg (0) != 1) {
		// Absent
		as3932_bring_down;
		as3932_detect_absent;
		as3932_status = AS3932_STATUS_ABSENT;
		return;
	}
#endif
	as3932_bring_down;
	// This is to be done only once (timeout timer for detecting DAT
	// changes)
	as3932_init_timer;
}

Boolean as3932_on () {

	byte b;

#ifdef	as3932_detect_absent
	if (as3932_status & AS3932_STATUS_ABSENT)
		return NO;
#endif
	as3932_bring_up;
	as3932_wcmd (AS3932_CMD_DEFAU);
	// This one seems to help a bit
	as3932_wreg (3, (0 << 7) | (0 << 6) | (0x0 << 3) | 0x2);
	// This one seems to be useless
	// as3932_wreg (2, (1 << 7) | (0x3 << 5) | 0x0);
	_BIS (as3932_status, AS3932_STATUS_ON);
	as3932_clearall (1);
	as3932_wcmd (AS3932_CMD_CFALS);

	return YES;
}

void as3932_off () {

	if (as3932_status & AS3932_STATUS_ON) {
		as3932_clearall (0);
		// Make sure we are in a decent state
		as3932_wcmd (AS3932_CMD_CWAKE);
		as3932_init ();
		_BIC (as3932_status, AS3932_STATUS_EVENT | AS3932_STATUS_ON);
	}
}

Boolean as3932_addbit () {

	byte bit;

	bit = 0x80 >> (bitnum & 7);

	as3932_bytes [bitnum >> 3] &= ~bit;

	if (as3932_status & AS3932_STATUS_DATA)
		as3932_bytes [bitnum >> 3] |= bit;

	bitnum++;

	return (bitnum == (8 * AS3932_DATASIZE));
}

// ============================================================================

void as3932_read (word st, const byte *junk, address val) {

	word r;

	if (val == NULL) {
		// Wait for event
		cli;
		if (as3932_status & AS3932_STATUS_EVENT) {
			// Restart immediately, as3932 interrupt already
			// disabled
			sti;
			proceed (st);
		}
		// No event ready, wait
		when (&as3932_status, st);
		_BIS (as3932_status, AS3932_STATUS_WAIT);
		sti;
		release;
	}

	if ((as3932_status & AS3932_STATUS_EVENT) == 0) {
		// No data
		bzero (val, AS3932_NBYTES);
		return;
	}

	memcpy (val, as3932_bytes, AS3932_NBYTES);

	// Reset and restart
	as3932_clearall (1);
}
