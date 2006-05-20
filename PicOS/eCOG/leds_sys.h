#ifndef	__pg_leds_sys_h
#define	__pg_leds_sys_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	ZZ_LEDS03(b)	(rg.io.gp0_3_out |= (b))
#define	ZZ_LEDS47(b)	(rg.io.gp4_7_out |= (b))

#define	LED0_ON		do { } while (0)
#define	LED1_ON		do { } while (0)
#define	LED2_ON		do { } while (0)
#define	LED3_ON		do { } while (0)
#define	LED0_OFF	do { } while (0)
#define	LED1_OFF	do { } while (0)
#define	LED2_OFF	do { } while (0)
#define	LED3_OFF	do { } while (0)

#if	TARGET_BOARD == BOARD_CYAN || TARGET_BOARD == BOARD_GEORGE

#undef	LED0_ON
#undef	LED1_ON
#undef	LED2_ON
#undef	LED3_ON
#undef	LED0_OFF
#undef	LED1_OFF
#undef	LED2_OFF
#undef	LED3_OFF

#if 	TARGET_BOARD == BOARD_CYAN

/* Negative polarity (as on the CYAN board) */

#define	LED0_ON		ZZ_LEDS03 (0x0002)
#define	LED1_ON		ZZ_LEDS03 (0x0020)
#define	LED2_ON		ZZ_LEDS03 (0x0200)
#define	LED3_ON		ZZ_LEDS03 (0x2000)

#define	LED0_OFF	ZZ_LEDS03 (0x0001)
#define	LED1_OFF	ZZ_LEDS03 (0x0010)
#define	LED2_OFF	ZZ_LEDS03 (0x0100)
#define	LED3_OFF	ZZ_LEDS03 (0x1000)

#define	leds_enable	( rg.io.gp0_3_out = \
			  IO_GP0_3_OUT_EN0_MASK | IO_GP0_3_OUT_SET0_MASK | \
			  IO_GP0_3_OUT_EN1_MASK | IO_GP0_3_OUT_SET1_MASK | \
			  IO_GP0_3_OUT_EN2_MASK | IO_GP0_3_OUT_SET2_MASK | \
			  IO_GP0_3_OUT_EN3_MASK | IO_GP0_3_OUT_SET3_MASK )
#else

/* Positive polarity, two leds only */

#define	LED0_ON		ZZ_LEDS47 (0x0001)
#define	LED1_ON		ZZ_LEDS03 (0x1000)
#define	LED2_ON		do { } while (0)
#define	LED3_ON		do { } while (0)

#define	LED0_OFF	ZZ_LEDS47 (0x0002)
#define	LED1_OFF	ZZ_LEDS03 (0x2000)
#define	LED2_OFF	do { } while (0)
#define	LED3_OFF	do { } while (0)

#define	leds_enable	do { \
			  rg.io.gp0_3_out = \
			    IO_GP0_3_OUT_EN3_MASK | IO_GP0_3_OUT_CLR3_MASK; \
			  rg.io.gp4_7_out = \
			    IO_GP4_7_OUT_EN4_MASK | IO_GP4_7_OUT_CLR4_MASK; \
			} while (0)
#endif	/* CYAN */

#endif	/* CYAN || GEORGE */

#endif
