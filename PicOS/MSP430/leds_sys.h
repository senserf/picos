#ifndef	__pg_leds_sys_h
#define	__pg_leds_sys_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
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

#if	TARGET_BOARD == BOARD_DM2100

#define	LEDS_HIGH_ON	0

#undef	LED0_ON
#undef	LED1_ON
#undef	LED2_ON
#undef	LED0_OFF
#undef	LED1_OFF
#undef	LED2_OFF

#define	LED0_ON		ZZ_LEDON (P4, 2)
#define	LED1_ON		ZZ_LEDON (P4, 4)
#define	LED2_ON		ZZ_LEDON (P4, 8)

#define	LED0_OFF	ZZ_LEDOFF (P4, 2)
#define	LED1_OFF	ZZ_LEDOFF (P4, 4)
#define	LED2_OFF	ZZ_LEDOFF (P4, 8)

#endif	/* TARGET_BOARD == BOARD_DM2100 */

#if	TARGET_BOARD == BOARD_GENESIS

#define	LEDS_HIGH_ON	1

#undef	LED0_ON
#undef	LED1_ON
#undef	LED2_ON
#undef	LED0_OFF
#undef	LED1_OFF
#undef	LED2_OFF

#define	LED0_ON		ZZ_LEDON (P6, 2)
#define	LED1_ON		ZZ_LEDON (P6, 4)
#define	LED2_ON		ZZ_LEDON (P6, 8)

#define	LED0_OFF	ZZ_LEDOFF (P6, 2)
#define	LED1_OFF	ZZ_LEDOFF (P6, 4)
#define	LED2_OFF	ZZ_LEDOFF (P6, 8)

#endif	/* TARGET_BOARD == BOARD_GENESIS */

#if	TARGET_BOARD == BOARD_VERSA2

#define	LEDS_HIGH_ON	0

#undef	LED0_ON
#undef	LED1_ON
#undef	LED2_ON
#undef	LED0_OFF
#undef	LED1_OFF
#undef	LED2_OFF

#define	LED0_ON		ZZ_LEDON (P4, 2)
#define	LED1_ON		ZZ_LEDON (P4, 4)
#define	LED2_ON		ZZ_LEDON (P4, 8)

#define	LED0_OFF	ZZ_LEDOFF (P4, 2)
#define	LED1_OFF	ZZ_LEDOFF (P4, 4)
#define	LED2_OFF	ZZ_LEDOFF (P4, 8)

#endif	/* TARGET_BOARD == BOARD_VERSA2 */

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

#endif
