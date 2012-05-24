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

#define	ini_regs		_BIC (P1IES, 0x10)

#define	RX_FIFO_READY		(P1IN & 0x10)

#define	sclk_up		_BIS (P1OUT, 0x02)
#define	sclk_down	_BIC (P1OUT, 0x02)

#define	csn_up		_BIS (P1OUT, 0x20)
#define	csn_down	_BIC (P1OUT, 0x20)

#define	si_up		_BIS (P1OUT, 0x01)
#define	si_down		_BIC (P1OUT, 0x01)

#define	so_val		(P1IN & 0x04)

