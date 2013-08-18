#include "sysio.h"
#include "bma250.h"

#ifndef	bma250_delay
#define	bma250_delay	CNOP
#endif

#ifdef	bma250_spi_read
// ============================================================================
// SPI access =================================================================
// ============================================================================

static byte rreg (byte reg) {

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

void msg_xx (word, word);

static void wreg (byte reg, byte val) {

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

#if 0
mdelay (2);

byte c;

if (reg != 0x14) {
c = rreg (reg);
if (c != val) {
msg_xx (1, reg);
msg_xx (0, (((word)c) << 8) | val);
mdelay (4000);
}
}
#endif





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

static void wreg (byte reg, byte val) {

	// Select the chip
	bma250_csel;
	// bma250_delay;

	// Address
	put_byte (reg);

	// The data
	put_byte (val);

	// Unselect
	bma250_cunsel;
	bma250_delay;
}

static byte rreg (byte reg) {

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

Boolean bma250_wait_pending = NO;
byte	bma250_mode = 0;

// ============================================================================

static void disint () {

	bma250_disable;
	wreg (0x16, 0);
	wreg (0x17, 0);
}

static void enaint (byte reg, byte bits) {

	mdelay (1);
	wreg (reg, bits);
	bma250_enable;
}

void bma250_on (byte range, byte bandwidth) {
//
	bma250_bring_up;
	// Reset the chip
	wreg (0x14, 0xB6);
	udelay (200);

	disint ();
	bma250_mode = 1;

	// range & bandwidth
	wreg (0x0F, range);
	wreg (0x10, bandwidth | 0x8);

	// Global interrupt configuration, all interrupts directed to INT1,
	// INT2 is not connected
	wreg (0x19, 0xF7);
	// New-data unmapped, we never use it
}

void bma250_move (byte axes, byte nsamples, byte threshold) {
//
	disint ();
	if (nsamples < 1)
		nsamples = 1;
	else if (nsamples > 4)
		nsamples = 4;

	bma250_mode = 2;
		
	wreg (0x27, nsamples - 1);
	wreg (0x28, threshold);
	enaint (0x16, axes);
}

void bma250_tap (byte mode, byte threshold, byte delay, byte nsamples) {
//
	byte inq;

	disint ();

	if (delay) {
		// double
		inq = 0x10;
		if (--delay > 7)
			delay = 7;
	} else {
		// single
		inq = 0x20;
	}
		
	wreg (0x2A, mode | delay);

	if (nsamples > 3)
		nsamples = 3;

	if (threshold > 31)
		threshold = 31;

	bma250_mode = 3;

	wreg (0x2B, nsamples << 6 | threshold);

	enaint (0x16, inq);
}

void bma250_orient (byte blocking, byte mode, byte theta, byte hysteresis) {
//
	disint ();

	if (hysteresis > 7)
		hysteresis = 7;

	wreg (0x2C, blocking | mode | (hysteresis << 4));

	if (theta > 63)
		theta = 63;

	bma250_mode = 4;

	wreg (0x2D, theta);

	enaint (0x16, 0x40);
}

void bma250_flat (byte theta, byte hold) {
//
	disint ();
	if (theta > 63)
		theta = 63;
	if (hold > 3)
		hold = 3;
	bma250_mode = 5;
	wreg (0x2E, theta);
	wreg (0x2F, hold << 4);

	enaint (0x16, 0x80);
}

void bma250_lowg (byte mode, byte threshold, byte hysteresis) {
//
	disint ();
	wreg (0x23, threshold);
	if (hysteresis > 7)
		hysteresis = 7;
	bma250_mode = 6;
	wreg (0x24, hysteresis | mode);

	enaint (0x17, 0x08);
}

void bma250_highg (byte axes, byte time, byte threshold, byte hysteresis) {
//
	disint ();

	if (hysteresis > 3)
		hysteresis = 3;

	wreg (0x24, hysteresis << 6);

	bma250_mode = 7;

	wreg (0x25, time);
	wreg (0x26, threshold);

	enaint (0x17, axes);
}

void bma250_off (byte pow) {

	if (!bma250_mode)
		// The sensor is off
		return;

	if (pow == 0) {
		// Power down
		disint ();
		wreg (0x11, 0x80);
		bma250_bring_down;
		bma250_mode = 0;
		return;
	}

	if (pow > 11)
		// Full power
		pow = 0;
	else
		pow = (16+64) - pow;

	wreg (0x11, pow);
}

// ============================================================================

void bma250_read (word st, const byte *junk, address val) {

	byte r;

	if (val == NULL) {
		when (&bma250_wait_pending, st);
		bma250_wait_pending = YES;
		release;
	}

	if (bma250_mode == 0) {
		// No data, the sensor is off
		bzero (val, sizeof (bma250_data_t));
		return;
	}

	// Acc data
	for (r = 0x02; r < 0x08; r += 2, val++) {
		if ((*val = ((word)(rreg (r) >> 6)) | ((word)(rreg (r+1)) << 2))
			& 0x200)
				*val |= 0xFC00;
	}

	// Status
	*((byte*)val) = rreg (0x09);

	// Temperature
	*(((byte*)val) + 1) = rreg (0x08);
}
