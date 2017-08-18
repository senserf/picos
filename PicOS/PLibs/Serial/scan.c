/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2017                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "form.h"

//+++ "vscan.c"

int scan (const char *buf, const char *fmt, ...) {
	va_list ap;
	va_start (ap, fmt);
	return vscan (buf, fmt, ap);
}
