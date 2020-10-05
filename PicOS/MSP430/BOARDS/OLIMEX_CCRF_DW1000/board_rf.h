/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

//
// This is for DW1000, CC430 parameters are hardwired
//

#if DW1000_USE_SPI == 0

//
// Direct pin control
//

// MOSI/CLK set to output (they are always parked low),
// CS set to output (parked low); make sure to set MOSI low
// before pulling CS down
#define	DW1000_SPI_START	do { \
					_BIS (P1DIR, 0x18); \
					_BIS (P3DIR, 0x04); \
				} while (0)
				
// CS set to input, pulled high, MOSI/CLK set to input
// (CLK is low at the end, MOSI parked low)				
#define	DW1000_SPI_STOP		do { \
					_BIC (P3DIR, 0x04); \
					_BIC (P1DIR, 0x18); \
					_BIC (P1OUT, 0x18); \
				} while (0)

#define	dw1000_sclk_up		_BIS (P1OUT, 0x10)
#define	dw1000_sclk_down	_BIC (P1OUT, 0x10)

#define	dw1000_so_val		(P1IN & 0x04)

#define	dw1000_si_up		_BIS (P1OUT, 0x08)
#define	dw1000_si_down		_BIC (P1OUT, 0x08)

#define	dw1000_spi_init		CNOP
#define	dw1000_spi_slow		CNOP
#define	dw1000_spi_fast		CNOP

#else

//
// Use USART SPI
//

#define	DW1000_SPI_START	do { \
					_BIS (P1DIR, 0x18); \
					_BIS (P1SEL, 0x1c); \
					_BIS (P3DIR, 0x04); \
				} while (0)

#define	DW1000_SPI_STOP		do { \
					_BIC (P3DIR, 0x04); \
					_BIC (P1DIR, 0x18); \
					_BIC (P1OUT, 0x18); \
					_BIC (P1SEL, 0x1c); \
				} while (0)

// This yields about 20MHz SPI rate, which seems to work just fine; however,
// it doesn't seem to shorten perceptibly the duty cycle, at least based on
// RST, which is still about 90ms
//
// The duration of the actual handshake is about 10ms; using USART SPI, i.e.,
// increasing the SPI rate from 500k to 20M, has only marginal impact on that
// time; it seems I can reduce FIN_DELAY from 4000 * 250 to 3000 * 250 causing
// a 1ms reduction in handshake duration (to ca. 9ms)
//
#define	DW1000_SPI_RATE_DIVIDER_FAST		1
#define	DW1000_SPI_RATE_DIVIDER_SLOW		8

#define	dw1000_spi_init		do { \
				    _BIS (UCB0CTL1, UCSWRST); \
				    UCB0CTL0 = UCMSB | UCMST | UCCKPH; \
				    UCB0CTL1 |= UCSSEL__SMCLK; \
				    UCB0BR0 = DW1000_SPI_RATE_DIVIDER_SLOW; \
				    _BIC (UCB0CTL1, UCSWRST); \
				} while (0)

// Assumes SPI initialized, just rate change
#define	dw1000_spi_fast		do { \
				    _BIS (UCB0CTL1, UCSWRST); \
				    UCB0BR0 = DW1000_SPI_RATE_DIVIDER_FAST; \
				    _BIC (UCB0CTL1, UCSWRST); \
				} while (0)

#define	dw1000_spi_slow		do { \
				    _BIS (UCB0CTL1, UCSWRST); \
				    UCB0BR0 = DW1000_SPI_RATE_DIVIDER_SLOW; \
				    _BIC (UCB0CTL1, UCSWRST); \
				} while (0)

#define	dw1000_tx_ready		(UCB0IFG & UCTXIFG)
#define	dw1000_rx_ready		(UCB0IFG & UCRXIFG)
#define	dw1000_xc_ready		((UCB0IFG & (UCRXIFG | UCTXIFG)) == \
					(UCRXIFG | UCTXIFG))

#define	dw1000_put(b)		UCB0TXBUF = (b)
#define	dw1000_get		UCB0RXBUF

#endif	/* SPI mode select */

#define	dw1000_int		(P2IFG & 0x04)
#define	dw1000_clear_int	_BIC (P2IFG, 0x04)
#define	DW1000_INT_PENDING	(P2IN & 0x04)

#define	dw1000_int_enable	do { \
					_BIS (P2IE, 0x04); \
					if (DW1000_INT_PENDING) \
						_BIS (P2IFG, 0x04); \
				} while (0)

#define	dw1000_int_disable	_BIC (P2IE, 0x04)

// The delay is probably redundant, the manual says that 10ns is enough
#define	DW1000_RESET		do { \
					_BIS (P3DIR, 0x01); \
					udelay (10); \
					_BIC (P3DIR, 0x01); \
				} while (0)

// Used after wakeup from lp to assess whether the chip is ready to go
#define	dw1000_ready		(P3IN & 0x01)

//+++ "p2irq.c"
REQUEST_EXTERNAL (p2irq);
