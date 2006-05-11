#ifndef	__pg_pins_h
#define	__pg_pins_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#if	PULSE_MONITOR

typedef	struct {

	byte	deb_mas,	// Master debounce timer
		deb_cnt,	// Counter debounce timer
		deb_not,	// Notifier debounce timer
		stat,		// Status bits
		state_cnt,	// Interrupt state (counter)
		state_not,	// Interrupt state (notifier)
		cnt [3],	// Counter
		cmp [3];	// Comparator
} zz_pmon_t;

extern	zz_pmon_t	zz_pmon;

#define	pmon		zz_pmon

#define	PMON_CNT_EDGE_UP	0x40	// Edge UP triggers counter
#define	PMON_NOT_EDGE_UP	0x80	// Edge UP triggers notifier
#define	PMON_CMP_ON		0x20	// Comparator is on
#define	PMON_CMP_PENDING	0x10	// Comparator event pending
#define	PMON_NOT_ON		0x08	// Notifier is on
#define	PMON_NOT_PENDING	0x04	// Notifier event pending
#define	PMON_CNT_ON		0x02	// Counter is on

#define	PCS_WPULSE		0	// Interrupt states
#define	PCS_WENDP		1
#define	PCS_WECYC		2
#define	PCS_WNEWC		3

#define	PMON_DEBOUNCE_UNIT	16	// 16 clock ticks
#define	PMON_RETRY_DELAY	255	// 1/4 sec (persistent status report)
#define	PMON_DEBOUNCE_CNT_ON	3	// 48 msec on
#define	PMON_DEBOUNCE_CNT_OFF	3	// 48 msec off
#define	PMON_DEBOUNCE_NOT_ON	4	// 64 msec on
#define	PMON_DEBOUNCE_NOT_OFF	100	// 2 sec off

/* ----------------------------------------
 * Used by pin interrupts and timer assists
 * ----------------------------------------
 */
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
	} while (0)

#define	update_cmp \
	do { \
		if ((pmon.stat & PMON_CMP_PENDING)) { \
			RISE_N_SHINE; \
			i_trigger (ETYPE_USER, PMON_CNTEVENT); \
			if (pmon.deb_mas == 0) \
				pmon.deb_mas = PMON_RETRY_DELAY; \
		} \
	} while (0)

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
		RISE_N_SHINE; \
		i_trigger (ETYPE_USER, PMON_NOTEVENT); \
	} while (0)

#define update_pnt \
	do { \
		if (pmon.deb_mas == 0) \
			pmon.deb_mas = PMON_RETRY_DELAY; \
	} while (0)

#endif  /* PULSE_MONITOR */

#include "pins_sys.h"

#endif
