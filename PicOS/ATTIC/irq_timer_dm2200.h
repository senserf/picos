#ifndef __pg_irq_timer_dm2200_h
#define __pg_irq_timer_dm2200_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#if	PULSE_MONITOR

	if (pmon.deb_mas) {
		if (--(pmon.deb_mas) == 0) {
			// Master debouncer goes off
			if (pmon.deb_cnt) {
				if (--(pmon.deb_cnt) == 0)
					// Trigger counter interrupt
					pin_trigger_cnt;
				else
					pmon.deb_mas = PMON_DEBOUNCE_UNIT;
			
			}
			if (pmon.deb_not) {
				if (--(pmon.deb_not) == 0)
					pin_trigger_not;
				else
					pmon.deb_mas = PMON_DEBOUNCE_UNIT;
			}
			// Pending notifications
			if (pmon.stat &
			    (PMON_CMP_PENDING || PMON_NOT_PENDING)) {
					pmon.deb_mas = PMON_RETRY_DELAY;
					if ((pmon.stat & PMON_CMP_PENDING))
						i_trigger (ETYPE_USER,
							PMON_CNTEVENT);
					if ((pmon.stat & PMON_NOT_PENDING))
						i_trigger (ETYPE_USER,
							PMON_NOTEVENT);
					if (pmon.deb_mas == 0)
						pmon.deb_mas = PMON_RETRY_DELAY;
			}
		}
	}
#endif

#endif
