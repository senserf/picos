#ifndef	__pg_bma250_h
#define	__pg_bma250_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2014                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// This illustrates how we can VUEE-ify some exotic hardware without pushing
// it to far; this file should be linked to VUEE/PICOS and included from 
// sysio.h

#define	BMA250_RANGE_2G		0x3
#define	BMA250_RANGE_4G		0x5
#define	BMA250_RANGE_8G		0x8
#define BMA250_RANGE_16G	0xC

#define	BMA250_X_AXIS		0x01
#define	BMA250_Y_AXIS		0x02
#define	BMA250_Z_AXIS		0x04
#define	BMA250_ALL_AXES		(BMA250_X_AXIS | BMA250_Y_AXIS | BMA250_Z_AXIS)

#define	BMA250_TAP_QUIET	0x80
#define	BMA250_TAP_SHOCK	0x40

#define	BMA250_ORIENT_BLKOFF	0x00
#define	BMA250_ORIENT_BLKTHT	0x04
#define	BMA250_ORIENT_BLKSLP	0x08
#define	BMA250_ORIENT_BLKANY	0x0C

#define	BMA250_ORIENT_MODSYM	0x00
#define	BMA250_ORIENT_MODHAS	0x01
#define BMA250_ORIENT_MODLAS	0x02
#define BMA250_ORIENT_MODSYB	0x03

#define	BMA250_LOWG_MODSGL	0x00
#define	BMA250_LOWG_MODSUM	0x04

#define	BMA250_STAT_LOWG	0x01
#define	BMA250_STAT_HIGHG	0x02
#define	BMA250_STAT_MOVE	0x04
#define	BMA250_STAT_TAP_D	0x10
#define	BMA250_STAT_TAP		0x20
#define	BMA250_STAT_TAP_S	BMA250_STAT_TAP
#define	BMA250_STAT_ORIENT	0x40
#define	BMA250_STAT_FLAT	0x80

#define	BMA250_STAT_HX		0x0100
#define	BMA250_STAT_HY		0x0200
#define	BMA250_STAT_HZ		0x0400
#define	BMA250_STAT_HS		0x0800
#define	BMA250_STAT_O0		0x1000
#define	BMA250_STAT_O1		0x2000
#define	BMA250_STAT_ZU		0x4000
#define	BMA250_STAT_ISFLAT	0x8000

#ifdef __SMURPH__
//
// A long value transformed into "some" attributes
//
typedef	struct {

	int 	stat:8, x:6, y:6, z:6, temp:6;

} bma250_data_t;

#else
//
// Actual sensor value
//
typedef struct {

	word	stat;		// Events
	sint	x, y, z;	// Accelerations
	char	temp;		// Temperature

} bma250_data_t;

#endif

// ============================================================================

#ifndef	__SMURPH__

#include "pins.h"
//+++ "bma250.c"

void bma250_init (void);

void bma250_read (word, const byte*, address);

extern byte bma250_status;

#define	BMA250_STATUS_ON	0x01
#define	BMA250_STATUS_WAIT	0x02
#define	BMA250_STATUS_EVENT	0x04
#define	BMA250_STATUS_ABSENT	0x80

#endif

// ============================================================================

#ifdef	BMA250_RAW_INTERFACE

typedef struct {
//
// Configuration registers; the population of configurable registers is
// described by a table in bma250.c
//
	lword	rmask;
	byte	regs [20];

} bma250_regs_t;

#ifdef	__SMURPH__

#define	bma250_on(a)		( emul (9, "BMA250_ON: ... regs ..."), YES)
#define	bma250_off()		emul (9, "BMA250_OFF: <>")

#else

Boolean bma250_on (bma250_regs_t*);
void bma250_off ();

#endif

#else	/* BMA250_RAW_INTERFACE */

#ifdef	__SMURPH__

#define	bma250_on(a,b,c)	( emul (1, "BMA250_ON: %1d %1d %02x", a, b, c),\
					YES)
#define	bma250_off(a)		emul (1, "BMA250_OFF: %1d", a)
#define	bma250_move(a,b)	emul (1, "BMA250_MOVE: %1d %1d", a, b)
#define	bma250_tap(a,b,c,d)	emul (1, "BMA250_TAP: %1d %1d %1d %1d", a, b,\
					c, d)
#define	bma250_orient(a,b,c,d)	emul (1, "BMA250_ORIENT: %1d %1d %1d %1d", a,\
					b, c, d)
#define	bma250_flat(a,b)	emul (1, "BMA250_FLAT: %1d %1d", a, b)
#define	bma250_lowg(a,b,c,d)	emul (1, "BMA250_LOWG: %1d %1d %1d %1d", a,\
					b, c, d)
#define	bma250_highg(a,b,c)	emul (1, "BMA250_HIGHG: %1d %1d %1d", a, b, c)

#else

Boolean bma250_on (byte range, byte bandwidth, byte events);
void bma250_move (byte nsamples, byte threshold);
void bma250_tap (byte mode, byte threshold, byte delay, byte nsamples);
void bma250_orient (byte blocking, byte mode, byte theta, byte hysteresis);
void bma250_flat (byte theta, byte hold);
void bma250_lowg (byte mode, byte threshold, byte dur, byte hysteresis);
void bma250_highg (byte threshold, byte dur, byte hysteresis);
void bma250_off (byte);

#endif

#endif	/* BMA250_RAW_INTERFACE */

#ifdef	BMA250_REGACCESS
#define	bma250_static
#else
#define	bma250_static	static
#endif

#endif	/* Conditional include */
