#ifndef __pg_mach_h
#define	__pg_mach_h		1

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* ========================================================================== */
/*                     PicOS                                                  */
/*                                                                            */
/* Hardware-specific definitions                                              */
/*                                                                            */

#define	__ECOG1__	1

#include "arch.h"

#define	LITTLE_ENDIAN	0
#define	BIG_ENDIAN	1

#define	RAM_START	(word*)(0xE800)
#define	RAM_END		(RAM_START+2048)

// ==========================================================================
// This must be the same as the stack fill pattern used in cstartup.asm
// ==========================================================================
#define	STACK_SENTINEL	0xB779
#define	ISTACK_SENTINEL	0xB77A
#if STACK_GUARD
#define	check_stack_overflow	do { \
				    if (*((word*)estk_) != STACK_SENTINEL) \
					syserror (ESTACK, "st"); \
				} while (0)
#else
#define	check_stack_overflow	CNOP
#endif
// ==========================================================================

#define	UART_BASE	UART_A	/* Device number of the firt UART */
#define	UART_BUF_SIZE	8	/* Size of the circular buffer */
#define	UART_TIMEOUT	1024	/* For lost interrupts - one second */

// OUT register base (for portnames)
#define _PDS_base_	((word*)0xFFAD)
// STS register base
#define _PDV_base_	((word*)0xFFB5)

#include "portnames.h"

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

// ============================================================================
#ifdef	MONITOR_PIN_CPU
#define	SLEEP		do { \
				_PVS (MONITOR_PIN_CPU, 0); \
				sleep (); evening; \
				_PVS (MONITOR_PIN_CPU, 1); \
			} while (0)
#else
#define	SLEEP		do { sleep (); evening; } while (0)
#endif	/* MONITOR_PIN_CPU */
// ============================================================================

#define	power_down_mode	(fd.ssm.cpu.cpu_clk_div == 6)
#define	clock_down_mode (rg.tim.tmr_ld == 4095)

#define	RISE_N_SHINE	morning
#define	RTNI		return

#if LEDS_DRIVER
#include "leds_sys.h"
#endif

#endif
