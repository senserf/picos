#ifndef	__pg_ualeds_h
#define	__pg_ualeds_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#if UART_USE_LEDS
#define	LEDIU(a,b)	do { \
				if ((UART_USE_LEDS & (1 << (a)))) \
					leds (a, b); \
			} while (0)
#else
#define	LEDIU(a,b)	CNOP
#endif

#endif
