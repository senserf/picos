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

#define	__ECOG1__	1

#include "arch.h"

#define	LITTLE_ENDIAN	0
#define	BIG_ENDIAN	1

#define	RAM_START	0xE800

#define	UART_BASE	UART_A	/* Device number of the firt UART */
#define	UART_BUF_SIZE	8	/* Size of the circular buffer */
#define	UART_TIMEOUT	1024	/* For lost interrupts - one second */

typedef struct	{
/* ============================== */
/* UART with two circular buffers */
/* ============================== */
	byte obuf [UART_BUF_SIZE], ibuf [UART_BUF_SIZE];
	byte oin, oou, iin, iou;
	/* 0 - UART_A, 1 - UART_B */
	byte selector;
	byte lock;
} uart_t;

extern uart_t zz_uart [];

#define	LCD_BUF_SIZE		5

typedef struct	{
/* === */
/* LCD */
/* === */
	word	cmd:2;		/* command */
	word	pos:6;		/* position: 0 - 31 */
	byte	par [3];	/* Command parameters */
	byte	cnt;		/* Counter needed by seek */
	byte	in, ou;		/* Circular buffer */
	byte	buf [LCD_BUF_SIZE];
} lcd_t;

/* Set the morning bit */
#define	morning		(fd.ssm.ex_ctrl.morning = 1)
#define evening 	(fd.ssm.ex_ctrl.evening = 1)
#define resetcpu 	(fd.ssm.ex_ctrl.cpu_rst = 1)

/* SDRAM array */
#define	sdram		((word*)SDRAM_ADDR)

/* =============================== */
/* Enable/disable clock interrupts */
/* =============================== */
#define sti_tim	(rg.tim.int_en1 = TIM_INT_EN1_TMR_EXP_MASK)
#define cli_tim	(rg.tim.int_dis1 = TIM_INT_DIS1_TMR_EXP_MASK)

#define	uart_a_disable_int	rg.duart.a_int_dis |= \
			DUART_A_INT_DIS_TX_RDY_MASK | \
			DUART_A_INT_DIS_TX_OFL_MASK | \
			DUART_A_INT_DIS_RX_1B_RDY_MASK | \
			DUART_A_INT_DIS_RX_2B_RDY_MASK | \
			DUART_A_INT_DIS_RX_BRK_MASK | \
			DUART_A_INT_DIS_RX_TMO_MASK | \
			DUART_A_INT_DIS_RX_PERR_MASK | \
			DUART_A_INT_DIS_RX_FRAME_ERR_MASK | \
			DUART_A_INT_DIS_RX_OFL_MASK | \
			DUART_A_INT_DIS_RX_UFL_MASK

#define	uart_b_disable_int	rg.duart.b_int_dis |= \
			DUART_B_INT_DIS_TX_RDY_MASK | \
			DUART_B_INT_DIS_TX_OFL_MASK | \
			DUART_B_INT_DIS_RX_1B_RDY_MASK | \
			DUART_B_INT_DIS_RX_2B_RDY_MASK | \
			DUART_B_INT_DIS_RX_BRK_MASK | \
			DUART_B_INT_DIS_RX_TMO_MASK | \
			DUART_B_INT_DIS_RX_PERR_MASK | \
			DUART_B_INT_DIS_RX_FRAME_ERR_MASK | \
			DUART_B_INT_DIS_RX_OFL_MASK | \
			DUART_B_INT_DIS_RX_UFL_MASK

#define	uart_a_enable_read_int	rg.duart.a_int_en |= \
         		DUART_A_INT_EN_RX_1B_RDY_MASK

#define	uart_b_enable_read_int	rg.duart.b_int_en |= \
         		DUART_B_INT_EN_RX_1B_RDY_MASK

#define	uart_a_enable_write_int	rg.duart.a_int_en |= \
         		DUART_A_INT_EN_TX_RDY_MASK

#define	uart_b_enable_write_int	rg.duart.b_int_en |= \
         		DUART_B_INT_EN_TX_RDY_MASK

#if	DIAG_MESSAGES || (dbg_level != 0)
/* ==================================================================== */
/* This is supposed to be useful for debugging,  so  it circumvents the */
/* UART driver and uses no interrupts. Unfortunately, it isn't going to */
/* work until the driver's init function has been called.               */
/* ==================================================================== */
#define DUART_a_STS_TX_RDY_MASK DUART_A_STS_TX_RDY_MASK
#define DUART_b_STS_TX_RDY_MASK DUART_B_STS_TX_RDY_MASK

#define	diag_wchar(c,a)		rg.duart. ## a ## _tx8 = (word)(c)
#define	diag_wait(a)		while ((rg.duart. ## a ## _sts & \
			        DUART_ ## a ## _STS_TX_RDY_MASK) \
					== 0);
#define	diag_disable_int(a)	uart_ ## a ## _disable_int
#define	diag_enable_int(a)	do { \
					if (zz_uart [0].lock == 0) { \
					    uart_ ## a ## _enable_read_int; \
					    uart_ ## a ## _enable_write_int; \
					} \
				} while (0)
#endif

#define	SLEEP		do { evening; sleep (); } while (0)
#define	RISE_N_SHINE	morning

#endif
