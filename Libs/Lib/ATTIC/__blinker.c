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

/* =========== */
/* LED blinker */
/* =========== */

#define	BL_NEXT		0
#define	BL_SHOW		1

address __blink_pmem;

process (__blinker, void)

	nodata;

#define	len    (*((int*)(__blink_pmem + 0)))
#define	pos    (*((int*)(__blink_pmem + 1)))
#define	chr    (*((int*)(__blink_pmem + 2)))
#define	ntv    (*((int*)(__blink_pmem + 3)))
#define	pattern ((char*)(__blink_pmem + 4))

  entry (BL_NEXT)

	if (pos >= len)
		pos = 0;

	chr = pattern [pos >> 1];
	if ((pos & 1) == 0)
		chr >>= 4;
	chr &= 0xf;
	pos++;

  entry (BL_SHOW)

	io (BL_SHOW, LEDS, CONTROL, ((char*)&chr) + 1, LEDS_CNTRL_SET);
	delay (ntv, BL_NEXT);

endprocess (1)
