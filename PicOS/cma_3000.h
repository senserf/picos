#ifndef	__pg_cma_3000_h
#define	__pg_cma_3000_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "cma_3000_sys.h"
//+++ "cma_3000.c"

extern int __pi_cma_3000_event_thread;

void cma_3000_on (word);
void cma_3000_on_md (), cma_3000_on_me ();
void cma_3000_off ();
void cma_3000_read (word, const byte*, address);

#endif
