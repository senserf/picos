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
	int quant;

  entry (OM_INIT)

	ptr = data;
	cport = __serial_port;
	if (*ptr)
		len = strlen (ptr);
	else
		len = ptr[1] +3; // 3: 0x00, len, 0x04

  entry (OM_WRITE)

	if (len == 0) {
		/* This is always a fresh buffer allocated dynamically */
		ufree (data);
		finish;
	}

  entry (OM_RETRY)

	quant = io (OM_RETRY, cport, WRITE, (char*)ptr, len);
	ptr += quant;
	len -= quant;
	proceed (OM_WRITE);

endprocess (1)

#undef 	OM_INIT
#undef	OM_WRITE
