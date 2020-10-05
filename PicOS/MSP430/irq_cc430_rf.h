/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_irq_cc430_rf_h
#define __pg_irq_cc430_rf_h

// ============================================================================
// CC430-related parameters needed by the driver and the interrupt service
// routine
// ============================================================================

#define	CC1100_RX_FIFO_READY		(RF1AIN & 0x01)

#define	cc1100_ini_regs		CNOP

#define	chip_not_ready		(RF1AIN & IRQ_RXTM)

#define	IRQ_RCPT		(1 << 0)	// Reception on GDO0
#define	IRQ_RXTM		(1 << 1)	// !Chip ready on GDO1

// ============================================================================

#ifndef	CC1101_RAW_INTERFACE
// Skip this if using raw interface (no interrupts in that case)

#ifndef	RADIO_WOR_MODE
// For compatibility with the old driver
#define	RADIO_WOR_MODE	0
#endif

//+++ "irq_cc430_rf.c"
REQUEST_EXTERNAL (irq_cc430_rf);


#define cc1100_rcv_int_enable		do { \
					RF1AIE = IRQ_RCPT; \
					if (CC1100_RX_FIFO_READY) \
						_BIS (RF1AIFG, IRQ_RCPT); \
				} while (0)

#if RADIO_WOR_MODE

#define	IRQ_PQTR		(1 << 11)	// PQT reached

#define	erx_enable_int		RF1AIE = (IRQ_PQTR | IRQ_RXTM)

extern byte 			cc1100_worstate;

// Needed by the interrupt service routine
byte cc1100_strobe (byte);

#define	IRQ_EVT0		(1 << 14)	// WOR EVENT0

#define	wor_enable_int		RF1AIE =  IRQ_EVT0

#endif	/* RADIO_WOR_MODE */

#endif	/* CC1101_RAW_INTERFACE */

#endif
