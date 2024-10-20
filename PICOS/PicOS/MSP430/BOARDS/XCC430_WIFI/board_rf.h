/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

//
// CC430 parameters are hardwired; this includes parameters for CC3000
//

#include "cc3000.h"

#define	CC3000_CLKH_DELAY	0
#define	CC3000_CLKL_DELAY	0
#define	CC3000_CSEL_DELAY	50
#define	CC3000_CUSE_DELAY	50
#define	CC3000_ENBL_DELAY	100
#define	CC3000_DSBL_DELAY	0

#if CC3000_CLKH_DELAY == 0
#define	__cc3000_clkh_del	CNOP
#else
#define	__cc3000_clkh_del	udelay (CC3000_CLKH_DELAY)
#endif

#if CC3000_CLKL_DELAY == 0
#define	__cc3000_clkl_del	CNOP
#else
#define	__cc3000_clkl_del	udelay (CC3000_CLKL_DELAY)
#endif

#if CC3000_CSEL_DELAY == 0
#define	__cc3000_csel_del	CNOP
#else
#define	__cc3000_csel_del	udelay (CC3000_CSEL_DELAY)
#endif

#if CC3000_CUSE_DELAY == 0
#define	__cc3000_cuse_del	CNOP
#else
#define	__cc3000_cuse_del	udelay (CC3000_CUSE_DELAY)
#endif

#if CC3000_ENBL_DELAY == 0
#define	__cc3000_enbl_del	CNOP
#else
#define	__cc3000_enbl_del	udelay (CC3000_ENBL_DELAY)
#endif

#if CC3000_DSBL_DELAY == 0
#define	__cc3000_dsbl_del	CNOP
#else
#define	__cc3000_dsbl_del	udelay (CC3000_DSBL_DELAY)
#endif

#if CC3000_USE_SPI == 0
// ============================================================================
// Direct pin control
// ============================================================================

#define	cc3000_reginit	_BIS (P1IES, 0x02)

#define	cc3000_clkh	do { _BIS (P1OUT, 0x10); __cc3000_clkh_del; } while (0)
#define	cc3000_clkl	do { _BIC (P1OUT, 0x10); __cc3000_clkl_del; } while (0)

#define	cc3000_outh	_BIS (P1OUT, 0x08)
#define	cc3000_outl	_BIC (P1OUT, 0x08)

#define	cc3000_inp	(P1IN & 0x04)

#else 

// ============================================================================
// SPI
// ============================================================================

#ifndef	CC3000_SPI_RATE
#define	CC3000_SPI_RATE		200000
#endif

#define	CC3000_BAUD_DIVIDER	(SMCLK_RATE / CC3000_SPI_RATE)

#define	cc3000_reginit		do { \
					_BIS (UCB0CTL1, UCSWRST); \
					_BIS (P1IES, 0x02); \
					UCB0CTL0 = UCMSB | UCMST; \
					UCB0CTL1 |= UCSSEL__SMCLK; \
					UCB0BR0 = CC3000_BAUD_DIVIDER; \
					_BIC (UCB0CTL1, UCSWRST); \
				} while (0)

#define	cc3000_tx_ready		(UCB0IFG & UCTXIFG)
#define	cc3000_rx_ready		(UCB0IFG & UCRXIFG)
#define	cc3000_xc_ready		((UCB0IFG & (UCRXIFG | UCTXIFG)) == \
					(UCRXIFG | UCTXIFG))

#define	cc3000_put(b)		UCB0TXBUF = (b)
#define	cc3000_get		UCB0RXBUF

#endif

// Try reducing the delay later
#define	cc3000_select	do { _BIC (P3OUT, 0x02); __cc3000_csel_del; } while (0)
#define	cc3000_unselect	do { _BIS (P3OUT, 0x02); __cc3000_cuse_del; } while (0)

#define	cc3000_irq_pending	((P1IN & 0x02) == 0)
#define	cc3000_irq_disable	_BIC (P1IE, 0x02)
#define	cc3000_irq_clear	_BIC (P1IFG, 0x02)

#define	cc3000_interrupt	(P1IFG & 0x02)

#define	cc3000_irq_enable	do { \
					_BIS (P1IE, 0x02); \
					if (cc3000_irq_pending) \
						_BIS (P1IFG, 0x02); \
				} while (0)

#define	cc3000_enable	do { _BIS (P3OUT, 0x01); __cc3000_enbl_del; } while (0)

// Do we need to make it more complicated, e.g., pull down CS to save power?
#define	cc3000_disable	do { __cc3000_dsbl_del; _BIC (P3OUT, 0x01); } while (0)
