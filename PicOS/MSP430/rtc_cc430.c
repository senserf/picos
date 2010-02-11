/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "rtc_cc430.h"

//
// Real Time Clock access for CC430
//

void rtc_set (const rtc_time_t *d) {
//
// Set the clock
//
	while (1) {
		while ((RTCCTL0 & RTCRDYIFG) == 0);
		cli;
		if ((RTCCTL0 & RTCRDYIFG)) {
			RTCSEC = d->second;
			RTCMIN = d->minute;
			RTCHOUR = d->hour;
			RTCDOW = d->dow;
			RTCDAY = d->day;
			RTCMON = d->month;
			RTCYEARL = d->year;
			sti;
			return;
		}
		sti;
	}
}

void rtc_get (rtc_time_t *d) {
//
// Get the date
//
	while (1) {
		while ((RTCCTL0 & RTCRDYIFG) == 0);
		cli;
		if ((RTCCTL0 & RTCRDYIFG)) {
			d->second = RTCSEC;
			d->minute = RTCMIN;
			d->hour = RTCHOUR;
			d->dow = RTCDOW;
			d->day = RTCDAY;
			d->month = RTCMON;
			d->year = RTCYEARL;
			sti;
			return;
		}
		sti;
	}
}

void rtc_calibrate (int ppm) {
//
// Calibrate (the argument is parts per million)
//
	if (ppm > 0)
		RTCCTL2 = RTCCALS | ((ppm > 2) & 0x3f);
	else
		RTCCTL2 = ((-ppm) >> 1) & 0x3f;
}
