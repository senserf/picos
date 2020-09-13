#ifndef __pg_ser_select_h
#define	__pg_ser_select_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2020                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

#if UART_DRIVER > 1
int ser_select (int);
#else
#define	ser_select(a)	CNOP
#endif

#endif


