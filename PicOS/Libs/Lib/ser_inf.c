/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "form.h"

//+++ "__inserial.c"

extern char *__inpline;

int __inserial (word, address);

int ser_inf (word st, const char *fmt, ...) {
/* ========= */
/* Formatted */
/* ========= */

	int prcs;
	va_list	ap;

	if (fmt == NULL)
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
	va_start (ap, fmt);

	prcs = vscan (__inpline, fmt, ap);

	ufree (__inpline);
	__inpline = NULL;

	return prcs;
}
