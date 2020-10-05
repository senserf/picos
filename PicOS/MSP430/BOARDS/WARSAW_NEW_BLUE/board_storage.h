/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

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

#define	STORAGE_AT45_TYPE	410	// Select the actual model used (D)

#include "pins.h"
#include "storage_at45xxx.h"

#ifndef	EE_USE_UART
#define	EE_USE_UART	0
#endif

#if EE_USE_UART

#define	SD_USE_UART	1

// ============================================================================
// SPI mode ===================================================================
// ============================================================================

#if	UART_TCV > 1
#error	"S: Cannot use two UARTs with SPI (needed for EEPROM)"
#endif

// Note: second UART is not pre-inited for this board; we will try to use
// it together with SPI

#ifndef	SPI_BAUD_RATE
#define	SPI_BAUD_RATE	15	// the fastest
#endif

//
// Note: with SPI_BAUD_RATE == 2, we get 1/2 of the 6MHz SMCLK, which is
// 3 mbits/sec, i.e., 375000 bytes/sec
//

// This is the same for both EE and SD
#define	spi_up		do { \
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

// When switching off power, we have to set those pins to high Z
#define	spi_down	_BIC (P5SEL, 0x0E)

// Timing measurements:
//
// EE test erase/write/read 100000 bytes:
//
//	direct	34.08s
//	SPI 4	21.70s	(2MHz)
//	SPI 2	19.86s	(4MHz)
//
// Test: powering from a pin:
//
//	First, the lowest (straight and stable) voltage at which the chip passes
//	the EWR test is Vcc = 2.3V. 2.2V causes lots of misreads.
//
//	When the chip is powered from a pin, we see lots of ripples in the
//	voltage on the chip, which are deeper during writing. Nonetheless,
//	the minimum Vcc (on the micro) at which the chip seems to function
//	is 2.6V. With a large (100u) capacitor put across the chip's power
//	supply, we see that the constant drop is about 0.2-0.3V, which agrees
//	with the performance observed under direct supply. The ripples do not
//	seem relevant. The capacitor doesn't help to lower the minimum
//	operating voltage.
//
//  Power usage (at 3.3V):
//
//	EEPROM (version D): quiescent 23.5uA, PD mode < 6uA
//                          writing/erasing command (w test) 7.3mA
//                          reading 2.2mA
//
//      SD (depends on the card):
//	Verbatim (2G):      idle state 80uA, ready 15mA, active (R/W) 16-17mA
//	Lexar (2G):                    60uA,       18mA,              16-20mA
//
//  Note: a 4GB card didn't want to work (perhaps will try again some other
//  time); we should be OK with 2GB cards for a while
//
//  Note: SPI/direct has no impact on power consumption, including PD mode,
//

#define	ee_bring_up	do { \
				spi_up; \
				_BIS (P5OUT, 0x21); \
				_BIS (P5DIR, 0x21); \
				cswitch_on (CSWITCH_EE+CSWITCH_SD); \
				mdelay (10); \
			} while (0)

#define	ee_bring_down	do { \
				mdelay (10); \
				cswitch_off (CSWITCH_EE+CSWITCH_SD); \
				_BIC (P5DIR, 0x2F); \
				spi_down; \
			} while (0)

#define	ee_rx_ready	(IFG2 & URXIFG1)
#define	ee_tx_ready	(IFG2 & UTXIFG1)

#define	ee_get		U1RXBUF
#define	ee_put(a)	U1TXBUF = (a)

#define	ee_start	_BIC (P5OUT, 0x01)
#define	ee_stop		_BIS (P5OUT, 0x01)

// SD =========================================================================

#define	sd_bring_up	do { \
				spi_up; \
				_BIS (P5OUT, 0x21); \
				_BIS (P5DIR, 0x21); \
				cswitch_on (CSWITCH_SD+CSWITCH_EE); \
				mdelay (10); \
			} while (0)

#define	sd_bring_down	do { \
				mdelay (10); \
				cswitch_off (CSWITCH_SD+CSWITCH_EE); \
				_BIC (P5DIR, 0x2F); \
				spi_down; \
			} while (0)

#define	sd_rx_ready	ee_rx_ready
#define	sd_tx_ready	ee_tx_ready

#define	sd_get		ee_get
#define	sd_put(a)	ee_put (a)

#else	/* EE_USE_UART */

#define	SD_USE_UART	0

// ============================================================================
// Direct mode ================================================================
// ============================================================================

#define	ee_bring_up	do { \
				_BIS (P5OUT, 0x29); \
				_BIS (P5DIR, 0x2B); \
				cswitch_on (CSWITCH_EE+CSWITCH_SD); \
				mdelay (10); \
			} while (0)

#define	ee_bring_down	do { \
				mdelay (10); \
				cswitch_off (CSWITCH_EE+CSWITCH_SD); \
				_BIC (P5DIR, 0x2B); \
			} while (0)

#define	ee_inp		(P5IN & 0x04)

#define	ee_outh		_BIS (P5OUT, 0x02)
#define	ee_outl		_BIC (P5OUT, 0x02)

#define	ee_clkh		_BIS (P5OUT, 0x08)
#define	ee_clkl		_BIC (P5OUT, 0x08)

#define	ee_start	do { _BIC (P5OUT, 0x01); ee_clkl; } while (0)
#define	ee_stop		do { _BIS (P5OUT, 0x01); ee_clkh; } while (0)


// SD =========================================================================

#define	sd_bring_up	do { \
				_BIS (P5OUT, 0x2B); \
				_BIS (P5DIR, 0x2B); \
				cswitch_on (CSWITCH_EE+CSWITCH_SD); \
				mdelay (10); \
			} while (0)

#define	sd_bring_down	do { \
				mdelay (10); \
				cswitch_off (CSWITCH_EE+CSWITCH_SD); \
				_BIC (P5DIR, 0x2B); \
			} while (0)

#define	sd_clkh		ee_clkh
#define	sd_clkl		ee_clkl

#define	sd_outh		ee_outh
#define	sd_outl		ee_outl

#define	sd_inp		ee_inp

#endif /* EE_USE_UART */

// ============================================================================
// ============================================================================
// ============================================================================

// CS signal is different, though
#define	sd_start	_BIC (P5OUT, 0x20)
#define	sd_stop		_BIS (P5OUT, 0x20)
