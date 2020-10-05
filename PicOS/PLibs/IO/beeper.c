/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"

#if !defined(beeper_pin_on) || !defined(beeper_pin_off)
#error "S: need to define a pin for beeper"
#endif

static byte freq = 1;

#define	BP_ON	0
#define	BP_OFF	1

strand (__beeper, word)

	entry (BP_ON)

		beeper_pin_on;
		delay (freq, BP_OFF);
		release;

	entry (BP_OFF)

		beeper_pin_off;
		data--;
		if (data == 0)
			finish;

		savedata (data);
		delay (freq, BP_ON);
endstrand

#undef	BP_ON
#undef	BP_OFF

void beep (word duration, word fq) {

	killall (__beeper);
	beeper_pin_off;

	if (fq == 0)
		fq = 1;
	else if (fq > 8)
		fq = 8;

	freq = (byte) fq;

	if (duration == 0)
		duration = 1;
	else if (duration > 60)
		duration = 60;

	runstrand (__beeper, duration * 64);
}
