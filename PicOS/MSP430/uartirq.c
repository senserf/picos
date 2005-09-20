/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "uart.h"

#if UART_TCV

interrupt (UART0TX_VECTOR) uart0tx_int (void) {

#define	UA	zz_uart
#define	XBUF	TXBUF0

#include "irq_uart_x.h"

}

interrupt (UART0RX_VECTOR) uart0rx_int (void) {

#define	UA	zz_uart
#define	RBUF	RXBUF0

#include "irq_uart_r.h"

}

#if UART_TCV > 1

interrupt (UART1TX_VECTOR) uart1tx_int (void) {

#define	UA	(zz_uart + 1)
#define	XBUF	TXBUF1

#include "irq_uart_x.h"

}

interrupt (UART1RX_VECTOR) uart1rx_int (void) {

#define	UA	(zz_uart + 1)
#define	RBUF	RXBUF1

#include "irq_uart_r.h"

}

#endif 	/* UART_TCV > 1 */


#endif
