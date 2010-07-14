/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "uart.h"

#ifdef	N_UARTS_TCV

interrupt (UART_A_TX_VECTOR) uart0tx_int (void) {

#define	UA		__pi_uart
#define	XBUF_STORE(a)	uart_a_write (a)

#include "irq_uart_x.h"

	RTNI;
#undef UA
#undef XBUF_STORE
}

interrupt (UART_A_RX_VECTOR) uart0rx_int (void) {

#define	UA	__pi_uart
#define	RBUF	uart_a_read

#include "irq_uart_r.h"

	RTNI;
#undef UA
#undef RBUF
}

#if N_UARTS_TCV > 1

interrupt (UART_B_TX_VECTOR) uart1tx_int (void) {

#define	UA		(__pi_uart + 1)
#define	XBUF_STORE(a)	uart_b_write (a)

#include "irq_uart_x.h"

	RTNI;
#undef UA
#undef XBUF_STORE
}

interrupt (UART_B_RX_VECTOR) uart1rx_int (void) {

#define	UA	(__pi_uart + 1)
#define	RBUF	uart_b_read

#include "irq_uart_r.h"

	RTNI;
#undef UA
#undef RBUF
}

#endif 	/* N_UARTS_TCV > 1 */

#endif
