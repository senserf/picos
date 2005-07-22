#ifndef	__pg_phys_uart_h
#define	__pg_phys_uart_h	1
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

//+++ "phys_uart.c"

#define	UART_DEF_BUF_LEN	256	/* Default buffer length */

/*
 * These values are merely defaults changeable with tcv_control
 */
#define	UART_DEF_MNRCVINT	2	/* Minimum inter-receive delay */
#define	UART_DEF_MXRCVINT	16	/* Maximum inter-receive delay */
#define	UART_DEF_RCVTMT		10	/* Soft receive timeout */
#define	UART_DEF_MNBACKOFF	8	/* Minimum backoff */
#define	UART_DEF_BSBACKOFF	0x1f	/* Randomized component */
#define	UART_DEF_TCVSENSE	2	/* Pre-Tx activity sense time */

#define	UART_CHAR_TIMEOUT	5	/* How long we wait for the next char */
#define	UART_PACKET_SPACE	6	/* After transmission */

#define	UART_PHYS_MODE_DIRECT	0
#define	UART_PHYS_MODE_EMU	1

void phys_uart (int, int, int, int);

#endif
