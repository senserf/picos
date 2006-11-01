/* ========================================================================== */
/*                                 V E R S A 2                                */
/* ========================================================================== */

#undef	LEDS_HIGH_ON
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

