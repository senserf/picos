/* ========================================================================== */
/*                               G E N E S I S                                */
/* ========================================================================== */

#undef	LED0_ON
#undef	LED1_ON
#undef	LED2_ON
#undef	LED0_OFF
#undef	LED1_OFF
#undef	LED2_OFF
#undef	leds_save
#undef	leds_off
#undef	leds_restore

#define	LED0_ON		ZZ_LEDON (P6, 0x02)
#define	LED1_ON		ZZ_LEDON (P6, 0x04)
#define	LED2_ON		ZZ_LEDON (P6, 0x08)

#define	LED0_OFF	ZZ_LEDOFF (P6, 0x02)
#define	LED1_OFF	ZZ_LEDOFF (P6, 0x04)
#define	LED2_OFF	ZZ_LEDOFF (P6, 0x08)

#define	leds_save()	(P6OUT & (0x02+0x04+0x08))
#define	leds_off()	ZZ_LEDOFF (P6, 0x02+0x04+0x08)
#define	leds_restore(w)	ZZ_LEDON (P6, (w) & (0x02+0x04+0x08))
