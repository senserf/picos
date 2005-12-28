/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

/* ==================== */
/* Serial communication */
/* ==================== */

//+++ "ser_select.c"

char *__inpline = NULL;

extern int __serial_port;

#define	MAX_LINE_LENGTH	63

#define	IM_INIT		00
#define	IM_READ		10
#define IM_BIN		20

process (__inserial, void)
/* ============================== */
/* Inputs a line from serial port */
/* ============================== */

	static char *tmp, *ptr;
	static int len, cport;
	int quant;

	nodata;

  entry (IM_INIT)

	if (__inpline != NULL)
		/* Never overwrite previous unclaimed stuff */
		finish;

	if ((tmp = ptr = (char*) umalloc (MAX_LINE_LENGTH + 1)) == NULL) {
		/*
		 * We have to wait for memory
		 */
		umwait (IM_INIT);
		release;
	}
	len = MAX_LINE_LENGTH;
	/* Make sure this doesn't change while we are reading */
	cport = __serial_port;

  entry (IM_READ)

	io (IM_READ, cport, READ, ptr, 1);
	if (ptr == tmp) { // new line
		if (*ptr == NULL) { // bin cmd
			ptr++;
			len--;
			proceed (IM_BIN);
		}

		if (*ptr < 0x20)
			/* Ignore codes below space at the beginning of line */
			proceed (IM_READ);
	}
	if (*ptr == '\n' || *ptr == '\r') {
		*ptr = '\0';
		__inpline = tmp;
		finish;
	}

	if (len) {
		ptr++;
		len--;
	}

	proceed (IM_READ);

  entry (IM_BIN)
	io (IM_BIN, cport, READ, ptr, 1);
	if (--len > *ptr +1) // 1 for 0x04
		len = *ptr +1;
	ptr++;

  entry (IM_BIN +1)
	quant = io (IM_BIN +1, cport, READ, ptr, len);
	len -= quant;
	if (len == 0) {
		__inpline = tmp;
		finish;
	}
	ptr += quant;
	proceed (IM_BIN +1);

endprocess (1)

#undef	IM_INIT
#undef	IM_READ
#undef  IM_BIN

