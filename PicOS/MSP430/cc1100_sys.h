#ifndef	__pg_cc1100_sys_h
#define	__pg_cc1100_sys_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "p1irq.c"

/* ================================================================== */

/*
 * Pin assignment
 *
 *  CC1100		        TARGET	DM2100
 * ===========================================================
 *  SI				P1.2    P6.3	GP3    OUTPUT
 *  SCLK			P1.3	P1.3	CFG3   OUTPUT
 *  SO				P1.4	P1.2	CFG2   INPUT
 *  GDO2			P1.5	P1.1	CFG1   UNUSED
 *  GDO0			P1.6	P1.0	CFG0   INPUT
 *  CSN				P1.7	P6.4	GP4    OUTPUT
 */

#define	UWAIT1		udelay (1)
#define	UWAIT41		udelay (41)
#define	SPI_WAIT	do { } while (0)
#define	STROBE_WAIT	udelay (10)

#if TARGET_BOARD == BOARD_GENESIS
/*
 * Pins for the Genesis board
 */

#define	ini_regs	do { \
				_BIC (P1OUT, 0xfc); \
				_BIC (P1DIR, 0x70); \
				_BIS (P1DIR, 0x8c); \
				_BIC (P1IES, 0x40); \
				_BIC (P1IFG, 0x40); \
			} while (0)

#define	cc1100_int		(P1IFG & 0x40)
#define	clear_cc1100_int	_BIC (P1IFG, 0x40)

#define	RX_FIFO_READY		(P1IN & 0x40)

#define rcv_enable_int		do { \
					zzv_iack = 1; \
					_BIS (P1IE, 0x40); \
					if (RX_FIFO_READY && zzv_iack) \
						_BIS (P1IFG, 0x40); \
				} while (0)
						
#define rcv_disable_int		_BIC (P1IE, 0x40)

#define	sclk_up		_BIS (P1OUT, 0x08)
#define	sclk_down	_BIC (P1OUT, 0x08)

#define	csn_up		_BIS (P1OUT, 0x80)
#define	csn_down	_BIC (P1OUT, 0x80)

#define	si_up		_BIS (P1OUT, 0x04)
#define	si_down		_BIC (P1OUT, 0x04)

#define	so_val		(P1IN & 0x10)

#define	LEDI(a,b)	do { \
				if ((a) == 2 || (a) == 3) \
					leds (a, b);\
			} while (0)

#define	GENESIS_RESET_KEY_PRESSED	((P6IN & 0x01) == 0)

#endif	/* TARGET_BOARD == BOARD_GENESIS */


#if TARGET_BOARD == BOARD_DM2100
/*
 * Pins for DM2100
 */

#define	ini_regs	do { \
				_BIC (P1OUT, 0x0f); \
				_BIC (P6OUT, 0x18); \
				_BIC (P1DIR, 0x07); \
				_BIS (P1DIR, 0x08); \
				_BIS (P6DIR, 0x18); \
				_BIC (P1IES, 0x02); \
			} while (0)

#define	cc1100_int		(P1IFG & 0x01)
#define	clear_cc1100_int	_BIC (P1IFG, 0x01)

#define	RX_FIFO_READY		(P1IN & 0x01)

#define rcv_enable_int		do { \
					zzv_iack = 1; \
					_BIS (P1IE, 0x01); \
					if (RX_FIFO_READY && zzv_iack) \
						_BIS (P1IFG, 0x01); \
				} while (0)
						
#define rcv_disable_int		_BIC (P1IE, 0x01)

#define	sclk_up		_BIS (P1OUT, 0x08)
#define	sclk_down	_BIC (P1OUT, 0x08)

#define	csn_up		_BIS (P6OUT, 0x10)
#define	csn_down	_BIC (P6OUT, 0x10)

#define	si_up		_BIS (P6OUT, 0x08)
#define	si_down		_BIC (P6OUT, 0x08)

#define	so_val		(P1IN & 0x04)

#define	LEDI(a,b)	leds (a,b)

#endif	/* TARGET_BOARD == BOARD_DM2100 */


#endif
