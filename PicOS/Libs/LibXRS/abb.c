#ifndef	__abb_c__
#define	__abb_c__

#include "sysio.h"
#include "abb.h"
#include "ab_params.h"

// ============================================================================

byte *abb_out (word st, word ln) {
//
// Send a binary message
//
	if (ln > ab_xrs_max)
		syserror (EREQPAR, "abb_out");

	if (ab_xrs_sln != 0 || ab_xrs_mod == 0) {
		// Off or busy
		when (AB_EVENT_OUT, st);
		release;
	}

	if ((ab_xrs_cou = (char*) umalloc (ln)) == NULL) {
		umwait (st);
		release;
	}

	ab_xrs_sln = (byte) ln;
	ab_xrs_new = AB_XTRIES;
	ptrigger (ab_xrs_han, AB_EVENT_RUN);
	return (byte*) ab_xrs_cou;
}

byte *abb_in (word st, word *ln) {
//
// Binary receive
//
	byte *res;

	if (ab_xrs_cin == NULL) {
		when (AB_EVENT_IN, st);
		release;
	}

	res = (byte*) ab_xrs_cin;
	*ln = (word) ab_xrs_rln;
	ab_xrs_cin = NULL;
	ptrigger (ab_xrs_han, AB_EVENT_RUN);
	return res;
}

#endif
