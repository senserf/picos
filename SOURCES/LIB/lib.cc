/* oooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-06  P. Gburzynski */
/* oooooooooooooooooooooooooooooooooooo */

/* --- */

#include "../version.h"
#include "lib.h"
#include "syshdr.h"

#define	NFMTBUF	4	// Number of buffers for form
#define	FMINISZ	64	// Initial size

static	char	*fmtbuf [NFMTBUF];
static	int	fmtbsiz [NFMTBUF];
static	int	cbi = 0;

char *vform (const char *f, va_list pmts) {

	int rs;
	char *tar;

	if (cbi == NFMTBUF)
		cbi = 0;

	if (fmtbuf [cbi] == NULL) {
		// Allocate initial buffer
		fmtbuf [cbi] = (char*) malloc (FMINISZ);
		fmtbsiz [cbi] = FMINISZ;
	}
Redo:
	rs = vsnprintf (fmtbuf [cbi], fmtbsiz [cbi], f, pmts);
	if (rs >= fmtbsiz [cbi]) {
		// Reallocate
		fmtbuf [cbi] = (char*) realloc (fmtbuf [cbi], rs + 1);
		fmtbsiz [cbi] = rs + 1;
		goto Redo;
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
