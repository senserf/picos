#ifndef	__pg_rf24g_sys_h
#define	__pg_rf24g_sys_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "p1irq.c"

/* ================================================================== */

/*
 * Pin assignment
 *
 *  RF24G                       MSP430Fxxx
 * ===========================================
 *  CE				P6.3	GP3	OUT
 *  CS				P6.4	GP4	OUT
 *  CLK1			P6.5	GP5	OUT
 *  CLK2					GND
 *  DR1				P1.0	CFG0	IN (Data Ready Interrupt)
 *  DR2						PULL DOWN?
 *  DOUT2					PULL DOWN?
 *  DATA			P2.4	GP2	IN/OUT
 */

#define	ini_regs	do { \
				_BIC (P1OUT, 0x01); \
				_BIC (P1DIR, 0x01); \
				_BIC (P2OUT, 0x10); \
				_BIC (P2DIR, 0x10); \
				_BIC (P6OUT, 0x38); \
				_BIS (P6DIR, 0x38); \
						    \
				_BIC (P2OUT, 0x01); \
				_BIS (P2DIR, 0x01); \
				_BIC (P5OUT, 0x03); \
				_BIS (P5DIR, 0x03); \
			} while (0)

#define	rf24g_int		(P1IFG & 0x01)
#define	clear_rf24g_int		P1IFG &= ~0x01
#define	set_rcv_int		_BIS (P1IE, 0x01)
#define	clr_rcv_int		_BIC (P1IE, 0x01)

#define	clk1_up			_BIS (P6OUT, 0x20)
#define	clk1_down		_BIC (P6OUT, 0x20)
#define	ce_up			_BIS (P6OUT, 0x08)
#define	ce_down			_BIC (P6OUT, 0x08)
#define	cs_up			_BIS (P6OUT, 0x10)
#define	cs_down			_BIC (P6OUT, 0x10)
#define	data_out		_BIS (P2DIR, 0x10)
#define	data_in			_BIC (P2DIR, 0x10)
#define	data_up			_BIS (P2OUT, 0x10)
#define	data_down		_BIC (P2OUT, 0x10)
#define	data_val		(P2IN & 0x10)

#endif
