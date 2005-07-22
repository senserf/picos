#ifndef	__pg_phys_radio_h
#define	__pg_phys_radio_h	1
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

#define	RADIO_DEF_BUF_LEN	32	/* Default buffer length */
/*
 * These values are merely defaults changeable with tcv_control
 */
#define	RADIO_DEF_MNBACKOFF	32	/* Minimum backoff */
#define	RADIO_DEF_BSBACKOFF	0xff	/* Randomized component */
#define	RADIO_DEF_TCVSENSE	4	/* Pre-Tx activity sense time */

#define	RADIO_POST_SPACE	1	/* Milliseconds */

void phys_radio (int, int, int);

#endif
