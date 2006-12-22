/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
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

#if UART_DRIVER > 1

extern int __serial_port;
static int __cport;
#define	set_cport	(__cport = __serial_port)

#else	/* UART_DRIVER <= 1 */

#define	__cport		UART_A
#define	set_cport	CNOP

#endif	/* UART_DRIVER > 1 */

strand (__outserial, const char)
/* ===================== */
/* Runs the output queue */
/* ===================== */

	static const char *ptr;
	static int len;
	int quant;

  entry (OM_INIT)

	set_cport;
	ptr = data;
	if (*ptr)
		len = strlen (ptr);
	else
		len = ptr [1] +3; // 3: 0x00, len, 0x04

  entry (OM_WRITE)

	if (len == 0) {
		/* This is always a fresh buffer allocated dynamically */
		ufree (data);
		finish;
	}

  entry (OM_RETRY)

	quant = io (OM_RETRY, __cport, WRITE, (char*)ptr, len);
	ptr += quant;
	len -= quant;
	proceed (OM_WRITE);

endprocess (1)

#undef 	OM_INIT
#undef	OM_WRITE
