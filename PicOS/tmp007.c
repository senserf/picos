#include "sysio.h"
#include "tmp007.h"

#ifndef	tmp007_bring_up
#define	tmp007_bring_up		CNOP
#endif

#ifndef	tmp007_bring_down
#define	tmp007_bring_down	CNOP
#endif

static	byte	lptr = BNONE;			// Last pointer register

byte	tmp007_status = 0;

#if I2C_INTERFACE
// Use I2C interface commands; for now, this only works for CC1350; MSP430
// implementation (raw pin) will be trivial, if we ever need it

void tmp007_wreg (byte reg, word val) {
//
// Write word to reg
//
	byte msg [3];

	msg [0] = reg;
	msg [1] = (byte)(val >> 8);
	msg [2] = (byte)(val     );

	while (__i2c_op (TMP007_ADDR, msg, 3, NULL, 0));

	lptr = reg;
}

word tmp007_rreg (byte reg) {
//
// Read a 16-bit value from the indicated register
//
	byte msg [2];

	if (lptr != reg) {
		while (__i2c_op (TMP007_ADDR, &reg, 1, msg, 2));
		lptr = reg;
	} else {
		while (__i2c_op (TMP007_ADDR, NULL, 0, msg, 2));
	}

	return ((word)(msg [0]) << 8) | (word)(msg [1]);
}

#else
#error "S: tmp007 only works with I2C_INTERFACE ops (CC1350)!!!"
#endif

static void clrevent () {

	_BIC (tmp007_status, TMP007_STATUS_EVENT);
	// This is edge-triggered, so let's enable it first
	tmp007_enable;
	// Read the status reg to clear the interrupt
	tmp007_rreg (TMP007_REG_STAT);
}

void tmp007_on (word mode, word enable) {

	tmp007_disable;
	tmp007_bring_up;
	tmp007_wreg (TMP007_REG_CONF, mode);
	tmp007_wreg (TMP007_REG_SMEN, enable);
	_BIS (tmp007_status, TMP007_STATUS_ON);
	clrevent ();
}

void tmp007_setlimits (sint oh, sint ol, sint lh, sint ll) {

	tmp007_wreg (TMP007_REG_TOHL, (word)(oh << 6));
	tmp007_wreg (TMP007_REG_TOLL, (word)(ol << 6));
	tmp007_wreg (TMP007_REG_TDHL, (word)(lh << 6));
	tmp007_wreg (TMP007_REG_TDLL, (word)(ll << 6));
}

void tmp007_off () {

	tmp007_wreg (TMP007_REG_CONF, TMP007_CONFIG_PD);
	tmp007_bring_down;
	_BIC (tmp007_status, TMP007_STATUS_ON);
}

void tmp007_init () {

	tmp007_bring_up;
	tmp007_off ();
}

void tmp007_read (word st, const byte *junk, address val) {

	int i;

	if (val == NULL) {
		// Called to wait for an event
		cli;
		if (tmp007_status & TMP007_STATUS_EVENT) {
			// Restart immediately
			sti;
			proceed (st);
		}
		when (&tmp007_status, st);
		_BIS (tmp007_status, TMP007_STATUS_WAIT);
		sti;
		release;
	}

	if (!(tmp007_status & TMP007_STATUS_ON)) {
		// No data
		*val = 0;
		return;
	}

	// Read the data register
	val [1] = tmp007_rreg (TMP007_REG_TOBJ);

	if (val [1] & 1) {
		// Invalid?
		delay (1, st);
		release;
	}

	val [0] = tmp007_rreg (TMP007_REG_TDIE);

	for (i = 0; i < 2; i++) {
		// Convert to signed temperature
		if ((val [i] >>= 2) & 0x2000)
			// Extend sign
			val [i] |= 0xc000;
	}

	if (tmp007_status & TMP007_STATUS_EVENT)
		clrevent ();
}
