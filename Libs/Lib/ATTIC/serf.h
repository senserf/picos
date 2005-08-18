#ifndef	__pg_serf_h
#define	__pg_serf_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "form.h"

//+++ "ser_outf.c" "ser_inf.c" "ser_select.c"

int ser_outf (word, const char*, ...);
int ser_inf (word, const char*, ...);
int ser_select (int);

#endif
