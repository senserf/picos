/*
 * EEPROM pins for the Genesis board
 */

/*
 * Pin assignment (M9xxxx <-> MSP430-148/DM2300)
 *
 *  M95xxx		       GENESIS
 *  =================================
 *  S				P5.0 STE1
 *  D				P5.1 SIMO1	DATA ->
 *  Q				P5.2 SOMI1	DATA <-
 *  C				P5.3 UCLK1
 */

#include "board_pins.h"
#include "storage_m95xxx.h"

#define	EE_NBLOCKS	256	// 8K

#ifndef	EE_USE_UART
#define	EE_USE_UART	0
#endif

#if	UART_DRIVER > 1 || UART_TCV > 1
#error	"Cannot use two UARTs with SPI (needed for EEPROM)"
#endif


#if EE_USE_UART

#define	SPI_BAUD_RATE	4	// 8MHz/4 = 2MHz

#define	ee_ini_regs	do { \
				_BIS (P5OUT, 0x01); \
				_BIS (P5DIR, 0x01); \
				_BIS (P5SEL, 0x0E); \
			} while (0)

#define	ee_ini_spi	do { \
				_BIS (UCTL1, SWRST); \
				_BIS (UCTL1, SYNC + MM + CHAR); \
				UTCTL1 = CKPH + SSEL_SMCLK + STC; \
				UBR01 = SPI_BAUD_RATE; \
				UBR11 = 0; \
				UMCTL1 = 0; \
				_BIS (ME2, USPIE1); \
				_BIC (UCTL1, SWRST); \
			} while (0)

#define	ee_rx_ready	(IFG2 & URXIFG1)
#define	ee_tx_ready	(IFG2 & UTXIFG1)

#define	ee_get		U1RXBUF
#define	ee_put(a)	U1TXBUF = (a)

#else	/* EE_USE_UART */

#define	ee_ini_regs	do { \
				_BIS (P5OUT, 0x01); \
				_BIS (P5DIR, 0x0b); \
				_BIS (P5OUT, 0x01); \
			} while (0)

#define	ee_inp		(P5IN & 0x04)

#define	ee_outh		_BIS (P5OUT, 0x02)
#define	ee_outl		_BIC (P5OUT, 0x02)

#define	ee_clkh		_BIS (P5OUT, 0x08)
#define	ee_clkl		_BIC (P5OUT, 0x08)

#define	ee_ini_spi	do { } while (0)

#endif	/* EE_USE_UART */

#define	ee_start	_BIC (P5OUT, 0x01)
#define	ee_stop		_BIS (P5OUT, 0x01)
