#ifndef	__pg_sht_xx_h
#define	__pg_sht_xx_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sht_xx_sys.h"
//+++ "sht_xx.c"

#define	SHTXX_TEMP	0
#define	SHTXX_HUMID	1

#define	SHTXX_CMD_TEMP	0x03	// Read temperature
#define	SHTXX_CMD_HUMID	0x05	// Read humidity
#define	SHTXX_CMD_RESET	0x1E	// Soft reset
#define	SHTXX_CMD_WSR	0x06	// Write status register
#define	SHTXX_CMD_RSR	0x07	// Read status register

#define	SHTXX_DELAY_TEMP	210	// milliseconds (measurement delay)
#define	SHTXX_DELAY_HUMID	55

void shtxx_temp (word, word, address);
void shtxx_humid (word, word, address);
void shtxx_init ();

#endif
