/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2014                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "watch.h"
#include "display.h"
#include "ez430_lcd.h"
#include "rtc_cc430.h"

// This is the global RTC which is updated either at second intervals or
// at minute intervals
rtc_time_t RTC;

time_callback_f every_second [MAX_TIME_CALLBACKS];
time_callback_f every_minute [MAX_TIME_CALLBACKS];

static byte last_second, last_minute;

static void do_callbacks (time_callback_f *ft) {

	word n;

	for (n = 0; n < MAX_TIME_CALLBACKS; n++, ft++) {
		if (*ft == NULL)
			// The list ends at first NULL (which means that it
			// needs to be compressed at callback deletion)
			return;
		(*ft) ();
	}
}

// ============================================================================

fsm watch_driver_fsm {

	word updtime;

	state LOOP:

		last_second = RTC.second;
		last_minute = RTC.minute;

		if (every_second [0]) {
			// Need to update every second
			delay (512, UPDATE_EVENT);
			updtime = 1024;
		} else {
			// Will get away with an update every minute
			delay ((60 - RTC.second) << 10, UPDATE_EVENT);
			updtime = (word)60 * 1024;
		}
WUpd:
		when (&RTC, LOOP);
		release;

	state UPDATE_EVENT:

		word trimdel;

		rtc_get (&RTC);
		trimdel = 0;
		if (every_second [0]) {
			// Updates every second
			if (last_second == RTC.second)
				trimdel = 16;
		} else if (last_minute == RTC.minute) {
			trimdel = 512;
		}

		if (trimdel) {
			updtime++;
			delay (trimdel, UPDATE_EVENT);
			goto WUpd;
		} else {
			updtime--;
		}

		if (every_second [0] && last_second != RTC.second)
			do_callbacks (every_second);

		if (every_minute [0] && last_minute != RTC.minute)
			do_callbacks (every_minute);

		last_second = RTC.second;
		last_minute = RTC.minute;

		delay (updtime, UPDATE_EVENT);

		goto WUpd;
}

// ============================================================================

void watch_start () {
//
// Called only once
//

#if 0
RTC.hour = 13;
RTC.minute = 15;
RTC.second = 31;
RTC.year = 14;
RTC.month = 2;
RTC.day = 17;
rtc_set (&RTC);
#endif

	runfsm watch_driver_fsm;
}

void watch_queue (time_callback_f f, Boolean sec) {

	time_callback_f *q;

	if (f == NULL)
		// This test simplifies usage
		return;

	if (sec) {
		if (every_second [0] == NULL) {
			// Make sure the very next second is perceived as new
			last_second = BNONE;
			rtc_get (&RTC);
			trigger (&RTC);
		}
		q = every_second;
	} else {
		q = every_minute;
	}

	while (*q) q++;

	*q = f;
}

void watch_dequeue (time_callback_f f) {

	time_callback_f *q, *p;
	word c, n;

	if (f == NULL)
		return;

	for (c = 0, q = every_second; c < 2; q = every_minute, c++) {
		for (p = q, n = 0; n < MAX_TIME_CALLBACKS; n++, p++) {
			if (*q == f)
				*q = NULL;
			else {
				*q = *p;
				q++;
			}
		}
	}
}

// ============================================================================
