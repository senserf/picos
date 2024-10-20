/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_ualeds_h
#define	__pg_ualeds_h	1

#if UART_USE_LEDS
#define	LEDIU(a,b)	do { \
				if ((UART_USE_LEDS & (1 << (a)))) \
					leds (a, b); \
			} while (0)
#else
#define	LEDIU(a,b)	CNOP
#endif

#endif
