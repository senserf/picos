#ifndef __pg_irq_cc430_rf_h
#define __pg_irq_cc430_rf_h

// ============================================================================
// CC430-related parameters needed by the driver and the interrupt service
// routine
// ============================================================================

#ifndef	RADIO_WOR_MODE
// For compatibility with the old driver
#define	RADIO_WOR_MODE	0
#endif

//+++ "irq_cc430_rf.c"

#define	RX_FIFO_READY		(RF1AIN & 0x01)

#define	ini_regs		CNOP

#define	IRQ_RCPT		(1 << 0)	// Reception on GDO0
#define	IRQ_RXTM		(1 << 1)	// !Chip ready on GDO1

#define rcv_enable_int		do { \
					RF1AIE = IRQ_RCPT; \
					if (RX_FIFO_READY) \
						_BIS (RF1AIFG, IRQ_RCPT); \
				} while (0)

#define	chip_not_ready		(RF1AIN & IRQ_RXTM)

#if RADIO_WOR_MODE

#define	IRQ_PQTR		(1 << 11)	// PQT reached

#define	erx_enable_int		RF1AIE = (IRQ_PQTR | IRQ_RXTM)

extern byte 			cc1100_worstate;

// Needed by the interrupt service routine
byte cc1100_strobe (byte);

#define	IRQ_EVT0		(1 << 14)	// WOR EVENT0

#define	wor_enable_int		RF1AIE =  IRQ_EVT0

#endif	/* RADIO_WOR_MODE */

#endif
