/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_opt3001_h
#define	__pg_opt3001_h


// Registers (word-sized)

#define	OPT3001_REG_RESULT	0x00
#define	OPT3001_REG_CONFIG	0x01
#define	OPT3001_REG_LIMIT_L	0x02
#define	OPT3001_REG_LIMIT_H	0x02
#define	OPT3001_REG_MID		0x7E
#define	OPT3001_REG_DID		0x7F

// Config register fields
#define	OPT3001_MODE_AUTORANGE	0xC000
#define	OPT3001_MODE_RANGE_0	0x0000
#define	OPT3001_MODE_RANGE_1	0x1000
#define	OPT3001_MODE_RANGE_2	0x2000
#define	OPT3001_MODE_RANGE_3	0x3000
#define	OPT3001_MODE_RANGE_4	0x4000
#define	OPT3001_MODE_RANGE_5	0x5000
#define	OPT3001_MODE_RANGE_6	0x6000
#define	OPT3001_MODE_RANGE_7	0x7000
#define	OPT3001_MODE_RANGE_8	0x8000
#define	OPT3001_MODE_RANGE_9	0x9000
#define	OPT3001_MODE_RANGE_10	0xa000
#define	OPT3001_MODE_RANGE_11	0xb000
#define	OPT3001_MODE_TIME_100	0x0000
#define	OPT3001_MODE_TIME_800	0x0800
#define	OPT3001_MODE_LATCH	0x0010		// Latch
#define	OPT3001_MODE_POLARITY_L	0x0000		// Int polarity
#define	OPT3001_MODE_POLARITY_H	0x0008
#define	OPT3001_MODE_NOEXP	0x0004		// Don't show exponent

#define	OPT3001_MODE_FAULT_1	0x0000		// Fault count
#define	OPT3001_MODE_FAULT_2	0x0001
#define	OPT3001_MODE_FAULT_4	0x0002
#define	OPT3001_MODE_FAULT_8	0x0003

// Conversion modes
#define	OPT3001_MODE_CMODE_SD	0x0000		// Shutdown
#define	OPT3001_MODE_CMODE_SS	0x0200		// Single shot
#define	OPT3001_MODE_CMODE_CN	0x0600		// Continuous

// Read-only fields
#define	OPT3001_STATUS_OVF	0x0100		// Range overflow
#define	OPT3001_STATUS_RDY	0x0080		// Conversion ready
#define	OPT3001_STATUS_HIGH	0x0040		// Above high limit
#define	OPT3001_STATUS_LOW	0x0020		// Below low limit

// Internal status (for the driver)
#define	OPT3001_STATUS_ON	0x01		// The sensor is up
#define	OPT3001_STATUS_SS	0x02		// Single shot mode
#define	OPT3001_STATUS_SLOW	0x04		// Slow mode
#define	OPT3001_STATUS_PENDING	0x08		// Converting in SS mode

// Conversion delays for SS mode (millisecs)
#define	OPT3001_DELAY_SLOW	800
#define	OPT3001_DELAY_FAST	100

// ============================================================================

typedef struct {

	union {
		word result;
		struct {
			word man:12;
			word exp:4;
		};
	};

	union {
		word status;
		struct {
			word unused3:5;
			word lo:1;
			word hi:1;
			word unused2:1;
			word ovf:1;
			word unused1:7;
		};
	};

} opt3001_data_t;

#ifndef __SMURPH__

#include "pins.h"
//+++ "opt3001.c"

void opt3001_read (word, const byte*, address);

void opt3001_on (word);		// Mode
void opt3001_off ();
void opt3001_wreg (byte, word);
word opt3001_rreg (byte);
void opt3001_setlimits (word, word);

#else	/* __SMURPH__ */

#define	opt3001_on(a)		emul (9, "OPT3001_ON: %04x", a)
#define	opt3001_off()		emul (9, "OPT3001_OFF: <>")
#define	opt3001_rreg(a)		0
#define	opt3001_wreg(a,b)	emul (9, "OPT3001_WREG: %02x %04x", a, b)
#define	opt3001_setlimits(a,b)	emul (9, "OPT3001_SETLIMITS: %04x %04x", a, b)

#endif	/* __SMURPH__ */

#endif
