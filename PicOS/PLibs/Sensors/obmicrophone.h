#ifndef __pg_obmicrophone_h
#define	__pg_obmicrophone_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2017                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

// This is a simple driver of one-bit microphone to sense the noise level

typedef struct {
//
// Sensor value: number of samples from last reset + level
//
	lword	nsamples;
	lword 	amplitude;
} obmicrophone_data_t;

#define	OBMICROPHONE_MINRATE	1000	// 100 kHz
#define	OBMICROPHONE_MAXRATE	24750	// 2.475 MHz

#ifndef __SMURPH__

#include "pins.h"
//+++ "obmicrophone.c"

void obmicrophone_on (word);
void obmicrophone_off ();
void obmicrophone_read (word, const byte*, address);
void obmicrophone_reset ();

#else	/* __SMURPH__ */

#define	obmicrophone_on(a)	emul (9, "OBMICROPHONE_ON: %d00", a, b)
#define	obmicrophone_off()	emul (9, "OBMICROPHONE_OFF: <>")
#define	obmicrophone_reset()	emul (9, "OBMICROPHONE_RESET: <>")

#endif	/* __SMURPH__ */

#endif
