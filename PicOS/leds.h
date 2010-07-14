#ifndef	__pg_leds_h
#define	__pg_leds_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "leds_sys.h"

#if LEDS_DRIVER

#include "board_leds.h"

#endif

// ============================================================================

#ifndef	LED0_ON
#define	LED0_ON		CNOP
#define	LED0_OFF	CNOP
#define	led0_on		CNOP
#define	led0_off	CNOP
#define	led0_blk	CNOP
#define	LED0_MASK_BIT	0
#else
#define	led0_on		do { LED0_ON ; led_unblink (0x1); } while (0)
#define	led0_off	do { LED0_OFF; led_unblink (0x1); } while (0)
#define	led0_blk	do { LED0_ON ; led_blink   (0x1); } while (0)
#define	LED0_MASK_BIT	1
#endif

// ============================================================================

#ifndef	LED1_ON
#define	LED1_ON		CNOP
#define	LED1_OFF	CNOP
#define	led1_on		CNOP
#define	led1_off	CNOP
#define	led1_blk	CNOP
#define	LED1_MASK_BIT	0
#else
#define	led1_on		do { LED1_ON ; led_unblink (0x2); } while (0)
#define	led1_off	do { LED1_OFF; led_unblink (0x2); } while (0)
#define	led1_blk	do { LED1_ON ; led_blink   (0x2); } while (0)
#define	LED1_MASK_BIT	2
#endif

// ============================================================================

#ifndef	LED2_ON
#define	LED2_ON		CNOP
#define	LED2_OFF	CNOP
#define	led2_on		CNOP
#define	led2_off	CNOP
#define	led2_blk	CNOP
#define	LED2_MASK_BIT	0
#else
#define	led2_on		do { LED2_ON ; led_unblink (0x4); } while (0)
#define	led2_off	do { LED2_OFF; led_unblink (0x4); } while (0)
#define	led2_blk	do { LED2_ON ; led_blink   (0x4); } while (0)
#define	LED2_MASK_BIT	4
#endif

// ============================================================================

#ifndef	LED3_ON
#define	LED3_ON		CNOP
#define	LED3_OFF	CNOP
#define	led3_on		CNOP
#define	led3_off	CNOP
#define	led3_blk	CNOP
#define	LED3_MASK_BIT	0
#else
#define	led3_on		do { LED3_ON ; led_unblink (0x8); } while (0)
#define	led3_off	do { LED3_OFF; led_unblink (0x8); } while (0)
#define	led3_blk	do { LED3_ON ; led_blink   (0x8); } while (0)
#define	LED3_MASK_BIT	8
#endif

// ============================================================================

#ifndef	LEDS_ON
#define	leds_on		do { LED0_ON;  LED1_ON;  LED2_ON;  LED3_ON; \
				led_unblink (0xF); \
			} while (0)
#define	leds_off	do { LED0_OFF; LED1_OFF; LED2_OFF; LED3_OFF; \
				led_unblink (0xF); \
			} while (0)
#define	leds_blk	do { LED0_ON;  LED1_ON;  LED2_ON;  LED3_ON; \
				led_blink ( \
					LED0_MASK_BIT | \
					LED1_MASK_BIT | \
					LED2_MASK_BIT | \
					LED3_MASK_BIT ); } while (0)
#else
#define	leds_on		do { LEDS_ON ; led_unblink (0xF); } while (0)
#define	leds_off	do { LEDS_OFF; led_unblink (0xF); } while (0)
#define	leds_blk	do { LEDS_ON ; led_blink   ( \
				LED0_MASK_BIT | \
				LED1_MASK_BIT | \
				LED2_MASK_BIT | \
				LED3_MASK_BIT ); } while (0)
#endif

// ============================================================================

#ifndef	LEDS_SAVE
#define	leds_save(a,b)		CNOP
#define	leds_restore(a,b)	CNOP
#else
#define	leds_save(a,b)	do { \
				LEDS_SAVE (a); \
				(b) = __pi_systat.ledsts; \
				leds_off; \
			} while (0)
#define	leds_restore(a,b) do { \
				LEDS_RESTORE (a); \
				__pi_systat.ledsts = (b); \
				if ((b)) \
					TCI_RUN_AUXILIARY_TIMER; \
			  } while (0)
#endif

// ============================================================================

#define	led_blink(a)	__pi_systat.ledsts |= (a)
#define	led_unblink(a)	__pi_systat.ledsts &= ~(a)

#endif
