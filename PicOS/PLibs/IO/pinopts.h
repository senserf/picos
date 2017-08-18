#ifndef	__pg_pinopts_h
#define	__pg_pinopts_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "pins.h"

//+++ "pin_read.c"

word pin_read (word);
int pin_write (word, word);
int pin_read_adc (word, word, word, word);
int pin_write_dac (word, word, word);

#ifdef	PULSE_MONITOR

void pmon_start_cnt (lint, Boolean);
void pmon_stop_cnt ();
void pmon_set_cmp (lint);
lword pmon_get_cnt ();
lword pmon_get_cmp ();
void pmon_start_not (Boolean);
void pmon_stop_not ();
word pmon_get_state ();
Boolean pmon_pending_not ();
Boolean pmon_pending_cmp ();
void pmon_dec_cnt ();
void pmon_sub_cnt (lint);
void pmon_add_cmp (lint);

#endif	/* PULSE_MONITOR */

#endif
