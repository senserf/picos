/*
 * RF pins for the Genesis board
 */

//    Make sure P1 interrupts are coming
//+++ "p1irq.c"

REQUEST_EXTERNAL (p1irq);

#define	cc1100_ini_regs	do { \
				_BIC (P1OUT, 0xfc); \
				_BIS (P1DIR, 0x8c); \
				_BIC (P1IES, 0x40); \
			} while (0)
			// The last one is needed for the reset button. Not the
			// most elegant place to set its direction.

#define	cc1100_int		(P1IFG & 0x40)
#define	cc1100_clear_int	_BIC (P1IFG, 0x40)

#define	CC1100_RX_FIFO_READY		(P1IN & 0x40)

#define cc1100_rcv_int_enable		do { \
					_BIS (P1IE, 0x40); \
					if (CC1100_RX_FIFO_READY) \
						_BIS (P1IFG, 0x40); \
				} while (0)
						
#define cc1100_rcv_int_disable		_BIC (P1IE, 0x40)

#define	cc1100_sclk_up		_BIS (P1OUT, 0x08)
#define	cc1100_sclk_down	_BIC (P1OUT, 0x08)

#define	cc1100_csn_up		_BIS (P1OUT, 0x80)
#define	cc1100_csn_down	_BIC (P1OUT, 0x80)

#define	cc1100_si_up		_BIS (P1OUT, 0x04)
#define	cc1100_si_down		_BIC (P1OUT, 0x04)

#define	cc1100_so_val		(P1IN & 0x10)

