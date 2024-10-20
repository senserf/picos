/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_cma3000_h
#define	__pg_cma3000_h

#ifndef	__SMURPH__

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

#else

#define	cma3000_reg(a)		0
#define	cma3000_off()		emul (9, "CMA3000_OFF")
#define	cma3000_on(a,b,c)	emul (9, "CMA3000_ON: %1d %1d %1d", a, b, c)
#define	cma3000_on_auto()	emul (9, "CMA3000_ON_AUTO")

#endif

#endif
