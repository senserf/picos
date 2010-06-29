/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
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
		join (prcs, st);
		release;
	}

	if (*m)
		prcs = strlen (m) +1;
	else
		prcs =  m [1] + 3;

	if ((buf = (char*) umalloc (prcs)) == NULL) {
		/*
		 * We have to wait for memory
		 */
		umwait (st);
		release;
	}

	if (*m)
		strcpy (buf, m);
	else
		memcpy (buf, m, prcs);

	if (runstrand (__outserial, buf) == 0) {
		// fork has failed, deallocate buf
		ufree (buf);
		// and wait for a process slot
		npwait (st);
		release;
	}

	return 0;
}
