/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

//+++ "__outserial.c"

procname (__outserial);

int ser_outb (word st, const char *m) {

	int prcs;

	if ((prcs = running (__outserial)) != 0) {
		join (prcs, st);
		release;
	}

	if (runstrand (__outserial, m) == 0) {
		ufree (m);
		npwait (st);
		release;
	}

	return 0;
}
