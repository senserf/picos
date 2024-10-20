/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_rfleds_h
#define	__pg_rfleds_h	1

#if RADIO_USE_LEDS
#define	LEDI(a,b)	do { \
				if ((RADIO_USE_LEDS & (1 << (a)))) \
					leds (a, b); \
			} while (0)
#else
#define	LEDI(a,b)	do { } while (0)
#endif

#endif
