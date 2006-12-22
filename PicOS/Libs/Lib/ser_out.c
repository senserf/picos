/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

//+++ "__outserial.c"

int __outserial (word, address);

int ser_out (word st, const char *m) {

	int prcs;
	char *buf;

	if ((prcs = running (__outserial)) != 0) {
		/* We have to wait */
		if (st == NONE)
			return prcs;
		join (prcs, st);
		release;
	}

	if (*m)
		prcs = strlen (m) +1;
	else
		prcs =  m[1] + 3;

	if ((buf = (char*) umalloc (prcs)) == NULL) {
		/*
		 * We have to wait for memory
		 */
		if (st == NONE)
			return NONE;
		umwait (st);
		release;
	}
	if (*m)
		strcpy (buf, m);
	else
		memcpy (buf, m, prcs);
	runstrand (__outserial, buf);
	return 0;
}
