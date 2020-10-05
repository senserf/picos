/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"

//+++ "__inserial.c"

extern char *__inpline;

procname (__inserial);

int ser_in (word st, char *buf, int len) {
/* ======= */
/* Direct */
/* ====== */
	int prcs;

	if (len == 0)
		// Just in case
		return 0;

	if (__inpline == NULL) {
		if ((prcs = running (__inserial)) == 0) {
			prcs = runthread (__inserial);
			if (prcs == 0) {
				npwait (st);
				release;
			}
		}
		join (prcs, st);
		release;
	}

	/* Input available */
	if (*__inpline == NULL) // bin cmd
		prcs = __inpline[1] + 3; // 0x00, len, 0x04
	else
		prcs = strlen (__inpline);

	if (prcs >= len)
		prcs = len-1;

	memcpy (buf, __inpline, prcs);
	ufree (__inpline);
	__inpline = NULL;
	if (*buf) // if it's NULL, it's a bin cmd
		buf [prcs] = '\0';
	return prcs;
}
