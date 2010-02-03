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

#if N_UARTS_TCV > 1
// ----------------------------------------------------------------------------

#define	UART_START_XMITTER	do { \
					UA->x_istate = IRQ_X_STRT; \
					if (UA == zz_uart) { \
						uart_a_set_write_int; \
						uart_a_enable_write_int; \
					} else { \
						uart_b_set_write_int; \
						uart_b_enable_write_int; \
					} \
				} while (0)

#define	UART_STOP_XMITTER	do { \
					if (UA == zz_uart) \
						uart_a_disable_write_int; \
					else \
						uart_b_disable_write_int; \
					UA->x_istate = IRQ_X_OFF; \
				} while (0)

#define	UART_START_RECEIVER	do { \
					UA->r_istate = IRQ_R_STRT; \
					if (UA == zz_uart) \
						uart_a_enable_read_int; \
					else \
						uart_b_enable_read_int; \
				} while (0)

#define	UART_STOP_RECEIVER	do { \
					if (UA == zz_uart) \
						uart_a_disable_read_int; \
					else \
						uart_b_disable_read_int; \
					UA->r_istate = IRQ_R_OFF; \
				} while (0)

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

#endif	/* N_UARTS_TCV > 1 */

#endif	/* defined N_UARTS_TCV */

#endif
