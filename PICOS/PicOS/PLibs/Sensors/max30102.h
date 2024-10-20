/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_max30102_h
#define	__pg_max30102_h

#ifndef	MAX30102_SHORT_SAMPLES
#define	MAX30102_SHORT_SAMPLES	0
#endif

#if	MAX30102_SHORT_SAMPLES
typedef	word	max30102_sample_t;
#define	MAX_SAMPLE	MAX_WORD
#else
typedef	lword	max30102_sample_t;
#define	MAX_SAMPLE	MAX_LWORD
#endif

void max30102_start (word);
void max30102_stop ();
void max30102_wreg (byte, byte);
byte max30102_rreg (byte);

// state, red, inf
void max30102_read_sample (word, max30102_sample_t*, max30102_sample_t*);

//+++ "max30102.c"

#endif
