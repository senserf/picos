/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "form.h"

//+++ "vscan.c"

int scan (const char *buf, const char *fmt, ...) {
	va_list ap;
	va_start (ap, fmt);
	return vscan (buf, fmt, ap);
}
