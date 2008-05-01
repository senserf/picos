/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "uart.h"

#ifdef	N_UARTS_TCV

interrupt (UART_A_TX_VECTOR) uart0tx_int (void) {

#define	UA	zz_uart
#define	XBUF	TXBUF_A

#include "irq_uart_x.h"

	RTNI;
#undef UA
#undef XBUF
}

interrupt (UART_A_RX_VECTOR) uart0rx_int (void) {

#define	UA	zz_uart
#define	RBUF	RXBUF_A

#include "irq_uart_r.h"

	RTNI;
#undef UA
#undef RBUF
}

#if N_UARTS_TCV > 1

interrupt (UART_B_TX_VECTOR) uart1tx_int (void) {

#define	UA	(zz_uart + 1)
#define	XBUF	TXBUF_B

#include "irq_uart_x.h"

	RTNI;
#undef UA
#undef XBUF
}

interrupt (UART_B_RX_VECTOR) uart1rx_int (void) {

#define	UA	(zz_uart + 1)
#define	RBUF	RXBUF_B

#include "irq_uart_r.h"

	RTNI;
#undef UA
#undef RBUF
}

#endif 	/* N_UARTS_TCV > 1 */

#endif
