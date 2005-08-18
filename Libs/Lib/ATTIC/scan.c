/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "form.h"

//+++ "vscan.c"

int scan (const char *buf, const char *fmt, ...) {

	return vscan (buf, fmt, va_par (fmt));
}
