/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

//
// TMP007 Infrared Thermopile Sensor
//

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

#define	sbus	__i2c_open (tmp007_scl, tmp007_sda, tmp007_rate)

void tmp007_wreg (byte reg, word val) {
//
// Write word to reg
//
	byte msg [3];

	msg [0] = reg;
	msg [1] = (byte)(val >> 8);
	msg [2] = (byte)(val     );

	sbus;
	while (__i2c_op (TMP007_ADDR, msg, 3, NULL, 0));

	lptr = reg;
}

word tmp007_rreg (byte reg) {
//
// Read a 16-bit value from the indicated register
//
	byte msg [2];

	sbus;
	if (lptr != reg) {
		while (__i2c_op (TMP007_ADDR, &reg, 1, msg, 2));
		lptr = reg;
	} else {
		while (__i2c_op (TMP007_ADDR, NULL, 0, msg, 2));
	}

	return ((word)(msg [0]) << 8) | (word)(msg [1]);
}

#else
#error "S: tmp007 only works with I2C_INTERFACE!!!"
#endif

static void clrevent () {

	_BIC (tmp007_status, TMP007_STATUS_EVENT);
	// This is edge-triggered, so let's enable it first
	tmp007_enable;
	// Read the status reg to clear the interrupt
	tmp007_rreg (TMP007_REG_STAT);
}

void tmp007_on (word mode, word enable) {
//
// Conf reg bits:
//
//	5 	- [0] int mode (0), comp mode (1) for comparator flags; we
//		  need INT mode whereby the int (alert) signal is held until
//		  the status register is read
//	6 	- [1] transient correction enable
//	7 	- [0] cumulative collection of flags mirrored in status reg
//	8 	- [0] set to 1 to make the int signal reflect the alert (int
//		  enable)
//	9-11 	- [010] conversion averaging mode: 000-1, 001-2, 020-4, 011-8
//		  100-16, 101-1, 110-2, 111-4
//	12	- [1] conversion mode select: 0-power down, 1-on; writing 0
//		  to CONF powers the module down
//	15	- [0] reset, same as power on reset
//
//	Our default: 0001 0100 0110 0000 = 1440
//
// Enable (mask) bits [reset = all zero]:
//
//	8	- memory corrupt enable
//	9	- data invalid enable
//	10	- low limit for TD enable
//	11	- high limit for TD enable
//	12	- low limit for TO
//	13	- high limit for TO
//	14	- temp conversion ready enable
//	15	- alert enable (set for interrupt reception), mirrored by 8 in
//		  CONF
//
// To enable all limits: 1011 1100 0000 0000 = BC00
//
	tmp007_disable;
	tmp007_bring_up;
	if (enable)
		// Make sure that if anything is enabled, then ALERT in CONF
		// is also set
		mode |= TMP007_CONFIG_ALERT;
	tmp007_wreg (TMP007_REG_CONF, mode);
	tmp007_wreg (TMP007_REG_SMEN, enable);
	_BIS (tmp007_status, TMP007_STATUS_ON);
	clrevent ();
}

void tmp007_setlimits (wint oh, wint ol, wint lh, wint ll) {
//
// These are specified at 0.5 degree resolution; should we use the same
// resolution as for temperature?
//
	tmp007_wreg (TMP007_REG_TOHL, ((word)(oh << 2)) & 0xffc0);
	tmp007_wreg (TMP007_REG_TOLL, ((word)(ol << 2)) & 0xffc0);
	tmp007_wreg (TMP007_REG_TDHL, ((word)(lh << 2)) & 0xffc0);
	tmp007_wreg (TMP007_REG_TDLL, ((word)(ll << 2)) & 0xffc0);
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
//
// V [0] - TD
// V [1] - TO
//
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
Inv:
		bzero (val, 4);
		return;
	}

	// Read the data register
	val [1] = tmp007_rreg (TMP007_REG_TOBJ);

	if (val [1] & 1)
		// Invalid?
		goto Inv;

	val [0] = tmp007_rreg (TMP007_REG_TDIE);

	for (i = 0; i < 2; i++) {
		// Convert to signed temperature; this is in degrees Celsius
		// times 32
		if ((val [i] >>= 2) & 0x2000)
			// Extend sign
			val [i] |= 0xc000;
	}

	if (tmp007_status & TMP007_STATUS_EVENT)
		clrevent ();
}
