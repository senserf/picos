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
/* LCD display process */
/* =================== */

#define	DS_INIT		0
#define	DS_EASY		1
#define	DS_SCROLL	2
#define	DS_ADVANCE	3
#define	DS_WRITE	4

/* ================================ */
/* Interface to the display process */
/* ================================ */
address __display_pmem = NULL;

#define	cnt	(*((int*)(__display_pmem + 0)))
#define	len	(*((int*)(__display_pmem + 1)))
#define	pos	(*((int*)(__display_pmem + 2)))
#define	ptr	(*((int*)(__display_pmem + 3)))
#define	shft	(*((int*)(__display_pmem + 4)))
#define	mess	( (char*)(__display_pmem + 5))

process (__display, void)
/* =================================== */
/* Writes a message to the LCD display */
/* =================================== */

	char c;

	nodata;

  entry (DS_INIT)

	io (DS_INIT, LCD, CONTROL, (char*) &cnt, LCD_CNTRL_ERASE);

	len = strlen (mess);
	/* Reset the display */
	if (len > 32) {
		/* Have to scroll */
		shft = 0;
		proceed (DS_SCROLL);
	}
	/* Do it the easy way */
	ptr = len;
	pos = 0;

  entry (DS_EASY)

	cnt = io (DS_EASY, LCD, WRITE, mess + pos, ptr);
	if (cnt != ptr) {
		ptr -= cnt;
		pos += cnt;
		proceed (DS_EASY);
	}
	/* We can disappear from the scene */
	ufree (__display_pmem);
	__display_pmem = NULL;
	finish;

  entry (DS_SCROLL)

	/* Move to the front */
	c = 0;
	io (DS_SCROLL, LCD, CONTROL, &c, LCD_CNTRL_POS);
	delay (JIFFIES/2, DS_ADVANCE);
	release;

  entry (DS_ADVANCE)

	ptr = 16;
	pos = shft;

  entry (DS_WRITE)

	cnt = io (DS_WRITE, LCD, WRITE, mess + pos, ptr);
	if (cnt == ptr) {
		/* We are done with this turn */
		if (shft == len - 16) {
			shft = 0;
		} else {
			shft ++;
		}
		proceed (DS_SCROLL);
	}
	ptr -= cnt;
	pos += cnt;
	proceed (DS_WRITE);

endprocess (1)
