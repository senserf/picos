
#undef	LED0_ON
#undef	LED1_ON
#undef	LED0_OFF
#undef	LED1_OFF
#undef	leds_save
#undef	leds_off
#undef	leds_restore

#define	LED0_ON		ZZ_LEDON (P3, 0x40)
#define	LED1_ON		ZZ_LEDON (P3, 0x80)

#define	LED0_OFF	ZZ_LEDOFF (P3, 0x40)
#define	LED1_OFF	ZZ_LEDOFF (P3, 0x80)

#define	leds_save()	(P3OUT & (0x40+0x80))
#define	leds_off()	ZZ_LEDOFF (P3, 0x40+0x80)
#define	leds_restore(w)	ZZ_LEDON (P3, (w) & (0x40+0x80))

#define	LEDS_HIGH_ON	0
