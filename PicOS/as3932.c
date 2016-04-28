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

byte as3932_rreg (byte reg) {

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

void as3932_wreg (byte reg, byte val) {

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

void as3932_wcmd (byte cmd) {

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
		if (as3932_data)
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

void as3932_wreg (byte reg, byte val) {

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

void as3932_wcmd (byte cmd) {

	// Select the chip
	as3932_csel;

	put_byte (cmd);

	as3932_cunsel;
	as3932_delay;
}

byte as3932_rreg (byte reg) {

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

byte as3932_status = 0;

// ============================================================================

static void clrevent () {

	_BIC (as3932_status, AS3932_STATUS_EVENT);
	// Clear the wake event
	as3932_wcmd (AS3932_CMD_CWAKE);
	as3932_wcmd (AS3932_CMD_CFALS);
	// RSSI should be reset by CWAKE
	as3932_enable;
}

void as3932_init () {

	// Put the chip into power down, off by default
	as3932_disable;
	as3932_bring_up;
	as3932_wreg (0, 1);
	as3932_bring_down;
}

void as3932_on (byte conf, byte mode, word patt) {

	byte b;

	as3932_bring_up;
	as3932_disable;

	if (conf == 0) {
		// defaults
		as3932_wcmd (AS3932_CMD_DEFAU);
	} else {
		as3932_wreg (0, (conf >> 4) & 0x0E);
		as3932_wreg (7,  conf       & 0x1F);
		b = 0x21;
		if (patt != 0) {
			// Data correlation
			b |= 0x02;
			if (mode & 1)
				// Double pattern
				b |= 0x04;
			// Pattern tolerance
			as3932_wreg (2, (mode << 4) & 0x60);
			as3932_wreg (5, (byte)(patt >> 8));
			as3932_wreg (6, (byte)(patt     ));
		}
		as3932_wreg (1, b);
	}

	_BIS (as3932_status, AS3932_STATUS_ON);
	clrevent ();
}

void as3932_off () {

	if (as3932_status & AS3932_STATUS_ON) {
		as3932_init ();
		_BIC (as3932_status, AS3932_STATUS_ON);
	}
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

	// Read value
	if (!(as3932_status & AS3932_STATUS_ON)) {
		// No data, the sensor is off
		bzero (val, sizeof (as3932_data_t));
		return;
	}

#define	__sd	((as3932_data_t*)val)

	__sd->fwake = as3932_rreg (13);
	for (r = 0; r < 3; r++)
		__sd->rss [r] = as3932_rreg (10 + r) & 0x1F;

#undef __sd
	
	// Clear events
	if (as3932_status & AS3932_STATUS_EVENT)
		clrevent ();
}
