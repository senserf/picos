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

#define	ini_regs		_BIC (P1IES, 0x10)

#define	cc1100_int		(P1IFG & 0x10)
#define	clear_cc1100_int	_BIC (P1IFG, 0x10)
#define	RX_FIFO_READY		(P1IN & 0x10)

#define rcv_enable_int		do { \
					zzv_iack = 1; \
					_BIS (P1IE, 0x10); \
					if (RX_FIFO_READY && zzv_iack) \
						_BIS (P1IFG, 0x10); \
				} while (0)
						
#define rcv_disable_int		_BIC (P1IE, 0x10)

#define	sclk_up		_BIS (P1OUT, 0x02)
#define	sclk_down	_BIC (P1OUT, 0x02)

#define	csn_up		_BIS (P1OUT, 0x20)
#define	csn_down	_BIC (P1OUT, 0x20)

#define	si_up		_BIS (P1OUT, 0x01)
#define	si_down		_BIC (P1OUT, 0x01)

#define	so_val		(P1IN & 0x04)

