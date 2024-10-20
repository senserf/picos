/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "cma3000.h"

#ifndef	cma3000_delay
#define	cma3000_delay	CNOP
#endif

#ifdef	cma3000_spi_read
// ============================================================================
// SPI access =================================================================
// ============================================================================

static void wreg (byte reg, byte val) {

	volatile byte res;

	// Select the chip
	cma3000_csel;
	cma3000_delay;

	// Remove SPI interrupt condition
	res = cma3000_spi_read;

	// Write the address
	res = (reg << 2) | 0x02;
	cma3000_spi_write (res);

	// Wait until accepted
	while (cma3000_busy);
	res = cma3000_spi_read;

	// Send the data
	cma3000_spi_write (val);

	// Wait until accepted
	while (cma3000_busy);
	res = cma3000_spi_read;

	cma3000_cunsel;
}

byte cma3000_rreg (byte reg) {

	volatile byte res;

	// Select the chip
	cma3000_csel;
	cma3000_delay;

	// Remove SPI interrupt condition
	res = cma3000_spi_read;

	// Write the address
	res = reg << 2;
	cma3000_spi_write (res);

	// Wait until accepted
	while (cma3000_busy);
	res = cma3000_spi_read;

	// Send dummy data
	cma3000_spi_write (0);

	// Wait until accepted
	while (cma3000_busy);
	res = cma3000_spi_read;

	cma3000_cunsel;

	return res;
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
		cma3000_clkh;
		if (cma3000_data)
			b |= 1;
		cma3000_clkl;
	}

	return b;
}

static void put_byte (byte b) {

	register int i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			cma3000_outh;
		else
			cma3000_outl;
		cma3000_clkh;
		cma3000_delay;
		cma3000_clkl;
		cma3000_delay;
		b <<= 1;
	}
}

static void wreg (byte reg, byte val) {

	// Select the chip
	cma3000_csel;
	// cma3000_delay;

	// Address + write
	put_byte ((reg << 2) | 0x02);

	// The data
	put_byte (val);

	// Unselect
	cma3000_cunsel;
	cma3000_delay;
}

byte cma3000_rreg (byte reg) {

	byte res;

	// Select the chip
	cma3000_csel;

	// Address + read
	put_byte (reg << 2);

	// Get the data
	res = get_byte ();

	// Unselect
	cma3000_cunsel;

	return res;
}

// ============================================================================
#endif

Boolean	cma3000_wait_pending;
char	cma3000_accdata [4];

static byte pmode;
static Boolean measuring;

static void wreg_n_check (byte reg, byte val) {

	do {
		wreg (reg, val);
	} while (cma3000_rreg (reg) != val);
}

void cma3000_on (byte mode, byte th, byte tm) {
//
	cma3000_off ();
	cma3000_bring_up;

	if (th > 31)
		th = 31;

	if (tm > 15)
		tm = 15;

	// Free fall / motion detection
	pmode = mode ? 0xAC : 0x28;
	tm = (tm << 4) | (tm & 0xf);
	measuring = 0;
	// Flag no event data
	cma3000_accdata [0] = 0;

	do {
		wreg (0x04, 0x02);	// Reset
		wreg (0x04, 0x0A);
		wreg (0x04, 0x04);
		wreg (0x02, pmode);
	} while (cma3000_rreg (0x02) != pmode);

	wreg_n_check (0x09, th);
	wreg_n_check (0x0A, tm);
	wreg_n_check (0x0B, th);

	cma3000_enable;
}

void cma3000_on_auto () { cma3000_on (0, 1, 3); }

void cma3000_off () {

	cma3000_disable;
	cma3000_bring_down;
	pmode = 0;
}

void cma3000_read (word st, const byte *junk, address val) {

	if (val == NULL) {
		// Called to issue a wait request for the event; note that
		// it also makes sense when the sensor is off
		if (pmode) {
			// The sensor is on
			if (measuring) {
				// If we have been measuring, revert the sensor
				// to pmode
				cma3000_accdata [0] = 0;
				wreg_n_check (0x02, pmode);
				measuring = NO;
			}
			cma3000_rreg (0x05);
		}

		if (st == WNONE)
			// Called to only revert to event mode
			return;

		when (&cma3000_wait_pending, st);
		// Mark the thread as waiting
		cma3000_wait_pending = YES;
		if (cma3000_accdata [0] != 0)
			proceed (st);
		release;
	}

	// ====================================================================
	// Called to read the values ==========================================
	// ====================================================================

	if (pmode == 0) {
		// The sensor is off, do nothing
		return;
	}

#define	v ((char*)val)

	if (!measuring && cma3000_accdata [0]) {
		// Return accel data from the event
		memcpy (v, cma3000_accdata, 4);
		cma3000_accdata [0] = 0;
		return;
	}

	if (st == WNONE) {
		v [0] = 0;
		return;
	}

	// Go into measurement mode

	if (!measuring) {
		// Temporarily suspend the previous mode and set to
		// interrupt-less measurements at 400Hz
		wreg_n_check (0x02, 0xA5);
		// Delay until stable (I think the max is 100 msec = 1/MD
		// sampling rate)
		measuring = YES;
		delay (120, st);
		release;
	}

	// Readout
	v [0] = 0;
	while (1) {
		// Read twice and check if the same
		v [1] = (char) cma3000_rreg (0x06);
		v [2] = (char) cma3000_rreg (0x07);
		v [3] = (char) cma3000_rreg (0x08);
		if ((char) cma3000_rreg (0x06) == v [1] &&
		    (char) cma3000_rreg (0x07) == v [2] &&
		    (char) cma3000_rreg (0x08) == v [3]   )
			break;
	}
#undef	v
}
