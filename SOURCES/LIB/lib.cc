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

#include "../version.h"
#include "lib.h"
#include "syshdr.h"

#define	NFMTBUF	16	// Number of buffers for form
#define	FMINISZ	64	// Initial size

static	char	*fmtbuf [NFMTBUF];
static	int	fmtbsiz [NFMTBUF];
static	int	cbi = 0;

char *vform (const char *f, va_list pmts) {

	int rs;
	char *tar;
	va_list psave;

	if (cbi == NFMTBUF)
		cbi = 0;

	if (fmtbuf [cbi] == NULL) {
		// Allocate initial buffer
		fmtbuf [cbi] = (char*) malloc (FMINISZ);
		fmtbsiz [cbi] = FMINISZ;
	}
	va_copy (psave, pmts);

	while ((rs = vsnprintf (fmtbuf [cbi], (size_t)(fmtbsiz [cbi]), f, pmts))
	    >= fmtbsiz [cbi]) {
		// Reallocate
		fmtbuf [cbi] = (char*) realloc (fmtbuf [cbi], rs + 1);
		fmtbsiz [cbi] = rs + 1;
		va_copy (pmts, psave);
	}

	tar = fmtbuf [cbi];
	cbi++;

	return tar;
}
		
char *form (const char *f, ...) {

	va_list pmts;
	va_start (pmts, f);
	return vform (f, pmts);
}
