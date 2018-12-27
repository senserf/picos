/*
	Copyright 1995-2018, 2019 Pawel Gburzynski

	This file is part of SMURPH/SIDE.

	SMURPH/SIDE is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	SMURPH/SIDE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SMURPH/SIDE. If not, see <https://www.gnu.org/licenses/>.
*/

/* --- */

/* --------------------------------------------- */
/* Functions for single precision BIG arithmetic */
/* --------------------------------------------- */

#if     BIG_precision == 1

#include        "system.h"

BIG     BIG_0 = 0, BIG_1 = 1, BIG_inf = MAX_LONG;        // Constants

BIG  atob (const char *s) {             // String to BIG conversion

	BIG     r;

	while (*s == ' ') s++;
	if (*s == '+') s++;
	while (*s == ' ') s++;

	r = BIG_0;

	while ((*s <= '9') && (*s >= '0')) {
		r = r * 10 + (*s - '0');
		s++;
	}

	return (r);
}

static  char    *btoabuf = NULL;
static  int     btoable = 0;

char    *btoa (const BIG a, char *s, int nc) {

	if (s == NULL) {
		if (nc > 0 && btoable < nc) {
			if (btoable > 0) delete [] btoabuf;
			btoabuf = new char [(btoable = nc) + 1];
		}
		s = btoabuf;
	}

	encodeLong (a, s, nc);
	return (s);
}

#endif
