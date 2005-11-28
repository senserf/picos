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

#define	ini_regs	do { \
				_BIC (P1OUT, 0x0f); \
				_BIC (P6OUT, 0x18); \
				_BIC (P1DIR, 0x07); \
				_BIS (P1DIR, 0x08); \
				_BIS (P6DIR, 0x18); \
				_BIC (P1IES, 0x01); \
				_BIC (P1IFG, 0x01); \
			} while (0)

#define	cc1100_int		(P1IFG & 0x01)
#define	clear_cc1100_int	_BIC (P1IFG, 0x01)

#define	fifo_ready		(P1IN & 0x01)

#define	request_rcv_int		_BIS (P1IFG, 0x01)

#define	rcv_enable_int		do { \
					_BIS (P1IE, 0x01); \
					if (fifo_ready) \
						request_rcv_int; \
				} while (0)

#define	rcv_disable_int		_BIC (P1IE, 0x01)

#define	sclk_up		_BIS (P1OUT, 0x08)
#define	sclk_down	_BIC (P1OUT, 0x08)

#define	csn_up		_BIS (P6OUT, 0x10)
#define	csn_down	_BIC (P6OUT, 0x10)

#define	si_up		_BIS (P6OUT, 0x08)
#define	si_down		_BIC (P6OUT, 0x08)

#define	so_val		(P1IN & 0x04)

#ifndef	USE_LEDS
#define	USE_LEDS	0
#endif

#if	USE_LEDS
#define LEDI(n,s)	do { \
				if (s) { \
					_BIC (P4OUT, 1 << (n)); \
					_BIS (P4DIR, (1 << (n))); \
				} else { \
					_BIS (P4OUT, 1 << (n)); \
					_BIC (P4DIR, (1 << (n))); \
				} \
			} while (0)
#else
#define	LEDI(n,s)	do { } while (0)
#endif

#endif
