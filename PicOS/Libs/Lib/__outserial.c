/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2016                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

//+++ "ser_select.c"

/* ==================== */
/* Serial communication */
/* ==================== */

#define	OM_INIT		00
#define	OM_WRITE	10

#if UART_DRIVER > 1

extern int __serial_port;
static int __cport;
#define	set_cport	(__cport = __serial_port)

#else	/* UART_DRIVER <= 1 */

#define	__cport		UART_A
#define	set_cport	CNOP

#endif	/* UART_DRIVER > 1 */

strand (__outserial, const char*)
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

__OM_WRITE:

	quant = io (OM_WRITE, __cport, WRITE, (char*)ptr, len);
	ptr += quant;
	len -= quant;
	if (len == 0) {
		/* This is always a fresh buffer allocated dynamically */
		ufree (data);
		finish;
	}

	// There appears to be a nasty race between I/O events and the proceed
	// that used to be here, surfacing when MAX_TASKS <= 0; besides, goto
	// (aka sameas) makes more sense here, as the serial operations that
	// need dynamic memory are deadlock prone
	goto __OM_WRITE;

	// proceed (OM_WRITE);

endstrand

#undef 	OM_INIT
#undef	OM_WRITE
