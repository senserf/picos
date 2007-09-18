/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

//+++ "__outserial.c"

int __outserial (word, address);

int ser_outb (word st, const char *m) {

	int prcs;

	if (m == NULL)
		return 0;

	if ((prcs = running (__outserial)) != 0) {
		if (st == NONE)
			return prcs;
		join (prcs, st);
		release;
	}
	if (runstrand (__outserial, m) == 0)
		// This means that fork has failed; we should deallocate the
		// buffer, which otherwise __outserial would have done
		ufree (m);

	return 0;
}
