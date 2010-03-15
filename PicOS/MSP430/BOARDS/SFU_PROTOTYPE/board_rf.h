/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//    Make sure P1 interrupts are coming in
//+++ "p1irq.c"

REQUEST_EXTERNAL (p1irq);

#define	ini_regs	do { \
				_BIC (P1OUT, 0xfc); \
				_BIS (P1DIR, 0x8c); \
				_BIC (P1IES, 0x40); \
			} while (0)
			// The last one is needed for the reset button. Not the
			// most elegant place to set its direction.

#define	cc1100_int		(P1IFG & 0x40)
#define	clear_cc1100_int	_BIC (P1IFG, 0x40)

#define	RX_FIFO_READY		(P1IN & 0x40)

#define rcv_enable_int		do { \
					_BIS (P1IE, 0x40); \
					if (RX_FIFO_READY) \
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
