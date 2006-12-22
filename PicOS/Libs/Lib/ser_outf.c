/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "form.h"

//+++ "__outserial.c"

int __outserial (word, address);

int ser_outf (word st, const char *m, ...) {

	int prcs;
	char *buf;
	va_list ap;

	if ((prcs = running (__outserial)) != 0) {
		/* We have to wait */
		if (st == NONE)
			return prcs;
		join (prcs, st);
		release;
	}

	va_start (ap, m);

	if ((buf = vform (NULL, m, ap)) == NULL) {
		/*
		 * This means that we are out of memory
		 */
		if (st == NONE)
			return NONE;
		umwait (st);
		release;
	}

	runstrand (__outserial, buf);
	/* No need to wait for anything */
	return 0;
}
