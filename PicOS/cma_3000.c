#include "sysio.h"
#include "cma_3000.h"

// To accumulate event count for the three coordinate triggers, 4-th entry
// not used
static	lword TStamp;
static	word NEvents;

int zz_cma_3000_event_thread;

static void wreg (byte reg, byte val) {

	volatile byte res;

	// Select the chip
	zz_cma_3000_csel;

	// Remove SPI interrupt condition
	res = zz_cma_3000_read;

	// Write the address
	res = (reg << 2) | 0x02;
	zz_cma_3000_write (res);

	// Wait until accepted
	while (zz_cma_3000_busy);
	res = zz_cma_3000_read;

	// Send the data
	zz_cma_3000_write (val);

	// Wait until accepted
	while (zz_cma_3000_busy);
	res = zz_cma_3000_read;

	zz_cma_3000_cunsel;
}

static byte rreg (byte reg) {

	volatile byte res;

	// Select the chip
	zz_cma_3000_csel;

	// Remove SPI interrupt condition
	res = zz_cma_3000_read;

	// Write the address
	res = reg << 2;
	zz_cma_3000_write (res);

	// Wait until accepted
	while (zz_cma_3000_busy);
	res = zz_cma_3000_read;

	// Send dummy data
	zz_cma_3000_write (0);

	// Wait until accepted
	while (zz_cma_3000_busy);
	res = zz_cma_3000_read;

	zz_cma_3000_cunsel;

	return res;
}

thread (cma_3000_event_handler)

	entry (0)

		if (rreg (0x05) & 0x03) {
			// Nonzero
			if (NEvents != MAX_UINT)
				NEvents++;
			TStamp = seconds ();
		}

		when (&zz_cma_3000_event_thread, 0);
		zz_cma_3000_enable;
endthread

void cma_3000_on () {
//
// Note: this can be called for automatic init (specified as the second arg of
// DIGITAL_SENSOR), but it can also be called explicitly to activate/power up
// the sensor, e.g., after calling cma_3000_off
//
	if (zz_cma_3000_event_thread)
		// Already active
		return;

	zz_cma_3000_bring_up;

	// This is the standard magic to reset the sensor
	wreg (0x04, 0x02);
	wreg (0x04, 0x0A);
	wreg (0x04, 0x04);

	mdelay (10);

	wreg (0x02, CMA3000_CONFIG);
	wreg (0x09, CMA3000_THRESHOLD);

	NEvents = 0;
	zz_cma_3000_event_thread = runthread (cma_3000_event_handler);
}

void cma_3000_off () {

	zz_cma_3000_disable;
	zz_cma_3000_bring_down;

	killall (cma_3000_event_handler);
	zz_cma_3000_event_thread = 0;
}

void cma_3000_read (word st, const byte *junk, address val) {

	if (NEvents == 0) {
		*((lword*)val) = 0;
	} else {
		cli;
		TStamp = seconds () - TStamp;
		*val = (TStamp > MAX_UINT) ? MAX_UINT : (word) TStamp;
		*(val + 1) = NEvents;
		NEvents = 0;
		sti;
	}
}
