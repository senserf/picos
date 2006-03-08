#ifndef	__pg_phys_dm2200_h
#define	__pg_phys_dm2200_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "phys_dm2200.c"

void phys_dm2200 (int, int);
int pin_get_adc (word, word, word, word);
word pin_get (word);
word pin_set (word, word);
void pin_wait (word, word);

#endif
