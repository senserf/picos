/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_uart_def_h
#define	__pg_uart_def_h		1

#include "mach.h"


#define uart_a_char_available		((HWREG (UART0_BASE + UART_O_FR) & \
						UART_FR_RXFE) == 0)

#define uart_a_room_in_tx		((HWREG (UART0_BASE + UART_O_FR) & \
						UART_FR_TXFF) == 0)

#define	uart_a_read			((byte) HWREG (UART0_BASE + UART_O_DR))

#define	uart_a_write(b)			HWREG (UART0_BASE + UART_O_DR) = \
						(byte)(b)

#define	uart_a_disable_int		IntDisable (INT_UART0_COMB)
#define	uart_a_enable_int		IntEnable (INT_UART0_COMB)

#define	uart_a_clear_interrupts		UARTIntClear (UART0_BASE, \
						UART_INT_OE | \
						UART_INT_BE | \
						UART_INT_PE | \
						UART_INT_FE | \
						UART_INT_RT | \
						UART_INT_TX | \
						UART_INT_RX | \
						UART_INT_CTS)
#endif
