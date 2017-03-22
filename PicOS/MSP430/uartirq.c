/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "uart.h"

#ifdef	N_UARTS_TCV

#define	UA		__pi_uart
#define	XBUF_STORE(a)	uart_a_write (a)
#define	RBUF		uart_a_read

#ifdef	UART_A_TX_RX_VECTOR

// A single vector for both RX and TX (as in CC430)

interrupt (UART_A_TX_RX_VECTOR) uart0xx_int (void) {

	if (uart_a_tx_interrupt) {

#include "irq_uart_x.h"

	} else {

#include "irq_uart_r.h"

	}

	RTNI;
}

#else 

// Two separate vectors

interrupt (UART_A_TX_VECTOR) uart0tx_int (void) {

#include "irq_uart_x.h"

	RTNI;
}

interrupt (UART_A_RX_VECTOR) uart0rx_int (void) {

#include "irq_uart_r.h"

	RTNI;
}

#endif	/* One or two vectors */

#undef UA
#undef XBUF_STORE
#undef RBUF

// ============================================================================

#if N_UARTS_TCV > 1

// Two UARTS (two separate vectors)

#define	UA		(__pi_uart + 1)
#define	XBUF_STORE(a)	uart_b_write (a)
#define	RBUF		uart_b_read

interrupt (UART_B_TX_VECTOR) uart1tx_int (void) {

#include "irq_uart_x.h"

	RTNI;

}

interrupt (UART_B_RX_VECTOR) uart1rx_int (void) {

#include "irq_uart_r.h"

	RTNI;

}

#undef UA
#undef XBUF_STORE
#undef RBUF

#endif 	/* N_UARTS_TCV > 1 */

#endif
