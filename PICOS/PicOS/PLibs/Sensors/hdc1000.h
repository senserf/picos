/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_hdc1000_h
#define	__pg_hdc1000_h


// Registers (word-sized)

#define	HDC1000_REG_TEMP	0x00
#define	HDC1000_REG_HUMID	0x01
#define	HDC1000_REG_CONFIG	0x02
#define	HDC1000_REG_SID_0	0xFB
#define	HDC1000_REG_SID_1	0xFC
#define	HDC1000_REG_SID_2	0xFD
#define	HDC1000_REG_MID		0xFE
#define	HDC1000_REG_DID		0xFF

// TEMP: 	bits 0-15  T = (T16 / 2^16) * 165 - 40 deg C
// HUMID:	bits 0-15  H = (H16 / 2^16) * 100 %
// CONF:	15	RST (self clears)
//		13	heater on
//		12	mode: 0 T or H, 1 T and H, T first
//		11	battery status: 1 -> < 2.8V
//		10	temp res
//		8-9	humid res

#define	HDC1000_MODE_HEATER	0x2000
#define	HDC1000_MODE_BOTH	0x1000
#define	HDC1000_MODE_TR14	0x0000
#define	HDC1000_MODE_TR11	0x0400
#define	HDC1000_MODE_HR14	0x0000
#define	HDC1000_MODE_HR11	0x0100
#define	HDC1000_MODE_HR8	0x0200

// These are our logical options overriding the "BOTH"
#define	HDC1000_MODE_HUMID	0x0001
#define	HDC1000_MODE_TEMP	0x0002

#define	HDC1000_STATUS_HUMID	0x01
#define	HDC1000_STATUS_TEMP	0x02
#define	HDC1000_STATUS_PENDING	0x04

// ============================================================================


typedef struct {
//
// Sensor value:
//
	wint	temp;
	word	humid;
} hdc1000_data_t;

#ifndef __SMURPH__

#include "pins.h"
//+++ "hdc1000.c"

void hdc1000_read (word, const byte*, address);

void hdc1000_on (word);		// Mode
void hdc1000_off ();
void hdc1000_wreg (byte, word);
word hdc1000_rreg (byte);

#else	/* __SMURPH__ */

#define	hdc1000_on(a)		emul (9, "HDC1000_ON: %04x", a)
#define	hdc1000_off()		emul (9, "HDC1000_OFF: <>")
#define	hdc1000_rreg(a)		0
#define	hdc1000_wreg(a,b)	emul (9, "HDC1000_WREG: %02x %04x", a, b)

#endif	/* __SMURPH__ */


#endif
