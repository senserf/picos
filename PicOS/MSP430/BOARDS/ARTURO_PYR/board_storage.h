/*
 * Pin assignment (AT45XXX <-> MSP430-xxx)
 *
 *  AT45XXX		        MSP430
 *  =================================
 *  CS				P5.0 STE1
 *  SI				P5.1 SIMO1	DATA ->
 *  SO				P5.2 SOMI1	DATA <-
 *  SCK				P5.3 UCLK1
 */

#define	STORAGE_AT45_TYPE	410	// Select the actual model (variant 'D')
#define	EEPROM_PDMODE_AVAILABLE	1

#include "board_pins.h"
#include "storage_at45xxx.h"

#if EE_USE_UART

// ============================================================================
// SPI mode ===================================================================
// ============================================================================

#if	UART_DRIVER > 1 || UART_TCV > 1
#error	"Cannot use two UARTs with SPI (needed for EEPROM)"
#endif

#ifndef	SPI_BAUD_RATE
#define	SPI_BAUD_RATE	2	// the fastest
#endif

// This is the same for both EE and SD
#define	spi_init	do { \
				_BIS (P5SEL, 0x0E); \
				_BIS (UCTL1, SWRST); \
				_BIS (UCTL1, SYNC + MM + CHAR); \
				UTCTL1 = CKPL + SSEL_SMCLK + STC; \
				UBR01 = SPI_BAUD_RATE; \
				UBR11 = 0; \
				UMCTL1 = 0; \
				_BIS (ME2, USPIE1); \
				_BIC (UCTL1, SWRST); \
			} while (0)

#define	ee_bring_up	spi_init

#define	ee_rx_ready	(IFG2 & URXIFG1)
#define	ee_tx_ready	(IFG2 & UTXIFG1)

#define	ee_get		U1RXBUF
#define	ee_put(a)	U1TXBUF = (a)

#define	ee_start	_BIC (P5OUT, 0x01)
#define	ee_stop		_BIS (P5OUT, 0x01)

#else	/* EE_USE_UART */

// ============================================================================
// Direct mode ================================================================
// ============================================================================

// Preinitialized in board_pins.h
#define	ee_bring_up	CNOP

#define	ee_inp		(P5IN & 0x04)

#define	ee_outh		_BIS (P5OUT, 0x02)
#define	ee_outl		_BIC (P5OUT, 0x02)

#define	ee_clkh		_BIS (P5OUT, 0x08)
#define	ee_clkl		_BIC (P5OUT, 0x08)

#define	ee_start	do { _BIC (P5OUT, 0x01); ee_clkl; } while (0)
#define	ee_stop		do { _BIS (P5OUT, 0x01); ee_clkh; } while (0)

#endif /* EE_USE_UART */

#define	ee_bring_down	CNOP
