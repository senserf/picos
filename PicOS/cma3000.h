#ifndef	__pg_cma3000_h
#define	__pg_cma3000_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "cma3000_sys.h"
//+++ "cma3000.c"

extern Boolean cma3000_wait_pending;
extern char cma3000_accdata [4];

byte cma3000_rreg (byte);
void cma3000_off ();
void cma3000_read (word, const byte*, address);

void cma3000_on (byte md, byte th, byte tm);
//
//	md: 0 - motion detection, 1 - free fall (400 Hz)
//	th: threshold
//	tm: time bracket
//

void cma3000_on_auto ();

#endif
