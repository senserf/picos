#include "sysio.h"
/* ============================================================================ */
/*                                                                              */
/* Copyright (C) Olsonet Communications Corporation, 2002, 2003                 */
/*                                                                              */
/* Permission is hereby granted, free of charge, to any person obtaining a copy */
/* of this software and associated documentation files (the "Software"), to     */
/* deal in the Software without restriction, including without limitation the   */
/* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  */
/* sell copies of the Software, and to permit persons to whom the Software is   */
/* furnished to do so, subject to the following conditions:                     */
/*                                                                              */
/* The above copyright notice and this permission notice shall be included in   */
/* all copies or substantial portions of the Software.                          */
/*                                                                              */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS */
/* IN THE SOFTWARE.                                                             */
/*                                                                              */
/* ============================================================================ */

/* ==================== */
/* Serial communication */
/* ==================== */

//+++ "ser_select.c"

char *__inpline = NULL;

extern int __serial_port;

#define	MAX_LINE_LENGTH	127

#define	IM_INIT		00
#define	IM_READ		10

process (__inserial, void)
/* ============================== */
/* Inputs a line from serial port */
/* ============================== */

	static char *tmp, *ptr;
	static int len, cport;

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
	if (*ptr == '\n' && ptr == tmp)
		/* Ignore new-lines at the beginning of line */
		proceed (IM_READ);
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

endprocess (1)

#undef	IM_INIT
#undef	IM_READ
