#ifndef __pg_irq_pins_h
#define __pg_irq_pins_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef	PULSE_MONITOR
/*
 * COUNTER and NOTIFIER pin interrupts. Depending on the edge selection,
 * the "ON" signal can be high (edge UP) or low (edge down).
 * The debouncing works this way: The signal must be ON for xxx_DEBOUNCE_ON
 * milliseconds to trigger the event. Then it must be OFF for at least
 * xxx_DEBOUNCE_OFF milliseconds before we start looking for another cycle.
 */
    if (pin_interrupt ()) {

	if (pin_int_cnt ()) {

		pin_clrint_cnt ();

		switch (pmon.state_cnt) {

		    case PCS_WPULSE:

			if (!pin_vedge_cnt ())
				// Spurious? Must be ON.
				goto WCon;

			wait_cnt_off (PCS_WENDP, PMON_DEBOUNCE_CNT_ON);
			break;

		    case PCS_WENDP:

			if (pin_vedge_cnt ()) {
				// Signal has gone OFF. Too short: ignore.
WCon:
				wait_cnt_on (PCS_WPULSE, 0);
				break;
			}

			// Duration OK - trigger the counter update
			update_cnt;
			update_cmp;
WCoff:
			wait_cnt_off (PCS_WECYC, 0);
			break;

		    case PCS_WECYC:

			// End of cycle
			wait_cnt_on (PCS_WNEWC, PMON_DEBOUNCE_CNT_OFF);
			break;

		    case PCS_WNEWC:

			if (pin_vedge_cnt ())
				goto WCoff;
			goto WCon;
		}
	}

	// =============================================================== //
	// =============================================================== //

	if (pin_int_not ()) {

		pin_clrint_not ();

		switch (pmon.state_not) {

		    case PCS_WPULSE:

			if (!pin_vedge_not ())
				// Spurious?
				goto WNon;

			// Waiting for a pulse
			wait_not_off (PCS_WENDP, PMON_DEBOUNCE_NOT_ON);
			break;

		    case PCS_WENDP:

			if (pin_vedge_not ()) {
				// Signal has gone OFF. Too short: ignore.
WNon:
				wait_not_on (PCS_WPULSE, 0);
				break;
			}

			// Duration OK - trigger the counter update
			update_not;
			update_pnt;
WNoff:
			wait_not_off (PCS_WECYC, 0);
			break;

		    case PCS_WECYC:

			// End of cycle
			wait_not_on (PCS_WNEWC, PMON_DEBOUNCE_NOT_OFF);
			break;

		    case PCS_WNEWC:

			if (pin_vedge_not ())
				// Too short
				goto WNoff;
			goto WNon;
		}
	}
			
	// =============================================================== //
    }
#endif	/* PULSE_MONITOR */

#endif
