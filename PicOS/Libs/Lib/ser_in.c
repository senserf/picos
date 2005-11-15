/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

//+++ "__inserial.c"

extern char *__inpline;

int __inserial (word, address);

int ser_in (word st, char *buf, int len) {
/* ======= */
/* Direct */
/* ====== */
	int prcs;

	if (__inpline == NULL) {
		if ((prcs = running (__inserial)) == 0)
			prcs = fork (__inserial, NULL);
		if (st == NONE)
			return prcs;
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
	return (st == NONE) ? 0 : prcs;
}
