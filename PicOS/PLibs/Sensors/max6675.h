/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_max6675_h
#define	__pg_max6675_h

#include "max6675_sys.h"
//+++ "max6675.c"

#define max6675_on()	max6675_bring_up
#define max6675_off()	max6675_bring_down

void max6675_read (word, const byte*, address);

#endif
