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

/* ========================================================================== */
/*                               D M 2 1 0 0                                  */
/* ========================================================================== */
#if	TARGET_BOARD == BOARD_DM2100

#define	LEDS_HIGH_ON	0

#undef	LED0_ON
#undef	LED1_ON
#undef	LED2_ON
#undef	LED0_OFF
#undef	LED1_OFF
#undef	LED2_OFF
#undef	leds_save
#undef	leds_off
#undef	leds_restore

#define	LED0_ON		ZZ_LEDON (P4, 2)
#define	LED1_ON		ZZ_LEDON (P4, 4)
#define	LED2_ON		ZZ_LEDON (P4, 8)

#define	LED0_OFF	ZZ_LEDOFF (P4, 2)
#define	LED1_OFF	ZZ_LEDOFF (P4, 4)
#define	LED2_OFF	ZZ_LEDOFF (P4, 8)

#define	leds_save()	(~(P4OUT & (2+4+8)))
#define	leds_off()	ZZ_LEDOFF (P4, 2+4+8)
#define	leds_restore(w)	ZZ_LEDON (P4, (w) & (2+4+8))

#endif	/* TARGET_BOARD == BOARD_DM2100 */

/* ========================================================================== */
/*                               G E N E S I S                                */
/* ========================================================================== */
#if	TARGET_BOARD == BOARD_GENESIS

#define	LEDS_HIGH_ON	1

#undef	LED0_ON
#undef	LED1_ON
#undef	LED2_ON
#undef	LED0_OFF
#undef	LED1_OFF
#undef	LED2_OFF
#undef	leds_save
#undef	leds_off
#undef	leds_restore

#define	LED0_ON		ZZ_LEDON (P6, 0x08)
#define	LED1_ON		ZZ_LEDON (P6, 0x10)
#define	LED2_ON		ZZ_LEDON (P6, 0x20)

#define	LED0_OFF	ZZ_LEDOFF (P6, 0x08)
#define	LED1_OFF	ZZ_LEDOFF (P6, 0x10)
#define	LED2_OFF	ZZ_LEDOFF (P6, 0x20)

#define	leds_save()	(P6OUT & (0x08+0x10+0x20))
#define	leds_off()	ZZ_LEDOFF (P6, 0x08+0x10+0x20)
#define	leds_restore(w)	ZZ_LEDON (P6, (w) & (0x08+0x10+0x20))

#endif	/* TARGET_BOARD == BOARD_GENESIS */

/* ========================================================================== */
/*                                 V E R S A 2                                */
/* ========================================================================== */
#if	TARGET_BOARD == BOARD_VERSA2

#define	LEDS_HIGH_ON	0

#undef	LED0_ON
#undef	LED1_ON
#undef	LED2_ON
#undef	LED0_OFF
#undef	LED1_OFF
#undef	LED2_OFF
#undef	leds_save
#undef	leds_off
#undef	leds_restore

#define	LED0_ON		ZZ_LEDON (P4, 2)
#define	LED1_ON		ZZ_LEDON (P4, 4)
#define	LED2_ON		ZZ_LEDON (P4, 8)

#define	LED0_OFF	ZZ_LEDOFF (P4, 2)
#define	LED1_OFF	ZZ_LEDOFF (P4, 4)
#define	LED2_OFF	ZZ_LEDOFF (P4, 8)

#define	leds_save()	(~(P4OUT & (2+4+8)))
#define	leds_off()	ZZ_LEDOFF (P4, 2+4+8)
#define	leds_restore(w)	ZZ_LEDON (P4, (w) & (2+4+8))

#endif	/* TARGET_BOARD == BOARD_VERSA2 */

/* ========================================================================== */
/*                        S F U    P R O T O T Y P E                          */
/* ========================================================================== */
#if	TARGET_BOARD == BOARD_SFU_PROTOTYPE

#define	LEDS_HIGH_ON	1

#undef	LED0_ON
#undef	LED1_ON
#undef	LED2_ON
#undef	LED0_OFF
#undef	LED1_OFF
#undef	LED2_OFF
#undef	leds_save
#undef	leds_off
#undef	leds_restore

#define	LED0_ON		ZZ_LEDON (P3, 0x01)
#define	LED1_ON		ZZ_LEDON (P3, 0x02)
#define	LED2_ON		ZZ_LEDON (P3, 0x04)

#define	LED0_OFF	ZZ_LEDOFF (P3, 0x01)
#define	LED1_OFF	ZZ_LEDOFF (P3, 0x02)
#define	LED2_OFF	ZZ_LEDOFF (P3, 0x04)

#define	leds_save()	(P3OUT & (0x01+0x02+0x04))
#define	leds_off()	ZZ_LEDOFF (P3, 0x01+0x02+0x04)
#define	leds_restore(w)	ZZ_LEDON (P6, (w) & (0x01+0x02+0x04))

#endif	/* TARGET_BOARD == BOARD_SFU_PROTOTYPE */

/* ========================================================================== */
/* ========================================================================== */
/* ========================================================================== */

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
