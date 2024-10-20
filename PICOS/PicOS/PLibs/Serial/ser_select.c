/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"

#if	UART_DRIVER > 1

int __serial_port;	// UART_A == 0, this is the default

int ser_select (int port) {

	int ret;

	ret = __serial_port;
	__serial_port = port ? UART_B : UART_A;
	return ret;
}

#endif
