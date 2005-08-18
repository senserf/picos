#ifndef	__pg_phys_dm2100_h
#define	__pg_phys_dm2100_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "phys_dm2100.c"

void phys_dm2100 (int, int);
int pin_get_adc (word, word, word);
word pin_get (word);
word pin_set (word, word);
void pin_wait (word, word);

#endif
