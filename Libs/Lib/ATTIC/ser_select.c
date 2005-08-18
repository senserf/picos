/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

#define THE_UART UART_A

int __serial_port = THE_UART;

int ser_select (int port) {

	int ret;

	ret = __serial_port;
	__serial_port = port ? UART_B : UART_A;
	return ret;
}
