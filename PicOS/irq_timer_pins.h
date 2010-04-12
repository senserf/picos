#ifndef __pg_irq_timer_pins_h
#define __pg_irq_timer_pins_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef	PULSE_MONITOR

#if	MONITOR_PINS_SEND_INTERRUPTS
	if (pmon.deb_mas) {
		if (--(pmon.deb_mas) == 0) {
			// Master debouncer goes off
			if (pmon.deb_cnt) {
				if (--(pmon.deb_cnt) == 0)
					// Trigger counter interrupt
					pin_trigger_cnt ();
				else
					pmon.deb_mas = PMON_DEBOUNCE_UNIT;
			
			}
			if (pmon.deb_not) {
				if (--(pmon.deb_not) == 0)
					pin_trigger_not ();
				else
					pmon.deb_mas = PMON_DEBOUNCE_UNIT;
			}
			// Pending notifications
			if (pmon.stat &
			    (PMON_CMP_PENDING | PMON_NOT_PENDING)) {
					RISE_N_SHINE;
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
		TCI_MARK_AUXILIARY_TIMER_ACTIVE;
	}
#else	/* MONITOR_PINS_SEND_INTERRUPTS */

	if (pmon.deb_mas == 0) {
		pmon.deb_mas = PMON_DEBOUNCE_UNIT-1;
		switch (pmon.state_cnt) {
			case PCS_WPULSE:
				if (pin_vedge_cnt ()) {
					// Gone up
					if ((pmon.stat & PMON_CNT_ON) == 0)
						// Ignore
						break;
					pmon.state_cnt = PCS_WENDP;
					pmon.deb_cnt = PMON_DEBOUNCE_CNT_ON;
				}
				break;
			case PCS_WENDP:
				if (!pin_vedge_cnt ()) {
					// Disappeared - ignore it
					pmon.state_cnt = PCS_WPULSE;
				} else if (--(pmon.deb_cnt) == 0) {
					// Trigger
					update_cnt;
					pmon.state_cnt = PCS_WECYC;
				}
				break;
			case PCS_WECYC:
				if (!pin_vedge_cnt ())
					pmon.state_cnt = PCS_WPULSE;
				break;
		}
		switch (pmon.state_not) {
			case PCS_WPULSE:
				if (pin_vedge_not ()) {
					// Gone up
					if ((pmon.stat & PMON_NOT_ON) == 0)
						// Ignore
						break;
					pmon.state_not = PCS_WENDP;
					pmon.deb_not = PMON_DEBOUNCE_NOT_ON;
				}
				break;
			case PCS_WENDP:
				if (!pin_vedge_not ()) {
					// Disappeared - ignore
					pmon.state_not = PCS_WPULSE;
				} else if (--(pmon.deb_not) == 0) {
					update_not;
					pmon.state_not = PCS_WECYC;
				}
				break;
			case PCS_WECYC:
				if (!pin_vedge_not ()) {
					pmon.state_not = PCS_WNEWC;
					pmon.deb_not = PMON_DEBOUNCE_NOT_OFF;
				}
				break;
			case PCS_WNEWC:
				if (pin_vedge_not ()) {
					// Ignore
					pmon.state_not = PCS_WECYC;
				} else if (--(pmon.deb_not) == 0) {
					pmon.state_not = PCS_WPULSE;
				}
				break;
		}
		// Pending notifications
		if (pmon.stat & (PMON_CMP_PENDING | PMON_NOT_PENDING)) {
			RISE_N_SHINE;
			if ((pmon.stat & PMON_CMP_PENDING))
				i_trigger (ETYPE_USER, PMON_CNTEVENT);
			if ((pmon.stat & PMON_NOT_PENDING))
				i_trigger (ETYPE_USER, PMON_NOTEVENT);
		}
	} else {
		--(pmon.deb_mas);
	}

	// Must be kept active constantly if monitor pins send no interrupts
	TCI_MARK_AUXILIARY_TIMER_ACTIVE;

#endif	/* MONITOR_PINS_SEND_INTERRUPTS */

#endif	/* PULSE_MONITOR */

#endif
