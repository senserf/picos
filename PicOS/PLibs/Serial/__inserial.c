/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"

/* ==================== */
/* Serial communication */
/* ==================== */

//+++ "ser_select.c"

char *__inpline = NULL;

#if UART_DRIVER > 1

extern int __serial_port;
static int __cport;
#define	set_cport	(__cport = __serial_port)

#else	/* UART_DRIVER <= 1 */

#define	__cport		UART_A
#define	set_cport	CNOP

#endif	/* UART_DRIVER > 1 */

#define	MAX_LINE_LENGTH	63

#define	IM_INIT		0
#define	IM_READ		1
#define IM_BIN		2
#define IM_BINL		3

strand (__inserial, address)
/* ============================== */
/* Inputs a line from serial port */
/* ============================== */

	static char *ptr;
	static int len;
	int quant;

  entry (IM_INIT)

	if (__inpline != NULL)
		/* Never overwrite previous unclaimed stuff */
		finish;

	// On eCOG char* is non-trivially upgraded from 'address'
	if ((ptr = (char*) (data = umalloc (MAX_LINE_LENGTH + 1))) == NULL) {
		/*
		 * We have to wait for memory
		 */
		umwait (IM_INIT);
		release;
	}
	savedata (data);
	len = MAX_LINE_LENGTH;
	/* Make sure this doesn't change while we are reading */
	set_cport;

  entry (IM_READ)

	io (IM_READ, __cport, READ, ptr, 1);
	if (ptr == (char*) data) { // new line
		if (*ptr == '\0') { // bin cmd
			ptr++;
			proceed (IM_BIN);
		}

		if (*ptr < 0x20)
			/* Ignore codes below space at the beginning of line */
			proceed (IM_READ);
	}
	if (*ptr == '\n' || *ptr == '\r') {
		*ptr = '\0';
		__inpline = (char*)data;
		finish;
	}

	if (len) {
		ptr++;
		len--;
	}

	proceed (IM_READ);

  entry (IM_BIN)

	io (IM_BIN, __cport, READ, ptr, 1);
	len -= 2;
	if (len > *ptr + 1) // 1 for 0x04
		len = *ptr + 1;
	ptr++;

  entry (IM_BINL)

	quant = io (IM_BINL, __cport, READ, ptr, len);
	len -= quant;
	if (len == 0) {
		__inpline = (char*)data;
		finish;
	}
	ptr += quant;
	proceed (IM_BINL);

endstrand

#undef	IM_INIT
#undef	IM_READ
#undef  IM_BIN
#undef  IM_BINL

