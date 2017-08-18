#ifndef	__pg_max6675_h
#define	__pg_max6675_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "max6675_sys.h"
//+++ "max6675.c"

#define max6675_on()	max6675_bring_up
#define max6675_off()	max6675_bring_down

void max6675_read (word, const byte*, address);

#endif
