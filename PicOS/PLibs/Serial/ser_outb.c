/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
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
