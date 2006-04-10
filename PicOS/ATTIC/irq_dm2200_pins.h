#ifndef __pg_irq_dm2200_pins_h
#define __pg_irq_dm2200_pins_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	wait_cnt(st,deb,on) \
	do { \
		pmon.deb_cnt = (deb); \
		if (deb) \
			pmon.deb_mas = PMON_DEBOUNCE_UNIT; \
		if (on) \
			pin_setedge_cnt; \
		else \
			pin_revedge_cnt; \
		pmon.state_cnt = (st); \
		pin_clrint_cnt; \
		if (pin_vedge_cnt) \
			pin_trigger_cnt; \
	} while (0)

#define	wait_cnt_on(st,deb)	wait_cnt (st, deb, 1)
#define	wait_cnt_off(st,deb)	wait_cnt (st, deb, 0)

#define	update_cnt \
	do { \
		if (++(pmon.cnt [0]) == 0) \
			if (++(pmon.cnt [1]) == 0) \
				++(pmon.cnt [2]); \
		if ((pmon.stat & PMON_CMP_ON) && \
		     pmon.cnt [0] == pmon.cmp [0] && \
		     pmon.cnt [1] == pmon.cmp [1] && \
		     pmon.cnt [2] == pmon.cmp [2] ) \
			_BIS (pmon.stat, PMON_CMP_PENDING); \
		if ((pmon.stat & PMON_CMP_PENDING)) { \
			RISE_N_SHINE; \
			i_trigger (ETYPE_USER, PMON_CNTEVENT); \
			if (pmon.deb_mas == 0) \
				pmon.deb_mas = PMON_RETRY_DELAY; \
		} \
	} while (0)

	// =============================================================== //

/*
 * P2 interrupts for dm2200 pins: COUNTER and NOTIFIER. Depending on the
 * edge selection, the "ON" signal can be high (edge UP) or low (edge down).
 * The debouncing works this way: The signal must be ON for xxx_DEBOUNCE_ON
 * milliseconds to trigger the event. Then it must be OFF for at least
 * xxx_DEBOUNCE_OFF milliseconds before we start looking for another cycle.
 */
	if (pin_int_cnt) {

		pin_clrint_cnt;

		switch (pmon.state_cnt) {

		    case PCS_WPULSE:

			if (!pin_vedge_cnt)
				// Spurious? Must be ON.
				goto WCon;

			wait_cnt_off (PCS_WENDP, PMON_DEBOUNCE_CNT_ON);
			break;

		    case PCS_WENDP:

			if (pin_vedge_cnt) {
				// Signal has gone OFF. Too short: ignore.
WCon:
				wait_cnt_on (PCS_WPULSE, 0);
				break;
			}

			// Duration OK - trigger the counter update
			update_cnt;
WCoff:
			wait_cnt_off (PCS_WECYC, 0);
			break;

		    case PCS_WECYC:

			// End of cycle
			wait_cnt_on (PCS_WNEWC, PMON_DEBOUNCE_CNT_OFF);
			break;

		    case PCS_WNEWC:

			if (pin_vedge_cnt)
				goto WCoff;
			goto WCon;
		}
	}

	// =============================================================== //

#define	wait_not(st,deb,on) \
	do { \
		pmon.deb_not = (deb); \
		if (deb) \
			pmon.deb_mas = PMON_DEBOUNCE_UNIT; \
		if (on) \
			pin_setedge_not; \
		else \
			pin_revedge_not; \
		pmon.state_not = (st); \
		pin_clrint_not; \
		if (pin_vedge_not) \
			pin_trigger_not; \
	} while (0)

#define	wait_not_on(st,deb)	wait_not (st, deb, 1)
#define	wait_not_off(st,deb)	wait_not (st, deb, 0)

#define	update_not \
	do { \
		_BIS (pmon.stat, PMON_NOT_PENDING); \
		i_trigger (ETYPE_USER, PMON_NOTEVENT); \
		if (pmon.deb_mas == 0) \
			pmon.deb_mas = PMON_RETRY_DELAY; \
	} while (0)

	// =============================================================== //

	if (pin_int_not) {

		pin_clrint_not;

		switch (pmon.state_not) {

		    case PCS_WPULSE:

			if (!pin_vedge_not)
				// Spurious?
				goto WNon;

			// Waiting for a pulse
			wait_not_off (PCS_WENDP, PMON_DEBOUNCE_NOT_ON);
			break;

		    case PCS_WENDP:

			if (pin_vedge_not) {
				// Signal has gone OFF. Too short: ignore.
WNon:
				wait_not_on (PCS_WPULSE, 0);
				break;
			}

			// Duration OK - trigger the counter update
			update_not;
WNoff:
			wait_not_off (PCS_WECYC, 0);
			break;

		    case PCS_WECYC:

			// End of cycle
			wait_not_on (PCS_WNEWC, PMON_DEBOUNCE_NOT_OFF);
			break;

		    case PCS_WNEWC:

			if (pin_vedge_not)
				// Too short
				goto WNoff;
			goto WNon;
		}
	}
			
	// =============================================================== //

#endif
