/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "app.h"

fsm blink (sint led) {

	state START:

		leds (led, 1);
		delay (LED_ON_TIME, STOP);
		release;

	state STOP:

		leds (led, 0);
		finish;
}

static void buttons (word but) {

	// Ignore if busy
	if (running (blink))
		return;

	switch (but) {
		case BUTTON_0:
			runfsm blink (0);
			return;
		case BUTTON_1:
			runfsm blink (1);
			return;
	}
}

fsm root {

	state START:

		setpowermode (POWER_MODE);
		buttons_action (buttons);
		finish;
}
