#ifndef	__pg_as3932_h
#define	__pg_as3932_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2015                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Request mode codes
#define	AS3932_MODE_WRITE	0x00
#define	AS3932_MODE_READ	0x40
#define	AS3932_MODE_CMD		0xC0

// Command codes
#define	AS3932_CMD_CWAKE	0xC0	// Back to listening mode
#define	AS3932_CMD_RSRST	0xC1	// Reset RSS measurement
#define	AS3932_CMD_OSTRM	0xC2	// Start oscillator trim
#define	AS3932_CMD_CFALS	0xC3	// Reset false wakeup register
#define	AS3932_CMD_DEFAU	0xC4	// All registers to default

typedef struct {
//
// This amounts to a long value: false wake count + 3 x RSS; each RSS is
// just 5 bits long
//
	byte	fwake;
	byte	rss [3];

} as3932_data_t;

#ifndef __SMURPH__

#include "as3932_sys.h"
//+++ "as3932.c"

void as3932_init (void);
void as3932_read (word, const byte*, address);

extern byte as3932_status;

#define	AS3932_STATUS_ON	0x01
#define	AS3932_STATUS_WAIT	0x02
#define	AS3932_STATUS_EVENT	0x04

#endif

#ifdef	__SMURPH__

#define as3932_on(a,b,c)	emul (12, "AS3932_ON: %02x %02x %04x", a, b, c)
#define as3932_off()		emul (12, "AS3932_OFF: <>")
#define as3932_rreg(a)		0
#define	as3932_wreg(a,b)	emul (12, "AS3932_WREG: %1d %02x", a, b)
#define	as3932_wcmd(a)		emul (12, "AS3932_WCMD: %02x", a, b)

#else

// Arguments: channels (3 bits), double pattern, tolerance (2 bits),
// pattern 16 bits, bit rate: 5 bits; 0 - defaults
// ttdccc
//
// conf = cccbbbbb	[ channels rate ]
// mode = ttd		[ tolerance double ]
// patt = pattern
void as3932_on (byte conf, byte mode, word patt);
void as3932_off ();
void as3932_wreg (byte, byte);
byte as3932_rreg (byte);
void as3932_wcmd (byte);

#endif

#endif
