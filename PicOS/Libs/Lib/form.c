/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "form.h"

//+++ "vform.c"

char *form (char *buf, const char *fm, ...) {

	return vform (buf, fm, va_par (fm));
}

