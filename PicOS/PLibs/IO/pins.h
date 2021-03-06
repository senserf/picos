/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_pins_h
#define	__pg_pins_h	1

#include "pins_sys.h"

#ifdef BUTTON_LIST
#include "buttons.h"
#endif

#ifdef	PULSE_MONITOR

#define	PMON_STATE_NOT_RISING	0x01
#define	PMON_STATE_NOT_ON	0x02
#define	PMON_STATE_NOT_PENDING	0x04
#define	PMON_STATE_CNT_RISING	0x10
#define	PMON_STATE_CNT_ON	0x20
#define	PMON_STATE_CMP_ON	0x40
#define	PMON_STATE_CMP_PENDING	0x80

typedef	struct {

	byte	deb_mas,	// Master debounce timer
		deb_cnt,	// Counter debounce timer
		deb_not,	// Notifier debounce timer
		stat,		// Status bits
		state_cnt,	// Interrupt state (counter)
		state_not,	// Interrupt state (notifier)
		cnt [3],	// Counter
		cmp [3];	// Comparator
} __pi_pmon_t;

extern	__pi_pmon_t	__pi_pmon;

#define	pmon		__pi_pmon

extern	word 	__pi_pmonevent [0];

#define	PMON_NOTEVENT	((word)&__pi_pmonevent)
#define	PMON_CNTEVENT	((word)&__pi_pmonevent + 1)
#define	PMON_CMPEVENT	PMON_CNTEVENT

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
#define	PMON_DEBOUNCE_NOT_OFF	100	// 1.6 sec off

/* ----------------------------------------
 * Used by pin interrupts and timer assists
 * ----------------------------------------
 */
void pins_int_wait_cnt (byte, byte, byte);

#define	wait_cnt_on(st,deb)	pins_int_wait_cnt (st, deb, 1)
#define	wait_cnt_off(st,deb)	pins_int_wait_cnt (st, deb, 0)

#if MONITOR_PINS_SEND_INTERRUPTS
#define	activate_deb_timer	TCI_RUN_AUXILIARY_TIMER
#else
// In this case, the timer never stops
#define	activate_deb_timer	CNOP
#endif

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
			i_trigger (PMON_CNTEVENT); \
			if (pmon.deb_mas == 0) { \
				activate_deb_timer; \
				pmon.deb_mas = PMON_RETRY_DELAY; \
			} \
		} \
	} while (0)

void pins_int_wait_not (byte, byte, byte);
//+++ "pin_read.c"

#define	wait_not_on(st,deb)	pins_int_wait_not (st, deb, 1)
#define	wait_not_off(st,deb)	pins_int_wait_not (st, deb, 0)

#define	update_not \
	do { \
		_BIS (pmon.stat, PMON_NOT_PENDING); \
		RISE_N_SHINE; \
		i_trigger (PMON_NOTEVENT); \
	} while (0)

#define update_pnt \
	do { \
		if (pmon.deb_mas == 0) { \
			activate_deb_timer; \
			pmon.deb_mas = PMON_RETRY_DELAY; \
		} \
	} while (0)

PULSE_MONITOR;

#endif  /* PULSE_MONITOR */

// Not sure if this is the best place. This code used to be in pins_sys.h, but
// it should rightfully be architecture independent, even if, at present, these
// protocols are implemented for CC1350 only.

// ============================================================================
#if I2C_INTERFACE

// To (re)initialize the I2C bus, the arguments are two pin numbers (scl, sda)
// + rate (low, high)
void __i2c_open (byte, byte, Boolean);
#define	__i2c_close()	__i2c_open (BNONE, 0, NO)
Boolean __i2c_op (byte, const byte*, lword, byte*, lword);

#endif
// ============================================================================
#if SSI_INTERFACE

// Interrupt function type
typedef	void (*__ssi_int_fun_t)(void);

// which [0,1] {clk, rx, tx, fs}, mode, rate, intfun
void __ssi_open (sint, const byte*, byte, word, __ssi_int_fun_t);
#define	__ssi_close(w) __ssi_open (w, NULL, 0, 0, NULL)

#endif
// ============================================================================

#endif
