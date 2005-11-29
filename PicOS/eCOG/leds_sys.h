#ifndef	__pg_leds_sys_h
#define	__pg_leds_sys_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	ZZ_LEDS(b)	(rg.io.gp0_3_out |= (b))

#define	LED0_ON		do { } while (0)
#define	LED1_ON		do { } while (0)
#define	LED2_ON		do { } while (0)
#define	LED3_ON		do { } while (0)
#define	LED0_OFF	do { } while (0)
#define	LED1_OFF	do { } while (0)
#define	LED2_OFF	do { } while (0)
#define	LED3_OFF	do { } while (0)

#if	TARGET_BOARD == BOARD_CYAN

#undef	LED0_ON
#undef	LED1_ON
#undef	LED2_ON
#undef	LED3_ON
#undef	LED0_OFF
#undef	LED1_OFF
#undef	LED2_OFF
#undef	LED3_OFF

#define	LED0_ON		ZZ_LEDS (0x0002)
#define	LED1_ON		ZZ_LEDS (0x0020)
#define	LED2_ON		ZZ_LEDS (0x0200)
#define	LED3_ON		ZZ_LEDS (0x2000)

#define	LED0_OFF	ZZ_LEDS (0x0001)
#define	LED1_OFF	ZZ_LEDS (0x0010)
#define	LED2_OFF	ZZ_LEDS (0x0100)
#define	LED3_OFF	ZZ_LEDS (0x1000)

#endif	/* TARGET_BOARD == BOARD_CYAN */

#endif
