#ifndef	__pg_cma_3000_h
#define	__pg_cma_3000_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "cma_3000_sys.h"
//+++ "cma_3000.c"

extern int zz_cma_3000_event_thread;

void cma_3000_on ();
void cma_3000_off ();
void cma_3000_read (word, const byte*, address);

#endif