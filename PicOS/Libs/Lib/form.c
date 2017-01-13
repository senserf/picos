/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "form.h"

//+++ "vform.c"

char *form (char *buf, const char *fm, ...) {
	va_list ap;
	va_start (ap, fm);
	return vform (buf, fm, ap);
}

word fsize (const char *fm, ...) {
	va_list ap;
	va_start (ap, fm);
	return vfsize (fm, ap);
}
