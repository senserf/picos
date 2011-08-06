#include "sysio.h"
#include "cma_3000.h"

// Low-sensitivity (8g range) motion detection mode
// 0x20 = permanently stay in motion detection mode
// 0x08 = motion detection mode (interrupts enabled)
#define	CMA3000_CONFIG_MD	(0x20 + 0x08)

// High-sensitivity (2g range) motion detection by measurement
// 0x80 = 2g range
// 0x02 = measurement mode at 100Hz
// 0x01 = interrupts disabled
#define	CMA3000_CONFIG_ME	(0x80 + 0x02 + 0x01)

// Motion detection threshold
#define	CMA3000_THRESHOLD_MD	1

// Difference threshold for acceleration in measurement mode
#define	CMA3000_THRESHOLD_ME	2

// Number of initial readings to ignore in measurement mode
#define	CMA3000_SKIPCNT		2

// ============================================================================

static	word NEvents;	// Event count since last readout
static	char LV [3];	// Last readout in measurement mode
static	byte ST;	// Mode: 0 - motion detection, low sensitivity
			//	 <= SKIPCNT - measurement mode (uninitialized)
			//	 SKIPCNT+1  - measurement mode (initialized)

int __pi_cma_3000_event_thread;

#ifdef	__pi_cma_3000_read
// ============================================================================
// SPI access =================================================================
// ============================================================================

static void wreg (byte reg, byte val) {

	volatile byte res;

	// Select the chip
	__pi_cma_3000_csel;
	udelay (10);

	// Remove SPI interrupt condition
	res = __pi_cma_3000_read;

	// Write the address
	res = (reg << 2) | 0x02;
	__pi_cma_3000_write (res);

	// Wait until accepted
	while (__pi_cma_3000_busy);
	res = __pi_cma_3000_read;

	// Send the data
	__pi_cma_3000_write (val);

	// Wait until accepted
	while (__pi_cma_3000_busy);
	res = __pi_cma_3000_read;

	__pi_cma_3000_cunsel;
}

static byte rreg (byte reg) {

	volatile byte res;

	// Select the chip
	__pi_cma_3000_csel;
	udelay (10);

	// Remove SPI interrupt condition
	res = __pi_cma_3000_read;

	// Write the address
	res = reg << 2;
	__pi_cma_3000_write (res);

	// Wait until accepted
	while (__pi_cma_3000_busy);
	res = __pi_cma_3000_read;

	// Send dummy data
	__pi_cma_3000_write (0);

	// Wait until accepted
	while (__pi_cma_3000_busy);
	res = __pi_cma_3000_read;

	__pi_cma_3000_cunsel;

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
		__pi_cma_3000_clkh;
		if (__pi_cma_3000_data)
			b |= 1;
		__pi_cma_3000_clkl;
	}

	return b;
}

static void put_byte (byte b) {

	register int i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			__pi_cma_3000_outh;
		else
			__pi_cma_3000_outl;
		__pi_cma_3000_clkh;
		__pi_cma_3000_clkl;
		b <<= 1;
	}
}

static void wreg (byte reg, byte val) {

	// Select the chip
	__pi_cma_3000_csel;
	udelay (10);

	// Address + write
	put_byte ((reg << 2) | 0x02);

	// The data
	put_byte (val);

	// Unselect
	__pi_cma_3000_cunsel;
}

static byte rreg (byte reg) {

	byte res;

	// Select the chip
	__pi_cma_3000_csel;
	udelay (10);

	// Address + read
	put_byte (reg << 2);

	// Get the data
	res = get_byte ();

	// Unselect
	__pi_cma_3000_cunsel;

	return res;
}

// ============================================================================
#endif

thread (cma_3000_eh_md)
//
// Motion detection mode: interrupt driven
//
	entry (0)

		if (rreg (0x05) & 0x03) {
			// Nonzero
			if (NEvents != MAX_UINT)
				NEvents++;
		}

		when (&__pi_cma_3000_event_thread, 0);
		__pi_cma_3000_enable;
endthread

thread (cma_3000_eh_me)
//
// Measurement mode: running periodically
//
	word d, i; int m;
	char v [3];

	entry (0)

		while (1) {
			// Read twice and compare
			v [0] = (char) rreg (0x06);
			v [1] = (char) rreg (0x07);
			v [2] = (char) rreg (0x08);
			if ((char) rreg (0x06) == v [0] &&
			    (char) rreg (0x07) == v [1] &&
			    (char) rreg (0x08) == v [2]   )
				break;
		}

		if (ST < CMA3000_SKIPCNT+1) {
			// unitialized
			ST++;
			LV [0] = v [0];
			LV [1] = v [1];
			LV [2] = v [2];
		} else {
			for (i = 0, d = 0; i < 3; i++) {
				if ((m = v [i] - LV [i]) < 0)
					m = -m;
				if (m > CMA3000_THRESHOLD_ME)
					d += m;
				LV [i] = v [i];
			}
			if (NEvents + d < NEvents)
				NEvents = MAX_UINT;
			else
				NEvents += d;
		
		}
		delay (20, 0);
endthread

void cma_3000_on (word mode) {
//
// Note: this cannot be used as an automatic initializer (an argument to
// DIGITAL_SENSOR), but you can use one of the wrappers below.
//
	if (__pi_cma_3000_event_thread)
		// Active, switch off first
		cma_3000_off ();

	__pi_cma_3000_bring_up;

	if (mode) {
		// Measurement mode
		ST = 1;
		mode = CMA3000_CONFIG_ME;
	} else {
		ST = 0;
		mode = CMA3000_CONFIG_MD;
	}

	do {
		// May have to retry
		wreg (0x04, 0x02);	// Reset
		wreg (0x04, 0x0A);
		wreg (0x04, 0x04);
		mdelay (10);
		wreg (0x02, mode);
		// Motion sensitivity (not needed, but doesn't hurt, in
		// measurement mode)
		wreg (0x09, CMA3000_THRESHOLD_MD);
		// FIXME: reduce this
		mdelay (10);

	} while (rreg (0x02) != mode);

	NEvents = 0;

	__pi_cma_3000_event_thread =
		runthread (ST ? cma_3000_eh_me : cma_3000_eh_md);
}

void cma_3000_on_md () { cma_3000_on (0); }
void cma_3000_on_me () { cma_3000_on (1); }

void cma_3000_off () {

	__pi_cma_3000_disable;
	__pi_cma_3000_bring_down;

	if (__pi_cma_3000_event_thread)
		kill (__pi_cma_3000_event_thread);
	__pi_cma_3000_event_thread = 0;
}

void cma_3000_read (word st, const byte *junk, address val) {

	cli;
	*val = NEvents;
	NEvents = 0;
	sti;
}
