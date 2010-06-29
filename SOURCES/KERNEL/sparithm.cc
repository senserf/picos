/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-08   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

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
