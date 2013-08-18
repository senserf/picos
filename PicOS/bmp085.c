#include "sysio.h"
#include "bmp085.h"

#ifndef	bmp085_delay
#define	bmp085_delay	CNOP
#endif

#define	ADDR_WRITE	0xEE
#define	ADDR_READ	0xEF
#define	REG_CAL_F	0xAA
#define	REG_CAL_L	0xBE
#define	REG_OPER	0xF4
#define	REG_RES		0xF6
#define	OPER_TEMP	0x2E
#define	OPER_PRES	0x34
#define	OPER_PROF	0x40

// ============================================================================

byte bmp085_osrs = 0, bmp085_state = 0;

static void sstrt () {

	// Both SCL and SDA are parked high
	bmp085_sda_lo;
	bmp085_delay;
	bmp085_scl_lo;
}

static void sstop () {

	bmp085_sda_lo;
	bmp085_scl_hi;
	bmp085_sda_hi;
	bmp085_delay;
}

// ============================================================================

static void put_byte (byte b) {

	register int i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			bmp085_sda_hi;
		else
			bmp085_sda_lo;

		bmp085_scl_hi;
		b <<= 1;
		bmp085_scl_lo;
	}

	// Absorb the ACK bit
	bmp085_sda_in;
	bmp085_scl_hi;
	bmp085_scl_lo;
	bmp085_sda_ou;
}

static word get_byte (Boolean wide) {

	register int i;
	register byte b;

	bmp085_sda_in;
	for (b = 0, i = 0; i < 8; i++) {
		bmp085_scl_hi;
		b <<= 1;
		if (bmp085_sda_val)
			b |= 1;
		bmp085_scl_lo;
	}
	bmp085_sda_ou;

	if (wide)
		bmp085_sda_lo;		// ACK
	else
		bmp085_sda_hi;		// NACK

	bmp085_scl_hi;
	bmp085_scl_lo;

	return b;
}

// ============================================================================

static void wreg (byte reg, byte data) {

	sstrt ();
	put_byte (ADDR_WRITE);
	put_byte (reg);
	put_byte (data);
	sstop ();
}

static word rreg (byte reg) {
//
// Read a 16-bit value from a register
//
	word res;

	sstrt ();

	put_byte (ADDR_WRITE);
	put_byte (reg);

	// Repeated start: SCL is low after put_byte, SDA is high

	bmp085_sda_hi;
	bmp085_scl_hi;

	sstrt ();

	put_byte (ADDR_READ);

	res  = get_byte (1) << 8;
	res |= get_byte (0)     ;

	sstop ();

	return res;
}

// ============================================================================

void bmp085_read_calib (word st, const byte *junk, address val) {
//
// Read the calibration data (this is a dummy sensor)
//
	byte reg;
	for (reg = REG_CAL_F; reg <= REG_CAL_L; reg += 2)
		*val++ = rreg (reg);
}

// ============================================================================

void bmp085_read (word st, const byte *junk, address val) {

#ifdef	bmp085_ready
	// Take advantage of the ready signal
	if ((bmp085_state & 1)) {
		// Odd state means waiting for result
		if (!bmp085_ready) {
Delay:
			delay (2, st);
			release;
		}
		bmp085_state++;
	}
#endif

	if (bmp085_state == 0) {
		// Initialize temp measurement
		wreg (REG_OPER, OPER_TEMP);
#ifdef	bmp085_ready
		bmp085_state = 1;
		goto Delay;
#else
		// No ready signal, just wait until you think the result is
		// ready
		bmp085_state = 2;
Delay:
		delay (40, st);
		release;
#endif
	}

	if (bmp085_state == 2) {
		// Temperature ready
		*val = rreg (REG_RES);
		// Start pressure readout
		if (bmp085_osrs > 3)
			bmp085_osrs = 3;
		wreg (REG_OPER, OPER_PRES | (bmp085_osrs << 6));
		bmp085_state = 3;
		goto Delay;
	}

	// Both results ready
	*(val + 1) = rreg (REG_RES);
	bmp085_state = 0;
}
