/* oooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-07  P. Gburzynski */
/* oooooooooooooooooooooooooooooooooooo */

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
