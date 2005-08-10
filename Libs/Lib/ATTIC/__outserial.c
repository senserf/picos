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
