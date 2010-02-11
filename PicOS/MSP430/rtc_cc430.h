#ifndef	__pg_rtc_cc430_h
#define	__pg_rtc_cc430_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "rtc_cc430.c"

#include "sysio.h"
#include "pins.h"
#include "board_rtc.h"

typedef	struct {

	byte	year, month, day, dow, hour, minute, second;

} rtc_time_t;

void	rtc_set (const rtc_time_t*);
void	rtc_get (rtc_time_t*);
void	rtc_calibrate (int);

#endif
