#ifndef	__pg_uart_sys_h
#define	__pg_uart_sys_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2017                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// This is for UART over TCV/VNETI
//

#if UART_TCV
#define	N_UARTS_TCV	UART_TCV

#define	UART_START_XMITTER	do { \
					UA->x_istate = IRQ_X_STRT; \
					IntPendSet (INT_UART0_COMB); \
				} while (0)

#define	UART_STOP_XMITTER	do { \
					UA->x_istate = IRQ_X_OFF; \
				} while (0)

#define	UART_START_RECEIVER	do { \
					UA->r_istate = IRQ_R_STRT; \
					IntPendSet (INT_UART0_COMB); \
				} while (0)

#define	UART_STOP_RECEIVER	do { \
					UA->r_istate = IRQ_R_OFF; \
				} while (0)

// ============================================================================
#ifdef	uart_a_xmitter_on

// Half duplex operations (RS 485)

#define	UART_XMITTER_ON		uart_a_xmitter_on
#define	UART_XMITTER_OFF	uart_a_xmitter_off

#ifndef	uart_a_xmitter_on_delay
#define	uart_a_xmitter_on_delay		0
#endif

#ifndef	uart_a_xmitter_off_delay
#define	uart_a_xmitter_off_delay	0
#endif

#define	UART_XMITTER_ON_DELAY		uart_a_xmitter_on_delay
#define	UART_XMITTER_OFF_DELAY		uart_a_xmitter_off_delay

#endif	/* Half duplex */
// ============================================================================

#define	UART_RCV_RUNNING	(UA->r_istate > IRQ_R_STRT)

#endif	/* UART_TCV */

#endif
