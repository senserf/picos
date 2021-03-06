/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "abb.h"
#include "ab_params.h"

// ============================================================================

byte *abb_outf (word st, word ln) {
//
// Send a binary message with internally allocated buffer
//
	if (ab_xrs_sln != 0 || ab_xrs_mod == 0) {
		// Off or busy
		if (st == WNONE)
			return NULL;
		when (AB_EVENT_OUT, st);
		release;
	}

	if (ln > ab_xrs_max)
		syserror (EREQPAR, "abb_outf");

	if ((ab_xrs_cou = (char*) umalloc (ln)) == NULL) {
		if (st == WNONE)
			return NULL;
		umwait (st);
		release;
	}

	ab_xrs_sln = (byte) ln;
	ab_xrs_new = AB_XTRIES;
	ptrigger (ab_xrs_han, AB_EVENT_RUN);
	return (byte*) ab_xrs_cou;
}

byte *abb_out (word st, byte *buf, word ln) {
//
// Send a binary message with user-allocated buffer
//
	if (ln > ab_xrs_max || buf == NULL)
		syserror (EREQPAR, "abb_out");

	if (ab_xrs_sln != 0 || ab_xrs_mod == 0) {
		// Off or busy
		if (st == WNONE)
			return NULL;
		when (AB_EVENT_OUT, st);
		release;
	}

	ab_xrs_cou = (char*) buf;
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
		if (st == WNONE)
			return NULL;
		when (AB_EVENT_IN, st);
		release;
	}

	res = (byte*) ab_xrs_cin;
	*ln = (word) ab_xrs_rln;
	ab_xrs_cin = NULL;
	ptrigger (ab_xrs_han, AB_EVENT_RUN);
	return res;
}
