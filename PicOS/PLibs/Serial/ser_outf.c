/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "form.h"

//+++ "__outserial.c"

procname (__outserial);

int ser_outf (word st, const char *m, ...) {

	int prcs;
	char *buf;
	va_list ap;

	if ((prcs = running (__outserial)) != 0) {
		/* We have to wait */
		join (prcs, st);
		release;
	}

	va_start (ap, m);

	if ((buf = vform (NULL, m, ap)) == NULL) {
		/*
		 * This means that we are out of memory
		 */
		umwait (st);
		release;
	}

	if (runstrand (__outserial, buf) == 0) {
		ufree (buf);
		npwait (st);
		release;
	}

	return 0;
}
