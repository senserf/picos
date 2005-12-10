#ifndef __pg_irq_timer_leds_h
#define __pg_irq_timer_leds_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

	if (zz_systat.ledsts) {
		// Some leds are supposed to blink
		if (zz_systat.ledblc++ == 0) {
			if (zz_systat.ledblk) {
				if (zz_systat.ledsts & 0x1)
					LED0_ON;
				if (zz_systat.ledsts & 0x2)
					LED1_ON;
				if (zz_systat.ledsts & 0x4)
					LED2_ON;
				if (zz_systat.ledsts & 0x8)
					LED3_ON;
				zz_systat.ledblk = 0;
			} else {
				if (zz_systat.ledsts & 0x1)
					LED0_OFF;
				if (zz_systat.ledsts & 0x2)
					LED1_OFF;
				if (zz_systat.ledsts & 0x4)
					LED2_OFF;
				if (zz_systat.ledsts & 0x8)
					LED3_OFF;
				zz_systat.ledblk = 1;
			}
			if (zz_systat.fstblk)
				zz_systat.ledblc = 200;
		}
	}
#endif
