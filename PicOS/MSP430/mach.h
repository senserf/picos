#ifndef __pg_mach_h
#define	__pg_mach_h		1

/* ============================================================================ */
/*                       PicOS                                                  */
/*                                                                              */
/* Hardware-specific definitions                                                */
/*                                                                              */
/*                                                                              */
/* Copyright (C) Olsonet Communications Corporation, 2002--2005                 */
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

#ifndef	__MSP430__
#error "this must be compiled with mspgcc!!!"
#endif

#include "arch.h"

#define	LITTLE_ENDIAN	1
#define	BIG_ENDIAN	0

#define	STACK_SIZE	256			// Bytes
#define	STACK_START	(0x200 + 2048)		// FWA + 1 of stack
#define	STACK_END	(STACK_START - STACK_SIZE)
						// LWA of stack
typedef struct	{
/* ============================== */
/* UART with two circular buffers */
/* ============================== */
	byte selector;
	volatile byte flags;
	byte in;
	byte out;
} uart_t;

#define	UART_BASE		UART_A

#define	UART_FLAGS_IN		0x80
#define	UART_FLAGS_OUT		0x40
#define	UART_FLAGS_LOCK		0x01

/* =============================== */
/* Enable/disable clock interrupts */
/* =============================== */
#define sti_tim	_BIS (TBCCTL0, CCIE)
#define cli_tim	_BIC (TBCCTL0, CCIE)

#define	uart_a_disable_int		_BIC (IE1, URXIE0 + UTXIE0)
#define	uart_b_disable_int		_BIC (IE2, URXIE1 + UTXIE1)
#define	uart_a_disable_read_int		_BIC (IE1, URXIE0)
#define	uart_a_disable_write_int	_BIC (IE1, UTXIE0)
#define	uart_b_disable_read_int		_BIC (IE2, URXIE1)
#define	uart_b_disable_write_int	_BIC (IE2, UTXIE1)
#define	uart_a_enable_read_int		_BIS (IE1, URXIE0)
#define	uart_a_enable_write_int		_BIS (IE1, UTXIE0)
#define	uart_b_enable_read_int		_BIS (IE2, URXIE1)
#define	uart_b_enable_write_int		_BIS (IE2, UTXIE1)

#define	SLEEP	do { \
			if (zz_systat.pdmode) { \
				_BIS_SR (LPM3_bits); \
			} else { \
				_BIS_SR (LPM0_bits); \
			} \
			_NOP (); \
		} while (0)

#define	RISE_N_SHINE	_BIC_SR_IRQ (LPM4_bits)

#endif
