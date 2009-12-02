#ifndef	__abb_c__
#define	__abb_c__

#include "sysio.h"
#include "abb.h"
#include "ab_params.h"

// ============================================================================

int abb_out (word st, address buf, word ln) {
//
// Send a binary message
//
	if (ab_xrs_sln != 0 || ab_xrs_mod == 0) {
		// Off or busy
		when (AB_EVENT_OUT, st);
		release;
	}

	if (ln > ab_xrs_max) {
		// Should we rather keep it?
		ufree (buf);
		return ERROR;
	}

	ab_xrs_sln = (byte) ln;
	ab_xrs_cou = (char*)buf;
	ab_xrs_new = AB_XTRIES;
	ptrigger (ab_xrs_han, AB_EVENT_RUN);
	return 0;
}

address abb_in (word st, word *ln) {
//
// Binary receive
//
	address res;

	if (ab_xrs_cin == NULL) {
		when (AB_EVENT_IN, st);
		release;
	}

	res = (address) ab_xrs_cin;
	*ln = (word) ab_xrs_rln;
	ab_xrs_cin = NULL;
	ptrigger (ab_xrs_han, AB_EVENT_RUN);
	return res;
}

#endif
