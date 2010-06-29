/*
 * Pin assignment for the bluetooth serial interface (LinkMatic 2.0):
 *
 * 	P1.5 - ATT
 *	P1.6 - ESC
 *	P1.7 - RESET
 */

#define	ini_blue_regs	do { \
				_BIC (P1OUT, 0xE0); \
				_BIC (P1SEL, 0xE0); \
				_BIC (P1DIR, 0x20); \
				_BIS (P1DIR, 0xC0); \
			} while (0)

#define	blue_ready	(P1IN & 0x20)

#define	blue_reset	do { \
				_BIS (P1OUT, 0x80); \
				mdelay (20); \
				_BIC (P1OUT, 0x80); \
			} while (0)
