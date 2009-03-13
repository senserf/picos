#ifndef	__pg_rtc_h
#define	__pg_rtc_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "pins.h"

// In case we included this accidentally
#ifdef	RTC_PRESENT

#include "board_rtc.h"

typedef	struct {

	byte	year, month, day, dow, hour, minute, second;

} rtc_time_t;

word	rtc_set (const rtc_time_t*);
word	rtc_get (rtc_time_t*);

#endif	/* RTC_PRESENT */
#endif
