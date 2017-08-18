#include "sysio.h"
#include "scp1000.h"

#define	REG_OPER	0x03			// Operation register
#define	REG_RSTR	0x06			// Reset register
#define REG_TEMP	0x81			// Temp readout
#define	REG_PREH	0x7F			// Two parts of pressure
#define	REG_PREL	0x80

#define	COD_RESET	0x01
#define	COD_TRIGGER	0x0c

#define	ADR_WRITE	((0x11 << 1) | 0)	// Dev address (write)
#define	ADR_READ	((0x11 << 1) | 1)	// Dev address (read)

// ============================================================================

static word State = 0;

static void sstrt () {

	// Both SCL and SDA are parked high
	scp1000_sda_lo;
	SCP1000_DELAY;
	scp1000_scl_lo;		// SDA transition HI->LO while SCL is high
}

static void sstop () {

	scp1000_sda_lo;
	scp1000_scl_hi;
	scp1000_sda_hi;		// SDA transition LO->HI while SCL == 1
	SCP1000_DELAY;
}

// ============================================================================

static void put_byte (byte b) {

	register int i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			scp1000_sda_hi;
		else
			scp1000_sda_lo;

		scp1000_scl_hi;
		b <<= 1;
		scp1000_scl_lo;
	}

	// Absorb the ACK bit
	scp1000_sda_in;
	scp1000_scl_hi;
	scp1000_scl_lo;
	scp1000_sda_ou;
}

static word get_byte (Boolean wide) {

	register int i;
	register byte b;

	scp1000_sda_in;
	for (b = 0, i = 0; i < 8; i++) {
		scp1000_scl_hi;
		b <<= 1;
		if (scp1000_sda_val)
			b |= 1;
		scp1000_scl_lo;
	}
	scp1000_sda_ou;

	if (wide)
		scp1000_sda_lo;		// ACK
	else
		scp1000_sda_hi;		// NACK

	scp1000_scl_hi;
	scp1000_scl_lo;

	return b;
}

// ============================================================================

static void wreg (byte reg, byte data) {

	sstrt ();
	put_byte (ADR_WRITE);
	put_byte (reg);
	put_byte (data);
	sstop ();
}

static word rreg (byte reg, Boolean wide) {
//
// Read a short or long value from a register
//
	word res;

	sstrt ();

	put_byte (ADR_WRITE);
	put_byte (reg);

	// Repeated start: SCL is low after put_byte, SDA is high

	scp1000_sda_hi;
	scp1000_scl_hi;
	sstrt ();

	put_byte (ADR_READ);

	if (wide) {
		res  = get_byte (1) << 8;
		res |= get_byte (0)     ;
	} else {
		res = get_byte (0);
	}

	sstop ();

	return res;
}

// ============================================================================

void scp1000_init () {
//
// Reset
//
	mdelay (120);
	wreg (REG_RSTR, COD_RESET);
	mdelay (120);
}

// ============================================================================

void scp1000_read (word st, const byte *junk, address val) {

	if (State == 0) {

		// Initializing measurement
		wreg (REG_OPER, COD_TRIGGER);
		State = 1;
Delay:
		delay (100, st);
		release;

	}

	if (State == 16) {
		// After timeout and reset
		bzero (val, 6);
		State = 0;
		return;
	}

	if (!scp1000_ready) {

		if (State == 15) {
			// Timeout
			wreg (REG_RSTR, COD_RESET);
		}

		State++;
		goto Delay;
	}

	State = 0;
	// Read the values: pressure first
	*((lword*)val) = (((lword) (rreg (REG_PREH, 0) & 0x7)) << 16) |
		rreg (REG_PREL, 1);
	val += 2;
	*val = rreg (REG_TEMP, 1);

	return;
}
