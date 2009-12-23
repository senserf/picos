#ifndef	__ab_c__
#define	__ab_c__

#include "sysio.h"
#include "ab.h"
#include "ab_params.h"

// ============================================================================

int ab_outf (word st, const char *fm, ...) {
//
// Send a message
//
	word ln;
	va_list ap;

	if (ab_xrs_sln != 0 || ab_xrs_mod == 0) {
		// Off or busy
		when (AB_EVENT_OUT, st);
		release;
	}

	va_start (ap, fm);

	if ((ab_xrs_cou = vform (NULL, fm, ap)) == NULL) {
		// Out of memory
		umwait (st);
		release;
	}

	// Need one more for the sentinel
	if ((ln = strlen (ab_xrs_cou) + 1) > ab_xrs_max) {
		// Too long
		ufree (ab_xrs_cou);
		return ERROR;
	}

	ab_xrs_sln = (byte) ln;
	ab_xrs_new = AB_XTRIES;
	ptrigger (ab_xrs_han, AB_EVENT_RUN);
	return 0;
}

int ab_out (word st, char *str) {
//
// Send a formatted message; the string is assumed to have been malloc'ed
//
	word ln;

	if (ab_xrs_sln != 0 || ab_xrs_mod == 0) {
		// Off or busy
		when (AB_EVENT_OUT, st);
		release;
	}

	if ((ln = strlen (str) + 1) > ab_xrs_max) {
		// Should we rather keep it?
		ufree (str);
		return ERROR;
	}

	ab_xrs_sln = (byte) ln;
	ab_xrs_cou = str;
	ab_xrs_new = AB_XTRIES;
	ptrigger (ab_xrs_han, AB_EVENT_RUN);
	return 0;
}

int ab_inf (word st, const char *fm, ...) {
//
// Receive a message
//
	va_list ap;
	char *lin;
	int res;

	lin = ab_in (st);
	va_start (ap, fm);
	res = vscan (lin, fm, ap);
	ufree (lin);
	return res;
}

char *ab_in (word st) {
//
// Raw receive (have to clean up yourself)
//
	char *res;

	if (ab_xrs_cin == NULL) {
		when (AB_EVENT_IN, st);
		release;
	}

	res = ab_xrs_cin;
	ab_xrs_cin = NULL;
	ptrigger (ab_xrs_han, AB_EVENT_RUN);
	return res;
}

#endif
