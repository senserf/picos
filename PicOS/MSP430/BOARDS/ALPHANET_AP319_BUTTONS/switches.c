/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "switches.h"

void read_switches (switches_t *s) {
//
//
	byte a;

	_BIS (P3REN, 0xFF);
	udelay (100);
	a = ~P3IN;
	_BIC (P3REN, 0xFF);

	s->S0 = a >> 4;
	s->S1 = a & 0x0F;

	// ========================

	_BIS (P5REN, 0x7F);
	udelay (100);
	a = ~P5IN;
	_BIC (P5REN, 0x7F);

	s->S2 = a & 0x3F;
	s->S3 = (a & 0x40) >> 4;

	// ========================

	_BIS (P1REN, 0x90);
	udelay (100);
	a = ~P1IN;
	_BIC (P1REN, 0x90);

	s->S3 |= ((a & 0x10) >> 4) | ((a & 0x80) >> 6);
}
