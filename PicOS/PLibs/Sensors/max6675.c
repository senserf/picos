/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "max6675.h"

void max6675_read (word st, const byte *junk, address val) {
//
// MAX6675 thermocouple read
//
	word res;
	int i;

	max6675_csel;

	for (res = 0, i = 0; i < 16; i++) {
		max6675_clkh;
		res <<= 1;
		if (max6675_data)
			res |= 1;
		max6675_clkl;
	}

	max6675_cunsel;

	*val = ((res & 0x4)) ? WNONE : (res >> 3);
}
