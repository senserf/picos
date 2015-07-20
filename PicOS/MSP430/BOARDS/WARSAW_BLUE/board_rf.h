/*
 * RF pins assignment:
 *
 *        RSI	P1.0	OUT
 *	  RSCLK	P1.1	OUT
 *	  RSO	P1.2	IN
 *	  GDO2	P1.3	UNUSED
 *	  GDO0	P1.4	IN
 *	  CSN	P1.5	OUT
 */

//    Make sure P1 interrupts are serviced
//+++ "p1irq.c"

REQUEST_EXTERNAL (p1irq);	// Needed to force the file to be loaded
				// if loading from library

#define	cc1100_ini_regs		_BIC (P1IES, 0x10)

#define	cc1100_int		(P1IFG & 0x10)
#define	cc1100_clear_int	_BIC (P1IFG, 0x10)
#define	CC1100_RX_FIFO_READY		(P1IN & 0x10)

#define cc1100_rcv_int_enable		do { \
					_BIS (P1IE, 0x10); \
					if (CC1100_RX_FIFO_READY) \
						_BIS (P1IFG, 0x10); \
				} while (0)
						
#define cc1100_rcv_int_disable		_BIC (P1IE, 0x10)

#define	cc1100_sclk_up		_BIS (P1OUT, 0x02)
#define	cc1100_sclk_down	_BIC (P1OUT, 0x02)

#define	cc1100_csn_up		_BIS (P1OUT, 0x20)
#define	cc1100_csn_down	_BIC (P1OUT, 0x20)

#define	cc1100_si_up		_BIS (P1OUT, 0x01)
#define	cc1100_si_down		_BIC (P1OUT, 0x01)

#define	cc1100_so_val		(P1IN & 0x04)

