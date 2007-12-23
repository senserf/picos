#ifndef	__pg_leds_sys_h
#define	__pg_leds_sys_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	LED0_ON		do { } while (0)
#define	LED1_ON		do { } while (0)
#define	LED2_ON		do { } while (0)
#define	LED3_ON		do { } while (0)
#define	LED0_OFF	do { } while (0)
#define	LED1_OFF	do { } while (0)
#define	LED2_OFF	do { } while (0)
#define	LED3_OFF	do { } while (0)

#define	leds_save()	0
#define	leds_off()	do { } while (0)
#define	leds_restore(w)	do { } while (0)

#if	LEDS_DRIVER

#include "board_leds.h"

#ifndef	LEDS_HIGH_ON
#define	LEDS_HIGH_ON	1	// This is the default
#endif

#if	LEDS_HIGH_ON

#define	ZZ_LEDON(a,b)	do { \
				_BIS ( a ## OUT, (b)); \
				_BIS ( a ## DIR, (b)); \
			} while (0)

#define	ZZ_LEDOFF(a,b)	do { \
				_BIC ( a ## OUT, (b)); \
				_BIC ( a ## DIR, (b)); \
			} while (0)

#else

#define	ZZ_LEDON(a,b)	do { \
				_BIC ( a ## OUT, (b)); \
				_BIS ( a ## DIR, (b)); \
			} while (0)

#define	ZZ_LEDOFF(a,b)	do { \
				_BIS ( a ## OUT, (b)); \
				_BIC ( a ## DIR, (b)); \
			} while (0)
#endif	/* LEDS_HIGH_ON */

#endif	/* LEDS_DRIVER */

#endif
