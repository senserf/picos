#include "sysio.h"
#include "max6675.h"
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

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
