/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "bma250.h"

#ifndef	bma250_delay
#define	bma250_delay		CNOP
#endif

#ifndef	bma250_bring_up
#define	bma250_bring_up		CNOP
#endif

#ifndef	bma250_bring_down
#define	bma250_bring_down_x	CNOP
#else
#define	bma250_bring_down_x	bma250_bring_down
#endif


#ifdef	bma250_spi_read
// ============================================================================
// SPI access =================================================================
// ============================================================================

bma250_static byte bma250_rreg (byte reg) {

	volatile byte res;

	// Select the chip
	bma250_csel;
	bma250_delay;

	// Remove SPI interrupt condition
	res = bma250_spi_read;

	// Write the address + the read bit
	bma250_spi_write (reg | 0x80);

	// Wait until accepted
	while (bma250_busy);
	res = bma250_spi_read;

	// Send dummy data
	bma250_spi_write (0);

	// Wait until accepted
	while (bma250_busy);
	res = bma250_spi_read;

	bma250_cunsel;
	bma250_delay;

	return res;
}

bma250_static void bma250_wreg (byte reg, byte val) {

	volatile byte res;

	// Select the chip
	bma250_csel;
	bma250_delay;

	// Remove SPI interrupt condition
	res = bma250_spi_read;

	// Write the address (straight, read bit == 0)
	bma250_spi_write (reg);

	// Wait until accepted
	while (bma250_busy);
	res = bma250_spi_read;

	// Send the data
	bma250_spi_write (val);

	// Wait until accepted
	while (bma250_busy);
	res = bma250_spi_read;

	bma250_cunsel;
	bma250_delay;
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
		// SCK is parked high
		bma250_clkl;
		if (bma250_data)
			b |= 1;
		bma250_clkh;
	}

	return b;
}

static void put_byte (byte b) {

	register int i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			bma250_outh;
		else
			bma250_outl;
		bma250_clkl;
		bma250_delay;
		bma250_clkh;
		bma250_delay;
		b <<= 1;
	}
}

bma250_static void bma250_wreg (byte reg, byte val) {

	// Select the chip
	bma250_csel;

	// Address
	put_byte (reg);

	// The data
	put_byte (val);

	// Unselect
	bma250_cunsel;
	bma250_delay;
}

bma250_static byte bma250_rreg (byte reg) {

	byte res;

	// Select the chip
	bma250_csel;

	// Address + read
	put_byte (reg | 0x80);

	// Get the data
	res = get_byte ();

	// Unselect
	bma250_cunsel;

	return res;
}

// ============================================================================
#endif

byte bma250_status = 0;

// ============================================================================

static void clrevent () {

	_BIC (bma250_status, BMA250_STATUS_EVENT);
	// Fully latched interrupts
	bma250_wreg (0x21, 0x87);
	bma250_enable;
}

void bma250_init () {

	sint c;

#ifdef	bma250_detect_absent
	// Bring up and try to reset
	bma250_bring_up;
	udelay (200);
	if (bma250_rreg (0) != 0x03) {
		// Chip ID
		bma250_bring_down_x;
		bma250_detect_absent;
		bma250_status = BMA250_STATUS_ABSENT;
		return;
	}
#endif

	for (c = 0; c < 3; c++) {
#ifdef bma250_extra_setreg
		bma250_extra_setreg;
#endif
		bma250_wreg (0x11, 0x80);
	}

	bma250_bring_down_x;
}

#ifdef	BMA250_RAW_INTERFACE

static const byte reglist [] = { 0x0F, 0x10, 0x11, 0x13, 0x16, 0x17, 0x1E,
				 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
				 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F };

Boolean bma250_on (bma250_regs_t *regs) {

	word i;
	byte *r;

#ifdef	bma250_detect_absent
	if (bma250_status & BMA250_STATUS_ABSENT)
		return NO;
#endif
	bma250_bring_up;
	// Reset
	bma250_wreg (0x14, 0xB6);
	udelay (200);

	bma250_disable;

	for (i = 0, r = regs->regs; i < sizeof (reglist); i++, r++)
		if ((regs->rmask >> i) & 1)
			bma250_wreg (reglist [i], *r);

	_BIS (bma250_status, BMA250_STATUS_ON);
	clrevent ();

	// Global interrupt configuration, all interrupts directed to INT1,
	// INT2 is not connected; new-data unmapped, we never use it
	bma250_wreg (0x19, 0xF7);
#ifdef bma250_extra_setreg
	bma250_extra_setreg;
#endif

	return YES;
}

void bma250_off () {

	if (bma250_status & BMA250_STATUS_ON) {
		bma250_init ();
		bma250_bring_down_x;
		_BIC (bma250_status, BMA250_STATUS_ON);
	}
}

#else	/* BMA250_RAW_INTERFACE not defined */

