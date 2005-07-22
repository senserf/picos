#include "sysio.h"
#include "options.sys"
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

#if !ECOG_SIM

//+++ "__dupdater.c"

extern address __dupdater_pmem;

int __dupdater (word, address);

#define	changed		(*((lword*)(__dupdater_pmem + 0)))
#define	dupw		(*((lword*)(__dupdater_pmem + 2)))
#define	pos		(*((int*)(__dupdater_pmem + 4)))
#define	len		(*((int*)(__dupdater_pmem + 5)))
#define	updpid		(*((int*)(__dupdater_pmem + 6)))
#define	lastend		(*((int*)(__dupdater_pmem + 7)))
#define	sbuf		( (char*)(__dupdater_pmem + 8))
#define	dupd_plen	(16 + 32)

void upd_lcd (const char *buf) {

	int i, j;
	lword w;

	if (__dupdater_pmem == NULL) {
		/* First time around, allocate buffer */
		if ((__dupdater_pmem = umalloc (dupd_plen)) == NULL)
			/* No way */
			return;
		memset (__dupdater_pmem, 0, dupd_plen);
		if (__dupdater_pmem == NULL)
			syserror (EMALLOC, "upd_lcd");
		updpid = 0;
		for (lastend = 0; lastend < 32; lastend++) {
			if (buf [lastend] == '\0')
				break;
			/* Make sure they are completely different */
			sbuf [lastend] = buf [lastend];
		}
		for (i = lastend; i < 32; i++)
			sbuf [i] = ' ';
		changed = 0xffffffff;
	} else {
		for (w = 1, i = 0; i < 32; i++, w <<= 1) {
			if (buf [i] == '\0')
				break;
			if (buf [i] != sbuf [i]) {
				sbuf [i] = buf [i];
				changed |= w;
			}
		}
		if (i < lastend) {
			for (j = i; j < lastend; j++, w <<= 1) {
				if (sbuf [j] != ' ') {
					sbuf [j] = ' ';
					changed |= w;
				}
			}
		}
		lastend = i;
	}
	if (changed && updpid == 0)
		updpid = fork (__dupdater, NULL);
}


#else

void sim_lcd (const char *m);

void upd_lcd (const char *buf) {
   sim_lcd (buf);
}

#endif
