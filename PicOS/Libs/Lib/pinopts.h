#ifndef	__pg_pinopts_h
#define	__pg_pinopts_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "pin_read.c"
#include "board_pins.h"

word pin_read (word);
int pin_write (word, word);
int pin_read_adc (word, word, word, word);
int pin_write_dac (word, word, word);

#ifdef	PULSE_MONITOR

#define	PMON_STATE_NOT_RISING	0x01
#define	PMON_STATE_NOT_ON	0x02
#define	PMON_STATE_NOT_PENDING	0x04
#define	PMON_STATE_CNT_RISING	0x10
#define	PMON_STATE_CNT_ON	0x20
#define	PMON_STATE_CMP_ON	0x40
#define	PMON_STATE_CMP_PENDING	0x80

void pmon_start_cnt (long, bool);
void pmon_stop_cnt ();
void pmon_set_cmp (long);
lword pmon_get_cnt ();
lword pmon_get_cmp ();
void pmon_start_not (bool);
void pmon_stop_not ();
word pmon_get_state ();
bool pmon_pending_not ();
bool pmon_pending_cmp ();
void pmon_dec_cnt ();
void pmon_sub_cnt (long);
void pmon_add_cmp (long);

extern	word zz_pmonevent;
#define	PMON_NOTEVENT	((word)&zz_pmonevent)
#define	PMON_CNTEVENT	((word)&zz_pmonevent + 1)
#define	PMON_CMPEVENT	PMON_CNTEVENT

#endif	/* PULSE_MONITOR */

#endif
