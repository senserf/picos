#ifndef	__pg_rtc_s35390_h
#define	__pg_rtc_s35390_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "rtc_s35390.c"

#include "sysio.h"
#include "pins.h"
#include "board_rtc.h"

typedef	struct {

	byte	year, month, day, dow, hour, minute, second;

} rtc_time_t;

word	rtc_set (const rtc_time_t*);
word	rtc_get (rtc_time_t*);
word	rtc_setr (byte);
word	rtc_getr (byte*);

#endif
