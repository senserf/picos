#include "sysio.h"
#include "sca3100.h"

// ============================================================================
// Raw pin access only =========================================================
// ============================================================================

static const byte aregs [] = {
//
// MSB-LSB register pairs (including parity) to be fetched by read_sensor
//
	0x15, 0x10,	// X	 5,  4
	0x1C, 0x19,	// Y	 7,  6
	0x25, 0x20,	// Z	 9,  8
	0x34, 0x31	// Temp 13, 12
};

static byte ioxb (byte b) {

	byte i, in;

	for (i = in = 0; i != 8; i++) {
		if (b & 0x80)
			sca3100_outh;
		else
			sca3100_outl;
		sca3100_clkh;
// udelay (20);
		in <<= 1;
		if (sca3100_data)
			in |= 1;
		sca3100_clkl;
// udelay (20);
		b <<= 1;
	}

	return in;
}

#if 0
//
// Not used, maybe later
//
static void wreg (byte reg, byte val) {

	byte sta;

	while (1) {

		// Select the chip
		sca3100_csel;
		udelay (2);

		// Address + params
		sta = ioxb (reg);

		// The data
		ioxb (val);

		// Unselect
		sca3100_cunsel;

		if ((sta & 0x6) == 2) 
			return;

		udelay (2);
	}
}

#endif

static byte rreg (byte reg) {

	byte res, sta, cnt;

	for (cnt = 16; cnt; cnt--) {

		sca3100_csel;
		udelay (2);

		// Address + read
		sta = ioxb (reg);

		// Get the data
		res = ioxb (0);

		// Unselect
		sca3100_cunsel;

		if ((sta & 0x6) == 2) 
			return res;

		udelay (2);
	}

	// syserror (EHARDWARE, "sca-r");
	return 0xAA;
}

static void sclear () {

	byte cnt;

	for (cnt = 16; cnt; cnt--) {
		if (rreg (0x58) == 0)
			return;
	}
	// syserror (EHARDWARE, "sca-c");
}

// ============================================================================

void sca3100_on () {
//
	byte w;

	sca3100_off ();
	mdelay (10);
	sca3100_bring_up;
	mdelay (240);
	sclear ();
}

void sca3100_off () {

	sca3100_bring_down;
}

void sca3100_read (word st, const byte *junk, address val) {

	sint *p;
	sint i;

	p = (sint*) val;

	// Ignore temp for now
	for (i = 0; i < 6; i += 2) {
		sclear ();
		*p =
		 (sint)(((word) (rreg (aregs [i])) << 8) | rreg (aregs [i+1]));
		p++;
	}
}
