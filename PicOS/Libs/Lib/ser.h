#ifndef	__pg_ser_h
#define	__pg_ser_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

//+++ "ser_out.c" "ser_outb.c" "ser_in.c" "ser_select.c"

int ser_out (word, const char*);
int ser_in (word, char*, int);
int ser_outb (word, const char*);

#if UART_DRIVER > 1
int ser_select (int);
#else
#ifndef ser_select
#define	ser_select(a)	CNOP
#endif
#endif

#endif
