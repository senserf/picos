/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_ser_select_h
#define	__pg_ser_select_h

#include "sysio.h"

#if UART_DRIVER > 1
int ser_select (int);
#else
#define	ser_select(a)	CNOP
#endif

#endif


