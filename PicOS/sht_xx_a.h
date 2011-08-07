#ifndef	__pg_sht_xx_a_h
#define	__pg_sht_xx_a_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// SHT sensor, array version

#include "sht_xx_sys.h"
//+++ "sht_xx_a.c"

#include "sht_xx_cmd.h"

void shtxx_a_temp (word, const byte*, address);
void shtxx_a_humid (word, const byte*, address);
void shtxx_a_init (void);

#endif
