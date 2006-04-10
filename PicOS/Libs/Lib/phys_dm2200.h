#ifndef	__pg_phys_dm2200_h
#define	__pg_phys_dm2200_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "phys_dm2200.c"

void phys_dm2200 (int, int);
word pin_read (word);
void pin_write (word pin, word val);
int pin_read_adc (word, word, word, word);

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

extern	volatile zz_pmon_t	zz_pmon;

#define	PMON_CNTEVENT	((word)&(zz_pmon.deb_mas))
#define	PMON_CMPEVENT	PMON_CNTEVENT
#define	PMON_NOTEVENT	((word)&(zz_pmon.deb_not))

#define	PMON_STATE_NOT_RISING	0x01
#define	PMON_STATE_NOT_ON	0x02
#define	PMON_STATE_NOT_PENDING	0x04
#define	PMON_STATE_CNT_RISING	0x10
#define	PMON_STATE_CNT_ON	0x20
#define	PMON_STATE_CMP_ON	0x40
#define	PMON_STATE_CMP_PENDING	0x80

void pmon_init_cnt (bool);
void pmon_start_cnt (long);
void pmon_stop_cnt ();
void pmon_set_cmp (long);
lword pmon_get_cnt ();
lword pmon_get_cmp ();
void pmon_init_not (bool);
void pmon_start_not ();
void pmon_stop_not ();
word pmon_get_state ();
bool pmon_pending_not ();
bool pmon_pending_cnt ();

#endif	/* PULSE_MONITOR */

#endif
