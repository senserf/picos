/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

//+++ "ser_select.c"

/* ==================== */
/* Serial communication */
/* ==================== */

#define	OM_INIT		00
#define	OM_WRITE	10
#define	OM_RETRY	20

extern int __serial_port;

process (__outserial, const char)
/* ===================== */
/* Runs the output queue */
/* ===================== */

	static const char *ptr;
	static int cport, len;

  entry (OM_INIT)

	ptr = data;
	cport = __serial_port;

  entry (OM_WRITE)

	if ((len = strlen (ptr)) == 0) {
		/* This is always a fresh buffer allocated dynamically */
		ufree (data);
		finish;
	}

  entry (OM_RETRY)

	// In case of a race-missed wakeup, retstart us after 1 msec. Note
	// that writing a character at 9600 bps will take less than that.
	delay (1, OM_RETRY);
	ptr += io (OM_RETRY, cport, WRITE, (char*)ptr, len);

	proceed (OM_WRITE);

endprocess (1)

#undef 	OM_INIT
#undef	OM_WRITE
