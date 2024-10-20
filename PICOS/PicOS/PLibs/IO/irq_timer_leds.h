/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_irq_timer_leds_h
#define __pg_irq_timer_leds_h

	if (__pi_systat.ledsts) {
		// Some leds are supposed to blink
		if (__pi_systat.ledblc++ == 0) {
			if (__pi_systat.ledblk) {
				if (__pi_systat.ledsts & 0x1)
					LED0_ON;
				if (__pi_systat.ledsts & 0x2)
					LED1_ON;
				if (__pi_systat.ledsts & 0x4)
					LED2_ON;
				if (__pi_systat.ledsts & 0x8)
					LED3_ON;
				__pi_systat.ledblk = 0;
			} else {
				if (__pi_systat.ledsts & 0x1)
					LED0_OFF;
				if (__pi_systat.ledsts & 0x2)
					LED1_OFF;
				if (__pi_systat.ledsts & 0x4)
					LED2_OFF;
				if (__pi_systat.ledsts & 0x8)
					LED3_OFF;
				__pi_systat.ledblk = 1;
			}
			if (__pi_systat.fstblk)
				__pi_systat.ledblc = 200;
		}
		TCI_MARK_AUXILIARY_TIMER_ACTIVE;
	}
#endif
