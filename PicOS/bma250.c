#include "sysio.h"
#include "bma250.h"

#ifndef	bma250_delay
#define	bma250_delay		CNOP
#endif

#ifndef	bma250_bring_up
#define	bma250_bring_up		CNOP
#endif

#ifndef	bma250_bring_down
#define	bma250_bring_down	CNOP
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

byte bma250_status = 0;

// ============================================================================

static void clrevent () {

	_BIC (bma250_status, BMA250_STATUS_EVENT);
	// Fully latched interrupts
	wreg (0x21, 0x87);
	bma250_enable;
}

void bma250_on (byte range, byte bandwidth, byte stat) {
//
	byte r16, r17;

	bma250_bring_up;
	// Reset the chip
	wreg (0x14, 0xB6);
	udelay (200);

	bma250_disable;
	_BIS (bma250_status, BMA250_STATUS_ON);
	clrevent ();

	// range & bandwidth
	wreg (0x0F, range);
	wreg (0x10, bandwidth | 0x8);

	// Global interrupt configuration, all interrupts directed to INT1,
	// INT2 is not connected; new-data unmapped, we never use it
	wreg (0x19, 0xF7);

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
	wreg (0x16, r16);
	wreg (0x17, r17);
}

void bma250_move (byte nsamples, byte threshold) {
//
	if (!(bma250_status & BMA250_STATUS_ON))
		return;

	if (nsamples < 1)
		nsamples = 1;
	else if (nsamples > 4)
		nsamples = 4;

	wreg (0x27, nsamples - 1);
	wreg (0x28, threshold);
}

void bma250_tap (byte mode, byte threshold, byte delay, byte nsamples) {
//
	if (!(bma250_status & BMA250_STATUS_ON))
		return;

	if (delay > 7)
		delay = 7;
		
	wreg (0x2A, mode | delay);

	if (nsamples > 3)
		nsamples = 3;

	if (threshold > 31)
		threshold = 31;

	wreg (0x2B, nsamples << 6 | threshold);
}

void bma250_orient (byte blocking, byte mode, byte theta, byte hysteresis) {
//
	if (!(bma250_status & BMA250_STATUS_ON))
		return;

	if (hysteresis > 7)
		hysteresis = 7;

	wreg (0x2C, blocking | mode | (hysteresis << 4));

	if (theta > 63)
		theta = 63;

	wreg (0x2D, theta);
}

void bma250_flat (byte theta, byte hold) {
//
	if (!(bma250_status & BMA250_STATUS_ON))
		return;

	if (theta > 63)
		theta = 63;
	if (hold > 3)
		hold = 3;

	wreg (0x2E, theta);
	wreg (0x2F, hold << 4);
}

void bma250_lowg (byte mode, byte threshold, byte del, byte hysteresis) {
//
	if (!(bma250_status & BMA250_STATUS_ON))
		return;

	wreg (0x22, del);
	wreg (0x23, threshold);
	if (hysteresis > 3)
		hysteresis = 3;

	wreg (0x24, hysteresis | mode);
}

void bma250_highg (byte threshold, byte del, byte hysteresis) {
//
	if (!(bma250_status & BMA250_STATUS_ON))
		return;

	if (hysteresis > 3)
		hysteresis = 3;

	wreg (0x24, hysteresis << 6);

	wreg (0x25, del);
	wreg (0x26, threshold);
}

void bma250_off (byte pow) {

	if (!(bma250_status & BMA250_STATUS_ON))
		// The sensor is off
		return;

	if (pow == 0) {
		// Power down
		wreg (0x11, 0x80);
		bma250_bring_down;
		_BIC (bma250_status, BMA250_STATUS_ON);
		return;
	}

	if (pow > 11)
		// Full power
		pow = 0;
	else
		pow = (16+64) - pow;

	wreg (0x11, pow);
}

void bma250_init (void) {
//
// This is optional, to be used if the sensor must be soft-powered down on
// startup
//
	_BIS (bma250_status, BMA250_STATUS_ON);
	bma250_off (0);
}

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
	*val++ = rreg (0x09) | (((word)(rreg (0x0c))) << 8);

	// Acc data
	for (r = 0x02; r < 0x08; r += 2, val++) {
		if ((*val = ((word)(rreg (r) >> 6)) | ((word)(rreg (r+1)) << 2))
			& 0x200)
				*val |= 0xFC00;
	}

	// Temperature
	*((byte*)val) = rreg (0x08);

	// Clear events
	if (bma250_status & BMA250_STATUS_EVENT)
		clrevent ();
}
