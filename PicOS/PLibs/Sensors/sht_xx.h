#ifndef	__pg_sht_xx_h
#define	__pg_sht_xx_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sht_xx_sys.h"
//+++ "sht_xx.c"

#include "sht_xx_cmd.h"

void shtxx_temp (word, const byte*, address);
void shtxx_humid (word, const byte*, address);
void shtxx_init (void);

#endif
