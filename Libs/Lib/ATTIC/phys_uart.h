#ifndef	__pg_phys_uart_h
#define	__pg_phys_uart_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "phys_uart.c"

#define	UART_DEF_BUF_LEN	256	/* Default buffer length */

/*
 * These values are merely defaults changeable with tcv_control
 */
#define	UART_DEF_MNRCVINT	2	/* Minimum inter-receive delay */
#define	UART_DEF_MXRCVINT	16	/* Maximum inter-receive delay */
#define	UART_DEF_RCVTMT		10	/* Soft receive timeout */
#define	UART_DEF_MNBACKOFF	8	/* Minimum backoff */
#define	UART_DEF_BSBACKOFF	0x1f	/* Randomized component */
#define	UART_DEF_TCVSENSE	2	/* Pre-Tx activity sense time */

#define	UART_CHAR_TIMEOUT	5	/* How long we wait for the next char */
#define	UART_PACKET_SPACE	6	/* After transmission */

#define	UART_PHYS_MODE_DIRECT	0
#define	UART_PHYS_MODE_EMU	1

void phys_uart (int, int, int, int);

#endif
