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

/* =================== */
/* LCD updater process */
/* =================== */

#define	UPD_INIT	00
#define	UPD_POS		10
#define	UPD_WRITE	20

address __dupdater_pmem = NULL;

#define	changed		(*((lword*)(__dupdater_pmem + 0)))
#define	dupw		(*((lword*)(__dupdater_pmem + 2)))
#define	pos		(*((int*)(__dupdater_pmem + 4)))
#define	len		(*((int*)(__dupdater_pmem + 5)))
#define	updpid		(*((int*)(__dupdater_pmem + 6)))
#define	lastend		(*((int*)(__dupdater_pmem + 7)))
#define	sbuf		( (char*)(__dupdater_pmem + 8))

process (__dupdater, void)

  byte k;

  nodata;

  entry (UPD_INIT)

	if (changed == 0) {
		updpid = 0;
		finish;
	}

	while (1) {
		if ((changed & dupw) != 0)
			break;
		if (++pos == 32) {
			pos = 0;
			dupw = 1;
		} else {
			dupw <<= 1;
		}
	}

	/* First different */

  entry (UPD_POS)

	k = (byte) pos;
	io (UPD_POS, LCD, CONTROL, (char*)&k, LCD_CNTRL_POS);

	/* Find the length */
	len = 0;
	do {
		changed &= ~dupw;
		len++;
		if ((dupw <<= 1) == 0) {
			dupw = 1;
			break;
		}
	} while ((changed & dupw) != 0);

  entry (UPD_WRITE)

	do {
		k = io (UPD_WRITE, LCD, WRITE, sbuf + pos, len);
		len -= k;
		pos += k;
	} while (len);

	if (pos == 32)
		pos = 0;

	proceed (UPD_INIT);

endprocess (1)
