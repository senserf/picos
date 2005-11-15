#ifndef	__pg_ser_h
#define	__pg_ser_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "ser_out.c" "ser_in.c" "ser_select.c"

int ser_out (word, const char*);
int ser_in (word, char*, int);
int ser_select (int);

#endif
