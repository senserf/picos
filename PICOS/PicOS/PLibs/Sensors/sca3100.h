/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_sca3100_h
#define	__pg_sca3100_h

#ifndef	__SMURPH__

#include "sca3100_sys.h"
//+++ "sca3100.c"

void sca3100_off ();
void sca3100_on ();

void sca3100_read (word, const byte*, address);

#else

#define	sca3100_off()		emul (9, "SCA3100_OFF")
#define	sca3100_on()		emul (9, "SCA3100_ON")

#endif

#endif
