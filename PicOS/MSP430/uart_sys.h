#ifndef	__pg_uart_sys_h
#define	__pg_uart_sys_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// This is for UART over TCV/VNETI
//

#if UART_TCV
#define	N_UARTS_TCV	UART_TCV
#endif

#ifdef N_UARTS_TCV

//+++ "uartirq.c"
#ifdef	UART_A_TX_RX_VECTOR
REQUEST_EXTERNAL (uart0xx_int);
#else
REQUEST_EXTERNAL (uart0tx_int);
#endif

#if N_UARTS_TCV > 1
// ----------------------------------------------------------------------------

#define	UART_START_XMITTER	do { \
					UA->x_istate = IRQ_X_STRT; \
					if (UA == __pi_uart) { \
						uart_a_set_write_int; \
						uart_a_enable_write_int; \
					} else { \
						uart_b_set_write_int; \
						uart_b_enable_write_int; \
					} \
				} while (0)

#define	UART_STOP_XMITTER	do { \
					if (UA == __pi_uart) \
						uart_a_disable_write_int; \
					else \
						uart_b_disable_write_int; \
					UA->x_istate = IRQ_X_OFF; \
				} while (0)

#define	UART_START_RECEIVER	do { \
					UA->r_istate = IRQ_R_STRT; \
					if (UA == __pi_uart) \
						uart_a_enable_read_int; \
					else \
						uart_b_enable_read_int; \
				} while (0)

#define	UART_STOP_RECEIVER	do { \
					if (UA == __pi_uart) \
						uart_a_disable_read_int; \
					else \
						uart_b_disable_read_int; \
					UA->r_istate = IRQ_R_OFF; \
				} while (0)

#if defined(uart_a_xmitter_on) || defined(uart_b_xmitter_on)
// Half-duplex operations (RS485) on at least one UART

#ifndef uart_a_xmitter_on
#define	uart_a_xmitter_on		CNOP
#define	uart_a_xmitter_off		CNOP
#endif

#ifndef	uart_a_xmitter_on_delay
#define	uart_a_xmitter_on_delay		0
#endif

#ifndef	uart_a_xmitter_off_delay
#define	uart_a_xmitter_off_delay	0
#endif

#ifndef uart_b_xmitter_on
#define	uart_b_xmitter_on		CNOP
#define	uart_b_xmitter_off		CNOP
#endif

#ifndef	uart_b_xmitter_on_delay
#define	uart_b_xmitter_on_delay		0
#endif

#ifndef	uart_b_xmitter_off_delay
#define	uart_b_xmitter_off_delay	0
#endif

#define	UART_XMITTER_ON		do { \
					if (UA == __pi_uart) \
						uart_a_xmitter_on; \
					else \
						uart_b_xmitter_on; \
				} while (0)

#define	UART_XMITTER_OFF	do { \
					if (UA == __pi_uart) \
						uart_a_xmitter_off; \
					else \
						uart_b_xmitter_off; \
				} while (0)

#define	UART_XMITTER_ON_DELAY	((UA == __pi_uart) ? \
					uart_a_xmitter_on_delay : \
					uart_b_xmitter_on_delay)
					
#define	UART_XMITTER_OFF_DELAY	((UA == __pi_uart) ? \
					uart_a_xmitter_off_delay : \
					uart_b_xmitter_off_delay)
					

#endif /* Half duplex operations */

#else /* single UART */

// ----------------------------------------------------------------------------

#define	UART_START_XMITTER	do { \
					UA->x_istate = IRQ_X_STRT; \
					uart_a_set_write_int; \
					uart_a_enable_write_int; \
				} while (0)

#define	UART_STOP_XMITTER	do { \
					uart_a_disable_write_int; \
					UA->x_istate = IRQ_X_OFF; \
				} while (0)

#define	UART_START_RECEIVER	do { \
					UA->r_istate = IRQ_R_STRT; \
					uart_a_enable_read_int; \
				} while (0)

#define	UART_STOP_RECEIVER	do { \
					uart_a_disable_read_int; \
					UA->r_istate = IRQ_R_OFF; \
				} while (0)

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

#endif	/* N_UARTS_TCV > 1 */

#define	UART_RCV_RUNNING	(UA->r_istate > IRQ_R_STRT)

#endif	/* defined N_UARTS_TCV */

#endif
