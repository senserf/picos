/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "ser.h"
#include "serf.h"

#define	ON_TIME		128
#define	OFF_TIME	(5 * 1024)

#if UART_DRIVER
#define	POWER_MODE	1
#else
#define	POWER_MODE	2
#endif

// This simple app features a somewhat artificially complicated FSM to
// periodically blink a LED. Its role is educational. There is a simple
// (built-in) mechanism for blinking LEDs in PicOS which we ignore.

typedef struct {
	// This data structure describes one LED to be blinked. It is assumed
	// that (in principle) we can have multiple copies of the blinker fsm,
	// one for each LED. Attribute led is the LED number (0, 1, ...), state
	// describes the current state of the LED needed by blinker (see below).
	byte led, state;
} led_status_t;

fsm blinker (led_status_t *lstat) {
	// Here is the FSM. The argument points to a data structure (see above)
	// representing the LED to be controled by the FSM. It appears to the
	// FSM as a "static" and private variable, i.e., one whose contents
	// survive state transitions. The primary role of an FSM argument is
	// to differentiate different copies of the same FSM by giving them
	// different (private) data to work on.

	state CHECK_STATUS:

		// The single state is activated whenever there is anything
		// to do. Attribute state can have one of the following
		// values:
		//
		//	0 - the LED should go off and stay off (stop blinking)
		//	1 - the LED should go off, and back on after a delay
		//	2 - the LED should go on, and back off after a delay

		if (lstat->state < 2) {
			// state is 0 or 1, first make sure the LED is off
			// (leds is a system function)
			leds (lstat->led, 0);
			if (lstat->state) {
				// state is nonzero, i.e., 1; this means that
				// we continue blinking; state is set to 2 and
				// the FSM delays for OFF_TIME
				lstat->state = 2;
				delay (OFF_TIME, CHECK_STATUS);
			}
			// Otherwise (state is zero) we don't wait for the
			// timer; this value means that we stop blinking (and
			// do nothing) until the blinking is turned on (by a
			// call to blink - see below)
		} else {
			// state is 2; the LED is turned on
			leds (lstat->led, 1);
			// Then we set state to 1 to turn the LED off ...
			lstat->state = 1;
			// ... after this many milliseconds
			delay (ON_TIME, CHECK_STATUS);
		}

		// Regardless of whether we are waiting on the timer (delay
		// has been called) or not, we also wait for an event triggered
		// when blink is called. When that happens, we kick the FSM to
		// run in its only state - to respond to the change; it is a
		// standard practice to use addresses of data structures as
		// event identifiers: any integer value can be used for that
		// purpose
		when (lstat, CHECK_STATUS);
}

void blink (led_status_t *lstat, Boolean on) {
	// This is called to turn the blinking on and off. If the argument is
	// YES (true or nonzero), state is set to 2 (so the LED starts in the
	// on state). Otherwise, state is set to 0, so the LED will be turned
	// off and the blinker will stop waiting for a next blink call.
	lstat->state = on ? 2 : 0;
	// Regardless of what has been requested, the blinker is kicked, so the
	// LED responds right away.
	trigger (lstat);
}

fsm root {

	led_status_t *my_led;

	state START:

		setpowermode (POWER_MODE);

#if UART_DRIVER
		ser_out (START,
			"Commands:\r\n"
			"  on\r\n"
			"  off\r\n"
		);
#endif
		my_led = (led_status_t*)umalloc (sizeof (led_status_t));
		my_led -> led = 1;
		blink (my_led, YES);
		runfsm blinker (my_led);

#if UART_DRIVER

	state INPUT:

		char cmd [4];

		ser_in (INPUT, cmd, 4);
		if (cmd [1] == 'n')
			blink (my_led, YES);
		else if (cmd [1] == 'f')
			blink (my_led, NO);
		else
			proceed BAD_INPUT;

	state OKMSG:

		ser_out (OKMSG, "OK\r\n");
		proceed INPUT;

	state BAD_INPUT:

		ser_out (BAD_INPUT, "Illegal command\r\n");
		proceed INPUT;
#else
		finish;
#endif

}
