/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

//
// OPT3001 light sensor (no interrupts yet)
//

#include "sysio.h"
#include "opt3001.h"

#ifndef	OPT3001_ADDR
#define	OPT3001_ADDR		0x45
#endif

#ifndef	opt3001_bring_up
#define	opt3001_bring_up	CNOP
#endif

#ifndef	opt3001_bring_down
#define	opt3001_bring_down	CNOP
#endif

#if I2C_INTERFACE
// Use I2C interface commands; for now, this only works for CC1350; MSP430
// implementation (raw pin) will be trivial, if we ever need it

#define	sbus	__i2c_open (opt3001_scl, opt3001_sda, opt3001_rate)

void opt3001_wreg (byte reg, word val) {
//
// Write word to reg
//
	byte msg [3];

	msg [0] = reg;
	msg [1] = (byte)(val >> 8);
	msg [2] = (byte)(val     );

	sbus;
	while (__i2c_op (OPT3001_ADDR, msg, 3, NULL, 0));
}

word opt3001_rreg (byte reg) {
//
// Read a 16-bit value from the indicated register
//
	byte msg [2];

	sbus;
	while (__i2c_op (OPT3001_ADDR, &reg, 1, msg, 2));

	return ((word) msg [0]) << 8 | msg [1];
}

#else
#error "S: opt3001 only works with I2C_INTERFACE!!!"
#endif

// ============================================================================

static byte ostatus;

void opt3001_on (word wmode) {
//
// Configuration
//
// #define	OPT3001_MODE_AUTORANGE	0xC000		// 0x0-0xB
// #define	OPT3001_MODE_TIME_100	0x0000		// fast
// #define	OPT3001_MODE_TIME_800	0x0800		// slow
// #define	OPT3001_MODE_LATCH	0x0010		// Int latch
// #define	OPT3001_MODE_POLARITY_L	0x0000		// Int polarity
// #define	OPT3001_MODE_POLARITY_H	0x0008
// #define	OPT3001_MODE_NOEXP	0x0004		// Don't show exponent
// 
// #define	OPT3001_MODE_FAULT_1	0x0000		// Fault count
// #define	OPT3001_MODE_FAULT_2	0x0001
// #define	OPT3001_MODE_FAULT_4	0x0002
// #define	OPT3001_MODE_FAULT_8	0x0003
// Conversion modes
// #define	OPT3001_MODE_CMODE_SD	0x0000		// Shutdown
// #define	OPT3001_MODE_CMODE_SS	0x0200		// Single shot
// #define	OPT3001_MODE_CMODE_CN	0x0600		// Continuous
//
	if (ostatus & OPT3001_STATUS_ON)
		opt3001_off ();

	opt3001_bring_up;

	ostatus = OPT3001_STATUS_ON;

	// Ignore R/O bits
	wmode &= 0xfe1f;

	if ((wmode & 0x0400) == 0) {
		// Single shot mode, shut it down
		wmode &= ~0x0600;
		ostatus |= OPT3001_STATUS_SS;
	}

	if ((wmode & OPT3001_MODE_TIME_800) == 0)
		ostatus |= OPT3001_STATUS_SLOW;
	
	opt3001_wreg (OPT3001_REG_CONFIG, wmode);
}

void opt3001_off () {

	// Shutdown
	if (ostatus) {
		opt3001_wreg (OPT3001_REG_CONFIG, 0x0000);
		opt3001_bring_down;
		ostatus = 0;
	}
}

void opt3001_setlimits (word hi, word lo) {

	opt3001_wreg (OPT3001_REG_LIMIT_L, lo);
	opt3001_wreg (OPT3001_REG_LIMIT_H, hi);
}

void opt3001_read (word st, const byte *junk, address val) {
//
// Do we need to wait for results?
//
	word md;

	if (ostatus & OPT3001_STATUS_SS) {
		// Single shot mode
		if ((ostatus & OPT3001_STATUS_PENDING) == 0) {
			// Need to start it up
			opt3001_wreg (OPT3001_REG_CONFIG,
				opt3001_rreg (OPT3001_REG_CONFIG) |
					OPT3001_MODE_CMODE_SS);
			ostatus |= OPT3001_STATUS_PENDING;
			md = (ostatus & OPT3001_STATUS_SLOW) ?
				 OPT3001_DELAY_SLOW : OPT3001_DELAY_FAST;
			if (st != WNONE) {
				delay (md, st);
				release;
			}
			mdelay (md);
		}
		while (opt3001_rreg (OPT3001_REG_CONFIG) &
		    OPT3001_MODE_CMODE_SS) {
			if (st != WNONE) {
				delay (2, st);
				release;
			}
			mdelay (2);
		}

		// Conversion done
		ostatus &= ~OPT3001_STATUS_PENDING;
	}
	val [0] = opt3001_rreg (OPT3001_REG_RESULT);
	val [1] = opt3001_rreg (OPT3001_REG_CONFIG);
}
