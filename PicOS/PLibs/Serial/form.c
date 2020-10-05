/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
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
