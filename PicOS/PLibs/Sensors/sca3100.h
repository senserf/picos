#ifndef	__pg_sca3100_h
#define	__pg_sca3100_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sca3100_sys.h"
//+++ "sca3100.c"

void sca3100_off ();
void sca3100_on ();

void sca3100_read (word, const byte*, address);

#endif
