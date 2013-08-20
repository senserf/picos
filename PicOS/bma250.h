#ifndef	__pg_bma250_h
#define	__pg_bma250_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "bma250_sys.h"
//+++ "bma250.c"

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

typedef	struct {

	word	stat;
	sint	x, y, z;
	char	temp;

} bma250_data_t;

void bma250_on (byte range, byte bandwidth, byte events);
void bma250_move (byte nsamples, byte threshold);
void bma250_tap (byte mode, byte threshold, byte delay, byte nsamples);
void bma250_orient (byte blocking, byte mode, byte theta, byte hysteresis);
void bma250_flat (byte theta, byte hold);
void bma250_lowg (byte mode, byte threshold, byte dur, byte hysteresis);
void bma250_highg (byte threshold, byte dur, byte hysteresis);
void bma250_off (byte);

void bma250_read (word, const byte*, address);

extern byte bma250_status;

#define	BMA250_STATUS_ON	0x01
#define	BMA250_STATUS_WAIT	0x02
#define	BMA250_STATUS_EVENT	0x04

#endif
