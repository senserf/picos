#ifndef	__pg_cc1000_sys_h
#define	__pg_cc1000_sys_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "p1irq.c"

REQUEST_EXTERNAL (p1irq);

/* ================================================================== */

/*
 * Pin assignment:
 *
 *  CC1000                        MSP430F1xx
 * ===========================================
 *  DCLK			P1.0			in
 *  PALE			P1.1			out
 *  PCLK			P1.2			out
 *  PDATA			P1.3			in/out
 *  DIO				P6.3			in/out
 *  RSSI			P6.5			analog in
 */

#define	ini_regs	do { \
				_BIS (P1DIR, 0x0e); \
				_BIC (P1DIR, 0x01); \
				_BIS (P6DIR, 0x08); \
				_BIS (P6SEL, 0x20); \
			} while (0)

#define	GP_INT_RCV	0x1000	// Rising edge of P1.0
#define	GP_INT_XMT	0x1001	// Falling edge of P1.0 (bit 0)
#define	GP_INT_MSK	0x0001  // interrupt on P1.0

#define	cc1000_int		(P1IFG & 0x01)
#define	clear_cc1000_int	P1IFG &= ~0x01

/*
 * CC1000 signal operations
 */
#define	chp_paledown	_BIC (P1OUT, 0x02)
#define	chp_paleup	_BIS (P1OUT, 0x02)
#define	chp_pclkdown	_BIC (P1OUT, 0x04)
#define	chp_pclkup	_BIS (P1OUT, 0x04)
#define chp_pdirout	_BIS (P1DIR, 0x08)
#define chp_pdirin	_BIC (P1DIR, 0x08)

#define chp_pdioout	do { \
				_BIC (P6OUT, 0x08); \
				_BIS (P6DIR, 0x08); \
			} while (0)
#define chp_pdioin	_BIC (P6DIR, 0x08)

#define chp_getpbit	((P1IN & 0x08) != 0)

#define	chp_outpbit(b)	do { \
				if (b) \
					_BIS (P1OUT, 0x08); \
				else \
					_BIC (P1OUT, 0x08); \
			} while (0)

#define	chp_getdbit	(P6IN & 0x08)
#define	chp_setdbit	_BIS (P6OUT, 0x08)
#define	chp_clrdbit	_BIC (P6OUT, 0x08)

#define	clr_xcv_int	_BIC (P1IE, 0x01)

#define	set_xmt_int	do { \
				_BIS (P1IES, 0x01); \
				clear_cc1000_int; \
				_BIS (P1IE, 0x01); \
			} while (0)
#define	set_rcv_int	do { \
				_BIC (P1IES, 0x01); \
				clear_cc1000_int; \
				_BIS (P1IE, 0x01); \
			} while (0)

#define	hard_lock	clr_xcv_int

#define	hard_drop	do { \
				if (__pi_v_status) { \
					if ((__pi_v_status & 0x01)) \
						set_xmt_int; \
					else \
						set_rcv_int; \
				} \
			} while (0)

#define	disable_xcv_timer	do { } while (0)

#define	RSSI_MIN	0x0000	// Minimum and maximum RSSI values (for scaling)
#define	RSSI_MAX	0x0fff
#define	RSSI_SHF	4	// Shift bits to fit into a (unsigned) byte

#define	lbt_ok(v)	((v) < (word)(((lint)LBT_THRESHOLD * 4096) / 100))

#endif