Boolean bma250_on (byte range, byte bandwidth, byte stat) {
//
	byte r16, r17;

#ifdef	bma250_detect_absent
	if (bma250_status & BMA250_STATUS_ABSENT)
		return NO;
#endif

	bma250_bring_up;
	// Reset the chip
	bma250_wreg (0x14, 0xB6);
	udelay (200);

	bma250_disable;
	_BIS (bma250_status, BMA250_STATUS_ON);
	clrevent ();

	// range & bandwidth
	bma250_wreg (0x0F, range);
	bma250_wreg (0x10, bandwidth | 0x8);

	// Global interrupt configuration, all interrupts directed to INT1,
	// INT2 is not connected; new-data unmapped, we never use it
	bma250_wreg (0x19, 0xF7);
#ifdef bma250_extra_setreg
	bma250_extra_setreg;
#endif
	// Enable the events
	r16 = r17 = 0;

	if (stat & BMA250_STAT_MOVE)
		_BIS (r16, BMA250_ALL_AXES);

	if (stat & BMA250_STAT_TAP_S)
		_BIS (r16, 0x20);
	else if (stat & BMA250_STAT_TAP_D)
		_BIS (r16, 0x10);

	if (stat & BMA250_STAT_ORIENT)
		_BIS (r16, 0x40);

	if (stat & BMA250_STAT_FLAT)
		_BIS (r16, 0x80);

	if (stat & BMA250_STAT_LOWG)
		_BIS (r17, 0x08);

	if (stat & BMA250_STAT_HIGHG)
		_BIS (r17, BMA250_ALL_AXES);

	// Enable the events
	bma250_wreg (0x16, r16);
	bma250_wreg (0x17, r17);

	return YES;
}

void bma250_move (byte nsamples, byte threshold) {
//
	if (!(bma250_status & BMA250_STATUS_ON))
		return;

	if (nsamples < 1)
		nsamples = 1;
	else if (nsamples > 4)
		nsamples = 4;

	bma250_wreg (0x27, nsamples - 1);
	bma250_wreg (0x28, threshold);
}

void bma250_tap (byte mode, byte threshold, byte delay, byte nsamples) {
//
	if (!(bma250_status & BMA250_STATUS_ON))
		return;

	if (delay > 7)
		delay = 7;
		
	bma250_wreg (0x2A, mode | delay);

	if (nsamples > 3)
		nsamples = 3;

	if (threshold > 31)
		threshold = 31;

	bma250_wreg (0x2B, nsamples << 6 | threshold);
}

void bma250_orient (byte blocking, byte mode, byte theta, byte hysteresis) {
//
	if (!(bma250_status & BMA250_STATUS_ON))
		return;

	if (hysteresis > 7)
		hysteresis = 7;

	bma250_wreg (0x2C, blocking | mode | (hysteresis << 4));

	if (theta > 63)
		theta = 63;

	bma250_wreg (0x2D, theta);
}

void bma250_flat (byte theta, byte hold) {
//
	if (!(bma250_status & BMA250_STATUS_ON))
		return;

	if (theta > 63)
		theta = 63;
	if (hold > 3)
		hold = 3;

	bma250_wreg (0x2E, theta);
	bma250_wreg (0x2F, hold << 4);
}

void bma250_lowg (byte mode, byte threshold, byte del, byte hysteresis) {
//
	if (!(bma250_status & BMA250_STATUS_ON))
		return;

	bma250_wreg (0x22, del);
	bma250_wreg (0x23, threshold);
	if (hysteresis > 3)
		hysteresis = 3;

	bma250_wreg (0x24, hysteresis | mode);
}

void bma250_highg (byte threshold, byte del, byte hysteresis) {
//
	if (!(bma250_status & BMA250_STATUS_ON))
		return;

	if (hysteresis > 3)
		hysteresis = 3;

	bma250_wreg (0x24, hysteresis << 6);

	bma250_wreg (0x25, del);
	bma250_wreg (0x26, threshold);
}

void bma250_off (byte pow) {

	if (!(bma250_status & BMA250_STATUS_ON))
		// The sensor is off
		return;

	if (pow == 0) {
		// Power down
		bma250_init ();
		bma250_bring_down_x;
		_BIC (bma250_status, BMA250_STATUS_ON);
		return;
	}

	if (pow > 11)
		// Full power
		pow = 0;
	else
		pow = (16+64) - pow;

	bma250_wreg (0x11, pow);
}

#endif	/* BMA250_RAW_INTERFACE */

// ============================================================================

void bma250_read (word st, const byte *junk, address val) {

	byte r;

	if (val == NULL) {
		cli;
		if (bma250_status & BMA250_STATUS_EVENT) {
			// Restart immediately, bma250 interrupt already
			// disabled
			sti;
			proceed (st);
		}
		// No event ready, wait
		when (&bma250_status, st);
		_BIS (bma250_status, BMA250_STATUS_WAIT);
		sti;
		release;
	}

	if (!(bma250_status & BMA250_STATUS_ON)) {
		// No data, the sensor is off
		bzero (val, sizeof (bma250_data_t));
		return;
	}

	// Status
	*val++ = bma250_rreg (0x09) | (((word)(bma250_rreg (0x0c))) << 8);

	// Acc data
	for (r = 0x02; r < 0x08; r += 2, val++) {
		if ((*val = ((word)(bma250_rreg (r) >> 6)) |
			((word)(bma250_rreg (r+1)) << 2)) & 0x200)
				*val |= 0xFC00;
	}

	// Temperature
	*((byte*)val) = bma250_rreg (0x08);

	// Clear events
	if (bma250_status & BMA250_STATUS_EVENT)
		clrevent ();
}
