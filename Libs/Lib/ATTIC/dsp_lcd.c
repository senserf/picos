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

//+++ "__display.c"

extern address __display_pmem;

int __display (word, address);

#define	cnt	(*((int*)(__display_pmem + 0)))
#define	len	(*((int*)(__display_pmem + 1)))
#define	pos	(*((int*)(__display_pmem + 2)))
#define	ptr	(*((int*)(__display_pmem + 3)))
#define	shft	(*((int*)(__display_pmem + 4)))
#define	mess	( (char*)(__display_pmem + 5))

int dsp_lcd (const char *m, bool kl) {

	if (__display_pmem) {
		if (!kl)
			return -1;
		kill (running (__display));
		ufree (__display_pmem);
		__display_pmem = NULL;
	}
	__display_pmem = umalloc (5 * sizeof (int) + strlen (m) + 1);
	if (__display_pmem != NULL) {
		strcpy (mess, m);
		fork (__display, NULL);
	}
	return 0;
}

#else
void sim_lcd (const char *m);

int dsp_lcd (const char *m, bool kl) {
	sim_lcd (m);
	return 0;
}
#endif

