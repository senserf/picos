#ifndef	__pg_rfleds_h
#define	__pg_rfleds_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#if RADIO_USE_LEDS
#define	LEDI(a,b)	do { \
				if ((RADIO_USE_LEDS & (1 << (a)))) \
					leds (a, b); \
			} while (0)
#else
#define	LEDI(a,b)	do { } while (0)
#endif

#endif